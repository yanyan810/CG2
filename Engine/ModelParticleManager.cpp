#include "ModelParticleManager.h"
#include <algorithm>
#include "Object3dCommon.h"
#include "Matrix4x4.h"
#define M_PI 3.141592653589793

std::mt19937 rng(std::random_device{}());

int Rand(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

float Rand(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

Vector2 Rand(const Vector2& min, const Vector2& max)
{
    return {
        Rand(min.x, max.x),
        Rand(min.y, max.y)
    };
}

Vector3 Rand(const Vector3& min, const Vector3& max) {
    return {
        Rand(min.x, max.x),
        Rand(min.y, max.y),
        Rand(min.z, max.z)
    };
}

Vector4 Rand(const Vector4& min, const Vector4& max) {
    return {
        Rand(min.x, max.x),
        Rand(min.y, max.y),
        Rand(min.z, max.z),
        Rand(min.w, max.w),
    };
}

Vector3 RandomUnitVector() {
    float theta = Rand(0.0f, 2.0f * float(M_PI));   // 0〜2π の角度
    float phi = acosf(Rand(-1.0f, 1.0f));           // -1〜1を使ってφを決定

    Vector3 dir;
    dir.x = sinf(phi) * cosf(theta);
    dir.y = sinf(phi) * sinf(theta);
    dir.z = cosf(phi);
    return dir; // すでに正規化済み
}

Vector4 Lerp(const Vector4& start, const Vector4& end, float t)
{
    // 線形補間
    Vector4 result = start + (end - start) * t;
    // 補間結果を返す
    return result;
}

ModelParticleManager* ModelParticleManager::GetInstance()
{
    static ModelParticleManager instance;
    return &instance;
}

void ModelParticleManager::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager) {
    dxCommon_ = dxCommon;
    srvManager_ = srvManager;

    // 1. モデルの取得（ModelManagerを使用）
    ModelManager::GetInstance()->LoadModel("triangleParticle.obj");
    model_ = ModelManager::GetInstance()->FindModel("triangleParticle.obj");

    // 2. インスタンシング用リソースの作成
    instancingResource_ = dxCommon_->CreateUAVBufferResource(sizeof(ModelParticleTransformationMatrix) * kMaxInstance, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    
    // 2. UAVの作成 (Compute Shaderで書き込むため)
    uavIndexRenderData_ = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(
        uavIndexRenderData_,
        instancingResource_.Get(),
        kMaxInstance,
        sizeof(ModelParticleTransformationMatrix)
    );
    
    // 3. SRVの作成 (SrvManagerを活用！)
    srvIndex_ = srvManager_->Allocate(); // 空き番号を自動取得
    srvManager_->CreateSRVforStructuredBuffer(
        srvIndex_,
        instancingResource_.Get(),
        kMaxInstance,
        sizeof(ModelParticleTransformationMatrix)
    );

    // マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
    materialResource_ = dxCommon_->CreateBufferResource(sizeof(Material));
    // マテリアルにデータを書き込む
    materialData_ = nullptr;
    // 書き込むためのアドレスを取得
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    // 赤を書き込む
    materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    // ライティングを有効にする
    materialData_->enableLighting = true;
    materialData_->lightingMode = false;
    materialData_->uvTransform = Matrix4x4::MakeIdentity4x4();
    materialData_->shininess = 32.0f;

    // 平行光源用のリソースを作る
    directionalLightResource_ = dxCommon_->CreateBufferResource(sizeof(DirectionalLight));
    // マテリアルにデータを書き込む
    DirectionalLight* directionalLightData = nullptr;
    // 書き込むためのアドレスを取得
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
    // デフォルト値
    directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
    directionalLightData->intensity = 1.0f;

    // カメラ用 CBV を作成
    cameraResource_ = dxCommon_->CreateBufferResource(sizeof(CameraData));

    // マップ
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));

    // 初期値（とりあえず原点）
    cameraData_->worldPosition = { 0.0f, 0.0f, 0.0f };
    cameraData_->padding = 0.0f;
    

    // 1. 物理バッファの作成 (Default Heap)
    particleResource_ = dxCommon_->CreateUAVBufferResource(sizeof(ParticleGPU) * kMaxInstance, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    // 3. UAVの作成 (SrvManagerにUAV作成機能がある想定)
    uavIndexParticles_ = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(uavIndexParticles_, particleResource_.Get(), kMaxInstance, sizeof(ParticleGPU));

    uavIndexRenderData_ = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(uavIndexRenderData_, instancingResource_.Get(), kMaxInstance, sizeof(ModelParticleTransformationMatrix));

    computeConfigResource_ = dxCommon_->CreateBufferResource(sizeof(GlobalConfig));
    computeConfigResource_->Map(0, nullptr, reinterpret_cast<void**>(&computeConfigData_));

    // ComputeShader用のシーン定数バッファ (b1)
    computeSceneResource_ = dxCommon_->CreateBufferResource(sizeof(SceneConfig));
    computeSceneResource_->Map(0, nullptr, reinterpret_cast<void**>(&computeSceneData_));

    // Emit用の転送バッファ (1個分)
    emitStagingResource_ = dxCommon_->CreateBufferResource(sizeof(ParticleGPU) * kMaxInstance);

    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW; // DrawInstanced用

    D3D12_COMMAND_SIGNATURE_DESC sigDesc = {};
    sigDesc.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS);
    sigDesc.NumArgumentDescs = 1;
    sigDesc.pArgumentDescs = &argDesc;

    dxCommon_->GetDevice()->CreateCommandSignature(&sigDesc, nullptr, IID_PPV_ARGS(&commandSignature_));
    
    // 1. 本番用のUAVバッファ作成 (DEFAULTヒープ)
    drawArgsResource_ = dxCommon_->CreateUAVBufferResource(sizeof(D3D12_DRAW_ARGUMENTS), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    // 2. CPUから書き込むための中継用バッファ (UPLOADヒープ) を作成
    // CreateBufferResource を使うのが正解！
    Microsoft::WRL::ComPtr<ID3D12Resource> stagingArgs = dxCommon_->CreateBufferResource(sizeof(D3D12_DRAW_ARGUMENTS));

    // 3. 中継用バッファに値を Map して書き込む
    D3D12_DRAW_ARGUMENTS* mappedArgs = nullptr;
    stagingArgs->Map(0, nullptr, reinterpret_cast<void**>(&mappedArgs));
    mappedArgs->VertexCountPerInstance = static_cast<UINT>(model_->GetModelData().meshes[0].vertices.size());
    mappedArgs->InstanceCount = 0;
    mappedArgs->StartVertexLocation = 0;
    mappedArgs->StartInstanceLocation = 0;
    stagingArgs->Unmap(0, nullptr);

    // 4. GPUコマンドで UPLOAD -> DEFAULT へコピーを実行
    dxCommon_->GetCommandList()->CopyBufferRegion(drawArgsResource_.Get(), 0, stagingArgs.Get(), 0, sizeof(D3D12_DRAW_ARGUMENTS));

    // 5. コピー完了を待つためのバリア (COPY_DEST -> UNORDERED_ACCESS)
    auto initBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        drawArgsResource_.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    dxCommon_->GetCommandList()->ResourceBarrier(1, &initBarrier);

    // 1. AliveIndicesバッファの作成 (kMaxInstance分)
    aliveIndicesResource_ = dxCommon_->CreateUAVBufferResource(sizeof(uint32_t) * kMaxInstance, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    uavIndexAliveIndices_ = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(uavIndexAliveIndices_, aliveIndicesResource_.Get(), kMaxInstance, sizeof(uint32_t));

    // 2. DrawArgsのUAV登録 (CSのu3にセットするため)
    uavIndexDrawArgs_ = srvManager_->Allocate();
    // ※RWByteAddressBufferとして扱う場合は CreateUAVforRawBuffer 等の関数が必要になります
    srvManager_->CreateUAVforRawBuffer(uavIndexDrawArgs_, drawArgsResource_.Get());

    // 3. カウンターリセット用リソース (中身が常に0の4バイトバッファ)
    resetResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t));
    uint32_t* mappedReset = nullptr;
    resetResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedReset));
    *mappedReset = 0;
    resetResource_->Unmap(0, nullptr);
    
    dxCommon_->ExecuteCommandListAndWait();

}

void ModelParticleManager::RegisterEffect(const std::string& effectName, const std::string& jsonPath)
{
    ParticleEmitterConfig config;
    LoadFromJson(jsonPath, config);
    effectLibrary_[effectName] = config;
}

void ModelParticleManager::Emit(const std::string& effectName, const Vector3& position, uint32_t count) {
    // 登録されているか確認
    if (effectLibrary_.find(effectName) == effectLibrary_.end()) {
        return; // 見つからなければ何もしない
    }

    // 設定を取り出し、座標をセット
    ParticleEmitterConfig& config = effectLibrary_[effectName];
    config.position = position;

    // 今回のエミット用のリストを作成
    std::vector<Particle> particles;
    particles.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        particles.push_back(MakeParticle(config));
    }

    // まとめてGPU転送
    EmitBatch(particles);
    auto& conf = effectLibrary_[effectName];
    char buf[256];
    sprintf_s(buf, "Emit: %s at (%.2f, %.2f, %.2f) Scale: %.2f ColorA: %.2f\n",
        effectName.c_str(), position.x, position.y, position.z, conf.startScale, conf.startColor.w);
    OutputDebugStringA(buf);
}

void ModelParticleManager::EmitBatch(const std::vector<Particle>& particles) {
    if (particles.empty()) return;

    // 今回発生させる数（念のためバッファを突き抜けないように制限）
    size_t count = std::min(particles.size(), (size_t)1000);
    if (freeIndex_ + count >= kMaxInstance) freeIndex_ = 0; // 簡易的なラップアラウンド処理

    std::vector<ParticleGPU> uploadData(count);
    for (size_t i = 0; i < count; ++i) {
        uploadData[i].position = particles[i].transform.translate;
        uploadData[i].velocity = particles[i].velocity;
        uploadData[i].acceleration = particles[i].acceleration;
        uploadData[i].angularVelocity = particles[i].angularVelocity;
        uploadData[i].currentTime = 0.0f;
        uploadData[i].lifeTime = particles[i].lifeTime;
        uploadData[i].startScale = particles[i].startScale;
        uploadData[i].endScale = particles[i].endScale;
        uploadData[i].startColor = particles[i].startColor;
        uploadData[i].endColor = particles[i].endColor;
        uploadData[i].rotate = particles[i].transform.rotate;
        uploadData[i].isActive = 1;
    }

    // ★ 修正：Stagingバッファの「freeIndex_」番目の位置に書き込む
    void* mappedPtr = nullptr;
    emitStagingResource_->Map(0, nullptr, &mappedPtr);

    // 書き込み先のアドレスを計算
    ParticleGPU* dest = static_cast<ParticleGPU*>(mappedPtr) + freeIndex_;
    memcpy(dest, uploadData.data(), sizeof(ParticleGPU) * count);

    emitStagingResource_->Unmap(0, nullptr);

    // ★ 修正：コピー命令も「freeIndex_」から開始するように指定
    dxCommon_->GetCommandList()->CopyBufferRegion(
        particleResource_.Get(), freeIndex_ * sizeof(ParticleGPU), // コピー先
        emitStagingResource_.Get(), freeIndex_ * sizeof(ParticleGPU), // コピー元もずらす！
        count * sizeof(ParticleGPU)
    );

    freeIndex_ += (uint32_t)count;
}

ModelParticleManager::Particle ModelParticleManager::MakeParticle(const ParticleEmitterConfig& config) {
    Particle particle;

    // 初期座標を少しバラけさせる
    particle.transform.translate = Rand(config.position - Vector3(0.1f, 0.1f, 0.1f), config.position + Vector3(0.1f, 0.1f, 0.1f));

    // 初速度と加速度
    Vector3 dir = RandomUnitVector();
    particle.velocity = dir * Rand(config.speedMin, config.speedMax);
    particle.acceleration = config.gravity;

    // 回転速度（氷などで重要）
    particle.angularVelocity = Rand(Vector3(-5.0f, -5.0f, -5.0f), Vector3(5.0f, 5.0f, 5.0f));

    // 色（ベースカラーに対してランダムな揺らぎを与える）
    Vector4 colorVariation = Rand(Vector4(-0.1f, -0.1f, -0.1f, 0.0f), Vector4(0.1f, 0.1f, 0.1f, 0.0f));
    particle.color = config.startColor + colorVariation;

    // 寿命とスケール設定
    particle.lifeTime = Rand(config.lifeTimeMin, config.lifeTimeMax);
    particle.currentTime = 0.0f;

    // Particle構造体にこれらの変数を保持しておく必要があります
    particle.startScale = config.startScale;
    particle.endScale = config.endScale;

    particle.startColor = config.startColor;
    particle.endColor = config.endColor;

    return particle;
}

void ModelParticleManager::Dispatch(float deltaTime, Camera* camera)
{
    auto commandList = dxCommon_->GetCommandList();
    
    // 1. カウンター(InstanceCount)をリセット (4バイト目に0をコピー)
    commandList->CopyBufferRegion(drawArgsResource_.Get(), 4, resetResource_.Get(), 0, 4);

    // バリアを張ってリセット完了を待つ
    auto preBarrier = CD3DX12_RESOURCE_BARRIER::Transition(drawArgsResource_.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(1, &preBarrier);

    auto& psoCS = dxCommon_->GetPSOComputeParticle();

    // 1. 定数バッファの更新
    computeConfigData_->deltaTime = deltaTime;
    computeConfigData_->maxParticles = kMaxInstance;
    computeSceneData_->viewProjection = camera->GetViewMatrix() * camera->GetProjectionMatrix();
    
    // ★ ここが重要！ SRV/UAV管理用のディスクリプタヒープをセットする
    ID3D12DescriptorHeap* ppHeaps[] = { srvManager_->GetDescriptorHeap() };
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // 2. Compute Pipeline の設定
    commandList->SetComputeRootSignature(psoCS.root_.GetSignature().Get());
    commandList->SetPipelineState(psoCS.computeState_.Get());

    // 3. ルートパラメータのセット (4と5を追加！)
    commandList->SetComputeRootConstantBufferView(0, computeConfigResource_->GetGPUVirtualAddress());
    commandList->SetComputeRootConstantBufferView(1, computeSceneResource_->GetGPUVirtualAddress());
    commandList->SetComputeRootDescriptorTable(2, srvManager_->GetGPUDescriptionHandle(uavIndexParticles_));
    commandList->SetComputeRootDescriptorTable(3, srvManager_->GetGPUDescriptionHandle(uavIndexRenderData_));
    commandList->SetComputeRootDescriptorTable(4, srvManager_->GetGPUDescriptionHandle(uavIndexAliveIndices_));
    commandList->SetComputeRootDescriptorTable(5, srvManager_->GetGPUDescriptionHandle(uavIndexDrawArgs_));

    commandList->Dispatch((kMaxInstance + 1023) / 1024, 1, 1);

    // 4. 描画前にバリアを張る (RenderDataとDrawArgsの両方)
    D3D12_RESOURCE_BARRIER barriers[2];
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(instancingResource_.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(drawArgsResource_.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    commandList->ResourceBarrier(2, barriers);
}

void ModelParticleManager::Draw() {

    auto commandList = dxCommon_->GetCommandList();

    // 1. シグネチャとPSOの設定
    commandList->SetGraphicsRootSignature(dxCommon_->GetPSOModelParticle().root_.GetSignature().Get());
    commandList->SetPipelineState(dxCommon_->GetPSOModelParticle().graphicsState_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 2. 頂点バッファの設定
    commandList->IASetVertexBuffers(0, 1, &model_->GetVBV());

    // --- ここからルートパラメータのセット (InitalizeForModelParticleの順番に合わせる) ---

    // Index 0: Material (b0) - CBV
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    // Index 1: DirectionalLight (b1) - CBV
    commandList->SetGraphicsRootConstantBufferView(1, directionalLightResource_->GetGPUVirtualAddress());

    // Index 2: Camera (b2) - CBV
    commandList->SetGraphicsRootConstantBufferView(2, cameraResource_->GetGPUVirtualAddress());

    // Index 3: Instancing Buffer (t1) - DescriptorTable
    // SrvManagerからGPUハンドルを取得してセットします
    commandList->SetGraphicsRootDescriptorTable(3, srvManager_->GetGPUDescriptionHandle(srvIndex_));

    // Index 4: Texture (t0) - DescriptorTable
	commandList->SetGraphicsRootDescriptorTable(4, TextureManager::GetInstance()->GetSrvHandleGPU(model_->GetModelData().materials[0].textureFilePath)); // 0番目のマテリアルのSRVをセット
    
    commandList->ExecuteIndirect(
        commandSignature_.Get(),
        1,                          // 実行するコマンド数
        drawArgsResource_.Get(),    // 引数バッファ
        0,                          // オフセット
        nullptr, 0                  // カウントバッファ（今回は未使用）
    );
    
    // 終了バリア (INDIRECT_ARGUMENT から戻す)
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(drawArgsResource_.Get(),
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->ResourceBarrier(1, &barrier);
}

void ModelParticleManager::UpdateImGui(const std::string& effectName, ParticleEmitterConfig& editingConfig) {
    ImGui::Begin("Particle Editor");

    ImGui::Text("Editing: %s", effectName.c_str());

    // 値が変更されたかどうかをチェックするフラグ
    bool changed = false;

    changed |= ImGui::ColorEdit4("StartColor", &editingConfig.startColor.x);
    changed |= ImGui::ColorEdit4("EndColor", &editingConfig.endColor.x);
    changed |= ImGui::DragFloat("Speed Min", &editingConfig.speedMin, 0.01f);
    changed |= ImGui::DragFloat("Speed Max", &editingConfig.speedMax, 0.01f);
    changed |= ImGui::DragFloat3("Gravity", &editingConfig.gravity.x, 0.01f);
    changed |= ImGui::DragFloat("Start Scale", &editingConfig.startScale, 0.01f);
    changed |= ImGui::DragFloat("End Scale", &editingConfig.endScale, 0.01f);

    // ★ リアルタイム反映：値が変わったら即座に Library を上書き
    if (changed) {
        effectLibrary_[effectName] = editingConfig;
    }

    ImGui::Separator();

    // 保存用のファイル名（デフォルトは登録時のファイル名など）
    static char filename[64] = "";
    if (filename[0] == '\0') sprintf_s(filename, "%s.json", effectName.c_str());

    ImGui::InputText("Save Filename", filename, IM_ARRAYSIZE(filename));

    if (ImGui::Button("Save to JSON")) {
        SaveToJson(filename, editingConfig);
        // 保存時にも確実に最新を反映
        effectLibrary_[effectName] = editingConfig;
    }

    if (ImGui::Button("Load from JSON")) {
        LoadFromJson(filename, editingConfig);
        // ロードしたら Library も更新
        effectLibrary_[effectName] = editingConfig;
    }

    ImGui::End();
}

void ModelParticleManager::SaveToJson(const std::string& path, const ParticleEmitterConfig& config)
{
    std::ofstream file("resources/particle/" + path);
    if (file.is_open()) {
        nlohmann::json j = config.ToJson();
        file << std::setw(4) << j << std::endl; // 見やすく整形して保存
    }
}

void ModelParticleManager::LoadFromJson(const std::string& path, ParticleEmitterConfig& config)
{
    std::ifstream file("resources/particle/" + path);
    if (file.is_open()) {
        nlohmann::json j;
        file >> j;
        config.FromJson(j);
    }
}
