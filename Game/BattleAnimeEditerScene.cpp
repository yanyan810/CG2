#include "BattleAnimeEditerScene.h"
#include "GameApp.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "CardDatabase.h"
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
    // Blender カメラ 50mm レンズ / センサー幅 36mm / 解像度 1280x720 に合わせた垂直FoV
    // 計算: 2 * atan(sensor_height / (2 * focal_length)) = 2 * atan(20.25 / 100) ≈ 0.3994 rad
    camera_->SetFovY(0.3994f);
    camera_->Update();
    app.ObjCom()->SetDefaultCamera(camera_.get());

    cameraAnim_ = std::make_unique<CameraAnimator>();
    cameraAnim_->Initialize(camera_.get());
    ReloadCameraFileList_();
    ReloadAnimationFileList_();
    ReloadSequenceFileList_();

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

    actionDirector_.Initialize(app.SpriteCom(), app.Dx(), app.ObjCom());
    LoadSequenceByPath_("resources/sequences/test_useCard_1.json");
    testCard_.defId = testCardDefId_;
    testCard_.number = 1;
    testCard_.suit = CardSuit::Spade;

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

    if (liveCameraSyncEnabled_ && !actionDirector_.IsPlaying()) {
        liveCameraPollTimer_ += dt;
        if (liveCameraPollTimer_ >= 0.2f) {
            liveCameraPollTimer_ = 0.0f;
            ReloadLiveCameraIfNeeded_(false);
        }
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
        Camera* cinCam = actionDirector_.GetCinematicCamera();
        if (actionDirector_.GetProfile().enableCameraWork && cinCam) {
            // SetDefaultCamera だけでは SetCamera() 済みのオブジェクトに効かないため
            // 各オブジェクトにも直接シネマカメラを設定する
            app.ObjCom()->SetDefaultCamera(cinCam);
            if (skyDome_)  skyDome_->SetCamera(cinCam);
            if (ground_)   ground_->SetCamera(cinCam);
            if (player_ && player_->GetObject3d()) player_->GetObject3d()->SetCamera(cinCam);
            enemyMgr_.UpdateCamera(cinCam);
        }

        if (actionDirector_.Update(dt)) {
            // Sequence Finished - デバッグカメラに戻す
            app.ObjCom()->SetDefaultCamera(camera_.get());
            if (skyDome_)  skyDome_->SetCamera(camera_.get());
            if (ground_)   ground_->SetCamera(camera_.get());
            if (player_ && player_->GetObject3d()) player_->GetObject3d()->SetCamera(camera_.get());
            enemyMgr_.UpdateCamera(camera_.get());
        }
    } else {
        camera_->Update();
        app.ObjCom()->SetDefaultCamera(camera_.get());
    }

    if (ground_) ground_->Update(dt);
    if (player_) player_->Update(dt);
    enemyMgr_.Update(dt);
}

void BattleAnimeEditerScene::DrawSkydome(GameApp& app) {
    // bloom_->PreDraw() の前に呼ばれる。SceneRT に描画する段階でスカイドームを出す
    app.Dx()->SetViewport(0, 0, WinApp::kClientWidth, WinApp::kClientHeight);
    app.Dx()->SetScissorRect(0, 0, WinApp::kClientWidth, WinApp::kClientHeight);
    app.ObjCom()->SetGraphicsPipelineState();
    if (skyDome_) skyDome_->Draw();
}

void BattleAnimeEditerScene::DrawPostEffect3D(GameApp& app) {
    // bloom_->PreDraw() 〜 bloom_->PostDraw() の間に呼ばれる (SceneRT に描画)
    app.Dx()->SetViewport(0, 0, WinApp::kClientWidth, WinApp::kClientHeight);
    app.Dx()->SetScissorRect(0, 0, WinApp::kClientWidth, WinApp::kClientHeight);
    app.ObjCom()->SetGraphicsPipelineState();
    app.Dx()->ClearDepthBuffer();
    if (ground_) ground_->Draw();
    actionDirector_.Draw3D();
    if (player_) player_->Draw();
    enemyMgr_.Draw();
}

void BattleAnimeEditerScene::Draw3D(GameApp& app) {
    // bloom_->PostDraw() の後に呼ばれる。バックバッファに直接描く追加描画があればここに
    // (現在は DrawSkydome / DrawPostEffect3D に移動したため空)
    (void)app;
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
    DrawSequenceDebugWindow_(app, targetEnemy);

    if (animationEditTarget_ || cameraEditTarget_) {
        animationEditor_.DrawImGui(BuildEditorContext_());
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

void BattleAnimeEditerScene::ReloadSequenceFileList_() {
    sequenceFiles_.clear();
    std::string path = "resources/sequences";
    if (!fs::exists(path)) {
        fs::create_directories(path);
        return;
    }

    for (const auto& entry : fs::directory_iterator(path)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        if (entry.path().filename() == "card_sequence_map.json") {
            continue;
        }
        if (entry.path().stem().string().ends_with("_camera")) {
            continue;
        }
        sequenceFiles_.push_back(entry.path().string());
    }
    std::sort(sequenceFiles_.begin(), sequenceFiles_.end());
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

bool BattleAnimeEditerScene::LoadSequenceByIndex_(int index) {
    if (index < 0 || index >= static_cast<int>(sequenceFiles_.size())) return false;
    return LoadSequenceByPath_(sequenceFiles_[index]);
}

bool BattleAnimeEditerScene::LoadSequenceByPath_(const std::string& path) {
    actionDirector_.SetProfilePath(path);
    actionDirector_.LoadProfile(path);

    auto it = std::find(sequenceFiles_.begin(), sequenceFiles_.end(), path);
    if (it != sequenceFiles_.end()) {
        currentSequenceIndex_ = static_cast<int>(std::distance(sequenceFiles_.begin(), it));
    }
    RefreshLiveCameraPath_();
    ApplyProfilePositions_(enemyMgr_.GetEnemies().empty() ? nullptr : &enemyMgr_.GetEnemies().back(), false);
    if (liveCameraSyncEnabled_) {
        ReloadLiveCameraIfNeeded_(true);
    }
    return true;
}

void BattleAnimeEditerScene::RefreshLiveCameraPath_() {
    const std::string& cameraPath = actionDirector_.GetProfile().cameraAnimFile;
    if (liveCameraPath_ == cameraPath) {
        return;
    }

    liveCameraPath_ = cameraPath;
    liveCameraLastWriteTime_ = {};
    liveCameraStatus_ = liveCameraPath_.empty()
        ? "Loaded sequence has no cameraAnimFile."
        : "Watching " + liveCameraPath_;
}

bool BattleAnimeEditerScene::ReloadLiveCameraIfNeeded_(bool force) {
    RefreshLiveCameraPath_();
    if (liveCameraPath_.empty()) {
        return false;
    }
    if (!fs::exists(liveCameraPath_)) {
        liveCameraStatus_ = "Camera JSON not found: " + liveCameraPath_;
        return false;
    }

    const auto writeTime = fs::last_write_time(liveCameraPath_);
    if (!force && writeTime == liveCameraLastWriteTime_) {
        return false;
    }

    if (!cameraAnim_ || !cameraAnim_->LoadFromJson(liveCameraPath_)) {
        liveCameraStatus_ = "Failed to load camera JSON: " + liveCameraPath_;
        return false;
    }

    liveCameraLastWriteTime_ = writeTime;
    cameraAnim_->SetPlaying(false);
    cameraAnim_->SampleAtTime(0.0f);
    camera_->Update();
    liveCameraStatus_ = "Synced " + fs::path(liveCameraPath_).filename().string();
    return true;
}

void BattleAnimeEditerScene::ApplyProfilePositions_(Enemy* targetEnemy, bool applyCamera) {
    const ActionSequenceProfile& profile = actionDirector_.GetProfile();
    if (player_) {
        player_->SetSpawnPos(profile.playerPos);
        player_->SetRotation(profile.playerRot);
        if (player_->GetObject3d()) {
            player_->GetObject3d()->SetTranslate(profile.playerPos);
            player_->GetObject3d()->SetRotate(profile.playerRot);
        }
    }
    if (targetEnemy) {
        targetEnemy->SetPosition(profile.enemyPos);
    }
    if (applyCamera && camera_) {
        camera_->SetTranslate(profile.cameraPos);
        camera_->SetRotate(profile.cameraRot);
        camera_->SetFovY(profile.cameraFov);
        camera_->Update();
    }
}

void BattleAnimeEditerScene::DrawSequenceDebugWindow_(GameApp& app, Enemy* targetEnemy) {
    (void)app;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Battle Anime Editor")) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Reload Sequences")) {
        ReloadSequenceFileList_();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload Camera")) {
        ReloadLiveCameraIfNeeded_(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Play Loaded Sequence") && player_ && targetEnemy) {
        if (const CardDef* testDef = GetTestCardDef_(app)) {
            actionDirector_.StartAction(player_.get(), targetEnemy, *testDef, testCard_);
        } else {
            actionDirector_.StartAction(player_.get(), targetEnemy);
        }
    }

    if (ImGui::Checkbox("Live Blender Camera", &liveCameraSyncEnabled_)) {
        if (liveCameraSyncEnabled_) {
            ReloadLiveCameraIfNeeded_(true);
        }
    }
    ImGui::TextWrapped("%s", liveCameraStatus_.c_str());
    if (player_) {
        bool releaseAnimationEnabled = player_->GetReleaseAnimationEnabled();
        if (ImGui::Checkbox("Player Release Animations", &releaseAnimationEnabled)) {
            player_->SetReleaseAnimationEnabled(releaseAnimationEnabled);
        }
    }

    ImGui::Separator();
    ImGui::Text("Sequence Files");
    ImGui::BeginChild("SequenceList", ImVec2(0, 160), true);
    for (int i = 0; i < static_cast<int>(sequenceFiles_.size()); ++i) {
        const std::string label = fs::path(sequenceFiles_[i]).filename().string();
        if (ImGui::Selectable(label.c_str(), i == currentSequenceIndex_)) {
            LoadSequenceByIndex_(i);
        }
    }
    ImGui::EndChild();

    const ActionSequenceProfile& profile = actionDirector_.GetProfile();
    ImGui::Separator();
    ImGui::Text("Profile: %s", currentSequenceIndex_ >= 0 ? fs::path(sequenceFiles_[currentSequenceIndex_]).filename().string().c_str() : "(none)");
    ImGui::Text("Camera: %s", profile.cameraAnimFile.empty() ? "(none)" : profile.cameraAnimFile.c_str());
    {
        // シーケンス再生中はシネマカメラの値を表示、それ以外はデバッグカメラ
        Camera* dispCam = (actionDirector_.IsPlaying() && actionDirector_.GetCinematicCamera())
            ? actionDirector_.GetCinematicCamera()
            : camera_.get();
        if (dispCam) {
            const Vector3 pos = dispCam->GetTranslate();
            const Vector3 rot = dispCam->GetRotate();
            ImGui::Text(actionDirector_.IsPlaying() ? "[Cinematic] Pos: %.3f, %.3f, %.3f" : "Camera Pos: %.3f, %.3f, %.3f", pos.x, pos.y, pos.z);
            ImGui::Text(actionDirector_.IsPlaying() ? "[Cinematic] Rot: %.3f, %.3f, %.3f" : "Camera Rot: %.3f, %.3f, %.3f", rot.x, rot.y, rot.z);
        }
    }

    if (ImGui::Button("Apply Profile Positions")) {
        ApplyProfilePositions_(targetEnemy, true);
        if (liveCameraSyncEnabled_) {
            ReloadLiveCameraIfNeeded_(true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Scene Positions")) {
        if (player_) {
            player_->SetSpawnPos({ -5.0f, 0.0f, 5.0f });
            if (player_->GetObject3d()) {
                player_->GetObject3d()->SetTranslate({ -5.0f, 0.0f, 5.0f });
            }
        }
        if (targetEnemy) {
            targetEnemy->SetPosition({ 5.0f, 0.0f, 5.0f });
        }
        if (camera_) {
            camera_->SetTranslate({ 0.0f, 4.0f, -20.0f });
            camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
            camera_->Update();
        }
        if (Camera* cinematicCamera = actionDirector_.GetCinematicCamera()) {
            cinematicCamera->SetTranslate({ 0.0f, 4.0f, -20.0f });
            cinematicCamera->SetRotate({ 0.0f, 0.0f, 0.0f });
            cinematicCamera->Update();
        }
    }

    DrawCardRevealEditor_(app);

    ImGui::End();
}

const CardDef* BattleAnimeEditerScene::GetTestCardDef_(GameApp& app) const {
    CardDatabase* db = app.GetCardDB();
    return db ? db->Find(testCardDefId_) : nullptr;
}

void BattleAnimeEditerScene::DrawCardRevealEditor_(GameApp& app) {
    ActionSequenceProfile& profile = actionDirector_.GetProfileRef();
    ActionCardRevealProfile& reveal = profile.cardReveal;

    ImGui::Separator();
    if (!ImGui::CollapsingHeader("Card Use Reveal", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::Checkbox("Enable Card Reveal", &reveal.enabled);
    ImGui::Checkbox("Player Relative", &reveal.playerRelative);
    ImGui::DragFloat("Start Time", &reveal.startTime, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("Duration", &reveal.duration, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat3("Start Pos", &reveal.startPos.x, 0.05f);
    ImGui::DragFloat3("End Pos", &reveal.endPos.x, 0.05f);
    ImGui::DragFloat3("Start Rot", &reveal.startRot.x, 0.01f);
    ImGui::DragFloat3("End Rot", &reveal.endRot.x, 0.01f);
    ImGui::DragFloat3("Start Scale", &reveal.startScale.x, 0.02f, 0.01f, 10.0f);
    ImGui::DragFloat3("End Scale", &reveal.endScale.x, 0.02f, 0.01f, 10.0f);
    ImGui::DragFloat("Glitter", &reveal.glitter, 0.02f, 0.0f, 10.0f);

    ImGui::Separator();
    ImGui::DragInt("Test Card ID", &testCardDefId_, 1.0f, 1, 100);
    testCard_.defId = testCardDefId_;
    ImGui::DragInt("Test Number", &testCard_.number, 1.0f, 1, 13);
    int suit = static_cast<int>(testCard_.suit);
    if (ImGui::Combo("Test Suit", &suit, "Spade\0Heart\0Diamond\0Club\0")) {
        testCard_.suit = static_cast<CardSuit>(std::clamp(suit, 0, 3));
    }

    if (ImGui::Button("Save Loaded Sequence")) {
        if (currentSequenceIndex_ >= 0 && currentSequenceIndex_ < static_cast<int>(sequenceFiles_.size())) {
            actionDirector_.SaveProfile(sequenceFiles_[currentSequenceIndex_]);
            ReloadSequenceFileList_();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Preview Card Reveal")) {
        Enemy* targetEnemy = enemyMgr_.GetEnemies().empty() ? nullptr : &enemyMgr_.GetEnemies().back();
        if (player_ && targetEnemy) {
            if (const CardDef* testDef = GetTestCardDef_(app)) {
                actionDirector_.StartAction(player_.get(), targetEnemy, *testDef, testCard_);
            } else {
                actionDirector_.StartAction(player_.get(), targetEnemy);
            }
        }
    }
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
