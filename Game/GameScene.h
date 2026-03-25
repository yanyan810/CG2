#pragma once
#include "IScene.h"
#include <memory>
#include "Camera.h"
#include "CameraAnimator.h"
#include "Object3d.h"
#include "Enemy.h"
#include "Player.h"
#include "BattleController.h"
#include "TextSprite.h"
#include "StringUtility.h"
#include "FieldUi.h"


class GameApp;

class GameScene : public IScene {
public:
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
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Camera> animCamera_;
    std::unique_ptr<CameraAnimator> cameraAnim_;
    std::unique_ptr<Object3d> skyDome_; // 背景の天球

    std::unique_ptr<Player> player_;
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

    std::unique_ptr<Sprite> powerBoostBg_;
    std::unique_ptr<TextSprite> powerBoostText_;
    std::unique_ptr<Sprite> blockBg_;
    std::unique_ptr<TextSprite> blockText_;

};