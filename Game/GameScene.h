#pragma once
#include "IScene.h"
#include <memory>

#include "Particle.h"
#include "Camera.h"
#include "Sprite.h"
#include "Object3d.h"
#include "Input.h"

#include "PlayerCombo.h"

#include "Enemy.h"
#include "Player.h"

class GameScene : public IScene {
public:
    GameScene() = default;
    ~GameScene(); // ★追加：ここが重要


    void OnEnter(GameApp& app) override;
    void OnExit(GameApp& app) override;

    void Update(GameApp& app, float dt) override;
    void Draw(GameApp& app) override;

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Sprite> sprite_;
    std::unique_ptr<Object3d> objA_;
    std::unique_ptr<Object3d> objB_;
    std::unique_ptr<Object3d> debugHitboxObj_;
    std::unique_ptr<Particle> particle_;
    std::unique_ptr<Particle> debugTitleParticle_;

    //player
    std::unique_ptr<Player> player_;

	Input* input_ = nullptr;

    //enemy 
    EnemyManager enemyMgr_;


};
