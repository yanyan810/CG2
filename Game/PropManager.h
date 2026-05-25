#pragma once

#include "MathStruct.h"
#include "Object3d.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "BloomConstantBuffer.h"
#include <string>
#include <vector>
#include <memory>

class GameApp;


struct PlacedProp {
	std::string name;
	std::string modelPath;

	Vector3 pos = { 0.0f, 0.0f, 0.0f };
	Vector3 rot = { 0.0f, 0.0f, 0.0f };
	Vector3 scale = { 1.0f, 1.0f, 1.0f };
	Vector2 uvScale = { 1.0f, 1.0f };
	bool useBillboard = false;

	bool enableLighting = true;
	Vector3 lightDir = { 0.0f, -1.0f, 0.0f };
	Vector4 lightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	float lightIntensity = 1.0f;
	Vector3 pointLightPos = { 0.0f, 3.0f, 0.0f };
	Vector4 pointLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	float pointLightIntensity = 0.0f;
	float pointLightRadius = 10.0f;
	float pointLightDecay = 1.0f;
	Vector3 spotLightPos = { 0.0f, 3.0f, 0.0f };
	Vector3 spotLightDir = { 0.0f, -1.0f, 0.0f };
	Vector4 spotLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	float spotLightIntensity = 0.0f;
	float spotLightDistance = 10.0f;
	float spotLightDecay = 1.0f;
	float spotLightAngleDeg = 30.0f;
	float spotLightFalloffStartDeg = 20.0f;

	// ポストエフェクト設定
	bool usePostEffect = false;  // true の場合 ObjectPostEffect で描画
	BloomParam postEffect = {};  // ブルーム・ブラー等のパラメーター

	std::unique_ptr<Object3d> object;
};

class PropManager {
public:
	PropManager();
	~PropManager() = default;

	void Initialize(Object3dCommon* objCom, DirectXCommon* dx, Camera* cam);
	void SetCamera(Camera* cam);
	void Update(float dt);
	void Draw3D();
	void DrawPostEffect3D(GameApp& app); // エフェクト付きオブジェクトの描画
	void DrawImGui(
		const char* windowName = "Scene Editor",
		const std::string& jsonPath = "resources/configs/sceneProps.json");

	void SaveToJson(const std::string& filepath);
	void LoadFromJson(const std::string& filepath);

	void ScanModelFiles();

	const std::vector<PlacedProp>& GetProps() const { return placedProps_; }
	std::vector<PlacedProp>& GetPropsMutable() { return placedProps_; }

private:
	Object3dCommon* objCom_ = nullptr;
	DirectXCommon* dx_ = nullptr;
	Camera* cam_ = nullptr;

	std::vector<PlacedProp> placedProps_;
	int selectedPropIndex_ = -1;
	int pendingDeletePropIndex_ = -1;
	bool pendingLoadFromJson_ = false;
	std::string pendingLoadFromJsonPath_;

	std::vector<std::string> availableModelPaths_;
	int selectedModelIndex_ = 0;

	void ApplyPendingChanges();
};
