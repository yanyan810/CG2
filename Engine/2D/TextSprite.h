#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Sprite.h"

class SpriteCommon;
class DirectXCommon;

class TextSprite {
public:
    void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dx);

    void SetText(const std::wstring& text);
    void SetPosition(const Vector2& pos) { position_ = pos; }
    void SetSize(const Vector3& size) { size_ = size; }

    void Update(const Matrix4x4& view, const Matrix4x4& proj);
    void Draw();

private:
    void RebuildTexture_();

private:
    SpriteCommon* spriteCommon_ = nullptr;
    DirectXCommon* dx_ = nullptr;

    std::unique_ptr<Sprite> sprite_;

    std::wstring text_;
    std::wstring prevText_;

    std::string textureKey_ = "__ui_text_card_desc__";

    Vector2 position_{ 40.0f, 620.0f };
    Vector3 size_{ 700.0f, 64.0f ,0.0f};
};