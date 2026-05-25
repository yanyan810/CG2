#pragma once
#include "Sprite.h"
#include "TextSprite.h"
#include <string>

class GameApp;

class Button {
public:
    Button() = default;
    ~Button() = default;

    // 初期化（テキスト、識別用の名前/パス、座標、サイズなどを一気に設定）
    void Initialize(GameApp& app, const std::wstring& text, const std::string& name, const Vector2& position, const Vector2& scale = { 320.f, 60.f });

    // 更新処理（当たり判定や色変えのエフェクトも中に閉じ込める）
    void Update(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj);

    // 描画処理
    void Draw();

    void DrawImGui();

    void SetPosition(const Vector2 & position) {position_ = position;}
    void SetScale(const Vector2& scale) { scale_ = scale; }
    void SetTextOffset(const Vector2& textOffset) {textOffset_ = textOffset;}
    void SetNormalColor(const Vector4& normalColor) { normalColor_ = normalColor;}
    void SetHoverColor(const Vector4& hoverColor) {hoverColor_ = hoverColor;}
    void SetTextString(const std::wstring& textString) {textString_ = textString;}
    const std::string& GetName() const { return name_; }
    bool IsPressed() const { return isPressed_; }
    bool IsMouseOver()const { return isMouseOver_; }

private:
    std::unique_ptr<Sprite> bg_;
    std::unique_ptr<TextSprite> text_;
    std::string name_;
    bool isPressed_ = false;

    Vector2 position_;      // ボタン全体の座標
    Vector2 scale_;
    Vector2 textOffset_;    // ボタン背景からのテキストのズレ量
    Vector4 normalColor_;   // 通常時の色 (RGBA)
    Vector4 hoverColor_;    // マウスホバー時の色 (RGBA)
    std::wstring textString_; // 表示テキストのキャッシュ
    bool isMouseOver_ = false;

};