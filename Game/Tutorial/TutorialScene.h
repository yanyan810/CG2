#pragma once
#include "IScene.h"
#include <memory>
#include "TextSprite.h"
#include "Sprite.h"
#include <array>
#include <vector>

#include "Camera.h"
#include "CameraAnimator.h"
#include "Object3d.h"
#include "Enemy.h"
#include "Player.h"
#include "BattleController.h"
#include "FieldUi.h"
#include "ModelParticleManager.h"
#include "BloomConstantBuffer.h"
#include "TutorialManager.h"
#include "TutorialUi.h"
#include "UI/PokerHandHelpView.h"
#include "PropManager.h"

class GameApp;
class TextSprite;
class Sprite;

class TutorialScene : public IScene {
public:
    TutorialScene() = default;
    ~TutorialScene() = default;

    void OnEnter(GameApp& app) override;
    void OnExit(GameApp& app) override;
    void Update(GameApp& app, float dt) override;
    void Draw3D(GameApp& app) override;
    void Draw2D(GameApp& app) override;
    void DrawImGui(GameApp& app) override;

    void DrawSkydome(GameApp& app) override;
    void DrawPostEffect3D(GameApp& app) override;
    void DrawPostEffect2D(GameApp& app) override;

private:
    void ResetParticleObjectPostParam_();
    void InitializeTutorialContent_(GameApp& app);
    void InitializeTutorialMenu_(GameApp& app);
    void UpdateTutorialMenu_(GameApp& app);
    void DrawTutorialMenu_(GameApp& app);
    void ShowTutorialMenu_(int page);
    void StartTutorialChapter_(GameApp& app, TutorialManager::TutorialChapter chapter);
    void ReturnToTitle_();
    void ReturnToTutorialMenu_(GameApp& app);

    enum class State {
        EnterOpen,
        Idle,
        ExitClose
    };

    struct TutorialMenuButton {
        UiRect rect;
        std::wstring label;
        TutorialManager::TutorialChapter chapter = TutorialManager::TutorialChapter::Full;
        int nextPage = -1;
        std::unique_ptr<Sprite> bg;
        std::unique_ptr<TextSprite> text;
        bool hovered = false;
    };

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Camera> animCamera_;
    std::unique_ptr<CameraAnimator> cameraAnim_;
    std::unique_ptr<Object3d> skyDome_;
    std::unique_ptr<PropManager> tutorialFieldProps_;

    float splitRatio_ = 0.465f;
    float fieldCameraZoom_ = 1.0f;
    float fieldCameraRotXOffset_ = 0.08f;
    float battleCameraZoom_ = 1.0f;
    float battleCameraRotXOffset_ = -0.08f;
    float cameraBlend_ = 0.0f;

    std::unique_ptr<Player> player_;
    EnemyManager enemyMgr_;

    BattleController battle_;
    std::unique_ptr<FieldUi> fieldUi_;
    ModelParticleManager* particleManager_ = nullptr;
    std::unique_ptr<ModelParticleManager> fieldParticleManager_;
    bool particleObjectPostEnabled_ = true;
    BloomParam particleObjectPostParam_{};

    std::unique_ptr<TutorialManager> tutorial_;
    std::unique_ptr<TutorialUi> tutorialUi_;
    std::vector<TutorialMenuButton> tutorialMenuButtons_;
    std::unique_ptr<Sprite> tutorialMenuBg_;
    bool tutorialMenuVisible_ = true;
    bool tutorialContentInitialized_ = false;
    int tutorialMenuPage_ = 0;
    TutorialManager::TutorialStep lastTutorialStep_ = TutorialManager::TutorialStep::Intro;
    float cardExplainInputBlockTimer_ = 0.0f;

    std::unique_ptr<TextSprite> playerHpText_;
    std::array<std::unique_ptr<TextSprite>, 8> playerHpOutlineTexts_;
    std::vector<std::unique_ptr<TextSprite>> enemyHpTexts_;

    std::unique_ptr<Sprite> powerBoostBg_;
    std::unique_ptr<TextSprite> powerBoostText_;

    std::unique_ptr<Sprite> blockBg_;
    std::unique_ptr<TextSprite> blockText_;
    std::array<std::unique_ptr<TextSprite>, 8> blockOutlineTexts_;
    std::unique_ptr<Sprite> highlightFilter_;

    std::unique_ptr<Sprite> startFadeMask_;
    float startFadeTimer_ = 0.0f;
    float startFadeDuration_ = 0.75f;
    bool startFadeActive_ = false;
    std::unique_ptr<PokerHandHelpView> pokerHandHelpView_;

    bool prevEsc_ = false;

    State state_ = State::EnterOpen;
    float circle_ = 0.0f;
    float softness_ = 0.6f;
    const char* nextSceneName_ = "Title";
};
