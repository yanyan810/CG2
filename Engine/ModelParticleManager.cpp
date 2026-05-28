#include "ModelParticleManager.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include "Object3dCommon.h"
#include "Matrix4x4.h"
#include "TextureManager.h"
#define M_PI 3.141592653589793

std::mt19937 rng(std::random_device{}());

namespace {
    void TransitionResource(
        ID3D12GraphicsCommandList* commandList,
        ID3D12Resource* resource,
        D3D12_RESOURCE_STATES& currentState,
        D3D12_RESOURCE_STATES nextState)
    {
        if (!resource || currentState == nextState) {
            return;
        }
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, currentState, nextState);
        commandList->ResourceBarrier(1, &barrier);
        currentState = nextState;
    }
}

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

namespace {
float Clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

Vector4 ClampColor(const Vector4& color) {
    return {
        Clamp01(color.x),
        Clamp01(color.y),
        Clamp01(color.z),
        Clamp01(color.w)
    };
}

float RandomizedScale(float baseScale, float randomRange) {
    return std::max(0.0f, baseScale + Rand(-randomRange, randomRange));
}

std::string NormalizeModelPath_(std::string path)
{
    std::replace(path.begin(), path.end(), '\\', '/');
    constexpr const char* resourcesPrefix = "resources/";
    if (path.rfind(resourcesPrefix, 0) == 0) {
        path = path.substr(std::strlen(resourcesPrefix));
    }
    return path;
}
}

ModelParticleManager* ModelParticleManager::GetInstance()
{
    static ModelParticleManager instance;
    return &instance;
}

void ModelParticleManager::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, uint32_t maxInstances) {
    dxCommon_ = dxCommon;
    srvManager_ = srvManager;
    maxInstance_ = std::max(1u, maxInstances);
    instancingResourceState_ = D3D12_RESOURCE_STATE_COMMON;
    particleResourceState_ = D3D12_RESOURCE_STATE_COMMON;
    drawArgsResourceState_ = D3D12_RESOURCE_STATE_COMMON;

    // 1. モデルの取得（ModelManagerを使用）
    ModelManager::GetInstance()->LoadModel("triangleParticle.obj");
    model_ = ModelManager::GetInstance()->FindModel("triangleParticle.obj");
    currentModelPath_ = "triangleParticle.obj";

    // 2. インスタンシング用リソースの作成
    instancingResource_ = dxCommon_->CreateUAVBufferResource(sizeof(ModelParticleTransformationMatrix) * maxInstance_, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    
    // 2. UAVの作成 (Compute Shaderで書き込むため)
    uavIndexRenderData_ = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(
        uavIndexRenderData_,
        instancingResource_.Get(),
        maxInstance_,
        sizeof(ModelParticleTransformationMatrix)
    );
    
    // 3. SRVの作成 (SrvManagerを活用！)
    srvIndex_ = srvManager_->Allocate(); // 空き番号を自動取得
    srvManager_->CreateSRVforStructuredBuffer(
        srvIndex_,
        instancingResource_.Get(),
        maxInstance_,
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
    particleResource_ = dxCommon_->CreateUAVBufferResource(sizeof(ParticleGPU) * maxInstance_, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    // 3. UAVの作成 (SrvManagerにUAV作成機能がある想定)
    uavIndexParticles_ = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(uavIndexParticles_, particleResource_.Get(), maxInstance_, sizeof(ParticleGPU));

    uavIndexRenderData_ = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(uavIndexRenderData_, instancingResource_.Get(), maxInstance_, sizeof(ModelParticleTransformationMatrix));

    computeConfigResource_ = dxCommon_->CreateBufferResource(sizeof(GlobalConfig));
    computeConfigResource_->Map(0, nullptr, reinterpret_cast<void**>(&computeConfigData_));

    // ComputeShader用のシーン定数バッファ (b1)
    computeSceneResource_ = dxCommon_->CreateBufferResource(sizeof(SceneConfig));
    computeSceneResource_->Map(0, nullptr, reinterpret_cast<void**>(&computeSceneData_));

    // Emit用の転送バッファ (1個分)
    emitStagingResource_ = dxCommon_->CreateBufferResource(sizeof(ParticleGPU) * maxInstance_);

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
    TransitionResource(
        dxCommon_->GetCommandList(),
        drawArgsResource_.Get(),
        drawArgsResourceState_,
        D3D12_RESOURCE_STATE_COPY_DEST);
    dxCommon_->GetCommandList()->CopyBufferRegion(drawArgsResource_.Get(), 0, stagingArgs.Get(), 0, sizeof(D3D12_DRAW_ARGUMENTS));

    // 5. コピー完了を待つためのバリア (COPY_DEST -> UNORDERED_ACCESS)
    TransitionResource(
        dxCommon_->GetCommandList(),
        drawArgsResource_.Get(),
        drawArgsResourceState_,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // 1. AliveIndicesバッファの作成
    aliveIndicesResource_ = dxCommon_->CreateUAVBufferResource(sizeof(uint32_t) * maxInstance_, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    uavIndexAliveIndices_ = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(uavIndexAliveIndices_, aliveIndicesResource_.Get(), maxInstance_, sizeof(uint32_t));

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

    // --- 新機能: エフェクトごとのモデル読み込み ---
    ModelManager::GetInstance()->LoadModel(config.modelPath);
    effectModels_[effectName] = ModelManager::GetInstance()->FindModel(config.modelPath);
    if (!config.texturePath.empty()) {
        TextureManager::GetInstance()->LoadTexture(config.texturePath);
    }
    ApplyRenderConfig_(config);
}

void ModelParticleManager::Emit(const std::string& effectName, const Vector3& position, uint32_t count) {
    // 登録されているか確認
    if (effectLibrary_.find(effectName) == effectLibrary_.end()) {
        // 未登録の場合はデフォルトのJSON名で自動登録を試みて継続する
        RegisterEffect(effectName, effectName + ".json");
    }

    // 設定を取り出し、座標をセット
    ParticleEmitterConfig& config = effectLibrary_[effectName];
    config.position = position;
    ApplyRenderConfig_(config);

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

void ModelParticleManager::Emit(const std::string& effectName, const Vector3& position, uint32_t count, const Vector4& color) {
    if (effectLibrary_.find(effectName) == effectLibrary_.end()) {
        RegisterEffect(effectName, effectName + ".json");
    }

    ParticleEmitterConfig config = effectLibrary_[effectName];
    config.position = position;
    config.startColor = color;
    config.endColor = { color.x, color.y, color.z, 0.0f };
    config.startColorRandom = { 0.05f, 0.05f, 0.05f, 0.0f };
    config.endColorRandom = { 0.04f, 0.04f, 0.04f, 0.0f };
    ApplyRenderConfig_(config);

    std::vector<Particle> particles;
    particles.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        particles.push_back(MakeParticle(config));
    }

    EmitBatch(particles);
}

void ModelParticleManager::EmitBatch(const std::vector<Particle>& particles) {
    if (particles.empty()) return;

    // 今回発生させる数（念のためバッファを突き抜けないように制限）
    size_t count = std::min({ particles.size(), static_cast<size_t>(1000), static_cast<size_t>(maxInstance_) });
    if (freeIndex_ + count >= maxInstance_) freeIndex_ = 0; // 簡易的なラップアラウンド処理

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
        uploadData[i].vortexAngularSpeed = particles[i].vortexAngularSpeed;
        uploadData[i].vortexRadialSpeed = particles[i].vortexRadialSpeed;
        uploadData[i].isActive = 1;
        // --- 新機能: イージングタイプとビルボードフラグを転送 ---
        uploadData[i].easingType = static_cast<uint32_t>(particles[i].easingType);
        uploadData[i].isBillboard = particles[i].isBillboard ? 1 : 0;
    }

    // ★ 修正：Stagingバッファの「freeIndex_」番目の位置に書き込む
    void* mappedPtr = nullptr;
    emitStagingResource_->Map(0, nullptr, &mappedPtr);

    // 書き込み先のアドレスを計算
    ParticleGPU* dest = static_cast<ParticleGPU*>(mappedPtr) + freeIndex_;
    memcpy(dest, uploadData.data(), sizeof(ParticleGPU) * count);

    emitStagingResource_->Unmap(0, nullptr);

    auto commandList = dxCommon_->GetCommandList();
    TransitionResource(
        commandList,
        particleResource_.Get(),
        particleResourceState_,
        D3D12_RESOURCE_STATE_COPY_DEST);

    // ★ 修正：コピー命令も「freeIndex_」から開始するように指定
    commandList->CopyBufferRegion(
        particleResource_.Get(), freeIndex_ * sizeof(ParticleGPU), // コピー先
        emitStagingResource_.Get(), freeIndex_ * sizeof(ParticleGPU), // コピー元もずらす！
        count * sizeof(ParticleGPU)
    );

    freeIndex_ += (uint32_t)count;
}

void ModelParticleManager::ClearParticles()
{
    if (!dxCommon_ || !particleResource_ || !emitStagingResource_) {
        freeIndex_ = 0;
        return;
    }

    const size_t clearSize = sizeof(ParticleGPU) * static_cast<size_t>(maxInstance_);
    void* mappedPtr = nullptr;
    emitStagingResource_->Map(0, nullptr, &mappedPtr);
    std::memset(mappedPtr, 0, clearSize);
    emitStagingResource_->Unmap(0, nullptr);

    auto commandList = dxCommon_->GetCommandList();
    TransitionResource(
        commandList,
        particleResource_.Get(),
        particleResourceState_,
        D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->CopyBufferRegion(particleResource_.Get(), 0, emitStagingResource_.Get(), 0, clearSize);

    freeIndex_ = 0;
}

ModelParticleManager::Particle ModelParticleManager::MakeParticle(const ParticleEmitterConfig& config) {
    Particle particle;

    // --- 新機能: エミッター形状に応じた初期座標の決定 ---
    switch (config.emitterShape) {
    case EmitterShape::Sphere: {
        // 球体内のランダム位置
        Vector3 dir = RandomUnitVector();
        float radius = Rand(0.0f, config.shapeSize.x);
        particle.transform.translate = config.position + dir * radius;
        break;
    }
    case EmitterShape::Box: {
        // ボックス内のランダム位置
        particle.transform.translate = Rand(
            config.position - config.shapeSize,
            config.position + config.shapeSize
        );
        break;
    }
    case EmitterShape::Point:
    default: {
        // 点発生（従来通り少しバラけさせる）
        particle.transform.translate = Rand(
            config.position - Vector3(0.1f, 0.1f, 0.1f),
            config.position + Vector3(0.1f, 0.1f, 0.1f)
        );
        break;
    }
    }

    // 初速度と加速度
    Vector3 dir = RandomUnitVector();
    particle.velocity = dir * Rand(config.speedMin, config.speedMax);
    particle.acceleration = config.gravity;

    // 初期回転と回転速度
    particle.transform.rotate = Rand(config.initialRotateMin, config.initialRotateMax);
    particle.angularVelocity = Rand(config.angularVelocityMin, config.angularVelocityMax);

    // 色（ベースカラーに対してランダムな揺らぎを与える）
    Vector4 startColorVariation = Rand(-config.startColorRandom, config.startColorRandom);
    Vector4 endColorVariation = Rand(-config.endColorRandom, config.endColorRandom);

    // 寿命とスケール設定
    particle.lifeTime = Rand(config.lifeTimeMin, config.lifeTimeMax);
    particle.currentTime = 0.0f;

    // Particle構造体にこれらの変数を保持しておく必要があります
    particle.startScale = RandomizedScale(config.startScale, config.startScaleRandom);
    particle.endScale = RandomizedScale(config.endScale, config.endScaleRandom);

    particle.startColor = ClampColor(config.startColor + startColorVariation);
    particle.endColor = ClampColor(config.endColor + endColorVariation);
    particle.color = particle.startColor;

    // --- 新機能: イージングタイプとビルボード ---
    particle.easingType = config.easingType;
    particle.isBillboard = config.isBillboard;

    return particle;
}

void ModelParticleManager::Dispatch(float deltaTime, Camera* camera)
{
    auto commandList = dxCommon_->GetCommandList();
    
    // 1. カウンター(InstanceCount)をリセット (4バイト目に0をコピー)
    TransitionResource(
        commandList,
        drawArgsResource_.Get(),
        drawArgsResourceState_,
        D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->CopyBufferRegion(drawArgsResource_.Get(), 4, resetResource_.Get(), 0, 4);

    TransitionResource(
        commandList,
        particleResource_.Get(),
        particleResourceState_,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    TransitionResource(
        commandList,
        instancingResource_.Get(),
        instancingResourceState_,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    TransitionResource(
        commandList,
        drawArgsResource_.Get(),
        drawArgsResourceState_,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    auto& psoCS = dxCommon_->GetPSOComputeParticle();

    // 1. 定数バッファの更新
    computeConfigData_->deltaTime = deltaTime;
    computeConfigData_->maxParticles = maxInstance_;
    computeSceneData_->viewProjection = camera->GetViewMatrix() * camera->GetProjectionMatrix();
    // --- 新機能: ビルボード用カメラ位置 ---
    computeSceneData_->cameraPosition = camera->GetTranslate();
    
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

    commandList->Dispatch((maxInstance_ + 1023) / 1024, 1, 1);

    TransitionResource(
        commandList,
        instancingResource_.Get(),
        instancingResourceState_,
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    TransitionResource(
        commandList,
        drawArgsResource_.Get(),
        drawArgsResourceState_,
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
}

void ModelParticleManager::ApplyRenderConfig_(const ParticleEmitterConfig& config)
{
    SetRenderModel_(config.modelPath.empty() ? "triangleParticle.obj" : config.modelPath);

    if (currentTexturePath_ != config.texturePath) {
        currentTexturePath_ = config.texturePath;
        if (!currentTexturePath_.empty()) {
            TextureManager::GetInstance()->LoadTexture(currentTexturePath_);
        }
    }

    currentUseJewelShader_ = config.useJewelShader;
    if (materialData_) {
        materialData_->enableLighting = currentUseJewelShader_ ? 1 : 0;
    }
}

void ModelParticleManager::SetRenderModel_(const std::string& modelPath)
{
    const std::string normalizedPath = NormalizeModelPath_(modelPath);
    if (normalizedPath.empty() || currentModelPath_ == normalizedPath) {
        return;
    }

    ModelManager::GetInstance()->LoadModel(normalizedPath);
    Model* nextModel = ModelManager::GetInstance()->FindModel(normalizedPath);
    if (!nextModel) {
        OutputDebugStringA(("[ModelParticle] model not found: " + normalizedPath + "\n").c_str());
        return;
    }

    model_ = nextModel;
    currentModelPath_ = normalizedPath;
    if (!model_->GetModelData().meshes.empty()) {
        UpdateDrawVertexCount_(static_cast<uint32_t>(model_->GetModelData().meshes[0].vertices.size()));
    }
}

void ModelParticleManager::UpdateDrawVertexCount_(uint32_t vertexCount)
{
    if (!dxCommon_ || !drawArgsResource_) {
        return;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> staging = dxCommon_->CreateBufferResource(sizeof(uint32_t));
    uint32_t* mapped = nullptr;
    staging->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
    *mapped = vertexCount;
    staging->Unmap(0, nullptr);

    TransitionResource(
        dxCommon_->GetCommandList(),
        drawArgsResource_.Get(),
        drawArgsResourceState_,
        D3D12_RESOURCE_STATE_COPY_DEST);
    dxCommon_->GetCommandList()->CopyBufferRegion(drawArgsResource_.Get(), 0, staging.Get(), 0, sizeof(uint32_t));
    TransitionResource(
        dxCommon_->GetCommandList(),
        drawArgsResource_.Get(),
        drawArgsResourceState_,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

std::string ModelParticleManager::ResolveRenderTexturePath_() const
{
    if (!currentTexturePath_.empty()) {
        return currentTexturePath_;
    }
    if (model_ && !model_->GetModelData().materials.empty()) {
        return model_->GetModelData().materials[0].textureFilePath;
    }
    return "";
}

void ModelParticleManager::Draw() {

    auto commandList = dxCommon_->GetCommandList();

    TransitionResource(
        commandList,
        instancingResource_.Get(),
        instancingResourceState_,
        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    TransitionResource(
        commandList,
        drawArgsResource_.Get(),
        drawArgsResourceState_,
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    // 1. シグネチャとPSOの設定
    commandList->SetGraphicsRootSignature(dxCommon_->GetPSOModelParticle().root_.GetSignature().Get());
    commandList->SetPipelineState(dxCommon_->GetPSOModelParticle().graphicsState_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 2. 頂点バッファの設定
    if (!model_) {
        return;
    }

    commandList->IASetVertexBuffers(0, 1, &model_->GetVBV());

    // --- ここからルートパラメータのセット (InitalizeForModelParticleの順番に合わせる) ---

    // Index 0: Material (b0) - CBV
    if (materialData_) {
        materialData_->enableLighting = currentUseJewelShader_ ? 1 : 0;
    }
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    // Index 1: DirectionalLight (b1) - CBV
    commandList->SetGraphicsRootConstantBufferView(1, directionalLightResource_->GetGPUVirtualAddress());

    // Index 2: Camera (b2) - CBV
    commandList->SetGraphicsRootConstantBufferView(2, cameraResource_->GetGPUVirtualAddress());

    // Index 3: Instancing Buffer (t1) - DescriptorTable
    // SrvManagerからGPUハンドルを取得してセットします
    commandList->SetGraphicsRootDescriptorTable(3, srvManager_->GetGPUDescriptionHandle(srvIndex_));

    // Index 4: Texture (t0) - DescriptorTable
    const std::string texturePath = ResolveRenderTexturePath_();
    if (!texturePath.empty()) {
        TextureManager::GetInstance()->LoadTexture(texturePath);
    }
	commandList->SetGraphicsRootDescriptorTable(4, TextureManager::GetInstance()->GetSrvHandleGPU(texturePath));
    
    commandList->ExecuteIndirect(
        commandSignature_.Get(),
        1,                          // 実行するコマンド数
        drawArgsResource_.Get(),    // 引数バッファ
        0,                          // オフセット
        nullptr, 0                  // カウントバッファ（今回は未使用）
    );
}

void ModelParticleManager::UpdateImGui(const std::string& effectName, ParticleEmitterConfig& editingConfig) {
    ImGui::Begin("37 Particle Editor");

    ImGui::Text("Editing: %s", effectName.c_str());
    ApplyRenderConfig_(editingConfig);

    // 値が変更されたかどうかをチェックするフラグ
    bool changed = false;

    if (ImGui::CollapsingHeader("Spawn", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::DragFloat("Speed Min", &editingConfig.speedMin, 0.01f, 0.0f);
        changed |= ImGui::DragFloat("Speed Max", &editingConfig.speedMax, 0.01f, 0.0f);
        changed |= ImGui::DragFloat("Life Min", &editingConfig.lifeTimeMin, 0.01f, 0.01f);
        changed |= ImGui::DragFloat("Life Max", &editingConfig.lifeTimeMax, 0.01f, 0.01f);
        changed |= ImGui::DragFloat3("Gravity", &editingConfig.gravity.x, 0.01f);
    }

    if (ImGui::CollapsingHeader("Color", ImGuiTreeNodeFlags_DefaultOpen)) {
    changed |= ImGui::ColorEdit4("StartColor", &editingConfig.startColor.x);
    changed |= ImGui::ColorEdit4("EndColor", &editingConfig.endColor.x);
        changed |= ImGui::DragFloat4("Start Color Random", &editingConfig.startColorRandom.x, 0.01f, 0.0f, 1.0f);
        changed |= ImGui::DragFloat4("End Color Random", &editingConfig.endColorRandom.x, 0.01f, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Scale", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::DragFloat("Start Scale", &editingConfig.startScale, 0.01f, 0.0f);
        changed |= ImGui::DragFloat("End Scale", &editingConfig.endScale, 0.01f, 0.0f);
        changed |= ImGui::DragFloat("Start Scale Random", &editingConfig.startScaleRandom, 0.01f, 0.0f);
        changed |= ImGui::DragFloat("End Scale Random", &editingConfig.endScaleRandom, 0.01f, 0.0f);
    }

    if (ImGui::CollapsingHeader("Rotation", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::DragFloat3("Initial Rotate Min", &editingConfig.initialRotateMin.x, 0.01f);
        changed |= ImGui::DragFloat3("Initial Rotate Max", &editingConfig.initialRotateMax.x, 0.01f);
        changed |= ImGui::DragFloat3("Angular Velocity Min", &editingConfig.angularVelocityMin.x, 0.01f);
        changed |= ImGui::DragFloat3("Angular Velocity Max", &editingConfig.angularVelocityMax.x, 0.01f);
    }

    // --- 新機能: エミッター形状 ---
    ImGui::Separator();
    ImGui::Text("Emitter Shape");
    const char* shapeNames[] = { "Point", "Sphere", "Box" };
    int currentShape = static_cast<int>(editingConfig.emitterShape);
    if (ImGui::Combo("Shape", &currentShape, shapeNames, IM_ARRAYSIZE(shapeNames))) {
        editingConfig.emitterShape = static_cast<EmitterShape>(currentShape);
        changed = true;
    }
    if (editingConfig.emitterShape != EmitterShape::Point) {
        changed |= ImGui::DragFloat3("Shape Size", &editingConfig.shapeSize.x, 0.01f);
    }

    // --- 新機能: イージングタイプ ---
    ImGui::Separator();
    ImGui::Text("Easing");
    const char* easingNames[] = { "Linear", "EaseIn", "EaseOut" };
    int currentEasing = static_cast<int>(editingConfig.easingType);
    if (ImGui::Combo("Easing Type", &currentEasing, easingNames, IM_ARRAYSIZE(easingNames))) {
        editingConfig.easingType = static_cast<EasingType>(currentEasing);
        changed = true;
    }

    // --- 新機能: ビルボード ---
    ImGui::Separator();
    changed |= ImGui::Checkbox("Billboard", &editingConfig.isBillboard);

    // --- Render Settings ---
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen)) {
        std::vector<std::string> modelPaths;
        std::vector<std::string> texturePaths;
        namespace fs = std::filesystem;
        if (fs::exists("resources")) {
            for (const auto& entry : fs::recursive_directory_iterator("resources")) {
                if (!entry.is_regular_file()) {
                    continue;
                }

                const fs::path path = entry.path();
                const std::string ext = path.extension().string();
                if (ext == ".obj") {
                    modelPaths.push_back(fs::relative(path, "resources").generic_string());
                } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                    texturePaths.push_back(path.generic_string());
                }
            }
            std::sort(modelPaths.begin(), modelPaths.end());
            std::sort(texturePaths.begin(), texturePaths.end());
        }

        if (ImGui::BeginCombo("Model", editingConfig.modelPath.c_str())) {
            for (const auto& path : modelPaths) {
                const bool selected = editingConfig.modelPath == path;
                if (ImGui::Selectable(path.c_str(), selected)) {
                    editingConfig.modelPath = path;
                    changed = true;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        char modelPathBuf[256]{};
        strncpy_s(modelPathBuf, editingConfig.modelPath.c_str(), sizeof(modelPathBuf) - 1);
        if (ImGui::InputText("Model Path", modelPathBuf, IM_ARRAYSIZE(modelPathBuf))) {
            editingConfig.modelPath = modelPathBuf;
            changed = true;
        }

        const std::string texturePreview = editingConfig.texturePath.empty() ? "None (model/default)" : editingConfig.texturePath;
        if (ImGui::BeginCombo("Texture Override", texturePreview.c_str())) {
            if (ImGui::Selectable("None (model/default)", editingConfig.texturePath.empty())) {
                editingConfig.texturePath.clear();
                changed = true;
            }
            for (const auto& path : texturePaths) {
                const bool selected = editingConfig.texturePath == path;
                if (ImGui::Selectable(path.c_str(), selected)) {
                    editingConfig.texturePath = path;
                    changed = true;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        char texturePathBuf[256]{};
        strncpy_s(texturePathBuf, editingConfig.texturePath.c_str(), sizeof(texturePathBuf) - 1);
        if (ImGui::InputText("Texture Path", texturePathBuf, IM_ARRAYSIZE(texturePathBuf))) {
            editingConfig.texturePath = texturePathBuf;
            changed = true;
        }

        changed |= ImGui::Checkbox("Use Jewel Shader", &editingConfig.useJewelShader);
    }

    editingConfig.speedMin = std::max(0.0f, editingConfig.speedMin);
    editingConfig.speedMax = std::max(0.0f, editingConfig.speedMax);
    editingConfig.lifeTimeMin = std::max(0.01f, editingConfig.lifeTimeMin);
    editingConfig.lifeTimeMax = std::max(0.01f, editingConfig.lifeTimeMax);
    if (editingConfig.speedMin > editingConfig.speedMax) {
        std::swap(editingConfig.speedMin, editingConfig.speedMax);
        changed = true;
    }
    if (editingConfig.lifeTimeMin > editingConfig.lifeTimeMax) {
        std::swap(editingConfig.lifeTimeMin, editingConfig.lifeTimeMax);
        changed = true;
    }
    editingConfig.startScaleRandom = std::max(0.0f, editingConfig.startScaleRandom);
    editingConfig.endScaleRandom = std::max(0.0f, editingConfig.endScaleRandom);
    editingConfig.startColorRandom = ClampColor(editingConfig.startColorRandom);
    editingConfig.endColorRandom = ClampColor(editingConfig.endColorRandom);

    // ★ リアルタイム反映：値が変わったら即座に Library を上書き
    if (changed) {
        effectLibrary_[effectName] = editingConfig;
        ApplyRenderConfig_(editingConfig);
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
        ApplyRenderConfig_(editingConfig);
    }

    if (ImGui::Button("Load from JSON")) {
        LoadFromJson(filename, editingConfig);
        // ロードしたら Library も更新
        effectLibrary_[effectName] = editingConfig;
        ApplyRenderConfig_(editingConfig);
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
