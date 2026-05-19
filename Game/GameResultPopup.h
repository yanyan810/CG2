#pragma once
#include <memory>
#include <string>
#include "Sprite.h"
#include "Matrix4x4.h"
#include "MathStruct.h"
class GameApp;

// ポップアップの種類
enum class ResultKind  { GameOver, GameClear };

// 選択されたアクション
enum class ResultAction { None, Retry, GoTitle, GoStageSelect };

// ポップアップ内部の状態
// GameOver: Continue画面 → NO押下後に Title/Select 画面
// GameClear: 最初から Title/Select 画面
enum class ResultPopupPhase { Continue, Buttons };

// 各スプライトのレイアウト設定
struct PopupSpriteLayout {
    float x      = 0.0f;
    float y      = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

class GameResultPopup {
public:
    // アプリ起動直後に1回だけ呼ぶ（スプライト生成）
    void Initialize(GameApp& app);

    // ポップアップを表示する
    void Show(ResultKind kind);

    // ポップアップを非表示にする
    void Hide();

    bool IsVisible() const { return visible_; }

    // 毎フレーム呼ぶ
    void Update(GameApp& app, float dt);

    // Draw2D() 内で呼ぶ
    void Draw2D(GameApp& app);

    // DrawImGui() 内で呼ぶ
    void DrawImGui();

    // 決定されたアクションを取得
    ResultAction GetAction() const { return action_; }
    void ClearAction()             { action_ = ResultAction::None; }

    // レイアウトをJSONに保存・読込
    void SaveLayout() const;
    void LoadLayout();

private:
    // ホバー判定（layoutから直接AABB計算）
    bool IsHoveredLayout_(const PopupSpriteLayout& layout,
                          const Vector2& mouse) const;
    // Sprite::IsMouseOver ラッパー（旧実装、未使用）
    bool IsHovered_(const Sprite& sp, const Vector2& mouse) const;
    // ImGui ForegroundDrawList に判定矩形を描画
    void DrawHitboxOverlay_() const;

    // スプライトに位置・スケール・アルファを適用してUpdate/Drawする
    void DrawSprite_(Sprite& sp, const PopupSpriteLayout& layout,
                     const Matrix4x4& view, const Matrix4x4& proj,
                     float alpha = 1.0f, float scaleBoost = 1.0f);

private:
    bool         visible_ = false;
    ResultKind   kind_    = ResultKind::GameOver;
    ResultAction action_  = ResultAction::None;
    ResultPopupPhase phase_ = ResultPopupPhase::Continue;

    // フェードイン/アウト
    float fadeAlpha_ = 0.0f;

    // スプライト
    std::unique_ptr<Sprite> bgSprite_;        // 半透明黒背景
    std::unique_ptr<Sprite> gameOverSprite_;  // gameover.png
    std::unique_ptr<Sprite> gameClearSprite_; // gameclear.png
    std::unique_ptr<Sprite> continueSprite_;  // continue.png
    std::unique_ptr<Sprite> yesSprite_;       // yes.png
    std::unique_ptr<Sprite> noSprite_;        // no.png
    std::unique_ptr<Sprite> titleSprite_;     // title.png
    std::unique_ptr<Sprite> selectSprite_;    // select.png

    // ホバー中の選択インデックス（0=yes/title, 1=no/select, -1=none）
    int hoveredButton_ = -1;

    // レイアウト
    struct Layout {
        float bgAlpha    = 0.80f;
        // GameOver  (scaleX/Y = 画面上のピクセル幅/高さ)
        PopupSpriteLayout gameOver  = { 640, 140, 500, 120 };
        PopupSpriteLayout cont      = { 640, 260, 420, 100 };
        PopupSpriteLayout yes       = { 340, 420, 260, 100 };
        PopupSpriteLayout no        = { 760, 420, 260, 100 };
        // Buttons (NO後 or GameClear)
        PopupSpriteLayout gameClear = { 640, 140, 500, 120 };
        PopupSpriteLayout title     = { 340, 420, 260, 100 };
        PopupSpriteLayout select    = { 760, 420, 260, 100 };
        // 当たり判定スケール（1.0 = scaleX/Y ぴったり, 小さくすると狭くなる）
        float hitScale = 0.85f;
    } layout_;

    // デバッグ
    bool debugShowHitboxes_ = false;

    static constexpr const char* kLayoutPath = "resources/configs/gameResultPopup.json";
};
