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
	directionalLightData->direction = Matrix4x4::Normalize(Vector3({ 0.0f, -1.0f, 0.0f }));//ライトの向き
	directionalLightData->intensity = 1.0f; // ライトの強度


	//Transform変数
	transform = { {1.0f,1.0f,1.0f},
				  {0.0f,0.0f,0.0f},
				  {0.0f,0.0f,0.0f} };
	cameraTransform = { {1.0f,1.0f,1.0f},
						{0.3f,0.0f,0.0f},
						{0.0f,4.0f,-10.0f} };
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

	//transform.rotate.y += 0.5f;

	Matrix4x4 cameraMatrixModel = Matrix4x4::MakeAffineMatrix(
		cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
	Matrix4x4 viewMatrixModel = Matrix4x4::Inverse(cameraMatrixModel);

	Matrix4x4 projectionMatrixModel = Matrix4x4::PerspectiveFov(
		0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 20000.0f);

	Matrix4x4 wvpModel = Matrix4x4::Multiply(
		worldMatrixModel,
		Matrix4x4::Multiply(viewMatrixModel, projectionMatrixModel)
	);

	transformationMatrixDataModel->WVP = wvpModel;
	transformationMatrixDataModel->World = worldMatrixModel;



}
void Object3d::Draw() {
	auto* cmd = dx_->GetCommandList();

	// Transform（WVP/World）
	cmd->SetGraphicsRootConstantBufferView(
		1, transformationMatrixResourceModel->GetGPUVirtualAddress());

	// DirectionalLight
	cmd->SetGraphicsRootConstantBufferView(
		3, directionalLightResource->GetGPUVirtualAddress());

	// ★ Model がセットされていたら描画する
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