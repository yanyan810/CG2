#include "Model.h"
#include <sstream>

void Model::Initialize(ModelCommon* modelCommon,
    const std::string& directoryPath,
    const std::string& filename) {

    // 1) ModelCommon ポインタ保存
    modelCommon_ = modelCommon;

    // 2) dx を ModelCommon からもらう（GetDxCommon は自分の実装に合わせて）
    DirectXCommon* dx = modelCommon_->GetDxCommon();

    // 3) モデル読み込み
    modelData_ = LoadObjFile(directoryPath, filename);

    // ===== 頂点バッファ作成 =====
    const size_t vtxCount = modelData_.vertices.size();
    assert(vtxCount > 0);

    vertexResource_ = dx->CreateBufferResource(sizeof(VertexData) * vtxCount);

    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    std::memcpy(vertexData_,
        modelData_.vertices.data(),
        sizeof(VertexData) * vtxCount);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vtxCount);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // ===== マテリアルの初期化 =====
    materialResource_ = dx->CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    materialData_->enableLighting = 0;
    materialData_->uvTransform = Matrix4x4::MakeIdentity4x4();

    // ===== テクスチャ読み込み＆番号取得 =====
    TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);

    modelData_.material.textureIndex =
        TextureManager::GetInstance()
        ->GetTextureIndexByFilePath(modelData_.material.textureFilePath);

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


Model::ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;
	std::string line;

	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::istringstream s(line);
		std::string identifier;
		s >> identifier;

		if (identifier == "v") {
			Vector4 pos{};
			s >> pos.x >> pos.y >> pos.z;
			pos.w = 1.0f;
			pos.x *= -1.0f; // X反転
			positions.push_back(pos);

		} else if (identifier == "vt") {
			Vector2 texcoord{};
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);

		} else if (identifier == "vn") {
			Vector3 normal{};
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f; // X反転
			normals.push_back(normal);

		} else if (identifier == "f") {
			std::vector<VertexData> polygonVertices;
			std::string vertexDefinition;

			while (s >> vertexDefinition) {
				std::istringstream v(vertexDefinition);
				uint32_t indices[3]{};
				for (int i = 0; i < 3; ++i) {
					std::string index;
					std::getline(v, index, '/');
					indices[i] = std::stoi(index);
				}

				Vector4 pos = positions[indices[0] - 1];
				Vector2 texcoord = texcoords[indices[1] - 1];
				Vector3 normal = normals[indices[2] - 1];
				polygonVertices.push_back({ pos, texcoord, normal });
			}

			// 三角形ファンに変換（左手系巻き順：CCW）
			for (size_t i = 1; i + 1 < polygonVertices.size(); ++i) {
				modelData.vertices.push_back(polygonVertices[i + 1]);
				modelData.vertices.push_back(polygonVertices[i]);
				modelData.vertices.push_back(polygonVertices[0]);
			}

		} else if (identifier == "mtllib") {
			std::string mtlFile;
			s >> mtlFile;
			modelData.material = LoadMaterialTemplateFile(directoryPath, mtlFile);
		}
	}

	return modelData;
}
