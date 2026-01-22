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
static Model::Node ReadNodeRecursive(const aiNode* node) {
    Model::Node out{};
    out.name = node->mName.C_Str();

    // (1) Assimp行列 -> SRT に分解
    aiVector3D s, t;
    aiQuaternion r;
    node->mTransformation.Decompose(s, r, t);

    // (2) RH->LH 変換（あなたのエンジンが LH 前提なら）
    out.transform.scale = Vector3{ s.x, s.y, s.z };      // 多くはそのまま
    out.transform.rotate = { r.x,-r.y,-r.z,r.w };
    out.transform.translate = {-t.x,t.y,t.z};

    // (3) transform から localMatrix を再構築
    out.localMatrix = MakeAffineMatrix(
        out.transform.scale,
        out.transform.rotate,
        out.transform.translate
    );

    // mesh index
    out.meshIndices.reserve(node->mNumMeshes);
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        out.meshIndices.push_back((uint32_t)node->mMeshes[i]);
    }

    // children
    out.children.resize(node->mNumChildren);
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        out.children[i] = ReadNodeRecursive(node->mChildren[i]);
    }
    return out;
}


//AnumHelper
static Quaternion ConvertAssimpQuat(const aiQuaternion& q) {
    Quaternion out{};
    out.x = q.x;
    out.y = q.y;
    out.z = q.z;
    out.w = q.w;
    return out;
}

//========================
//キーフレーム読み取り
//========================

static void ReadVectorKeys(AnimationCurveVector3& outCurve, const aiVectorKey* keys, unsigned int count, double tps) {
    outCurve.keyframes.clear();
    outCurve.keyframes.reserve(count);

    const double inv = (tps > 0.0) ? (1.0 / tps) : (1.0 / 25.0); // tps==0対策

    for (unsigned int i = 0; i < count; ++i) {
        KeyframeVector3 k{};
        k.time = static_cast<float>(keys[i].mTime * inv);
        k.value = Vector3{ keys[i].mValue.x, keys[i].mValue.y, keys[i].mValue.z };
        outCurve.keyframes.push_back(k);
    }

    std::sort(outCurve.keyframes.begin(), outCurve.keyframes.end(),
        [](auto& a, auto& b) { return a.time < b.time; });
}

static void ReadQuatKeys(AnimationCurveQuaternion& outCurve, const aiQuatKey* keys, unsigned int count, double tps) {
    outCurve.keyframes.clear();
    outCurve.keyframes.reserve(count);

    const double inv = (tps > 0.0) ? (1.0 / tps) : (1.0 / 25.0);

    for (unsigned int i = 0; i < count; ++i) {
        KeyframeQuaternion k{};
        k.time = static_cast<float>(keys[i].mTime * inv);
        k.value = ConvertAssimpQuat(keys[i].mValue);
        outCurve.keyframes.push_back(k);
    }

    std::sort(outCurve.keyframes.begin(), outCurve.keyframes.end(),
        [](auto& a, auto& b) { return a.time < b.time; });
}

//アニメーション全体を読む
static void ReadAnimationsFromAssimp(Model::ModelData& out, const aiScene* scene) {
    out.animations.clear();

    if (!scene || scene->mNumAnimations == 0) { return; }

    for (unsigned int ai = 0; ai < scene->mNumAnimations; ++ai) {
        const aiAnimation* a = scene->mAnimations[ai];
        if (!a) continue;

        Animation clip{};
        const double tps = (a->mTicksPerSecond != 0.0) ? a->mTicksPerSecond : 25.0;
        clip.duration = static_cast<float>(a->mDuration / tps);

        // アニメ名（空のことがあるのでフォールバック）
        std::string animName = a->mName.C_Str();
        if (animName.empty()) {
            animName = "anim_" + std::to_string(ai);
        }

        // channels = ノードごとのTRSカーブ
        for (unsigned int ci = 0; ci < a->mNumChannels; ++ci) {
            const aiNodeAnim* ch = a->mChannels[ci];
            if (!ch) continue;

            std::string nodeName = ch->mNodeName.C_Str();
            if (nodeName.empty()) continue;

            NodeAnimation na{};

            if (ch->mNumPositionKeys > 0) {
                ReadVectorKeys(na.translate, ch->mPositionKeys, ch->mNumPositionKeys, tps);
            }
            if (ch->mNumRotationKeys > 0) {
                ReadQuatKeys(na.rotate, ch->mRotationKeys, ch->mNumRotationKeys, tps);
            }
            if (ch->mNumScalingKeys > 0) {
                ReadVectorKeys(na.scale, ch->mScalingKeys, ch->mNumScalingKeys, tps);
            }

            clip.nodeAnimations.emplace(std::move(nodeName), std::move(na));
        }

        out.animations.emplace(std::move(animName), std::move(clip));
    }
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
        // ★ここでSkeletonを作る（Assimp形式のみ）
        skeleton_ = CreateSkeleton(modelData_.rootNode);
        UpdateSkeleton(skeleton_); // bind pose の skeletonSpace を初期計算（1回だけ）

      
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
// IB 作成（全meshのindexを1本に連結）★追加
// ======================
    const size_t totalIdx = modelData_.indices.size();
    if (totalIdx == 0) {
        OutputDebugStringA("[Model] WARN: totalIdx == 0 (non-indexed). Skip creating IB.\n");
    }
    else {

        indexResource_ = dx->CreateBufferResource(sizeof(uint32_t) * totalIdx);
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

        // GPU上のIBは「VB連結後の頂点番号」に直したものを詰める
        uint32_t ibCursor = 0;

        for (auto& mesh : modelData_.meshes) {

            // mesh.startVertex は VB 連結で設定済み（vertexOffset）
            const uint32_t vOffset = mesh.startVertex;

            const uint32_t srcStart = mesh.startIndex;  // LoadAssimpFileで入れた ModelData::indices の範囲
            const uint32_t count = mesh.indexCount;

            // このmeshが使う GPU IB 範囲に付け替える
            mesh.startIndex = ibCursor;

            for (uint32_t k = 0; k < count; ++k) {
                const uint32_t localIndex = modelData_.indices[srcStart + k];
                indexData_[ibCursor + k] = localIndex + vOffset; // ★ここが超重要
            }

            ibCursor += count;
        }

        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * totalIdx);
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

        OutputDebugStringA(("[Model] total index count = " + std::to_string(totalIdx) + "\n").c_str());
    }


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


void Model::Draw(ID3D12GraphicsCommandList* cmd) {
    if (!vertexResource_ || !materialResource_ || !materialData_) {
        OutputDebugStringA("[Model] Draw skipped: resources not initialized\n");
        return;
    }
    if (!indexResource_) {
        OutputDebugStringA("[Model] Draw skipped: indexResource_ is null (not indexed)\n");
        return;
    }

    cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);
    cmd->IASetIndexBuffer(&indexBufferView_);

    cmd->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    for (const auto& mesh : modelData_.meshes) {

        std::string texPath;
        if (mesh.materialIndex < modelData_.materials.size()) {
            texPath = modelData_.materials[mesh.materialIndex].textureFilePath;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE handle{};
        bool useWhite = false;

        if (!texPath.empty()) {
            if (!TextureManager::GetInstance()->HasTexture(texPath)) {
                TextureManager::GetInstance()->LoadTexture(texPath);
            }
            handle = TextureManager::GetInstance()->GetSrvHandleGPU(texPath);
        }
        else {
            useWhite = true;
        }

        if (useWhite) {
            handle = TextureManager::GetInstance()->GetSrvHandleGPU(""); // 空→白
        }

        cmd->SetGraphicsRootDescriptorTable(2, handle);

        // ★Indexed
        cmd->DrawIndexedInstanced(
            mesh.indexCount,
            1,
            mesh.startIndex,
            0, // BaseVertexLocation は IB を vOffset 加算済みなので 0 でOK
            0
        );
    }
}




void Model::Draw(ID3D12GraphicsCommandList* cmd, uint32_t instanceCount) {
    if (!vertexResource_ || !materialResource_ || !materialData_) {
        OutputDebugStringA("[Model] Draw skipped: resources not initialized\n");
        return;
    }

    cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // ★IBがある場合だけセット（無い場合は非Indexed）
    const bool useIndexed = (indexResource_ != nullptr);
    if (useIndexed) {
        cmd->IASetIndexBuffer(&indexBufferView_);
    }

    cmd->SetGraphicsRootConstantBufferView(
        0, materialResource_->GetGPUVirtualAddress());

    for (const auto& mesh : modelData_.meshes) {

        // ---- texture ----
        std::string texPath;
        if (mesh.materialIndex < modelData_.materials.size()) {
            texPath = modelData_.materials[mesh.materialIndex].textureFilePath;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE handle{};
        bool useWhite = false;

        if (!texPath.empty()) {
            if (!TextureManager::GetInstance()->HasTexture(texPath)) {
                TextureManager::GetInstance()->LoadTexture(texPath);
            }
            handle = TextureManager::GetInstance()->GetSrvHandleGPU(texPath);
        }
        else {
            useWhite = true;
        }

        if (useWhite) {
            handle = TextureManager::GetInstance()->GetSrvHandleGPU("");
        }

        cmd->SetGraphicsRootDescriptorTable(2, handle);

        // ---- draw ----
        if (useIndexed && mesh.indexCount > 0) {
            // ★Indexed描画
            // IB詰めで index に vOffset 足してるなら BaseVertexLocation は 0 でOK
            cmd->DrawIndexedInstanced(
                mesh.indexCount,   // IndexCountPerInstance
                instanceCount,     // InstanceCount
                mesh.startIndex,   // StartIndexLocation
                0,                 // BaseVertexLocation
                0                  // StartInstanceLocation
            );
        }
        else {
            // ★非Indexed描画（従来）
            cmd->DrawInstanced(
                mesh.vertexCount,
                instanceCount,
                mesh.startVertex,
                0
            );
        }
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

        // ★ここに追加（絶対パス確認）
        OutputDebugStringA(
            ("[Assimp] abs = " +
                std::filesystem::absolute(fullPath).string() + "\n").c_str()
        );

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
    }
    catch (const std::exception& e) {
        OutputDebugStringA("[Assimp] Exception in ReadFile\n");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
        return out; // ★必ず抜ける
    }
    catch (...) {
        OutputDebugStringA("[Assimp] Unknown exception in ReadFile\n");
        std::string err = importer.GetErrorString();
        if (!err.empty()) {
            OutputDebugStringA(("[Assimp] GetErrorString: " + err + "\n").c_str());
        }
        return out; // ★必ず抜ける
    }

    if (!scene || !scene->HasMeshes()) {
        OutputDebugStringA(("[Assimp] ReadFile failed: " + std::string(importer.GetErrorString()) + "\n").c_str());
        return out;
    }

    {
        char buf[128];
        std::snprintf(buf, sizeof(buf),
            "[Scene] mNumMeshes=%u mNumAnimations=%u\n",
            scene->mNumMeshes, scene->mNumAnimations);
        OutputDebugStringA(buf);
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

    // --- animations ---
    ReadAnimationsFromAssimp(out, scene);

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
    out.indices.clear();

    aiVector3D zero3(0, 0, 0);

    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
        const aiMesh* mesh = scene->mMeshes[mi];

        if (mesh->HasBones()) {
            out.hasSkinning = true;
        }

        MeshData md{};
        md.materialIndex = mesh->mMaterialIndex;

        // ===== vertices: mNumVertices をそのまま =====
        md.vertices.resize(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            const aiVector3D* pos = &mesh->mVertices[i];
            const aiVector3D* nrm = mesh->HasNormals() ? &mesh->mNormals[i] : &zero3;
            const aiVector3D* uv = mesh->HasTextureCoords(0) ? &mesh->mTextureCoords[0][i] : &zero3;

            VertexData v{};
            v.position = { pos->x, pos->y, pos->z, 1.0f };
            v.normal = { nrm->x, nrm->y, nrm->z };
            v.texcoord = { uv->x, 1.0f - uv->y }; // 自前反転

            md.vertices[i] = v;
        }

        // ===== indices: faces から結合して out.indices に入れる =====
        md.startIndex = static_cast<uint32_t>(out.indices.size());
        md.indexCount = 0;

        out.indices.reserve(out.indices.size() + mesh->mNumFaces * 3);

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;

            const uint32_t i0 = static_cast<uint32_t>(face.mIndices[0]);
            const uint32_t i1 = static_cast<uint32_t>(face.mIndices[1]);
            const uint32_t i2 = static_cast<uint32_t>(face.mIndices[2]);

            // ★あなたの左手系寄せ（v2,v1,v0）を index で再現
            out.indices.push_back(i2);
            out.indices.push_back(i1);
            out.indices.push_back(i0);

            md.indexCount += 3;
        }

        out.meshes.push_back(std::move(md));
    }


    return out;
}

const Matrix4x4& Model::GetRootLocalMatrix() const {
    return modelData_.rootNode.localMatrix;
}

Model::Skeleton Model::CreateSkeleton(const Model::Node& rootNode) {

    Model::Skeleton skeleton;
    skeleton.root = CreateJoint(rootNode, {}, skeleton.joints);

    //名前とインデックスのマッピング
    for (const Model::Joint joint: skeleton.joints) {

        skeleton.jointMap.emplace(joint.name, joint.index);

    }

    return skeleton;

}

int32_t Model::CreateJoint(const Model::Node& node, const std::optional<int32_t>& parent, std::vector<Model::Joint>& joints) {

    Model::Joint joint;
    joint.name = node.name;
    joint.localMatrix = node.localMatrix;
    joint.skeletonSpaceMatrix = Matrix4x4::MakeIdentity4x4();
    joint.transform = node.transform;
    joint.index = int32_t(joints.size());
    joint.parent = parent;
    const int32_t myIndex = (int32_t)joints.size();
    joints.push_back(joint);
    for (const Model::Node& child : node.children) {

        //子jointを作成し、そのインデックスを取得
        int32_t childIndex = CreateJoint(child, myIndex, joints);
        joints[myIndex].children.push_back(childIndex);


    }

    return joint.index;

}

void Model::UpdateSkeleton(Skeleton& skeleton) {
    for (auto& joint : skeleton.joints) {

        // 1) Transform -> localMatrix
        joint.localMatrix = MakeAffineMatrix(
            joint.transform.scale,
            joint.transform.rotate,
            joint.transform.translate
        );

        // 2) local -> skeletonSpace（親から累積）
        if (joint.parent.has_value()) {
            const auto& parent = skeleton.joints[*joint.parent];

            // 行ベクトル想定：子 * 親
            joint.skeletonSpaceMatrix =
                Matrix4x4::Multiply(joint.localMatrix, parent.skeletonSpaceMatrix);
        }
        else {
            // root
            joint.skeletonSpaceMatrix = joint.localMatrix;
        }
    }
}
