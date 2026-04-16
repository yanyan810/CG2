#pragma once
#include "IScene.h"
#include "Camera.h"
#include "CameraAnimator.h"
#include "Object3d.h"
#include "Enemy.h"
#include "Player.h"
#include "AnimationEditorSession.h"
#include "BattleController.h"
#include "TextSprite.h"
#include "StringUtility.h"
#include "FieldUi.h"
#include "ModelParticleManager.h"
#include "TrailManager.h"
#include "AudioManager.h"
#include "PausingUI/PausingUI.h"

class GameApp;

class GameScene : public IScene {
public:
    enum class EditorTargetKind {
        Animation,
        Camera,
    };

    GameScene() = default;
    ~GameScene() = default;

    void OnEnter(GameApp& app) override;
    void OnExit(GameApp& app) override;
    void Update(GameApp& app, float dt) override;
    void Draw3D(GameApp& app) override;
    void Draw2D(GameApp& app) override;
    void DrawImGui(GameApp& app) override;

    void DrawSkydome(GameApp& app) override;

    void DrawPostEffect3D(GameApp& app) override;
    void DrawPostEffect2D(GameApp& app) override;

    void ChangeRandomCamera();

private:

    
	// カメラアニメーション関連の関数
    void ReloadCameraFileList_();
    bool LoadCameraByIndex_(int index);
    bool LoadCameraByPath_(const std::string& path);
    AnimationEditorSession::EditorContext BuildEditorContext_();

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Camera> animCamera_;
    std::unique_ptr<CameraAnimator> cameraAnim_;
    std::unique_ptr<Object3d> skyDome_; // 背景の天球

    float cameraBlend_ = 0.0f;

    std::unique_ptr<Player> player_;
    Object3d* animationEditTarget_ = nullptr;
    Camera* cameraEditTarget_ = nullptr;
    AnimationEditorSession animationEditor_;
    EditorTargetKind editorTargetKind_ = EditorTargetKind::Animation;
    bool battleDebugVisible_ = true;
    bool battleEffectsDebugVisible_ = true;
    EnemyManager enemyMgr_;

    std::unique_ptr<Sprite> cardDescBg_;

    // ライトの設定
    LightingParam light_;

    BattleController battle_;
    std::unique_ptr<TextSprite> cardDescText_; //文字描画

    // ESCキーの入力状態を保持（タイトルに戻るなどの処理用）
    bool prevEsc_ = false;

    std::unique_ptr<FieldUi> fieldUi_;

    // ターン数描画関連
    std::unique_ptr<TextSprite> turnText_;
    std::unique_ptr<Sprite> turnTextBg_;

    // コスト描画関連
    std::unique_ptr<TextSprite> costText_;
    std::unique_ptr<Sprite> costTextBg_;
    Vector2 position_;
    Vector3 scale_;

    std::unique_ptr<TextSprite> playerHpText_;
    std::vector<std::unique_ptr<TextSprite>> enemyHpTexts_;

    ParticleEmitterConfig attackEffectConfig_;
    
    std::unique_ptr<Sprite> powerBoostBg_;
    std::unique_ptr<TextSprite> powerBoostText_;
    std::unique_ptr<Sprite> blockBg_;
    std::unique_ptr<TextSprite> blockText_;

    std::unique_ptr<Sprite> highlightFilter_;

    //カメラアニメ
    std::vector<std::string> cameraFiles_;
    int currentCameraIndex_ = -1;
    bool randomCameraEnabled_ = true;   // true: ランダム切替
    bool sameCameraLoopEnabled_ = false; // true: 同じアニメをループ

    ModelParticleManager* particleManager_;

    
    std::unique_ptr<TrailManager> trailManager_;
    TrailInstance* testTrail_ = nullptr; // マネージャが寿命管理するので生のポインタでOK
    TrailConfig trailConfig_;            // インスペクタ調整用

	std::unique_ptr<PausingUI> pausingUI_;
};
