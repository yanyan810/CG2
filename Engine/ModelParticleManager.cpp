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
    instancingResource_ = dxCommon_->CreateBufferResource(sizeof(ModelParticleTransformationMatrix) * kMaxInstance);
    instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_));

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

}

void ModelParticleManager::Emit(const Particle& particle) {
    if (particles_.size() >= kMaxInstance) return;

    particles_.push_back(particle);
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
    particle.color = config.baseColor + colorVariation;

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

void ModelParticleManager::Update(float deltaTime, Camera* camera) {
    uint32_t instanceCount = 0;
    for (auto it = particles_.begin(); it != particles_.end(); ) {
        it->currentTime += deltaTime;
        if (it->currentTime >= it->lifeTime) {
            it = particles_.erase(it);
            continue;
        }

        // 進捗率 (0.0 ～ 1.0)
        float t = it->currentTime / it->lifeTime;
        
        // 色の線形補間 (Lerp)
        it->color.x = it->startColor.x + (it->endColor.x - it->startColor.x) * t;
        it->color.y = it->startColor.y + (it->endColor.y - it->startColor.y) * t;
        it->color.z = it->startColor.z + (it->endColor.z - it->startColor.z) * t;
        it->color.w = it->startColor.w + (it->endColor.w - it->startColor.w) * t;
        
        // 1. 加速度の適用 (速度を更新してから移動)
        it->velocity += it->acceleration * deltaTime;
        it->transform.translate += it->velocity * deltaTime;

        // 2. スケールのイージング (線形補間)
        float currentScale = it->startScale + (it->endScale - it->startScale) * t;
        it->transform.scale = { currentScale, currentScale, currentScale };

        // 3. 回転
        it->transform.rotate += it->angularVelocity * deltaTime;

        if (instanceCount < kMaxInstance) {
            Matrix4x4 world = Matrix4x4::MakeAffineMatrix(it->transform.scale, it->transform.rotate, it->transform.translate);

            // 色の計算（フェードアウト）
            // 炎などの場合、後半で一気に透明にするなら t*t などを使うと綺麗です
            float alpha = 1.0f - t;
            Vector4 finalColor = it->color;
            finalColor.w *= alpha;

            // GPUへのデータ転送
            instancingData_[instanceCount].WVP = world * camera->GetViewMatrix() * camera->GetProjectionMatrix();
            instancingData_[instanceCount].World = world;
            instancingData_[instanceCount].color = finalColor;

            instanceCount++;
        }
        ++it;
    }
}

void ModelParticleManager::Draw() {
    if (particles_.empty()) return;

    //skyDome_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeAdd);

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

    // 3. 描画実行
	commandList->DrawInstanced(static_cast<UINT>(model_->GetModelData().meshes[0].vertices.size()), static_cast<UINT>(particles_.size()), 0, 0); // 頂点数は3（plane.objの三角形1枚分）、インスタンス数はパーティクルの数
}