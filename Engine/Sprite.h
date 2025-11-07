#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Vector3.h"
#include "Matrix4x4.h"

class SpriteCommon;
class DirectXCommon;

class Sprite {
	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

public:
	void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dx);

	// === New: 位置と色（スライド準拠） ===
	const Vector2& GetPosition() const { return position_; }
	void SetPosition(const Vector2& p) { position_ = p; }
	// --- スケール/回転/色の I/F を公開 ---
	const Vector3& GetScale()   const { return scale_; }
	void SetScale(const Vector3& s) { scale_ = s; }

	const Vector3& GetRotation() const { return rotate_; } // ラジアン想定
	void SetRotation(const Vector3& r) { rotate_ = r; }

	const Vector4& GetColor() const { return color_; }
	void SetColor(const Vector4& c) { color_ = c; if (materialData_) materialData_->color = c; }

	// 便利：Z回転だけ度数で扱いたいとき
	void SetRotationZDegrees(float deg) {
		float rad = deg * (3.1415926535f / 180.0f);
		rotate_.z = rad;
	}
	float GetRotationZDegrees() const {
		return rotate_.z * (180.0f / 3.1415926535f);
	}

	// === 既存：UV, 行列など ===
	void SetUVTransform(const Matrix4x4& m) { if (materialData_) materialData_->uvTransform = m; }

	// 今まで通りのハンドル直指定も残す
	void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE srv) { srv_ = srv; srvSlot_ = UINT32_MAX; }

	// 新規：スロット番号だけ指定（例：1, 2, ...）
	void SetTextureSlot(uint32_t slot) { srvSlot_ = slot; }

	// === Updateは position_ を反映（座標-反映処理） ===
	void Update(const Matrix4x4& view, const Matrix4x4& proj);

	// === 引数なし Draw（内部でPSOとSRVをセット） ===
	void Draw();

private:
	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

	SpriteCommon* spriteCommon_ = nullptr;

	// バッファ類（既存）
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	D3D12_INDEX_BUFFER_VIEW  indexBufferView_{};

	// マテリアル（既存）
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;

	// 変換（既存）
	Microsoft::WRL::ComPtr<ID3D12Resource> transformResource_;
	TransformationMatrix* transformData_ = nullptr;

	// === New: 内部状態 ===
	Vector2 position_{ 0.0f, 0.0f };     // スライドの「座標」メンバ変数
	Vector3 scale_{ 1.0f, 1.0f, 1.0f };  // 必要ならsetter追加してOK
	Vector3 rotate_{ 0.0f, 0.0f, 0.0f }; // 必要ならsetter追加してOK
	Vector4 color_{ 1,1,1,1 };

	DirectXCommon* dx_ = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE srv_{}; // Drawで使用
	uint32_t srvSlot_ = UINT32_MAX; // UINT32_MAXなら未指定扱い
};