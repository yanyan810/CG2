#include "Particle.h"
#include "ParticleCommon.h"


//Vector3 Normalize(const Vector3& v) {
//	float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
//	if (length == 0.0f) return { 0.0f, 0.0f, 0.0f };
//	return { v.x / length, v.y / length, v.z / length };
//}


void Particle::Initialize(ParticleCommon* particleCommon, DirectXCommon* dx) {
	// 初期化処理
	this->particleCommon = particleCommon;
	dx_ = dx;

	// ※ Transform 用の CBV は instancing では使わないので作らない or 残っていても OK
	//   ここではもう作らないことにします。

	// 平行光源
	directionalLightResource = dx->CreateBufferResource(sizeof(DirectionalLight));
	directionalLightResource->Map(0, nullptr,
		reinterpret_cast<void**>(&directionalLightData));
	// 初期化
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = Matrix4x4::Normalize({ 0.0f, -1.0f, 0.0f });
	directionalLightData->intensity = 1.0f;

	// Transform 変数
	transform = { {1.0f,1.0f,1.0f},{0.0f,1.55f,0.0f},{0.0f,0.0f,0.0f} };
	cameraTransform = { {1.0f,1.0f,1.0f},{0.3f,0.0f,0.0f},{0.0f,4.0f,-20.0f} };

	// ===== instancing 用 StructuredBuffer<Resource> =====
	instancingResource_ =
		dx->CreateBufferResource(sizeof(TransformationMatrix) * kMaxInstance);
	instancingResource_->Map(0, nullptr,
		reinterpret_cast<void**>(&instancingData_));
	// ---- SRV の作成 ----
	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = kMaxInstance;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(TransformationMatrix);

	// SRVヒープの好きな空き番号を決める（ここでは 3）
	const uint32_t instancingSrvIndex = 3;

	// CPUハンドル取得（DirectXCommon が計算してくれる）
	D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvCPU =
		dx_->GetSRVCPUDescriptorHandle(instancingSrvIndex);

	// GPUハンドル取得（描画時に使うため保存）
	instancingSrvHandleGPU_ =
		dx_->GetSRVGPUDescriptorHandle(instancingSrvIndex);

	// SRV を作成
	dx_->GetDevice()->CreateShaderResourceView(
		instancingResource_.Get(),
		&instancingSrvDesc,
		instancingSrvCPU);


	// 初期値として、インスタンス数 1 個を使う
	instanceCount_ = 1;
}

void Particle::Update() {

	// --- ビュー・プロジェクション計算 ---
	Matrix4x4 cameraMatrix =
		Matrix4x4::MakeAffineMatrix(
			cameraTransform.scale,
			cameraTransform.rotate,
			cameraTransform.translate);
	Matrix4x4 viewMatrix = Matrix4x4::Inverse(cameraMatrix);

	Matrix4x4 projMatrix =
		Matrix4x4::PerspectiveFov(
			0.45f,
			float(WinApp::kClientWidth) / float(WinApp::kClientHeight),
			0.1f, 100.0f);

	Matrix4x4 vp = Matrix4x4::Multiply(viewMatrix, projMatrix);

	// --- 3D グリッド状に配置（例: 4×4×4 個）---
	const uint32_t numX = 2;
	const uint32_t numY = 2;
	const uint32_t numZ = 4;
	const float spacing = 3.0f;      // 間隔

	instanceCount_ = numX * numY * numZ;
	if (instanceCount_ > kMaxInstance) {
		instanceCount_ = kMaxInstance;
	}

	uint32_t index = 0;
	for (uint32_t y = 0; y < numY; ++y) {
		for (uint32_t z = 0; z < numZ; ++z) {
			for (uint32_t x = 0; x < numX; ++x) {
				if (index >= instanceCount_) { break; }

				Transform t = transform; // 基本形

				// 中心が原点になるようオフセットして配置
				t.translate.x = (float(x) - (numX - 1) * 0.5f) * spacing;
				t.translate.y = (float(y) - (numY - 1) * 0.5f) * spacing;
				t.translate.z = (float(z) - (numZ - 1) * 0.5f) * spacing;

				Matrix4x4 world =
					Matrix4x4::MakeAffineMatrix(t.scale, t.rotate, t.translate);
				Matrix4x4 wvp = Matrix4x4::Multiply(world, vp);

				instancingData_[index].World = world;
				instancingData_[index].WVP = wvp;

				++index;
			}
		}
	}
}
void Particle::Draw() {
	auto* cmd = dx_->GetCommandList();

	// ★ instancing 用 StructuredBuffer<TransformationMatrix> の SRV を t0 にセット
	cmd->SetGraphicsRootDescriptorTable(1, instancingSrvHandleGPU_);

	// DirectionalLight (b1)
	cmd->SetGraphicsRootConstantBufferView(
		3, directionalLightResource->GetGPUVirtualAddress());

	if (model_) {
		// ★ インスタンス数を指定して描画
		model_->Draw(cmd, instanceCount_);
	}
}


void Particle::SetModel(const std::string& filePath) {
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