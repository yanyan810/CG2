#pragma once
#include "IScene.h"
#include <memory>
#include "TextSprite.h"
#include "Sprite.h"
#include <vector>

#include "Camera.h"
#include "CameraAnimator.h"
#include "Object3d.h"
#include "Enemy.h"
#include "Player.h"
#include "BattleController.h"
#include "FieldUi.h"
#include "TutorialManager.h"
#include "TutorialUi.h"

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
    enum class State {
        EnterOpen,
        Idle,
        ExitClose
    };

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Camera> animCamera_;
    std::unique_ptr<CameraAnimator> cameraAnim_;
    std::unique_ptr<Object3d> skyDome_;

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

    std::unique_ptr<TutorialManager> tutorial_;
    std::unique_ptr<TutorialUi> tutorialUi_;

    std::unique_ptr<TextSprite> playerHpText_;
    std::vector<std::unique_ptr<TextSprite>> enemyHpTexts_;

    std::unique_ptr<Sprite> powerBoostBg_;
    std::unique_ptr<TextSprite> powerBoostText_;

    std::unique_ptr<Sprite> blockBg_;
    std::unique_ptr<TextSprite> blockText_;

    bool prevEsc_ = false;

    State state_ = State::EnterOpen;
    float circle_ = 0.0f;
    float softness_ = 0.6f;
    const char* nextSceneName_ = "Title";
};
