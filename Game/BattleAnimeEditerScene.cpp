#include "BattleAnimeEditerScene.h"
#include "GameApp.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <imgui.h>

namespace fs = std::filesystem;

void BattleAnimeEditerScene::OnEnter(GameApp& app) {
    input_ = app.GetInput();

    ModelManager::GetInstance()->LoadModel("ground/ground.obj");
    ModelManager::GetInstance()->LoadModel("skydome/skydome.obj");

    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate({ 0.0f, 4.0f, -20.0f });
    app.ObjCom()->SetDefaultCamera(camera_.get());

    cameraAnim_ = std::make_unique<CameraAnimator>();
    cameraAnim_->Initialize(camera_.get());
    ReloadCameraFileList_();
    ReloadAnimationFileList_();

    light_.lightingMode = 2;
    light_.dirIntensity = 1.6f;
    light_.pointIntensity = 2.5f;
    light_.spotIntensity = 0.0f;

    ground_ = std::make_unique<Object3d>();
    ground_->Initialize(app.ObjCom(), app.Dx());
    ground_->SetModel("ground/ground.obj");
    ground_->SetTranslate({ 0.0f, -0.05f, 0.0f });
    ground_->SetScale({ 1.0f, 1.0f, 1.0f });
    ground_->SetEnableLighting(2);
    ground_->SetIntensity(2.0f);
    ground_->SetLightColor(light_.dirColor);

    skyDome_ = std::make_unique<Object3d>();
    skyDome_->Initialize(app.ObjCom(), app.Dx());
    skyDome_->SetModel("skydome/skydome.obj");
    skyDome_->SetCamera(camera_.get());

    player_ = std::make_unique<Player>();
    player_->Initialize(app.ObjCom(), app.Dx(), camera_.get());
    player_->SetSpawnPos({ -5.0f, 0.0f, 5.0f });
    if (player_->GetObject3d()) {
        player_->GetObject3d()->SetEnableLighting(light_.lightingMode);
        player_->GetObject3d()->SetDirection(light_.dir);
        player_->GetObject3d()->SetIntensity(light_.dirIntensity);
        player_->GetObject3d()->SetLightColor(light_.dirColor);
    }

    enemyMgr_.Initialize(app.ObjCom(), app.Dx(), camera_.get());
    enemyMgr_.Spawn(EnemyType::Boss, { 5.0f, 0.0f, 5.0f });
    enemyMgr_.SetLighting(light_);

    actionDirector_.Initialize(app.SpriteCom(), app.Dx());

    animationEditTarget_ = player_->GetObject3d();
    cameraEditTarget_ = camera_.get();
}

void BattleAnimeEditerScene::OnExit(GameApp& app) {
    ground_.reset();
    skyDome_.reset();
    player_.reset();
    camera_.reset();
}

void BattleAnimeEditerScene::Update(GameApp& app, float dt) {
    if (!input_) return;

    if (input_->IsKeyPressed(DIK_ESCAPE)) {
        app.RequestQuit();
        return;
    }

    // --- Animation/Camera Edit Logic ---
    bool isEditing = false;
    if (editorTargetKind_ == EditorTargetKind::Camera && cameraAnim_) {
        if (!cameraAnim_->IsEditing() && cameraAnim_->GetPlaying()) {
            cameraAnim_->Update(dt);
        } else {
            isEditing = true;
        }
    } else if (editorTargetKind_ == EditorTargetKind::Animation) {
        // Animation editor suppresses debug camera
        isEditing = true;
    }

    // --- Debug Camera Control ---
    if (!isEditing) {
        POINT mousePos = input_->GetMousePosition();
        if (input_->IsMouseTrigger(1)) {
            isRightClickDragging_ = true;
            lastMousePos_ = mousePos;
        } else if (input_->IsMouseReleased(1)) {
            isRightClickDragging_ = false;
        }

        if (isRightClickDragging_) {
            float dx = (mousePos.x - lastMousePos_.x) * 0.005f;
            float dy = (mousePos.y - lastMousePos_.y) * 0.005f;
            Vector3 rot = camera_->GetRotate();
            rot.y += dx;
            rot.x += dy;
            camera_->SetRotate(rot);
            lastMousePos_ = mousePos;
        }

        Vector3 pos = camera_->GetTranslate();
        float ry = camera_->GetRotate().y;
        Vector3 forward = { std::sinf(ry), 0.0f, std::cosf(ry) };
        Vector3 right = { std::cosf(ry), 0.0f, -std::sinf(ry) };

        float speed = 0.5f;
        if (input_->IsKeyPressed(DIK_LSHIFT)) speed *= 3.0f;
        if (input_->IsKeyPressed(DIK_W)) { pos.x += forward.x * speed; pos.z += forward.z * speed; }
        if (input_->IsKeyPressed(DIK_S)) { pos.x -= forward.x * speed; pos.z -= forward.z * speed; }
        if (input_->IsKeyPressed(DIK_D)) { pos.x += right.x * speed; pos.z += right.z * speed; }
        if (input_->IsKeyPressed(DIK_A)) { pos.x -= right.x * speed; pos.z -= right.z * speed; }
        if (input_->IsKeyPressed(DIK_E)) { pos.y += speed; }
        if (input_->IsKeyPressed(DIK_Q)) { pos.y -= speed; }

        camera_->SetTranslate(pos);
        camera_->Update();
    }

    // --- Action Director Update ---
    if (actionDirector_.IsPlaying()) {
        if (actionDirector_.GetProfile().enableCameraWork && actionDirector_.GetCinematicCamera()) {
            app.ObjCom()->SetDefaultCamera(actionDirector_.GetCinematicCamera());
        }

        if (actionDirector_.Update(dt)) {
            // Sequence Finished
            app.ObjCom()->SetDefaultCamera(camera_.get());
        }
    } else {
        app.ObjCom()->SetDefaultCamera(camera_.get());
    }

    if (ground_) ground_->Update(dt);
    if (player_) player_->Update(dt);
    enemyMgr_.Update(dt);
}

void BattleAnimeEditerScene::Draw3D(GameApp& app) {
    app.Dx()->SetBackBuffer();
    app.ObjCom()->SetGraphicsPipelineState();
    if (skyDome_) skyDome_->Draw();
    if (ground_) ground_->Draw();
    if (player_) player_->Draw();
    enemyMgr_.Draw();
}

void BattleAnimeEditerScene::Draw2D(GameApp& app) {
    app.SpriteCom()->SetGraphicsPipelineState();
    actionDirector_.Draw2D();
}

void BattleAnimeEditerScene::DrawImGui(GameApp& app) {
#ifdef USE_IMGUI
    Enemy* targetEnemy = nullptr;
    if (!enemyMgr_.GetEnemies().empty()) {
        targetEnemy = &enemyMgr_.GetEnemies().back();
    }
    actionDirector_.DrawImGuiEditor(camera_.get(), player_.get(), targetEnemy);

    if (animationEditTarget_ || cameraEditTarget_) {
        animationEditor_.DrawImGui(BuildEditorContext_());
    }

    // Animation File Browser Window
    if (editorTargetKind_ == EditorTargetKind::Animation) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Animation Browser")) {
            if (ImGui::Button("Reload Files")) {
                ReloadAnimationFileList_();
            }

            ImGui::Separator();
            ImGui::Text("Animation Target:");
            if (ImGui::RadioButton("Player", animationEditTarget_ == player_->GetObject3d())) {
                animationEditTarget_ = player_->GetObject3d();
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Boss", animationEditTarget_ == targetEnemy->GetObject3d())) {
                animationEditTarget_ = targetEnemy->GetObject3d();
            }
            ImGui::Separator();

            ImGui::Text("Found Animations (%d)", (int)animationFiles_.size());
            ImGui::BeginChild("AnimList", ImVec2(0, 200), true);
            for (int i = 0; i < animationFiles_.size(); ++i) {
                if (ImGui::Selectable(fs::path(animationFiles_[i]).filename().string().c_str())) {
                    animationEditor_.SetExportFileName(animationFiles_[i]);
                }
            }
            ImGui::EndChild();
            ImGui::TextWrapped("Select an animation above, then click 'Load JSON' in the Animation Toolbar!");
        }
        ImGui::End();
    }

#endif
}

void BattleAnimeEditerScene::ReloadCameraFileList_() {
    cameraFiles_.clear();
    std::string path = "resources/camera";
    if (!fs::exists(path)) {
        fs::create_directories(path);
        return;
    }
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            cameraFiles_.push_back(entry.path().string());
        }
    }
    std::sort(cameraFiles_.begin(), cameraFiles_.end());
}

void BattleAnimeEditerScene::ReloadAnimationFileList_() {
    animationFiles_.clear();
    std::string path = "Resources/CustomAnim";
    if (!fs::exists(path)) {
        fs::create_directories(path);
        return;
    }
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            animationFiles_.push_back(entry.path().string());
        }
    }
    std::sort(animationFiles_.begin(), animationFiles_.end());
}

bool BattleAnimeEditerScene::LoadCameraByIndex_(int index) {
    if (index < 0 || index >= static_cast<int>(cameraFiles_.size())) return false;
    return LoadCameraByPath_(cameraFiles_[index]);
}

bool BattleAnimeEditerScene::LoadCameraByPath_(const std::string& path) {
    if (!cameraAnim_) return false;
    if (!cameraAnim_->LoadFromJson(path)) return false;
    auto it = std::find(cameraFiles_.begin(), cameraFiles_.end(), path);
    if (it != cameraFiles_.end()) currentCameraIndex_ = static_cast<int>(std::distance(cameraFiles_.begin(), it));
    cameraAnim_->SetLoop(sameCameraLoopEnabled_);
    cameraAnim_->SetPlaying(true);
    return true;
}

AnimationEditorSession::EditorContext BattleAnimeEditerScene::BuildEditorContext_() {
    AnimationEditorSession::EditorContext context{};
    context.editorCamera = cameraEditTarget_ ? cameraEditTarget_ : camera_.get();
    context.cameraAnimator = cameraAnim_.get();
    context.canEditAnimation = (animationEditTarget_ != nullptr);
    context.canEditCamera = (cameraEditTarget_ != nullptr && cameraAnim_ != nullptr);
    context.editCameraMode = (editorTargetKind_ == EditorTargetKind::Camera);
    context.cameraFiles = &cameraFiles_;
    context.currentCameraIndex = currentCameraIndex_;
    context.randomCameraEnabled = &randomCameraEnabled_;
    context.sameCameraLoopEnabled = &sameCameraLoopEnabled_;
    context.switchToAnimation = [this]() { editorTargetKind_ = EditorTargetKind::Animation; };
    context.switchToCamera = [this]() { editorTargetKind_ = EditorTargetKind::Camera; };
    context.reloadCameraFiles = [this]() { ReloadCameraFileList_(); };
    context.loadCameraByIndex = [this](int index) { LoadCameraByIndex_(index); };
    context.playRandomCamera = [this]() { /* No random camera in editor */ };

    if (editorTargetKind_ == EditorTargetKind::Camera) {
        context.cameraTarget = cameraEditTarget_;
        context.animationTarget = nullptr;
    } else {
        context.animationTarget = animationEditTarget_;
        context.cameraTarget = nullptr;
    }

    return context;
}
