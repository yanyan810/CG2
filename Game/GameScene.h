#pragma once
#include <array>
#include <string>
#include "IScene.h"
#include "Camera.h"
#include "CameraAnimator.h"
#include "Object3d.h"
#include "Enemy.h"
#include "Player.h"
#include "AnimationEditorSession.h"
#include "BattleController.h"
#include "TextSprite.h"
#include "Sprite.h"
#include "StringUtility.h"
#include "FieldUi.h"
#include "ModelParticleManager.h"
#include "TrailManager.h"
#include "AudioManager.h"
#include "PausingUI/PausingUI.h"
#include "EffectSequencer.h"
#include "BloomConstantBuffer.h"
#include "GameResultPopup.h"
#include "UI/PokerHandHelpView.h"
#include "PropManager.h"

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
    void BeginEnterPreparation(GameApp& app);
    bool PrepareEnterStep(GameApp& app);
    float GetEnterPreparationProgress() const;
    bool IsEnterPreparationComplete() const { return enterPreparationComplete_; }

private:

    
	// カメラアニメーション関連の関数
    void ReloadCameraFileList_();
    bool LoadCameraByIndex_(int index);
    bool LoadCameraByPath_(const std::string& path);
    AnimationEditorSession::EditorContext BuildEditorContext_();
    void ResetParticleObjectPostParam_();
    void DrawParticleObjectPostEditor_();
    void DrawBattleAnimationDebugWindow_();
    void DrawPlayerHudImGui_();
    void UpdateReleaseDebugText_();
    void PrepareEnterReset_(GameApp& app);

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Camera> animCamera_;
    std::unique_ptr<CameraAnimator> cameraAnim_;
    std::unique_ptr<PropManager> battleForestProps_;
    std::string stageFieldConfigPath_;
    std::unique_ptr<Object3d> skyDome_; // 背景の天球

    float splitRatio_ = 0.465f; // 画面分割割合（上がバトル画面、下がカード画面）
    float fieldCameraZoom_ = 1.0f;
    float fieldCameraRotXOffset_ = 0.08f;
    float battleCameraZoom_ = 1.0f;
    float battleCameraRotXOffset_ = -0.08f;

    float cameraBlend_ = 0.0f;

    std::unique_ptr<Player> player_;
    Object3d* animationEditTarget_ = nullptr;
    Camera* cameraEditTarget_ = nullptr;
    AnimationEditorSession animationEditor_;
    EditorTargetKind editorTargetKind_ = EditorTargetKind::Animation;
    bool battleDebugVisible_ = true;
    bool battleEffectsDebugVisible_ = true;
    bool forceActionCameraLookAt_ = false;
    bool releaseDebugVisible_ = false;
    EnemyManager enemyMgr_;

    std::unique_ptr<Sprite> cardDescBg_;
    std::unique_ptr<Sprite> startFadeMask_;
    float startFadeTimer_ = 0.0f;
    float startFadeDuration_ = 0.75f;
    bool startFadeActive_ = false;

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

    std::unique_ptr<TextSprite> releaseDebugText_;

    ParticleEmitterConfig attackEffectConfig_;
    
    std::unique_ptr<Sprite> highlightFilter_;
    std::unique_ptr<Sprite> bossStageBannerEffectOverlay_;
    std::unique_ptr<Sprite> bossStageBannerBg_;
    std::unique_ptr<TextSprite> bossStageBannerGlowText_;
    std::unique_ptr<TextSprite> bossStageBannerText_;
    bool isBossStage_ = false;
    float bossStageBannerTimer_ = 0.0f;

    //カメラアニメ
    std::vector<std::string> cameraFiles_;
    int currentCameraIndex_ = -1;
    bool randomCameraEnabled_ = true;   // true: ランダム切替
    bool sameCameraLoopEnabled_ = false; // true: 同じアニメをループ

    ModelParticleManager* particleManager_;
    std::unique_ptr<ModelParticleManager> fieldParticleManager_;
    bool particleObjectPostEnabled_ = true;
    BloomParam particleObjectPostParam_{};

    
    std::unique_ptr<TrailManager> trailManager_;
    TrailInstance* testTrail_ = nullptr; // マネージャが寿命管理するので生のポインタでOK
    TrailConfig trailConfig_;            // インスペクタ調整用

	std::unique_ptr<PausingUI> pausingUI_;

	// エフェクトシーケンサー（攻撃エフェクトエディター）
	std::unique_ptr<EffectSequencer> effectSequencer_;

    int battleEndTimer_ = 180;
    bool clearTransitionActive_ = false;
    float clearTransitionTimer_ = 0.0f;
    Vector3 clearBaseFieldCameraPos_{};
    Vector3 clearBaseFieldCameraRot_{};
    Vector3 clearBaseBattleCameraPos_{};
    Vector3 clearBaseBattleCameraRot_{};

    // ゲーム結果ポップアップ
    std::unique_ptr<GameResultPopup> resultPopup_;
    std::unique_ptr<PokerHandHelpView> pokerHandHelpView_;
    int enterPreparationStep_ = 0;
    bool enterPreparationComplete_ = false;
    bool gameResultShown_ = false; // ポップアップ表示済みフラグ
};

