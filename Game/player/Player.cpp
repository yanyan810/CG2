#include "Player.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"

#include "Enemy.h" // EnemyManager を使うため

// あなたのコンボヘッダに変更してください
#include "PlayerCombo.h"

static Vector4 Mul(const Matrix4x4& m, const Vector4& v);

void Player::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam) {
    cam_ = cam;

    model_ = std::make_unique<Object3d>();
    model_->Initialize(objCommon, dx);
    model_->SetCamera(cam_);

    // ★ここは「確実に存在するモデルパス」にしてください
    // 例：既に表示できたモデル（Resources配下など）
    model_->SetModel("cube/cube.obj");

    model_->SetTranslate({ pos_.x, pos_.y, 0.0f });
    //model_->Update();

    combo_ = std::make_unique<PlayerCombo>();
    combo_->Reset(); 
}

void Player::SetCamera(Camera* cam) {
    cam_ = cam;
    if (model_) model_->SetCamera(cam_);
}

void Player::Update(float dt, const Input& input, EnemyManager& enemyMgr) {
    // ★移動ロック更新
    if (moveLockSec_ > 0.0f) {
        moveLockSec_ -= dt;
        if (moveLockSec_ < 0.0f) moveLockSec_ = 0.0f;
    }

    // ★I / O 押下した瞬間に「攻撃全体時間」ぶん移動をロックする
    // （PlayerCombo::GetData_ の I/O 強制仕様に追従）
    if (combo_) {
        if (input.IsKeyTrigger(DIK_I)) {
            LockMove(combo_->PreviewAttackDuration(!onGround_, /*step=*/0, AttackBtn::Weak));
        }
        if (input.IsKeyTrigger(DIK_O)) {
            LockMove(combo_->PreviewAttackDuration(!onGround_, /*step=*/0, AttackBtn::Strong));
        }
    }

    // 1) 移動入力＆ジャンプ（ロック中は無視）
    if (!IsMoveLocked()) {
        UpdateMove_(dt, input);
    } else {
        // 入力移動を完全停止（落下やノックバックのYは残す）
        vel_.x = 0.0f;
        vel_.z = 0.0f;
    }

    // 2) 重力などの物理
    ApplyPhysics_(dt);

    // 3) コンボ（攻撃更新自体は止めない）
    if (combo_) {
        Vector2 p{ pos_.x, pos_.y };
        Vector2 v{ vel_.x, vel_.y };

        combo_->Update(dt, input, p, v, onGround_, facing_, pos_.z, enemyMgr);

        vel_.x = v.x;
        vel_.y = v.y;
        pos_.x = p.x;
        pos_.y = p.y;
    }

    // 4) 見た目更新
    UpdateModel_();
}


void Player::UpdateMove_(float /*dt*/, const Input& input) {
    // --- 左右（X） ---
    float mx = 0.0f;
    if (input.IsKeyPressed(DIK_LEFT) || input.IsKeyPressed(DIK_A))  mx -= 1.0f;
    if (input.IsKeyPressed(DIK_RIGHT) || input.IsKeyPressed(DIK_D))  mx += 1.0f;

    if (mx < -0.1f) facing_ = -1;
    if (mx > +0.1f) facing_ = +1;

    vel_.x = mx * moveSpeed_;

    // --- 奥行き（Z） ---
    float mz = 0.0f;
    if (input.IsKeyPressed(DIK_UP) || input.IsKeyPressed(DIK_W)) mz += 1.0f; // 奥へ +Z
    if (input.IsKeyPressed(DIK_DOWN) || input.IsKeyPressed(DIK_S)) mz -= 1.0f; // 手前へ -Z

    vel_.z = mz * depthSpeed_;

    // --- ジャンプ（Y） ---
    if (onGround_ && input.IsKeyTrigger(DIK_SPACE)) {
        onGround_ = false;
        vel_.y = jumpVel_;
        OutputDebugStringA("[Jump Trigger]\n");
    }
}

void Player::ApplyPhysics_(float dt) {
    // 重力（Yだけ）
    if (!onGround_) {
        vel_.y -= gravity_ * dt;
    }

    // 位置更新（X/Y/Z）
    pos_.x += vel_.x * dt;
    pos_.y += vel_.y * dt;
    pos_.z += vel_.z * dt;

    // 地面（y=0）
    if (pos_.y <= 0.0f) {
        pos_.y = 0.0f;
        vel_.y = 0.0f;
        onGround_ = true;
    }

    // 奥行き制限
    const float zNear = -10.0f; // 手前（DIK_DOWNで行く側）
    const float zFar = 20.0f; // 奥（DIK_UPで行く側）
    pos_.z = std::clamp(pos_.z, zNear, zFar);

    // ★ Zに応じて X の範囲を変える
    const float xMaxNear = 15.0f; // 手前での左右幅（狭く）
    const float xMaxFar = 20.0f; // 奥での左右幅（広く）

    float t = (pos_.z - zNear) / (zFar - zNear); // 0:手前 → 1:奥
    t = std::clamp(t, 0.0f, 1.0f);

    // 線形補間（Lerp）
    float xMax = xMaxNear + (xMaxFar - xMaxNear) * t;

    // X制限
    pos_.x = std::clamp(pos_.x, -xMax, xMax);

}


void Player::UpdateModel_() {
    if (!model_) return;

    model_->SetTranslate({ pos_.x, pos_.y, pos_.z });

    // 向き反転を見た目に反映したいなら、ScaleのXを反転させるなど（あなたのObject3d仕様次第）
    // 例：model_->SetScale({ (facing_>0)?1.0f:-1.0f, 1.0f, 1.0f });

    model_->Update();
}

void Player::Draw() {
    if (model_) model_->Draw();
}
