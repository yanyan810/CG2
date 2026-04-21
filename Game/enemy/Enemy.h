#pragma once
#include <memory>
#include <vector>
#include <cstdint>

#include"Camera.h"
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
    Boss,
    Slime
};

class Enemy {
public:
    void Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam,
        EnemyType type, const Vector3& spawnXYZ);

    void Update(float dt); // プレイヤー座標などを渡さないシンプルな形に
    void Draw();

    bool IsAlive() const { return alive_; }
    void Damage(int damage) {
        hp_ -= damage;
        if (hp_ < 0) {
            hp_ = 0;
            alive_ = false;
        }
    }
    int GetHP() const { return hp_; }
    int GetMaxHP() const { return maxHp_; }
    void SetHighlight(bool enable) { isHighlighted_ = enable; }
    void Heal(int value) {
        hp_ += value;
        if (hp_ > GetMaxHP()) {
            hp_ = GetMaxHP();
        }
    }

    void SetCamera(Camera* camera) {
        if (model_) {
            model_->SetCamera(camera);
        }
    }

    Vector3 GetPos() const { return pos_; }
    AABB GetBodyAABB() const { return body_; }
    BossAI& GetBossAI() { return ai_; }
    void SetLighting(const LightingParam& p);

    void TriggerHitFlash(float sec) { flashTimer_ = sec; }
    void PlayAttackAnim(const Vector3& targetPos);
    void PlayDamageAnim();

    int GetIncomingDamage() const {
        if (!alive_) return 0;
        // AIの現在の行動を取得
        auto action = ai_.GetNextAction();
        if (action.type == "Attack") {
            return action.value;
        }
        return 0;
    }

public:

    //チュートリアル用
    void SetMaxHp(int maxHp, bool healToFull = true);
    void SetHp(int hp);
   // int GetTutorialMaxHp() const { return maxHp_; }

private:

    int maxHp_=150;

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
    bool isHighlighted_ = false;
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

    std::vector<Enemy>& GetEnemies() { return enemies_; }
    int PickEnemyByMouse(int mouseX, int mouseY, const Matrix4x4& viewProj, float screenWidth, float screenHeight);

    void UpdateCamera(Camera* camera) {
        camera_ = camera;
        for (auto& enemy : enemies_) {
            enemy.SetCamera(camera);
        }
    }

public:

    //チュートリアル用
    Enemy* GetEnemy(size_t index);
    const Enemy* GetEnemy(size_t index) const;

private:
    Camera* camera_ = nullptr;
    Object3dCommon* objCommon_ = nullptr;
    DirectXCommon* dx_ = nullptr;
    Camera* cam_ = nullptr;

    std::vector<Enemy> enemies_;
};