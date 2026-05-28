#pragma once
#include <vector>
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "ModelManager.h"
#include "Camera.h"
#include "DebugCamera.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <map>

// エミッター形状
enum class EmitterShape : uint32_t {
	Point = 0,
	Sphere = 1,
	Box = 2,
};

// イージングタイプ
enum class EasingType : uint32_t {
	Linear = 0,
	EaseIn = 1,
	EaseOut = 2,
};

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
	float startScaleRandom = 0.0f;
	float endScaleRandom = 0.0f;

	Vector4 startColor = { 1, 1, 1, 1 };
	Vector4 endColor = { 1, 1, 1, 0 };
	Vector4 startColorRandom = { 0, 0, 0, 0 };
	Vector4 endColorRandom = { 0, 0, 0, 0 };

	Vector3 initialRotateMin = { 0.0f, 0.0f, 0.0f };
	Vector3 initialRotateMax = { 0.0f, 0.0f, 0.0f };
	Vector3 angularVelocityMin = { -5.0f, -5.0f, -5.0f };
	Vector3 angularVelocityMax = { 5.0f, 5.0f, 5.0f };

	// --- 新機能: モデルパス ---
	std::string modelPath = "triangleParticle.obj";
	std::string texturePath = "";
	bool useJewelShader = true;

	// --- 新機能: エミッター形状 ---
	EmitterShape emitterShape = EmitterShape::Point;
	Vector3 shapeSize = { 1.0f, 1.0f, 1.0f };

	// --- 新機能: イージングタイプ ---
	EasingType easingType = EasingType::Linear;

	// --- 新機能: ビルボード ---
	bool isBillboard = false;

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
			{"endScale", endScale},
			{"startScaleRandom", startScaleRandom},
			{"endScaleRandom", endScaleRandom},
			{"startColorRandom", {startColorRandom.x, startColorRandom.y, startColorRandom.z, startColorRandom.w}},
			{"endColorRandom", {endColorRandom.x, endColorRandom.y, endColorRandom.z, endColorRandom.w}},
			{"initialRotateMin", {initialRotateMin.x, initialRotateMin.y, initialRotateMin.z}},
			{"initialRotateMax", {initialRotateMax.x, initialRotateMax.y, initialRotateMax.z}},
			{"angularVelocityMin", {angularVelocityMin.x, angularVelocityMin.y, angularVelocityMin.z}},
			{"angularVelocityMax", {angularVelocityMax.x, angularVelocityMax.y, angularVelocityMax.z}},
			{"modelPath", modelPath},
			{"texturePath", texturePath},
			{"useJewelShader", useJewelShader},
			{"emitterShape", static_cast<int>(emitterShape)},
			{"shapeSize", {shapeSize.x, shapeSize.y, shapeSize.z}},
			{"easingType", static_cast<int>(easingType)},
			{"isBillboard", isBillboard}
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
		startScaleRandom = j.value("startScaleRandom", startScaleRandom);
		endScaleRandom = j.value("endScaleRandom", endScaleRandom);
		if (j.contains("startColorRandom")) {
			startColorRandom = { j["startColorRandom"][0], j["startColorRandom"][1], j["startColorRandom"][2], j["startColorRandom"][3] };
		}
		if (j.contains("endColorRandom")) {
			endColorRandom = { j["endColorRandom"][0], j["endColorRandom"][1], j["endColorRandom"][2], j["endColorRandom"][3] };
		}
		if (j.contains("initialRotateMin")) {
			initialRotateMin = { j["initialRotateMin"][0], j["initialRotateMin"][1], j["initialRotateMin"][2] };
		}
		if (j.contains("initialRotateMax")) {
			initialRotateMax = { j["initialRotateMax"][0], j["initialRotateMax"][1], j["initialRotateMax"][2] };
		}
		if (j.contains("angularVelocityMin")) {
			angularVelocityMin = { j["angularVelocityMin"][0], j["angularVelocityMin"][1], j["angularVelocityMin"][2] };
		}
		if (j.contains("angularVelocityMax")) {
			angularVelocityMax = { j["angularVelocityMax"][0], j["angularVelocityMax"][1], j["angularVelocityMax"][2] };
		}
		modelPath = j.value("modelPath", modelPath);
		texturePath = j.value("texturePath", texturePath);
		useJewelShader = j.value("useJewelShader", useJewelShader);
		if (j.contains("emitterShape")) {
			emitterShape = static_cast<EmitterShape>(j["emitterShape"].get<int>());
		}
		if (j.contains("shapeSize")) {
			shapeSize = { j["shapeSize"][0], j["shapeSize"][1], j["shapeSize"][2] };
		}
		if (j.contains("easingType")) {
			easingType = static_cast<EasingType>(j["easingType"].get<int>());
		}
		isBillboard = j.value("isBillboard", isBillboard);
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
		uint32_t easingType; uint32_t isBillboard;   // 16 bytes (イージング + ビルボードフラグ)
		Vector3 rotate;      float vortexAngularSpeed; // 16 bytes
		Vector3 angularVelocity; float vortexRadialSpeed; // 16 bytes
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
		float vortexAngularSpeed = 0.0f;
		float vortexRadialSpeed = 0.0f;

		// --- 新機能 ---
		EasingType easingType = EasingType::Linear;
		bool isBillboard = false;
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
		Vector3 cameraPosition;  // ビルボード用カメラ位置
		float scenePadding;
	};
	
	// シングルトン
	static ModelParticleManager* GetInstance();

	// パーティクルの最大数
	static const uint32_t kMaxInstance = 1000000;

	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, uint32_t maxInstances = kMaxInstance);
	void Dispatch(float deltaTime, Camera* camera);
	void Draw();
	void ClearParticles();

	// パーティクル発生
	void EmitBatch(const std::vector<Particle>& particles);

	Particle MakeParticle(const ParticleEmitterConfig& config);

	void UpdateImGui(const std::string& effectName, ParticleEmitterConfig& editingConfig);
	void SaveToJson(const std::string& path, const ParticleEmitterConfig& config);
	void LoadFromJson(const std::string& path, ParticleEmitterConfig& config);
	
	// --- 追加：エフェクトの事前登録 ---
	// これを呼ぶとJSONを読み込んで、名前（"fire"など）と紐づけて保存する
	void RegisterEffect(const std::string& effectName, const std::string& jsonPath);

	// --- 変更：名前指定でパーティクルを発生させる ---
	void Emit(const std::string& effectName, const Vector3& position, uint32_t count);
	void Emit(const std::string& effectName, const Vector3& position, uint32_t count, const Vector4& color);
private:
	void ApplyRenderConfig_(const ParticleEmitterConfig& config);
	void SetRenderModel_(const std::string& modelPath);
	void UpdateDrawVertexCount_(uint32_t vertexCount);
	std::string ResolveRenderTexturePath_() const;

	// エフェクト設定を名前で引けるようにする
	std::map<std::string, ParticleEmitterConfig> effectLibrary_;
	std::map<std::string, Model*> effectModels_;  // エフェクト名 → モデル

	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	// リソース類
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
	D3D12_RESOURCE_STATES instancingResourceState_ = D3D12_RESOURCE_STATE_COMMON;
	uint32_t srvIndex_;

	Model* model_ = nullptr;
	std::string currentModelPath_ = "triangleParticle.obj";
	std::string currentTexturePath_;
	bool currentUseJewelShader_ = true;

	// 定数バッファ関連
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
	CameraData* cameraData_ = nullptr;

	// GPU計算用 (UAV)
	Microsoft::WRL::ComPtr<ID3D12Resource> particleResource_;
	D3D12_RESOURCE_STATES particleResourceState_ = D3D12_RESOURCE_STATE_COMMON;
	uint32_t uavIndexParticles_;
	uint32_t uavIndexRenderData_;

	// Compute関連
	Microsoft::WRL::ComPtr<ID3D12Resource> computeConfigResource_;
	GlobalConfig* computeConfigData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> computeSceneResource_;
	SceneConfig* computeSceneData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> emitStagingResource_;

	// 間接描画用
	Microsoft::WRL::ComPtr<ID3D12CommandSignature> commandSignature_;
	Microsoft::WRL::ComPtr<ID3D12Resource> drawArgsResource_;
	D3D12_RESOURCE_STATES drawArgsResourceState_ = D3D12_RESOURCE_STATE_COMMON;
	uint32_t uavIndexAliveIndices_;
	uint32_t uavIndexDrawArgs_;
	Microsoft::WRL::ComPtr<ID3D12Resource> aliveIndicesResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> resetResource_;

	uint32_t freeIndex_ = 0;
	uint32_t maxInstance_ = kMaxInstance;
	
};
