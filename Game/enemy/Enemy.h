#pragma once
#include <memory>
#include <vector>
#include <cstdint>

#include "Vector3.h"
#include "AABB.h"
#include "Object3d.h"
#include "LightingParam.h"
#include "BossAI.h"

class Object3dCommon;
class DirectXCommon;
class Camera;
class Player;

enum class EnemyType : uint8_t {
    Boss
};

class Enemy {
public:
    void Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam,
        EnemyType type, const Vector3& spawnXYZ);

    void Update(float dt); // プレイヤー座標などを渡さないシンプルな形に
    void Draw();

    bool IsAlive() const { return alive_; }
    void Damage(int damage) { hp_ -= damage; if (hp_ < 0) hp_ = 0; }
    int GetHP() const { return hp_; }
    int GetMaxHP() const { return ai_.GetMaxHP(); }
    Vector3 GetPos() const { return pos_; }
    AABB GetBodyAABB() const { return body_; }

    void SetLighting(const LightingParam& p);

    void TriggerHitFlash(float sec) { flashTimer_ = sec; }
    void PlayAttackAnim(const Vector3& targetPos);
    void PlayDamageAnim();

private:
    Object3dCommon* objCommon_ = nullptr;
    DirectXCommon* dx_ = nullptr;
    Camera* cam_ = nullptr;

    EnemyType type_ = EnemyType::Boss;
    bool alive_ = true;
    int hp_ = 300;

    Vector3 pos_{ 0,0,0 };
    Vector3 rot_{ 0,0,0 };

    AABB body_{};
    std::unique_ptr<Object3d> model_;
    LightingParam light_;

    BossAI ai_; // シンプルになったAI

    enum class AnimState { Idle, AttackForward, AttackReturn, Damage };
    AnimState animState_ = AnimState::Idle;

    float animTimer_ = 0.0f;
    float animDuration_ = 0.0f;
    Vector3 basePos_{};
    Vector3 startPos_{};
    Vector3 targetPos_{};

    float flashTimer_ = 0.0f;
};

// ==========================================
// マネージャー
// ==========================================
class EnemyManager {
public:
    void Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam);

    void Spawn(EnemyType type, const Vector3& pos);

    void Update(float dt);
    void Draw();

    void SetLighting(const LightingParam& p);

    // ボスの情報を取得（UIやバトルコントローラー用）
    Enemy* GetBoss();

private:
    Object3dCommon* objCommon_ = nullptr;
    DirectXCommon* dx_ = nullptr;
    Camera* cam_ = nullptr;

    std::vector<Enemy> enemies_;
};