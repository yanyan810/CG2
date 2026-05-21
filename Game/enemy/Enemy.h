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
class GameApp;
class Model;

enum class EnemyType : uint8_t {
	Boss,
	Slime,
	Goblin,
	Golem,
	Needle
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
	void DrawShieldBloom(GameApp& app);

	bool IsAlive() const { return alive_; }
	int Damage(int damage);
	int GetHP() const { return hp_; }
	int GetMaxHP() const { return maxHp_; }
	int GetBlock() const { return block_; }
	void AddBlock(int value);
	void ResetBlock();
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
		for (auto& cell : shieldCells_) {
			if (cell) {
				cell->SetCamera(camera);
			}
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


	/// 状態異常
	enum class BadCondition {
		kNone,
		kPoison,
		kFrost,
	};

	void SetBC(BadCondition condition); // 状態異常切り替え
	BadCondition GetBC() const { return badCondition_; } // 現在の状態異常を取得
	int GetBCPoint() const { return badConditionPoint_; } // 状態異常のポイントを取得
	void AddBC(int value); // 加算
	void TurnEndApplyBC(); // ターン終了時に状態異常の効果を適用
	void AmplifyBC(int value); //倍加
	void DamageBC(int count); // 状態異常ポイントに応じたダメージを与える
	void RemoveBC(); // 状態異常の解除

public:

	//チュートリアル用
	void SetMaxHp(int maxHp, bool healToFull = true);
	void SetHp(int hp);

private:
	void InitializeShieldEffect_();
	void EnsureShieldCellCount_();
	void UpdateShieldEffect_(float dt);
	void DrawShield_(const Vector4& color, float scaleMultiplier);
	void TriggerShieldBreak_(int cellCount);
	bool ShouldDrawShield_() const;
	int GetTargetShieldCellCount_() const;

	int maxHp_ = 150;

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

	int badConditionPoint_ = 0;
	BadCondition badCondition_ = BadCondition::kNone;

	bool isAbleToAct_ = true;

	int freezeResistance_ = 5;

	Model* shieldHexModel_ = nullptr;
	std::vector<std::unique_ptr<Object3d>> shieldCells_;
	float shieldTimer_ = 0.0f;
	float shieldVisibleTimer_ = 0.0f;
	float shieldDisplayCount_ = 0.0f;
	float shieldBuildSpeed_ = 6.0f;
	float shieldReduceSpeed_ = 20.0f;
	int shieldCellCount_ = 13;
	Vector3 shieldOffset_{ -2.05f, 1.35f, 0.10f };
	Vector3 shieldRotation_{ 0.0f, -2.0f, 0.0f };
	float shieldBaseScale_ = 0.46f;
	float shieldSpacingX_ = 1.0f;
	float shieldSpacingY_ = 0.8f;
	float shieldTiltY_ = 0.0f;
	float shieldPulseSpeed_ = 5.5f;
	float shieldPulseScale_ = 0.05f;
	Vector4 shieldColor_{ 1.0f, 0.72f, 0.22f, 0.72f };
	Vector4 shieldBloomColor_{ 1.0f, 0.45f, 0.15f, 1.0f };
	float shieldBloomScale_ = 1.0f;
	float shieldBloomIntensity_ = 2.2f;
	float shieldBloomChromAb_ = 0.0f;
	bool shieldBreakActive_ = false;
	float shieldBreakTimer_ = 0.0f;
	float shieldBreakDuration_ = 0.85f;
	float shieldBreakGravity_ = 7.5f;
	int shieldBreakCellCount_ = 0;
	std::vector<Vector3> shieldBreakBasePositions_;
	std::vector<Vector3> shieldBreakVelocities_;
	std::vector<Vector3> shieldBreakRotations_;
	std::vector<Vector3> shieldBreakAngularVelocities_;

	// 毒の値のセット（負数防止）を共通化
	void SetBC(int value) {
		badConditionPoint_ = (value < 0) ? 0 : value;
	}
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
	void DrawShieldBloom(GameApp& app);

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
