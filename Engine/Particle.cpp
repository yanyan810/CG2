#include "Particle.h"
#include "ParticleCommon.h"
#include "imgui.h"

//Vector3 Normalize(const Vector3& v) {
//	float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
//	if (length == 0.0f) return { 0.0f, 0.0f, 0.0f };
//	return { v.x / length, v.y / length, v.z / length };
//}

// === AABB と点の当たり判定 ===
static bool IsCollision(const AABB& aabb, const Vector3& point) {
	if (aabb.min.x <= point.x && point.x <= aabb.max.x &&
		aabb.min.y <= point.y && point.y <= aabb.max.y &&
		aabb.min.z <= point.z && point.z <= aabb.max.z) {
		return true;
	}
	return false;
}

void Particle::Initialize(ParticleCommon* particleCommon, DirectXCommon* dx) {
	this->particleCommon = particleCommon;
	dx_ = dx;

	// ライト周りは今のままでOK
	directionalLightResource = dx->CreateBufferResource(sizeof(DirectionalLight));
	directionalLightResource->Map(0, nullptr,
		reinterpret_cast<void**>(&directionalLightData));
	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = Matrix4x4::Normalize(Vector3({ 0.0f,-1.0f,0.0f }));
	directionalLightData->intensity = 1.0f;

	// カメラ
	cameraTransform = {
	{1,1,1},
	{0, std::numbers::pi_v<float>, 0}, // 真後ろ向き
	{0,4, 20}        // 位置を反対側にする
	};

	// ===== instancing 用 StructuredBuffer =====
	instancingResource_ =
		dx->CreateBufferResource(sizeof(ParticleForGPU) * kMaxInstance);
	instancingResource_->SetName(L"ParticleInstancingBuffer");
	instancingResource_->Map(0, nullptr,
		reinterpret_cast<void**>(&instancingData_));
	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = kMaxInstance;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

	const uint32_t instancingSrvIndex = 3;
	D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvCPU =
		dx_->GetSRVCPUDescriptorHandle(instancingSrvIndex);
	instancingSrvHandleGPU_ =
		dx_->GetSRVGPUDescriptorHandle(instancingSrvIndex);

	dx_->GetDevice()->CreateShaderResourceView(
		instancingResource_.Get(),
		&instancingSrvDesc,
		instancingSrvCPU);


	// ランダム分布
	std::uniform_real_distribution<float> distPos(-10.0f, 10.0f);   // 位置のランダム範囲
	std::uniform_real_distribution<float> distVel(-0.2f, 0.2f);     // 速度のランダム範囲
	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);

	emitter.count = 3;
	emitter.frequency = 0.5f;
	emitter.frequencyTime = 0.0f;

	emitter.transform.translate = { 0.0f,0.0f,0.0f };
	emitter.transform.scale = { 1.0f,1.0f,1.0f };
	emitter.transform.rotate = { 0.0f,0.0f,0.0f };

	accelerationField.acceleration={ 15.0f,0.0f,0.0f };
	accelerationField.area.min = { -1.0f, -1.0f, -1.0f };
	accelerationField.area.max = { 1.0f,  1.0f,  1.0f };
}

Particle::ParticleData Particle::MakeNewParticle(std::mt19937& ramdomEngine, const Vector3& translate) {

	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);
	Vector3 randomTranslate{ distribution(randomEngine),distribution(randomEngine),distribution(randomEngine) };

	Particle::ParticleData particle;
	particle.transform.scale = { 1.0f, 1.0f, 1.0f };
	particle.transform.rotate = { 0.0f, 1.55f, 0.0f };
	particle.transform.translate = translate + randomTranslate;
	particle.color = { distColor(ramdomEngine),distColor(ramdomEngine),distColor(ramdomEngine) ,1.0f };
	particle.velocity = { distribution(ramdomEngine),distribution(ramdomEngine),distribution(ramdomEngine) };
	particle.lifeTime = distTime(randomEngine);
	particle.currentTime = 0;

	return particle;

}

void Particle::Update() {

	instanceCount_ = 0;

	// --- カメラ行列 ---
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

	// --- スライド通りの billboardMatrix ---
	Matrix4x4 backToFrontMatrix = Matrix4x4::RotateY(std::numbers::pi_v<float> *0.5f);
	Matrix4x4 billboardMatrix = Matrix4x4::Multiply(backToFrontMatrix, cameraMatrix);
	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;

	// ==== list を回しながら消す（スライド通り）====
	for (std::list<ParticleData>::iterator particleIterator = particles.begin();
		particleIterator != particles.end() && instanceCount_ < kMaxInstance; )
	{
		ParticleData& p = *particleIterator;

		// 寿命を超えたら消す
		if (p.lifeTime <= p.currentTime) {
			particleIterator = particles.erase(particleIterator); // erase した戻り値で次要素
			continue;
		}
		// 生存中なら更新
		p.currentTime += deltaTime;

		// === Field の範囲内にいるかチェックして加速度を適用 ===
		if (IsCollision(accelerationField.area, p.transform.translate)) {
			// v = v + a * dt
			p.velocity += accelerationField.acceleration * deltaTime;
		}

		// 位置更新 x = x + v * dt
		p.transform.translate += p.velocity * deltaTime;

		// 寿命に応じて alpha
		float alpha = 1.0f - (p.currentTime / p.lifeTime);

		// 行列計算（scale * billboard * translate）
		Matrix4x4 scaleM = Matrix4x4::MakeScaleMatrix(p.transform.scale);
		Matrix4x4 translateM = Matrix4x4::Translation(p.transform.translate);
		Matrix4x4 world =
			Matrix4x4::Multiply(
				Matrix4x4::Multiply(scaleM, billboardMatrix),
				translateM);
		Matrix4x4 wvp = Matrix4x4::Multiply(world, vp);

		// ==== GPU へ書き込む前に最大数チェック ====
		if (instanceCount_ >= kMaxInstance) {
			break;  // or ループを抜ける
		}

		// CPU へ書き込み
		instancingData_[instanceCount_].World = world;
		instancingData_[instanceCount_].WVP = wvp;
		instancingData_[instanceCount_].color = p.color;
		instancingData_[instanceCount_].color.w = alpha;

		++instanceCount_;
		++particleIterator; // 次へ
	}
}

void Particle::SpawnParticle() {

	emitter.frequencyTime += deltaTime;
	if (emitter.frequency <= emitter.frequencyTime) {
		particles.splice(particles.end(), Emit(emitter, randomEngine));
		emitter.frequencyTime -= emitter.frequency;

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

void Particle::DebugImGui() {
	ImGui::Begin("Particle Camera");

	// 位置
	ImGui::DragFloat3(
		"Camera Pos",
		&cameraTransform.translate.x,
		0.1f
	);

	// 回転（ラジアンそのまま）
	ImGui::DragFloat3(
		"Camera Rot (rad)",
		&cameraTransform.rotate.x,
		0.01f
	);

	// 距離だけ別スライダーでいじりたいなら
	// Zだけ出して、変更を反映する方法もあり
	float dist = cameraTransform.translate.z;
	if (ImGui::SliderFloat("Dist(Z)", &dist, -100.0f, 100.0f)) {
		cameraTransform.translate.z = dist;
	}

	ImGui::DragFloat3("EmitterTranslate", &emitter.transform.translate.x, 0.01f, -100.0f, 100.0f);

	if (ImGui::Button("AddParticle")) {
		particles.splice(particles.end(), Emit(emitter, randomEngine));
	}

	ImGui::Separator();
	ImGui::Text("Acceleration Field");

	ImGui::DragFloat3("Field Min", &accelerationField.area.min.x, 0.1f);
	ImGui::DragFloat3("Field Max", &accelerationField.area.max.x, 0.1f);
	ImGui::DragFloat3("Field Accel", &accelerationField.acceleration.x, 0.1f);


	ImGui::End();
}

std::list<Particle::ParticleData> Particle::Emit(const Particle::Emitter& emitter, std::mt19937& randomEngine) {
	std::list<ParticleData> result;

	for (uint32_t count = 0; count < emitter.count; ++count) {
		result.push_back(MakeNewParticle(randomEngine, emitter.transform.translate));
	}

	return result;
}
