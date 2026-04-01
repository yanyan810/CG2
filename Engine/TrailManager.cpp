#include "TrailManager.h"

void TrailManager::Initialize(DirectXCommon* dxcommon, Object3dCommon* object3dCommon, const std::string& textureFilePath) {

	object3dCommon_ = object3dCommon;

	dxCommon_ = dxcommon;

	textureFilePath_ = textureFilePath;

	// 1. 頂点リソースの作成（書き込み頻度が高いのでMapしたままにする）
	vertexResource_ = dxCommon_->CreateBufferResource(sizeof(TrailVertex) * kMaxVertices);

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(TrailVertex) * kMaxVertices;
	vertexBufferView_.StrideInBytes = sizeof(TrailVertex);

	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	// 2. 定数バッファ (ViewProjection用)
	constResource_ = dxCommon_->CreateBufferResource(sizeof(Matrix4x4));
	constResource_->Map(0, nullptr, reinterpret_cast<void**>(&constData_));
}

void TrailManager::Update(const Vector3& tipPos, const Vector3& basePos, const TrailConfig& config) {
	// 1. 履歴の追加
	points_.push_front({ tipPos, basePos });

	// Configで指定された長さで制限（最大値はバッファサイズ kMaxPoints に依存）
	uint32_t limit = (std::min)(config.maxPoints, kMaxPoints);
	while (points_.size() > limit) {
		points_.pop_back();
	}

	if (points_.size() < 4) return;

	uint32_t vertexCount = 0;
	// 分割数もConfigから取得
	uint32_t steps = (std::max)(1u, config.interpolationSteps);

	for (size_t i = 0; i < points_.size() - 1; ++i) {
		// ... (Catmull-Romの計算 ...

		// Catmull-Rom用の4点を取得（端の処理：存在しないインデックスはクランプする）
		size_t i0 = (i == 0) ? 0 : i - 1;
		size_t i1 = i;
		size_t i2 = i + 1;
		size_t i3 = (i + 2 >= points_.size()) ? points_.size() - 1 : i + 2;

		for (uint32_t j = 0; j < steps; ++j) {
			// バッファオーバーフロー防止
			if (vertexCount + 2 >= kMaxVertices) break;

			float t = (float)j / (float)steps;
			float globalRatio = (float)(i * steps + j) / (float)((points_.size() - 1) * steps);

			// 色をConfigのLerpで計算
			Vector4 color = Lerp(config.startColor, config.endColor, globalRatio);
			
			// スプライン補間実行
			Vector3 interpolatedTip = CatmullRom(points_[i0].tip, points_[i1].tip, points_[i2].tip, points_[i3].tip, t);
			Vector3 interpolatedBase = CatmullRom(points_[i0].base, points_[i1].base, points_[i2].base, points_[i3].base, t);

			vertexData_[vertexCount].pos = interpolatedTip;
			vertexData_[vertexCount].color = color;
			vertexData_[vertexCount].uv = { globalRatio, 0.0f };
			vertexCount++;

			vertexData_[vertexCount].pos = interpolatedBase;
			vertexData_[vertexCount].color = color;
			vertexData_[vertexCount].uv = { globalRatio, 1.0f };
			vertexCount++;
		}
	}
	currentVertexCount_ = vertexCount;
}

void TrailManager::Draw(const Matrix4x4& viewProjection) {
	if (points_.size() < 2) return;

	auto commandList = dxCommon_->GetCommandList();
	*constData_ = viewProjection; // カメラ行列を転送

	commandList->SetGraphicsRootSignature(dxCommon_->GetPSOTrail().root_.GetSignature().Get());
	commandList->SetPipelineState(dxCommon_->GetPSOTrail().graphicsState_.Get());

	// プリミティブトポロジを TRIANGLESTRIP に！
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// ルートパラメータの設定
	// 0: Material(仮に定数なしでもOKだが共通化のため), 1: VP行列, 2: テクスチャ
	commandList->SetGraphicsRootConstantBufferView(1, constResource_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureFilePath_));

	// 描画 (頂点数は points.size() * 2)
	commandList->DrawInstanced(currentVertexCount_, 1, 0, 0);
}

Vector3 TrailManager::CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t)
{
	float t2 = t * t;
	float t3 = t2 * t;

	return ((p1 * 2.0f) +
		(-p0 + p2) * t +
		(p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
		(-p0 + p1 * 3.0f - p2 * 3.0f + p3) * t3) * 0.5f;
}

Vector4 TrailManager::Lerp(const Vector4& start, const Vector4& end, float t)
{
	// 線形補間
	Vector4 result = start + (end - start) * t;
	// 補間結果を返す
	return result;
}

Vector3 TrailManager::Transform(const Vector3& v, const Matrix4x4& m)
{

	Vector3 result{
		v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
		v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
		v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2],
	};

	return result;
}

void TrailManager::UpdateImGui(TrailConfig& editingConfig) {
	ImGui::Begin("Trail Editor");
	
	ImGui::Checkbox("Preview Move", &isPreviewActive_);
	if (isPreviewActive_) {
		debugTimer_ += 0.05f; // スピード
		Vector3 tip = { cosf(debugTimer_) * 2.0f, 2.0f, sinf(debugTimer_) * 2.0f };
		Vector3 base = { cosf(debugTimer_) * 0.5f, -2.0f, sinf(debugTimer_) * 0.5f };
		this->Update(tip, base, editingConfig); // プレビュー更新
	}
	
	// プレビュー用の設定
	ImGui::Text("Base Settings");
	ImGui::ColorEdit4("StartColor", &editingConfig.startColor.x);
	ImGui::ColorEdit4("EndColor", &editingConfig.endColor.x);
	ImGui::DragInt("Max Points", (int*)&editingConfig.maxPoints, 2, kMaxPoints);
	ImGui::DragInt("Steps", (int*)&editingConfig.interpolationSteps, 1, 10);

	static char filename[64] = "sample.json";
	ImGui::InputText("Save Filename", filename, IM_ARRAYSIZE(filename));

	if (ImGui::Button("Save to JSON")) {
		SaveToJson(filename, editingConfig);
	}

	if (ImGui::Button("Load from JSON")) {
		LoadFromJson(filename, editingConfig);
	}

	ImGui::End();
}

void TrailManager::SaveToJson(const std::string& path, const TrailConfig& config)
{
	std::ofstream file("resources/trail/" + path);
	if (file.is_open()) {
		nlohmann::json j = config.ToJson();
		file << std::setw(4) << j << std::endl; // 見やすく整形して保存
	}
}

void TrailManager::LoadFromJson(const std::string& path, TrailConfig& config)
{
	std::ifstream file("resources/trail/" + path);
	if (file.is_open()) {
		nlohmann::json j;
		file >> j;
		config.FromJson(j);
	}
}
