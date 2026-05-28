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
    void SetAlpha(float a) { alpha_ = a; }

    void SetFontFilePath(const std::wstring& path);
    void SetFontFaceName(const std::wstring& faceName);
    void SetFontSize(int size);
    void SetColor(const Vector3& color) { color_ = color; }
    void SetOutline(const Vector3& color, float width = 2.0f);
    void SetOutlineEnabled(bool enabled) { outlineEnabled_ = enabled; }
    void SetOutlineColor(const Vector3& color) { outlineColor_ = color; }
    void SetOutlineWidth(float width) { outlineWidth_ = width; }
    void ClearOutline() { outlineEnabled_ = false; }

    void Update(const Matrix4x4& view, const Matrix4x4& proj);
    void Draw();

    bool HasText() const { return !text_.empty(); }

    static void InitFontSystem();

private:
    void RebuildTexture_();
    bool LoadPrivateFont_();

private:
    static int s_nextId_;

    SpriteCommon* spriteCommon_ = nullptr;
    DirectXCommon* dx_ = nullptr;

    std::unique_ptr<Sprite> sprite_;
    std::vector<std::unique_ptr<Sprite>> outlineSprites_;

    std::wstring text_;
    std::wstring prevText_;

    float alpha_ = 1.0f;

    std::string textureKey_;

    Vector2 position_{ 40.0f, 620.0f };
    Vector3 size_{ 350.0f, 120.0f, 1.0f };

    std::wstring fontFilePath_ = L"resources/fonts/MPLUS1-Regular.otf";
    std::wstring fontFaceName_ = L"M PLUS 1";
    int fontSize_ = 28;
    bool privateFontLoaded_ = false;

    Vector3 color_{ 1.0f, 1.0f, 1.0f };
    Vector3 outlineColor_{ 0.0f, 0.0f, 0.0f };
    float outlineWidth_ = 0.0f;
    bool outlineEnabled_ = false;
    Matrix4x4 lastView_{};
    Matrix4x4 lastProj_{};
    bool hasLastMatrices_ = false;
};
