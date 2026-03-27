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

void TrailManager::Update(const Vector3& tipPos, const Vector3& basePos) {
	// 1. 最新座標の追加（既存通り）
	points_.push_front({ tipPos, basePos });
	if (points_.size() > kMaxPoints) {
		points_.pop_back();
	}

	if (points_.size() < 4) return; // 補間には最低4点必要

	uint32_t vertexCount = 0;

	// 2. 記録された座標の間を補間していく
	// points_[i] と points_[i+1] の間を補間する
	for (size_t i = 0; i < points_.size() - 1; ++i) {

		// Catmull-Rom用の4点を取得（端の処理：存在しないインデックスはクランプする）
		size_t i0 = (i == 0) ? 0 : i - 1;
		size_t i1 = i;
		size_t i2 = i + 1;
		size_t i3 = (i + 2 >= points_.size()) ? points_.size() - 1 : i + 2;

		for (uint32_t j = 0; j < kInterpolationSteps; ++j) {
			float t = (float)j / (float)kInterpolationSteps;

			// 全体を通した割合（0.0〜1.0）
			float globalRatio = (float)(i * kInterpolationSteps + j) / (float)((points_.size() - 1) * kInterpolationSteps);
			float alpha = 1.0f - globalRatio;

			// スプライン補間実行
			Vector3 interpolatedTip = CatmullRom(points_[i0].tip, points_[i1].tip, points_[i2].tip, points_[i3].tip, t);
			Vector3 interpolatedBase = CatmullRom(points_[i0].base, points_[i1].base, points_[i2].base, points_[i3].base, t);

			// 頂点バッファへ書き込み
			// 先端
			vertexData_[vertexCount].pos = interpolatedTip;
			vertexData_[vertexCount].color = { 1.0f, 1.0f, 1.0f, alpha };
			vertexData_[vertexCount].uv = { globalRatio, 0.0f };
			vertexCount++;

			// 根元
			vertexData_[vertexCount].pos = interpolatedBase;
			vertexData_[vertexCount].color = { 1.0f, 1.0f, 1.0f, alpha };
			vertexData_[vertexCount].uv = { globalRatio, 1.0f };
			vertexCount++;
		}
	}

	// 描画時に使う頂点数を保存しておく
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

Vector3 TrailManager::Transform(const Vector3& v, const Matrix4x4& m)
{

	Vector3 result{
		v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
		v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
		v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2],
	};

	return result;
}
