#pragma once
#include "Matrix4x4.h"
#include <algorithm>
#include <cmath>
#include <dinput.h>

class Input;
class EnemyManager;
class Enemy;

struct AABB2 {
    float x = 0, y = 0;     // center
    float hx = 0.5f, hy = 0.5f;
};
inline bool Intersect(const AABB2& a, const AABB2& b) {
    return (std::abs(a.x - b.x) <= (a.hx + b.hx)) &&
        (std::abs(a.y - b.y) <= (a.hy + b.hy));
}

enum class AttackBtn { Weak, Strong };

enum class AttackType {
    None,
    I, // 速攻
    O, // 重攻
};

// 技データ（後でImGuiで調整しやすい形）
struct AttackData {
    float duration = 0.35f;
    float hitStart = 0.08f;
    float hitEnd = 0.18f;

    float chainOpen = 0.12f;
    float chainClose = 0.30f;

    float knockX = 6.0f;
    float launchY = 7.0f;

    float hbOffX = 0.9f;
    float hbOffY = 0.8f;
    float hbHalfX = 0.6f;
    float hbHalfY = 0.5f;

    float hitZ = 0.8f;

    bool airFloatOnHit = true; // 空中ヒット中浮遊
};

class PlayerCombo {
public:
    void Reset();

    void Start(AttackType type);

    // ★ Player::Update から呼ぶ（呼び出し側はそのまま enemyMgr を渡せる）
    void Update(float dt,
        const Input& in,
        Vector2& playerPos, Vector2& playerVel,
        bool onGround,
        int facing,
        float playerZ,            // ★追加
        EnemyManager& enemyMgr);

    bool IsAttacking() const { return attacking_; }

    // ★現在の攻撃の全体時間（秒）を取得（I/Oの強制仕様にも追従）
    float GetCurrentAttackDuration() const {
        return GetData_(attackAir_, step_, curBtn_).duration;
    }

    // ★攻撃開始前のプレビュー（空中/段数/ボタンに応じた全体時間）
    float PreviewAttackDuration(bool airborne, int step, AttackBtn btn) const {
        return GetData_(airborne, step, btn).duration;
    }

    bool GetDebugHitBox(AABB2& out) const {
        if (!debugHbValid_) return false;
        out = debugHb_;
        return true;
    }

private:
    // 入力バッファ
    struct BufItem { AttackBtn btn; float life; };
    std::vector<BufItem> buf_;
    float bufKeep_ = 0.25f;

    bool attacking_ = false;
    bool attackAir_ = false;
    int  step_ = 0;        // 0..2（3段）
    float t_ = 0.0f;
    AttackBtn curBtn_ = AttackBtn::Weak;

    // ★攻撃開始時に固定する方向（+1上 / 0横 / -1下）
    int startDirY_ = 0;

    // デバッグ可視化用（今フレームのヒットボックス）
    bool  debugHbValid_ = false;
    AABB2 debugHb_{};

    AttackType attackType_ = AttackType::None;

private:
    void Push_(AttackBtn b);
    bool Pop_(AttackBtn& out);
    void UpdateBuf_(float dt);

    int ReadDirY_(const Input& in) const;                 // ↑↔↓
    AttackData GetData_(bool airborne, int step, AttackBtn btn) const;
    AABB2 MakeHitBox_(const Vector2& p, int facing, const AttackData& a) const;

    // Enemy の AABB を 2D に変換
    AABB2 MakeEnemyBody2D_(const Enemy& e) const;

    void StartAttack_(bool airborne, AttackBtn btn, int dirY);
    void NextStep_(bool airborne, AttackBtn btn);
};
