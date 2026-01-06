#include "Enemy.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include <cstdlib>
#include <algorithm>
#include <cmath>

static const char* kShooterWalkModels[] = {
	"enemy/shooter/walk/walk1.obj",
	"enemy/shooter/walk/walk2.obj",
	"enemy/shooter/walk/walk3.obj",
	"enemy/shooter/walk/walk4.obj",
	"enemy/shooter/walk/walk5.obj",
};

static const char* kMeleeWalkModels[] = {
	"enemy/melee/dush/dush.fbx",   // ★1枚目はFBX
	"enemy/melee/dush/dush2.obj",
	"enemy/melee/dush/dush3.obj",
	"enemy/melee/dush/dush4.obj",
	"enemy/melee/dush/dush5.obj",
};


static const char* kMeleeAttackModels[] = {
	"enemy/melee/iAttak/attak1.fbx",
	"enemy/melee/iAttak/attak2.fbx",
	"enemy/melee/iAttak/attak3.fbx",
};

static const char* kMeleeDamageModel = "enemy/melee/damage/damage.obj";
static const char* kShooterDamageModel = "enemy/shooter/damage/damage.obj";

static const char* kBossIdleModels[] = {
	"enemy/boss/idol/idol1.obj",
	"enemy/boss/idol/idol2.obj",
	"enemy/boss/idol/idol3.obj",
};



struct AttackHitboxParam {
	float rangeX = 0.0f;  // 前に出す距離
	float liftY = 0.0f;  // 上下オフセット（+で上、-で下）
	float hx = 0.8f;  // 半幅
	float hy = 1.0f;  // 高さ（下端から上へ）
	float hz = 0.6f;  // Z厚み（±）
};

inline AABB MakeAttackHitboxAABB(
	const Vector3& attackerPos,
	int facing,                 // +1 right / -1 left
	const AttackHitboxParam& p
) {
	AABB hit{};
	const float cx = attackerPos.x + float(facing) * p.rangeX;

	hit.min = { cx - p.hx, attackerPos.y + p.liftY,       attackerPos.z - p.hz };
	hit.max = { cx + p.hx, attackerPos.y + p.liftY + p.hy, attackerPos.z + p.hz };
	return hit;
}

inline bool Intersect3(const EnemyManager::AABB3& a, const EnemyManager::AABB3& b) {
	return (std::abs(a.x - b.x) <= (a.hx + b.hx)) &&
		(std::abs(a.y - b.y) <= (a.hy + b.hy)) &&
		(std::abs(a.z - b.z) <= (a.hz + b.hz));
}



void Enemy::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam,
	EnemyType type, const Vector3& spawnXYZ) {
	type_ = type;
	alive_ = true;

	pos_ = { spawnXYZ.x, spawnXYZ.y, spawnXYZ.z };
	vel_ = { 0,0,0 };

	hitstun_ = false;
	hitstunTime_ = 0.0f;
	onGround_ = true;
	airborne_ = false;
	if (type_ == EnemyType::Boss) {
		hp_ = 400;
		damageTaken_ = 1; // 仕様でダメージ無効なら0
		bossAI_.Reset(400);

	} else if (type_ == EnemyType::Shooter) {
		hp_ = 20;
		damageTaken_ = 1; // Shooterは2減る（例）

	} else if (type_ == EnemyType::Melee) {
		hp_ = 20;
		damageTaken_ = 1; // Meleeは1減る（例）
	}


	model_ = std::make_unique<Object3d>();
	model_->Initialize(objCommon, dx);
	model_->SetCamera(cam);

	// 仮モデル（あるものに差し替えてOK）
	//model_->SetModel("cube/cube.obj");

	UpdateBody_();
	UpdateModel_();

	meleeState_ = MeleeState::Approach;
	shooterState_ = ShooterState::Retreat;

	meleeWindup_ = 0.0f;
	meleeAttack_ = 0.0f;
	shootWindup_ = 0.0f;

	requestMeleeAttack_ = false;
	requestShoot_ = false;
	shootDir_ = +1;
	shootMuzzlePos_ = pos_;

	debugHitboxCube_ = std::make_unique<Object3d>();
	debugHitboxCube_->Initialize(objCommon, dx);
	debugHitboxCube_->SetCamera(cam);
	//debugHitboxCube_->SetModel("cube/cube.obj");

	//ボス用
	meleeKind_ = MeleeKind::Normal;

	auto* mgr = ModelManager::GetInstance();

	if (type_ == EnemyType::Shooter) {

		for (auto& path : kShooterWalkModels) { mgr->LoadModel(path); }
		for (int i = 0; i < 5; ++i) {
			shooterWalkModels_[i] = mgr->FindModel(kShooterWalkModels[i]);
			assert(shooterWalkModels_[i]);
		}

		// ★ダメージモデル
		mgr->LoadModel(kShooterDamageModel);
		shooterDamageModel_ = mgr->FindModel(kShooterDamageModel);
		assert(shooterDamageModel_);

		model_->SetModel(shooterWalkModels_[0]);

	} else  if (type_ == EnemyType::Melee) {

		for (auto& path : kMeleeWalkModels) { mgr->LoadModel(path); }
		for (int i = 0; i < 5; ++i) {
			meleeWalkModels_[i] = mgr->FindModel(kMeleeWalkModels[i]);
			assert(meleeWalkModels_[i]);
		}

		// ★攻撃（iAttak）ロード
		for (auto& path : kMeleeAttackModels) { mgr->LoadModel(path); }
		for (int i = 0; i < 3; ++i) {
			meleeAttackModels_[i] = mgr->FindModel(kMeleeAttackModels[i]);
			assert(meleeAttackModels_[i]);
		}

		// ★ダメージモデル
		mgr->LoadModel(kMeleeDamageModel);
		meleeDamageModel_ = mgr->FindModel(kMeleeDamageModel);
		assert(meleeDamageModel_);

		model_->SetModel(meleeWalkModels_[0]);

	} else {
		// Bossなど
		//model_->SetModel("cube/cube.obj");
	}

	if (type_ == EnemyType::Melee) {
		damageScaleMul_ = 3.0f; // ←まずは3倍くらいから。後で調整
	} else {
		damageScaleMul_ = 1.0f;
	}

	if (type_ == EnemyType::Boss) {
		for (auto& path : kBossIdleModels) { mgr->LoadModel(path); }
		for (int i = 0; i < kBossIdleFrameCount; ++i) {
			bossIdleModels_[i] = mgr->FindModel(kBossIdleModels[i]);
			assert(bossIdleModels_[i]);
		}
		model_->SetModel(bossIdleModels_[0]);
	} else {
	//	model_->SetModel("cube/cube.obj");
	}

}

void Enemy::Update(float dt, const Vector2& playerXY, float playerZ) {
	if (!alive_) return;

	// ★Test用：完全停止
	if (frozen_) {
		vel_ = { 0,0,0 };
		requestMeleeAttack_ = false;
		requestShoot_ = false;

		// 見た目と当たり判定は更新しておく（攻撃判定がちゃんと当たる）
		UpdateBody_();
		UpdateModel_();
		return;
	}

	facing_ = (playerXY.x < pos_.x) ? -1 : +1;

	if (hitstunTime_ > 0.0f) {
		hitstunTime_ -= dt;
		if (hitstunTime_ <= 0.0f) hitstun_ = false;
	}

	if (meleeTimer_ > 0.0f) meleeTimer_ -= dt;
	if (shootTimer_ > 0.0f) shootTimer_ -= dt;

	if (!aiDisabled_) {
		if (!hitstun_ || type_ == EnemyType::Boss) {
			if (type_ == EnemyType::Melee) UpdateAI_Melee_(dt, playerXY, playerZ);
			else if (type_ == EnemyType::Shooter) UpdateAI_Shooter_(dt, playerXY, playerZ);
			else UpdateAI_Boss_(dt, playerXY, playerZ);
		}
	}


	// ★被弾フラッシュ更新
	if (hitFlashSec_ > 0.0f) {
		hitFlashSec_ -= dt;
		if (hitFlashSec_ < 0.0f) hitFlashSec_ = 0.0f;
	}

	// ★色反映（毎フレームでOK）
	if (model_) {
		if (hitFlashSec_ > 0.0f) model_->SetMaterialColor(hitColor_);
		else                      model_->SetMaterialColor(normalColor_);
	}

	// ===== Melee 歩きアニメ（動いてる時だけ）=====
	if (type_ == EnemyType::Melee && meleeWalkModels_[0]) {

		const bool moving =
			(std::abs(vel_.x) > 0.01f) ||
			(std::abs(vel_.z) > 0.01f);

		if (moving) {
			walkAnimTime_ += dt;
			int frame = static_cast<int>(walkAnimTime_ * kWalkFps_) % kWalkFrameCount_;
			model_->SetModel(meleeWalkModels_[frame]);
		} else {
			walkAnimTime_ = 0.0f;
			model_->SetModel(meleeWalkModels_[0]); // idle扱い
		}
	}

	// ===== Shooter 歩きアニメ（AIが動かしてる時だけ）=====
	if (type_ == EnemyType::Shooter && shooterWalkModels_[0]) {

		const bool moving =
			(std::abs(vel_.x) > 0.01f) ||
			(std::abs(vel_.z) > 0.01f);

		if (moving) {
			walkAnimTime_ += dt;
			int frame = static_cast<int>(walkAnimTime_ * kWalkFps_) % kWalkFrameCount_;
			model_->SetModel(shooterWalkModels_[frame]);
		} else {
			walkAnimTime_ = 0.0f;
			model_->SetModel(shooterWalkModels_[0]); // idle扱い
		}
	}

	// ★物理は常に回す（吹き飛びたいので）
	ApplyPhysics_(dt);
	UpdateBody_();
	UpdateModel_();

}

void Enemy::Draw() {
	if (!alive_) return;
	if (model_) model_->Draw();
}

EnemyHitResult Enemy::ApplyHit2D(float knockVx, float launchVy, bool requestHitstun) {
	EnemyHitResult r{};
	if (!alive_) return r;

	r.hit = true;

	// =========================
	// ★ダメージ（invincible なら減らさない）
	// =========================
	if (!invincible_) {
		hp_ -= damageTaken_;
		if (hp_ <= 0) {
			hp_ = 0;
			alive_ = false;
			r.killed = true;
			return r;
		}
	}

	hitFlashSec_ = std::max(hitFlashSec_, 0.20f); // 0.2秒赤く

	// =========================
	// ★攻撃（AI）リセット：殴られたら最初から
	// =========================
	meleeState_ = MeleeState::Approach;
	meleeWindup_ = 0.0f;
	meleeAttack_ = 0.0f;
	meleeTimer_ = 0.0f;
	requestMeleeAttack_ = false;

	shooterState_ = ShooterState::Retreat;
	shootWindup_ = 0.0f;
	shootTimer_ = 0.0f;
	requestShoot_ = false;

	// =========================
	// ★吹き飛ばし（ここは invincible でも効かせる）
	// =========================
	vel_.x = knockVx;
	vel_.y = launchVy;
	airborne_ = true;
	onGround_ = false;

	if (requestHitstun) {
		hitstun_ = true;
		hitstunTime_ = 0.40f;
	}

	return r;
}

void Enemy::UpdateAI_Melee_(float dt, const Vector2& playerXY, float playerZ) {
	const float dx = playerXY.x - pos_.x;
	const float adx = std::abs(dx);

	// 見た目Z追従（今のまま）
	const float dz = playerZ - pos_.z;
	const float adz = std::abs(dz);
	if (adz > zFollowDeadZone_) vel_.z = (dz > 0) ? depthSpeed_ : -depthSpeed_;
	else                       vel_.z = 0.0f;

	switch (meleeState_) {
	case MeleeState::Approach:
		if (adx > meleeRange_) {
			vel_.x = (dx > 0) ? moveSpeed_ : -moveSpeed_;
		} else {
			// 範囲内に入ったら停止→溜めへ
			vel_.x = 0.0f;
			meleeWindup_ = meleeWindupTime_;
			meleeState_ = MeleeState::Windup;
		}
		break;

	case MeleeState::Windup:
		vel_.x = 0.0f;
		meleeWindup_ -= dt;
		if (meleeWindup_ <= 0.0f) {
			// ★攻撃発生（判定は別で作る：ここでは要求フラグだけ）
			RequestMelee(MeleeKind::Normal);

			meleeAttack_ = meleeAttackTime_;
			meleeState_ = MeleeState::Attack;
		}
		break;

	case MeleeState::Attack:
		vel_.x = 0.0f;
		meleeAttack_ -= dt;
		if (meleeAttack_ <= 0.0f) {
			meleeTimer_ = meleeCooldown_;     // 既存 cooldown をここで使う
			meleeState_ = MeleeState::Cooldown;
		}
		break;

	case MeleeState::Cooldown:
		vel_.x = 0.0f;

		// クールダウンが終わったらまた接近へ
		if (meleeTimer_ <= 0.0f) {
			meleeState_ = MeleeState::Approach;
		}
		break;
	}
}

static float CalcXMaxByZ(float z) {
	const float zNear = -10.0f;
	const float zFar = 20.0f;
	const float xMaxNear = 15.0f;
	const float xMaxFar = 20.0f;

	float t = (z - zNear) / (zFar - zNear);
	t = std::clamp(t, 0.0f, 1.0f);
	return xMaxNear + (xMaxFar - xMaxNear) * t;
}

void Enemy::UpdateAI_Shooter_(float dt, const Vector2& playerXY, float playerZ)
{
	// 死亡/ひるみ中は撃たない（好みで）
	if (!alive_) return;
	if (hitstun_) {
		vel_.x = 0.0f;
		vel_.y = 0.0f;
		return;
	}

	// プレイヤー方向
	const float dx = playerXY.x - pos_.x;
	const float dy = playerXY.y - pos_.y;

	// 向き更新
	facing_ = (dx < 0.0f) ? -1 : +1;

	// =========================
// ★ステージ境界へ戻す（X/Zが制限外なら、入るまで移動）
// =========================
	{
		// Playerと同じZ制限
		const float zNear = -10.0f;
		const float zFar = 20.0f;

		// Zはプレイヤーへ追従してるけど、まず制限内に収めたい
		const float zClamped = std::clamp(pos_.z, zNear, zFar);

		// Zの制限外なら、まずZを制限内へ押し戻す（優先）
		if (pos_.z != zClamped) {
			const float targetZ = zClamped;
			const float dz = targetZ - pos_.z;
			vel_.z = (dz > 0.0f) ? depthSpeed_ : -depthSpeed_;
			vel_.x = 0.0f;
			vel_.y = 0.0f;
			return; // ★このフレームは「戻る」だけ
		}

		// X制限（Shooterは少し内側に入れたいなら margin）
		const float margin = 0.5f; // 0でもOK。内側に寄せたいなら少し
		const float xMax = CalcXMaxByZ(pos_.z) - margin;

		const float xClamped = std::clamp(pos_.x, -xMax, xMax);

		if (pos_.x != xClamped) {
			// Xが外なら、境界へ戻る
			const float targetX = xClamped;
			const float dxTo = targetX - pos_.x;

			// 速度で戻す（moveSpeed_ を使う）
			vel_.x = (dxTo > 0.0f) ? moveSpeed_ : -moveSpeed_;
			vel_.y = 0.0f;
			// vel_.z は既にプレイヤー追従の設定があるならそのままでもOK
			// ただし「戻る優先」にしたいなら vel_.z = 0 にしても良い
			return; // ★このフレームは「戻る」だけ
		}
	}


	// ★Z追従（プレイヤーZに寄せる）
	{
		const float dz = playerZ - pos_.z;
		if (std::abs(dz) > zFollowDeadZone_) {
			vel_.z = (dz > 0.0f) ? depthSpeed_ : -depthSpeed_;
		} else {
			vel_.z = 0.0f;
		}
	}

	// ShooterState：簡易ステートマシン
	switch (shooterState_) {
	case ShooterState::Retreat:
	{
		// ★プレイヤーが近づいても動かない
		vel_.x = 0.0f;
		vel_.y = 0.0f;

		// すぐ狙いへ（またはAim固定でもOK）
		shooterState_ = ShooterState::Aim;
	}
	break;



	case ShooterState::Aim:
	{
		// Y がある程度揃ってたら溜めへ（揃ってなくても撃つなら条件外す）
		if (std::abs(dy) <= shooterAlignYEps_) {
			shooterState_ = ShooterState::Windup;
			shootWindup_ = shootWindupTime_;
		}

		vel_.x = 0.0f;
		vel_.y = 0.0f;
	}
	break;

	case ShooterState::Windup:
	{
		vel_.x = 0.0f;
		vel_.y = 0.0f;

		shootWindup_ -= dt;
		if (shootWindup_ <= 0.0f) {
			// ★ここが一番重要：発射要求を立てる
			requestShoot_ = true;

			// 銃口位置（適当にオフセット）
			shootMuzzlePos_.x = pos_.x + 1.0f * float(facing_);
			shootMuzzlePos_.y = pos_.y + 0.8f;
			shootMuzzlePos_.z = playerZ; // Z見た目だけならプレイヤーZに合わせると確実に見える

			shootDir_ = facing_;

			// クールダウンへ
			shooterState_ = ShooterState::Cooldown;
			shootTimer_ = shootCooldown_;
		}
	}
	break;

	case ShooterState::Cooldown:
	{
		vel_.x = 0.0f;
		vel_.y = 0.0f;

		shootTimer_ -= dt;
		if (shootTimer_ <= 0.0f) {
			shooterState_ = ShooterState::Retreat;
		}
	}
	break;
	}

	// Zは見た目だけ運用なら pos_.z は固定でもOK
	// pos_.z = zView_;
}



void Enemy::UpdateAI_Boss_(float dt, const Vector2& playerXY, float playerZ) {
	bossAI_.Update(*this, dt, playerXY, playerZ);
}


void Enemy::ApplyPhysics_(float dt) {
	if (!onGround_) {
		vel_.y -= gravity_ * dt;
	}

	pos_.x += vel_.x * dt;
	pos_.y += vel_.y * dt;

	// ★Zも動かす（見た目用）
	pos_.z += vel_.z * dt;
	//  pos_.z = std::clamp(pos_.z, zMin_, zMax_);

	if (pos_.y <= 0.0f) {
		pos_.y = 0.0f;
		vel_.y = 0.0f;
		onGround_ = true;
		airborne_ = false;
	}
}

void Enemy::UpdateBody_() {
	// 足元基準の簡易AABB（ボスは大きく）
	float hx = 0.4f, hy = 0.75f, hz = 0.6f;         // ★hz追加
	if (type_ == EnemyType::Boss) { hx = 1.2f; hy = 2.0f; hz = 1.4f; }

	body_.min = { pos_.x - hx, pos_.y,           pos_.z - hz };   // ★Zもpos_.z基準
	body_.max = { pos_.x + hx, pos_.y + hy * 2.0f, pos_.z + hz }; // ★Zもpos_.z基準
}

void Enemy::UpdateModel_() {
	if (!model_) return;

	model_->SetTranslate({ pos_.x, pos_.y, pos_.z });

	const float flipX = (facing_ > 0) ? 1.0f : -1.0f;
	if (type_ == EnemyType::Boss) model_->SetScale({ 2.0f * flipX, 2.0f, 2.0f });
	else                         model_->SetScale({ 1.0f * flipX, 1.0f, 1.0f });

	// -------------------------
	// ★ここから「見た目モデル選択」を1本化
	// -------------------------
	Model* chosen = nullptr;

	bool usingDamageModel =
		(chosen == meleeDamageModel_) ||
		(chosen == shooterDamageModel_);

	// 1) 被弾中は damage を優先
	if (hitFlashSec_ > 0.0f) {
		if (type_ == EnemyType::Melee && meleeDamageModel_) {
			chosen = meleeDamageModel_;
		} else if (type_ == EnemyType::Shooter && shooterDamageModel_) {
			chosen = shooterDamageModel_;
		}
	}

	// 2) 近接の攻撃（Windup / Attack）は iAttak
	if (!chosen && type_ == EnemyType::Melee && meleeAttackModels_[0]) {
		if (meleeState_ == MeleeState::Windup) {
			chosen = meleeAttackModels_[0]; // 溜め
			meleeAtkAnimTime_ = 0.0f;       // 溜め中は固定でもOK
		} else if (meleeState_ == MeleeState::Attack) {
			// 攻撃中だけ時間進める（Update()側で進めてもOK）
			meleeAtkAnimTime_ += (1.0f / 60.0f); // dt渡せないならこう。渡せるなら dt を使うのが理想

			// 例：攻撃時間を 3コマに割る
			float t = meleeAtkAnimTime_;
			if (t < 0.06f)      chosen = meleeAttackModels_[0];
			else if (t < 0.12f) chosen = meleeAttackModels_[1];
			else                chosen = meleeAttackModels_[2];
		} else {
			meleeAtkAnimTime_ = 0.0f;
		}
	}

	// 3) それ以外：walk
	if (!chosen) {
		if (type_ == EnemyType::Boss && bossIdleModels_[0]) {

			const bool moving =
				(std::abs(vel_.x) > 0.01f) ||
				(std::abs(vel_.z) > 0.01f) ||
				(std::abs(vel_.y) > 0.01f);

			if (moving) {
				bossIdleAnimTime_ += (1.0f / 60.0f); // dtを渡せないなら今はこれ
				int frame = int(bossIdleAnimTime_ * kBossIdleFps_) % kBossIdleFrameCount;
				chosen = bossIdleModels_[frame];
			} else {
				bossIdleAnimTime_ = 0.0f;
				chosen = bossIdleModels_[0];
			}


		} else if (type_ == EnemyType::Shooter && shooterWalkModels_[0]) {
			const bool moving = (std::abs(vel_.x) > 0.01f) || (std::abs(vel_.z) > 0.01f);
			if (moving) {
				int frame = static_cast<int>(walkAnimTime_ * kWalkFps_) % kWalkFrameCount_;
				chosen = shooterWalkModels_[frame];
			} else {
				chosen = shooterWalkModels_[0];
			}
		} else if (type_ == EnemyType::Melee && meleeWalkModels_[0]) {
			const bool moving = (std::abs(vel_.x) > 0.01f) || (std::abs(vel_.z) > 0.01f);
			if (moving) {
				int frame = static_cast<int>(walkAnimTime_ * kWalkFps_) % kWalkFrameCount_;
				chosen = meleeWalkModels_[frame];
			} else {
				chosen = meleeWalkModels_[0];
			}
		}
	}

	if (chosen) model_->SetModel(chosen);

	model_->Update();
}


// -------- EnemyManager --------

float EnemyManager::RandRange_(float a, float b) {
	return a + (b - a) * Rand01_();
}

void EnemyManager::QueueSpawn(EnemyType type, float delaySec) {
	if (enemies_.size() >= maxAlive_) {
		return; // ★上限なら予約しない（B）
	}
	pendingSpawns_.push_back({ type, delaySec });
}


void EnemyManager::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam) {
	objCommon_ = objCommon;
	dx_ = dx;
	cam_ = cam;

	enemies_.clear();
	meleeHitboxes_.clear();

	debugHitboxCube_ = std::make_unique<Object3d>();
	debugHitboxCube_->Initialize(objCommon_, dx_);
	debugHitboxCube_->SetCamera(cam_);
	debugHitboxCube_->SetModel("heal/heal.obj");

	//弾
	bullets_.Initialize(objCommon_, dx_, cam_);

}

void EnemyManager::Clear() {
	enemies_.clear();
	meleeHitboxes_.clear();
	bullets_.Clear();
}

void EnemyManager::Spawn(EnemyType type, const Vector3& posXYZ) {
	Enemy e;
	e.Initialize(objCommon_, dx_, cam_, type, posXYZ);
	enemies_.push_back(std::move(e));
}

void EnemyManager::Update(float dt, const Vector2& playerXY, float playerZ, Player& player) {
	// 1) 敵本体の更新
	for (auto& e : enemies_) {
		e.Update(dt, playerXY, playerZ); // ← もし使うなら引数を戻してOK
	}

	// 2) 近接ヒットボックス寿命更新
	for (auto& h : meleeHitboxes_) h.life -= dt;
	meleeHitboxes_.erase(
		std::remove_if(meleeHitboxes_.begin(), meleeHitboxes_.end(),
			[](const MeleeHitbox& h) { return h.life <= 0.0f; }),
		meleeHitboxes_.end()
	);

	auto ToAABB3 = [](const AABB& a)->AABB3 {
		AABB3 b{};
		b.x = (a.min.x + a.max.x) * 0.5f;
		b.y = (a.min.y + a.max.y) * 0.5f;
		b.z = (a.min.z + a.max.z) * 0.5f;
		b.hx = (a.max.x - a.min.x) * 0.5f;
		b.hy = (a.max.y - a.min.y) * 0.5f;
		b.hz = (a.max.z - a.min.z) * 0.5f;
		return b;
		};

	// 近接ヒットボックス vs プレイヤー
	const AABB3 playerBody3 = ToAABB3(player.GetBodyAABB());

	for (auto& h : meleeHitboxes_) {
		if (Intersect3(h.box, playerBody3)) {
			player.TriggerHitFlash(0.25f); // 好きな秒数
			player.Damage(h.damage);

			// 1回当たったら消すなら
			h.life = 0.0f;
		}
	}


	// 3) 攻撃要求を回収
	for (auto& e : enemies_) {

		// ---- Shooter：弾発射要求 ----
		Vector3 muzzle{};
		int dir = +1;
		if (e.ConsumeShootRequest(muzzle, dir)) {

			OutputDebugStringA("[Shoot] request OK\n");

			bullets_.Spawn(muzzle, dir,7);
		}

		// ---- Melee：近接攻撃ヒットボックス生成 ----
		MeleeKind kind{};
		if (e.ConsumeMeleeRequest(kind)) {

			Vector3 ep = e.GetPos3D();
			const bool isBoss = e.IsBoss();

			// ★ 先に保険：非ボスは必ず Normal
			if (!isBoss) {
				kind = MeleeKind::Normal;
			}

			int facing = (playerXY.x < ep.x) ? -1 : +1;
			const float zCenter = ep.z;

			float offX = 1.2f, offY = 0.0f;
			float halfX = 0.6f, halfY = 0.5f, halfZ = 0.5f;
			float life = 0.10f;

			int dmg = 1;

			switch (kind) {
			case MeleeKind::Normal:

				

				break;

			case MeleeKind::Land:

				dmg = 10;

				offX = 0.0f; halfX = 2.2f; halfY = 1.3f; halfZ = 1.2f; life = 0.08f;
				break;

			case MeleeKind::Rush:

				dmg = 10;

				offX = 0.8f; halfX = 1.4f; halfY = 1.0f; halfZ = 1.2f; life = 0.06f;
				break;
			}



			AABB3 hb{};
			hb.x = ep.x + offX * float(facing);
			hb.y = ep.y + offY;
			hb.z = zCenter;
			hb.hx = halfX;
			hb.hy = halfY;
			hb.hz = halfZ;

			meleeHitboxes_.push_back({ hb, life, dmg });
		}

	}

	// 4) 死亡削除（★削除直前に回復ドロップ抽選）
	enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
		[this](const Enemy& e) {
			if (!e.IsAlive()) {

				// ★ボス死亡フラグ
				if (e.GetType() == EnemyType::Boss) {
					bossDefeated_ = true;
					// ボスは復活予約しない・回復ドロップもしないならここでreturnでもOK
				}

				// ★回復ドロップ抽選（ボスはTrySpawnHealDrop_側で弾いてる）
				TrySpawnHealDrop_(e);

				// ★Melee / Shooter が倒されたら予約スポーン
				if (e.GetType() == EnemyType::Melee || e.GetType() == EnemyType::Shooter) {
					QueueSpawn(e.GetType(), respawnDelay_);
				}

				return true;
			}
			return false;
		}), enemies_.end());


	bullets_.Update(dt, player);

	UpdateHealDrops_(dt, player);

	UpdatePendingSpawns_(dt, playerXY, playerZ);

}

void EnemyManager::Draw() {

	DrawHealDrops_();

	for (auto& e : enemies_) e.Draw();

	for (auto& e : enemies_) {
		if (e.IsBoss()) {
			Vector3 p = e.GetPos3D();
			char buf[128];
			sprintf_s(buf, "[Boss] pos=(%.2f, %.2f, %.2f)\n", p.x, p.y, p.z);
			OutputDebugStringA(buf);
		}
	}


	bullets_.Draw();

	//if (debugDrawMeleeHitbox_ && debugHitboxCube_) {
	//	for (const auto& h : meleeHitboxes_) {
	//		const auto& b = h.box; // AABB3（center + half）

	//		// center はそのまま使える
	//		Vector3 center{ b.x, b.y, b.z };

	//		// Object3d の cube は「scale が全サイズ」なら 2倍する
	//		// （あなたの実装が半幅スケールならここは調整）
	//		Vector3 size{ b.hx * 2.0f, b.hy * 2.0f, b.hz * 2.0f };

	//		debugHitboxCube_->SetTranslate(center);
	//		debugHitboxCube_->SetScale(size);
	//		debugHitboxCube_->Update();
	//		debugHitboxCube_->Draw();
	//	}
	//}



}

// 0..1
float EnemyManager::Rand01_() {
	return float(std::rand()) / float(RAND_MAX);
}

void EnemyManager::TrySpawnHealDrop_(const Enemy& e) {
	// ボスは落とさない
	if (e.GetType() != EnemyType::Melee && e.GetType() != EnemyType::Shooter) return;

	// 確率
	if (Rand01_() > healDropChance_) return;

	HealDrop d;
	d.pos = e.GetPos3D();
	d.life = 10.0f;
	d.radius = 0.6f;
	d.amount = healDropAmount_;
	healDrops_.push_back(d);
}

void EnemyManager::UpdateHealDrops_(float dt, Player& player) {
	const Vector3 p = player.GetPos3D();

	for (auto& d : healDrops_) {
		d.life -= dt;
		if (d.life <= 0.0f) continue;

		const float dx = p.x - d.pos.x;
		const float dy = p.y - d.pos.y;
		const float dz = p.z - d.pos.z;

		const float r = d.radius;
		if ((dx * dx + dy * dy + dz * dz) <= (r * r)) {
			player.AddHP(d.amount);

			// ★デバッグログ（回復したか確認）
			char buf[128];
			sprintf_s(buf, "[Heal] +%d hp -> %d\n", d.amount, player.GetHP());
			OutputDebugStringA(buf);

			d.life = 0.0f;
		}
	}

	healDrops_.erase(
		std::remove_if(healDrops_.begin(), healDrops_.end(),
			[](const HealDrop& d) { return d.life <= 0.0f; }),
		healDrops_.end()
	);
}


void EnemyManager::DrawHealDrops_() {
	// 見た目は「キューブ」で代用（手軽）
	// 既に debugHitboxCube_ を持ってるのでそれを流用できます
	if (!debugHitboxCube_) return;

	for (auto& d : healDrops_) {
		// ここはあなたの Object3d の使い方に合わせてください
		// 例：位置だけ置いて描画（色替えできるなら緑っぽく）
		debugHitboxCube_->SetTranslate(d.pos);
		debugHitboxCube_->SetScale({ 0.4f, 0.4f, 0.4f });
		debugHitboxCube_->Update();
		debugHitboxCube_->Draw();
	}
}

Vector3 EnemyManager::MakeOutsideSpawnPos_(const Vector2& playerXY, float playerZ) {
	const float halfW = 12.0f;   // 画面の半分幅（調整）
	const float pad = 3.0f;    // 画面外にどれだけ出すか
	const float xRand = 3.0f;    // 外側でのばらつき
	const float yRand = 0.0f;    // Yのばらつき

	const bool fromLeft = (std::rand() % 2) == 0;

	float x;
	if (fromLeft) {
		x = RandRange_(playerXY.x - halfW - pad - xRand,
			playerXY.x - halfW - pad);
	} else {
		x = RandRange_(playerXY.x + halfW + pad,
			playerXY.x + halfW + pad + xRand);
	}

	float y = RandRange_(playerXY.y - yRand, playerXY.y + yRand);

	// Zは見た目用なら playerZ に合わせるのが無難
	return Vector3{ x, y, playerZ };
}

void EnemyManager::UpdatePendingSpawns_(float dt, const Vector2& playerXY, float playerZ) {
	// タイマー更新
	for (auto& p : pendingSpawns_) p.t -= dt;

	// ★上限に達してるなら予約を全部捨てる（B）
	if (enemies_.size() >= maxAlive_) {
		pendingSpawns_.clear();
		return;
	}

	// t<=0 のものを、空きがある分だけスポーン
	for (size_t i = 0; i < pendingSpawns_.size();) {
		if (enemies_.size() >= maxAlive_) {
			// 途中で上限に達したら、残り予約は捨てる（B）
			pendingSpawns_.clear();
			return;
		}

		if (pendingSpawns_[i].t <= 0.0f) {
			EnemyType type = pendingSpawns_[i].type;
			Vector3 pos = MakeOutsideSpawnPos_(playerXY, playerZ);
			Spawn(type, pos);

			pendingSpawns_.erase(pendingSpawns_.begin() + i);
		} else {
			++i;
		}
	}
}

