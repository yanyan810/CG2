#include "Player.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"

void Player::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam) {
    model_ = std::make_unique<Object3d>();
    model_->Initialize(objCommon, dx);
    model_->SetCamera(cam);

    // 複雑なコンボモデルは捨てて、基本となるモデルを1つだけ読み込む
    model_->SetModel("enemy/boss/boss.gltf");
    model_->SetScale({ 0.5f, 0.5f, 0.5f });
    // 待機アニメーションをループ再生
    if (model_->HasAnimation()) {
        model_->PlayAnimation("Idle", true);
    }

    basePos_ = pos_;

    block_ = 0;
    boostedPower_ = 0;

    isAlive_ = true;
}

void Player::PlayAttackAnim(const Vector3& targetPos) {
    animState_ = AnimState::AttackForward;
    animTimer_ = 0.0f;
    animDuration_ = 0.2f; // 0.2秒で突進
    startPos_ = basePos_;
    // 相手の位置の少し手前まで移動
    targetPos_ = Lerp(basePos_, targetPos, 0.8f);
}

void Player::PlayDamageAnim() {
    animState_ = AnimState::Damage;
    animTimer_ = 0.0f;
    animDuration_ = 0.15f; // 0.15秒でのけぞる
    startPos_ = basePos_;
    // プレイヤーは左側にいる想定なので、左(Xのマイナス方向)に下がる
    targetPos_ = { basePos_.x - 2.0f, basePos_.y, basePos_.z };
}

void Player::Update(float dt) {
    // 動き（アニメーション）の計算
    if (animState_ != AnimState::Idle) {
        animTimer_ += dt;
        float t = animTimer_ / animDuration_;
        if (t > 1.0f) t = 1.0f;

        // イージング（後半ゆっくりになる計算）
        float easeT = t * (2.0f - t);
        pos_ = Lerp(startPos_, targetPos_, easeT);

        if (animTimer_ >= animDuration_) {
            if (animState_ == AnimState::AttackForward) {
                // 突進が終わったら戻る
                animState_ = AnimState::AttackReturn;
                animTimer_ = 0.0f;
                animDuration_ = 0.3f; // 戻る時は少しゆっくり
                startPos_ = pos_;
                targetPos_ = basePos_;
            } else if (animState_ == AnimState::AttackReturn || animState_ == AnimState::Damage) {
                // 戻り・ダメージ終了で待機状態へ
                animState_ = AnimState::Idle;
                pos_ = basePos_;
            }
        }
        if (trailInstance_) {
            if (animState_ == AnimState::AttackForward) {
                // 攻撃中：軌跡を出す
                trailInstance_->SetActive(true);
                // 毎フレーム新しい座標を覚えさせる
                trailInstance_->Update(GetWeaponTipPos(), GetWeaponBasePos(), trailConfig_);
            } else {
                // 攻撃終了後：SetActive(false) にすると TrailInstance 内で古い点から消えていく
                trailInstance_->SetActive(false);
                // isActive_がfalseの時も、Updateを呼ぶことで「古い点を消す」処理が進む
                trailInstance_->Update(GetWeaponTipPos(), GetWeaponBasePos(), trailConfig_);
            }
        }
    }

    // 赤点滅（フラッシュ）の計算
    if (flashTimer_ > 0.0f) {
        flashTimer_ -= dt;
        model_->SetMaterialColor({ 1.0f, 0.2f, 0.2f, 1.0f });
    } else {
        model_->SetMaterialColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    }
    if (model_) {
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
    basePos_ = p;
}

void Player::SetRotation(const Vector3& r) {
    rot_ = r;
}

void Player::Damage(int damage) 
{
    // ブロックの値が０じゃないならブロックの値分ダメージを減らす
    if (block_ > 0) {
        if (block_ >= damage) {
            // ブロックの方が大きい（全ダメージ防げる）場合
            block_ -= damage;
            damage = 0;
        } else {
            // ダメージの方が大きい（ブロックが壊れる）場合
            damage -= block_;
            block_ = 0;
        }
    }

    // ダメージが0以下ならダメージ処理なし
    if (damage <= 0) {
        return;
    }

    hp_ -= damage;

    if (hp_ < 0) {
        hp_ = 0;
        isAlive_ = false;
    }
}

void Player::DrawAnimationEditorImGui(Camera* editorCamera) {
    OutputDebugStringA("[AnimEditor] Player::DrawAnimationEditorImGui CALLED\n");

#ifdef USE_IMGUI
    if (!model_) {
        OutputDebugStringA("[AnimEditor] model_ is null in Player\n");
        return;
    }

    animationEditor_.DrawImGui(model_.get(), editorCamera);
#else
    OutputDebugStringA("[AnimEditor] USE_IMGUI OFF in Player\n");
#endif
}

Vector3 Player::GetWeaponTipPos()
{
    // 武器の根本（手元）の位置を基準にする
    Vector3 base = kWeaponOffset;

    // スイングの計算：突進中(AttackForward)に扇形に動かす
    float swingOffset = 0.0f;
    if (animState_ == AnimState::AttackForward) {
        float t = animTimer_ / animDuration_;
        // -1.5ラジアン(約-85度)から1.5ラジアン(約85度)まで回転させる
        swingOffset = -1.5f + (t * 3.0f);
    }

    // 剣先のローカル座標（Y軸周りに回転させて横振りを表現）
    Vector3 localTip = {
        sinf(swingOffset) * kWeaponLength,
        0.0f,
        cosf(swingOffset) * kWeaponLength
    };

    // キャラのワールド行列を使ってワールド座標に変換
    return Matrix4x4::TransformPos(base + localTip, model_->GetWorldMatrix());
}

Vector3 Player::GetWeaponBasePos()
{
    return Matrix4x4::TransformPos(kWeaponOffset, model_->GetWorldMatrix());
}
