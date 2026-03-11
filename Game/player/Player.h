#pragma once
#include <memory>
#include "Vector3.h"
#include "AABB.h"
#include "Object3d.h"

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
	void Damage(int damage) { hp_ -= damage; if (hp_ < 0) hp_ = 0; }
	void TriggerHitFlash(float sec) { /* カードバトルなので一旦無視 */ }
	int GetHP() const { return hp_; }

	void Heal(int value) {
		hp_ += value;
		if (hp_ > maxHp_) {
			hp_ = maxHp_;
		}
	}
	int GetMaxHP() const { return maxHp_; }

private:
	std::unique_ptr<Object3d> model_;

	Vector3 pos_{ 0.0f, 0.0f, 0.0f };
	Vector3 rot_{ 0.0f, 0.0f, 0.0f };

	int hp_ = 100;
	int maxHp_ = 100;
	AABB body_{};
};