#pragma once
#include "IScene.h"
#include <memory>
#include "Camera.h"
#include "Object3d.h"
#include "Enemy.h"
#include "Player.h"

class GameApp;

class GameScene : public IScene {
public:
    GameScene() = default;
    ~GameScene() = default;

    void OnEnter(GameApp& app) override;
    void OnExit(GameApp& app) override;
    void Update(GameApp& app, float dt) override;
    void Draw(GameApp& app) override;

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Object3d> skyDome_; // 背景の天球

    std::unique_ptr<Player> player_;
    EnemyManager enemyMgr_;

    // ライトの設定
    LightingParam light_;

    // ESCキーの入力状態を保持（タイトルに戻るなどの処理用）
    bool prevEsc_ = false;
};