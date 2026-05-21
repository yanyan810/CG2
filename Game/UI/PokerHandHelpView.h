#pragma once

#include <memory>
#include <vector>

#include "Matrix4x4.h"
#include "Vector3.h"

class DirectXCommon;
class Input;
class Sprite;
class SpriteCommon;
class TextSprite;

class PokerHandHelpView {
public:
    void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dx);
    void Update(Input* input);
    void Draw(const Matrix4x4& view, const Matrix4x4& proj);
#ifdef USE_IMGUI
    void DrawImGui();
#endif

private:
    struct Row {
        std::unique_ptr<Sprite> swatch;
        std::unique_ptr<TextSprite> text;
        Vector3 color{ 1.0f, 1.0f, 1.0f };
    };

    bool IsMouseOverIcon_(const Vector2& mouse) const;
    void ApplyGuideLayout_();

private:
    std::unique_ptr<Sprite> iconBg_;
    std::unique_ptr<TextSprite> iconText_;
    std::unique_ptr<Sprite> panelBg_;
    std::unique_ptr<TextSprite> titleText_;
    std::unique_ptr<Sprite> guideLine_;
    std::unique_ptr<TextSprite> guideTopArrow_;
    std::unique_ptr<TextSprite> guideBottomArrow_;
    std::unique_ptr<TextSprite> guideStrongText_;
    std::unique_ptr<TextSprite> guideWeakText_;
    std::vector<Row> rows_;
    bool visible_ = false;
    int appliedGuideArrowFontSize_ = -1;
    int appliedGuideStrongFontSize_ = -1;
    int appliedGuideWeakFontSize_ = -1;
};
