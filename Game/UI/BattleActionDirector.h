#pragma once
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "Vector3.h"
//#include "Vector4.h"

class Player;
class Enemy;
class Sprite;
class SpriteCommon;
class DirectXCommon;
class Camera;
class CameraAnimator;
class AnimationClipDocument;

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

    void Initialize(SpriteCommon* spriteCom, DirectXCommon* dx);

    void StartAction(Player* player, Enemy* target);

    // Returns true if the entire sequence is complete
    bool Update(float dt);

    void Draw2D();

    Camera* GetCinematicCamera() const { return cinematicCam_.get(); }
    bool IsPlaying() const { return phase_ != ActionPhase::Idle; }
    ActionPhase GetPhase() const { return phase_; }

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
    void SaveOriginalState_();
    void RestoreOriginalState_();

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

    ActionSequenceProfile profile_;
    char profileFilename_[128] = "resources/sequences/default_attack.json";

    // Variables for easing
    float easeT_ = 0.0f;
    
    // Animation trigger flags
    bool hasPlayedPlayerAnim_ = false;
    bool hasPlayedEnemyAnim_ = false;
    bool cameraAnimIsStatic_ = false;

    bool hasOriginalState_ = false;
    Vector3 originalPlayerPos_{};
    Vector3 originalEnemyPos_{};
    Vector3 originalCameraPos_{};
    Vector3 originalCameraRot_{};
    float originalCameraFov_ = 0.8f;
};
