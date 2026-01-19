#include "Model.h"
#include <sstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cassert>

// ================================
// Assimp helpers (materials/meshes)
// ================================
static void BuildMaterials(Model::ModelData& out,
    const aiScene* scene,
    const std::string& directoryPath)
{
    out.materials.clear();
    out.materials.resize(scene->mNumMaterials);

    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        const aiMaterial* mtl = scene->mMaterials[i];
        aiString texPath;

        if (mtl->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            out.materials[i].textureFilePath = directoryPath + "/" + texPath.C_Str();
        } else {
            out.materials[i].textureFilePath.clear();
        }
    }
}

#include <windows.h>
#include <string>

static void PrintLoadedAssimpPath_()
{
    const char* names[] = {
        "assimp-vc143-mtd.dll",
        "assimp-vc142-mtd.dll",
        "assimp-vc143-mt.dll",
        "assimp-vc142-mt.dll",
        "assimp.dll"
    };

    for (auto* n : names) {
        HMODULE h = GetModuleHandleA(n);
        if (h) {
            char path[MAX_PATH]{};
            GetModuleFileNameA(h, path, MAX_PATH);
            OutputDebugStringA(("[AssimpDLL] loaded: " + std::string(n) + "\n").c_str());
            OutputDebugStringA(("[AssimpDLL] path  : " + std::string(path) + "\n").c_str());
            return;
        }
    }

    OutputDebugStringA("[AssimpDLL] not loaded (any known name)\n");
}



static Model::MeshData BuildMeshTriList(const aiMesh* mesh)
{
    Model::MeshData md{};
    md.materialIndex = mesh->mMaterialIndex;

    std::vector<Model::VertexData> base(mesh->mNumVertices);
    aiVector3D zero3(0, 0, 0);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        const aiVector3D* pos = &mesh->mVertices[i];
        const aiVector3D* nrm = mesh->HasNormals() ? &mesh->mNormals[i] : &zero3;
        const aiVector3D* uv = mesh->HasTextureCoords(0) ? &mesh->mTextureCoords[0][i] : &zero3;

        Model::VertexData v{};
        v.position = { pos->x, pos->y, pos->z, 1.0f };

        // ★統一方針：自前で 1 - v 反転（aiProcess_FlipUVs は使わない）
        v.texcoord = { uv->x, 1.0f - uv->y };

        v.normal = { nrm->x, nrm->y, nrm->z };
        base[i] = v;
    }

    md.vertices.reserve(mesh->mNumFaces * 3);

    for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
        const aiFace& face = mesh->mFaces[f];
        if (face.mNumIndices != 3) continue;

        auto v0 = base[face.mIndices[0]];
        auto v1 = base[face.mIndices[1]];
        auto v2 = base[face.mIndices[2]];

        // あなたの現行と同じ並び（左手系っぽくしたい意図）
        md.vertices.push_back(v2);
        md.vertices.push_back(v1);
        md.vertices.push_back(v0);
    }

    return md;
}

// Assimp matrix -> your Matrix4x4
static Matrix4x4 ConvertAssimpMatrix(const aiMatrix4x4& mIn)
{
    // スライドの通り transpose を挟む（あなたの行列配置に合わせやすい）
    aiMatrix4x4 m = mIn;
    m.Transpose();

    Matrix4x4 out = Matrix4x4::MakeIdentity4x4();
    out.m[0][0] = m.a1; out.m[0][1] = m.a2; out.m[0][2] = m.a3; out.m[0][3] = m.a4;
    out.m[1][0] = m.b1; out.m[1][1] = m.b2; out.m[1][2] = m.b3; out.m[1][3] = m.b4;
    out.m[2][0] = m.c1; out.m[2][1] = m.c2; out.m[2][2] = m.c3; out.m[2][3] = m.c4;
    out.m[3][0] = m.d1; out.m[3][1] = m.d2; out.m[3][2] = m.d3; out.m[3][3] = m.d4;
    return out;
}

static Model::Node ReadNodeRecursive(const aiNode* node)
{
    Model::Node out{};
    out.name = node->mName.C_Str();
    out.localMatrix = ConvertAssimpMatrix(node->mTransformation);

    out.meshIndices.reserve(node->mNumMeshes);
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        out.meshIndices.push_back((uint32_t)node->mMeshes[i]); // scene->mMeshes の index
    }

    out.children.resize(node->mNumChildren);
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        out.children[i] = ReadNodeRecursive(node->mChildren[i]);
    }
    return out;
}



void Model::Initialize(ModelCommon* modelCommon,
    const std::string& directoryPath,
    const std::string& filename)
{
    OutputDebugStringA("[Model] Initialize start\n");

    modelCommon_ = modelCommon;
    DirectXCommon* dx = modelCommon_->GetDxCommon();

    // ===== ここで拡張子判定 =====
    std::filesystem::path p(filename);
    std::string ext = p.extension().string();
    for (auto& c : ext) c = (char)std::tolower(c);

    std::string fullPath = directoryPath + "/" + filename;

    // assimpで読む形式を増やす（fbx/gltf/glb/obj など）
    const bool useAssimp =
        (ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".obj");

    if (useAssimp) {
        OutputDebugStringA(("[Model] LoadAssimpFile: " + fullPath + "\n").c_str());
        modelData_ = LoadAssimpFile(fullPath);
    } else {
        // もし「OBJだけ別実装」を残したいならここに置く
        OutputDebugStringA(("[Model] LoadObjFile: " + directoryPath + "/" + filename + "\n").c_str());
        modelData_ = LoadObjFile(directoryPath, filename);
    }


    // ======================
    // AABB / Normal デバッグ（全Mesh走査）
    // ======================
    bool hasAny = false;

    Vector3 minPos{ +FLT_MAX, +FLT_MAX, +FLT_MAX };
    Vector3 maxPos{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

    Vector3 nMin{ +FLT_MAX, +FLT_MAX, +FLT_MAX };
    Vector3 nMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (const auto& mesh : modelData_.meshes) {
        for (const auto& v : mesh.vertices) {
            hasAny = true;

            // AABB
            const Vector4& pp = v.position;
            minPos.x = std::min(minPos.x, pp.x);
            minPos.y = std::min(minPos.y, pp.y);
            minPos.z = std::min(minPos.z, pp.z);
            maxPos.x = std::max(maxPos.x, pp.x);
            maxPos.y = std::max(maxPos.y, pp.y);
            maxPos.z = std::max(maxPos.z, pp.z);

            // Normal range
            const Vector3& nn = v.normal;
            nMin.x = std::min(nMin.x, nn.x);
            nMin.y = std::min(nMin.y, nn.y);
            nMin.z = std::min(nMin.z, nn.z);
            nMax.x = std::max(nMax.x, nn.x);
            nMax.y = std::max(nMax.y, nn.y);
            nMax.z = std::max(nMax.z, nn.z);
        }
    }

    if (hasAny) {
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

        char bufN[256];
        std::snprintf(bufN, sizeof(bufN),
            "[Normal] min=(%.3f,%.3f,%.3f) max=(%.3f,%.3f,%.3f)\n",
            nMin.x, nMin.y, nMin.z,
            nMax.x, nMax.y, nMax.z);
        OutputDebugStringA(bufN);
    }

    // ======================
    // VB 作成（全meshの頂点を1本に連結）
    // ======================
    size_t totalVtx = 0;
    for (auto& mesh : modelData_.meshes) {
        totalVtx += mesh.vertices.size();
    }

    if (totalVtx == 0) {
        OutputDebugStringA("[Model] ERROR: totalVtx == 0 (import failed). Skip creating buffers.\n");
        // ここで “描画不可モデル” として終了
        return;
    }

    vertexResource_ = dx->CreateBufferResource(sizeof(VertexData) * totalVtx);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    uint32_t cursor = 0;
    for (auto& mesh : modelData_.meshes) {
        mesh.startVertex = cursor;
        mesh.vertexCount = static_cast<uint32_t>(mesh.vertices.size());

        if (!mesh.vertices.empty()) {
            std::memcpy(vertexData_ + cursor,
                mesh.vertices.data(),
                sizeof(VertexData) * mesh.vertices.size());
        }
        cursor += mesh.vertexCount;
    }

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * totalVtx);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // ======================
    // Material CB
    // ======================
    materialResource_ = dx->CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    *materialData_ = {};
    materialData_->color = { 1,1,1,1 };
    materialData_->enableLighting = 1;
    materialData_->uvTransform = Matrix4x4::MakeIdentity4x4();
    materialData_->shininess = 64.0f;

    // ======================
    // Texture load（materials 全部）
    // ======================
    for (const auto& m : modelData_.materials) {
        if (!m.textureFilePath.empty()) {
            TextureManager::GetInstance()->LoadTexture(m.textureFilePath);
        }
    }

    OutputDebugStringA(("[Model] total vertex count = " +
        std::to_string(totalVtx) + "\n").c_str());

    char buf2[256];
    std::snprintf(buf2, sizeof(buf2),
        "[VertexData Offsets] pos=%zu uv=%zu nrm=%zu sizeof=%zu\n",
        offsetof(Model::VertexData, position),
        offsetof(Model::VertexData, texcoord),
        offsetof(Model::VertexData, normal),
        sizeof(Model::VertexData));
    OutputDebugStringA(buf2);
}


void Model::Draw(ID3D12GraphicsCommandList* cmd)
{

    if (!vertexResource_ || !materialResource_ || !materialData_) {
        OutputDebugStringA("[Model] Draw skipped: resources not initialized\n");
        return;
    }

    cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);

    cmd->SetGraphicsRootConstantBufferView(
        0, materialResource_->GetGPUVirtualAddress());

    for (const auto& mesh : modelData_.meshes) {

        std::string texPath;
        if (mesh.materialIndex < modelData_.materials.size()) {
            texPath = modelData_.materials[mesh.materialIndex].textureFilePath;
        }

        // ★ 空は TextureManager が白にしてくれるのでそのまま渡してOK
        // ★ ただし「未ロードの非空パス」は at() で落ちる可能性があるので保険
        D3D12_GPU_DESCRIPTOR_HANDLE handle{};
        bool useWhite = false;

        if (!texPath.empty()) {
            // 未ロードでも LoadTexture は二重ロードしないので呼んでOK
            TextureManager::GetInstance()->LoadTexture(texPath);

            // ここで例外/アサートを避けるため、存在確認が欲しいが
            // 現状APIに無いので try/catch で保険（MSVCなら unordered_map::at は例外）
            try {
                handle = TextureManager::GetInstance()->GetSrvHandleGPU(texPath);
            } catch (...) {
                useWhite = true;
            }
        } else {
            useWhite = true;
        }

        if (useWhite) {
            handle = TextureManager::GetInstance()->GetSrvHandleGPU(""); // 空→白
        }

        cmd->SetGraphicsRootDescriptorTable(2, handle);
        cmd->DrawInstanced(mesh.vertexCount, 1, mesh.startVertex, 0);
    }
}



void Model::Draw(ID3D12GraphicsCommandList* cmd, uint32_t instanceCount)
{

    if (!vertexResource_ || !materialResource_ || !materialData_) {
        OutputDebugStringA("[Model] Draw skipped: resources not initialized\n");
        return;
    }

    cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);

    cmd->SetGraphicsRootConstantBufferView(
        0, materialResource_->GetGPUVirtualAddress());

    for (const auto& mesh : modelData_.meshes) {

        std::string texPath;
        if (mesh.materialIndex < modelData_.materials.size()) {
            texPath = modelData_.materials[mesh.materialIndex].textureFilePath;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE handle{};
        bool useWhite = false;

        if (!texPath.empty()) {
            TextureManager::GetInstance()->LoadTexture(texPath);
            try {
                handle = TextureManager::GetInstance()->GetSrvHandleGPU(texPath);
            } catch (...) {
                useWhite = true;
            }
        } else {
            useWhite = true;
        }

        if (useWhite) {
            handle = TextureManager::GetInstance()->GetSrvHandleGPU("");
        }

        cmd->SetGraphicsRootDescriptorTable(2, handle);
        cmd->DrawInstanced(mesh.vertexCount, instanceCount, mesh.startVertex, 0);
    }
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
    ModelData out{};
    std::string path = directoryPath + "/" + filename;

    Assimp::Importer importer;

    unsigned int flags = 0;
    flags |= aiProcess_Triangulate;
    flags |= aiProcess_PreTransformVertices; // ノード変換を焼き込み
    flags |= aiProcess_GenSmoothNormals;
    // ★自前で 1-v するので FlipUVs は使わない
    // flags |= aiProcess_FlipUVs;

    const aiScene* scene = importer.ReadFile(path, flags);
    if (!scene || !scene->HasMeshes()) {
        OutputDebugStringA(importer.GetErrorString());
        OutputDebugStringA("\n");
        assert(false && "Assimp ReadFile failed");
        return out;
    }

    // materials 全取得
    BuildMaterials(out, scene, directoryPath);

    // meshes 全取得
    out.meshes.clear();
    out.meshes.reserve(scene->mNumMeshes);
    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
        out.meshes.push_back(BuildMeshTriList(scene->mMeshes[mi]));
    }

    return out;
}

static void DebugAssimpSupport_(Assimp::Importer& importer)
{
    const bool gltf = importer.IsExtensionSupported("gltf");
    const bool glb = importer.IsExtensionSupported("glb");
    const bool fbx = importer.IsExtensionSupported("fbx");

    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "[Assimp] IsExtensionSupported: fbx=%d gltf=%d glb=%d\n",
        fbx ? 1 : 0, gltf ? 1 : 0, glb ? 1 : 0);
    OutputDebugStringA(buf);
}


Model::ModelData Model::LoadAssimpFile(const std::string& fullPath)
{

    PrintLoadedAssimpPath_();

    ModelData out{};

    // 1) パス確認
    {
        OutputDebugStringA(("[Assimp] fullPath = " + fullPath + "\n").c_str());
        if (!std::filesystem::exists(fullPath)) {
            OutputDebugStringA("[Assimp] FILE NOT FOUND\n");
            return out;
        }
    }

    {
        std::ifstream f(fullPath, std::ios::binary);
        char magic[4]{};
        if (f.read(magic, 4)) {
            char buf[128];
            std::snprintf(buf, sizeof(buf),
                "[glb header] %02X %02X %02X %02X (%c%c%c%c)\n",
                (unsigned char)magic[0], (unsigned char)magic[1],
                (unsigned char)magic[2], (unsigned char)magic[3],
                magic[0], magic[1], magic[2], magic[3]);
            OutputDebugStringA(buf);
        }
    }


    Assimp::Importer importer;

    {
        bool gltf = importer.IsExtensionSupported("gltf");
        bool glb = importer.IsExtensionSupported("glb");
        const bool fbx = importer.IsExtensionSupported("fbx");

        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "[Assimp] IsExtensionSupported: fbx=%d gltf=%d glb=%d\n",
            fbx ? 1 : 0, gltf ? 1 : 0, glb ? 1 : 0);
        OutputDebugStringA(buf);
    }

    // 2) 拡張子サポート確認（glb/gltf が 0 なら Assimp 側の問題）
    DebugAssimpSupport_(importer);

    unsigned int flags = 0;
    flags |= aiProcess_Triangulate;
    flags |= aiProcess_JoinIdenticalVertices;
    flags |= aiProcess_GenSmoothNormals;
    // flags |= aiProcess_FlipUVs; // ← 自前で 1-v するなら付けない

    const aiScene* scene = nullptr;

    try {
        scene = importer.ReadFile(fullPath.c_str(), flags);
    } catch (...) {
        OutputDebugStringA("[Assimp] ReadFile threw an exception\n");
        std::string err = importer.GetErrorString();
        if (!err.empty()) {
            OutputDebugStringA(("[Assimp] GetErrorString: " + err + "\n").c_str());
        }
        return out;
    }

    if (!scene || !scene->HasMeshes()) {
        OutputDebugStringA(("[Assimp] ReadFile failed: " + std::string(importer.GetErrorString()) + "\n").c_str());
        return out;
    }


    // --- directory (for texture paths) ---
    std::filesystem::path p(fullPath);
    std::string dir = p.parent_path().string();

    // --- root node ---
    if (scene->mRootNode) {
        out.rootNode = ReadNodeRecursive(scene->mRootNode);
    } else {
        out.rootNode.localMatrix = Matrix4x4::MakeIdentity4x4();
        out.rootNode.name = "root";
    }

    // --- materials ---
    out.materials.clear();
    out.materials.resize(scene->mNumMaterials);

    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        const aiMaterial* mtl = scene->mMaterials[i];
        aiString texPath;

        if (mtl->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {

            std::string t = texPath.C_Str();

            // ★ 埋め込みテクスチャ（"*0" みたいなやつ）
            if (!t.empty() && t[0] == '*') {

                const aiTexture* emb = scene->GetEmbeddedTexture(texPath.C_Str());
                if (emb && emb->mHeight == 0 && emb->mWidth > 0) {
                    // mHeight==0: 圧縮データ（png/jpg等）が pcData に入ってる
                    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(emb->pcData);
                    const size_t   size = static_cast<size_t>(emb->mWidth);

                    // ★キーは一意なら何でもOK（モデルごとに衝突しないよう dir を混ぜる）
                    std::string key = dir + "/__emb" + t;   // 例: ".../__emb*0"

                    TextureManager::GetInstance()->LoadTextureFromMemory(key, bytes, size);

                    out.materials[i].textureFilePath = key; // ★ここ重要：後段は key を使う
                } else {
                    out.materials[i].textureFilePath.clear(); // 読めないなら白へ
                }

            } else {
                // 外部ファイル
                out.materials[i].textureFilePath = dir + "/" + t;
            }

        } else {
            out.materials[i].textureFilePath.clear();
        }

    }

    // --- meshes ---
    out.meshes.clear();
    out.meshes.reserve(scene->mNumMeshes);

    aiVector3D zero3(0, 0, 0);

    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
        const aiMesh* mesh = scene->mMeshes[mi];

        MeshData md{};
        md.materialIndex = mesh->mMaterialIndex;

        // base vertices
        std::vector<VertexData> base(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            const aiVector3D* pos = &mesh->mVertices[i];
            const aiVector3D* nrm = mesh->HasNormals() ? &mesh->mNormals[i] : &zero3;
            const aiVector3D* uv = mesh->HasTextureCoords(0) ? &mesh->mTextureCoords[0][i] : &zero3;

            VertexData v{};
            v.position = { pos->x, pos->y, pos->z, 1.0f };

            // ★FlipUVs を flags に付けてない前提で自前反転
            v.texcoord = { uv->x, 1.0f - uv->y };

            v.normal = { nrm->x, nrm->y, nrm->z };
            base[i] = v;
        }

        // tri list expand
        md.vertices.reserve(mesh->mNumFaces * 3);
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;

            VertexData v0 = base[face.mIndices[0]];
            VertexData v1 = base[face.mIndices[1]];
            VertexData v2 = base[face.mIndices[2]];

            // あなたの左手系寄せ（v2,v1,v0）
            md.vertices.push_back(v2);
            md.vertices.push_back(v1);
            md.vertices.push_back(v0);
        }

        out.meshes.push_back(std::move(md));
    }

    return out;
}

const Matrix4x4& Model::GetRootLocalMatrix() const
{
    static Matrix4x4 I = Matrix4x4::MakeIdentity4x4();
    return I;
}
