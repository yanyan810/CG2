#include "Player.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "GameApp.h"
#include "GeometryGenerator.h"
#include "ModelManager.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

namespace {
    Model::ModelData MakePrimitiveModelData(const std::vector<Model::VertexData>& vertices) {
        Model::ModelData modelData{};
        modelData.materials.push_back({ "" });

        Model::MeshData mesh{};
        mesh.materialIndex = 0;
        mesh.vertices = vertices;
        mesh.skinned = false;
        mesh.startVertex = 0;
        mesh.vertexCount = static_cast<uint32_t>(vertices.size());
        mesh.startIndex = 0;
        mesh.indexCount = static_cast<uint32_t>(vertices.size());
        modelData.meshes.push_back(std::move(mesh));

        modelData.indices.resize(vertices.size());
        for (uint32_t i = 0; i < static_cast<uint32_t>(vertices.size()); ++i) {
            modelData.indices[i] = i;
        }

        modelData.rootNode.name = "PlayerShieldHexRoot";
        modelData.rootNode.localMatrix = Matrix4x4::MakeIdentity4x4();
        modelData.rootNode.meshIndices.push_back(0);
        return modelData;
    }

    bool HasAnimationNamed(const Object3d* object, const char* name) {
        if (!object || !object->GetModel()) {
            return false;
        }

        const auto& animations = object->GetModel()->GetAnimations();
        return animations.find(name) != animations.end();
    }

    std::vector<Vector2> GenerateShieldHexOffsets(int requestedCount) {
        const int count = std::clamp(requestedCount, 1, 61);
        int radius = 0;
        while (1 + 3 * radius * (radius + 1) < count) {
            ++radius;
        }

        std::vector<Vector2> offsets;
        offsets.reserve(1 + 3 * radius * (radius + 1));
        for (int q = -radius; q <= radius; ++q) {
            for (int r = -radius; r <= radius; ++r) {
                const int s = -q - r;
                if (std::abs(s) > radius) {
                    continue;
                }
                offsets.push_back({
                    (static_cast<float>(q) + static_cast<float>(r) * 0.5f) * 0.52f,
                    static_cast<float>(r) * 0.45f
                    });
            }
        }

        std::sort(offsets.begin(), offsets.end(), [](const Vector2& a, const Vector2& b) {
            const float da = a.x * a.x + a.y * a.y;
            const float db = b.x * b.x + b.y * b.y;
            if (std::abs(da - db) > 0.0001f) {
                return da < db;
            }
            if (std::abs(a.y - b.y) > 0.0001f) {
                return a.y > b.y;
            }
            return a.x < b.x;
            });

        if (static_cast<int>(offsets.size()) > count) {
            offsets.resize(count);
        }
        return offsets;
    }
}

void Player::AddAttackMove(const AttackMove& move) {
    attackList_.push_back(move);

    if (move.effectJSON.empty()) {
        return;
    }

    EffectProfile profile;
    if (!EffectSequencer::LoadProfileCached(move.effectJSON, profile)) {
        return;
    }

    effectProfileCache_[move.effectJSON] = profile;
    if (!profile.projectile.modelPath.empty()) {
        ModelManager::GetInstance()->LoadModel(profile.projectile.modelPath);
    }
}

void Player::AddBlock(int value) {
    block_ = std::max(0, block_ + value);
    if (block_ > 0) {
        shieldBreakActive_ = false;
        shieldBreakCellCount_ = 0;
    }
}

void Player::DecayBlock(float decayRate) {
    if (block_ <= 0) {
        return;
    }

    decayRate = std::clamp(decayRate, 0.0f, 1.0f);
    if (decayRate <= 0.0f) {
        return;
    }

    const int reduce = std::clamp(
        static_cast<int>(std::ceil(static_cast<float>(block_) * decayRate)),
        1,
        block_
    );
    block_ -= reduce;
}

void Player::ResetBlock() {
    block_ = 0;
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
    InitializeShieldEffect_();

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

    // エフェクトプロファイルをJSONから読み込む
    auto cachedProfile = effectProfileCache_.find(move.effectJSON);
    if (!move.effectJSON.empty()) {
        if (cachedProfile != effectProfileCache_.end()) {
            // プロファイルに fireDelay などの設定が含まれていることを確認
            effectSequencer_.SetProfile(cachedProfile->second);
        } else {
            EffectProfile profile;
            if (EffectSequencer::LoadProfileCached(move.effectJSON, profile)) {
                effectProfileCache_[move.effectJSON] = profile;
                if (!profile.projectile.modelPath.empty()) {
                    ModelManager::GetInstance()->LoadModel(profile.projectile.modelPath);
                }
                effectSequencer_.SetProfile(profile);
            }
        }
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
    UpdatePowerBoostEffect_(dt);
    UpdateShieldEffect_(dt);

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
    DrawShield_(shieldColor_, 1.0f);

    // EffectSequencerの描画
    effectSequencer_.Draw();
}

void Player::DrawPostEffect(GameApp& app) {
    effectSequencer_.DrawPostEffect(app);
}

void Player::DrawShieldBloom(GameApp& app) {
    if (!ShouldDrawShield_() || shieldCells_.empty()) {
        return;
    }

    BloomParam param = app.ObjectPost()->GetParam();
    param.threshold = 0.0f;
    param.intensity = shieldBloomIntensity_;
    param.vignetteIntensity = 0.0f;
    param.vignetteScale = 0.0f;
    param.distortionAmount = 0.0f;
    param.chromAbAmount = shieldBloomChromAb_;
    param.isGrayscale = 0.0f;
    param.isInverted = 0.0f;
    param.noiseIntensity = 0.0f;
    param.scanlineIntensity = 0.0f;
    param.curvature = 0.0f;
    param.borderSharp = 0.0f;
    param.glitchAmount = 0.0f;
    param.dissolveAmount = -1.0f;

    app.ObjectPost()->SetParam(param);
    app.BeginObjectPostEffect();
    DrawShield_(shieldBloomColor_, shieldBloomScale_);
    app.EndObjectPostEffect();
    app.ObjCom()->SetGraphicsPipelineState();
}

void Player::DrawShieldImGui() {
#ifdef USE_IMGUI
    if (!ImGui::CollapsingHeader("Player Hex Shield", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::Checkbox("Preview Always", &shieldPreviewAlways_);
    ImGui::Text("Block: %d  Target Hexes: %d", block_, GetTargetShieldCellCount_());
    ImGui::SliderInt("Preview Hex Count", &shieldCellCount_, 1, 61);
    ImGui::DragFloat("Build Speed", &shieldBuildSpeed_, 0.1f, 1.0f, 80.0f);
    ImGui::DragFloat("Reduce Speed", &shieldReduceSpeed_, 0.1f, 1.0f, 80.0f);
    ImGui::DragFloat3("Offset", &shieldOffset_.x, 0.01f, -5.0f, 5.0f);
    ImGui::DragFloat3("Rotation", &shieldRotation_.x, 0.01f, -3.1416f, 3.1416f);
    ImGui::DragFloat("Base Scale", &shieldBaseScale_, 0.01f, 0.05f, 2.0f);
    ImGui::DragFloat("Spacing X", &shieldSpacingX_, 0.01f, 0.05f, 2.0f);
    ImGui::DragFloat("Spacing Y", &shieldSpacingY_, 0.01f, 0.05f, 2.0f);
    ImGui::DragFloat("Tilt Y", &shieldTiltY_, 0.01f, -1.5f, 1.5f);
    ImGui::DragFloat("Pulse Speed", &shieldPulseSpeed_, 0.05f, 0.0f, 20.0f);
    ImGui::DragFloat("Pulse Scale", &shieldPulseScale_, 0.005f, 0.0f, 0.3f);
    ImGui::ColorEdit4("Base Color", &shieldColor_.x);
    ImGui::ColorEdit4("Bloom Color", &shieldBloomColor_.x);
    ImGui::DragFloat("Bloom Scale", &shieldBloomScale_, 0.01f, 1.0f, 2.0f);
    ImGui::DragFloat("Bloom Intensity", &shieldBloomIntensity_, 0.01f, 0.0f, 5.0f);
    ImGui::DragFloat("Bloom ChromAb", &shieldBloomChromAb_, 0.0005f, 0.0f, 0.05f, "%.4f");
    ImGui::DragFloat("Break Duration", &shieldBreakDuration_, 0.01f, 0.1f, 3.0f);
    ImGui::DragFloat("Break Gravity", &shieldBreakGravity_, 0.1f, 0.0f, 30.0f);
    if (ImGui::Button("Test Shield Break")) {
        TriggerShieldBreak_(std::max(1, GetTargetShieldCellCount_()));
    }

    if (ImGui::Button("Reset Shield Params")) {
        shieldCellCount_ = 13;
        shieldBuildSpeed_ = 1.0f;
        shieldReduceSpeed_ = 22.0f;
        shieldOffset_ = { 2.05f, 1.35f, 0.10f };
        shieldRotation_ = { 0.0f, 1.5708f, 0.0f };
        shieldBaseScale_ = 0.46f;
        shieldSpacingX_ = 0.52f;
        shieldSpacingY_ = 0.45f;
        shieldTiltY_ = 0.16f;
        shieldPulseSpeed_ = 5.5f;
        shieldPulseScale_ = 0.05f;
        shieldColor_ = { 0.28f, 0.82f, 1.0f, 0.74f };
        shieldBloomColor_ = { 0.78f, 0.96f, 1.0f, 1.0f };
        shieldBloomScale_ = 1.08f;
        shieldBloomIntensity_ = 1.45f;
        shieldBloomChromAb_ = 0.002f;
        shieldBreakDuration_ = 0.85f;
        shieldBreakGravity_ = 7.5f;
    }

    if (ImGui::CollapsingHeader("Player Power Aura", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Power Aura Enabled", &powerBoostEffectEnabled_);
        ImGui::Text("Power: %d", boostedPower_);
        ImGui::DragFloat("Aura Base Rate", &powerBoostBaseRate_, 0.5f, 0.0f, 1000.0f);
        ImGui::DragFloat("Aura Rate Per Power", &powerBoostRatePerPower_, 0.1f, 0.0f, 20.0f);
        ImGui::DragFloat("Aura Radius", &powerBoostRadius_, 0.01f, 0.1f, 3.0f);
        ImGui::DragFloat("Aura Radius Per Power", &powerBoostRadiusPerPower_, 0.001f, 0.0f, 0.2f);
        ImGui::DragFloat("Aura Height", &powerBoostHeight_, 0.01f, 0.1f, 4.0f);
        ImGui::DragFloat("Aura Swirl Speed", &powerBoostSwirlSpeed_, 0.05f, 0.0f, 12.0f);
        ImGui::DragFloat("Aura Swirl Per Power", &powerBoostSwirlSpeedPerPower_, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Aura Vortex Angular", &powerBoostVortexAngularSpeed_, 0.05f, -20.0f, 20.0f);
        ImGui::DragFloat("Aura Vortex Angular Per Power", &powerBoostVortexAngularPerPower_, 0.01f, -3.0f, 3.0f);
        ImGui::DragFloat("Aura Vortex Radial", &powerBoostVortexRadialSpeed_, 0.005f, -2.0f, 2.0f);
        ImGui::DragFloat("Aura Vortex Radial Per Power", &powerBoostVortexRadialPerPower_, 0.001f, -0.2f, 0.2f);
        ImGui::DragFloat("Aura Up Speed", &powerBoostUpSpeed_, 0.05f, 0.0f, 6.0f);
        ImGui::DragFloat("Aura Up Speed Per Power", &powerBoostUpSpeedPerPower_, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Aura Start Scale", &powerBoostStartScale_, 0.001f, 0.01f, 0.5f);
        ImGui::DragFloat("Aura Scale Per Power", &powerBoostScalePerPower_, 0.001f, 0.0f, 0.08f);
        ImGui::DragFloat("Aura End Scale Rate", &powerBoostEndScaleRate_, 0.005f, 0.0f, 1.0f);
        ImGui::DragFloat("Aura Life Min", &powerBoostLifeMin_, 0.01f, 0.05f, 5.0f);
        ImGui::DragFloat("Aura Life Max", &powerBoostLifeMax_, 0.01f, 0.05f, 5.0f);
        ImGui::DragFloat("Aura Color Intensity", &powerBoostColorIntensity_, 0.01f, 0.0f, 5.0f);
        ImGui::Checkbox("Aura Billboard", &powerBoostBillboard_);
        ImGui::ColorEdit4("Aura Start Color", &powerBoostStartColor_.x);
        ImGui::ColorEdit4("Aura End Color", &powerBoostEndColor_.x);
        if (ImGui::Button("Test Power Aura Burst")) {
            EmitPowerBoostParticles_(24);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Aura Params")) {
            powerBoostBaseRate_ = 18.0f;
            powerBoostRatePerPower_ = 4.0f;
            powerBoostRadius_ = 0.75f;
            powerBoostRadiusPerPower_ = 0.035f;
            powerBoostHeight_ = 1.55f;
            powerBoostSwirlSpeed_ = 3.2f;
            powerBoostSwirlSpeedPerPower_ = 0.22f;
            powerBoostVortexAngularSpeed_ = 5.2f;
            powerBoostVortexAngularPerPower_ = 0.28f;
            powerBoostVortexRadialSpeed_ = -0.08f;
            powerBoostVortexRadialPerPower_ = -0.006f;
            powerBoostUpSpeed_ = 1.4f;
            powerBoostUpSpeedPerPower_ = 0.08f;
            powerBoostStartScale_ = 0.2f;
            powerBoostScalePerPower_ = 0.008f;
            powerBoostEndScaleRate_ = 0.12f;
            powerBoostLifeMin_ = 1.0f;
            powerBoostLifeMax_ = 1.6f;
            powerBoostColorIntensity_ = 1.0f;
            powerBoostBillboard_ = false;
            powerBoostStartColor_ = { 0.42f, 1.0f, 0.50f, 1.0f };
            powerBoostEndColor_ = { 0.45f, 1.0f, 0.70f, 0.0f };
        }
    }
#endif
}

void Player::SetSpawnPos(const Vector3& p) {
    pos_ = p;
    basePos_ = p;
}

void Player::SetRotation(const Vector3& r) {
    rot_ = r;
}

void Player::InitializeShieldEffect_() {
    const std::string modelKey = "PlayerShieldHexRing";
    shieldHexModel_ = ModelManager::GetInstance()->FindModel(modelKey);
    if (!shieldHexModel_) {
        auto vertices = GeometryGenerator::GenerateHexRingTriListXY(1.0f, 0.82f);
        shieldHexModel_ = ModelManager::GetInstance()->CreatePrimitiveModel(modelKey, MakePrimitiveModelData(vertices));
    }

    EnsureShieldCellCount_();
}

void Player::EnsureShieldCellCount_() {
    constexpr int kMaxShieldCellCount = 61;
    shieldCellCount_ = std::clamp(shieldCellCount_, 1, 61);
    while (static_cast<int>(shieldCells_.size()) < kMaxShieldCellCount) {
        auto cell = std::make_unique<Object3d>();
        cell->Initialize(objCommon_, dx_);
        cell->SetModel(shieldHexModel_);
        cell->SetCamera(camera_);
        cell->SetEnableLighting(0);
        cell->SetMaterialColor({ 0.28f, 0.82f, 1.0f, 0.74f });
        shieldCells_.push_back(std::move(cell));
    }
}

int Player::GetTargetShieldCellCount_() const {
    if (block_ > 0) {
        return std::clamp(block_, 1, 61);
    }
    if (shieldPreviewAlways_) {
        return std::clamp(shieldCellCount_, 1, 61);
    }
    return 0;
}

void Player::TriggerShieldBreak_(int cellCount) {
    EnsureShieldCellCount_();

    shieldBreakActive_ = true;
    shieldBreakTimer_ = 0.0f;
    shieldBreakCellCount_ = std::clamp(cellCount, 1, 61);
    shieldDisplayCount_ = 0.0f;
    shieldVisibleTimer_ = 1.0f;

    const std::vector<Vector2> hexOffsets = GenerateShieldHexOffsets(shieldBreakCellCount_);
    shieldBreakBasePositions_.resize(shieldBreakCellCount_);
    shieldBreakVelocities_.resize(shieldBreakCellCount_);
    shieldBreakRotations_.resize(shieldBreakCellCount_);
    shieldBreakAngularVelocities_.resize(shieldBreakCellCount_);

    const Vector3 center = {
        pos_.x + shieldOffset_.x,
        pos_.y + shieldOffset_.y,
        pos_.z + shieldOffset_.z
    };
    const Vector3 groupRotation = {
        shieldRotation_.x,
        shieldRotation_.y + shieldTiltY_ * std::sinf(shieldTimer_ * 1.8f),
        shieldRotation_.z
    };
    const Matrix4x4 groupRotationMatrix = Matrix4x4::RotateXYZ(
        groupRotation.x,
        groupRotation.y,
        groupRotation.z);

    for (int i = 0; i < shieldBreakCellCount_; ++i) {
        const Vector2& offset = hexOffsets[i];
        const Vector3 localOffset = {
            offset.x * shieldSpacingX_ / 0.52f,
            offset.y * shieldSpacingY_ / 0.45f,
            0.0f
        };
        const Vector3 rotatedOffset = Matrix4x4::TransformNormal(localOffset, groupRotationMatrix);
        shieldBreakBasePositions_[i] = {
            center.x + rotatedOffset.x,
            center.y + rotatedOffset.y,
            center.z + rotatedOffset.z
        };

        const float angle = static_cast<float>(i) * 1.37f;
        const float side = (i % 2 == 0) ? 1.0f : -1.0f;
        shieldBreakVelocities_[i] = {
            0.85f + std::cosf(angle) * 1.2f,
            2.1f + 0.18f * static_cast<float>(i % 5),
            std::sinf(angle) * 1.15f + side * 0.35f
        };
        shieldBreakRotations_[i] = groupRotation;
        shieldBreakAngularVelocities_[i] = {
            2.2f + 0.17f * static_cast<float>(i % 4),
            side * (3.0f + 0.11f * static_cast<float>(i % 7)),
            -1.7f + 0.19f * static_cast<float>(i % 6)
        };
    }
}

void Player::UpdateShieldEffect_(float dt) {
    EnsureShieldCellCount_();
    shieldTimer_ += dt;

    if (shieldBreakActive_) {
        shieldBreakTimer_ += dt;
        const float duration = std::max(0.1f, shieldBreakDuration_);
        const float life = std::clamp(1.0f - shieldBreakTimer_ / duration, 0.0f, 1.0f);

        for (int i = 0; i < shieldBreakCellCount_ && i < static_cast<int>(shieldCells_.size()); ++i) {
            Object3d* cell = shieldCells_[i].get();
            const Vector3& base = shieldBreakBasePositions_[i];
            const Vector3& velocity = shieldBreakVelocities_[i];
            const Vector3& baseRotation = shieldBreakRotations_[i];
            const Vector3& angularVelocity = shieldBreakAngularVelocities_[i];
            const float t = shieldBreakTimer_;
            cell->SetTranslate({
                base.x + velocity.x * t,
                base.y + velocity.y * t - 0.5f * shieldBreakGravity_ * t * t,
                base.z + velocity.z * t
                });
            cell->SetRotate({
                baseRotation.x + angularVelocity.x * t,
                baseRotation.y + angularVelocity.y * t,
                baseRotation.z + angularVelocity.z * t
                });
            const float scale = shieldBaseScale_ * (0.35f + 0.65f * life);
            cell->SetScale({ scale, scale, scale });
            cell->SetMaterialColor({
                shieldColor_.x,
                shieldColor_.y,
                shieldColor_.z,
                shieldColor_.w * life
                });
            cell->Update(dt);
        }

        if (shieldBreakTimer_ >= duration) {
            shieldBreakActive_ = false;
            shieldBreakCellCount_ = 0;
            shieldVisibleTimer_ = 0.0f;
        }
        return;
    }

    const int targetCellCount = GetTargetShieldCellCount_();
    if (shieldDisplayCount_ < static_cast<float>(targetCellCount)) {
        shieldDisplayCount_ = std::min(static_cast<float>(targetCellCount), shieldDisplayCount_ + shieldBuildSpeed_ * dt);
    } else if (shieldDisplayCount_ > static_cast<float>(targetCellCount)) {
        shieldDisplayCount_ = std::max(static_cast<float>(targetCellCount), shieldDisplayCount_ - shieldReduceSpeed_ * dt);
    }

    if (shieldDisplayCount_ > 0.01f || targetCellCount > 0) {
        shieldVisibleTimer_ = std::min(shieldVisibleTimer_ + dt * 8.0f, 1.0f);
    } else {
        shieldVisibleTimer_ = std::max(shieldVisibleTimer_ - dt * 6.0f, 0.0f);
    }

    const int activeCellCount = std::clamp(static_cast<int>(std::ceil(shieldDisplayCount_)), 0, 61);
    if (activeCellCount <= 0 || shieldVisibleTimer_ <= 0.0f || shieldCells_.empty()) {
        return;
    }

    const std::vector<Vector2> hexOffsets = GenerateShieldHexOffsets(activeCellCount);

    const float pulse = 0.5f + 0.5f * std::sinf(shieldTimer_ * shieldPulseSpeed_);
    const float globalAppear = shieldVisibleTimer_ * shieldVisibleTimer_ * (3.0f - 2.0f * shieldVisibleTimer_);
    const Vector3 center = {
        pos_.x + shieldOffset_.x,
        pos_.y + shieldOffset_.y,
        pos_.z + shieldOffset_.z
    };
    const Vector3 groupRotation = {
        shieldRotation_.x,
        shieldRotation_.y + shieldTiltY_ * std::sinf(shieldTimer_ * 1.8f),
        shieldRotation_.z
    };
    const Matrix4x4 groupRotationMatrix = Matrix4x4::RotateXYZ(
        groupRotation.x,
        groupRotation.y,
        groupRotation.z);

    for (int i = 0; i < activeCellCount && i < static_cast<int>(shieldCells_.size()) && i < static_cast<int>(hexOffsets.size()); ++i) {
        const Vector2& offset = hexOffsets[i];
        Object3d* cell = shieldCells_[i].get();
        const Vector3 localOffset = {
            offset.x * shieldSpacingX_ / 0.52f,
            offset.y * shieldSpacingY_ / 0.45f,
            0.0f
        };
        const Vector3 rotatedOffset = Matrix4x4::TransformNormal(localOffset, groupRotationMatrix);
        cell->SetTranslate({
            center.x + rotatedOffset.x,
            center.y + rotatedOffset.y,
            center.z + rotatedOffset.z
            });
        cell->SetRotate(groupRotation);
        const float cellAppearRaw = std::clamp(shieldDisplayCount_ - static_cast<float>(i), 0.0f, 1.0f);
        const float cellAppear = cellAppearRaw * cellAppearRaw * (3.0f - 2.0f * cellAppearRaw);
        const float baseScale = shieldBaseScale_ * globalAppear * cellAppear;
        const float cellPulse = 1.0f + shieldPulseScale_ * std::sinf(shieldTimer_ * shieldPulseSpeed_ + static_cast<float>(i) * 0.55f);
        cell->SetScale({ baseScale * cellPulse, baseScale * cellPulse, baseScale * cellPulse });
        cell->SetMaterialColor({
            shieldColor_.x * (0.75f + 0.25f * pulse),
            shieldColor_.y * (0.75f + 0.25f * pulse),
            shieldColor_.z,
            shieldColor_.w * cellAppear * (0.82f + 0.18f * pulse)
            });
        cell->Update(dt);
    }
}

bool Player::ShouldDrawShield_() const {
    return shieldBreakActive_ || shieldDisplayCount_ > 0.01f || GetTargetShieldCellCount_() > 0;
}

void Player::DrawShield_(const Vector4& color, float scaleMultiplier) {
    if (!ShouldDrawShield_() || shieldVisibleTimer_ <= 0.0f || shieldCells_.empty()) {
        return;
    }

    const float alpha = std::clamp(shieldVisibleTimer_, 0.0f, 1.0f);
    const int drawCount = shieldBreakActive_
        ? std::clamp(shieldBreakCellCount_, 0, static_cast<int>(shieldCells_.size()))
        : std::clamp(static_cast<int>(std::ceil(shieldDisplayCount_)), 0, static_cast<int>(shieldCells_.size()));
    for (int i = 0; i < drawCount; ++i) {
        auto& cell = shieldCells_[i];
        if (!cell) {
            continue;
        }
        const float cellAlpha = shieldBreakActive_
            ? std::clamp(1.0f - shieldBreakTimer_ / std::max(0.1f, shieldBreakDuration_), 0.0f, 1.0f)
            : std::clamp(shieldDisplayCount_ - static_cast<float>(i), 0.0f, 1.0f);
        if (cellAlpha <= 0.0f) {
            continue;
        }
        const Vector3 baseScale = cell->GetScale();
        const Vector4 drawColor = {
            color.x,
            color.y,
            color.z,
            color.w * alpha * cellAlpha
        };
        cell->SetMaterialColor(drawColor);
        cell->SetScale({
            baseScale.x * scaleMultiplier,
            baseScale.y * scaleMultiplier,
            baseScale.z * scaleMultiplier
            });
        cell->Update(0.0f);
        cell->Draw();
        cell->SetScale(baseScale);
        cell->Update(0.0f);
    }
}

void Player::Damage(int damage) 
{
    const int blockBefore = block_;

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

    if (blockBefore > 0 && block_ == 0) {
        const int visibleCells = std::max(blockBefore, static_cast<int>(std::ceil(shieldDisplayCount_)));
        TriggerShieldBreak_(visibleCells);
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

void Player::UpdatePowerBoostEffect_(float dt) {
    powerBoostEffectTimer_ += dt;

    if (!powerBoostEffectEnabled_ || !particleManager_ || boostedPower_ <= 0 || !isAlive_) {
        powerBoostEmitAccumulator_ = 0.0f;
        return;
    }

    const float power = static_cast<float>(std::max(0, boostedPower_));
    const float rate = std::clamp(powerBoostBaseRate_ + powerBoostRatePerPower_ * power, 0.0f, 120.0f);
    powerBoostEmitAccumulator_ += rate * dt;

    const uint32_t emitCount = static_cast<uint32_t>(std::min(10.0f, std::floor(powerBoostEmitAccumulator_)));
    if (emitCount == 0) {
        return;
    }

    powerBoostEmitAccumulator_ -= static_cast<float>(emitCount);
    EmitPowerBoostParticles_(emitCount);
}

void Player::EmitPowerBoostParticles_(uint32_t count) {
    if (!particleManager_ || count == 0) {
        return;
    }

    const float power = static_cast<float>(std::max(0, boostedPower_));
    const float power01 = std::clamp(power / 10.0f, 0.0f, 1.0f);
    const float radius = powerBoostRadius_ + powerBoostRadiusPerPower_ * power;
    const float spawnSwirlSpeed = powerBoostSwirlSpeed_ + powerBoostSwirlSpeedPerPower_ * power;
    const float upSpeed = powerBoostUpSpeed_ + powerBoostUpSpeedPerPower_ * power;
    const float vortexAngularSpeed = powerBoostVortexAngularSpeed_ + powerBoostVortexAngularPerPower_ * power;
    const float vortexRadialSpeed = powerBoostVortexRadialSpeed_ + powerBoostVortexRadialPerPower_ * power;
    const float startScale = std::clamp(powerBoostStartScale_ + powerBoostScalePerPower_ * power, 0.035f, 0.8f);
    const float lifeMin = std::max(0.05f, std::min(powerBoostLifeMin_, powerBoostLifeMax_));
    const float lifeMax = std::max(lifeMin, std::max(powerBoostLifeMin_, powerBoostLifeMax_));
    const float colorIntensity = std::max(0.0f, powerBoostColorIntensity_);

    std::vector<ModelParticleManager::Particle> particles;
    particles.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        const float randomAngle = Rand(0.0f, 6.2831853f);
        const float angle = randomAngle + powerBoostEffectTimer_ * spawnSwirlSpeed;
        const float ringRadius = radius * Rand(0.65f, 1.15f);
        const float height = Rand(0.05f, powerBoostHeight_);
        const float c = std::cosf(angle);
        const float s = std::sinf(angle);

        ModelParticleManager::Particle particle{};
        particle.transform.translate = {
            pos_.x + c * ringRadius,
            pos_.y + height,
            pos_.z + s * ringRadius
        };

        const Vector3 outward = { c, 0.0f, s };
        particle.velocity = {
            outward.x * Rand(-0.10f, 0.15f),
            upSpeed * Rand(0.65f, 1.25f),
            outward.z * Rand(-0.10f, 0.15f)
        };
        particle.acceleration = {
            pos_.x,
            0.45f + power01 * 0.35f,
            pos_.z
        };
        particle.vortexAngularSpeed = vortexAngularSpeed * Rand(0.82f, 1.18f);
        particle.vortexRadialSpeed = vortexRadialSpeed * Rand(0.75f, 1.25f);

        particle.transform.rotate = Rand(
            Vector3{ -3.1416f, -3.1416f, -3.1416f },
            Vector3{ 3.1416f, 3.1416f, 3.1416f });
        particle.angularVelocity = Rand(
            Vector3{ -5.0f, -7.0f, -5.0f },
            Vector3{ 5.0f, 7.0f, 5.0f });

        particle.lifeTime = Rand(lifeMin, lifeMax);
        particle.currentTime = 0.0f;
        particle.startScale = startScale * Rand(0.75f, 1.35f);
        particle.endScale = startScale * powerBoostEndScaleRate_ * Rand(0.75f, 1.25f);
        particle.startColor = {
            powerBoostStartColor_.x * colorIntensity,
            powerBoostStartColor_.y * colorIntensity,
            powerBoostStartColor_.z * colorIntensity,
            powerBoostStartColor_.w
        };
        particle.endColor = {
            powerBoostEndColor_.x * colorIntensity,
            powerBoostEndColor_.y * colorIntensity,
            powerBoostEndColor_.z * colorIntensity,
            powerBoostEndColor_.w
        };
        particle.color = particle.startColor;
        particle.easingType = EasingType::EaseOut;
        particle.isBillboard = powerBoostBillboard_;
        particles.push_back(particle);
    }

    particleManager_->EmitBatch(particles);
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
