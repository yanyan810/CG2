#include "Enemy.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include <algorithm>
#include <cmath>

static bool IntersectXY(const AABB& a, const AABB& b) {
    // XY だけで重なり判定（Zは見た目だけ）
    if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
    if (a.max.y < b.min.y || a.min.y > b.max.y) return false;
    return true;
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
        hp_ = 9999;
        moveSpeed_ = 1.6f;
        meleeRange_ = 2.0f;
    } else if (type_ == EnemyType::Shooter) {
        hp_ = 2;
        moveSpeed_ = 2.2f;
    } else {
        hp_ = 3;
        moveSpeed_ = 2.6f;
    }

    model_ = std::make_unique<Object3d>();
    model_->Initialize(objCommon, dx);
    model_->SetCamera(cam);

    // 仮モデル（あるものに差し替えてOK）
    model_->SetModel("cube/cube.obj");

    UpdateBody_();
    UpdateModel_();
}

void Enemy::Update(float dt, const Vector2& playerXY, float playerZ) {
    if (!alive_) return;

    facing_ = (playerXY.x < pos_.x) ? -1 : +1;

    if (hitstunTime_ > 0.0f) {
        hitstunTime_ -= dt;
        if (hitstunTime_ <= 0.0f) hitstun_ = false;
    }

    if (meleeTimer_ > 0.0f) meleeTimer_ -= dt;
    if (shootTimer_ > 0.0f) shootTimer_ -= dt;

    if (!hitstun_ || type_ == EnemyType::Boss) {
        if (type_ == EnemyType::Melee) {
            UpdateAI_Melee_(dt, playerXY, playerZ);
        } else if (type_ == EnemyType::Shooter) {
            UpdateAI_Shooter_(dt, playerXY, playerZ);
        } else {
            UpdateAI_Boss_(dt, playerXY, playerZ);
        }
    }

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

    // ボス：浮かない＆ひるまない（仕様）
    if (type_ == EnemyType::Boss) {
        return r;
    }

    vel_.x = knockVx;
    vel_.y = launchVy;

    airborne_ = true;
    onGround_ = false;

    if (requestHitstun) {
        hitstun_ = true;
        hitstunTime_ = 0.20f;
    }

 /*   hp_ -= 1;
    if (hp_ <= 0) {
        alive_ = false;
        r.killed = true;
    }*/

    return r;
}

void Enemy::UpdateAI_Melee_(float, const Vector2& playerXY, float playerZ) {
    const float dx = playerXY.x - pos_.x;
    const float adx = std::abs(dx);

    if (adx > meleeRange_) {
        vel_.x = (dx > 0) ? moveSpeed_ : -moveSpeed_;
    } else {
        vel_.x = 0.0f;
        if (meleeTimer_ <= 0.0f) {
            meleeTimer_ = meleeCooldown_;
        }
    }

    // ★ Z追従（見た目用）
    const float dz = playerZ - pos_.z;
    const float adz = std::abs(dz);
    if (adz > zFollowDeadZone_) {
        vel_.z = (dz > 0) ? depthSpeed_ : -depthSpeed_;
    } else {
        vel_.z = 0.0f;
    }
}


void Enemy::UpdateAI_Shooter_(float, const Vector2& playerXY, float playerZ) {
    const float dx = playerXY.x - pos_.x;
    const float adx = std::abs(dx);

    if (adx < 3.0f) {
        vel_.x = (dx > 0) ? -moveSpeed_ : moveSpeed_;
    } else if (adx > 6.0f) {
        vel_.x = (dx > 0) ? moveSpeed_ : -moveSpeed_;
    } else {
        vel_.x = 0.0f;
    }

    if (shootTimer_ <= 0.0f) {
        shootTimer_ = shootCooldown_;
        // 弾生成は後で
    }

    // ★ Z追従（見た目用）
    const float dz = playerZ - pos_.z;
    const float adz = std::abs(dz);
    if (adz > zFollowDeadZone_) {
        vel_.z = (dz > 0) ? depthSpeed_ : -depthSpeed_;
    } else {
        vel_.z = 0.0f;
    }

}

void Enemy::UpdateAI_Boss_(float, const Vector2& playerXY, float playerZ) {
    const float dx = playerXY.x - pos_.x;
    const float adx = std::abs(dx);

    if (adx > 4.0f) {
        vel_.x = (dx > 0) ? moveSpeed_ : -moveSpeed_;
    } else {
        vel_.x = 0.0f;
    }

    // ★ Z追従（見た目用）
    const float dz = playerZ - pos_.z;
    const float adz = std::abs(dz);
    if (adz > zFollowDeadZone_) {
        vel_.z = (dz > 0) ? depthSpeed_ : -depthSpeed_;
    } else {
        vel_.z = 0.0f;
    }

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
    float hx = 0.4f, hy = 0.75f;
    if (type_ == EnemyType::Boss) { hx = 1.2f; hy = 2.0f; }

    body_.min = { pos_.x - hx, pos_.y,       0.0f };
    body_.max = { pos_.x + hx, pos_.y + hy * 2.0f, 0.0f };
}

void Enemy::UpdateModel_() {
    if (!model_) return;

    model_->SetTranslate({ pos_.x, pos_.y,  pos_.z });

    if (type_ == EnemyType::Boss) model_->SetScale({ 2.0f, 2.0f, 2.0f });
    else                         model_->SetScale({ 1.0f, 1.0f, 1.0f });

    model_->Update();
}

// -------- EnemyManager --------

void EnemyManager::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam) {
    objCommon_ = objCommon;
    dx_ = dx;
    cam_ = cam;
}

void EnemyManager::Clear() { enemies_.clear(); }

void EnemyManager::Spawn(EnemyType type, const Vector3& posXYZ) {
    Enemy e;
    e.Initialize(objCommon_, dx_, cam_, type, posXYZ);
    enemies_.push_back(std::move(e));
}

void EnemyManager::Update(float dt, const Vector2& playerXY, float playerZ) {
    for (auto& e : enemies_) e.Update(dt, playerXY, playerZ);

    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
        [](const Enemy& e) { return !e.IsAlive(); }), enemies_.end());
}


void EnemyManager::Draw() {
    for (auto& e : enemies_) e.Draw();
}
