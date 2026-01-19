#include "Object3d.h"
#include "Object3dCommon.h"


//Vector3 Normalize(const Vector3& v) {
//	float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
//	if (length == 0.0f) return { 0.0f, 0.0f, 0.0f };
//	return { v.x / length, v.y / length, v.z / length };
//}


void Object3d::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dx) {
	// 初期化処理
	this->object3dCommon = object3dCommon;

	dx_ = dx;

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
	//directionalLightData->direction = Matrix4x4::Normalize(Vector3({ 0.0f, -1.0f, 0.0f }));//ライトの向き
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 0.0f; // ライトの強度


	//Transform変数
	transform = { {1.0f,1.0f,1.0f},
				  {0.0f,0.0f,0.0f},
				  {0.0f,0.0f,0.0f} };
	cameraTransform = { {1.0f,1.0f,1.0f},
						{0.3f,0.0f,0.0f},
						{0.0f,4.0f,-10.0f} };

	this->camera_ = object3dCommon->GetDefaultCamera();

	cameraResource_ = dx_->CreateBufferResource(sizeof(CameraGPU));
	cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));

	pointLightResource_ = dx_->CreateBufferResource(sizeof(PointLight));
	pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));

	// 初期値（とりあえず）
	pointLightData_->color = { 1,1,1,1 };
	pointLightData_->position = { 0, 3, 0 };
	pointLightData_->intensity = 1.0f;
	pointLightData_->radius = 10.0f;
	pointLightData_->decay = 1.0f;

	//スポットライト
	spotLightResource_ = dx_->CreateBufferResource(sizeof(SpotLight));
	spotLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData_));

	// スポットライト 初期値
	spotLightData_->color = { 1,1,1,1 };
	spotLightData_->position = { 0, 3, 0 };
	spotLightData_->intensity = 1.0f;
	spotLightData_->direction = { 0, -1, 0 }; // ※正規化しておくのが理想
	spotLightData_->distance = 10.0f;
	spotLightData_->decay = 1.0f;

	// 例：内側30度のcos（ラジアンにしてcos）
	spotLightData_->cosAngle = std::cosf(30.0f * (3.14159265f / 180.0f));

}

void Object3d::Update() {

	Matrix4x4 worldMatrixModel = Matrix4x4::MakeAffineMatrix(
		transform.scale,
		transform.rotate,
		transform.translate
	);

	// ★ まだ未設定なら default を取り直す（保険）
	if (!camera_) {
		camera_ = object3dCommon->GetDefaultCamera();
	}

	Matrix4x4 wvpModel = worldMatrixModel;

	if (camera_) {
		const Matrix4x4& vp = camera_->GetViewProjectionMatrix(); // View*Proj
		wvpModel = Matrix4x4::Multiply(worldMatrixModel, vp);     // World * (ViewProj)
	}

	transformationMatrixDataModel->WVP = wvpModel;
	transformationMatrixDataModel->World = worldMatrixModel;

	// ★非均一スケール対応：WorldInverseTranspose
	Matrix4x4 invW = Matrix4x4::Inverse(worldMatrixModel);
	transformationMatrixDataModel->WorldInverseTranspose = Matrix4x4::Transpose(invW);
}



void Object3d::Draw() {

	if (cameraData_ && camera_) {
		cameraData_->worldPosition = camera_->GetTranslate();
	}

	auto* cmd = dx_->GetCommandList();

	// ★ SRVヒープを必ずセット（これがないとテクスチャが読めない）
	ID3D12DescriptorHeap* heaps[] = {
		TextureManager::GetInstance()->GetSrvDescriptorHeap()
	};
	cmd->SetDescriptorHeaps(_countof(heaps), heaps);

	// Transform（WVP/World）
	cmd->SetGraphicsRootConstantBufferView(
		1, transformationMatrixResourceModel->GetGPUVirtualAddress());

	// DirectionalLight
	cmd->SetGraphicsRootConstantBufferView(
		3, directionalLightResource->GetGPUVirtualAddress());

	cmd->SetGraphicsRootConstantBufferView(
		4, cameraResource_->GetGPUVirtualAddress());

	cmd->SetGraphicsRootConstantBufferView(5, pointLightResource_->GetGPUVirtualAddress());

	cmd->SetGraphicsRootConstantBufferView(6, spotLightResource_->GetGPUVirtualAddress());

	// Model
	if (model_) {
		model_->Draw(cmd);
	}
}



void Object3d::SetModel(const std::string& filePath) {
	//モデルを検索してセットする
	auto* mgr = ModelManager::GetInstance();

	// まず探す
	Model* m = mgr->FindModel(filePath);

	// なければロードして再取得
	if (!m) {
		mgr->LoadModel(filePath);
		m = mgr->FindModel(filePath);
	}

	model_ = m;

}

void Object3d::SetDirection(const Vector3& direction)
{
	if (!directionalLightData) return;

	Vector3 d = direction;
	float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);

	if (len < 1e-6f) {
		d = { 0.0f, -1.0f, -1.0f }; // 0ベクトル保険
		len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
	}

	d.x /= len; d.y /= len; d.z /= len;
	directionalLightData->direction = d;
}
