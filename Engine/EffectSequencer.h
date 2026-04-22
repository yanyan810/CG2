#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include "MathStruct.h"

// 前方宣言
class Object3d;
class Object3dCommon;
class DirectXCommon;
class Camera;
class ModelParticleManager;
class TrailManager;
class TrailInstance;
struct TrailConfig;

// =============================================
// エフェクトプロファイル（JSON構造）
// =============================================

struct ProjectileProfile {
	std::string modelPath = "sphere/sphere.obj";
	Vector3 scale = { 0.3f, 0.3f, 0.3f };
	Vector3 rotationSpeed = { 0.0f, 5.0f, 0.0f };

	nlohmann::json ToJson() const {
		return nlohmann::json{
			{"modelPath", modelPath},
			{"scale", {scale.x, scale.y, scale.z}},
			{"rotationSpeed", {rotationSpeed.x, rotationSpeed.y, rotationSpeed.z}}
		};
	}

	void FromJson(const nlohmann::json& j) {
		modelPath = j.value("modelPath", modelPath);
		if (j.contains("scale")) {
			scale = { j["scale"][0], j["scale"][1], j["scale"][2] };
		}
		if (j.contains("rotationSpeed")) {
			rotationSpeed = { j["rotationSpeed"][0], j["rotationSpeed"][1], j["rotationSpeed"][2] };
		}
	}
};

struct TrailProfile {
	Vector4 startColor = { 0.5f, 0.0f, 1.0f, 1.0f };
	Vector4 endColor = { 0.5f, 0.0f, 1.0f, 0.0f };
	uint32_t maxPoints = 100;
	uint32_t interpolationSteps = 6;
	Vector3 tipOffset = { 0.0f, 0.3f, 0.0f };
	Vector3 baseOffset = { 0.0f, -0.3f, 0.0f };
	float lifetime = 0.5f; // 各頂点の生存時間

	nlohmann::json ToJson() const {
		return nlohmann::json{
			{"startColor", {startColor.x, startColor.y, startColor.z, startColor.w}},
			{"endColor", {endColor.x, endColor.y, endColor.z, endColor.w}},
			{"maxPoints", maxPoints},
			{"interpolationSteps", interpolationSteps},
			{"tipOffset", {tipOffset.x, tipOffset.y, tipOffset.z}},
			{"baseOffset", {baseOffset.x, baseOffset.y, baseOffset.z}},
			{"lifetime", lifetime}
		};
	}

	void FromJson(const nlohmann::json& j) {
		if (j.contains("startColor")) {
			startColor = { j["startColor"][0], j["startColor"][1], j["startColor"][2], j["startColor"][3] };
		}
		if (j.contains("endColor")) {
			endColor = { j["endColor"][0], j["endColor"][1], j["endColor"][2], j["endColor"][3] };
		}
		maxPoints = j.value("maxPoints", maxPoints);
		interpolationSteps = j.value("interpolationSteps", interpolationSteps);
		if (j.contains("tipOffset")) {
			tipOffset = { j["tipOffset"][0], j["tipOffset"][1], j["tipOffset"][2] };
		}
		if (j.contains("baseOffset")) {
			baseOffset = { j["baseOffset"][0], j["baseOffset"][1], j["baseOffset"][2] };
		}
		lifetime = j.value("lifetime", lifetime);
	}
};

struct EffectProfile {
	ProjectileProfile projectile;
	std::string flyParticle = "shadow_ball_fly";  // ModelParticleManagerに登録済みのエフェクト名
	std::string hitParticle = "shadow_ball_hit";  // 着弾時エフェクト名
	TrailProfile trail;
	float duration = 1.0f;      // 飛翔時間（秒）
	float hitDuration = 0.5f;   // ヒットエフェクト表示時間
	uint32_t flyParticleCount = 5;   // 毎フレーム発生するパーティクル数
	uint32_t hitParticleCount = 50;  // 着弾時に発生するパーティクル数
	bool enableTrail = true;         // 軌跡ON/OFF

	nlohmann::json ToJson() const {
		return nlohmann::json{
			{"projectile", projectile.ToJson()},
			{"flyParticle", flyParticle},
			{"hitParticle", hitParticle},
			{"trail", trail.ToJson()},
			{"duration", duration},
			{"hitDuration", hitDuration},
			{"flyParticleCount", flyParticleCount},
			{"hitParticleCount", hitParticleCount},
			{"enableTrail", enableTrail}
		};
	}

	void FromJson(const nlohmann::json& j) {
		if (j.contains("projectile")) { projectile.FromJson(j["projectile"]); }
		flyParticle = j.value("flyParticle", flyParticle);
		hitParticle = j.value("hitParticle", hitParticle);
		if (j.contains("trail")) { trail.FromJson(j["trail"]); }
		duration = j.value("duration", duration);
		hitDuration = j.value("hitDuration", hitDuration);
		flyParticleCount = j.value("flyParticleCount", flyParticleCount);
		hitParticleCount = j.value("hitParticleCount", hitParticleCount);
		enableTrail = j.value("enableTrail", enableTrail);
	}
};

// =============================================
// エフェクトシーケンサークラス
// =============================================

class EffectSequencer {
public:
	// ステート
	enum class State {
		Idle,       // 待機
		Firing,     // 発射初期化（1フレーム）
		Flying,     // 飛翔中
		Hit,        // 着弾エフェクト中
		Finished    // 完了
	};

	// 初期化：各マネージャーへの参照を渡す
	void Initialize(
		Object3dCommon* objCommon,
		DirectXCommon* dx,
		Camera* camera,
		ModelParticleManager* particleMgr,
		TrailManager* trailMgr
	);

	// 発射！ startPos から targetPos へ profile に従って飛ぶ
	void Fire(const EffectProfile& profile, const Vector3& startPos, const Vector3& targetPos);

	// 毎フレーム更新
	void Update(float dt);

	// 弾の描画（3Dオブジェクト描画タイミングで呼ぶ）
	void Draw();

	// 状態確認
	bool IsFinished() const { return state_ == State::Finished || state_ == State::Idle; }
	bool IsActive() const { return state_ != State::Idle && state_ != State::Finished; }
	State GetState() const { return state_; }

	// リセット（Idle に戻す）
	void Reset();

	// 着弾時コールバック
	void SetOnHitCallback(std::function<void()> callback) { onHitCallback_ = callback; }

	// ------ エディターUI ------
#ifdef USE_IMGUI
	void DrawImGuiEditor(const Vector3& defaultStartPos, const Vector3& defaultTargetPos);
#endif

	// ------ JSON読み書き ------
	static void SaveProfile(const std::string& path, const EffectProfile& profile);
	static bool LoadProfile(const std::string& path, EffectProfile& profile);

	// 現在のプロファイルの取得/設定
	const EffectProfile& GetProfile() const { return profile_; }
	void SetProfile(const EffectProfile& profile) { profile_ = profile; }

private:
	// ステート別処理
	void UpdateFiring(float dt);
	void UpdateFlying(float dt);
	void UpdateHit(float dt);

	// ヘルパー：Lerp補間
	static Vector3 LerpVec3(const Vector3& a, const Vector3& b, float t);

private:
	State state_ = State::Idle;

	// 参照（所有しない）
	Object3dCommon* objCommon_ = nullptr;
	DirectXCommon* dx_ = nullptr;
	Camera* camera_ = nullptr;
	ModelParticleManager* particleMgr_ = nullptr;
	TrailManager* trailMgr_ = nullptr;

	// 現在のプロファイル
	EffectProfile profile_;

	// 飛翔パラメータ
	Vector3 startPos_ = {};
	Vector3 targetPos_ = {};
	Vector3 currentPos_ = {};
	float elapsedTime_ = 0.0f;

	// 弾オブジェクト
	std::unique_ptr<Object3d> projectile_;
	Vector3 projectileRotation_ = {};

	// 軌跡
	TrailInstance* trail_ = nullptr;

	// コールバック
	std::function<void()> onHitCallback_;

	// エディター用
	EffectProfile editingProfile_;
	char profileFilename_[128] = "effect_default.json";
};
