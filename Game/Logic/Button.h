#pragma once
#include "Sprite.h"
#include <string>
#include <memory>

class GameApp;

class Button {
public:
    Button() = default;
    ~Button() = default;

    // 初期化：テキスト引数を削除し、背景と枠のパスを指定できるように変更
    void Initialize(GameApp& app,
        const std::string& name,
        const Vector2& position,
        const Vector2& scale = { 320.f, 60.f },
        const std::string& bgPath = "resources/ui/white.png",
        const std::string& framePath = "resources/ui/frame.png");

    void Update(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj);
    void Draw();
    void DrawImGui();

    // セッター・ゲッター
    void SetPosition(const Vector2& position);
    void SetScale(const Vector2& scale);

    // 背景色・枠色のコントロール
    void SetNormalColor(const Vector4& normalColor) { normalColor_ = normalColor; }
    void SetHoverColor(const Vector4& hoverColor) { hoverColor_ = hoverColor; }
    void SetFrameColor(const Vector4& frameColor);

    const std::string& GetName() const { return name_; }
    bool IsPressed() const { return isPressed_; }
    bool IsMouseOver() const { return isMouseOver_; }

    Vector2 GetPosition() const { return position_; }
    Vector2 GetScale() const { return scale_; }

private:
    std::string name_;
    Vector2 position_{ 0.f, 0.f };
    Vector2 scale_{ 1.f, 1.f };

    // --- 枠と背景の2つのスプライトで構成 ---
    std::unique_ptr<Sprite> bg_;
    std::unique_ptr<Sprite> frame_;

    Vector4 normalColor_{ 0.3f, 0.3f, 0.3f, 1.0f };
    Vector4 hoverColor_{ 0.2f, 0.5f, 0.4f, 1.0f };
    Vector4 frameColor_{ 1.0f, 1.0f, 1.0f, 1.0f };

    bool isPressed_ = false;
    bool isMouseOver_ = false;
};