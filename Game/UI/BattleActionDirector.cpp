#include "BattleActionDirector.h"
#include "Sprite.h"
#include "MathStruct.h"
#include "WinApp.h"
#include "Camera.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <cstring>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include "player/Player.h"
#include "enemy/Enemy.h"
#include "Camera/CameraAnimator.h"
#include "AnimationClipDocument.h"
#include "Object3d.h"
#include "Model.h"

// =====================================
// JSON Serialization
// =====================================
nlohmann::json ActionSequenceProfile::ToJson() const {
    return nlohmann::json{
        {"cutinDuration", cutinDuration},
        {"cutinBgColor", {cutinBgColor.x, cutinBgColor.y, cutinBgColor.z, cutinBgColor.w}},
        {"cutinLineColor", {cutinLineColor.x, cutinLineColor.y, cutinLineColor.z, cutinLineColor.w}},
        {"enableCameraWork", enableCameraWork},
        {"cameraPos", {cameraPos.x, cameraPos.y, cameraPos.z}},
        {"cameraRot", {cameraRot.x, cameraRot.y, cameraRot.z}},
        {"cameraFov", cameraFov},
        {"cameraAnimFile", cameraAnimFile},
        {"playerPos", {playerPos.x, playerPos.y, playerPos.z}},
        {"playerRot", {playerRot.x, playerRot.y, playerRot.z}},
        {"enemyPos", {enemyPos.x, enemyPos.y, enemyPos.z}},
        {"approachDuration", approachDuration},
        {"attackWaitDuration", attackWaitDuration},
        {"returnDuration", returnDuration},
        {"playerAttackAnim", playerAttackAnim},
        {"playerAttackAnimStartTime", playerAttackAnimStartTime},
        {"enemyDamageAnim", enemyDamageAnim},
        {"enemyDamageAnimStartTime", enemyDamageAnimStartTime}
    };
}

void ActionSequenceProfile::FromJson(const nlohmann::json& j) {
    cutinDuration = j.value("cutinDuration", cutinDuration);
    if (j.contains("cutinBgColor")) {
        cutinBgColor = { j["cutinBgColor"][0], j["cutinBgColor"][1], j["cutinBgColor"][2], j["cutinBgColor"][3] };
    }
    if (j.contains("cutinLineColor")) {
        cutinLineColor = { j["cutinLineColor"][0], j["cutinLineColor"][1], j["cutinLineColor"][2], j["cutinLineColor"][3] };
    }
    enableCameraWork = j.value("enableCameraWork", enableCameraWork);
    if (j.contains("cameraPos")) {
        cameraPos = { j["cameraPos"][0], j["cameraPos"][1], j["cameraPos"][2] };
    }
    if (j.contains("cameraRot")) {
        cameraRot = { j["cameraRot"][0], j["cameraRot"][1], j["cameraRot"][2] };
    }
    cameraFov = j.value("cameraFov", cameraFov);
    cameraAnimFile = j.value("cameraAnimFile", cameraAnimFile);
    if (j.contains("playerPos")) {
        playerPos = { j["playerPos"][0], j["playerPos"][1], j["playerPos"][2] };
    }
    if (j.contains("playerRot")) {
        playerRot = { j["playerRot"][0], j["playerRot"][1], j["playerRot"][2] };
    }
    if (j.contains("enemyPos")) {
        enemyPos = { j["enemyPos"][0], j["enemyPos"][1], j["enemyPos"][2] };
    }
    approachDuration = j.value("approachDuration", approachDuration);
    attackWaitDuration = j.value("attackWaitDuration", attackWaitDuration);
    returnDuration = j.value("returnDuration", returnDuration);
    playerAttackAnim = j.value("playerAttackAnim", playerAttackAnim);
    playerAttackAnimStartTime = j.value("playerAttackAnimStartTime", playerAttackAnimStartTime);
    enemyDamageAnim = j.value("enemyDamageAnim", enemyDamageAnim);
    enemyDamageAnimStartTime = j.value("enemyDamageAnimStartTime", enemyDamageAnimStartTime);
}

// =====================================
// Easing
// =====================================
namespace {
    float EaseOutQuart(float t) {
        float t1 = t - 1.0f;
        return 1.0f - (t1 * t1 * t1 * t1);
    }

    float AbsSum(const Vector3& v) {
        return std::abs(v.x) + std::abs(v.y) + std::abs(v.z);
    }

    bool IsCameraAnimStatic(const std::vector<CameraKeyframe>& keys) {
        if (keys.size() <= 1) {
            return true;
        }

        constexpr float kPositionEpsilon = 0.0001f;
        constexpr float kRotationEpsilon = 0.0001f;
        constexpr float kFovEpsilon = 0.0001f;
        const CameraKeyframe& first = keys.front();

        for (const CameraKeyframe& key : keys) {
            Vector3 posDelta{
                key.pos.x - first.pos.x,
                key.pos.y - first.pos.y,
                key.pos.z - first.pos.z
            };
            Vector3 rotDelta{
                key.rot.x - first.rot.x,
                key.rot.y - first.rot.y,
                key.rot.z - first.rot.z
            };

            if (AbsSum(posDelta) > kPositionEpsilon ||
                AbsSum(rotDelta) > kRotationEpsilon ||
                std::abs(key.fov - first.fov) > kFovEpsilon) {
                return false;
            }
        }

        return true;
    }
}

// =====================================
// BattleActionDirector
// =====================================

void BattleActionDirector::Initialize(SpriteCommon* spriteCom, DirectXCommon* dx) {
    cutinBg_ = std::make_unique<Sprite>();
    cutinBg_->Initialize(spriteCom, dx, "resources/ui/white.png");
    
    cutinLine_ = std::make_unique<Sprite>();
    cutinLine_->Initialize(spriteCom, dx, "resources/ui/white.png");
    cutinLine_->SetRotation(Vector3(0.0f, 0.0f, 0.2f)); 

    cinematicCam_ = std::make_unique<Camera>();
    cinematicCam_->SetTranslate(profile_.cameraPos);
    cinematicCam_->SetRotate(profile_.cameraRot);
    cinematicCam_->SetFovY(profile_.cameraFov);

    cinematicCamAnim_ = std::make_unique<CameraAnimator>();
    cinematicCamAnim_->Initialize(cinematicCam_.get());

    playerAnimDoc_ = std::make_unique<AnimationClipDocument>();
    enemyAnimDoc_ = std::make_unique<AnimationClipDocument>();
}

void BattleActionDirector::SetProfile(const ActionSequenceProfile& profile) {
    profile_ = profile;
}

void BattleActionDirector::SetProfilePath(const std::string& path) {
    strcpy_s(profileFilename_, path.c_str());
}

void BattleActionDirector::SaveOriginalState_() {
    hasOriginalState_ = true;

    if (player_) {
        originalPlayerPos_ = player_->GetPos();
    }
    if (target_) {
        originalEnemyPos_ = target_->GetPos();
    }
    if (cinematicCam_) {
        originalCameraPos_ = cinematicCam_->GetTranslate();
        originalCameraRot_ = cinematicCam_->GetRotate();
        originalCameraFov_ = cinematicCam_->GetFovY();
    }
}

void BattleActionDirector::RestoreOriginalState_() {
    if (!hasOriginalState_) {
        return;
    }

    if (player_) {
        player_->SetSpawnPos(originalPlayerPos_);
        if (player_->GetObject3d()) {
            player_->GetObject3d()->SetTranslate(originalPlayerPos_);
        }
    }
    if (target_) {
        target_->SetPosition(originalEnemyPos_);
    }
    if (cinematicCam_) {
        cinematicCam_->SetTranslate(originalCameraPos_);
        cinematicCam_->SetRotate(originalCameraRot_);
        cinematicCam_->SetFovY(originalCameraFov_);
        cinematicCam_->Update();
    }

    hasOriginalState_ = false;
}

void BattleActionDirector::StartAction(Player* player, Enemy* target) {
    player_ = player;
    target_ = target;
    SaveOriginalState_();
    
    // いきなりアニメーション再生フェーズから開始する
    phase_ = ActionPhase::Attack;
    timer_ = 0.0f;
    duration_ = profile_.attackWaitDuration; 

    // Initial Placement
    if (player_) {
        player_->SetSpawnPos(profile_.playerPos);
        player_->SetRotation(profile_.playerRot);  // Blender の向きを反映
        if (player_->GetObject3d()) {
            player_->GetObject3d()->SetTranslate(profile_.playerPos);
            player_->GetObject3d()->SetRotate(profile_.playerRot);
        }
    }
    if (target_) target_->SetPosition(profile_.enemyPos);

    // Preload Animation JSONs and inject to Model
    if (player_ && player_->GetObject3d() && player_->GetObject3d()->GetModel()) {
        if (!profile_.playerAttackAnim.empty()) {
            if (playerAnimDoc_->LoadFromJson(profile_.playerAttackAnim)) {
                player_->GetObject3d()->GetModel()->AddAnimation("SequenceAttack", playerAnimDoc_->GetAnimation());
            }
        }
    }

    if (target_ && target_->GetObject3d() && target_->GetObject3d()->GetModel()) {
        if (!profile_.enemyDamageAnim.empty()) {
            if (enemyAnimDoc_->LoadFromJson(profile_.enemyDamageAnim)) {
                target_->GetObject3d()->GetModel()->AddAnimation("SequenceDamage", enemyAnimDoc_->GetAnimation());
            }
        }
    }

    // カメラアニメーションもすぐに開始
    cameraAnimIsStatic_ = false;
    if (!profile_.cameraAnimFile.empty()) {
        cinematicCamAnim_->LoadFromJson(profile_.cameraAnimFile);
        cameraAnimIsStatic_ = IsCameraAnimStatic(cinematicCamAnim_->GetKeyframes());
        cinematicCamAnim_->SetLoop(false);
        cinematicCamAnim_->SetPlaying(true);
        cinematicCamAnim_->SetCurrentTime(0.0f);
        cinematicCamAnim_->SampleAtTime(0.0f);
        if (cinematicCam_) {
            cinematicCam_->Update();
        }
        
        float camDuration = cinematicCamAnim_->GetMaxTime();
        if (camDuration > 0.0f) {
            duration_ = camDuration;
        }
    }

    // アニメーション再生フラグをリセット。実際の再生はUpdateで行う
    hasPlayedPlayerAnim_ = false;
    hasPlayedEnemyAnim_ = false;
}

bool BattleActionDirector::Update(float dt) {
    if (phase_ == ActionPhase::Idle) {
        return false;
    }

    timer_ += dt;
    easeT_ = std::clamp(timer_ / std::max(duration_, 0.001f), 0.0f, 1.0f);

    if (profile_.enableCameraWork) {
        const bool hasCameraAnim = cinematicCamAnim_ && !profile_.cameraAnimFile.empty();
        if (phase_ == ActionPhase::Attack && hasCameraAnim && !cameraAnimIsStatic_) {
            cinematicCamAnim_->Update(dt);
        } else if (hasCameraAnim) {
            cinematicCamAnim_->SampleAtTime(0.0f);
            if (cinematicCam_) {
                cinematicCam_->Update();
            }
        } else {
            cinematicCam_->SetTranslate(profile_.cameraPos);
            cinematicCam_->SetRotate(profile_.cameraRot);
            cinematicCam_->SetFovY(profile_.cameraFov);
            cinematicCam_->Update();
        }
    }

    if (phase_ == ActionPhase::Cutin) {
        cutinBg_->SetColor(profile_.cutinBgColor);
        cutinBg_->SetScale({(float)WinApp::kClientWidth, 300.0f, 1.0f});
        float bgX = std::lerp(-(float)WinApp::kClientWidth, 0.0f, EaseOutQuart(easeT_));
        cutinBg_->SetPosition({bgX, (float)WinApp::kClientHeight / 2.0f - 150.0f});

        cutinLine_->SetColor(profile_.cutinLineColor);
        cutinLine_->SetScale({(float)WinApp::kClientWidth * 1.5f, 50.0f, 1.0f});
        float lineX = std::lerp((float)WinApp::kClientWidth, -((float)WinApp::kClientWidth * 1.5f), easeT_);
        cutinLine_->SetPosition({lineX, (float)WinApp::kClientHeight / 2.0f});

        if (timer_ >= duration_) {
            // Cutin -> Approach
            phase_ = ActionPhase::Approach;
            timer_ = 0.0f;
            duration_ = profile_.approachDuration;
        }
    }
    else if (phase_ == ActionPhase::Approach) {
        if (timer_ >= duration_) {
            // Approach -> Attack
            phase_ = ActionPhase::Attack;
            timer_ = 0.0f;
            duration_ = profile_.attackWaitDuration;
            hasPlayedPlayerAnim_ = false;
            hasPlayedEnemyAnim_ = false;
            
            // Trigger camera animation
            if (cinematicCamAnim_ && !profile_.cameraAnimFile.empty()) {
                cinematicCamAnim_->SetCurrentTime(0.0f);
                cinematicCamAnim_->SetPlaying(true);
            }
        }
    }
    else if (phase_ == ActionPhase::Attack) {
        // キャラクターアニメーションのディレイ再生
        if (!hasPlayedPlayerAnim_ && timer_ >= profile_.playerAttackAnimStartTime) {
            if (player_ && player_->GetObject3d() && playerAnimDoc_->HasValidClip()) {
                player_->GetObject3d()->PlayAnimation("SequenceAttack", false);
            }
            hasPlayedPlayerAnim_ = true;
        }
        if (!hasPlayedEnemyAnim_ && timer_ >= profile_.enemyDamageAnimStartTime) {
            if (target_ && target_->GetObject3d() && enemyAnimDoc_->HasValidClip()) {
                target_->GetObject3d()->PlayAnimation("SequenceDamage", false);
            }
            hasPlayedEnemyAnim_ = true;
        }

        if (timer_ >= duration_) {
            phase_ = ActionPhase::Idle;
            RestoreOriginalState_();

            // --- アクション終了時に待機モーションに戻す ---
            if (player_ && player_->GetObject3d() && player_->GetObject3d()->GetModel()) {
                auto& anims = player_->GetObject3d()->GetModel()->GetAnimations();
                if (anims.find("CustomAnim") != anims.end()) {
                    player_->GetObject3d()->PlayAnimation("CustomAnim", true);
                } else if (anims.find("Idle") != anims.end()) {
                    player_->GetObject3d()->PlayAnimation("Idle", true);
                }
            }
            if (target_ && target_->GetObject3d() && target_->GetObject3d()->GetModel()) {
                auto& anims = target_->GetObject3d()->GetModel()->GetAnimations();
                if (anims.find("CustomAnim") != anims.end()) {
                    target_->GetObject3d()->PlayAnimation("CustomAnim", true);
                } else if (anims.find("Idle") != anims.end()) {
                    target_->GetObject3d()->PlayAnimation("Idle", true);
                }
            }

            return true; // Finish the sequence
        }
    }
    // Returnフェーズは不要なのでスキップ

    Matrix4x4 viewMat = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 projMat = Matrix4x4::MakeOrthographicMatrix((float)WinApp::kClientWidth, (float)WinApp::kClientHeight);
    
    if (cutinBg_) cutinBg_->Update(viewMat, projMat);
    if (cutinLine_) cutinLine_->Update(viewMat, projMat);

    return false;
}

void BattleActionDirector::Draw2D() {
    // カットインの帯と背景は表示しない
    // if (phase_ == ActionPhase::Cutin) {
    //     if (cutinBg_) cutinBg_->Draw();
    //     if (cutinLine_) cutinLine_->Draw();
    // }
}

void BattleActionDirector::SaveProfile(const std::string& path) {
    nlohmann::json j = profile_.ToJson();
    std::ofstream file(path);
    if (file.is_open()) {
        file << std::setw(4) << j << std::endl;
    }
}

void BattleActionDirector::LoadProfile(const std::string& path) {
    std::ifstream file(path);
    if (file.is_open()) {
        nlohmann::json j;
        file >> j;
        profile_.FromJson(j);
    }
}

#ifdef USE_IMGUI
void BattleActionDirector::DrawImGuiEditor(Camera* debugCamera, Player* player, Enemy* target) {
    ImGui::Begin("Sequence Editor");

    ImGui::InputText("Save File", profileFilename_, sizeof(profileFilename_));
    if (ImGui::Button("Save")) {
        SaveProfile(profileFilename_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        LoadProfile(profileFilename_);
    }

    ImGui::Separator();
    if (ImGui::Button("Play Preview")) {
        StartAction(player, target); // Run with assigned character
    }

    if (ImGui::CollapsingHeader("1. Cutin Phase", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Cutin Duration", &profile_.cutinDuration, 0.01f, 0.1f, 5.0f);
        ImGui::ColorEdit4("Cutin Bg Color", &profile_.cutinBgColor.x);
        ImGui::ColorEdit4("Cutin Line Color", &profile_.cutinLineColor.x);
    }

    if (ImGui::CollapsingHeader("2. Camera Work", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable Cinematic Camera", &profile_.enableCameraWork);
        
        // --- BLENDER-LIKE FEATURE ---
        if (debugCamera) {
            if (ImGui::Button("Copy from Debug Camera")) {
                profile_.cameraPos = debugCamera->GetTranslate();
                profile_.cameraRot = debugCamera->GetRotate();
                profile_.cameraFov = debugCamera->GetFovY();
            }
        }

        ImGui::DragFloat3("Camera Pos", &profile_.cameraPos.x, 0.1f);
        ImGui::DragFloat3("Camera Rot", &profile_.cameraRot.x, 0.01f);
        ImGui::DragFloat("Camera FOV", &profile_.cameraFov, 0.01f, 0.1f, 3.14f);

        char cAnim[256];
        strcpy_s(cAnim, profile_.cameraAnimFile.c_str());
        if (ImGui::InputText("Camera Anim File (.json)", cAnim, sizeof(cAnim))) {
            profile_.cameraAnimFile = cAnim;
        }
    }

    if (ImGui::CollapsingHeader("3. Placements", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Player Start Pos", &profile_.playerPos.x, 0.1f);
        ImGui::DragFloat3("Enemy Start Pos", &profile_.enemyPos.x, 0.1f);
    }

    if (ImGui::CollapsingHeader("3. Action Timings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Approach Duration", &profile_.approachDuration, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Attack Wait Duration", &profile_.attackWaitDuration, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Return Duration", &profile_.returnDuration, 0.01f, 0.0f, 5.0f);
    }

    if (ImGui::CollapsingHeader("5. Animations", ImGuiTreeNodeFlags_DefaultOpen)) {
        char pAnim[256];
        strcpy_s(pAnim, profile_.playerAttackAnim.c_str());
        if (ImGui::InputText("Player Attack Anim (.json)", pAnim, sizeof(pAnim))) {
            profile_.playerAttackAnim = pAnim;
        }
        ImGui::DragFloat("Player Anim Start Time", &profile_.playerAttackAnimStartTime, 0.01f, 0.0f, 10.0f);

        char eAnim[256];
        strcpy_s(eAnim, profile_.enemyDamageAnim.c_str());
        if (ImGui::InputText("Enemy Damage Anim (.json)", eAnim, sizeof(eAnim))) {
            profile_.enemyDamageAnim = eAnim;
        }
        ImGui::DragFloat("Enemy Anim Start Time", &profile_.enemyDamageAnimStartTime, 0.01f, 0.0f, 10.0f);
    }

    ImGui::End();
}
#endif
