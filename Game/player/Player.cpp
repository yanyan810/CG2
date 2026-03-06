#include "Player.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"

void Player::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam) {
    model_ = std::make_unique<Object3d>();
    model_->Initialize(objCommon, dx);
    model_->SetCamera(cam);

    // 複雑なコンボモデルは捨てて、基本となるモデルを1つだけ読み込む
    model_->SetModel("human/walk.gltf");

    // 待機アニメーションをループ再生
    if (model_->HasAnimation()) {
        model_->PlayAnimation("Idle", true);
    }
}

void Player::Update(float dt) {
    if (model_) {
        // ★ ここで毎フレーム「指定した座標」と「回転」を確実に適用する
        // これで謎の力によって座標がズレることを完全に防ぎます
        model_->SetTranslate(pos_);
        model_->SetRotate(rot_);

        // アニメーションの更新
        model_->Update(dt);
    }

    // 敵の弾などが当たったか判定するための箱（AABB）を自分の位置に合わせて更新
    body_.min = { pos_.x - 1.0f, pos_.y, pos_.z - 1.0f };
    body_.max = { pos_.x + 1.0f, pos_.y + 2.0f, pos_.z + 1.0f };
}

void Player::Draw() {
    if (model_) {
        model_->Draw();
    }
}

void Player::SetSpawnPos(const Vector3& p) {
    pos_ = p;
}

void Player::SetRotation(const Vector3& r) {
    rot_ = r;
}