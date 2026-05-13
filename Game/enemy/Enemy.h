#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <string>

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

struct StageEnemyConfig {
    EnemyType type = EnemyType::Slime;
    Vector3 position{ 0.0f, 0.0f, 0.0f };
    int maxHp = -1;
    int hp = -1;
    std::string behaviorJson;
};

class Enemy {
public:
    void Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam,
        EnemyType type, const Vector3& spawnXYZ);
    void ApplyStageConfig(const StageEnemyConfig& config);

    void Update(float dt); // プレイヤー座標などを渡さないシンプルな形に
    void Draw();

    bool IsAlive() const { return alive_; }
    int Damage(int damage) {
        if (damage <= 0) {
            return 0;
        }

        if (block_ > 0) {
            const int blocked = block_ < damage ? block_ : damage;
            block_ -= blocked;
            damage -= blocked;
        }

        if (damage <= 0) {
            return 0;
        }

        const int beforeHp = hp_;
        hp_ -= damage;
        if (hp_ <= 0) {
            hp_ = 0;
            alive_ = false;
        }
        return beforeHp - hp_;
    }
    int GetHP() const { return hp_; }
    int GetMaxHP() const { return maxHp_; }
    int GetBlock() const { return block_; }
    void AddBlock(int value) {
        if (value <= 0) {
            return;
        }

        block_ += value;
        if (block_ < 0) {
            block_ = 0;
        }
    }
    void ResetBlock() { block_ = 0; }
    void SetHighlight(bool enable) { isHighlighted_ = enable; }
    bool IsHighlighted() const { return isHighlighted_; }
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
    void SetPosition(const Vector3& pos) {
        pos_ = pos;
        if (model_) {
            model_->SetTranslate(pos_);
        }
    }
    AABB GetBodyAABB() const { return body_; }
    BossAI& GetBossAI() { return ai_; }
    void SetLighting(const LightingParam& p);

    void TriggerHitFlash(float sec) { flashTimer_ = sec; }
    void PlayAttackAnim(const Vector3& targetPos);
    void PlayDamageAnim();

    Object3d* GetObject3d() const { return model_.get(); }

    int GetIncomingDamage() const {
        if (!alive_) return 0;
        // AIの現在の行動を取得
        auto action = ai_.GetNextAction();
        if (action.type == "Attack") {
            return action.value;
        }
        return 0;
    }

	/// ----- 毒の処理 ----- ///
    // 毒を付与する
    void AddPoison(int value) { poison_ += value; }
    // 毒の値を取得する
    int GetPoison() const { return poison_; }
    // ターン終了時などに毒のダメージを適用する（必要に応じて実装）
    void ApplyPoisonDamage() {
		hp_ -= poison_;
        poison_ -= 1;
        if (poison_ < 0) {
			poison_ = 0;
        }
        if (hp_ < 0) {
            hp_ = 0;
            alive_ = false;
        }
    }

    void PoisonDouble() {
        poison_ *= 2;
	}

    void PoisonDamage(int count) {
        hp_ -= poison_ * count;
        if (hp_ < 0) {
            hp_ = 0;
            alive_ = false;
        }
	}

    void PoisonRemove() {
        poison_ = 0;
	}

public:

    //チュートリアル用
    void SetMaxHp(int maxHp, bool healToFull = true);
    void SetHp(int hp);

private:

    int maxHp_=150;

private:
    Object3dCommon* objCommon_ = nullptr;
    DirectXCommon* dx_ = nullptr;
    Camera* cam_ = nullptr;

    EnemyType type_ = EnemyType::Boss;
    bool alive_ = true;
    int hp_ = 300;
    int block_ = 0;

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

    int poison_ = 0;
};

// ==========================================
// マネージャー
// ==========================================
class EnemyManager {
public:
    void Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam);

    void Spawn(EnemyType type, const Vector3& pos);
    void SpawnWithConfig(const StageEnemyConfig& config);

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
