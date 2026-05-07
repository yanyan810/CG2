#include "PropManager.h"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

PropManager::PropManager() {}

void PropManager::Initialize(Object3dCommon* objCom, DirectXCommon* dx, Camera* cam) {
	objCom_ = objCom;
	dx_ = dx;
	cam_ = cam;

	ScanModelFiles();
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
	for (auto& prop : placedProps_) {
		if (prop.object) {
			prop.object->SetTranslate(prop.pos);
			prop.object->SetRotate(prop.rot);
			prop.object->SetScale(prop.scale);
			prop.object->SetUVTransform(prop.uvScale, { 0.0f, 0.0f });

			prop.object->SetEnableLighting(prop.enableLighting ? 1 : 0);
			prop.object->SetDirection(prop.lightDir);
			prop.object->SetLightColor(prop.lightColor);
			prop.object->SetIntensity(prop.lightIntensity);

			prop.object->Update(dt);
		}
	}
}

void PropManager::Draw3D() {
	for (auto& prop : placedProps_) {
		if (prop.object) {
			prop.object->Draw();
		}
	}
}

void PropManager::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Scene Editor");

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

		ImGui::Checkbox("Enable Lighting", &prop.enableLighting);
		if (prop.enableLighting) {
			ImGui::DragFloat3("Light Dir", &prop.lightDir.x, 0.01f);
			ImGui::ColorEdit4("Light Color", &prop.lightColor.x);
			ImGui::DragFloat("Light Intensity", &prop.lightIntensity, 0.1f);
		}

		if (ImGui::Button("Delete Prop")) {
			placedProps_.erase(placedProps_.begin() + selectedPropIndex_);
			selectedPropIndex_ = -1;
		}
	}

	ImGui::Separator();
	if (ImGui::Button("Save Scene to JSON")) {
		SaveToJson("resources/configs/sceneProps.json");
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Scene from JSON")) {
		LoadFromJson("resources/configs/sceneProps.json");
	}

	ImGui::End();
#endif
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
		j["enableLighting"] = prop.enableLighting;
		j["lightDir"] = { prop.lightDir.x, prop.lightDir.y, prop.lightDir.z };
		j["lightColor"] = { prop.lightColor.x, prop.lightColor.y, prop.lightColor.z, prop.lightColor.w };
		j["lightIntensity"] = prop.lightIntensity;
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
		if (j.contains("enableLighting")) prop.enableLighting = j["enableLighting"];
		if (j.contains("lightDir")) {
			prop.lightDir.x = j["lightDir"][0]; prop.lightDir.y = j["lightDir"][1]; prop.lightDir.z = j["lightDir"][2];
		}
		if (j.contains("lightColor")) {
			prop.lightColor.x = j["lightColor"][0]; prop.lightColor.y = j["lightColor"][1]; prop.lightColor.z = j["lightColor"][2]; prop.lightColor.w = j["lightColor"][3];
		}
		if (j.contains("lightIntensity")) prop.lightIntensity = j["lightIntensity"];

		if (!prop.modelPath.empty()) {
			prop.object = std::make_unique<Object3d>();
			prop.object->Initialize(objCom_, dx_);
			prop.object->SetModel(prop.modelPath);
			prop.object->SetCamera(cam_);
		}

		placedProps_.push_back(std::move(prop));
	}
}
