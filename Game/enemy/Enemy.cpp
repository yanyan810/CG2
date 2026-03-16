#include "Enemy.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include <cmath>

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

	model_ = std::make_unique<Object3d>();
	model_->Initialize(objCommon_, dx_);
	model_->SetCamera(cam_);

	// 今回はボスのモデルのみ読み込む
	model_->SetModel("enemy/boss/boss.gltf");
	model_->SetScale({ 3.0f, 3.0f, 3.0f });
	// エネミーを左側（プレイヤー側）に向かせる
	rot_ = { 0.0f, -1.5708f, 0.0f };

	// 待機アニメーションの再生
	if (model_->HasAnimation()) {
		model_->PlayAnimation("Idle", true);
	}

	// AIの初期化（もうHP管理など簡単なものだけ）
	ai_.Reset(hp_);

	basePos_ = pos_;

	if (type == EnemyType::Boss) {
		ai_.LoadPattern("resources/cards/Boos.json");
	}
	else if (type == EnemyType::Slime) {
		ai_.LoadPattern("resources/cards/Slime.json");
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