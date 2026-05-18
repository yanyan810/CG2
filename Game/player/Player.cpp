#include "Player.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include <array>
#include <random>
#include <vector>

namespace {
    bool HasAnimationNamed(const Object3d* object, const char* name) {
        if (!object || !object->GetModel()) {
            return false;
        }

        const auto& animations = object->GetModel()->GetAnimations();
        return animations.find(name) != animations.end();
    }
}

void Player::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam,
    ModelParticleManager* particleMgr, TrailManager* trailMgr) {
    // 参照を保持（EffectSequencer初期化用）
    objCommon_ = objCommon;
    dx_ = dx;
    camera_ = cam;
    trailMgr_ = trailMgr;

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

    particleManager_ = particleMgr ? particleMgr : ModelParticleManager::GetInstance();

    // EffectSequencerの初期化
    effectSequencer_.Initialize(objCommon, dx, cam, particleManager_, trailMgr);

    // 攻撃エフェクト関連の初期化
    attackEffectTimer_ = 0.0f;
    effectFired_ = false;
    currentAttackIndex_ = -1;
}

void Player::PlayAttackAnim(const Vector3& targetPos) {
    animState_ = AnimState::AttackForward;
    animTimer_ = 0.0f;
    animDuration_ = 0.2f; // 0.2秒で突進
    startPos_ = basePos_;
    // 相手の位置の少し手前まで移動
    targetPos_ = Lerp(basePos_, targetPos, 0.8f);

    if (releaseAnimationEnabled_) {
        PlayRandomReleaseAttackAnimation_();
    }
}

void Player::PlayAttackAnimWithEffect(const Vector3& targetPos, int moveIndex) {
    if (attackList_.empty()) return;

    animState_ = AnimState::AttackForward;
    animTimer_ = 0.0f;
    animDuration_ = 0.2f; // 0.2秒で突進
    startPos_ = basePos_;
    // 相手の位置の少し手前まで移動
    targetPos_ = Lerp(basePos_, targetPos, 0.8f);

    // --- ランダム選択のロジックを追加 ---
    int index = moveIndex;
    // indexが-1、または範囲外の場合はランダムに決定する
    if (index < 0 || index >= static_cast<int>(attackList_.size())) {
        index = Rand(0, static_cast<int>(attackList_.size()) - 1);
    }

    // エフェクト連動のセットアップ
    attackTargetPos_ = targetPos;
    attackEffectTimer_ = 0.0f;
    effectFired_ = false;
    currentAttackIndex_ = index; // 決定したインデックスを保持

    // 選択された技のデータを取得
    const AttackMove& move = attackList_[index];

    // エフェクトプロファイルをJSONから読み込む（同じファイルならスキップ）
    if (!move.effectJSON.empty()) {
        if (move.effectJSON != cachedEffectJSONPath_) {
            EffectProfile profile;
            if (EffectSequencer::LoadProfile(move.effectJSON, profile)) {
                effectSequencer_.SetProfile(profile);
                cachedEffectJSONPath_ = move.effectJSON;
            }
        }
        // 同じJSONなら既にSetProfile済みなのでそのまま使う
    }

    // アニメーションの再生
    if (!move.animationName.empty() && model_) {
        // model_内のアニメーション存在チェックを簡略化して確実に再生
        model_->PlayAnimation(move.animationName, false);
        releaseAttackAnimationPlaying_ = releaseAnimationEnabled_;
    }
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
                // 攻撃終了時にエフェクト関連もリセット
                currentAttackIndex_ = -1;
            }
        }
        if (trailInstance_) {
            if (animState_ == AnimState::AttackForward) {
                // 攻撃中：軌跡を出す
                trailInstance_->SetActive(true);
                // 毎フレーム新しい座標を覚えさせる
                trailInstance_->Update(dt, GetWeaponTipPos(), GetWeaponBasePos(), trailConfig_);

            } else {
                // 攻撃終了後：SetActive(false) にすると TrailInstance 内で古い点から消えていく
                trailInstance_->SetActive(false);
                // isActive_がfalseの時も、Updateを呼ぶことで「古い点を消す」処理が進む
                trailInstance_->Update(dt, GetWeaponTipPos(), GetWeaponBasePos(), trailConfig_);
            }
        }
    }

    // === 攻撃エフェクトの遅延発射処理 ===
    if (currentAttackIndex_ >= 0 && currentAttackIndex_ < static_cast<int>(attackList_.size())) {
        attackEffectTimer_ += dt;

        const AttackMove& move = attackList_[currentAttackIndex_];

        // fireDelayを超えた瞬間に一度だけFire
        if (!effectFired_ && attackEffectTimer_ >= move.fireDelay) {
            effectFired_ = true;

            // 発射座標：プレイヤーの手の位置（前方1.0m程度）
            Vector3 fireStartPos = pos_ + Vector3{ 0.0f, 1.2f, 1.0f };
            // ターゲット座標：攻撃対象の座標（いなければ前方5.0m程度）
            Vector3 fireTargetPos = attackTargetPos_;
            if (fireTargetPos.x == 0.0f && fireTargetPos.y == 0.0f && fireTargetPos.z == 0.0f) {
                // ターゲットがない場合は前方5.0m
                fireTargetPos = pos_ + Vector3{ 0.0f, 1.0f, 5.0f };
            }

            effectSequencer_.Fire(effectSequencer_.GetProfile(), fireStartPos, fireTargetPos);
        }
    }

    // EffectSequencerの更新
    effectSequencer_.Update(dt);

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

        if (releaseAnimationEnabled_ && releaseAttackAnimationPlaying_ && model_->IsAnimationFinished()) {
            PlayReleaseIdleAnimation_();
        }
    }

    // 敵の弾などが当たったか判定するための箱（AABB）を自分の位置に合わせて更新
    body_.min = { pos_.x - 1.0f, pos_.y, pos_.z - 1.0f };
    body_.max = { pos_.x + 1.0f, pos_.y + 2.0f, pos_.z + 1.0f };
    
}

void Player::PlayReleaseIdleAnimation_() {
    if (!HasAnimationNamed(model_.get(), "CustomAnim")) {
        releaseAttackAnimationPlaying_ = false;
        return;
    }

    model_->PlayAnimation("CustomAnim", true);
    releaseAttackAnimationPlaying_ = false;
}

void Player::PlayRandomReleaseAttackAnimation_() {
    if (!model_) {
        return;
    }

    static std::mt19937 rng(std::random_device{}());
    static const std::array<const char*, 3> kAttackAnimations = {
        "CustomAnim_attack_1",
        "CustomAnim_attack_2",
        "CustomAnim_attack_3",
    };

    std::vector<const char*> availableAnimations;
    availableAnimations.reserve(kAttackAnimations.size());
    for (const char* name : kAttackAnimations) {
        if (HasAnimationNamed(model_.get(), name)) {
            availableAnimations.push_back(name);
        }
    }

    if (availableAnimations.empty()) {
        releaseAttackAnimationPlaying_ = false;
        return;
    }

    std::uniform_int_distribution<size_t> dist(0, availableAnimations.size() - 1);
    model_->PlayAnimation(availableAnimations[dist(rng)], false);
    releaseAttackAnimationPlaying_ = true;
}

void Player::PlayReleaseDamageAnimation_() {
    if (!model_) {
        return;
    }

    const char* animationName =
        (hp_ > (maxHp_ / 2))
        ? "CustomAnim_attack_received_1"
        : "CustomAnim_attack_received_2";

    if (!HasAnimationNamed(model_.get(), animationName)) {
        releaseAttackAnimationPlaying_ = false;
        return;
    }

    model_->PlayAnimation(animationName, false);
    releaseAttackAnimationPlaying_ = true;
}

void Player::Draw() {
    if (model_&&isAlive_) {
        model_->Draw();
    }

    // EffectSequencerの描画
    effectSequencer_.Draw();
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

    if (hp_ <= 0) {
        hp_ = 0;
        isAlive_ = false;
    }

    if (releaseAnimationEnabled_) {
        PlayReleaseDamageAnimation_();
    }
}

Vector3 Player::GetWeaponTipPos() {
    // 高め（頭より上くらいまで上げる）
    Vector3 baseOffset = { 0.0f, 2.5f, 0.0f };
    float length = 6.0f;

    float t = (animState_ == AnimState::AttackForward) ? (animTimer_ / animDuration_) : 1.0f;
    float angle = -1.2f + (t * 2.4f);

    Vector3 localTip = {
        cosf(angle) * length,
        baseOffset.y + sinf(angle) * 1.5f, // 少し斜めに振る
        sinf(angle) * length
    };
    return Matrix4x4::TransformPos(localTip, model_->GetWorldMatrix());
}

Vector3 Player::GetWeaponBasePos() {
    // 低め（膝くらいの高さまで下げる）
    // TipとBaseのY座標の差が「軌跡の太さ」になります！
    Vector3 baseOffset = { 0.0f, 0.5f, 0.0f };
    float length = 3.0f; // 根元も少し外側に広げるとさらに太く見える

    float t = (animState_ == AnimState::AttackForward) ? (animTimer_ / animDuration_) : 1.0f;
    float angle = -1.2f + (t * 2.4f);

    Vector3 localBase = {
        cosf(angle) * length,
        baseOffset.y,
        sinf(angle) * length
    };
    return Matrix4x4::TransformPos(localBase, model_->GetWorldMatrix());
}
