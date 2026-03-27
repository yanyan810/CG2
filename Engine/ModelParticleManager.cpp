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

    // 用のModelParticleTransformationMatrix用のリソースを作る。Matrix4x41つ分のサイズを用意する
    transformationMatrixResource = dxCommon_->CreateBufferResource(sizeof(ModelParticleTransformationMatrix));
    // 書き込むためのアドレスを取得
    transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
    // 単位行列を書き込んでおく
    transformationMatrixData->WVP = Matrix4x4::MakeIdentity4x4();
    transformationMatrixData->World = Matrix4x4::MakeIdentity4x4();

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
    directionalLightResource = dxCommon_->CreateBufferResource(sizeof(DirectionalLight));
    // マテリアルにデータを書き込む
    DirectionalLight* directionalLightData = nullptr;
    // 書き込むためのアドレスを取得
    directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
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
    emitStagingResource_ = dxCommon_->CreateBufferResource(sizeof(ParticleGPU) * 1000);

}

void ModelParticleManager::Emit(const Particle& particle) {
    // 1. すでに作成済みの emitStagingResource_ に書き込む
    ParticleGPU* mappedData = nullptr;
    emitStagingResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
    
    mappedData->position = particle.transform.translate;
    mappedData->velocity = particle.velocity;
    mappedData->acceleration = particle.acceleration;
    mappedData->angularVelocity = particle.angularVelocity;
    mappedData->currentTime = 0.0f;
    mappedData->lifeTime = particle.lifeTime;
    mappedData->startScale = particle.startScale;
    mappedData->endScale = particle.endScale;
    mappedData->startColor = particle.startColor;
    mappedData->endColor = particle.endColor;
    mappedData->rotate = particle.transform.rotate;
    mappedData->isActive = 1;

    emitStagingResource_->Unmap(0, nullptr);

    // 2. GPU上の StructuredBuffer の特定のインデックスへコピー
    dxCommon_->GetCommandList()->CopyBufferRegion(
        particleResource_.Get(), freeIndex_ * sizeof(ParticleGPU),
        emitStagingResource_.Get(), 0, sizeof(ParticleGPU)
    );

    freeIndex_ = (freeIndex_ + 1) % kMaxInstance;
}

void ModelParticleManager::EmitBatch(const std::vector<Particle>& particles) {
    if (particles.empty()) return;

    size_t count = std::min(particles.size(), (size_t)1000); // 一度の転送制限

    // 1. Stagingバッファを配列としてマップ
    ParticleGPU* mappedData = nullptr;
    emitStagingResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));

    for (size_t i = 0; i < count; ++i) {
        mappedData[i].position = particles[i].transform.translate;
        mappedData[i].velocity = particles[i].velocity;
        mappedData[i].acceleration = particles[i].acceleration;
        mappedData[i].angularVelocity = particles[i].angularVelocity;
        mappedData[i].currentTime = 0.0f;
        mappedData[i].lifeTime = particles[i].lifeTime;
        mappedData[i].startScale = particles[i].startScale;
        mappedData[i].endScale = particles[i].endScale;
        mappedData[i].startColor = particles[i].startColor;
        mappedData[i].endColor = particles[i].endColor;
        mappedData[i].rotate = particles[i].transform.rotate;
        mappedData[i].isActive = 1;
    }
    emitStagingResource_->Unmap(0, nullptr);

    // 2. まとめてコピー
    dxCommon_->GetCommandList()->CopyBufferRegion(
        particleResource_.Get(), freeIndex_ * sizeof(ParticleGPU),
        emitStagingResource_.Get(), 0, count * sizeof(ParticleGPU)
    );

    freeIndex_ = (freeIndex_ + count) % kMaxInstance;
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

    // 3. ルートパラメータのセット (Shaderのregister番号に合わせる)
    // b0: GlobalConfig
    commandList->SetComputeRootConstantBufferView(0, computeConfigResource_->GetGPUVirtualAddress());
    // b1: SceneConfig
    commandList->SetComputeRootConstantBufferView(1, computeSceneResource_->GetGPUVirtualAddress());
    // u0: ParticleBuffer
    commandList->SetComputeRootDescriptorTable(2, srvManager_->GetGPUDescriptionHandle(uavIndexParticles_));
    // u1: RenderDataBuffer
    commandList->SetComputeRootDescriptorTable(3, srvManager_->GetGPUDescriptionHandle(uavIndexRenderData_));

    commandList->Dispatch((kMaxInstance + 1023) / 1024, 1, 1);

    // 4. 描画前にバリアを張る
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        instancingResource_.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);
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
    commandList->SetGraphicsRootConstantBufferView(1, directionalLightResource->GetGPUVirtualAddress());

    // Index 2: Camera (b2) - CBV
    commandList->SetGraphicsRootConstantBufferView(2, cameraResource_->GetGPUVirtualAddress());

    // Index 3: Instancing Buffer (t1) - DescriptorTable
    // SrvManagerからGPUハンドルを取得してセットします
    commandList->SetGraphicsRootDescriptorTable(3, srvManager_->GetGPUDescriptionHandle(srvIndex_));

    // Index 4: Texture (t0) - DescriptorTable
	commandList->SetGraphicsRootDescriptorTable(4, TextureManager::GetInstance()->GetSrvHandleGPU(model_->GetModelData().materials[0].textureFilePath)); // 0番目のマテリアルのSRVをセット
    
    uint32_t drawCount = std::min(freeIndex_ + 1000, kMaxInstance);
    commandList->DrawInstanced(
        static_cast<UINT>(model_->GetModelData().meshes[0].vertices.size()),
        drawCount,
        0, 0
    );
    
    // 描画が終わったら、次のフレームの計算のために UAV 状態に戻しておく
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        instancingResource_.Get(),
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(1, &barrier);
}

void ModelParticleManager::UpdateImGui(ParticleEmitterConfig& editingConfig) {
    ImGui::Begin("Particle Editor");

    // プレビュー用の設定
    ImGui::Text("Base Settings");
    ImGui::ColorEdit4("StartColor", &editingConfig.startColor.x);
    ImGui::ColorEdit4("EndColor", &editingConfig.endColor.x);
    ImGui::DragFloat("Speed Min", &editingConfig.speedMin, 0.01f);
    ImGui::DragFloat("Speed Max", &editingConfig.speedMax, 0.01f);
    ImGui::DragFloat3("Gravity", &editingConfig.gravity.x, 0.01f);

    ImGui::Separator();

    ImGui::Text("Scale Easing");
    ImGui::DragFloat("Start Scale", &editingConfig.startScale, 0.01f);
    ImGui::DragFloat("End Scale", &editingConfig.endScale, 0.01f);

    static char filename[64] = "fire_particle.json";
    ImGui::InputText("Save Filename", filename, IM_ARRAYSIZE(filename));

    if (ImGui::Button("Save to JSON")) {
        SaveToJson(filename, editingConfig);
    }

    if (ImGui::Button("Load from JSON")) {
        LoadFromJson(filename, editingConfig);
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
