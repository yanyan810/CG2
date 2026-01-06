#pragma once
#include <memory>

#include "AABB.h"   // Vector2 / Vector3 が入っている想定
#include "Input.h"
#include "Model.h"
#include "ModelManager.h"

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

    enum class ModelId { Walk, I0, I1, I2, O0, O1, O2 /*など*/ };


    void Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam);
    void SetCamera(Camera* cam);

    void Update(float dt, const Input& input, EnemyManager& enemyMgr);
    void Draw();
    void DrawDebugHitBoxes(EnemyManager& enemyMgr);

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

    void ClampToScreenX_(const Camera& cam, float marginNdc /*例:0.08f*/);

    AABB GetBodyAABB() const { return body_; }

    void TriggerHitFlash(float sec = 0.20f) { hitFlashSec_ = std::max(hitFlashSec_, sec); }

    void AddHP(int heal);

    void Damage(int Damage);

    float GetX() const { return pos_.x; }

	int GetHP() const { return hp_; }
	int GetMaxHP() const { return 100; } // 固定値で良ければ

    void SetSpawnPos(const Vector3& p);

    bool IsDead() const { return dead_; }

private:
    void UpdateMove_(float dt, const Input& input);
    void ApplyPhysics_(float dt);
    void UpdateModel_();

    void UpdateBody_();

    void ApplyModel_(ModelId want);

    void SetModelIfChanged_(void* asset, const char* logTag);

private:
    
    ModelId currentModel_ = ModelId::Walk; // 今表示中

    // 見た目
    std::unique_ptr<Object3d> model_;
    std::unique_ptr<Object3d> debugAtkCube_;   // 攻撃HB用
    std::unique_ptr<Object3d> debugEnemyCube_; // 敵AABB用
    Camera* cam_ = nullptr;

    // 内部は3Dで持つが、Zは固定（見た目だけ）
    Vector3 pos_{ 0.0f, 0.0f, 15.0f };
    Vector3 vel_{ 0.0f, 0.0f, 0.0f };

    bool onGround_ = true;
    int  facing_ = +1; // +1:right / -1:left

    // ★移動ロック（秒）: >0 の間は移動入力を無視する
    float moveLockSec_ = 0.0f;

    // パラメータ
    float moveSpeed_ = 10.0f;
    float depthSpeed_ = 14.0f;
    float jumpVel_ = 12.0f;
    float gravity_ = 25.0f;
    float zView_ = 15.0f;

    int hp_ = 100;

    // コンボ（あなたの既存クラスに差し替える）
    std::unique_ptr<PlayerCombo> combo_;

    AABB body_{};

    float hitFlashSec_ = 0.0f;
    Vector4 normalColor_{ 1,1,1,1 };
    Vector4 hitColor_{ 1,0.2f,0.2f,1 }; // 赤っぽく

    // 歩き（OBJ差し替え）アニメ
    float walkAnimTime_ = 0.0f;
    static constexpr int   kWalkFrameCount_ = 5;
    static constexpr float kWalkFps_ = 10.0f; // 1秒に10コマ → 5枚なら0.5秒で一周（好みで）

    Model* walkModels_[5];

    Model* iAtkModels_[3] = { nullptr, nullptr, nullptr };
    float iAtkAnimTime_ = 0.0f;
    static constexpr float kIAttackFps_ = 12.0f; // 2↔3の切り替え速度（好み）

    Model* oAtkModels_[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    float oAtkAnimTime_ = 0.0f;
    static constexpr int   kOFrameCount_ = 5;
    static constexpr float kOAttackFps_ = 12.0f; // 好みで（強攻撃っぽくなら 8〜12 くらい）

    bool dead_ = false;
 
};