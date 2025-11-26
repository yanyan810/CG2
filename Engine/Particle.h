#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"
#include "AABB.h"
#include <string>
#include <vector>
#include <format>
#include <filesystem>
#include <fstream>
#include "DirectXCommon.h"
#include "TextureManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "ParticleCommon.h"
#include <numbers>

//class Object3dCommon;

class Particle
{

public:

	struct ParticleForGPU {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Vector4 color;
	};

	struct DirectionalLight {
		Vector4 color;
		Vector3 direction;
		float intensity;
	};

	struct ParticleData {
		Transform transform;
		Vector3 velocity;
		Vector4 color;
		float lifeTime;
		float currentTime;
	};

	struct Emitter {
		Transform transform;//エミッタのトランスフォーム
		uint32_t count;//発生数
		float frequency;//発生頻度
		float frequencyTime;//頻度用時刻
	};

	struct AccelerationField {
		Vector3 acceleration;//加速度
		AABB area; //範囲

	};

public:

	void Initialize(ParticleCommon* particleCommon, DirectXCommon* dx);

	void Update();

	void Draw();

	void SetModel(Model* model) { this->model_ = model; }

	void SetModel(const std::string& filePath);

	// ===== Transform 用 setter =====
	void SetScale(const Vector3& scale) {
		if (!particles.empty()) particles.front().transform.scale = scale;
	}
	void SetRotate(const Vector3& rotate) {
		if (!particles.empty()) particles.front().transform.rotate = rotate;
	}
	void SetTranslate(const Vector3& translate) {
		if (!particles.empty()) particles.front().transform.translate = translate;
	}

	// ===== Transform 用 getter =====
	const Vector3& GetScale() const {
		return particles.front().transform.scale;
	}
	const Vector3& GetRotate() const {
		return particles.front().transform.rotate;
	}
	const Vector3& GetTranslate() const {
		return particles.front().transform.translate;
	}

	//光源用
	void SetLightColor(const Vector4& color) { directionalLightData->color = color; }
	void SetDirection(const Vector3& direction) { directionalLightData->direction = direction; }
	void SetIntensity(const float& intensity) { directionalLightData->intensity = intensity; }

	// 光源 getter（正しく返すように修正）
	const Vector4& GetLightColor()     const { return directionalLightData->color; }
	const Vector3& GetDirection() const { return directionalLightData->direction; }
	float          GetIntensity() const { return directionalLightData->intensity; }

	//ブレンド設定
	void SetBlendMode(ParticleCommon::BlendMode m) { particleCommon->SetBlendMode(m); }

	//色関係
	void SetMaterialColor(const Vector4& c) {
		if (model_) {
			model_->SetMaterialColor(c);
		}
	}
	Vector4 GetMaterialColor() const {
		return model_ ? model_->GetMaterialColor() : Vector4{ 1,1,1,1 };
	}

	ParticleData MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate);

	void DebugImGui();

	void SpawnParticle();

	std::list<ParticleData> Emit(const Emitter& emitter, std::mt19937& randomEngine);

private:

	DirectXCommon* dx_;

	ParticleCommon* particleCommon = nullptr;

	Model* model_ = nullptr;

	//モデル用のTransformationMatrix用のリソースを作る。Matrix4x4 一つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceModel;/* = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));*/
	//データを書き込む
	ParticleForGPU* transformationMatrixDataModel = nullptr;

	//ライトのリソース作成
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;
	DirectionalLight* directionalLightData = nullptr;

	Transform cameraTransform;

	// instancing 用 StructuredBuffer の SRV（GPU 側ハンドル）
	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU_{};

	static const uint32_t kMaxInstance = 100; // 好きな数

	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
	ParticleForGPU* instancingData_ = nullptr;
	uint32_t instanceCount_ = 0; // 今フレーム描く数

	std::list<ParticleData> particles;

	//Δtを設定
	const float deltaTime = 1 / 60.0f;
	std::random_device seedGenerator;
	std::mt19937 randomEngine{ seedGenerator() };

	Emitter emitter{};

	AccelerationField accelerationField;

};

