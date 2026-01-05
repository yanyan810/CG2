#pragma once
#include <memory>

#include "Vector3.h"   // Vector2 / Vector3 が入っている想定
#include "Input.h"

class Object3d;
class Object3dCommon;
class DirectXCommon;
class Camera;

class EnemyManager;

// あなたが既に作ったコンボクラスに差し替えてOK
// 例: #include "PlayerCombo.h"
class PlayerCombo;

class Player {
public:
    void Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam);
    void SetCamera(Camera* cam);

    void Update(float dt, const Input& input, EnemyManager& enemyMgr);
    void Draw();

    // ★I/O 攻撃中など「一定時間移動不可」にする
    void LockMove(float sec) { if (sec > moveLockSec_) moveLockSec_ = sec; }
    bool IsMoveLocked() const { return moveLockSec_ > 0.0f; }


    Vector2 GetPos2D() const { return { pos_.x, pos_.y }; }
    Vector2 GetVel2D() const { return { vel_.x, vel_.y }; }

    float GetZ() const { return pos_.z; }
    Vector3 GetPos3D() const { return pos_; } // 使いたければ

    bool IsOnGround() const { return onGround_; }
    int  GetFacing() const { return facing_; }

    // デバッグ可視化用（ヒットボックス取り出しに使える）
    PlayerCombo* GetCombo() { return combo_.get(); }
    const PlayerCombo* GetCombo() const { return combo_.get(); }

    //void ClampToScreenX_(const Camera& cam, float marginNdc /*例:0.08f*/);

    void SetMoveLock(int frames) { moveLockTimer_ = (std::max)(moveLockTimer_, frames); }
   //6 bool IsMoveLocked() const { return moveLockTimer_ > 0; }

private:
    void UpdateMove_(float dt, const Input& input);
    void ApplyPhysics_(float dt);
    void UpdateModel_();

private:
    // 見た目
    std::unique_ptr<Object3d> model_;
    Camera* cam_ = nullptr;

    // 内部は3Dで持つが、Zは固定（見た目だけ）
    Vector3 pos_{ 0.0f, 0.0f, 15.0f };
    Vector3 vel_{ 0.0f, 0.0f, 0.0f };

    bool onGround_ = true;
    int  facing_ = +1; // +1:right / -1:left

    // パラメータ
    float moveSpeed_ = 12.0f;
    float depthSpeed_ = 16.0f;
    float jumpVel_ = 12.0f;
    float gravity_ = 25.0f;
    float zView_ = 15.0f;

    int moveLockTimer_ = 0;

    // ★移動ロック（秒）: >0 の間は移動入力を無視する
    float moveLockSec_ = 0.0f;

    // コンボ（あなたの既存クラスに差し替える）
    std::unique_ptr<PlayerCombo> combo_;
};
