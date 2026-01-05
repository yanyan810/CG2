#include "PlayerCombo.h"
#include "Input.h"
#include "Enemy.h"   // Enemy / EnemyManager



void PlayerCombo::Start(AttackType type) {
	if (attacking_) return;

	attackType_ = type;
	attacking_ = true;
	t_ = 0.0f;      // 攻撃経過時間
	//hitDone_ = false;   // 多段ヒット防止などがあるなら
}

void PlayerCombo::Reset() {
	buf_.clear();
	attacking_ = false;
	attackAir_ = false;
	step_ = 0;
	t_ = 0.0f;
	curBtn_ = AttackBtn::Weak;
	startDirY_ = 0;
}

void PlayerCombo::Push_(AttackBtn b) { buf_.push_back({ b, bufKeep_ }); }

bool PlayerCombo::Pop_(AttackBtn& out) {
	if (buf_.empty()) return false;
	out = buf_.front().btn;
	buf_.erase(buf_.begin());
	return true;
}

void PlayerCombo::UpdateBuf_(float dt) {
	for (auto& it : buf_) it.life -= dt;
	buf_.erase(std::remove_if(buf_.begin(), buf_.end(),
		[](const BufItem& i) { return i.life <= 0.0f; }), buf_.end());
}

int PlayerCombo::ReadDirY_(const Input& in) const {
	// 押しっぱなし方向を読む（攻撃開始時に固定する）
	if (in.IsKeyPressed(DIK_UP))   return +1;
	if (in.IsKeyPressed(DIK_DOWN)) return -1;
	return 0;
}

AttackData PlayerCombo::GetData_(bool airborne, int step, AttackBtn btn) const {
	AttackData a{};
	if (!airborne) {
		if (btn == AttackBtn::Weak) {
			a.duration = (step == 2) ? 0.42f : 0.34f;
			a.hitStart = 0.08f; a.hitEnd = 0.18f;
			a.chainOpen = 0.12f; a.chainClose = a.duration - 0.08f;
			a.knockX = 6.0f + step * 1.0f;
			a.launchY = 7.0f + step * 1.0f;
		} else {
			a.duration = (step == 2) ? 0.55f : 0.48f;
			a.hitStart = 0.12f; a.hitEnd = 0.24f;
			a.chainOpen = 0.18f; a.chainClose = a.duration - 0.10f;
			a.knockX = 8.0f + step * 1.0f;
			a.launchY = 9.0f + step * 1.0f;
		}
	} else {
		if (btn == AttackBtn::Weak) {
			a.duration = (step == 2) ? 0.40f : 0.32f;
			a.hitStart = 0.06f; a.hitEnd = 0.16f;
			a.chainOpen = 0.10f; a.chainClose = a.duration - 0.06f;
			a.knockX = 5.5f + step * 1.0f;
			a.launchY = 6.5f + step * 0.8f;
			a.airFloatOnHit = true;
		} else {
			a.duration = (step == 2) ? 0.52f : 0.44f;
			a.hitStart = 0.10f; a.hitEnd = 0.22f;
			a.chainOpen = 0.16f; a.chainClose = a.duration - 0.08f;
			a.knockX = 7.0f + step * 1.2f;
			a.launchY = 8.0f + step * 1.0f;
			a.airFloatOnHit = true;
		}
	}

	// =========================
  // ★ I / O の時間仕様を強制
  // I(Weak): 全体0.2秒、0.1秒で判定出て、0.2秒で消える
  // O(Strong): 全体0.5秒、0.3秒で判定出て、0.5秒で消える
  // =========================
	if (btn == AttackBtn::Weak) {        // I
		a.duration = 0.20f;
		a.hitStart = 0.10f;
		a.hitEnd = 0.20f;
		a.chainOpen = 0.12f;
		a.chainClose = 0.18f;

		a.hitZ = 0.6f;   // ★細め
	} else {                             // O
		a.duration = 0.50f;
		a.hitStart = 0.30f;
		a.hitEnd = 0.50f;
		a.chainOpen = 0.35f;
		a.chainClose = 0.48f;

		a.hitZ = 1.2f;   // ★広め
	}

	return a;
}

AABB2 PlayerCombo::MakeHitBox_(const Vector2& p, int facing, const AttackData& a) const {
	AABB2 hb{};
	hb.x = p.x + a.hbOffX * float(facing);
	hb.y = p.y + a.hbOffY;
	hb.hx = a.hbHalfX;
	hb.hy = a.hbHalfY;
	return hb;
}

AABB2 PlayerCombo::MakeEnemyBody2D_(const Enemy& e) const {
	// Enemy::GetBodyAABB() は 3D AABB（min/max）なので XY だけ使う
	AABB a3 = e.GetBodyAABB();

	float cx = (a3.min.x + a3.max.x) * 0.5f;
	float cy = (a3.min.y + a3.max.y) * 0.5f;
	float hx = (a3.max.x - a3.min.x) * 0.5f;
	float hy = (a3.max.y - a3.min.y) * 0.5f;

	AABB2 b{};
	b.x = cx; b.y = cy;
	b.hx = hx; b.hy = hy;
	return b;
}

void PlayerCombo::StartAttack_(bool airborne, AttackBtn btn, int dirY) {
	attacking_ = true;
	attackAir_ = airborne;
	step_ = 0;
	t_ = 0.0f;
	curBtn_ = btn;
	startDirY_ = dirY;
}

void PlayerCombo::NextStep_(bool airborne, AttackBtn btn) {
	step_ = std::min(step_ + 1, 2);
	attackAir_ = airborne;
	t_ = 0.0f;
	curBtn_ = btn;
}

void PlayerCombo::Update(float dt,
	const Input& in,
	Vector2& playerPos, Vector2& playerVel,
	bool onGround,
	int facing,
	float playerZ,            // ★追加
	EnemyManager& enemyMgr) {


	// ★毎フレーム最初に無効化
	debugHbValid_ = false;

	UpdateBuf_(dt);

	// I/O をバッファへ
	if (in.IsKeyTrigger(DIK_I)) Push_(AttackBtn::Weak);
	if (in.IsKeyTrigger(DIK_O)) Push_(AttackBtn::Strong);

	// 開始
	if (!attacking_) {
		AttackBtn b;
		if (Pop_(b)) {
			const bool airborne = !onGround;
			const int dirY = ReadDirY_(in);
			StartAttack_(airborne, b, dirY);
		}
		return;
	}

	const bool airborneNow = !onGround;
	const bool treatAir = attackAir_ || airborneNow;

	AttackData a = GetData_(treatAir, step_, curBtn_);
	t_ += dt;

	const bool hitActive = (t_ >= a.hitStart && t_ <= a.hitEnd);
	bool hittingNow = false;

	// ★敵配列を EnemyManager から取る
	auto& enemies = enemyMgr.GetEnemies();

	if (hitActive) {
		AABB2 hb = MakeHitBox_(playerPos, facing, a);

		// ★デバッグ可視化用に保存
		debugHb_ = hb;
		debugHbValid_ = true;

		for (auto& e : enemies) {
			if (!e.IsAlive()) continue;

			AABB2 body2 = MakeEnemyBody2D_(e);
			if (!Intersect(hb, body2)) continue;

			// ★ Z判定（奥行き）を追加：
// Enemyの3D AABBから「Z中心」と「Z半幅」を取り出す
			AABB a3 = e.GetBodyAABB();
			float ezC = (a3.min.z + a3.max.z) * 0.5f;
			float ehz = (a3.max.z - a3.min.z) * 0.5f;

			// 攻撃のZ半幅 a.hitZ と重なるかチェック
			if (std::abs(ezC - playerZ) > (a.hitZ + ehz)) continue;

			hittingNow = true;

			float knockX = a.knockX;
			float launchY = a.launchY;

			// ↑↔↓（開始時方向固定）
			if (startDirY_ > 0) {
				launchY *= 1.4f;
			} else if (startDirY_ < 0) {
				launchY *= 0.6f;
			}

			// ボス：ひるませない＆浮かせない（仕様）
			if (e.IsBoss()) {
				e.ApplyHit2D(0.0f, 0.0f, false);
			} else {
				e.ApplyHit2D(knockX * float(facing), launchY, true);
			}
		}
	}

	// 空中ヒット中だけ浮遊（プレイヤーだけ止める）
	if (!onGround && a.airFloatOnHit) {
		if (hittingNow) {
			playerVel.y = 0.0f;
		}
	}

	// チェイン窓
	if (t_ >= a.chainOpen && t_ <= a.chainClose) {
		AttackBtn next;
		if (Pop_(next)) {
			NextStep_(!onGround, next);
			return;
		}
	}

	if (t_ >= a.duration) {
		attacking_ = false;
		step_ = 0;
		t_ = 0.0f;
	}
}
