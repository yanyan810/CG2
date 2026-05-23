#include "PokerHandHelpView.h"

#include <algorithm>
#include <array>
#include <cwchar>

#include "DirectXCommon.h"
#include "Input/Input.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "TextSprite.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

namespace {
    constexpr float kIconX = 18.0f;
    constexpr float kIconY = 660.0f;
    constexpr float kIconSize = 42.0f;

    constexpr float kPanelX = 18.0f;
    constexpr float kPanelY = 292.0f;
    constexpr float kPanelW = 432.0f;
    constexpr float kPanelH = 356.0f;
    constexpr float kRowX = kPanelX + 24.0f;
    constexpr float kRowW = 330.0f;
    constexpr float kRowH = 28.0f;
    float sGuideX = 400.0f;
    float sGuideTopY = 334.0f;
    float sGuideBottomY = 645.0f;
    float sGuideLineWidth = 4.2f;
    float sGuideArrowFontSize = 32.0f;
    float sGuideArrowScale = 0.74f;
    float sGuideStrongFontSize = 64.0f;
    float sGuideWeakFontSize = 64.0f;
    float sGuideTopArrowOffsetX = -20.5f;
    float sGuideTopArrowOffsetY = -9.0f;
    float sGuideBottomArrowOffsetX = -20.5f;
    float sGuideBottomArrowOffsetY = -51.0f;
    float sGuideStrongOffsetX = 6.0f;
    float sGuideStrongOffsetY = -12.0f;
    float sGuideWeakOffsetX = 6.0f;
    float sGuideWeakOffsetY = -54.0f;
    constexpr float kRowBottomY = 604.0f;
    constexpr float kRowStepY = 30.0f;

    struct PokerHelpRowDef {
        const wchar_t* label;
        Vector3 color;
    };

    Vector3 GetTextColorForBar_(const Vector3& color)
    {
        const float luminance = color.x * 0.299f + color.y * 0.587f + color.z * 0.114f;
        return luminance > 0.55f
            ? Vector3{ 0.06f, 0.06f, 0.07f }
            : Vector3{ 1.0f, 1.0f, 1.0f };
    }

    float EstimateCenteredTextX_(const wchar_t* label)
    {
        const float glyphW = 17.0f;
        const float textW = static_cast<float>(std::wcslen(label)) * glyphW;
        return kRowX + (kRowW - textW) * 0.5f - 6.0f;
    }

    const std::array<PokerHelpRowDef, 10> kRowsWeakToStrong = {{
        { L"役なし", { 1.0f, 1.0f, 1.0f } },
        { L"ワンペア", { 1.0f, 0.85f, 0.20f } },
        { L"ツーペア", { 1.0f, 0.85f, 0.20f } },
        { L"スリーカード", { 0.25f, 0.95f, 0.35f } },
        { L"ストレート", { 0.25f, 0.95f, 0.35f } },
        { L"フラッシュ", { 0.25f, 0.95f, 0.35f } },
        { L"フルハウス", { 0.25f, 0.60f, 1.0f } },
        { L"フォーカード", { 1.0f, 0.25f, 0.20f } },
        { L"ストレートフラッシュ", { 1.0f, 0.25f, 0.20f } },
        { L"ロイヤルストレートフラッシュ(虹)", { 1.0f, 0.70f, 1.0f } },
    }};
}

void PokerHandHelpView::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dx)
{
    iconBg_ = std::make_unique<Sprite>();
    iconBg_->Initialize(spriteCommon, dx, "resources/ui/white.png");
    iconBg_->SetAnchorPoint({ 0.0f, 0.0f });
    iconBg_->SetPosition({ kIconX, kIconY });
    iconBg_->SetScale({ kIconSize, kIconSize, 1.0f });

    iconText_ = std::make_unique<TextSprite>();
    iconText_->Initialize(spriteCommon, dx);
    iconText_->SetText(L"?");
    iconText_->SetFontSize(28);
    iconText_->SetColor({ 0.95f, 0.95f, 1.0f });
    iconText_->SetSize({ 0.18f, 0.18f, 1.0f });
    iconText_->SetPosition({ kIconX + 11.0f, kIconY - 1.0f });

    panelBg_ = std::make_unique<Sprite>();
    panelBg_->Initialize(spriteCommon, dx, "resources/ui/white.png");
    panelBg_->SetAnchorPoint({ 0.0f, 0.0f });
    panelBg_->SetPosition({ kPanelX, kPanelY });
    panelBg_->SetScale({ kPanelW, kPanelH, 1.0f });
    panelBg_->SetColor({ 0.02f, 0.025f, 0.035f, 0.86f });

    titleText_ = std::make_unique<TextSprite>();
    titleText_->Initialize(spriteCommon, dx);
    titleText_->SetText(L"下から弱い順");
    titleText_->SetFontSize(28);
    titleText_->SetColor({ 1.0f, 0.95f, 0.75f });
    titleText_->SetSize({ 0.8f, 0.8f, 1.0f });
    titleText_->SetPosition({ kPanelX + 22.0f, kPanelY + 8.0f });

    guideLine_ = std::make_unique<Sprite>();
    guideLine_->Initialize(spriteCommon, dx, "resources/ui/white.png");
    guideLine_->SetColor({ 0.94f, 0.94f, 0.94f, 0.95f });

    guideTopArrow_ = std::make_unique<TextSprite>();
    guideTopArrow_->Initialize(spriteCommon, dx);
    guideTopArrow_->SetText(L"▲");
    guideTopArrow_->SetColor({ 0.94f, 0.94f, 0.94f });

    guideBottomArrow_ = std::make_unique<TextSprite>();
    guideBottomArrow_->Initialize(spriteCommon, dx);
    guideBottomArrow_->SetText(L"▼");
    guideBottomArrow_->SetColor({ 0.94f, 0.94f, 0.94f });

    guideStrongText_ = std::make_unique<TextSprite>();
    guideStrongText_->Initialize(spriteCommon, dx);
    guideStrongText_->SetText(L"強");
    guideStrongText_->SetFontSize(28);
    guideStrongText_->SetColor({ 1.0f, 0.88f, 0.25f });
    guideStrongText_->SetSize({ 0.55f, 0.55f, 1.0f });

    guideWeakText_ = std::make_unique<TextSprite>();
    guideWeakText_->Initialize(spriteCommon, dx);
    guideWeakText_->SetText(L"弱");
    guideWeakText_->SetFontSize(28);
    guideWeakText_->SetColor({ 0.9f, 0.92f, 1.0f });
    guideWeakText_->SetSize({ 0.55f, 0.55f, 1.0f });
    ApplyGuideLayout_();

    rows_.clear();
    rows_.reserve(kRowsWeakToStrong.size());
    for (size_t i = 0; i < kRowsWeakToStrong.size(); ++i) {
        const PokerHelpRowDef& def = kRowsWeakToStrong[i];
        const float y = kRowBottomY - static_cast<float>(i) * kRowStepY;

        Row row{};
        row.color = def.color;
        row.swatch = std::make_unique<Sprite>();
        row.swatch->Initialize(spriteCommon, dx, "resources/ui/white.png");
        row.swatch->SetAnchorPoint({ 0.0f, 0.0f });
        row.swatch->SetPosition({ kRowX, y + 1.0f });
        row.swatch->SetScale({ kRowW, kRowH, 1.0f });
        row.swatch->SetColor({ def.color.x, def.color.y, def.color.z, 1.0f });

        row.text = std::make_unique<TextSprite>();
        row.text->Initialize(spriteCommon, dx);
        row.text->SetText(def.label);
        row.text->SetFontSize(28);
        row.text->SetColor(GetTextColorForBar_(def.color));
        row.text->SetSize({ 0.72f, 0.72f, 1.0f });
        row.text->SetPosition({ EstimateCenteredTextX_(def.label), y - 8.0f });

        rows_.push_back(std::move(row));
    }
}

void PokerHandHelpView::Update(Input* input)
{
    if (!input) {
        visible_ = false;
        return;
    }

    const POINT mousePos = input->GetMousePosition();
    visible_ = IsMouseOverIcon_({
        static_cast<float>(mousePos.x),
        static_cast<float>(mousePos.y)
    });
}

void PokerHandHelpView::Draw(const Matrix4x4& view, const Matrix4x4& proj)
{
    if (iconBg_) {
        iconBg_->SetColor(visible_
            ? Vector4{ 1.0f, 0.88f, 0.25f, 0.95f }
            : Vector4{ 0.1f, 0.12f, 0.16f, 0.78f });
        iconBg_->Update(view, proj);
        iconBg_->Draw();
    }
    if (iconText_) {
        iconText_->SetColor(visible_
            ? Vector3{ 0.08f, 0.07f, 0.02f }
            : Vector3{ 0.95f, 0.95f, 1.0f });
        iconText_->Update(view, proj);
        iconText_->Draw();
    }

    if (!visible_) {
        return;
    }

    if (panelBg_) {
        panelBg_->Update(view, proj);
        panelBg_->Draw();
    }
    if (titleText_) {
        titleText_->Update(view, proj);
        titleText_->Draw();
    }

    if (guideLine_) {
        guideLine_->Update(view, proj);
        guideLine_->Draw();
    }
    if (guideTopArrow_) {
        guideTopArrow_->Update(view, proj);
        guideTopArrow_->Draw();
    }
    if (guideBottomArrow_) {
        guideBottomArrow_->Update(view, proj);
        guideBottomArrow_->Draw();
    }
    if (guideStrongText_) {
        guideStrongText_->Update(view, proj);
        guideStrongText_->Draw();
    }
    if (guideWeakText_) {
        guideWeakText_->Update(view, proj);
        guideWeakText_->Draw();
    }

    for (auto& row : rows_) {
        if (row.swatch) {
            row.swatch->Update(view, proj);
            row.swatch->Draw();
        }
        if (row.text) {
            row.text->Update(view, proj);
            row.text->Draw();
        }
    }
}

bool PokerHandHelpView::IsMouseOverIcon_(const Vector2& mouse) const
{
    constexpr float kHoverPad = 18.0f;
    const bool overIcon = mouse.x >= kIconX - kHoverPad &&
        mouse.x <= kIconX + kIconSize + kHoverPad &&
        mouse.y >= kIconY - kHoverPad &&
        mouse.y <= kIconY + kIconSize + kHoverPad;
    if (!visible_) {
        return overIcon;
    }

    const bool overPanel = mouse.x >= kPanelX &&
        mouse.x <= kPanelX + kPanelW &&
        mouse.y >= kPanelY &&
        mouse.y <= kPanelY + kPanelH;
    return overIcon || overPanel;
}

void PokerHandHelpView::ApplyGuideLayout_()
{
    const float guideHeight = std::max(1.0f, sGuideBottomY - sGuideTopY - 44.0f);
    const int guideArrowFontSize = static_cast<int>(sGuideArrowFontSize);
    const bool guideArrowFontSizeChanged = appliedGuideArrowFontSize_ != guideArrowFontSize;
    const int guideStrongFontSize = static_cast<int>(sGuideStrongFontSize);
    const bool guideStrongFontSizeChanged = appliedGuideStrongFontSize_ != guideStrongFontSize;
    const int guideWeakFontSize = static_cast<int>(sGuideWeakFontSize);
    const bool guideWeakFontSizeChanged = appliedGuideWeakFontSize_ != guideWeakFontSize;
    if (guideLine_) {
        guideLine_->SetAnchorPoint({ 0.5f, 0.0f });
        guideLine_->SetPosition({ sGuideX, sGuideTopY + 22.0f });
        guideLine_->SetScale({ sGuideLineWidth, guideHeight, 1.0f });
    }
    if (guideTopArrow_) {
        if (guideArrowFontSizeChanged) {
            guideTopArrow_->SetFontSize(guideArrowFontSize);
        }
        guideTopArrow_->SetSize({ sGuideArrowScale, sGuideArrowScale, 1.0f });
        guideTopArrow_->SetPosition({
            sGuideX + sGuideTopArrowOffsetX,
            sGuideTopY + sGuideTopArrowOffsetY
        });
    }
    if (guideBottomArrow_) {
        if (guideArrowFontSizeChanged) {
            guideBottomArrow_->SetFontSize(guideArrowFontSize);
        }
        guideBottomArrow_->SetSize({ sGuideArrowScale, sGuideArrowScale, 1.0f });
        guideBottomArrow_->SetPosition({
            sGuideX + sGuideBottomArrowOffsetX,
            sGuideBottomY + sGuideBottomArrowOffsetY
        });
    }
    if (guideStrongText_) {
        if (guideStrongFontSizeChanged) {
            guideStrongText_->SetFontSize(guideStrongFontSize);
        }
        guideStrongText_->SetPosition({
            sGuideX + sGuideStrongOffsetX,
            sGuideTopY + sGuideStrongOffsetY
        });
    }
    if (guideWeakText_) {
        if (guideWeakFontSizeChanged) {
            guideWeakText_->SetFontSize(guideWeakFontSize);
        }
        guideWeakText_->SetPosition({
            sGuideX + sGuideWeakOffsetX,
            sGuideBottomY + sGuideWeakOffsetY
        });
    }
    appliedGuideArrowFontSize_ = guideArrowFontSize;
    appliedGuideStrongFontSize_ = guideStrongFontSize;
    appliedGuideWeakFontSize_ = guideWeakFontSize;
}

#ifdef USE_IMGUI
void PokerHandHelpView::DrawImGui()
{
    if (!ImGui::CollapsingHeader("Poker Hand Help")) {
        return;
    }

    bool changed = false;
    changed |= ImGui::DragFloat("Guide X", &sGuideX, 1.0f);
    changed |= ImGui::DragFloat("Guide Top Y", &sGuideTopY, 1.0f);
    changed |= ImGui::DragFloat("Guide Bottom Y", &sGuideBottomY, 1.0f);
    changed |= ImGui::DragFloat("Guide Line Width", &sGuideLineWidth, 0.2f, 1.0f, 20.0f);
    changed |= ImGui::DragFloat("Arrow Font Size", &sGuideArrowFontSize, 1.0f, 8.0f, 80.0f);
    changed |= ImGui::DragFloat("Arrow Scale", &sGuideArrowScale, 0.01f, 0.1f, 2.0f);
    changed |= ImGui::DragFloat("Strong Font Size", &sGuideStrongFontSize, 1.0f, 8.0f, 80.0f);
    changed |= ImGui::DragFloat("Weak Font Size", &sGuideWeakFontSize, 1.0f, 8.0f, 80.0f);

    ImGui::TextUnformatted("Top Arrow Offset");
    changed |= ImGui::DragFloat("Top Arrow X", &sGuideTopArrowOffsetX, 1.0f);
    changed |= ImGui::DragFloat("Top Arrow Y", &sGuideTopArrowOffsetY, 1.0f);

    ImGui::TextUnformatted("Bottom Arrow Offset");
    changed |= ImGui::DragFloat("Bottom Arrow X", &sGuideBottomArrowOffsetX, 1.0f);
    changed |= ImGui::DragFloat("Bottom Arrow Y", &sGuideBottomArrowOffsetY, 1.0f);

    ImGui::TextUnformatted("Text Offset");
    changed |= ImGui::DragFloat("Strong Text X", &sGuideStrongOffsetX, 1.0f);
    changed |= ImGui::DragFloat("Strong Text Y", &sGuideStrongOffsetY, 1.0f);
    changed |= ImGui::DragFloat("Weak Text X", &sGuideWeakOffsetX, 1.0f);
    changed |= ImGui::DragFloat("Weak Text Y", &sGuideWeakOffsetY, 1.0f);

    if (changed) {
        ApplyGuideLayout_();
    }
}
#endif
