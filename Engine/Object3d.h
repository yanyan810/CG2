#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"
#include <string>
#include <vector>
#include <format>
#include <filesystem>
#include <fstream>
#include "DirectXCommon.h"
#include "TextureManager.h"
#include "Model.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "Camera.h"

//class Object3dCommon;

class Object3d
{

public:

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	struct DirectionalLight {
		Vector4 color;
		Vector3 direction;
		float intensity;
	};

	struct CameraGPU {
		Vector3 worldPosition;
		float pad; // ★16byte揃え
	};

public:

	void Initialize(Object3dCommon* object3dCommon, DirectXCommon* dx);

	void Update();

	void Draw();

	void SetModel(Model* model) { this->model_ = model; }

	void SetModel(const std::string& filePath);

	// ===== Transform 用 setter =====
public:
	void SetScale(const Vector3& s) { transform.scale = s; }
	void SetRotate(const Vector3& r) { transform.rotate = r; }
	void SetTranslate(const Vector3& t) { transform.translate = t; }

	// ===== Transform 用 getter =====
	const Vector3& GetScale()     const { return transform.scale; }
	const Vector3& GetRotate()    const { return transform.rotate; }
	const Vector3& GetTranslate() const { return transform.translate; }

	//光源用
	void SetLightColor(const Vector4& color) { directionalLightData->color = color; }
	void SetDirection(const Vector3& direction) { directionalLightData->direction = direction; }
	void SetIntensity(const float& intensity) { directionalLightData->intensity = intensity; }

	// 光源 getter（正しく返すように修正）
	const Vector4& GetLightColor()     const { return directionalLightData->color; }
	const Vector3& GetDirection() const { return directionalLightData->direction; }
	float          GetIntensity() const { return directionalLightData->intensity; }

	void SetEnableLighting(int enable) {
		if (model_ && model_->GetMaterial()) {
			model_->GetMaterial()->enableLighting = enable;
		}
	}
	void SetShininess(float s) {
		if (model_ && model_->GetMaterial()) {
			model_->GetMaterial()->shininess = s;
		}
	}
	int GetEnableLighting() const {
		return (model_ && model_->GetMaterial()) ? model_->GetMaterial()->enableLighting : 0;
	}
	float GetShininess() const {
		return (model_ && model_->GetMaterial()) ? model_->GetMaterial()->shininess : 0.0f;
	}

	//ブレンド設定
	void SetBlendMode(Object3dCommon::BlendMode m) { object3dCommon->SetBlendMode(m); }

	//色関係
	void SetMaterialColor(const Vector4& c) {
		if (model_) {
			model_->SetMaterialColor(c);
		}
	}
	Vector4 GetMaterialColor() const {
		return model_ ? model_->GetMaterialColor() : Vector4{ 1,1,1,1 };
	}

	//カメラセッター
	void SetCamera(Camera* camera) { camera_ = camera; }

private:

	DirectXCommon* dx_ = nullptr;

	Object3dCommon* object3dCommon = nullptr;

	Model* model_ = nullptr;

	//モデル用のTransformationMatrix用のリソースを作る。Matrix4x4 一つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceModel;/* = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));*/
	//データを書き込む
	TransformationMatrix* transformationMatrixDataModel = nullptr;

	//ライトのリソース作成
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;
	DirectionalLight* directionalLightData = nullptr;

	Transform transform;
	Transform cameraTransform;
	//カメラ
	Camera* camera_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
	CameraGPU* cameraData_ = nullptr;

};

