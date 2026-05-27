#include "PropManager.h"
#include "GameApp.h"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;

float DegToCos(float degrees) {
	return std::cosf(degrees * (kPi / 180.0f));
}

Vector3 NormalizeOrDown(const Vector3& v) {
	const float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (length <= 0.0001f) {
		return { 0.0f, -1.0f, 0.0f };
	}
	return { v.x / length, v.y / length, v.z / length };
}
}

PropManager::PropManager() {}

void PropManager::Initialize(Object3dCommon* objCom, DirectXCommon* dx, Camera* cam) {
	objCom_ = objCom;
	dx_ = dx;
	cam_ = cam;

	ScanModelFiles();
}

void PropManager::SetCamera(Camera* cam) {
	cam_ = cam;
	for (auto& prop : placedProps_) {
		if (prop.object) {
			prop.object->SetCamera(cam_);
		}
	}
}

void PropManager::ScanModelFiles() {
	availableModelPaths_.clear();
	std::string basePath = "resources";
	if (!std::filesystem::exists(basePath)) return;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath)) {
		if (entry.is_regular_file()) {
			std::string ext = entry.path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
			if (ext == ".obj" || ext == ".gltf") {
				// Get relative path from "resources/"
				std::string pathStr = entry.path().string();
				// Replace backward slashes with forward slashes
				std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
				
				size_t pos = pathStr.find("resources/");
				if (pos != std::string::npos) {
					std::string relativePath = pathStr.substr(pos + 10);
					availableModelPaths_.push_back(relativePath);
				}
			}
		}
	}
}

void PropManager::Update(float dt) {
	ApplyPendingChanges();

	for (auto& prop : placedProps_) {
		if (prop.object) {
			if (prop.useBillboard && cam_) {
				const Vector3 camPos = cam_->GetTranslate();
				const float dx = camPos.x - prop.pos.x;
				const float dz = camPos.z - prop.pos.z;
				prop.rot.y = std::atan2(dx, dz);
			}
			prop.object->SetTranslate(prop.pos);
			prop.object->SetRotate(prop.rot);
			prop.object->SetScale(prop.scale);
			prop.object->SetUVTransform(prop.uvScale, { 0.0f, 0.0f });

			prop.object->SetEnableLighting(prop.enableLighting ? 1 : 0);
			prop.object->SetDirection(prop.lightDir);
			prop.object->SetLightColor(prop.lightColor);
			prop.object->SetIntensity(prop.lightIntensity);
			prop.object->SetPointLightPos(prop.pointLightPos);
			prop.object->SetPointLightColor(prop.pointLightColor);
			prop.object->SetPointLightIntensity(prop.pointLightIntensity);
			prop.object->SetPointLightRadius(prop.pointLightRadius);
			prop.object->SetPointLightDecay(prop.pointLightDecay);
			prop.spotLightDir = NormalizeOrDown(prop.spotLightDir);
			if (prop.spotLightFalloffStartDeg > prop.spotLightAngleDeg - 0.1f) {
				prop.spotLightFalloffStartDeg = prop.spotLightAngleDeg - 0.1f;
			}
			prop.object->SetSpotLightPos(prop.spotLightPos);
			prop.object->SetSpotLightDirection(prop.spotLightDir);
			prop.object->SetSpotLightColor(prop.spotLightColor);
			prop.object->SetSpotLightIntensity(prop.spotLightIntensity);
			prop.object->SetSpotLightDistance(prop.spotLightDistance);
			prop.object->SetSpotLightDecay(prop.spotLightDecay);
			prop.object->SetSpotLightCosAngle(DegToCos(prop.spotLightAngleDeg));
			prop.object->SetSpotLightCosFalloffStart(DegToCos(prop.spotLightFalloffStartDeg));

			prop.object->Update(dt);
		}
	}
}

void PropManager::WarmupDrawResources()
{
	for (auto& prop : placedProps_) {
		if (prop.object) {
			prop.object->WarmupDrawResources();
		}
	}
}

void PropManager::Draw3D() {
	for (auto& prop : placedProps_) {
		if (prop.object && !prop.usePostEffect) {
			prop.object->Draw();
		}
	}
}

void PropManager::DrawPostEffect3D(GameApp& app) {
	for (auto& prop : placedProps_) {
		if (!prop.object || !prop.usePostEffect) {
			continue;
		}
		app.ObjectPost()->SetParam(prop.postEffect);
		app.BeginObjectPostEffect();
		prop.object->Draw();
		app.EndObjectPostEffect();
		app.ObjCom()->SetGraphicsPipelineState();
	}
}

void PropManager::DrawImGui(const char* windowName, const std::string& jsonPath) {
#ifdef USE_IMGUI
	ImGui::Begin(windowName);

	if (ImGui::Button("Refresh Model List")) {
		ScanModelFiles();
	}

	if (!availableModelPaths_.empty()) {
		std::string comboPreview = availableModelPaths_[selectedModelIndex_];
		if (ImGui::BeginCombo("Models", comboPreview.c_str())) {
			for (int i = 0; i < availableModelPaths_.size(); ++i) {
				const bool isSelected = (selectedModelIndex_ == i);
				if (ImGui::Selectable(availableModelPaths_[i].c_str(), isSelected)) {
					selectedModelIndex_ = i;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("Spawn Selected Model")) {
			PlacedProp newProp;
			newProp.name = "Prop " + std::to_string(placedProps_.size() + 1);
			newProp.modelPath = availableModelPaths_[selectedModelIndex_];
			newProp.object = std::make_unique<Object3d>();
			newProp.object->Initialize(objCom_, dx_);
			newProp.object->SetModel(newProp.modelPath);
			newProp.object->SetCamera(cam_);
			placedProps_.push_back(std::move(newProp));
			selectedPropIndex_ = (int)placedProps_.size() - 1;
		}
	}

	ImGui::Separator();
	ImGui::Text("Placed Props:");

	if (ImGui::BeginListBox("##PlacedProps")) {
		for (int i = 0; i < placedProps_.size(); ++i) {
			const bool isSelected = (selectedPropIndex_ == i);
			std::string displayName = placedProps_[i].name + " (" + placedProps_[i].modelPath + ")";
			if (ImGui::Selectable(displayName.c_str(), isSelected)) {
				selectedPropIndex_ = i;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndListBox();
	}

	if (selectedPropIndex_ >= 0 && selectedPropIndex_ < placedProps_.size()) {
		auto& prop = placedProps_[selectedPropIndex_];

		ImGui::Separator();
		ImGui::Text("Edit Prop: %s", prop.name.c_str());

		char nameBuffer[256];
		strcpy_s(nameBuffer, prop.name.c_str());
		if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
			prop.name = nameBuffer;
		}

		ImGui::DragFloat3("Position", &prop.pos.x, 0.1f);
		ImGui::DragFloat3("Rotation", &prop.rot.x, 0.01f);
		ImGui::DragFloat3("Scale", &prop.scale.x, 0.1f);
		ImGui::DragFloat2("UV Scale", &prop.uvScale.x, 0.1f);
		ImGui::Checkbox("Use Billboard", &prop.useBillboard);

		ImGui::Checkbox("Enable Lighting", &prop.enableLighting);
		if (prop.enableLighting) {
			ImGui::DragFloat3("Light Dir", &prop.lightDir.x, 0.01f);
			ImGui::ColorEdit4("Light Color", &prop.lightColor.x);
			ImGui::DragFloat("Light Intensity", &prop.lightIntensity, 0.1f);

			if (ImGui::TreeNode("Point Light")) {
				ImGui::DragFloat3("Point Pos", &prop.pointLightPos.x, 0.1f);
				ImGui::ColorEdit4("Point Color", &prop.pointLightColor.x);
				ImGui::DragFloat("Point Intensity", &prop.pointLightIntensity, 0.05f, 0.0f, 50.0f);
				ImGui::DragFloat("Point Radius", &prop.pointLightRadius, 0.1f, 0.1f, 200.0f);
				ImGui::DragFloat("Point Decay", &prop.pointLightDecay, 0.01f, 0.0f, 10.0f);
				if (ImGui::Button("Disable Point Light")) {
					prop.pointLightIntensity = 0.0f;
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Spot Light")) {
				ImGui::DragFloat3("Spot Pos", &prop.spotLightPos.x, 0.1f);
				ImGui::DragFloat3("Spot Dir", &prop.spotLightDir.x, 0.01f, -1.0f, 1.0f);
				ImGui::ColorEdit4("Spot Color", &prop.spotLightColor.x);
				ImGui::DragFloat("Spot Intensity", &prop.spotLightIntensity, 0.05f, 0.0f, 200.0f);
				ImGui::DragFloat("Spot Distance", &prop.spotLightDistance, 0.1f, 0.1f, 500.0f);
				ImGui::DragFloat("Spot Decay", &prop.spotLightDecay, 0.01f, 0.0f, 10.0f);
				ImGui::DragFloat("Spot Angle", &prop.spotLightAngleDeg, 0.1f, 1.0f, 89.0f);
				ImGui::DragFloat("Spot Falloff Start", &prop.spotLightFalloffStartDeg, 0.1f, 0.0f, 88.0f);
				if (ImGui::Button("Disable Spot Light")) {
					prop.spotLightIntensity = 0.0f;
				}
				ImGui::TreePop();
			}
		}

		ImGui::Separator();
		ImGui::Checkbox("Use Post Effect", &prop.usePostEffect);
		if (prop.usePostEffect) {
			ImGui::DragFloat("Bloom Threshold",   &prop.postEffect.threshold,  0.01f, 0.0f, 2.0f);
			ImGui::DragFloat("Bloom Intensity",   &prop.postEffect.intensity,   0.01f, 0.0f, 5.0f);
			ImGui::DragFloat("Blur (Chrom Ab)",   &prop.postEffect.chromAbAmount, 0.001f, 0.0f, 0.05f);
			ImGui::DragFloat("Distortion",        &prop.postEffect.distortionAmount, 0.001f, 0.0f, 0.1f);
			ImGui::DragFloat("Noise",             &prop.postEffect.noiseIntensity, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Scanline",          &prop.postEffect.scanlineIntensity, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Glitch",            &prop.postEffect.glitchAmount, 0.001f, 0.0f, 0.1f);
			ImGui::DragFloat("Vignette",          &prop.postEffect.vignetteIntensity, 0.01f, 0.0f, 2.0f);
			ImGui::DragFloat("Dissolve",          &prop.postEffect.dissolveAmount, 0.01f, -1.0f, 1.0f);
		}

		if (ImGui::Button("Delete Prop")) {
			pendingDeletePropIndex_ = selectedPropIndex_;
			selectedPropIndex_ = -1;
		}
	}

	ImGui::Separator();
	if (ImGui::Button("Save Scene to JSON")) {
		SaveToJson(jsonPath);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Scene from JSON")) {
		pendingLoadFromJson_ = true;
		pendingLoadFromJsonPath_ = jsonPath;
		pendingDeletePropIndex_ = -1;
		selectedPropIndex_ = -1;
	}

	ImGui::End();
#endif
}

void PropManager::ApplyPendingChanges() {
	if (pendingLoadFromJson_) {
		pendingLoadFromJson_ = false;
		LoadFromJson(pendingLoadFromJsonPath_);
		pendingLoadFromJsonPath_.clear();
		return;
	}

	if (pendingDeletePropIndex_ >= 0) {
		if (pendingDeletePropIndex_ < static_cast<int>(placedProps_.size())) {
			placedProps_.erase(placedProps_.begin() + pendingDeletePropIndex_);
		}
		pendingDeletePropIndex_ = -1;
		selectedPropIndex_ = -1;
	}
}

void PropManager::SaveToJson(const std::string& filepath) {
	nlohmann::json jArray = nlohmann::json::array();

	for (const auto& prop : placedProps_) {
		nlohmann::json j;
		j["name"] = prop.name;
		j["modelPath"] = prop.modelPath;
		j["pos"] = { prop.pos.x, prop.pos.y, prop.pos.z };
		j["rot"] = { prop.rot.x, prop.rot.y, prop.rot.z };
		j["scale"] = { prop.scale.x, prop.scale.y, prop.scale.z };
		j["uvScale"] = { prop.uvScale.x, prop.uvScale.y };
		j["useBillboard"] = prop.useBillboard;
		j["enableLighting"] = prop.enableLighting;
		j["lightDir"] = { prop.lightDir.x, prop.lightDir.y, prop.lightDir.z };
		j["lightColor"] = { prop.lightColor.x, prop.lightColor.y, prop.lightColor.z, prop.lightColor.w };
		j["lightIntensity"] = prop.lightIntensity;
		j["pointLightPos"] = { prop.pointLightPos.x, prop.pointLightPos.y, prop.pointLightPos.z };
		j["pointLightColor"] = { prop.pointLightColor.x, prop.pointLightColor.y, prop.pointLightColor.z, prop.pointLightColor.w };
		j["pointLightIntensity"] = prop.pointLightIntensity;
		j["pointLightRadius"] = prop.pointLightRadius;
		j["pointLightDecay"] = prop.pointLightDecay;
		j["spotLightPos"] = { prop.spotLightPos.x, prop.spotLightPos.y, prop.spotLightPos.z };
		j["spotLightDir"] = { prop.spotLightDir.x, prop.spotLightDir.y, prop.spotLightDir.z };
		j["spotLightColor"] = { prop.spotLightColor.x, prop.spotLightColor.y, prop.spotLightColor.z, prop.spotLightColor.w };
		j["spotLightIntensity"] = prop.spotLightIntensity;
		j["spotLightDistance"] = prop.spotLightDistance;
		j["spotLightDecay"] = prop.spotLightDecay;
		j["spotLightAngleDeg"] = prop.spotLightAngleDeg;
		j["spotLightFalloffStartDeg"] = prop.spotLightFalloffStartDeg;
		j["usePostEffect"] = prop.usePostEffect;
		j["postEffect"] = {
			{"threshold",         prop.postEffect.threshold},
			{"intensity",         prop.postEffect.intensity},
			{"vignetteIntensity", prop.postEffect.vignetteIntensity},
			{"vignetteScale",     prop.postEffect.vignetteScale},
			{"chromAbAmount",     prop.postEffect.chromAbAmount},
			{"distortionAmount",  prop.postEffect.distortionAmount},
			{"noiseIntensity",    prop.postEffect.noiseIntensity},
			{"scanlineIntensity", prop.postEffect.scanlineIntensity},
			{"scanlineFrequency", prop.postEffect.scanlineFrequency},
			{"glitchAmount",      prop.postEffect.glitchAmount},
			{"dissolveAmount",    prop.postEffect.dissolveAmount},
			{"curvature",         prop.postEffect.curvature},
			{"borderSharp",       prop.postEffect.borderSharp}
		};
		jArray.push_back(j);
	}

	std::filesystem::path dir = std::filesystem::path(filepath).parent_path();
	if (!std::filesystem::exists(dir)) {
		std::filesystem::create_directories(dir);
	}

	std::ofstream file(filepath);
	if (file.is_open()) {
		file << jArray.dump(4);
	}
}

void PropManager::LoadFromJson(const std::string& filepath) {
	std::ifstream file(filepath);
	if (!file.is_open()) return;

	nlohmann::json jArray;
	file >> jArray;

	placedProps_.clear();
	selectedPropIndex_ = -1;

	for (const auto& j : jArray) {
		PlacedProp prop;
		if (j.contains("name")) prop.name = j["name"];
		if (j.contains("modelPath")) prop.modelPath = j["modelPath"];
		
		if (j.contains("pos")) {
			prop.pos.x = j["pos"][0]; prop.pos.y = j["pos"][1]; prop.pos.z = j["pos"][2];
		}
		if (j.contains("rot")) {
			prop.rot.x = j["rot"][0]; prop.rot.y = j["rot"][1]; prop.rot.z = j["rot"][2];
		}
		if (j.contains("scale")) {
			prop.scale.x = j["scale"][0]; prop.scale.y = j["scale"][1]; prop.scale.z = j["scale"][2];
		}
		if (j.contains("uvScale")) {
			prop.uvScale.x = j["uvScale"][0]; prop.uvScale.y = j["uvScale"][1];
		}
		if (j.contains("useBillboard")) prop.useBillboard = j["useBillboard"];
		if (j.contains("enableLighting")) prop.enableLighting = j["enableLighting"];
		if (j.contains("lightDir")) {
			prop.lightDir.x = j["lightDir"][0]; prop.lightDir.y = j["lightDir"][1]; prop.lightDir.z = j["lightDir"][2];
		}
		if (j.contains("lightColor")) {
			prop.lightColor.x = j["lightColor"][0]; prop.lightColor.y = j["lightColor"][1]; prop.lightColor.z = j["lightColor"][2]; prop.lightColor.w = j["lightColor"][3];
		}
		if (j.contains("lightIntensity")) prop.lightIntensity = j["lightIntensity"];
		if (j.contains("pointLightPos")) {
			prop.pointLightPos.x = j["pointLightPos"][0]; prop.pointLightPos.y = j["pointLightPos"][1]; prop.pointLightPos.z = j["pointLightPos"][2];
		}
		if (j.contains("pointLightColor")) {
			prop.pointLightColor.x = j["pointLightColor"][0]; prop.pointLightColor.y = j["pointLightColor"][1]; prop.pointLightColor.z = j["pointLightColor"][2]; prop.pointLightColor.w = j["pointLightColor"][3];
		}
		if (j.contains("pointLightIntensity")) prop.pointLightIntensity = j["pointLightIntensity"];
		if (j.contains("pointLightRadius")) prop.pointLightRadius = j["pointLightRadius"];
		if (j.contains("pointLightDecay")) prop.pointLightDecay = j["pointLightDecay"];
		if (j.contains("spotLightPos")) {
			prop.spotLightPos.x = j["spotLightPos"][0]; prop.spotLightPos.y = j["spotLightPos"][1]; prop.spotLightPos.z = j["spotLightPos"][2];
		}
		if (j.contains("spotLightDir")) {
			prop.spotLightDir.x = j["spotLightDir"][0]; prop.spotLightDir.y = j["spotLightDir"][1]; prop.spotLightDir.z = j["spotLightDir"][2];
		}
		if (j.contains("spotLightColor")) {
			prop.spotLightColor.x = j["spotLightColor"][0]; prop.spotLightColor.y = j["spotLightColor"][1]; prop.spotLightColor.z = j["spotLightColor"][2]; prop.spotLightColor.w = j["spotLightColor"][3];
		}
		if (j.contains("spotLightIntensity")) prop.spotLightIntensity = j["spotLightIntensity"];
		if (j.contains("spotLightDistance")) prop.spotLightDistance = j["spotLightDistance"];
		if (j.contains("spotLightDecay")) prop.spotLightDecay = j["spotLightDecay"];
		if (j.contains("spotLightAngleDeg")) prop.spotLightAngleDeg = j["spotLightAngleDeg"];
		if (j.contains("spotLightFalloffStartDeg")) prop.spotLightFalloffStartDeg = j["spotLightFalloffStartDeg"];
		if (j.contains("usePostEffect")) prop.usePostEffect = j["usePostEffect"];
		if (j.contains("postEffect")) {
			const auto& pe = j["postEffect"];
			if (pe.contains("threshold"))         prop.postEffect.threshold         = pe["threshold"];
			if (pe.contains("intensity"))         prop.postEffect.intensity         = pe["intensity"];
			if (pe.contains("vignetteIntensity")) prop.postEffect.vignetteIntensity = pe["vignetteIntensity"];
			if (pe.contains("vignetteScale"))     prop.postEffect.vignetteScale     = pe["vignetteScale"];
			if (pe.contains("chromAbAmount"))     prop.postEffect.chromAbAmount     = pe["chromAbAmount"];
			if (pe.contains("distortionAmount"))  prop.postEffect.distortionAmount  = pe["distortionAmount"];
			if (pe.contains("noiseIntensity"))    prop.postEffect.noiseIntensity    = pe["noiseIntensity"];
			if (pe.contains("scanlineIntensity")) prop.postEffect.scanlineIntensity = pe["scanlineIntensity"];
			if (pe.contains("scanlineFrequency")) prop.postEffect.scanlineFrequency = pe["scanlineFrequency"];
			if (pe.contains("glitchAmount"))      prop.postEffect.glitchAmount      = pe["glitchAmount"];
			if (pe.contains("dissolveAmount"))    prop.postEffect.dissolveAmount    = pe["dissolveAmount"];
			if (pe.contains("curvature"))         prop.postEffect.curvature         = pe["curvature"];
			if (pe.contains("borderSharp"))       prop.postEffect.borderSharp       = pe["borderSharp"];
		}

		if (!prop.modelPath.empty()) {
			prop.object = std::make_unique<Object3d>();
			prop.object->Initialize(objCom_, dx_);
			prop.object->SetModel(prop.modelPath);
			prop.object->SetCamera(cam_);
		}

		placedProps_.push_back(std::move(prop));
	}
}
