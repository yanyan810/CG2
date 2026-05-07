#pragma once

#include "MathStruct.h"
#include "Object3d.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include <string>
#include <vector>
#include <memory>

struct PlacedProp {
	std::string name;
	std::string modelPath;

	Vector3 pos = { 0.0f, 0.0f, 0.0f };
	Vector3 rot = { 0.0f, 0.0f, 0.0f };
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Vector2 uvScale = { 1.0f, 1.0f };

	bool enableLighting = true;
	Vector3 lightDir = { 0.0f, -1.0f, 0.0f };
	Vector4 lightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	float lightIntensity = 1.0f;

	std::unique_ptr<Object3d> object;
};

class PropManager {
public:
	PropManager();
	~PropManager() = default;

	void Initialize(Object3dCommon* objCom, DirectXCommon* dx, Camera* cam);
	void Update(float dt);
	void Draw3D();
	void DrawImGui();

	void SaveToJson(const std::string& filepath);
	void LoadFromJson(const std::string& filepath);

	void ScanModelFiles();

	const std::vector<PlacedProp>& GetProps() const { return placedProps_; }

private:
	Object3dCommon* objCom_ = nullptr;
	DirectXCommon* dx_ = nullptr;
	Camera* cam_ = nullptr;

	std::vector<PlacedProp> placedProps_;
	int selectedPropIndex_ = -1;

	std::vector<std::string> availableModelPaths_;
	int selectedModelIndex_ = 0;
};
