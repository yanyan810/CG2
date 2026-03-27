#pragma once
#include <vector>
#include <list>
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "ModelManager.h"
#include "Camera.h"
#include "DebugCamera.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>

extern std::mt19937 rng;

int Rand(int min, int max);
float Rand(float min, float max);
Vector2 Rand(const Vector2& min, const Vector2& max);
Vector3 Rand(const Vector3& min, const Vector3& max);
Vector4 Rand(const Vector4& min = { 0.0f, 0.0f, 0.0f, 1.0f }, const Vector4& max = { 1.0f, 1.0f, 1.0f, 1.0f });

Vector4 Lerp(const Vector4& start, const Vector4& end, float t);

struct ParticleEmitterConfig {
	Vector3 position = { 0, 0, 0 };
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

	// JSONオブジェクトに変換する
	nlohmann::json ToJson() const {
		return nlohmann::json{
			{"startColor", {startColor.x, startColor.y, startColor.z, startColor.w}},
			{"endColor", {endColor.x, endColor.y, endColor.z, endColor.w}},
			{"speedMin", speedMin},
			{"speedMax", speedMax},
			{"lifeTimeMin", lifeTimeMin},
			{"lifeTimeMax", lifeTimeMax},
			{"gravity", {gravity.x, gravity.y, gravity.z}},
			{"startScale", startScale},
			{"endScale", endScale}
		};
	}

	// JSONオブジェクトから読み込む
	void FromJson(const nlohmann::json& j) {
		if (j.contains("startColor")) {
			startColor = { j["startColor"][0], j["startColor"][1], j["startColor"][2], j["startColor"][3] };
		}
		if (j.contains("endColor")) {
			endColor = { j["endColor"][0], j["endColor"][1], j["endColor"][2], j["endColor"][3] };
		}
		speedMin = j.value("speedMin", speedMin);
		speedMax = j.value("speedMax", speedMax);
		lifeTimeMin = j.value("lifeTimeMin", lifeTimeMin);
		lifeTimeMax = j.value("lifeTimeMax", lifeTimeMax);
		if (j.contains("gravity")) {
			gravity = { j["gravity"][0], j["gravity"][1], j["gravity"][2] };
		}
		startScale = j.value("startScale", startScale);
		endScale = j.value("endScale", endScale);
	}
};

class ModelParticleManager {
public:
	struct ParticleGPU {
		Vector3 position;    float currentTime;      // 16 bytes
		Vector3 velocity;    float lifeTime;         // 16 bytes
		Vector3 acceleration; float startScale;       // 16 bytes
		Vector4 startColor;                          // 16 bytes
		Vector4 endColor;                            // 16 bytes
		float   endScale;    uint32_t isActive;
		Vector2 padding0;                            // 16 bytes (isActiveの後は8バイト余る)
		Vector3 rotate;      float padding1;         // 16 bytes
		Vector3 angularVelocity; float padding2;     // 16 bytes
	};

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

	struct GlobalConfig
	{
		float deltaTime;
		uint32_t maxParticles;
	};

	struct SceneConfig
	{
		Matrix4x4 viewProjection;
	};
	
	// シングルトン
	static ModelParticleManager* GetInstance();

	// パーティクルの最大数
	static const uint32_t kMaxInstance = 1000000;

	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
	void Dispatch(float deltaTime, Camera* camera);
	void Draw();

	// パーティクル発生
	void Emit(const Particle& particle);

	Particle MakeParticle(const ParticleEmitterConfig& config);

	void UpdateImGui(ParticleEmitterConfig& editingConfig);
	
	void SaveToJson(const std::string& path, const ParticleEmitterConfig& config);

	void LoadFromJson(const std::string& path, ParticleEmitterConfig& config);
	void EmitBatch(const std::vector<Particle>& particles);

private:
	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	// インスタンシング用リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
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
	
	Microsoft::WRL::ComPtr<ID3D12Resource> particleResource_; // 物理計算用 (UAV)
	uint32_t uavIndexParticles_; // UAV(u0)用インデックス
	uint32_t uavIndexRenderData_; // UAV(u1)用インデックス
	
	Microsoft::WRL::ComPtr<ID3D12Resource> computeConfigResource_;
	GlobalConfig* computeConfigData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> computeSceneResource_;
	SceneConfig* computeSceneData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> emitStagingResource_;

	uint32_t freeIndex_ = 0;
};