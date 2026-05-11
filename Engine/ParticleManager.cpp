#include "ParticleManager.h"
#include "DirectXCommon.h"
#include "Camera.h" 
#include "Model.h"
#include "ModelManager.h"
#include "GeometryGenerator.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ParticleManager* ParticleManager::GetInstance() {
    static ParticleManager instance;
    return &instance;
}

namespace {
    Model::ModelData MakeParticlePrimitiveModelData(const std::vector<Model::VertexData>& vertices) {
        Model::ModelData md{};
        md.materials.push_back({ "" });

        Model::MeshData mesh{};
        mesh.materialIndex = 0;
        mesh.vertices = vertices;
        mesh.skinned = false;
        mesh.startVertex = 0;
        mesh.vertexCount = static_cast<uint32_t>(vertices.size());
        mesh.startIndex = 0;
        mesh.indexCount = static_cast<uint32_t>(vertices.size());

        md.meshes.push_back(std::move(mesh));

        md.indices.resize(vertices.size());
        for (uint32_t i = 0; i < static_cast<uint32_t>(vertices.size()); ++i) {
            md.indices[i] = i;
        }

        md.rootNode.name = "PrimitiveRoot";
        md.rootNode.localMatrix = Matrix4x4::MakeIdentity4x4();
        md.rootNode.meshIndices.push_back(0);

        return md;
    }
}

Model* ParticleManager::GetOrMakeParticlePrimitiveModel(int typeIndex) {
    std::string key = "ParticlePrimitive_" + std::to_string(typeIndex);
    Model* m = ModelManager::GetInstance()->FindModel(key);
    if (m) return m;

    std::vector<Model::VertexData> vertices;
    switch (typeIndex) {
    case 0: vertices = GeometryGenerator::GenerateRingTriListXY(64, 1.0f, 0.5f); break;
    case 1: vertices = GeometryGenerator::GenerateSphereTriList(32, 16, 1.0f); break;
    case 2: vertices = GeometryGenerator::GenerateBoxTriList(2.0f, 2.0f, 2.0f); break;
    case 3: vertices = GeometryGenerator::GeneratePlaneTriListXY(2.0f, 2.0f); break;
    case 4: vertices = GeometryGenerator::GenerateTorusTriList(32, 16, 1.0f, 0.3f); break;
    case 5: vertices = GeometryGenerator::GenerateCylinderTriList(32, 1.0f, 2.0f); break;
    case 6: vertices = GeometryGenerator::GenerateConeTriList(32, 1.0f, 2.0f); break;
    case 7: vertices = GeometryGenerator::GenerateTriangleTriListXY(2.0f, 2.0f); break;
    default: return nullptr;
    }

    auto modelData = MakeParticlePrimitiveModelData(vertices);
    return ModelManager::GetInstance()->CreatePrimitiveModel(key, modelData);
}

void ParticleManager::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, ParticleCommon* particleCommon) {
    dxCommon_ = dxCommon;
    srvManager_ = srvManager;
    particleCommon_ = particleCommon;
    // ランダム初期化
    std::random_device rd;
    randomEngine_ = std::mt19937(rd());

    // 頂点配列確保
    vertices_.resize(kVertexCount);

    // 左上(0) 右上(1) 右下(2) / 左上(0) 右下(2) 左下(3)
    auto setV = [&](int i, float x, float y, float u, float v) {
        vertices_[i].position[0] = x;
        vertices_[i].position[1] = y;
        vertices_[i].position[2] = 0.0f;
        vertices_[i].position[3] = 1.0f;
        vertices_[i].uv[0] = u;
        vertices_[i].uv[1] = v;
        vertices_[i].normal[0] = 0.0f;
        vertices_[i].normal[1] = 0.0f;
        vertices_[i].normal[2] = -1.0f;
        };

    const float s = 0.5f;
    setV(0, -s, +s, 0.0f, 0.0f);
    setV(1, +s, +s, 1.0f, 0.0f);
    setV(2, +s, -s, 1.0f, 1.0f);
    setV(3, -s, +s, 0.0f, 0.0f);
    setV(4, +s, -s, 1.0f, 1.0f);
    setV(5, -s, -s, 0.0f, 1.0f);

    const UINT bufferSize = sizeof(ParticleVertex) * kVertexCount;

    vertexBuffer_ = dxCommon_->CreateBufferResource(bufferSize);

    vbView_.BufferLocation =
        vertexBuffer_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = bufferSize;
    vbView_.StrideInBytes = sizeof(ParticleVertex);

    // GPUへ書き込み
    void* mapped = nullptr;
    vertexBuffer_->Map(0, nullptr, &mapped);
    memcpy(mapped, vertices_.data(), bufferSize);
    vertexBuffer_->Unmap(0, nullptr);

    // PerView リソース作成
    perViewResource_ = dxCommon_->CreateBufferResource(sizeof(PerView));
    perViewResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedPerView_));
    mappedPerView_->viewProjection = Matrix4x4::MakeIdentity4x4();
    mappedPerView_->billboardMatrix = Matrix4x4::MakeIdentity4x4();

    // PerFrame リソース作成
    perFrameResource_ = dxCommon_->CreateBufferResource(sizeof(PerFrame));
    perFrameResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedPerFrame_));
    mappedPerFrame_->time = 0.0f;
    mappedPerFrame_->deltaTime = 0.0f;
    time_ = 0.0f;

    // --- ダミーマテリアル＆ライト ---
    materialResource_ = dxCommon_->CreateBufferResource(256);
    struct DummyMaterial { Vector4 color; int enableLighting; float padding[3]; Matrix4x4 uvTransform; };
    DummyMaterial* mappedMat = nullptr;
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedMat));
    mappedMat->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    mappedMat->enableLighting = 0;
    mappedMat->uvTransform = Matrix4x4::MakeIdentity4x4();

    dirLightResource_ = dxCommon_->CreateBufferResource(256);
    struct DummyLight { Vector4 color; Vector3 direction; float intensity; };
    DummyLight* mappedLight = nullptr;
    dirLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedLight));
    mappedLight->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    mappedLight->direction = { 0.0f, -1.0f, 0.0f };
    mappedLight->intensity = 1.0f;
}

void ParticleManager::Finalize() {
    particleGroups_.clear();
    vertexBuffer_.Reset();
}

void ParticleManager::Update(float dt, const Camera& camera)
{
    const Matrix4x4& vp = camera.GetViewProjectionMatrix();
    const Matrix4x4& cameraMatrix = camera.GetWorldMatrix();

    Matrix4x4 billboardMatrix = cameraMatrix;
    billboardMatrix.m[3][0] = 0.0f;
    billboardMatrix.m[3][1] = 0.0f;
    billboardMatrix.m[3][2] = 0.0f;

    if (mappedPerView_) {
        mappedPerView_->viewProjection = vp;
        mappedPerView_->billboardMatrix = billboardMatrix;
    }

    time_ += dt;
    if (mappedPerFrame_) {
        mappedPerFrame_->time = time_;
        mappedPerFrame_->deltaTime = dt;
    }

    // Emitterの更新
    for (auto& [name, group] : particleGroups_) {
        if (group.mappedEmitter) {
            if (group.isAutoEmit) {
                group.mappedEmitter->frequencyTime += dt;
                if (group.mappedEmitter->frequency <= group.mappedEmitter->frequencyTime) {
                    group.mappedEmitter->frequencyTime -= group.mappedEmitter->frequency;
                    group.mappedEmitter->emit = 1;
                } else {
                    group.mappedEmitter->emit = 0;
                }
            } else {
                if (group.isEmitRequested) {
                    group.mappedEmitter->emit = 1;
                    group.isEmitRequested = false;
                } else {
                    group.mappedEmitter->emit = 0;
                }
            }
        }
    }
}

void ParticleManager::Emit(const std::string& groupName,
    const Vector3& pos,
    uint32_t count)
{
    auto it = particleGroups_.find(groupName);
    if (it == particleGroups_.end()) return;

    auto& group = it->second;
    if (group.mappedEmitter) {
        group.mappedEmitter->translate = pos;
        if (count > 0) {
            group.mappedEmitter->count = count;
        }
        group.isEmitRequested = true;
    }
}

void ParticleManager::EmitAll(const Vector3& pos, uint32_t count)
{
    for (auto& [name, group] : particleGroups_) {
        if (group.mappedEmitter) {
            group.mappedEmitter->translate = pos;
            if (count > 0) {
                group.mappedEmitter->count = count;
            }
            group.isEmitRequested = true;
        }
    }
}

void ParticleManager::LoadEffect(const std::string& effectName, const std::string& filename) {
    std::string path = "Resources/Particles/" + filename;
    std::ifstream file(path);
    if (!file.is_open()) return;

    json root;
    file >> root;

    for (const auto& g : root) {
        std::string originalName = g["name"];
        std::string name = effectName + "_" + originalName; // プレフィックスを付ける
        std::string texturePath = g["texturePath"];
        int modelType = g["modelType"];
        std::string modelName = g["modelName"];

        if (modelType == 0) {
            CreateParticleGroup(name, texturePath);
        } else if (modelType == 1) {
            int primIndex = 0;
            try { primIndex = std::stoi(modelName); } catch (...) {}
            Model* model = GetOrMakeParticlePrimitiveModel(primIndex);
            CreateParticleGroup(name, model);
        } else if (modelType == 2) {
            ModelManager::GetInstance()->LoadModel(modelName);
            Model* model = ModelManager::GetInstance()->FindModel(modelName);
            CreateParticleGroup(name, model);
        }

        auto& group = particleGroups_[name];
        group.texturePath = texturePath;
        if (texturePath != "") {
            TextureManager::GetInstance()->LoadTexture(texturePath);
            group.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(texturePath);
        } else {
            std::string whiteTex = "resources/ui/white.png";
            TextureManager::GetInstance()->LoadTexture(whiteTex);
            group.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(whiteTex);
        }

        group.modelType = modelType;
        group.modelName = modelName;
        group.blendMode = static_cast<ParticleCommon::BlendMode>(g["blendMode"].get<int>());
        group.billboardMode = g["billboardMode"];
        group.isAutoEmit = g["isAutoEmit"];

        if (g.contains("emitter") && group.mappedEmitter) {
            auto e = g["emitter"];
            group.mappedEmitter->count = e["count"];
            group.mappedEmitter->frequency = e["frequency"];
            group.mappedEmitter->translate = { e["translate"][0], e["translate"][1], e["translate"][2] };
            group.mappedEmitter->radius = e["radius"];
            group.mappedEmitter->lifeTimeMin = e["lifeTimeMin"];
            group.mappedEmitter->lifeTimeMax = e["lifeTimeMax"];
            group.mappedEmitter->velocityBase = { e["velocityBase"][0], e["velocityBase"][1], e["velocityBase"][2] };
            group.mappedEmitter->velocityVariance = e["velocityVariance"];
            group.mappedEmitter->startColor = { e["startColor"][0], e["startColor"][1], e["startColor"][2], e["startColor"][3] };
            group.mappedEmitter->endColor = { e["endColor"][0], e["endColor"][1], e["endColor"][2], e["endColor"][3] };
            group.mappedEmitter->shapeType = e["shapeType"];
            group.mappedEmitter->shapeAngle = e["shapeAngle"];
            group.mappedEmitter->shapeSize = { e["shapeSize"][0], e["shapeSize"][1], e["shapeSize"][2] };
            group.mappedEmitter->acceleration = { e["acceleration"][0], e["acceleration"][1], e["acceleration"][2] };
        }
    }
}

void ParticleManager::LoadAllEffects() {
    std::string particleDir = "Resources/Particles";
    if (std::filesystem::exists(particleDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(particleDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                std::string effectName = entry.path().stem().string(); // 拡張子なしのファイル名
                LoadEffect(effectName, filename);
            }
        }
    }
}

void ParticleManager::EmitEffect(const std::string& effectName, const Vector3& pos, uint32_t count) {
    std::string prefix = effectName + "_";
    for (auto& [name, group] : particleGroups_) {
        // グループ名が effectName_ から始まるかチェック
        if (name.starts_with(prefix)) {
            if (group.mappedEmitter) {
                group.mappedEmitter->translate = pos;
                if (count > 0) {
                    group.mappedEmitter->count = count;
                }
                group.isEmitRequested = true;
            }
        }
    }
}

void ParticleManager::CreateParticleGroup(
    const std::string& name,
    const std::string& texturePath)
{
    assert(!particleGroups_.contains(name));

    ParticleGroup group{};
    group.texturePath = texturePath;
    group.modelType = 0;
    group.modelName = "";

    if (!texturePath.empty()) {
        TextureManager::GetInstance()->LoadTexture(texturePath);
        group.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(texturePath);
    } else {
        std::string whiteTex = "resources/ui/white.png";
        TextureManager::GetInstance()->LoadTexture(whiteTex);
        group.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(whiteTex);
    }

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    
    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Alignment = 0;
    resDesc.Width = sizeof(Particles) * kMaxInstance;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, 
        D3D12_RESOURCE_STATE_COMMON, nullptr, 
        IID_PPV_ARGS(&group.instancingResource));
    assert(SUCCEEDED(hr));

    group.instancingSrvIndex = srvManager_->Allocate();
    srvManager_->CreateSRVforStructuredBuffer(
        group.instancingSrvIndex,
        group.instancingResource.Get(),
        kMaxInstance,
        sizeof(Particles)
    );

    group.instancingUavIndex = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(
        group.instancingUavIndex,
        group.instancingResource.Get(),
        kMaxInstance,
        sizeof(Particles)
    );

    D3D12_RESOURCE_DESC counterDesc = resDesc;
    counterDesc.Width = sizeof(int32_t);
    
    hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &counterDesc, 
        D3D12_RESOURCE_STATE_COMMON, nullptr, 
        IID_PPV_ARGS(&group.freeListIndexResource));
    assert(SUCCEEDED(hr));

    group.freeListIndexUavIndex = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(
        group.freeListIndexUavIndex,
        group.freeListIndexResource.Get(),
        1,
        sizeof(int32_t)
    );

    D3D12_RESOURCE_DESC listDesc = resDesc;
    listDesc.Width = sizeof(uint32_t) * kMaxInstance;
    hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &listDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(&group.freeListResource));
    assert(SUCCEEDED(hr));

    group.freeListUavIndex = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(
        group.freeListUavIndex,
        group.freeListResource.Get(),
        kMaxInstance,
        sizeof(uint32_t)
    );

    auto* cmd = dxCommon_->GetCommandList();

    D3D12_RESOURCE_BARRIER barriers[3]{};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = group.instancingResource.Get();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = group.freeListIndexResource.Get();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[2].Transition.pResource = group.freeListResource.Get();
    barriers[2].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barriers[2].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[2].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(3, barriers);

    particleCommon_->SetComputePipelineState();
    
    srvManager_->PreDraw();

    cmd->SetComputeRootDescriptorTable(0, srvManager_->GetGPUDescriptionHandle(group.instancingUavIndex));
    cmd->SetComputeRootDescriptorTable(1, srvManager_->GetGPUDescriptionHandle(group.freeListIndexUavIndex));
    cmd->SetComputeRootDescriptorTable(2, srvManager_->GetGPUDescriptionHandle(group.freeListUavIndex));

    cmd->Dispatch(kMaxInstance, 1, 1);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = group.instancingResource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(1, &barrier);

    group.emitterResource = dxCommon_->CreateBufferResource(sizeof(EmitterData));
    group.emitterResource->Map(0, nullptr, reinterpret_cast<void**>(&group.mappedEmitter));
    group.mappedEmitter->count = 10;
    group.mappedEmitter->frequency = 0.5f;
    group.mappedEmitter->frequencyTime = 0.0f;
    group.mappedEmitter->translate = { 0.0f, 0.0f, 0.0f };
    group.mappedEmitter->radius = 5.0f;
    group.mappedEmitter->emit = 0;

    group.mappedEmitter->lifeTimeMin = 0.5f;
    group.mappedEmitter->lifeTimeMax = 2.0f;
    group.mappedEmitter->velocityBase = { 0.0f, 0.0f, 0.0f };
    group.mappedEmitter->velocityVariance = 0.1f;
    group.mappedEmitter->startColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    group.mappedEmitter->endColor = { 1.0f, 1.0f, 1.0f, 0.0f };

    group.mappedEmitter->shapeType = 0; // 0:Sphere
    group.mappedEmitter->shapeAngle = 0.5f; // Cone用
    group.mappedEmitter->shapeSize = { 5.0f, 5.0f, 5.0f }; // Box用
    group.mappedEmitter->acceleration = { 0.0f, 0.0f, 0.0f }; // 重力など

    group.billboardMode = 0; // 0:Billboard

    particleGroups_.emplace(name, std::move(group));
}

void ParticleManager::CreateParticleGroup(
    const std::string& name,
    Model* model)
{
    assert(!particleGroups_.contains(name));
    assert(model != nullptr);

    ParticleGroup group{};
    group.model = model;
    group.modelType = 2;
    group.modelName = "";

    const auto& materials = model->GetModelData().materials;
    if (!materials.empty() && !materials[0].textureFilePath.empty()) {
        const std::string& texPath = materials[0].textureFilePath;
        if (!TextureManager::GetInstance()->HasTexture(texPath)) {
            TextureManager::GetInstance()->LoadTexture(texPath);
        }
        group.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(texPath);
        group.texturePath = texPath;
    } else {
        std::string whiteTex = "resources/ui/white.png";
        TextureManager::GetInstance()->LoadTexture(whiteTex);
        group.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(whiteTex);
        group.texturePath = "";
    }

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    
    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Alignment = 0;
    resDesc.Width = sizeof(Particles) * kMaxInstance;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, 
        D3D12_RESOURCE_STATE_COMMON, nullptr, 
        IID_PPV_ARGS(&group.instancingResource));
    assert(SUCCEEDED(hr));

    group.instancingSrvIndex = srvManager_->Allocate();
    srvManager_->CreateSRVforStructuredBuffer(
        group.instancingSrvIndex,
        group.instancingResource.Get(),
        kMaxInstance,
        sizeof(Particles)
    );

    group.instancingUavIndex = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(
        group.instancingUavIndex,
        group.instancingResource.Get(),
        kMaxInstance,
        sizeof(Particles)
    );

    D3D12_RESOURCE_DESC counterDesc = resDesc;
    counterDesc.Width = sizeof(int32_t);
    
    hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &counterDesc, 
        D3D12_RESOURCE_STATE_COMMON, nullptr, 
        IID_PPV_ARGS(&group.freeListIndexResource));
    assert(SUCCEEDED(hr));

    group.freeListIndexUavIndex = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(
        group.freeListIndexUavIndex,
        group.freeListIndexResource.Get(),
        1,
        sizeof(int32_t)
    );

    D3D12_RESOURCE_DESC listDesc = resDesc;
    listDesc.Width = sizeof(uint32_t) * kMaxInstance;
    hr = dxCommon_->GetDevice()->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &listDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_PPV_ARGS(&group.freeListResource));
    assert(SUCCEEDED(hr));

    group.freeListUavIndex = srvManager_->Allocate();
    srvManager_->CreateUAVforStructuredBuffer(
        group.freeListUavIndex,
        group.freeListResource.Get(),
        kMaxInstance,
        sizeof(uint32_t)
    );

    auto* computeCmd = dxCommon_->GetCommandList();

    D3D12_RESOURCE_BARRIER barriers[3]{};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = group.instancingResource.Get();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = group.freeListIndexResource.Get();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[2].Transition.pResource = group.freeListResource.Get();
    barriers[2].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barriers[2].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[2].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    computeCmd->ResourceBarrier(3, barriers);

    particleCommon_->SetComputePipelineState(computeCmd);
    
    ID3D12DescriptorHeap* heaps[] = { srvManager_->GetDescriptorHeap() };
    computeCmd->SetDescriptorHeaps(1, heaps);

    computeCmd->SetComputeRootDescriptorTable(0, srvManager_->GetGPUDescriptionHandle(group.instancingUavIndex));
    computeCmd->SetComputeRootDescriptorTable(1, srvManager_->GetGPUDescriptionHandle(group.freeListIndexUavIndex));
    computeCmd->SetComputeRootDescriptorTable(2, srvManager_->GetGPUDescriptionHandle(group.freeListUavIndex));

    computeCmd->Dispatch(kMaxInstance, 1, 1);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = group.instancingResource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    computeCmd->ResourceBarrier(1, &barrier);

    group.emitterResource = dxCommon_->CreateBufferResource(sizeof(EmitterData));
    group.emitterResource->Map(0, nullptr, reinterpret_cast<void**>(&group.mappedEmitter));
    group.mappedEmitter->count = 10;
    group.mappedEmitter->frequency = 0.5f;
    group.mappedEmitter->frequencyTime = 0.0f;
    group.mappedEmitter->translate = { 0.0f, 0.0f, 0.0f };
    group.mappedEmitter->radius = 5.0f;
    group.mappedEmitter->emit = 0;

    group.mappedEmitter->lifeTimeMin = 0.5f;
    group.mappedEmitter->lifeTimeMax = 2.0f;
    group.mappedEmitter->velocityBase = { 0.0f, 0.0f, 0.0f };
    group.mappedEmitter->velocityVariance = 0.1f;
    group.mappedEmitter->startColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    group.mappedEmitter->endColor = { 1.0f, 1.0f, 1.0f, 0.0f };

    group.mappedEmitter->shapeType = 0; // 0:Sphere
    group.mappedEmitter->shapeAngle = 0.5f; // Cone用
    group.mappedEmitter->shapeSize = { 5.0f, 5.0f, 5.0f }; // Box用
    group.mappedEmitter->acceleration = { 0.0f, 0.0f, 0.0f }; // 重力など

    group.billboardMode = 0; // 0:Billboard

    particleGroups_.emplace(name, std::move(group));
}

void ParticleManager::SetGroupBlendMode(const std::string& groupName, ParticleCommon::BlendMode mode) {
    auto it = particleGroups_.find(groupName);
    if (it == particleGroups_.end()) return;
    it->second.blendMode = mode;
}

void ParticleManager::UpdateCompute(ID3D12GraphicsCommandList* computeCmd) {
    for (auto& [name, group] : particleGroups_) {

        if (particleCommon_) {
            particleCommon_->SetEmitComputePipelineState(computeCmd);
        }

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = group.instancingResource.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        computeCmd->ResourceBarrier(1, &barrier);

        ID3D12DescriptorHeap* heaps[] = { srvManager_->GetDescriptorHeap() };
        computeCmd->SetDescriptorHeaps(1, heaps);

        computeCmd->SetComputeRootConstantBufferView(3, group.emitterResource->GetGPUVirtualAddress());
        computeCmd->SetComputeRootConstantBufferView(4, perFrameResource_->GetGPUVirtualAddress());
        computeCmd->SetComputeRootDescriptorTable(0, srvManager_->GetGPUDescriptionHandle(group.instancingUavIndex));
        computeCmd->SetComputeRootDescriptorTable(1, srvManager_->GetGPUDescriptionHandle(group.freeListIndexUavIndex));
        computeCmd->SetComputeRootDescriptorTable(2, srvManager_->GetGPUDescriptionHandle(group.freeListUavIndex));

        computeCmd->Dispatch(1, 1, 1);

        D3D12_RESOURCE_BARRIER uavBarrier{};
        uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        uavBarrier.UAV.pResource = group.instancingResource.Get();
        computeCmd->ResourceBarrier(1, &uavBarrier);

        if (particleCommon_) {
            particleCommon_->SetUpdateComputePipelineState(computeCmd);
        }

        computeCmd->Dispatch((kMaxInstance + 1023) / 1024, 1, 1);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        computeCmd->ResourceBarrier(1, &barrier);
    }
}

void ParticleManager::Draw(ID3D12GraphicsCommandList* cmd) {
    for (auto& [name, group] : particleGroups_) {

        if (particleCommon_) {
            particleCommon_->SetBlendMode(group.blendMode);
            particleCommon_->SetGraphicsPipelineState();
        }

        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cmd->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
        cmd->SetGraphicsRootConstantBufferView(3, dirLightResource_->GetGPUVirtualAddress());
        cmd->SetGraphicsRootConstantBufferView(4, perViewResource_->GetGPUVirtualAddress());
        cmd->SetGraphicsRoot32BitConstants(5, 1, &group.billboardMode, 0);

        srvManager_->SetGraphicsDescriptorTable(2, group.textureSrvIndex);
        srvManager_->SetGraphicsDescriptorTable(1, group.instancingSrvIndex);

        if (group.model) {
            auto vbv = group.model->GetVBV();
            if (vbv.StrideInBytes == 0) continue;

            cmd->IASetVertexBuffers(0, 1, &vbv);
            
            if (group.model->HasIndexBuffer()) {
                auto ibv = group.model->GetIBV();
                cmd->IASetIndexBuffer(&ibv);
                uint32_t indexCount = ibv.SizeInBytes / sizeof(uint32_t);
                if (indexCount > 0) {
                    cmd->DrawIndexedInstanced(indexCount, kMaxInstance, 0, 0, 0);
                }
            } else {
                uint32_t vertexCount = vbv.SizeInBytes / vbv.StrideInBytes;
                if (vertexCount > 0) {
                    cmd->DrawInstanced(vertexCount, kMaxInstance, 0, 0);
                }
            }
        } else {
            cmd->IASetVertexBuffers(0, 1, &vbView_);
            if (kVertexCount > 0) {
                cmd->DrawInstanced(kVertexCount, kMaxInstance, 0, 0);
            }
        }
    }
}
