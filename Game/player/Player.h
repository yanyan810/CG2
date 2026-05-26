#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "Vector3.h"
#include "AABB.h"
#include "Object3d.h"
#include "ModelParticleManager.h"
#include "AnimationEditorSession.h"
#include "TrailManager.h"
#include "EffectSequencer.h"

// 攻撃技の定義構造体
struct AttackMove {
	std::string animationName;  // 再生するアニメーション名
	std::string effectJSON;     // 読み込むエフェクトのJSONファイル名
	float fireDelay = 0.0f;     // アニメーション開始からエフェクト発射までの遅延時間（秒）
};

class Object3dCommon;
class DirectXCommon;
class Camera;
class GameApp;
class Model;

class Player {
public:
	Player() = default;
	~Player() = default;

	// 初期化
	void Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam,
		ModelParticleManager* particleMgr = nullptr, TrailManager* trailMgr = nullptr);

	// 毎フレームの更新（WASD入力などは受け取らず、アニメーションだけ進める）
	void Update(float dt);

	// 描画
	void Draw();
	void DrawPostEffect(GameApp& app);
	void DrawShieldBloom(GameApp& app);
	void DrawShieldImGui();

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
	// 攻撃技インデックスを指定して攻撃（attackList_から選択）
	void PlayAttackAnimWithEffect(const Vector3& targetPos, int moveIndex = 0);
	void PlayDamageAnim();
	int GetHP() const { return hp_; }

	int GetBlock() { return block_; }
	void AddBlock(int value);
	void DecayBlock(float decayRate);
	void ResetBlock();

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
	const Vector3& GetRotation() const { return rot_; }


	int GetVampireHeal() const { return vampireHeal_; }
	void AddVampireHeal(int value) { vampireHeal_ += value; }
	void ResetVampireHeal() { vampireHeal_ = 0; }

	bool GetIsAlive() { return isAlive_; }

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

	// アニメーションエディタの描画
	Object3d* GetObject3d() const { return model_.get(); }

	// 攻撃技リストへのアクセス
	std::vector<AttackMove>& GetAttackList() { return attackList_; }
	const std::vector<AttackMove>& GetAttackList() const { return attackList_; }
	void AddAttackMove(const AttackMove& move);

	// EffectSequencerへのアクセス
	EffectSequencer& GetEffectSequencer() { return effectSequencer_; }
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

	void SetPoisonDrawActive(bool active) { poisonDrawActive_ = active; }
	bool GetPoisonDrawActive() const { return poisonDrawActive_; }
	void SetReleaseAnimationEnabled(bool enabled) { releaseAnimationEnabled_ = enabled; }
	bool GetReleaseAnimationEnabled() const { return releaseAnimationEnabled_; }

	void SetFrostBiteActive(bool active) { frostBiteActive_ = active; }
	bool GetFrostBiteActive() const { return frostBiteActive_; }

private:
	void InitializeShieldEffect_();
	void UpdateShieldEffect_(float dt);
	void DrawShield_(const Vector4& color, float scaleMultiplier);
	bool ShouldDrawShield_() const;
	void EnsureShieldCellCount_();
	int GetTargetShieldCellCount_() const;
	void TriggerShieldBreak_(int cellCount);
	void UpdatePowerBoostEffect_(float dt);
	void EmitPowerBoostParticles_(uint32_t count);

	void PlayReleaseIdleAnimation_();
	void PlayRandomReleaseAttackAnimation_();
	void PlayReleaseDamageAnimation_();

	std::unique_ptr<Object3d> model_;

	Vector3 pos_{ 0.0f, 0.0f, 0.0f };
	Vector3 rot_{ 0.0f, 0.0f, 0.0f };

	int hp_ = 100;
	int maxHp_ = 100;

	int block_ = 0;

	int boostedPower_ = 0;

	// 攻撃時に回復する量
	int vampireHeal_ = 0;

	bool poisonDrawActive_ = false;
	bool frostBiteActive_ = false;

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

	bool releaseAttackAnimationPlaying_ = false;
#ifdef _DEBUG
	bool releaseAnimationEnabled_ = false;
#else
	bool releaseAnimationEnabled_ = true;
#endif

	bool isRecordingTrail_ = false;

	// 修正: Managerそのものではなく、生成されたインスタンスを保持
	TrailInstance* trailInstance_ = nullptr;
	TrailConfig trailConfig_;

	// 武器の計算用（モデルに合わせて微調整してください）
	const float kWeaponLength = 2.0f;
	const Vector3 kWeaponOffset = { 0.0f, 1.2f, 0.0f }; // モデルの手に合わせる

	ModelParticleManager* particleManager_ = nullptr;

	bool powerBoostEffectEnabled_ = true;
	float powerBoostEffectTimer_ = 0.0f;
	float powerBoostEmitAccumulator_ = 0.0f;
	float powerBoostBaseRate_ = 120.0f;
	float powerBoostRatePerPower_ = 4.0f;
	float powerBoostRadius_ = 0.75f;
	float powerBoostRadiusPerPower_ = 0.035f;
	float powerBoostHeight_ = 1.55f;
	float powerBoostSwirlSpeed_ = 3.2f;
	float powerBoostSwirlSpeedPerPower_ = 0.22f;
	float powerBoostVortexAngularSpeed_ = 5.2f;
	float powerBoostVortexAngularPerPower_ = 0.28f;
	float powerBoostVortexRadialSpeed_ = -0.08f;
	float powerBoostVortexRadialPerPower_ = -0.006f;
	float powerBoostUpSpeed_ = 1.4f;
	float powerBoostUpSpeedPerPower_ = 0.08f;
	float powerBoostStartScale_ = 0.2f;
	float powerBoostScalePerPower_ = 0.008f;
	float powerBoostEndScaleRate_ = 0.12f;
	float powerBoostLifeMin_ = 1.0f;
	float powerBoostLifeMax_ = 1.6f;
	float powerBoostColorIntensity_ = 1.0f;
	bool powerBoostBillboard_ = false;
	Vector4 powerBoostStartColor_{ 0.42f, 1.0f, 0.50f, 1.0f };
	Vector4 powerBoostEndColor_{ 0.45f, 1.0f, 0.70f, 0.0f };

	Model* shieldHexModel_ = nullptr;
	std::vector<std::unique_ptr<Object3d>> shieldCells_;
	float shieldTimer_ = 0.0f;
	float shieldVisibleTimer_ = 0.0f;
	bool shieldPreviewAlways_ = false;
	int shieldCellCount_ = 13;
	float shieldDisplayCount_ = 0.0f;
	float shieldBuildSpeed_ = 6.0f;
	float shieldReduceSpeed_ = 20.0f;
	Vector3 shieldOffset_{ 2.05f, 1.35f, 0.10f };
	Vector3 shieldRotation_{ 0.0f, 2.0f, 0.0f };
	float shieldBaseScale_ = 0.46f;
	float shieldSpacingX_ = 1.0f;
	float shieldSpacingY_ = 0.8f;
	float shieldTiltY_ = 0.0f;
	float shieldPulseSpeed_ = 5.5f;
	float shieldPulseScale_ = 0.05f;
	Vector4 shieldColor_{ 0.28f, 0.82f, 1.0f, 0.74f };
	Vector4 shieldBloomColor_{ 0.17f, 0.85f, 1.0f, 1.0f };
	float shieldBloomScale_ = 1.0f;
	float shieldBloomIntensity_ = 2.5f;
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

	// === 攻撃エフェクト連動 ===
	std::vector<AttackMove> attackList_;       // 使用可能な技のリスト
	std::unordered_map<std::string, EffectProfile> effectProfileCache_;
	EffectSequencer effectSequencer_;           // プレイヤー専用のシーケンサー
	float attackEffectTimer_ = 0.0f;            // 現在の攻撃アニメーション経過時間
	bool effectFired_ = false;                  // エフェクト二重発射防止フラグ
	int currentAttackIndex_ = -1;              // 現在実行中の攻撃技インデックス
	Vector3 attackTargetPos_{};                 // 攻撃対象の座標

	// EffectJSONキャッシュ（同じJSONは再読み込みしない）
	std::string cachedEffectJSONPath_;          // 前回ロードしたeffectJSONのパス

	// EffectSequencer初期化用の参照保持
	Object3dCommon* objCommon_ = nullptr;
	DirectXCommon* dx_ = nullptr;
	Camera* camera_ = nullptr;
	TrailManager* trailMgr_ = nullptr;
};
