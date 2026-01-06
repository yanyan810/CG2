#include "Player.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"

#include "Enemy.h" 

#include "PlayerCombo.h"

static const char* kWalkModels[] = {
    "Player/dush/dush.fbx",
    "Player/dush/dush2.obj",
    "Player/dush/dush3.obj",
    "Player/dush/dush4.obj",
    "Player/dush/dush5.obj",
};

static const char* kIAttackModels[] = {
    "Player/iAttak/attak1.fbx", // 1: 攻撃する前
    "Player/iAttak/attak2.fbx", // 2: 攻撃中
    "Player/iAttak/attak3.fbx", // 3: 攻撃中
};

static const char* kOAttackModels[] = {
    "Player/oAttak/oattak1.fbx", // 0: 攻撃前
    "Player/oAttak/oattak2.fbx", // 1: 攻撃中
    "Player/oAttak/oattak3.fbx", // 2: 攻撃中
    "Player/oAttak/oattak4.fbx", // 3: 攻撃中
    "Player/oAttak/oattak5.fbx", // 4: 攻撃後（または締め）
};


static Vector4 Mul(const Matrix4x4& m, const Vector4& v);

auto LogModel = [](const char* tag) {
    OutputDebugStringA(tag);
    OutputDebugStringA("\n");
    };


void Player::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam) {
    cam_ = cam;

    model_ = std::make_unique<Object3d>();
    model_->Initialize(objCommon, dx);
    model_->SetCamera(cam_);

    // 先に全歩きモデルをロード
    auto* mgr = ModelManager::GetInstance();
    for (auto& path : kWalkModels) {
        mgr->LoadModel(path);
    }

    // 最初のモデルをセット
    model_->SetModel(kWalkModels[0]);

    for (int i = 0; i < 5; i++) {
        walkModels_[i] = ModelManager::GetInstance()->FindModel(kWalkModels[i]);
        assert(walkModels_[i]); // ← ここで止まるならパスがおかしい
    }

    debugAtkCube_ = std::make_unique<Object3d>();
    debugAtkCube_->Initialize(objCommon, dx);
    debugAtkCube_->SetCamera(cam_);
   // debugAtkCube_->SetModel("cube/cube.obj");

    debugEnemyCube_ = std::make_unique<Object3d>();
    debugEnemyCube_->Initialize(objCommon, dx);
    debugEnemyCube_->SetCamera(cam_);
   // debugEnemyCube_->SetModel("cube/cube.obj");

    model_->SetTranslate({ pos_.x, pos_.y, 0.0f });
    //model_->Update();

    combo_ = std::make_unique<PlayerCombo>();
    combo_->Reset(); 

    if (model_) {
        model_->SetMaterialColor(normalColor_);
    }

    // 先に全I攻撃モデルをロード
    for (auto& path : kIAttackModels) {
        mgr->LoadModel(path);
    }

    for (int i = 0; i < 3; i++) {
        iAtkModels_[i] = ModelManager::GetInstance()->FindModel(kIAttackModels[i]);
        assert(iAtkModels_[i]);
    }

    // 先に全O攻撃モデルをロード
    for (auto& path : kOAttackModels) {
        mgr->LoadModel(path);
    }
    for (int i = 0; i < 5; i++) {
        oAtkModels_[i] = ModelManager::GetInstance()->FindModel(kOAttackModels[i]);
        assert(oAtkModels_[i]);
    }

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
    } else {
        // combo_ を使わない場合の保険（必要なら残す）
        // if (input.IsKeyTrigger(DIK_I)) LockMove(0.20f);
        // if (input.IsKeyTrigger(DIK_O)) LockMove(0.50f);
    }

    // --- ジャンプ（Y） ---
    if (onGround_ && input.IsKeyTrigger(DIK_SPACE)) {
        onGround_ = false;
        vel_.y = jumpVel_;
        OutputDebugStringA("[Jump Trigger]\n");
    }

    // 1) 移動入力（ロック中は無視）
    if (!IsMoveLocked()) {
        UpdateMove_(dt, input);
    } else {
        vel_.x = 0.0f;
        vel_.z = 0.0f;
    }

    // 3) コンボ
    if (combo_) {
        Vector2 p{ pos_.x, pos_.y };
        Vector2 v{ vel_.x, vel_.y };

        combo_->Update(dt, input, p, v, onGround_, facing_, pos_.z, enemyMgr);

        // comboが速度を書き換える場合は戻す（空中ヒット中浮遊など）
        vel_.x = v.x;
        vel_.y = v.y;

        // comboが位置も動かす設計なら pos_ も戻す
        pos_.x = p.x;
        pos_.y = p.y;
    }

    // ★ ここから下は「ロック中でも毎フレーム」実行する
    bool usedAttackModel = false;

    // ===== 攻撃アニメ優先 =====
    if (combo_) {
        PlayerCombo::AttackAnimState st{};
        if (combo_->GetAnimState(st)) {

            char buf[128];
            sprintf_s(buf, "[AnimState] btn=%d step=%d t=%.3f\n",
                (int)st.btn, st.step, st.t);
            OutputDebugStringA(buf);
            // -------------------------
            // I（Weak）: 3枚
            // -------------------------
            if (st.btn == AttackBtn::Weak) {

                LogModel("[SetModel] I");

                if (st.t < st.data.hitStart) {
                    model_->SetModel(iAtkModels_[0]);
                    iAtkAnimTime_ = 0.0f;
                } else if (st.t <= st.data.hitEnd) {
                    iAtkAnimTime_ += dt;
                    int f = static_cast<int>(iAtkAnimTime_ * kIAttackFps_) % 2;
                    model_->SetModel(iAtkModels_[1 + f]);
                } else {
                    model_->SetModel(iAtkModels_[2]);
                }

                usedAttackModel = true;
            }
            // -------------------------
            // O（Strong）: 5枚
            // -------------------------
            else if (st.btn == AttackBtn::Strong) {

                LogModel("[SetModel] O");

                if (st.t < st.data.hitStart) {
                    model_->SetModel(oAtkModels_[0]);
                    oAtkAnimTime_ = 0.0f;
                } else if (st.t <= st.data.hitEnd) {
                    oAtkAnimTime_ += dt;
                    int f = static_cast<int>(oAtkAnimTime_ * kOAttackFps_) % 3;
                    model_->SetModel(oAtkModels_[1 + f]);
                } else {
                    model_->SetModel(oAtkModels_[4]);
                }

                usedAttackModel = true;
            }
        } else {


            OutputDebugStringA("[AnimState] false -> walk\n");
        }
    }

    bool justPressedAttack =
        input.IsKeyTrigger(DIK_I) || input.IsKeyTrigger(DIK_O);

    // 攻撃モデルを使ってない時だけ歩き
    if (!usedAttackModel) {

        if (!justPressedAttack) {

            LogModel("[SetModel] WALK");

            const bool moving =
                (std::abs(vel_.x) > 0.01f) ||
                (std::abs(vel_.z) > 0.01f);

            if (moving) {
                walkAnimTime_ += dt;
                int frame = static_cast<int>(walkAnimTime_ * kWalkFps_) % kWalkFrameCount_;
                model_->SetModel(walkModels_[frame]);
            } else {
                walkAnimTime_ = 0.0f;
                model_->SetModel(walkModels_[0]);
            }
        }
    }


    // ★被弾フラッシュ更新
    if (hitFlashSec_ > 0.0f) {
        hitFlashSec_ -= dt;
        if (hitFlashSec_ < 0.0f) hitFlashSec_ = 0.0f;
    }

    // ★色反映（毎フレームでOK）
    if (model_) {
        if (hitFlashSec_ > 0.0f) {
            model_->SetMaterialColor(hitColor_);
        } else {
            model_->SetMaterialColor(normalColor_);
        }
    }


    // 2) 重力などの物理
    ApplyPhysics_(dt);

    UpdateBody_();

    // 4) 見た目更新（Z固定）
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
    const float zNear = -15.0f; // 手前（DIK_DOWNで行く側）
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

void Player::Damage(int d) {
    if (dead_) return;
    hp_ -= d;
    if (hp_ <= 0) {
        hp_ = 0;
        dead_ = true;
    }
}

void Player::SetSpawnPos(const Vector3& p) {
    pos_ = p;
    vel_ = { 0,0,0 };
    onGround_ = true;

    UpdateBody_();
    UpdateModel_(); // 見た目も即反映
}

void Player::UpdateModel_() {
    if (!model_) return;

    // 位置
    model_->SetTranslate({ pos_.x, pos_.y, pos_.z });

    // ★向き反転（Xスケールを反転）
    const float sx = (facing_ > 0) ? 1.0f : -1.0f;
    model_->SetScale({ sx, 1.0f, 1.0f });

    model_->Update();
}


void Player::Draw() {

    if (model_) model_->Draw();
}

void Player::DrawDebugHitBoxes(EnemyManager& enemyMgr) {
    if (!combo_ || !debugAtkCube_) return;

    AABB3 hb{};
    if (combo_->GetDebugHitBox3(hb)) {
        // hb は center + half なので、そのまま
        debugAtkCube_->SetTranslate({ hb.x, hb.y, hb.z });

        // cube.obj は元が 2x2x2（-1..+1）
        // → scale = halfSize をそのまま入れれば一致
        debugAtkCube_->SetScale({ hb.hx, hb.hy, hb.hz });

        debugAtkCube_->Update();
        debugAtkCube_->Draw();
    }
}


void Player::UpdateBody_() {
    // ここはあなたの見た目サイズに合わせて調整
    const float hx = 0.4f;
    const float hy = 0.9f;
    const float hz = 0.6f;

    body_.min = { pos_.x - hx, pos_.y,         pos_.z - hz };
    body_.max = { pos_.x + hx, pos_.y + hy * 2,  pos_.z + hz };
}

void Player::AddHP(int heal) {
	hp_ += heal;
	if (hp_ > GetMaxHP()) hp_ = GetMaxHP();
}