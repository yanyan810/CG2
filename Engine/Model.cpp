#include "Model.h"
#include <sstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cassert>


void Model::Initialize(ModelCommon* modelCommon,
    const std::string& directoryPath,
    const std::string& filename) {

    OutputDebugStringA("[Model] Initialize start\n");

    modelCommon_ = modelCommon;
    DirectXCommon* dx = modelCommon_->GetDxCommon();

    // ===== ここで拡張子判定 =====
    std::filesystem::path p(filename);
    std::string ext = p.extension().string();

    // directoryPath + filename → 実際のフルパス（例: resources/Human2.fbx）
    std::string fullPath = directoryPath + "/" + filename;

    if (ext == ".fbx" || ext == ".FBX") {
        OutputDebugStringA(("[Model] LoadAssimpFile: " + fullPath + "\n").c_str());
        modelData_ = LoadAssimpFile(fullPath);
    } else {
        OutputDebugStringA(("[Model] LoadObjFile: " + directoryPath + "/" + filename + "\n").c_str());
        modelData_ = LoadObjFile(directoryPath, filename);
    }

    // ===== 頂点バッファ作成 =====
    const size_t vtxCount = modelData_.vertices.size();
    assert(vtxCount > 0);

    vertexResource_ = dx->CreateBufferResource(sizeof(VertexData) * vtxCount);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    std::memcpy(vertexData_, modelData_.vertices.data(), sizeof(VertexData) * vtxCount);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vtxCount);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // ===== マテリアルリソース =====
    materialResource_ = dx->CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1,1,1,1 };
    materialData_->enableLighting = 0;
    materialData_->uvTransform = Matrix4x4::MakeIdentity4x4();

    // ===== テクスチャ読み込み（パスが空ならスキップ）=====
    if (!modelData_.material.textureFilePath.empty()) {
        TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);
        modelData_.material.textureIndex =
            TextureManager::GetInstance()
            ->GetTextureIndexByFilePath(modelData_.material.textureFilePath);
    } else {
        modelData_.material.textureIndex = 0; // 白テクスチャなど
    }

    OutputDebugStringA(("[Model] vertex count = " +
        std::to_string(modelData_.vertices.size()) + "\n").c_str());
}


void Model::Draw(ID3D12GraphicsCommandList* cmd) {

	// 1) VBV
	cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// 2) マテリアルCB
	cmd->SetGraphicsRootConstantBufferView(
		0, materialResource_->GetGPUVirtualAddress());

	// 3) テクスチャSRV
	auto handle = TextureManager::GetInstance()
		->GetSrvHandleGPU(modelData_.material.textureIndex);
	cmd->SetGraphicsRootDescriptorTable(2, handle);

	// 4) DrawCall
	cmd->DrawInstanced(static_cast<UINT>(modelData_.vertices.size()), 1, 0, 0);
}

void Model::Draw(ID3D12GraphicsCommandList* cmd, uint32_t instanceCount) {

	cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);

	cmd->SetGraphicsRootConstantBufferView(
		0, materialResource_->GetGPUVirtualAddress());

	auto handle = TextureManager::GetInstance()
		->GetSrvHandleGPU(modelData_.material.textureIndex);
	cmd->SetGraphicsRootDescriptorTable(2, handle);

	// ★ インスタンス数を引数で指定
	cmd->DrawInstanced(static_cast<UINT>(modelData_.vertices.size()),
		instanceCount,
		0, 0);
}


Model::MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	MaterialData materialData;//構築するMaterialData
	std::string line;// ファイルから読んだ1行を格納するもの
	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
	assert(file.is_open()); // ファイルが開けなかったらエラー

	while (std::getline(file, line)) {
		std::string identifer;
		std::istringstream s(line);
		s >> identifer; // 先頭の識別子を読む
		if (identifer == "map_Kd") {
			std::string textureFilePath;
			s >> textureFilePath; // テクスチャファイルのパスを読み込む
			materialData.textureFilePath = directoryPath + "/" + textureFilePath; // ディレクトリパスと結合
		}
	}

	return materialData;
}

Model::ModelData Model::LoadObjFile(
    const std::string& directoryPath,
    const std::string& filename)
{
    ModelData modelData{};

    // ==== Assimp でファイル読み込み ====
    std::string path = directoryPath + "/" + filename;

    Assimp::Importer importer;

    unsigned int flags = 0;
    flags |= aiProcess_Triangulate;           // 三角形化
    flags |= aiProcess_PreTransformVertices;  // ノードの変換を頂点に焼き込む
    flags |= aiProcess_CalcTangentSpace;
    flags |= aiProcess_GenSmoothNormals;
    flags |= aiProcess_GenUVCoords;
    flags |= aiProcess_RemoveRedundantMaterials;
    flags |= aiProcess_OptimizeMeshes;

    const aiScene* scene = importer.ReadFile(path, flags);
    if (!scene) {
        OutputDebugStringA(importer.GetErrorString());
        OutputDebugStringA("\n");
        // 空のモデルを返して assert を踏ませる
        assert(false && "Assimp ReadFile failed");
        return modelData;
    }

    assert(scene->mNumMeshes > 0);
    const aiMesh* mesh = scene->mMeshes[0];

    // ==== いったん「元頂点配列」を作る ====
    std::vector<VertexData> baseVertices;
    baseVertices.resize(mesh->mNumVertices);

    aiVector3D zero3(0.0f, 0.0f, 0.0f);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        const aiVector3D* pos = &mesh->mVertices[i];
        const aiVector3D* nrm = mesh->HasNormals() ? &mesh->mNormals[i] : &zero3;
        const aiVector3D* uv = mesh->HasTextureCoords(0) ? &mesh->mTextureCoords[0][i] : &zero3;

        VertexData v{};
        v.position = { pos->x, pos->y, pos->z, 1.0f };

        // 以前と同じく V だけ反転したければここで
        float u = uv->x;
        float vtex = 1.0f - uv->y;   // 上下反転
        v.texcoord = { u, vtex };

        v.normal = { nrm->x, nrm->y, nrm->z };

        baseVertices[i] = v;
    }

    // ==== 三角形リスト用に展開（今の設計に合わせる） ====
    modelData.vertices.clear();
    modelData.vertices.reserve(mesh->mNumFaces * 3);

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        assert(face.mNumIndices == 3); // Triangulate 済みのはず

        // 以前と同様「左手系の巻き順」になるように並び替え
        VertexData v0 = baseVertices[face.mIndices[0]];
        VertexData v1 = baseVertices[face.mIndices[1]];
        VertexData v2 = baseVertices[face.mIndices[2]];

        // ここは好みだけど、もともとのコードに寄せるなら
        // (i+1, i, 0) の順番で push していたのでそれに合わせるなら：
        modelData.vertices.push_back(v2); // i+1 相当
        modelData.vertices.push_back(v1); // i   相当
        modelData.vertices.push_back(v0); // 0   相当
    }

    // ==== マテリアル（テクスチャパス） ====
    MaterialData material{};

    if (mesh->mMaterialIndex < scene->mNumMaterials)
    {
        const aiMaterial* mtl = scene->mMaterials[mesh->mMaterialIndex];
        aiString texPath;

        if (mtl->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
        {
            // directoryPath + テクスチャファイル名
            material.textureFilePath = directoryPath + "/" + texPath.C_Str();
        } else {
            material.textureFilePath.clear(); // テクスチャなし
        }
    }

    modelData.material = material;

    return modelData;
}

Model::ModelData Model::LoadAssimpFile(const std::string& fullPath)
{
    ModelData data;

    OutputDebugStringA("[Assimp] LoadAssimpFile() enter\n");
    OutputDebugStringA(("[Assimp] fullPath = " + fullPath + "\n").c_str());

    Assimp::Importer importer;

    unsigned int flags = 0;
    // ==== 基本変換 ====
    flags |= aiProcess_Triangulate;           // 三角形化
    flags |= aiProcess_ConvertToLeftHanded;   // 左手座標系
    flags |= aiProcess_PreTransformVertices;  // ノード変換を焼き込み

    // ==== 品質系オプション ====
    flags |= aiProcess_JoinIdenticalVertices;
    flags |= aiProcess_GenSmoothNormals;
    flags |= aiProcess_CalcTangentSpace;
    flags |= aiProcess_ImproveCacheLocality;
    flags |= aiProcess_RemoveRedundantMaterials;
    flags |= aiProcess_OptimizeMeshes;
    flags |= aiProcess_SortByPType;
    flags |= aiProcess_FlipUVs;               // UV の V 反転

    OutputDebugStringA("[Assimp] Call ReadFile()\n");
    const aiScene* scene = importer.ReadFile(fullPath.c_str(), flags);

    if (!scene || !scene->HasMeshes()) {
        std::string err = importer.GetErrorString();
        OutputDebugStringA(("[Assimp] ReadFile FAILED: " + err + "\n").c_str());
        assert(false && "Assimp ReadFile failed");
        return data;
    }

    OutputDebugStringA(("[Assimp] ReadFile SUCCESS, mNumMeshes = " +
        std::to_string(scene->mNumMeshes) + "\n").c_str());

    data.vertices.clear();

    // ==== 全メッシュ結合 ====
    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
        const aiMesh* mesh = scene->mMeshes[mi];
        aiVector3D zero(0, 0, 0);

        OutputDebugStringA(("[Assimp] Process mesh #" +
            std::to_string(mi) +
            " (vertices=" + std::to_string(mesh->mNumVertices) +
            ", faces=" + std::to_string(mesh->mNumFaces) + ")\n").c_str());

        // まず元頂点テーブルを作る（OBJ と同じスタイル）
        std::vector<VertexData> baseVertices(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            const aiVector3D& p = mesh->mVertices[i];
            const aiVector3D& n = mesh->HasNormals() ? mesh->mNormals[i] : zero;
            const aiVector3D& uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : zero;

            VertexData v{};
            v.position = { p.x, p.y, p.z, 1.0f };  // 左手変換は Assimp 済み
            v.normal = { n.x, n.y, n.z };
            v.texcoord = { uv.x, uv.y };           // FlipUVs 済み

            baseVertices[i] = v;
        }

        // そのあと三角形ごとに「v2, v1, v0」の順で push
        for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi) {
            const aiFace& face = mesh->mFaces[fi];
            assert(face.mNumIndices == 3);

            VertexData v0 = baseVertices[face.mIndices[0]];
            VertexData v1 = baseVertices[face.mIndices[1]];
            VertexData v2 = baseVertices[face.mIndices[2]];

            // ←← ここが重要：OBJ と同じく順番を反転
            data.vertices.push_back(v2);
            data.vertices.push_back(v1);
            data.vertices.push_back(v0);
        }
    }

    data.material.textureFilePath.clear();

    OutputDebugStringA(("[Assimp] Total vertices = " +
        std::to_string(data.vertices.size()) + "\n").c_str());

    return data;
}
