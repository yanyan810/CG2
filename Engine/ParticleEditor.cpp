#include "ParticleEditor.h"
#include "ParticleManager.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ParticleEditor* ParticleEditor::GetInstance() {
    static ParticleEditor instance;
    return &instance;
}

void ParticleEditor::Initialize() {
    isResourcesScanned_ = false;
    isLoadRequested_ = false;
}

void ParticleEditor::Update() {
    if (isLoadRequested_) {
        Load(loadFileName_);
        isLoadRequested_ = false;
    }
}

void ParticleEditor::ScanResources() {
    modelFiles_.clear();
    textureFiles_.clear();
    jsonFiles_.clear();
    
    std::string particleDir = "Resources/Particles";
    if (std::filesystem::exists(particleDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(particleDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                jsonFiles_.push_back(entry.path().filename().string());
            }
        }
    }

    std::string targetDir = "Resources";
    if (!std::filesystem::exists(targetDir)) return;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(targetDir)) {
        if (!entry.is_regular_file()) continue;
        
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        std::string path = entry.path().string();
        std::replace(path.begin(), path.end(), '\\', '/'); 
        
        if (ext == ".obj" || ext == ".gltf" || ext == ".glb") {
            std::string lowerPath = path;
            std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
            if (lowerPath.starts_with("resources/")) {
                path = path.substr(10);
            }
            modelFiles_.push_back(path);
        } else if (ext == ".png" || ext == ".jpg" || ext == ".dds") {
            textureFiles_.push_back(path);
        }
    }
    isResourcesScanned_ = true;
}

void ParticleEditor::DrawImGui() {
    ImGui::Begin("Particle Editor");

    if (ImGui::Button("Scan Resources")) {
        ScanResources();
    }
    if (!isResourcesScanned_) {
        ScanResources();
    }

    if (!jsonFiles_.empty()) {
        std::string currentJson = selectedJsonIndex_ >= 0 && selectedJsonIndex_ < jsonFiles_.size() ? jsonFiles_[selectedJsonIndex_] : "Select JSON...";
        if (ImGui::BeginCombo("Saved JSONs", currentJson.c_str())) {
            for (int i = 0; i < jsonFiles_.size(); ++i) {
                bool isSelected = (selectedJsonIndex_ == i);
                if (ImGui::Selectable(jsonFiles_[i].c_str(), isSelected)) {
                    selectedJsonIndex_ = i;
                    strcpy_s(saveFileName_, sizeof(saveFileName_), jsonFiles_[i].c_str());
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::InputText("File Name", saveFileName_, sizeof(saveFileName_));
    if (ImGui::Button("Save Particles")) {
        Save(saveFileName_);
        ScanResources(); // 保存後にリストを更新
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Particles")) {
        isLoadRequested_ = true;
        loadFileName_ = saveFileName_;
    }

    ImGui::Separator();
    
    ImGui::Text("Batch Controls");
    if (ImGui::Button("All Auto Emit ON")) {
        for (auto& [name, group] : ParticleManager::GetInstance()->GetParticleGroups()) {
            group.isAutoEmit = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("All Auto Emit OFF")) {
        for (auto& [name, group] : ParticleManager::GetInstance()->GetParticleGroups()) {
            group.isAutoEmit = false;
        }
    }

    ImGui::Separator();
    
    ImGui::Text("Create New Particle");
    ImGui::InputText("Group Name", newGroupName_, sizeof(newGroupName_));
    if (ImGui::Button("Create")) {
        auto& particleGroups = ParticleManager::GetInstance()->GetParticleGroups();
        if (!particleGroups.contains(newGroupName_) && strlen(newGroupName_) > 0) {
            ParticleManager::GetInstance()->CreateParticleGroup(newGroupName_, "");
        }
    }

    ImGui::Separator();

    auto& particleGroups_ = ParticleManager::GetInstance()->GetParticleGroups();

    for (auto& [name, group] : particleGroups_) {
        if (ImGui::TreeNode(name.c_str())) {
            
            // ブレンドモード
            const char* blendModes[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
            int currentBlend = static_cast<int>(group.blendMode);
            if (ImGui::Combo("Blend Mode", &currentBlend, blendModes, IM_ARRAYSIZE(blendModes))) {
                group.blendMode = static_cast<ParticleCommon::BlendMode>(currentBlend);
            }

            // ビルボードモード
            const char* billboardModes[] = { "Billboard (Camera Face)", "Velocity Aligned (Arrow)", "None (Fixed)" };
            int currentBMode = static_cast<int>(group.billboardMode);
            if (ImGui::Combo("Billboard Mode", &currentBMode, billboardModes, IM_ARRAYSIZE(billboardModes))) {
                group.billboardMode = static_cast<uint32_t>(currentBMode);
            }

            // モデル選択
            std::string currentModelName = group.model ? "Custom Model / Primitive" : "Default Plane";
            if (ImGui::BeginCombo("Model", currentModelName.c_str())) {
                bool isSelected = (group.model == nullptr);
                if (ImGui::Selectable("Default Plane", isSelected)) {
                    group.model = nullptr;
                    group.modelType = 0;
                    group.modelName = "";
                }
                if (isSelected) ImGui::SetItemDefaultFocus();

                ImGui::Separator();
                ImGui::Text("Primitives");
                const char* primNames[] = { "Ring", "Sphere", "Box", "Plane", "Torus", "Cylinder", "Cone", "Triangle" };
                for (int i = 0; i < 8; ++i) {
                    bool isPrimSelected = (group.modelType == 1 && group.modelName == std::to_string(i));
                    if (ImGui::Selectable(primNames[i], isPrimSelected)) {
                        group.model = ParticleManager::GetOrMakeParticlePrimitiveModel(i);
                        group.modelType = 1;
                        group.modelName = std::to_string(i);
                    }
                }
                ImGui::Separator();
                ImGui::Text("Files");

                for (size_t i = 0; i < modelFiles_.size(); ++i) {
                    isSelected = false; 
                    if (ImGui::Selectable(modelFiles_[i].c_str(), isSelected)) {
                        std::string path = modelFiles_[i];
                        ModelManager::GetInstance()->LoadModel(path);
                        group.model = ModelManager::GetInstance()->FindModel(path);
                        group.modelType = 2;
                        group.modelName = path;
                    }
                }
                ImGui::EndCombo();
            }

            // テクスチャ選択
            std::string currentTexName = group.texturePath.empty() ? "None (White)" : group.texturePath;
            if (ImGui::BeginCombo("Texture", currentTexName.c_str())) {
                bool isTexSelected = group.texturePath.empty();
                if (ImGui::Selectable("None (White)", isTexSelected)) {
                    std::string whiteTex = "resources/ui/white.png";
                    TextureManager::GetInstance()->LoadTexture(whiteTex);
                    group.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(whiteTex);
                    group.texturePath = "";
                }
                if (isTexSelected) ImGui::SetItemDefaultFocus();
                ImGui::Separator();
                
                for (size_t i = 0; i < textureFiles_.size(); ++i) {
                    bool isSelected = false;
                    if (ImGui::Selectable(textureFiles_[i].c_str(), isSelected)) {
                        std::string path = textureFiles_[i];
                        TextureManager::GetInstance()->LoadTexture(path);
                        group.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(path);
                        group.texturePath = path;
                    }
                }
                ImGui::EndCombo();
            }

            // Emitterパラメータ
            if (group.mappedEmitter) {
                ImGui::Separator();
                ImGui::Text("Emission Settings");
                ImGui::Checkbox("Auto Emit", &group.isAutoEmit);
                
                if (ImGui::Button("Emit Now!")) {
                    group.isEmitRequested = true;
                }

                int count = static_cast<int>(group.mappedEmitter->count);
                if (ImGui::DragInt("Count", &count, 1, 1, 1024)) {
                    group.mappedEmitter->count = static_cast<uint32_t>(count);
                }

                if (group.isAutoEmit) {
                    ImGui::DragFloat("Frequency", &group.mappedEmitter->frequency, 0.01f, 0.01f, 5.0f);
                }

                ImGui::Separator();
                ImGui::Text("Shape Settings");
                const char* shapeTypes[] = { "Sphere", "Cone", "Box" };
                int currentShape = static_cast<int>(group.mappedEmitter->shapeType);
                if (ImGui::Combo("Shape Type", &currentShape, shapeTypes, IM_ARRAYSIZE(shapeTypes))) {
                    group.mappedEmitter->shapeType = static_cast<uint32_t>(currentShape);
                }

                ImGui::DragFloat3("Translate", &group.mappedEmitter->translate.x, 0.1f);
                
                if (currentShape == 0 || currentShape == 1) { // Sphere or Cone
                    ImGui::DragFloat("Radius", &group.mappedEmitter->radius, 0.1f);
                }
                if (currentShape == 1) { // Cone
                    ImGui::DragFloat("Angle", &group.mappedEmitter->shapeAngle, 0.01f, 0.0f, 3.14159f);
                }
                if (currentShape == 2) { // Box
                    ImGui::DragFloat3("Size", &group.mappedEmitter->shapeSize.x, 0.1f);
                }

                ImGui::Separator();
                ImGui::Text("Particle Settings");
                float lifetime[2] = { group.mappedEmitter->lifeTimeMin, group.mappedEmitter->lifeTimeMax };
                if (ImGui::DragFloat2("LifeTime (Min/Max)", lifetime, 0.1f, 0.1f, 10.0f)) {
                    group.mappedEmitter->lifeTimeMin = lifetime[0];
                    group.mappedEmitter->lifeTimeMax = lifetime[1];
                }

                ImGui::DragFloat3("Velocity Base", &group.mappedEmitter->velocityBase.x, 0.01f);
                ImGui::DragFloat("Velocity Variance", &group.mappedEmitter->velocityVariance, 0.01f, 0.0f, 5.0f);
                ImGui::DragFloat3("Acceleration (Gravity)", &group.mappedEmitter->acceleration.x, 0.01f);

                ImGui::ColorEdit4("Start Color", &group.mappedEmitter->startColor.x);
                ImGui::ColorEdit4("End Color", &group.mappedEmitter->endColor.x);
            }

            ImGui::TreePop();
        }
    }

    ImGui::End();
}

void ParticleEditor::Save(const std::string& filename) {
    json root = json::array();

    auto& particleGroups_ = ParticleManager::GetInstance()->GetParticleGroups();

    for (const auto& [name, group] : particleGroups_) {
        json g;
        g["name"] = name;
        g["texturePath"] = group.texturePath;
        g["modelType"] = group.modelType;
        g["modelName"] = group.modelName;
        g["blendMode"] = static_cast<int>(group.blendMode);
        g["billboardMode"] = group.billboardMode;
        g["isAutoEmit"] = group.isAutoEmit;

        if (group.mappedEmitter) {
            json e;
            e["count"] = group.mappedEmitter->count;
            e["frequency"] = group.mappedEmitter->frequency;
            e["translate"] = { group.mappedEmitter->translate.x, group.mappedEmitter->translate.y, group.mappedEmitter->translate.z };
            e["radius"] = group.mappedEmitter->radius;
            e["lifeTimeMin"] = group.mappedEmitter->lifeTimeMin;
            e["lifeTimeMax"] = group.mappedEmitter->lifeTimeMax;
            e["velocityBase"] = { group.mappedEmitter->velocityBase.x, group.mappedEmitter->velocityBase.y, group.mappedEmitter->velocityBase.z };
            e["velocityVariance"] = group.mappedEmitter->velocityVariance;
            e["startColor"] = { group.mappedEmitter->startColor.x, group.mappedEmitter->startColor.y, group.mappedEmitter->startColor.z, group.mappedEmitter->startColor.w };
            e["endColor"] = { group.mappedEmitter->endColor.x, group.mappedEmitter->endColor.y, group.mappedEmitter->endColor.z, group.mappedEmitter->endColor.w };
            e["shapeType"] = group.mappedEmitter->shapeType;
            e["shapeAngle"] = group.mappedEmitter->shapeAngle;
            e["shapeSize"] = { group.mappedEmitter->shapeSize.x, group.mappedEmitter->shapeSize.y, group.mappedEmitter->shapeSize.z };
            e["acceleration"] = { group.mappedEmitter->acceleration.x, group.mappedEmitter->acceleration.y, group.mappedEmitter->acceleration.z };
            g["emitter"] = e;
        }

        root.push_back(g);
    }

    std::filesystem::path dir("Resources/Particles");
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
    std::string path = dir.string() + "/" + filename;

    std::ofstream file(path);
    if (file.is_open()) {
        file << root.dump(4);
    }
}

void ParticleEditor::Load(const std::string& filename) {
    std::string path = "Resources/Particles/" + filename;
    std::ifstream file(path);
    if (!file.is_open()) return;

    json root;
    file >> root;

    auto mgr = ParticleManager::GetInstance();
    auto& particleGroups_ = mgr->GetParticleGroups();

    particleGroups_.clear();

    for (const auto& g : root) {
        std::string name = g["name"];
        std::string texturePath = g["texturePath"];
        int modelType = g["modelType"];
        std::string modelName = g["modelName"];

        if (modelType == 0) {
            mgr->CreateParticleGroup(name, texturePath);
        } else if (modelType == 1) {
            int primIndex = 0;
            try { primIndex = std::stoi(modelName); } catch(...) {}
            Model* model = ParticleManager::GetOrMakeParticlePrimitiveModel(primIndex);
            mgr->CreateParticleGroup(name, model);
        } else if (modelType == 2) {
            ModelManager::GetInstance()->LoadModel(modelName);
            Model* model = ModelManager::GetInstance()->FindModel(modelName);
            mgr->CreateParticleGroup(name, model);
        }

        auto& group = particleGroups_[name];
        group.texturePath = texturePath;
        if (texturePath != "") {
            TextureManager::GetInstance()->LoadTexture(texturePath);
            group.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(texturePath);
        } else {
            std::string whiteTex = "resources/ui/white.png";
            TextureManager::GetInstance()->LoadTexture(whiteTex);
            group.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(whiteTex);
        }

        group.modelType = modelType;
        group.modelName = modelName;
        group.blendMode = static_cast<ParticleCommon::BlendMode>(g["blendMode"].get<int>());
        group.billboardMode = g["billboardMode"];
        group.isAutoEmit = g["isAutoEmit"];

        if (g.contains("emitter") && group.mappedEmitter) {
            auto e = g["emitter"];
            group.mappedEmitter->count = e["count"];
            group.mappedEmitter->frequency = e["frequency"];
            group.mappedEmitter->translate = { e["translate"][0], e["translate"][1], e["translate"][2] };
            group.mappedEmitter->radius = e["radius"];
            group.mappedEmitter->lifeTimeMin = e["lifeTimeMin"];
            group.mappedEmitter->lifeTimeMax = e["lifeTimeMax"];
            group.mappedEmitter->velocityBase = { e["velocityBase"][0], e["velocityBase"][1], e["velocityBase"][2] };
            group.mappedEmitter->velocityVariance = e["velocityVariance"];
            group.mappedEmitter->startColor = { e["startColor"][0], e["startColor"][1], e["startColor"][2], e["startColor"][3] };
            group.mappedEmitter->endColor = { e["endColor"][0], e["endColor"][1], e["endColor"][2], e["endColor"][3] };
            group.mappedEmitter->shapeType = e["shapeType"];
            group.mappedEmitter->shapeAngle = e["shapeAngle"];
            group.mappedEmitter->shapeSize = { e["shapeSize"][0], e["shapeSize"][1], e["shapeSize"][2] };
            group.mappedEmitter->acceleration = { e["acceleration"][0], e["acceleration"][1], e["acceleration"][2] };
        }
    }
}
