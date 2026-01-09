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

    // ===== ここから AABB デバッグ =====
    if (!modelData_.vertices.empty()) {
        Vector3 minPos{ +FLT_MAX, +FLT_MAX, +FLT_MAX };
        Vector3 maxPos{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

        for (const auto& v : modelData_.vertices) {
            const Vector4& p = v.position;
            if (p.x < minPos.x) minPos.x = p.x;
            if (p.y < minPos.y) minPos.y = p.y;
            if (p.z < minPos.z) minPos.z = p.z;

            if (p.x > maxPos.x) maxPos.x = p.x;
            if (p.y > maxPos.y) maxPos.y = p.y;
            if (p.z > maxPos.z) maxPos.z = p.z;
        }

        Vector3 size{
            maxPos.x - minPos.x,
            maxPos.y - minPos.y,
            maxPos.z - minPos.z
        };
        Vector3 center{
            (minPos.x + maxPos.x) * 0.5f,
            (minPos.y + maxPos.y) * 0.5f,
            (minPos.z + maxPos.z) * 0.5f
        };

        char buf[256];
        std::snprintf(
            buf, sizeof(buf),
            "[Model AABB] min=(%.3f, %.3f, %.3f), max=(%.3f, %.3f, %.3f), "
            "center=(%.3f, %.3f, %.3f), size=(%.3f, %.3f, %.3f)\n",
            minPos.x, minPos.y, minPos.z,
            maxPos.x, maxPos.y, maxPos.z,
            center.x, center.y, center.z,
            size.x, size.y, size.z
        );
        OutputDebugStringA(buf);
    }
    // ===== AABB デバッグここまで =====
    // ===== Normal デバッグ =====
    if (!modelData_.vertices.empty()) {
        Vector3 nMin{ +FLT_MAX, +FLT_MAX, +FLT_MAX };
        Vector3 nMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

        for (const auto& v : modelData_.vertices) {
            const Vector3& n = v.normal;
            nMin.x = std::min(nMin.x, n.x); nMin.y = std::min(nMin.y, n.y); nMin.z = std::min(nMin.z, n.z);
            nMax.x = std::max(nMax.x, n.x); nMax.y = std::max(nMax.y, n.y); nMax.z = std::max(nMax.z, n.z);
        }

        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "[Normal] min=(%.3f,%.3f,%.3f) max=(%.3f,%.3f,%.3f)\n",
            nMin.x, nMin.y, nMin.z, nMax.x, nMax.y, nMax.z);
        OutputDebugStringA(buf);
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

    *materialData_ = {}; // ★ゼロクリア（まずこれが強い）

    materialData_->color = { 1,1,1,1 };
    materialData_->enableLighting = 1;                 // ★最初からLambertにして確認しやすく
    materialData_->uvTransform = Matrix4x4::MakeIdentity4x4();

    materialData_->shininess = 64.0f;                  // ★必ず入れる（32〜128でOK）


    // ===== テクスチャ読み込み（パスが空ならスキップ）=====
    if (!modelData_.material.textureFilePath.empty()) {
        TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);
        if (!modelData_.material.textureFilePath.empty()) {
            TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);
        }

    } //else {
    //    modelData_.material.textureFilePath = 0; // 白テクスチャなど
    //}

    OutputDebugStringA(("[Model] vertex count = " +
        std::to_string(modelData_.vertices.size()) + "\n").c_str());

    char buf2[256];
    std::snprintf(buf2, sizeof(buf2),
        "[VertexData Offsets] pos=%zu uv=%zu nrm=%zu sizeof=%zu\n",
        offsetof(Model::VertexData, position),
        offsetof(Model::VertexData, texcoord),
        offsetof(Model::VertexData, normal),
        sizeof(Model::VertexData));
    OutputDebugStringA(buf2);

}


void Model::Draw(ID3D12GraphicsCommandList* cmd) {

	// 1) VBV
	cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// 2) マテリアルCB
	cmd->SetGraphicsRootConstantBufferView(
		0, materialResource_->GetGPUVirtualAddress());

	// 3) テクスチャSRV
    auto handle = TextureManager::GetInstance()
        ->GetSrvHandleGPU(modelData_.material.textureFilePath);

	cmd->SetGraphicsRootDescriptorTable(2, handle);

	// 4) DrawCall
	cmd->DrawInstanced(static_cast<UINT>(modelData_.vertices.size()), 1, 0, 0);
}

void Model::Draw(ID3D12GraphicsCommandList* cmd, uint32_t instanceCount) {

	cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);

	cmd->SetGraphicsRootConstantBufferView(
		0, materialResource_->GetGPUVirtualAddress());

    auto handle = TextureManager::GetInstance()
        ->GetSrvHandleGPU(modelData_.material.textureFilePath);

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
    ModelData data{};

    Assimp::Importer importer;

    // ==== OBJ とほぼ同じフラグ構成にする ====
    unsigned int flags = 0;
    flags |= aiProcess_Triangulate;
    flags |= aiProcess_JoinIdenticalVertices;
    flags |= aiProcess_GenSmoothNormals;
    flags |= aiProcess_FlipUVs;
    // flags |= aiProcess_ConvertToLeftHanded;   // 付ける場合は後で X反転を消す
    // flags |= aiProcess_PreTransformVertices;  // とりあえず外す

    const aiScene* scene = importer.ReadFile(fullPath.c_str(), flags);
    if (!scene || !scene->HasMeshes()) {
        OutputDebugStringA(("[Assimp] Failed: " + std::string(importer.GetErrorString()) + "\n").c_str());
        assert(false && "Assimp ReadFile failed");
        return data;
    }


    // 追加：Assimp は scene が非nullでもエラーを持っていることがある
    std::string err2 = importer.GetErrorString();
    if (!err2.empty()) {
        OutputDebugStringA(("[Assimp WARNING/ERROR AFTER LOAD] " + err2 + "\n").c_str());
    }

    OutputDebugStringA(("[Assimp] ReadFile SUCCESS, mNumMeshes = " +
        std::to_string(scene->mNumMeshes) + "\n").c_str());

    data.vertices.clear();

    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
        const aiMesh* mesh = scene->mMeshes[mi];
        aiVector3D zero3(0, 0, 0);

        OutputDebugStringA(("[Assimp] Process mesh #" +
            std::to_string(mi) +
            " (vertices=" + std::to_string(mesh->mNumVertices) +
            ", faces=" + std::to_string(mesh->mNumFaces) + ")\n").c_str());

        // ==== まず頂点テーブルを作る（OBJ と同じスタイル）====
        std::vector<VertexData> baseVertices(mesh->mNumVertices);

        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            const aiVector3D* pos = &mesh->mVertices[i];
            const aiVector3D* nrm = mesh->HasNormals() ? &mesh->mNormals[i] : &zero3;
            const aiVector3D* uv = mesh->HasTextureCoords(0) ? &mesh->mTextureCoords[0][i] : &zero3;

            VertexData v{};
            // ★ OBJ と同じ：そのまま（x,y,z）で入れる
            v.position = { pos->x, pos->y, pos->z, 1.0f };

            // ★ OBJ と同じ：V だけ 1 - v で反転
            float u = uv->x;
            float vtex = 1.0f - uv->y;
            v.texcoord = { u, vtex };

            v.normal = { nrm->x, nrm->y, nrm->z };

            baseVertices[i] = v;
        }

        // ==== 三角形リスト用に展開（OBJ と同じ順番）====
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            const aiFace& face = mesh->mFaces[i];
            assert(face.mNumIndices == 3);

            VertexData v0 = baseVertices[face.mIndices[0]];
            VertexData v1 = baseVertices[face.mIndices[1]];
            VertexData v2 = baseVertices[face.mIndices[2]];

            // 左手系っぽい順番に合わせて v2, v1, v0 で push（OBJ と同じ）
            data.vertices.push_back(v2);
            data.vertices.push_back(v1);
            data.vertices.push_back(v0);
        }
    }

    // 今はテクスチャは未対応としてクリア
    data.material.textureFilePath.clear();

    OutputDebugStringA(("[Assimp] Total vertices = " +
        std::to_string(data.vertices.size()) + "\n").c_str());

    return data;
}
