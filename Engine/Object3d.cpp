#include "Object3d.h"
#include "Object3dCommon.h"


Vector3 Normalize(const Vector3& v) {
	float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	if (length == 0.0f) return { 0.0f, 0.0f, 0.0f };
	return { v.x / length, v.y / length, v.z / length };
}


void Object3d::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dx) {
	// 初期化処理
	this->object3dCommon = object3dCommon;

	dx_ = dx;

	//モデル読み込み
	modelData = LoadObjFile("resources", "plane.obj");

	// ===== 頂点バッファ作成 =====
	const size_t vtxCount = modelData.vertices.size();
	assert(vtxCount > 0 && "modelData.vertices が空です（OBJのロード前に Initialize してない？）");

	// GPU上に頂点バッファを確保
	vertexResource_ = dx->CreateBufferResource(sizeof(VertexData) * vtxCount);

	// CPUから書き込むためにMap
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	// OBJから読み込んだ頂点配列を丸ごとコピー
	std::memcpy(vertexData_, modelData.vertices.data(), sizeof(VertexData) * vtxCount);
	// 必要なら Unmap してもOK（UPLリソースなら常時Mapでも可）
	// vertexResource_->Unmap(0, nullptr);

	// ビュー設定
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vtxCount);
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// デバッグ確認（任意）
	OutputDebugStringA((std::string("[Object3d] vertex count=") + std::to_string(vtxCount) + "\n").c_str());

	materialResource_ = dx->CreateBufferResource(sizeof(Material));
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	// 初期値（スライド通り）
	materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->enableLighting = 0; // まずは Unlit（必要に応じて 1/2 に）
	materialData_->uvTransform = Matrix4x4::MakeIdentity4x4();

	transformationMatrixResourceModel= dx->CreateBufferResource(sizeof(TransformationMatrix));
	//書き込むためのアドレスを取得
	transformationMatrixResourceModel->Map(0, nullptr,
		reinterpret_cast<void**>(&transformationMatrixDataModel));
	//単位行列を書き込んでおく
	transformationMatrixDataModel->WVP = Matrix4x4::MakeIdentity4x4();
	transformationMatrixDataModel->World = Matrix4x4::MakeIdentity4x4();

	//平行光源
	directionalLightResource = dx->CreateBufferResource(sizeof(DirectionalLight));
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//初期化
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // ライトの色
	directionalLightData->direction = Normalize({ 0.0f, -1.0f, 0.0f });//ライトの向き
	directionalLightData->intensity = 1.0f; // ライトの強度

	//.objの参照知っているテクスチャファイル読み込み
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	//読み込んだテクスチャ番号の取得
	modelData.material.textureIndex =
		TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);

	//Transfrom変数
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	cameraTransform = { {1.0f,1.0f,1.0f},{0.3f,0.0f,0.0f},{0.0f,4.0f,-10.0f} };

	OutputDebugStringA(("[Object3d] vertex count = " +
		std::to_string(modelData.vertices.size()) + "\n").c_str());


}

void Object3d::Update() {

	//===========
		//モデルの計算
		//===========
	Matrix4x4 worldMatrixModel = Matrix4x4::MakeAffineMatrix(
		transform.scale,
		transform.rotate,
		transform.translate
	);

	transform.rotate.y += 0.5f;

	Matrix4x4 cameraMatrixModel = Matrix4x4::MakeAffineMatrix(
		cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
	Matrix4x4 viewMatrixModel = Matrix4x4::Inverse(cameraMatrixModel);

	Matrix4x4 projectionMatrixModel = Matrix4x4::PerspectiveFov(
		0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);

	Matrix4x4 wvpModel = Matrix4x4::Multiply(
		worldMatrixModel,
		Matrix4x4::Multiply(viewMatrixModel, projectionMatrixModel)
	);

	transformationMatrixDataModel->WVP = wvpModel;
	transformationMatrixDataModel->World = worldMatrixModel;



}

void Object3d::Draw() {
	auto* cmd = dx_->GetCommandList();

	cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// ★ slot0: Material（Pixel Shader b0）
	cmd->SetGraphicsRootConstantBufferView(
		0, materialResource_->GetGPUVirtualAddress());

	// ★ slot1: Transform（Vertex Shader b0：WVP/World）
	cmd->SetGraphicsRootConstantBufferView(
		1, transformationMatrixResourceModel->GetGPUVirtualAddress());

	// ★ slot2: Texture (SRV t0)
	cmd->SetGraphicsRootDescriptorTable(
		2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData.material.textureIndex));

	// ★ slot3: DirectionalLight（Pixel Shader b1）
	cmd->SetGraphicsRootConstantBufferView(
		3, directionalLightResource->GetGPUVirtualAddress());

	cmd->DrawInstanced(static_cast<UINT>(modelData.vertices.size()), 1, 0, 0);
}


Object3d::MaterialData Object3d::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
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


Object3d::ModelData Object3d::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
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
