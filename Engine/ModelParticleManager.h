#pragma once
#include <vector>
#include <list>
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "ModelManager.h"
#include "Camera.h"
#include "DebugCamera.h"

extern std::mt19937 rng;

int Rand(int min, int max);
float Rand(float min, float max);
Vector2 Rand(const Vector2& min, const Vector2& max);
Vector3 Rand(const Vector3& min, const Vector3& max);
Vector4 Rand(const Vector4& min = { 0.0f, 0.0f, 0.0f, 1.0f }, const Vector4& max = { 1.0f, 1.0f, 1.0f, 1.0f });

Vector4 Lerp(const Vector4& start, const Vector4& end, float t);

struct ParticleEmitterConfig {
	Vector3 position = { 0, 0, 0 };
	Vector4 baseColor = { 1, 1, 1, 1 };
	float speedMin = 0.0f;
	float speedMax = 1.0f;
	float lifeTimeMin = 2.0f;
	float lifeTimeMax = 3.0f;
	Vector3 gravity = { 0.0f, -0.08f, 0.0f };

	// イージング用に追加
	float startScale = 1.0f;
	float endScale = 0.0f;

	Vector4 startColor = { 1, 1, 1, 1 };
	Vector4 endColor = { 1, 1, 1, 0 };

	// 炎：下から上へ昇りながら小さくなって消える
	static ParticleEmitterConfig CreateFire(const Vector3& pos) {
		ParticleEmitterConfig config;
		config.position = pos;
		config.baseColor = { 1.0f, 0.4f, 0.1f, 1.0f }; // 鮮やかなオレンジ
		config.speedMin = 0.8f;
		config.speedMax = 1.5f;
		config.gravity = { 0.0f, 1.5f, 0.0f }; // 強い上昇気流
		config.lifeTimeMin = 0.4f;
		config.lifeTimeMax = 0.7f;
		config.startScale = 1.2f;
		config.endScale = 0.2f;
		config.startColor = { 1.0f, 0.4f, 0.1f, 1.0f };
		config.endColor = { 0.2f, 0.0f, 0.0f, 0.0f };
		return config;
	}

	// 水：放物線を描いて飛び散り、少し小さくなる
	static ParticleEmitterConfig CreateWater(const Vector3& pos) {
		ParticleEmitterConfig config;
		config.position = pos;
		config.baseColor = { 0.2f, 0.6f, 1.0f, 0.7f };
		config.speedMin = 2.0f;
		config.speedMax = 4.0f;
		config.gravity = { 0.0f, -9.8f, 0.0f }; // 重力で落下
		config.lifeTimeMin = 0.8f;
		config.lifeTimeMax = 1.2f;
		config.startScale = 0.5f;
		config.endScale = 0.3f;
		return config;
	}

	// 氷：キラキラと回転しながら停滞し、ゆっくり消える
	static ParticleEmitterConfig CreateIce(const Vector3& pos) {
		ParticleEmitterConfig config;
		config.position = pos;
		config.baseColor = { 0.7f, 0.9f, 1.0f, 1.0f }; // 薄い水色
		config.speedMin = 0.2f;
		config.speedMax = 0.5f;
		config.gravity = { 0.0f, -0.1f, 0.0f }; // ほとんど落ちない
		config.lifeTimeMin = 1.5f;
		config.lifeTimeMax = 2.5f;
		config.startScale = 0.0f; // 出現時は0
		config.endScale = 0.8f;   // 徐々に広がる
		return config;
	}
};

class ModelParticleManager {
public:

	struct Particle {
		Transform transform;
		Vector3 velocity;
		Vector3 acceleration;
		Vector3 angularVelocity;
		Vector3 kVelocity;
		Vector4 color;
		float lifeTime;
		float currentTime;
		// イージング用に追加
		float startScale = 1.0f;
		float endScale = 0.0f;

		Vector4 startColor = { 1, 1, 1, 1 };
		Vector4 endColor = { 1, 1, 1, 0 };
	};

	struct ModelParticleTransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Matrix4x4 WorldInverseTranspose;
		Vector4 color;
	};

	struct alignas(16) Material {
		Vector4 color;
		int32_t enableLighting;
		int32_t lightingMode;
		float padding[2];
		Matrix4x4 uvTransform;
		float shininess;
		float pad2[3];
	};

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Matrix4x4 WorldInverseTranspose;
	};

	struct DirectionalLight {
		Vector4 color; // ライトの色
		Vector3 direction; // ライトの方向
		float intensity; // ライトの光度
	};

	struct CameraData
	{
		Vector3 worldPosition;
		float padding; // 16byte アラインメント用（重要）
	};

	// シングルトン
	static ModelParticleManager* GetInstance();

	// パーティクルの最大数
	static const uint32_t kMaxInstance = 10000;

	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
	void Update(float deltaTime, Camera* camera);
	void Draw();

	// パーティクル発生
	void Emit(const Particle& particle);

	Particle MakeParticle(const ParticleEmitterConfig& config);

private:
	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	// インスタンシング用リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
	ModelParticleTransformationMatrix* instancingData_ = nullptr;
	uint32_t srvIndex_; // SrvManagerで割り当てられたインデックス

	// 使用するモデル（plane.objなど）
	Model* model_ = nullptr;

	// パーティクルリスト
	std::list<Particle> particles_;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;

	TransformationMatrix* transformationMatrixData;
	DirectionalLight* directionalLightData;

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
	CameraData* cameraData_ = nullptr;
};