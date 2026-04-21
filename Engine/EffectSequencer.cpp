#include "EffectSequencer.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "ModelParticleManager.h"
#include "TrailManager.h"
#include "TrailInstance.h"
#include "ModelManager.h"
#include <algorithm>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

// =============================================
// ヘルパー関数
// =============================================

Vector3 EffectSequencer::LerpVec3(const Vector3& a, const Vector3& b, float t) {
	return {
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t
	};
}

// =============================================
// 初期化
// =============================================

void EffectSequencer::Initialize(
	Object3dCommon* objCommon,
	DirectXCommon* dx,
	Camera* camera,
	ModelParticleManager* particleMgr,
	TrailManager* trailMgr)
{
	objCommon_ = objCommon;
	dx_ = dx;
	camera_ = camera;
	particleMgr_ = particleMgr;
	trailMgr_ = trailMgr;

	state_ = State::Idle;

	// エディター用の初期プロファイルをセット
	editingProfile_ = EffectProfile{};
}

// =============================================
// 発射
// =============================================

void EffectSequencer::Fire(const EffectProfile& profile, const Vector3& startPos, const Vector3& targetPos) {
	// 既に実行中なら先にリセット
	if (state_ != State::Idle && state_ != State::Finished) {
		Reset();
	}

	profile_ = profile;
	startPos_ = startPos;
	targetPos_ = targetPos;
	currentPos_ = startPos;
	elapsedTime_ = 0.0f;
	projectileRotation_ = { 0.0f, 0.0f, 0.0f };

	state_ = State::Firing;
}

// =============================================
// 毎フレーム更新
// =============================================

void EffectSequencer::Update(float dt) {
	switch (state_) {
	case State::Idle:
	case State::Finished:
		// 何もしない
		break;
	case State::Firing:
		UpdateFiring(dt);
		break;
	case State::Flying:
		UpdateFlying(dt);
		break;
	case State::Hit:
		UpdateHit(dt);
		break;
	}
}

// =============================================
// ステート別処理
// =============================================

void EffectSequencer::UpdateFiring(float dt) {
	// --- 弾オブジェクトの生成 ---
	projectile_ = std::make_unique<Object3d>();
	projectile_->Initialize(objCommon_, dx_);
	projectile_->SetModel(profile_.projectile.modelPath);
	projectile_->SetScale(profile_.projectile.scale);
	projectile_->SetTranslate(startPos_);
	projectile_->SetEnableLighting(0); // ライティングOFF（エフェクトなので）
	projectile_->SetCamera(camera_);

	// --- 軌跡の生成 ---
	if (profile_.enableTrail && trailMgr_) {
		trail_ = trailMgr_->CreateInstance();
		if (trail_) {
			trail_->SetIsPermanent(false);
			trail_->SetActive(true);

			TrailConfig trailConfig;
			trailConfig.startColor = profile_.trail.startColor;
			trailConfig.endColor = profile_.trail.endColor;
			trailConfig.maxPoints = profile_.trail.maxPoints;
			trailConfig.interpolationSteps = profile_.trail.interpolationSteps;
			trail_->SetConfig(trailConfig);
		}
	}

	// 即座にFlyingへ遷移
	state_ = State::Flying;

	// 1フレーム目のUpdateも実行
	UpdateFlying(dt);
}

void EffectSequencer::UpdateFlying(float dt) {
	elapsedTime_ += dt;

	// 正規化時間 t (0.0 ~ 1.0)
	float duration = (std::max)(profile_.duration, 0.001f);
	float t = elapsedTime_ / duration;
	t = (std::min)(t, 1.0f);

	// EaseInOut（SmoothStep）による滑らかな補間
	float eased = t * t * (3.0f - 2.0f * t);

	// 弾の位置を更新
	currentPos_ = LerpVec3(startPos_, targetPos_, eased);

	// 弾の回転
	projectileRotation_.x += profile_.projectile.rotationSpeed.x * dt;
	projectileRotation_.y += profile_.projectile.rotationSpeed.y * dt;
	projectileRotation_.z += profile_.projectile.rotationSpeed.z * dt;

	// Object3d の更新
	if (projectile_) {
		projectile_->SetTranslate(currentPos_);
		projectile_->SetRotate(projectileRotation_);
		projectile_->Update(dt);
	}

	// 飛翔パーティクルの発生
	if (particleMgr_ && !profile_.flyParticle.empty()) {
		particleMgr_->Emit(profile_.flyParticle, currentPos_, profile_.flyParticleCount);
	}

	// 軌跡の更新（オフセット付き）
	if (trail_ && trail_->IsActive()) {
		Vector3 tipPos = {
			currentPos_.x + profile_.trail.tipOffset.x,
			currentPos_.y + profile_.trail.tipOffset.y,
			currentPos_.z + profile_.trail.tipOffset.z
		};
		Vector3 basePos = {
			currentPos_.x + profile_.trail.baseOffset.x,
			currentPos_.y + profile_.trail.baseOffset.y,
			currentPos_.z + profile_.trail.baseOffset.z
		};

		TrailConfig trailConfig;
		trailConfig.startColor = profile_.trail.startColor;
		trailConfig.endColor = profile_.trail.endColor;
		trailConfig.maxPoints = profile_.trail.maxPoints;
		trailConfig.interpolationSteps = profile_.trail.interpolationSteps;
		trail_->Update(tipPos, basePos, trailConfig);
	}

	// ターゲット到達判定
	if (t >= 1.0f) {
		// → Hit ステートへ
		state_ = State::Hit;
		elapsedTime_ = 0.0f; // ヒットタイマーをリセット

		// 弾を非表示
		if (projectile_) {
			projectile_->SetScale({ 0.0f, 0.0f, 0.0f });
			projectile_->Update(0.0f);
		}

		// 軌跡を停止（消化モード）
		if (trail_) {
			trail_->SetActive(false);
		}

		// ヒットパーティクルの発生
		if (particleMgr_ && !profile_.hitParticle.empty()) {
			particleMgr_->Emit(profile_.hitParticle, targetPos_, profile_.hitParticleCount);
		}

		// コールバック呼び出し
		if (onHitCallback_) {
			onHitCallback_();
		}
	}
}

void EffectSequencer::UpdateHit(float dt) {
	elapsedTime_ += dt;

	// 軌跡の消化アニメーション
	if (trail_) {
		// TrailInstance は SetActive(false) 後に自動で消化される
		// 追加の消化は不要（TrailManager::Update が処理する）
	}

	// ヒット演出終了判定
	if (elapsedTime_ >= profile_.hitDuration) {
		state_ = State::Finished;

		// クリーンアップ
		projectile_.reset();
		trail_ = nullptr; // TrailManagerが所有しているので解放はしない
	}
}

// =============================================
// 描画
// =============================================

void EffectSequencer::Draw() {
	if (state_ == State::Flying && projectile_) {
		projectile_->Draw();
	}
}

// =============================================
// リセット
// =============================================

void EffectSequencer::Reset() {
	state_ = State::Idle;
	elapsedTime_ = 0.0f;
	projectile_.reset();

	if (trail_) {
		trail_->SetActive(false);
		trail_ = nullptr;
	}

	currentPos_ = {};
	projectileRotation_ = {};
}

// =============================================
// JSON 保存/読み込み
// =============================================

void EffectSequencer::SaveProfile(const std::string& path, const EffectProfile& profile) {
	std::ofstream file("resources/effect/" + path);
	if (file.is_open()) {
		nlohmann::json j = profile.ToJson();
		file << std::setw(4) << j << std::endl;
	}
}

bool EffectSequencer::LoadProfile(const std::string& path, EffectProfile& profile) {
	std::ifstream file("resources/effect/" + path);
	if (file.is_open()) {
		nlohmann::json j;
		file >> j;
		profile.FromJson(j);
		return true;
	}
	return false;
}

// =============================================
// ImGui エディター
// =============================================

#ifdef USE_IMGUI
void EffectSequencer::DrawImGuiEditor(const Vector3& defaultStartPos, const Vector3& defaultTargetPos) {
	ImGui::Begin("Attack Effect Editor");

	// --- テキストバッファ（staticで保持） ---
	static char modelBuf[256] = "";
	static char flyBuf[128] = "";
	static char hitBuf[128] = "";
	static bool needsSync = true;

	// 初回 or Load後にバッファを同期
	if (needsSync) {
		snprintf(modelBuf, sizeof(modelBuf), "%s", editingProfile_.projectile.modelPath.c_str());
		snprintf(flyBuf, sizeof(flyBuf), "%s", editingProfile_.flyParticle.c_str());
		snprintf(hitBuf, sizeof(hitBuf), "%s", editingProfile_.hitParticle.c_str());
		needsSync = false;
	}

	// --- ステート表示 ---
	const char* stateNames[] = { "Idle", "Firing", "Flying", "Hit", "Finished" };
	ImGui::Text("State: %s", stateNames[static_cast<int>(state_)]);
	ImGui::Separator();

	// --- プロファイル編集 ---
	if (ImGui::CollapsingHeader("Projectile", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::InputText("Model Path", modelBuf, sizeof(modelBuf))) {
			editingProfile_.projectile.modelPath = modelBuf;
		}
		ImGui::DragFloat3("Scale", &editingProfile_.projectile.scale.x, 0.01f, 0.01f, 10.0f);
		ImGui::DragFloat3("Rotation Speed", &editingProfile_.projectile.rotationSpeed.x, 0.1f);
	}

	if (ImGui::CollapsingHeader("Particles", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::InputText("Fly Particle", flyBuf, sizeof(flyBuf))) {
			editingProfile_.flyParticle = flyBuf;
		}

		int flyCount = static_cast<int>(editingProfile_.flyParticleCount);
		if (ImGui::SliderInt("Fly Count/Frame", &flyCount, 1, 100)) {
			editingProfile_.flyParticleCount = static_cast<uint32_t>(flyCount);
		}

		if (ImGui::InputText("Hit Particle", hitBuf, sizeof(hitBuf))) {
			editingProfile_.hitParticle = hitBuf;
		}

		int hitCount = static_cast<int>(editingProfile_.hitParticleCount);
		if (ImGui::SliderInt("Hit Count", &hitCount, 1, 1000)) {
			editingProfile_.hitParticleCount = static_cast<uint32_t>(hitCount);
		}
	}

	if (ImGui::CollapsingHeader("Trail", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Enable Trail", &editingProfile_.enableTrail);
		ImGui::ColorEdit4("Trail Start Color", &editingProfile_.trail.startColor.x);
		ImGui::ColorEdit4("Trail End Color", &editingProfile_.trail.endColor.x);

		int maxPts = static_cast<int>(editingProfile_.trail.maxPoints);
		if (ImGui::SliderInt("Max Points", &maxPts, 10, 500)) {
			editingProfile_.trail.maxPoints = static_cast<uint32_t>(maxPts);
		}

		int steps = static_cast<int>(editingProfile_.trail.interpolationSteps);
		if (ImGui::SliderInt("Interp Steps", &steps, 1, 16)) {
			editingProfile_.trail.interpolationSteps = static_cast<uint32_t>(steps);
		}

		ImGui::DragFloat3("Tip Offset", &editingProfile_.trail.tipOffset.x, 0.01f);
		ImGui::DragFloat3("Base Offset", &editingProfile_.trail.baseOffset.x, 0.01f);
	}

	if (ImGui::CollapsingHeader("Timing", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat("Duration (sec)", &editingProfile_.duration, 0.05f, 0.1f, 10.0f);
		ImGui::DragFloat("Hit Duration (sec)", &editingProfile_.hitDuration, 0.05f, 0.1f, 5.0f);
	}

	ImGui::Separator();

	// --- テスト発射ボタン ---
	if (ImGui::Button("Test Fire!")) {
		Fire(editingProfile_, defaultStartPos, defaultTargetPos);
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop")) {
		Reset();
	}

	ImGui::Separator();

	// --- ファイル操作 ---
	ImGui::InputText("Filename", profileFilename_, sizeof(profileFilename_));

	if (ImGui::Button("Save to JSON")) {
		SaveProfile(profileFilename_, editingProfile_);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load from JSON")) {
		if (LoadProfile(profileFilename_, editingProfile_)) {
			// Load成功時にバッファを再同期
			needsSync = true;
		}
	}

	ImGui::End();
}
#endif
