#include "Enemy.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include <cmath>
#include <algorithm>

namespace {
	constexpr float kFacePlayerYaw = -1.5708f;
	constexpr float kImportedEnemyYawCorrection = 1.5708f;

	float GetEnemyYaw_(EnemyType type)
	{
		switch (type) {
		case EnemyType::Goblin:
		case EnemyType::Golem:
		case EnemyType::Needle:
			return kFacePlayerYaw + kImportedEnemyYawCorrection;
		default:
			return kFacePlayerYaw;
		}
	}

	Vector3 GetEnemyScale_(EnemyType type)
	{
		switch (type) {
		case EnemyType::Slime:
			return { 1.5f, 1.5f, 1.5f };
		case EnemyType::Goblin:
			return { 0.45f, 0.45f, 0.45f };
		case EnemyType::Golem:
			return { 0.35f, 0.35f, 0.35f };
		default:
			return { 1.0f, 1.0f, 1.0f };
		}
	}
}

// ==========================================
// Enemy 本体
// ==========================================
void Enemy::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam,
	EnemyType type, const Vector3& spawnXYZ)
{
	objCommon_ = objCommon;
	dx_ = dx;
	cam_ = cam;
	type_ = type;
	pos_ = spawnXYZ;

	alive_ = true;
	hp_ = 300;
	block_ = 0;

	model_ = std::make_unique<Object3d>();
	model_->Initialize(objCommon_, dx_);
	model_->SetCamera(cam_);

	if (type_ == EnemyType::Boss) {
		model_->SetModel("enemy/boss/Boss.obj");
		ai_.LoadPattern("resources/cards/Boos.json");
	} else if (type_ == EnemyType::Slime) {

		model_->SetModel("slime/slime.obj");
		ai_.LoadPattern("resources/cards/Slime.json");
	} else if (type_ == EnemyType::Goblin) {
		model_->SetModel("Goblin/Goblin.obj");
		ai_.LoadPattern("resources/cards/Goblin.json");
	} else if (type_ == EnemyType::Golem) {
		model_->SetModel("Golem/Golem.obj");
		ai_.LoadPattern("resources/cards/Golem.json");
	} else if (type_ == EnemyType::Needle) {
		model_->SetModel("needle/needle.obj");
		ai_.LoadPattern("resources/cards/needle.json");
	}
	model_->SetScale(GetEnemyScale_(type_));

	// エネミーを左側（プレイヤー側）に向かせる
	rot_ = { 0.0f, GetEnemyYaw_(type_), 0.0f };

	if (model_->HasAnimation()) {
		model_->PlayAnimation("Idle", true);
	}

	// 読み込んだJSONの最大HPを、現在のHPにセットする
	maxHp_ = ai_.GetMaxHP();
	hp_ = maxHp_;

	basePos_ = pos_;

	model_->SetTranslate(pos_);
	model_->SetRotate(rot_);
	model_->Update(0.0f);

}

void Enemy::ApplyStageConfig(const StageEnemyConfig& config)
{
	if (!config.behaviorJson.empty()) {
		ai_.LoadPattern(config.behaviorJson);
	}

	if (config.maxHp > 0) {
		SetMaxHp(config.maxHp, true);
	}

	if (config.hp >= 0) {
		SetHp(config.hp);
	}
}

void Enemy::PlayAttackAnim(const Vector3& targetPos) {
	animState_ = AnimState::AttackForward;
	animTimer_ = 0.0f;
	animDuration_ = 0.2f;
	startPos_ = basePos_;
	targetPos_ = Lerp(basePos_, targetPos, 0.8f);
}

void Enemy::PlayDamageAnim() {
	animState_ = AnimState::Damage;
	animTimer_ = 0.0f;
	animDuration_ = 0.15f;
	startPos_ = basePos_;
	// ボスは右側にいる想定なので、右(Xのプラス方向)に下がる
	targetPos_ = { basePos_.x + 2.0f, basePos_.y, basePos_.z };
}

void Enemy::Update(float dt)
{
	if (!alive_) return;

	// 動き（アニメーション）の計算
	if (animState_ != AnimState::Idle) {
		animTimer_ += dt;
		float t = animTimer_ / animDuration_;
		if (t > 1.0f) t = 1.0f;

		float easeT = t * (2.0f - t);
		pos_ = Lerp(startPos_, targetPos_, easeT);

		if (animTimer_ >= animDuration_) {
			if (animState_ == AnimState::AttackForward) {
				animState_ = AnimState::AttackReturn;
				animTimer_ = 0.0f;
				animDuration_ = 0.3f;
				startPos_ = pos_;
				targetPos_ = basePos_;
			} else if (animState_ == AnimState::AttackReturn || animState_ == AnimState::Damage) {
				animState_ = AnimState::Idle;
				pos_ = basePos_;
			}
		}
	}

	// 赤点滅（フラッシュ）の計算
	if (flashTimer_ > 0.0f) {
		flashTimer_ -= dt;
		model_->SetMaterialColor({ 1.0f, 0.2f, 0.2f, 1.0f });
	} else if (isHighlighted_) {
		model_->SetMaterialColor({ 1.5f, 1.5f, 0.5f, 1.0f });
	} else {
		model_->SetMaterialColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
	isHighlighted_ = false;
	// AIの更新（やられ判定など最低限の処理）
	ai_.Update(*this, dt);

	// 死亡判定
	if (hp_ <= 0) {
		alive_ = false;
		return;
	}

	if (model_) {
		// 座標と回転を適用
		model_->SetTranslate(pos_);
		model_->SetRotate(rot_);

		// アニメーション更新
		model_->Update(dt);
	}

	// 当たり判定用の箱（カードバトルで使うかもしれないので残しておく）
	body_.min = { pos_.x - 1.5f, pos_.y, pos_.z - 1.5f };
	body_.max = { pos_.x + 1.5f, pos_.y + 3.0f, pos_.z + 1.5f };
}

void Enemy::Draw()
{
	if (!alive_ || !model_) return;
	model_->Draw();
}

void Enemy::SetLighting(const LightingParam& p)
{
	light_ = p;
	if (!model_) return;

	model_->SetEnableLighting(light_.lightingMode);
	model_->SetDirection(light_.dir);
	model_->SetIntensity(light_.dirIntensity);
	model_->SetLightColor(light_.dirColor);
	model_->SetPointLightPos(light_.pointPos);
	model_->SetPointLightIntensity(light_.pointIntensity);
	model_->SetPointLightColor(light_.pointColor);
	model_->SetPointLightRadius(light_.pointRadius);
	model_->SetPointLightDecay(light_.pointDecay);
	model_->SetSpotLightPos(light_.spotPos);
	model_->SetSpotLightDirection(light_.spotDir);
	model_->SetSpotLightIntensity(light_.spotIntensity);
}

//チュートリアル用にHP設定

void Enemy::SetMaxHp(int maxHp, bool healToFull)
{
	maxHp_ = (std::max)(1, maxHp);

	if (healToFull) {
		hp_ = maxHp_;
	} else {
		hp_ = (std::min)(hp_, maxHp_);
	}
}

void Enemy::SetHp(int hp)
{
	hp_ = (std::clamp)(hp, 0, maxHp_);
}


// ==========================================
// EnemyManager
// ==========================================
void EnemyManager::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam)
{
	objCommon_ = objCommon;
	dx_ = dx;
	cam_ = cam;
	enemies_.clear();
}

void EnemyManager::Spawn(EnemyType type, const Vector3& pos)
{
	Enemy e{};
	e.Initialize(objCommon_, dx_, cam_, type, pos);
	enemies_.push_back(std::move(e));
}

void EnemyManager::SpawnWithConfig(const StageEnemyConfig& config)
{
	Enemy e{};
	e.Initialize(objCommon_, dx_, cam_, config.type, config.position);
	e.ApplyStageConfig(config);
	enemies_.push_back(std::move(e));
}

void EnemyManager::Update(float dt)
{
	// 全てのエネミーを更新
	for (auto& e : enemies_) {
		e.Update(dt);
	}

	// 死んだエネミーをリストから削除
	enemies_.erase(
		std::remove_if(enemies_.begin(), enemies_.end(),
			[](const Enemy& e) { return !e.IsAlive(); }),
		enemies_.end()
	);
}

void EnemyManager::Draw()
{
	for (auto& e : enemies_) {
		e.Draw();
	}
}

void EnemyManager::SetLighting(const LightingParam& p)
{
	for (auto& e : enemies_) {
		e.SetLighting(p);
	}
}

Enemy* EnemyManager::GetBoss()
{
	for (auto& e : enemies_) {
		if (e.IsAlive()) return &e;
	}
	return nullptr;
}
int EnemyManager::PickEnemyByMouse(int mouseX, int mouseY, const Matrix4x4& viewProj, float screenWidth, float screenHeight)
{
	int closestIndex = -1;
	float minDepth = 1.0f; // 1.0f(一番奥) ～ 0.0f(一番手前)

	for (int i = 0; i < (int)enemies_.size(); ++i) {
		if (!enemies_[i].IsAlive()) continue;

		// 敵の足元座標を少し上にずらし、体の中央付近をターゲットにする
		Vector3 pos = enemies_[i].GetPos();
		pos.y += 4.0f;
		pos.x += 2.0f;
		// 3Dのワールド座標を、2Dの画面座標に変換する計算
		float w = pos.x * viewProj.m[0][3] + pos.y * viewProj.m[1][3] + pos.z * viewProj.m[2][3] + viewProj.m[3][3];
		if (w <= 0.0f) continue; // カメラより後ろにいる場合は無視

		float cx = (pos.x * viewProj.m[0][0] + pos.y * viewProj.m[1][0] + pos.z * viewProj.m[2][0] + viewProj.m[3][0]) / w;
		float cy = (pos.x * viewProj.m[0][1] + pos.y * viewProj.m[1][1] + pos.z * viewProj.m[2][1] + viewProj.m[3][1]) / w;
		float cz = (pos.x * viewProj.m[0][2] + pos.y * viewProj.m[1][2] + pos.z * viewProj.m[2][2] + viewProj.m[3][2]) / w;

		float screenX = (cx + 1.0f) * 0.5f * screenWidth;
		float screenY = (1.0f - cy) * 0.5f * screenHeight;

		// マウス座標との距離を計算（半径 120 ピクセル以内ならクリック成功とみなす）
		float dx = screenX - (float)mouseX;
		float dy = screenY - (float)mouseY;
		if (dx * dx + dy * dy < 120.0f * 120.0f) {
			// 重なっている場合は、より手前にいる敵を優先する
			if (cz < minDepth) {
				minDepth = cz;
				closestIndex = i;
			}
		}
	}
	return closestIndex; // 誰もクリックされていなければ -1 を返す
}

Enemy* EnemyManager::GetEnemy(size_t index)
{
	if (index >= enemies_.size()) {
		return nullptr;
	}
	return &enemies_[index];
}

const Enemy* EnemyManager::GetEnemy(size_t index) const
{
	if (index >= enemies_.size()) {
		return nullptr;
	}
	return &enemies_[index];
}

void Enemy::SetBC(BadCondition condition) {
	if (badCondition_ == condition) {
		return;
	}
	badConditionPoint_ = 0; // 状態異常ポイントのリセット
	badCondition_ = condition;
}

// 状態異常ポイントの加算
void Enemy::AddBC(int value) {
	SetBC(badConditionPoint_ + value);
}

void Enemy::SubtractBC(int value) {
	SetBC(badConditionPoint_ - value);
}

void  Enemy::AmplifyBC(int value) {
	SetBC(badConditionPoint_ * value);
}

// 状態異常ダメージの適用
void  Enemy::DamageBC(int count) {
	Damage(badConditionPoint_ * count);
}

void  Enemy::RemoveBC() {
	SetBC(BadCondition::kNone);
	SetBC(0);
}

// ターン終了時の状態異常ダメージ処理　いったんあとで
void Enemy::TurnEndApplyBC() {

	// 毒のポイントを半減（切り上げ）
	switch (badCondition_)
	{
	case BadCondition::kPoison:

		Damage(badConditionPoint_);
		badConditionPoint_ = (badConditionPoint_ + 1) / 2;

		break;
	case BadCondition::kFrost:

		// 凍結耐性を超えるポイントが溜まったら状態異常を解除して耐性を上げる
		if (badConditionPoint_ >= 5) {
			badConditionPoint_ = 0;
			freezeResistance_ += 5;
			isAbleToAct_ = false;
		} else {
			badConditionPoint_ -= 1;
		}
		break;

	}

	// ポイントが0以下になったら状態異常を解除
	if (badConditionPoint_ <= 0) {
		badConditionPoint_ = 0;
		badCondition_ = BadCondition::kNone;
	}

}
