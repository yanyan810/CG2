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
#include "ScopedTimer.h"  // パフォーマンス計測用
#include "Input/Input.h"  // スキップ用
#include <cstdint>
#include <filesystem>
#include <unordered_map>


#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include "player/Player.h"
#include "enemy/Enemy.h"
#include "Camera/CameraAnimator.h"
#include "AnimationClipDocument.h"
#include "AnimationJsonSerializer.h"
#include "Object3d.h"
#include "Model.h"
#include "Card3D.h"

namespace {
    struct CachedAnimation {
        std::filesystem::file_time_type writeTime{};
        Animation animation{};
    };

    struct CachedCameraAnimation {
        std::filesystem::file_time_type writeTime{};
        CameraAnimator::StateSnapshot snapshot{};
    };

    struct InjectedAnimation {
        std::string path;
        std::filesystem::file_time_type writeTime{};
    };

    bool LoadAnimationCached(const std::string& path, AnimationClipDocument& document) {
        namespace fs = std::filesystem;
        if (path.empty() || !fs::exists(path)) {
            return false;
        }

        static std::unordered_map<std::string, CachedAnimation> cache;
        const auto writeTime = fs::last_write_time(path);
        auto it = cache.find(path);
        if (it != cache.end() && it->second.writeTime == writeTime) {
            document.ReplaceWith(it->second.animation);
            return true;
        }

        Animation animation;
        if (!AnimationJsonSerializer::LoadFromJson(path, animation)) {
            return false;
        }

        cache[path] = CachedAnimation{ writeTime, animation };
        document.ReplaceWith(animation);
        return true;
    }

    bool LoadCameraAnimationCached(const std::string& path, CameraAnimator& animator) {
        namespace fs = std::filesystem;
        if (path.empty() || !fs::exists(path)) {
            return false;
        }

        static std::unordered_map<std::string, CachedCameraAnimation> cache;
        const auto writeTime = fs::last_write_time(path);
        auto it = cache.find(path);
        if (it != cache.end() && it->second.writeTime == writeTime) {
            animator.RestoreState(it->second.snapshot);
            return true;
        }

        if (!animator.LoadFromJson(path)) {
            return false;
        }

        cache[path] = CachedCameraAnimation{ writeTime, animator.CaptureState() };
        return true;
    }

    bool InjectAnimationCached(
        const std::string& path,
        AnimationClipDocument& document,
        Model& model,
        const std::string& animationName)
    {
        namespace fs = std::filesystem;
        if (path.empty() || !fs::exists(path)) {
            return false;
        }

        static std::unordered_map<std::string, InjectedAnimation> injected;
        const auto writeTime = fs::last_write_time(path);
        const std::string key =
            std::to_string(reinterpret_cast<std::uintptr_t>(&model)) + ":" + animationName;

        auto it = injected.find(key);
        const auto& animations = model.GetAnimations();
        const bool alreadyInjected =
            it != injected.end() &&
            it->second.path == path &&
            it->second.writeTime == writeTime &&
            animations.find(animationName) != animations.end();

        if (alreadyInjected) {
            return true;
        }

        if (!LoadAnimationCached(path, document)) {
            return false;
        }

        model.AddAnimation(animationName, document.GetAnimation());
        injected[key] = InjectedAnimation{ path, writeTime };
        return true;
    }
}

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
        {"enemyDamageAnimStartTime", enemyDamageAnimStartTime},
        {"cardMotionFile", cardMotionFile}
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
    cardMotionFile = j.value("cardMotionFile", cardMotionFile);
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

    Vector3 LerpVec3(const Vector3& a, const Vector3& b, float t) {
        return {
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t
        };
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

void BattleActionDirector::Initialize(SpriteCommon* spriteCom, DirectXCommon* dx, Object3dCommon* objCom) {
    spriteCom_ = spriteCom;
    dx_ = dx;
    objCom_ = objCom;

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

void BattleActionDirector::SetDebugPlaybackTime(float time) {
	if (phase_ == ActionPhase::Idle) {
		return;
	}

	timer_ = std::clamp(time, 0.0f, std::max(duration_, 0.0f));
	easeT_ = std::clamp(timer_ / std::max(duration_, 0.001f), 0.0f, 1.0f);
	hasReachedImpact_ = phase_ == ActionPhase::Attack && timer_ >= GetImpactTime();
	SamplePreviewAtCurrentTime_();
}

float BattleActionDirector::GetImpactTime() const {
	const float fallbackImpact = std::max(0.0f, duration_);
	const float profileImpact = profile_.enemyDamageAnimStartTime > 0.0f
		? profile_.enemyDamageAnimStartTime
		: fallbackImpact;
	return std::clamp(profileImpact, 0.0f, fallbackImpact);
}

bool BattleActionDirector::ReloadCardMotionFromProfile() {
    if (profile_.cardMotionFile.empty()) {
        cardMotionKeyframes_.clear();
        cardMotionVisible_ = false;
        return false;
    }

    if (!LoadCardMotion_(profile_.cardMotionFile, true)) {
        return false;
    }

    duration_ = std::max(duration_, cardMotionKeyframes_.back().time);
    SamplePreviewAtCurrentTime_();
    return true;
}

void BattleActionDirector::SamplePreviewAtCurrentTime_() {
    if (profile_.enableCameraWork) {
        const bool hasCameraAnim = cinematicCamAnim_ && !profile_.cameraAnimFile.empty();
        if (phase_ == ActionPhase::Attack && hasCameraAnim && !cameraAnimIsStatic_) {
            cinematicCamAnim_->SetCurrentTime(timer_);
            cinematicCamAnim_->SampleAtTime(timer_);
            if (cinematicCam_) {
                cinematicCam_->Update();
            }
        } else if (hasCameraAnim) {
            cinematicCamAnim_->SampleAtTime(0.0f);
            if (cinematicCam_) {
                cinematicCam_->Update();
            }
        } else if (cinematicCam_) {
            cinematicCam_->SetTranslate(profile_.cameraPos);
            cinematicCam_->SetRotate(profile_.cameraRot);
            cinematicCam_->SetFovY(profile_.cameraFov);
            cinematicCam_->Update();
        }
    }

    UpdateCardMotion_(0.0f);
}

void BattleActionDirector::SaveOriginalState_() {
    hasOriginalState_ = true;

    if (player_) {
        originalPlayerPos_ = player_->GetPos();
        originalPlayerRot_ = player_->GetRotation();
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
        player_->SetRotation(originalPlayerRot_);
        if (player_->GetObject3d()) {
            player_->GetObject3d()->SetTranslate(originalPlayerPos_);
            player_->GetObject3d()->SetRotate(originalPlayerRot_);
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
    StartActionInternal_(player, target, nullptr, nullptr);
}

void BattleActionDirector::StartAction(Player* player, Enemy* target, const CardDef& cardDef, const CardInstance& cardInstance) {
    StartActionInternal_(player, target, &cardDef, &cardInstance);
}

void BattleActionDirector::StartActionInternal_(Player* player, Enemy* target, const CardDef* cardDef, const CardInstance* cardInstance) {
    ScopedTimer timerTotal("StartActionInternal [TOTAL]", 2.0f);

    player_ = player;
    target_ = target;
    SaveOriginalState_();
    debugPaused_ = false;
    // ★ cardMotionCard_ と cardMotionKeyframes_ は意図的にリセットしない
    //    cachedCardMotionPath_ があるため reset/clear すると次回JSON未読み込みのまま演出が消える
    cardMotionVisible_ = false;
    
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

    {
        ScopedTimer t("  CardMotion Initialize", 2.0f);
        Camera* cardCamera = (profile_.enableCameraWork && cinematicCam_) ? cinematicCam_.get() : nullptr;
        if (!cardCamera && player_ && player_->GetObject3d()) {
            cardCamera = player_->GetObject3d()->GetCamera();
        }
        if (!profile_.cardMotionFile.empty() && cardDef && cardInstance && objCom_ && dx_ && cardCamera) {
            // 同じカードモーションファイルならJSONキャッシュを使う
            if (profile_.cardMotionFile != cachedCardMotionPath_) {
                ScopedTimer t2("    CardMotion JSON Load", 1.0f);
                if (LoadCardMotion_(profile_.cardMotionFile)) {
                    cachedCardMotionPath_ = profile_.cardMotionFile;
                } else {
                    cachedCardMotionPath_.clear();
                }
            }
            if (!cardMotionKeyframes_.empty()) {
                bool needInit = !cardMotionCard_;  // まだ一度も作っていない
                if (needInit) {
                    // ★初回のみ Initialize（GPUバッファ確保 = 重い処理）
                    ScopedTimer t3("    Card3D::Initialize [FIRST]", 0.0f);
                    cardMotionCard_ = std::make_unique<Card3D>();
                    cardMotionCard_->Initialize(objCom_, dx_, cardCamera, *cardDef, *cardInstance);
                    cachedCardDefId_ = cardDef->id;
                    cachedCardInst_  = *cardInstance;
                } else {
                    // ★2回目以降：カードが変わった場合のみ SetCardData を呼ぶ
                    if (cardDef->id != cachedCardDefId_ ||
                        cardInstance->number != cachedCardInst_.number ||
                        cardInstance->suit   != cachedCardInst_.suit) {
                        cardMotionCard_->SetCardData(*cardDef, *cardInstance);
                        cachedCardDefId_ = cardDef->id;
                        cachedCardInst_  = *cardInstance;
                    }
                    // カメラだけ最新のものに更新
                    cardMotionCard_->SetCamera(cardCamera);
                }
                cardMotionCard_->SetIsHand(false);
                const CardMotionKeyframe first = SampleCardMotion_(0.0f);
                cardMotionCard_->SetTransform(first.pos, first.rot, first.scale);
                duration_ = std::max(duration_, cardMotionKeyframes_.back().time);
            }
        }
    }

    // Preload Animation JSONs and inject to Model
    {
        ScopedTimer t("  PlayerAnim LoadFromJson", 2.0f);
        if (player_ && player_->GetObject3d() && player_->GetObject3d()->GetModel()) {
            if (!profile_.playerAttackAnim.empty()) {
                // 同じパスなら再読み込みをスキップ
                if (profile_.playerAttackAnim != cachedPlayerAnimPath_) {
                    if (playerAnimDoc_->LoadFromJson(profile_.playerAttackAnim)) {
                        cachedPlayerAnimPath_ = profile_.playerAttackAnim;
                    } else {
                        cachedPlayerAnimPath_.clear();
                    }
                    // パスが変わった場合はAddAnimationキャッシュを無効化
                    cachedPlayerAnimInjected_.clear();
                }
                // ★ 同じモデル ✕ 同じパスに既に登録済みならコピーをスキップ
                void* modelPtr = player_->GetObject3d()->GetModel();
                bool alreadyInjected = (cachedPlayerAnimModel_ == modelPtr &&
                                        cachedPlayerAnimInjected_ == cachedPlayerAnimPath_);
                if (!alreadyInjected && playerAnimDoc_->HasValidClip()) {
                    player_->GetObject3d()->GetModel()->AddAnimation("SequenceAttack", playerAnimDoc_->GetAnimation());
                    cachedPlayerAnimModel_    = modelPtr;
                    cachedPlayerAnimInjected_ = cachedPlayerAnimPath_;
                }
            }
        }
    }

    {
        ScopedTimer t("  EnemyAnim LoadFromJson", 2.0f);
        if (target_ && target_->GetObject3d() && target_->GetObject3d()->GetModel()) {
            if (!profile_.enemyDamageAnim.empty()) {
                // 同じパスなら再読み込みをスキップ
                if (profile_.enemyDamageAnim != cachedEnemyAnimPath_) {
                    if (enemyAnimDoc_->LoadFromJson(profile_.enemyDamageAnim)) {
                        cachedEnemyAnimPath_ = profile_.enemyDamageAnim;
                    } else {
                        cachedEnemyAnimPath_.clear();
                    }
                    // パスが変わった場合はAddAnimationキャッシュを無効化
                    cachedEnemyAnimInjected_.clear();
                }
                // ★ 同じモデル ✕ 同じパスに既に登録済みならコピーをスキップ
                void* modelPtr = target_->GetObject3d()->GetModel();
                bool alreadyInjected = (cachedEnemyAnimModel_ == modelPtr &&
                                        cachedEnemyAnimInjected_ == cachedEnemyAnimPath_);
                if (!alreadyInjected && enemyAnimDoc_->HasValidClip()) {
                    target_->GetObject3d()->GetModel()->AddAnimation("SequenceDamage", enemyAnimDoc_->GetAnimation());
                    cachedEnemyAnimModel_    = modelPtr;
                    cachedEnemyAnimInjected_ = cachedEnemyAnimPath_;
                }
            }
        }
    }

    // カメラアニメーションもすぐに開始
    {
        ScopedTimer t("  CameraAnim LoadFromJson", 2.0f);
        cameraAnimIsStatic_ = false;
        if (!profile_.cameraAnimFile.empty()) {
            // 同じパスなら再読み込みをスキップ
            if (profile_.cameraAnimFile != cachedCameraAnimPath_) {
                cinematicCamAnim_->LoadFromJson(profile_.cameraAnimFile);
                cachedCameraAnimPath_ = profile_.cameraAnimFile;
            }
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
                duration_ = std::max(duration_, camDuration);
            }
        }
    }

    // アニメーション再生フラグをリセット。実際の再生はUpdateで行う
	hasPlayedPlayerAnim_ = false;
	hasPlayedEnemyAnim_ = false;
	hasReachedImpact_ = false;
}

bool BattleActionDirector::Update(float dt, Input* input) {
    if (phase_ == ActionPhase::Idle) {
        return false;
    }

    // ★マウス左クリックでスキップ（Cutin以外のフェーズで有効）
    if (skipEnabled_ && !debugPaused_ && input && input->IsMouseTrigger(0)) {
        if (phase_ != ActionPhase::Cutin) {
            // 全フェーズを即終了させる
            timer_ = duration_;
        }
    }

    const float stepDt = debugPaused_ ? 0.0f : dt;
    timer_ += stepDt;
    easeT_ = std::clamp(timer_ / std::max(duration_, 0.001f), 0.0f, 1.0f);

    if (profile_.enableCameraWork) {
        const bool hasCameraAnim = cinematicCamAnim_ && !profile_.cameraAnimFile.empty();
        if (phase_ == ActionPhase::Attack && hasCameraAnim && !cameraAnimIsStatic_) {
            if (debugPaused_) {
                cinematicCamAnim_->SetCurrentTime(timer_);
                cinematicCamAnim_->SampleAtTime(timer_);
                if (cinematicCam_) {
                    cinematicCam_->Update();
                }
            } else {
                cinematicCamAnim_->Update(stepDt);
            }
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

    UpdateCardMotion_(stepDt);

    if (phase_ == ActionPhase::Cutin) {
        cutinBg_->SetColor(profile_.cutinBgColor);
        cutinBg_->SetScale({(float)WinApp::kClientWidth, 300.0f, 1.0f});
        float bgX = std::lerp(-(float)WinApp::kClientWidth, 0.0f, EaseOutQuart(easeT_));
        cutinBg_->SetPosition({bgX, (float)WinApp::kClientHeight / 2.0f - 150.0f});

        cutinLine_->SetColor(profile_.cutinLineColor);
        cutinLine_->SetScale({(float)WinApp::kClientWidth * 1.5f, 50.0f, 1.0f});
        float lineX = std::lerp((float)WinApp::kClientWidth, -((float)WinApp::kClientWidth * 1.5f), easeT_);
        cutinLine_->SetPosition({lineX, (float)WinApp::kClientHeight / 2.0f});

        if (!debugPaused_ && timer_ >= duration_) {
            // Cutin -> Approach
            phase_ = ActionPhase::Approach;
            timer_ = 0.0f;
            duration_ = profile_.approachDuration;
        }
    }
    else if (phase_ == ActionPhase::Approach) {
        if (!debugPaused_ && timer_ >= duration_) {
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
        if (timer_ >= GetImpactTime()) {
            hasReachedImpact_ = true;
        }

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

        if (!debugPaused_ && timer_ >= duration_) {
            phase_ = ActionPhase::Idle;
            RestoreOriginalState_();
            cardMotionVisible_ = false;
            // ★ cardMotionCard_ と cardMotionKeyframes_ はキャッシュとして保持する
            //    reset()/clear() するとcachedCardMotionPath_が残ったまま
            //    次回JSON再読み込みがスキップされてカード演出が発生しなくなるため

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

bool BattleActionDirector::LoadCardMotion_(const std::string& path, bool forceReload) {
    cardMotionKeyframes_.clear();

    namespace fs = std::filesystem;
    if (path.empty() || !fs::exists(path)) {
        return false;
    }

    struct CachedCardMotion {
        fs::file_time_type writeTime{};
        std::vector<CardMotionKeyframe> keyframes;
    };
    static std::unordered_map<std::string, CachedCardMotion> cache;

    const auto writeTime = fs::last_write_time(path);
    auto cached = cache.find(path);
    if (!forceReload && cached != cache.end() && cached->second.writeTime == writeTime) {
        cardMotionKeyframes_ = cached->second.keyframes;
        return !cardMotionKeyframes_.empty();
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    nlohmann::json j;
    file >> j;
    if (!j.contains("keyframes") || !j["keyframes"].is_array()) {
        return false;
    }

    auto readVec3 = [](const nlohmann::json& value, const Vector3& fallback) {
        if (!value.is_array() || value.size() < 3) {
            return fallback;
        }
        return Vector3{
            value[0].get<float>(),
            value[1].get<float>(),
            value[2].get<float>()
        };
    };

    for (const auto& key : j["keyframes"]) {
        CardMotionKeyframe frame{};
        frame.time = key.value("time", 0.0f);
        frame.pos = readVec3(key.value("pos", nlohmann::json::array()), frame.pos);
        frame.rot = readVec3(key.value("rot", nlohmann::json::array()), frame.rot);
        frame.scale = readVec3(key.value("scale", nlohmann::json::array()), frame.scale);
        cardMotionKeyframes_.push_back(frame);
    }

    std::sort(cardMotionKeyframes_.begin(), cardMotionKeyframes_.end(),
        [](const CardMotionKeyframe& a, const CardMotionKeyframe& b) {
            return a.time < b.time;
        });

    if (!cardMotionKeyframes_.empty()) {
        cache[path] = CachedCardMotion{ writeTime, cardMotionKeyframes_ };
    }

    return !cardMotionKeyframes_.empty();
}

BattleActionDirector::CardMotionKeyframe BattleActionDirector::SampleCardMotion_(float time) const {
    if (cardMotionKeyframes_.empty()) {
        return {};
    }
    if (time <= cardMotionKeyframes_.front().time) {
        return cardMotionKeyframes_.front();
    }
    if (time >= cardMotionKeyframes_.back().time) {
        return cardMotionKeyframes_.back();
    }

    for (size_t i = 1; i < cardMotionKeyframes_.size(); ++i) {
        const CardMotionKeyframe& next = cardMotionKeyframes_[i];
        if (time > next.time) {
            continue;
        }

        const CardMotionKeyframe& prev = cardMotionKeyframes_[i - 1];
        const float span = std::max(next.time - prev.time, 0.001f);
        const float t = std::clamp((time - prev.time) / span, 0.0f, 1.0f);
        CardMotionKeyframe result{};
        result.time = time;
        result.pos = LerpVec3(prev.pos, next.pos, t);
        result.rot = LerpVec3(prev.rot, next.rot, t);
        result.scale = LerpVec3(prev.scale, next.scale, t);
        return result;
    }

    return cardMotionKeyframes_.back();
}

void BattleActionDirector::UpdateCardMotion_(float dt) {
    if (!cardMotionCard_ || cardMotionKeyframes_.empty()) {
        return;
    }

    if (profile_.enableCameraWork && cinematicCam_) {
        cardMotionCard_->SetCamera(cinematicCam_.get());
    }

    cardMotionVisible_ = timer_ >= cardMotionKeyframes_.front().time;
    if (!cardMotionVisible_) {
        return;
    }

    const CardMotionKeyframe frame = SampleCardMotion_(timer_);
    cardMotionCard_->SetTransform(frame.pos, frame.rot, frame.scale);
    cardMotionCard_->Update(dt);
}

void BattleActionDirector::Draw3D() {
    if (cardMotionVisible_ && cardMotionCard_) {
        cardMotionCard_->Draw();
    }
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
