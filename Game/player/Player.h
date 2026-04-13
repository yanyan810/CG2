#pragma once
#include <memory>
#include "Vector3.h"
#include "AABB.h"
#include "Object3d.h"
#include "ModelParticleManager.h"
#include "AnimationEditorSession.h"
#include "TrailManager.h"

class Object3dCommon;
class DirectXCommon;
class Camera;

class Player {
public:
	Player() = default;
	~Player() = default;

	// 初期化
	void Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam);

	// 毎フレームの更新（WASD入力などは受け取らず、アニメーションだけ進める）
	void Update(float dt);

	// 描画
	void Draw();

	// 座標と向きの設定
	void SetSpawnPos(const Vector3& p);
	void SetRotation(const Vector3& r);

	// ===============================================
	// エネミー（弾など）から参照される互換性用の関数群
	// ===============================================
	Vector3 GetWorldPos() const { return pos_; }
	AABB GetBodyAABB() const { return body_; }
	void Damage(int damage); 
	void TriggerHitFlash(float sec) { flashTimer_ = sec; }

	// 動きのトリガー関数
	void PlayAttackAnim(const Vector3& targetPos);
	void PlayDamageAnim();
	int GetHP() const { return hp_; }

	int GetBlock() { return block_; }
	void AddBlock(int value) { block_ += value; }
	void ResetBlock() { block_ = 0; }

	int GetBoostedPower() { return boostedPower_; }
	void PowerBoost(int value) { boostedPower_ += value; }
	void ResetPowerBoost() { boostedPower_ = 0; }

	void Heal(int value) {
		hp_ += value;
		if (hp_ > maxHp_) {
			hp_ = maxHp_;
		}
	}
	int GetMaxHP() const { return maxHp_; }
	const Vector3& GetPos() const { return pos_; }


	int GetVampireHeal() const { return vampireHeal_; }
	void AddVampireHeal(int value) { vampireHeal_ += value; }
	void ResetVampireHeal() { vampireHeal_ = 0; }

	bool GetIsAlive() { return isAlive_; }

	void SetCamera(Camera* camera) {
		if (model_) {
			model_->SetCamera(camera);
		}
	}

	// アニメーションエディタの描画
	Object3d* GetObject3d() const { return model_.get(); }
	void DrawAnimationEditorImGui(Camera* editorCamera);

	// 軌跡のための座標取得
	Vector3 GetWeaponTipPos();
	Vector3 GetWeaponBasePos();

	// TrailManagerから自分用のインスタンスを受け取るための関数
	void SetTrailInstance(TrailInstance* instance) {
		trailInstance_ = instance;
	}

	// 軌跡の色の設定などを外から変えられるように
	void SetTrailConfig(const TrailConfig& config) {
		trailConfig_ = config;
	}

private:
	std::unique_ptr<Object3d> model_;

	Vector3 pos_{ 0.0f, 0.0f, 0.0f };
	Vector3 rot_{ 0.0f, 0.0f, 0.0f };

	int hp_ = 100;
	int maxHp_ = 100;

	int block_ = 0;

	int boostedPower_ = 0;

	// 攻撃時に回復する量
	int vampireHeal_ = 0;

	AABB body_{};
	enum class AnimState { Idle, AttackForward, AttackReturn, Damage };
	AnimState animState_ = AnimState::Idle;

	float animTimer_ = 0.0f;
	float animDuration_ = 0.0f;
	Vector3 basePos_{};   // 本来の立ち位置
	Vector3 startPos_{};  // アニメーション開始位置
	Vector3 targetPos_{}; // アニメーション目標位置

	float flashTimer_ = 0.0f;

	bool isAlive_ = true;

	ParticleEmitterConfig attackEffectConfig_;

	bool isRecordingTrail_ = false;

	// 修正: Managerそのものではなく、生成されたインスタンスを保持
	TrailInstance* trailInstance_ = nullptr;
	TrailConfig trailConfig_;

	// 武器の計算用（モデルに合わせて微調整してください）
	const float kWeaponLength = 100.0f;
	const Vector3 kWeaponOffset = { 0.0f, 1.2f, 0.0f }; // モデルの手に合わせる
};