#pragma once
#include <memory>
#include <vector>
#include <cstdint>

#include "Vector3.h" // ← Vector2 / Vector3 がここにある
#include "AABB.h"    // ← AABB{ Vector3 min,max }

class Object3d;
class Object3dCommon;
class DirectXCommon;
class Camera;

enum class EnemyType : uint8_t {
    Melee,
    Shooter,
    Boss
};

struct EnemyHitResult {
    bool hit = false;
    bool killed = false;
};

class Enemy {
public:
    void Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam,
        EnemyType type, const Vector3& spawnXYZ);

    void Update(float dt, const Vector2& playerXY, float playerZ);
    void Draw();

    bool IsAlive() const { return alive_; }
    bool IsBoss()  const { return type_ == EnemyType::Boss; }

    Vector2 GetPos2D() const { return { pos_.x, pos_.y }; }
    AABB    GetBodyAABB() const { return body_; }

    // プレイヤー攻撃ヒット時
    EnemyHitResult ApplyHit2D(float knockVx, float launchVy, bool requestHitstun);

private:
    void UpdateAI_Melee_(float dt, const Vector2& playerXY, float playerZ);
    void UpdateAI_Shooter_(float dt, const Vector2& playerXY, float playerZ);
    void UpdateAI_Boss_(float dt, const Vector2& playerXY, float playerZ);


    void ApplyPhysics_(float dt);
    void UpdateBody_();
    void UpdateModel_();

private:
    EnemyType type_ = EnemyType::Melee;
    bool alive_ = true;

    // 位置/速度（内部は3Dで持つが、Zは見た目だけ）
    Vector3 pos_{ 0,0,15 };
    Vector3 vel_{ 0,0,0 };

    float hitRadiusZ_ = 0.5f; // ★Z方向の当たり半径

    bool onGround_ = true;
    bool airborne_ = false;

    // ひるみ
    bool hitstun_ = false;
    float hitstunTime_ = 0.0f;

    int hp_ = 3;

    // 見た目
    std::unique_ptr<Object3d> model_;
    float zView_ = 15.0f;

    // 当たり判定（3D AABB だが、判定はXYだけ使う想定）
    AABB body_{};

    // AIパラメータ
    int facing_ = -1;
    float moveSpeed_ = 2.6f;
    float gravity_ = 25.0f;
    float depthSpeed_ = 8.0f;
    float zFollowDeadZone_ = 0.25f;

    float meleeRange_ = 1.3f;
    float meleeCooldown_ = 0.8f;
    float meleeTimer_ = 0.0f;

    float shootCooldown_ = 1.2f;
    float shootTimer_ = 0.0f;
};

// 管理
class EnemyManager {
public:
    void Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam);

    void Clear();
    void Spawn(EnemyType type, const Vector3& posXY);

    void Update(float dt, const Vector2& playerXY, float playerZ);

    void Draw();

    std::vector<Enemy>& GetEnemies() { return enemies_; }
    const std::vector<Enemy>& GetEnemies() const { return enemies_; }

private:
    Object3dCommon* objCommon_ = nullptr;
    DirectXCommon* dx_ = nullptr;
    Camera* cam_ = nullptr;

    std::vector<Enemy> enemies_;
};
