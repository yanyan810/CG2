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

    void SpawnEnemyFromOutside_(EnemyType type);

    void  UpdateHPDigits_(int hp);

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

    std::unique_ptr<Sprite> hpBack_;
    std::unique_ptr<Sprite> hpFill_;
    int maxHP_ = 100; // Playerの最大HPに合わせる

	Input* input_ = nullptr;

    //enemy 
    EnemyManager enemyMgr_;

    // 数字テクスチャ（0..9）
    std::string numTex_[10];

    // HP数字表示（3桁）
    std::unique_ptr<Sprite> hpDigits_[3];
    int hpDigitsCount_ = 3;

    // 表示位置など
    Vector2 hpBarPos_{ 30.0f, 30.0f };
    Vector2 hpBarSize_{ 300.0f, 20.0f };

    Vector2 hpNumPos_{ 30.0f + 310.0f, 30.0f - 2.0f }; // ★バーの右側に表示（好みで）
    Vector2 hpNumSize_{ 16.0f, 20.0f };                // 1桁サイズ（PNGの見た目に合わせて調整）
    float   hpNumSpacing_ = 2.0f;

    std::unique_ptr<Object3d> ground_;

};
