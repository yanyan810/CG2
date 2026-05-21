#pragma once
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Vector3.h"
#include "CardDef.h"
#include "CardInstance.h"
//#include "Vector4.h"

class Player;
class Enemy;
class Sprite;
class SpriteCommon;
class DirectXCommon;
class Object3dCommon;
class Camera;
class CameraAnimator;
class AnimationClipDocument;
class Card3D;
class Input;  // スキップ用


struct ActionSequenceProfile {
    // 1. カットイン設定
    float cutinDuration = 0.6f;
    Vector4 cutinBgColor = {0.0f, 0.0f, 0.0f, 0.7f};
    Vector4 cutinLineColor = {1.0f, 0.3f, 0.1f, 1.0f};

    // 2. カメラワーク設定
    bool enableCameraWork = true;
    Vector3 cameraPos = { 0.0f, 3.0f, -10.0f };
    Vector3 cameraRot = { 0.2f, 0.0f, 0.0f };
    float cameraFov = 0.8f;
    std::string cameraAnimFile = ""; // カメラアニメーションのパス(JSON)

    // 3. キャラクター設定
    Vector3 playerPos = { -5.0f, 0.0f, 5.0f };
    Vector3 playerRot = { 0.0f, 0.0f, 0.0f };  // Blender からエクスポートされた向き
    Vector3 enemyPos = { 5.0f, 0.0f, 5.0f };
    std::string playerAttackAnim = ""; // 自作アニメーションパス
    float playerAttackAnimStartTime = 0.0f; // 再生開始時間(秒)
    std::string enemyDamageAnim = "";  // 自作アニメーションパス
    float enemyDamageAnimStartTime = 0.0f; // 再生開始時間(秒)
    std::string cardMotionFile = "";

    // 4. アクション時間設定
    float approachDuration = 0.3f;
    float attackWaitDuration = 0.5f;
    float returnDuration = 0.3f;

  //  float returnDuration = 0.3f;

    nlohmann::json ToJson() const;
    void FromJson(const nlohmann::json& j);
};

enum class ActionPhase {
    Idle,
    Cutin,
    Approach,
    Attack,
    Return
};

class BattleActionDirector {
public:
    BattleActionDirector() = default;
    ~BattleActionDirector() = default;

    void Initialize(SpriteCommon* spriteCom, DirectXCommon* dx, Object3dCommon* objCom = nullptr);

    void StartAction(Player* player, Enemy* target);
    void StartAction(Player* player, Enemy* target, const CardDef& cardDef, const CardInstance& cardInstance);

    // Returns true if the entire sequence is complete
    // input: マウスクリックでスキップする場合は Input* を渡す（nullptrでスキップ無効）
    bool Update(float dt, Input* input = nullptr);

    void Draw2D();
    void Draw3D();

    Camera* GetCinematicCamera() const { return cinematicCam_.get(); }
    Enemy* GetTarget() const { return target_; }
    bool IsPlaying() const { return phase_ != ActionPhase::Idle; }
    ActionPhase GetPhase() const { return phase_; }
    float GetPlaybackTime() const { return timer_; }
    float GetPlaybackDuration() const { return duration_; }
    float GetImpactTime() const;
    bool HasReachedImpact() const { return hasReachedImpact_; }
    bool IsDebugPaused() const { return debugPaused_; }
    void SetDebugPaused(bool paused) { debugPaused_ = paused; }
    void SetDebugPlaybackTime(float time);
    bool ReloadCardMotionFromProfile();
    void SetSkipEnabled(bool enabled) { skipEnabled_ = enabled; }
    bool GetSkipEnabled() const { return skipEnabled_; }

    const ActionSequenceProfile& GetProfile() const { return profile_; }
    ActionSequenceProfile& GetProfileRef() { return profile_; }
    void SetProfile(const ActionSequenceProfile& profile);
    void SetProfilePath(const std::string& path);

#ifdef USE_IMGUI
    void DrawImGuiEditor(Camera* debugCamera, Player* player = nullptr, Enemy* target = nullptr);
#endif

    // JSON Save/Load
    void SaveProfile(const std::string& path);
    void LoadProfile(const std::string& path);

private:
    struct CardMotionKeyframe {
        float time = 0.0f;
        Vector3 pos{ 0.0f, 0.0f, 0.0f };
        Vector3 rot{ 0.0f, 0.0f, 0.0f };
        Vector3 scale{ 1.0f, 1.0f, 1.0f };
    };

    void SaveOriginalState_();
    void RestoreOriginalState_();
    void StartActionInternal_(Player* player, Enemy* target, const CardDef* cardDef, const CardInstance* cardInstance);
    void SamplePreviewAtCurrentTime_();
    bool LoadCardMotion_(const std::string& path, bool forceReload = false);
    CardMotionKeyframe SampleCardMotion_(float time) const;
    void UpdateCardMotion_(float dt);

    ActionPhase phase_ = ActionPhase::Idle;
    float timer_ = 0.0f;
    float duration_ = 1.0f;

    Player* player_ = nullptr;
    Enemy* target_ = nullptr;

    std::unique_ptr<Sprite> cutinBg_;
    std::unique_ptr<Sprite> cutinLine_;
    std::unique_ptr<Camera> cinematicCam_;
    std::unique_ptr<CameraAnimator> cinematicCamAnim_;

    // アニメーションローダーの保持
    std::unique_ptr<AnimationClipDocument> playerAnimDoc_;
    std::unique_ptr<AnimationClipDocument> enemyAnimDoc_;
    std::unique_ptr<Card3D> cardMotionCard_;
    std::vector<CardMotionKeyframe> cardMotionKeyframes_;

    ActionSequenceProfile profile_;
    char profileFilename_[128] = "resources/sequences/default_attack.json";

    // Variables for easing
    float easeT_ = 0.0f;
    
    // Animation trigger flags
    bool hasPlayedPlayerAnim_ = false;
    bool hasPlayedEnemyAnim_ = false;
    bool cameraAnimIsStatic_ = false;
    bool cardMotionVisible_ = false;
    bool hasReachedImpact_ = false;
    bool debugPaused_ = false;
    bool skipEnabled_ = true;   // マウスクリックでスキップできるか

    SpriteCommon* spriteCom_ = nullptr;
    DirectXCommon* dx_ = nullptr;
    Object3dCommon* objCom_ = nullptr;

    bool hasOriginalState_ = false;
    Vector3 originalPlayerPos_{};
    Vector3 originalPlayerRot_{};
    Vector3 originalEnemyPos_{};
    Vector3 originalCameraPos_{};
    Vector3 originalCameraRot_{};
    float originalCameraFov_ = 0.8f;

    // JSONロードキャッシュ（同じファイルは再読み込みしない）
    std::string cachedPlayerAnimPath_;
    std::string cachedEnemyAnimPath_;
    std::string cachedCameraAnimPath_;
    std::string cachedCardMotionPath_;

    // AddAnimationキャッシュ（同じアニメを同じモデルに二度コピーしない）
    void*       cachedPlayerAnimModel_    = nullptr; // 登録済みモデルポインタ
    std::string cachedPlayerAnimInjected_;           // 登録済みアニメパス
    void*       cachedEnemyAnimModel_     = nullptr;
    std::string cachedEnemyAnimInjected_;

    // Card3D再利用キャッシュ（Initializeは最初の1回だけ）
    int cachedCardDefId_ = -1;       // 前回のCardDef::id
    CardInstance cachedCardInst_{};  // 前回のCardInstance
};
