#include "TutorialUi.h"
#include "GameApp.h"
#include "TutorialManager.h"
#include "BattleController.h"
#include "FieldUi.h"
#include "WinApp.h"

#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

using json = nlohmann::json;


static std::wstring Utf8ToWString(const std::string& s) {
    if (s.empty()) return L"";

    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (size <= 0) return L"";

    std::wstring out(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), size);
    return out;
}

static std::string WStringToUtf8(const std::wstring& ws) {
    if (ws.empty()) return "";

    int size = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";

    std::string out(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, out.data(), size, nullptr, nullptr);
    return out;
}

void TutorialUi::Initialize(GameApp& app)
{
    bg_ = std::make_unique<Sprite>();
    bg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    bg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.75f });

    text_ = std::make_unique<TextSprite>();
    text_->Initialize(app.SpriteCom(), app.Dx());
    text_->SetSize({ 1.0f, 1.0f, 1.0f });

    darkOverlay_ = std::make_unique<Sprite>();
    darkOverlay_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    darkOverlay_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

    for (auto& panel : spotlightPanels_) {
        panel = std::make_unique<Sprite>();
        panel->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
        panel->SetColor({ 0.0f, 0.0f, 0.0f, 0.58f });
        panel->SetPosition({ 0.0f, 0.0f });
        panel->SetScale({ 0.0f, 0.0f, 1.0f });
    }

    dimOverlay_ = std::make_unique<Sprite>();
    dimOverlay_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    dimOverlay_->SetColor({ 0.0f, 0.0f, 0.0f, 0.75f });
    dimOverlay_->SetPosition({ 0.0f, 0.0f });
    dimOverlay_->SetScale({ 1280.0f, 720.0f, 1.0f });

    for (auto& frame : focusFrames_) {
        frame = std::make_unique<Sprite>();
        frame->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
        frame->SetColor({ 1.0f, 1.0f, 1.0f, 0.10f });
        frame->SetPosition({ 0.0f, 0.0f });
        frame->SetScale({ 0.0f, 0.0f, 1.0f });
    }

    for (int i = 0; i < 3; ++i) {
        guideCircles_[i] = std::make_unique<Sprite>();
        guideCircles_[i]->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
        guideCircles_[i]->SetColor({ 1.0f, 1.0f, 0.2f, 0.30f });

        guideArrows_[i] = std::make_unique<Sprite>();
        guideArrows_[i]->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
        guideArrows_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.95f });
    }

    for (int i = 0; i < 3; ++i) {
        guideBoxBgs_[i] = std::make_unique<Sprite>();
        guideBoxBgs_[i]->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
        guideBoxBgs_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.95f });

        guideBoxTexts_[i] = std::make_unique<TextSprite>();
        guideBoxTexts_[i]->Initialize(app.SpriteCom(), app.Dx());
        guideBoxTexts_[i]->SetSize({ 1.0f, 1.0f, 1.0f });
    }

    // 仮配置
    cardGuideLayout_.costCircle.center = { 895.0f, 290.0f };
    cardGuideLayout_.costCircle.size = { 90.0f, 90.0f };

    cardGuideLayout_.suitCircle.center = { 1010.0f, 345.0f };
    cardGuideLayout_.suitCircle.size = { 100.0f, 100.0f };

    cardGuideLayout_.numberCircle.center = { 1015.0f, 520.0f };
    cardGuideLayout_.numberCircle.size = { 100.0f, 100.0f };

    cardGuideLayout_.costBox.bg = { 180.0f, 220.0f, 290.0f, 100.0f };
    cardGuideLayout_.costBox.textPos = { 210.0f, 245.0f };
    cardGuideLayout_.costBox.textScale = 0.9f;

    cardGuideLayout_.suitBox.bg = { 1140.0f, 285.0f, 290.0f, 100.0f };
    cardGuideLayout_.suitBox.textPos = { 1170.0f, 310.0f };
    cardGuideLayout_.suitBox.textScale = 0.9f;

    cardGuideLayout_.numberBox.bg = { 1140.0f, 470.0f, 290.0f, 100.0f };
    cardGuideLayout_.numberBox.textPos = { 1170.0f, 495.0f };
    cardGuideLayout_.numberBox.textScale = 0.9f;

    cardGuideLayout_.costArrow.rect = { 470.0f, 260.0f, 330.0f, 10.0f };
    cardGuideLayout_.suitArrow.rect = { 1045.0f, 335.0f, 90.0f, 10.0f };
    cardGuideLayout_.numberArrow.rect = { 1045.0f, 520.0f, 90.0f, 10.0f };

    LoadLayout(layoutPath_);
}

std::vector<UiRect> TutorialUi::ResolveFocusRects_(
    const TutorialManager& tutorial,
    const BattleController& battle,
    const FieldUi& fieldUi) const
{
    const auto& field = fieldUi.GetFieldUiLayout();
    const auto& poker = fieldUi.GetPokerEffectChoiceLayout();

    std::vector<UiRect> rects;

    // -----------------------------
    // ChoosePokerEffect 中は battle の実状態に応じて
    // 2択 / 3択 / 戻る を個別矩形で返す
    // -----------------------------
    if (tutorial.GetStep() == TutorialManager::TutorialStep::ChoosePokerEffect) {

        if (battle.IsWaitingActivateChoice()) {
            rects.push_back(poker.activateYesRect);
            rects.push_back(poker.activateNoRect);
            rects.push_back(poker.activateViewBoardRect);
            return rects;
        }

        if (battle.IsWaitingEffectChoice()) {
           // rects.push_back(poker.effectRects[0]);
            rects.push_back(poker.effectRects[1]);
            //rects.push_back(poker.effectRects[2]);
            //rects.push_back(poker.effectViewBoardRect);
            return rects;
        }

        if (battle.IsViewingBoardFromPokerUi()) {
            rects.push_back(poker.backRect);
            return rects;
        }
    }

    using Focus = TutorialManager::FocusType;

    switch (tutorial.GetFocusType()) {
    case Focus::HandArea:
        rects.push_back(layout_.handArea);
        break;

    case Focus::FieldArea:
        rects.push_back(layout_.fieldArea);
        break;

    case Focus::EnergyArea:
        rects.push_back(field.costBg);
        break;

    case Focus::EndTurnButtonArea:
        rects.push_back(field.endTurnBg);
        break;

    case Focus::PokerActivateChoiceArea:
        rects.push_back(poker.activateYesRect);
        rects.push_back(poker.activateNoRect);
        rects.push_back(poker.activateViewBoardRect);
        break;

    case Focus::PokerEffectChoiceArea:
        rects.push_back(poker.effectRects[0]);
        rects.push_back(poker.effectRects[1]);
        rects.push_back(poker.effectRects[2]);
        rects.push_back(poker.effectViewBoardRect);
        break;

    case Focus::PokerBackButtonArea:
        rects.push_back(poker.backRect);
        break;

    case Focus::PokerViewBoardButtonArea:
        if (battle.IsWaitingActivateChoice()) {
            rects.push_back(poker.activateViewBoardRect);
        } else if (battle.IsWaitingEffectChoice()) {
            rects.push_back(poker.effectViewBoardRect);
        } else if (battle.IsViewingBoardFromPokerUi()) {
            rects.push_back(poker.backRect);
        } else {
            rects.push_back(poker.backRect);
        }
        break;

    case Focus::PlayerHpArea:
        rects.push_back(layout_.playerHpArea);
        break;

    case Focus::EnemyHpArea:
        rects.push_back(layout_.enemyHpArea);
        break;

    case Focus::TurnTextArea:
        rects.push_back(layout_.turnTextArea);
        break;

    case Focus::RoleTextArea:
        rects.push_back(layout_.roleTextArea);
        break;

    case Focus::DeckCountArea:
        rects.push_back(layout_.deckCountArea);
        break;

    case Focus::PokerHandHelpArea:
        rects.push_back(layout_.pokerHandHelpArea);
        break;

    case Focus::EnemyTurnArea:
        rects.push_back(field.turnBg);
        break;

    case Focus::PlayerIncomingDamageArea:
        rects.push_back(layout_.playerIncomingDamageArea);
        break;

    case Focus::EnemyNextActionArea:
        rects.push_back(layout_.enemyNextActionArea);
        break;

    default:
        break;
    }

    return rects;
}

void TutorialUi::Update(GameApp& app,
    const TutorialManager& tutorial,
    const BattleController& battle,
    const FieldUi& fieldUi)
{
    (void)app;

    if (!tutorial.IsActive()) {
        if (text_) {
            text_->SetText(L"");
        }
        return;
    }

    if (tutorial.GetMessage() != prevText_) {
        textAlpha_ = 0.0f;
        prevText_ = tutorial.GetMessage();
    }

    textAlpha_ += 0.02f;
    if (textAlpha_ > 1.0f) {
        textAlpha_ = 1.0f;
    }

    focusBlink_ += 0.05f;
    float blink = 0.08f + (sinf(focusBlink_) * 0.5f + 0.5f) * 0.10f;

    float offsetY = 0.0f;
    if (tutorial.GetStep() == TutorialManager::TutorialStep::ExplainCardAll) {
        offsetY = layout_.explainCardMessageOffsetY;
    }

    bg_->SetPosition({ layout_.messageBg.x, layout_.messageBg.y + offsetY });
    bg_->SetScale({ layout_.messageBg.w, layout_.messageBg.h, 1.0f });

    text_->SetPosition({ layout_.messageText.x, layout_.messageText.y + offsetY });
    text_->SetText(prevText_);

    darkOverlay_->SetPosition({ layout_.darkOverlay.x, layout_.darkOverlay.y });
    darkOverlay_->SetScale({ layout_.darkOverlay.w, layout_.darkOverlay.h, 1.0f });

    std::vector<UiRect> focusRects = ResolveFocusRects_(tutorial, battle, fieldUi);
    hasSpotlight_ = !focusRects.empty();

    if (hasSpotlight_) {
        UiRect focus = focusRects.front();
        for (const UiRect& r : focusRects) {
            const float left = std::min(focus.x, r.x);
            const float top = std::min(focus.y, r.y);
            const float right = std::max(focus.x + focus.w, r.x + r.w);
            const float bottom = std::max(focus.y + focus.h, r.y + r.h);
            focus = { left, top, right - left, bottom - top };
        }

        constexpr float kSpotlightPadding = 12.0f;
        const float screenW = static_cast<float>(WinApp::kClientWidth);
        const float screenH = static_cast<float>(WinApp::kClientHeight);
        const float left = std::clamp(focus.x - kSpotlightPadding, 0.0f, screenW);
        const float top = std::clamp(focus.y - kSpotlightPadding, 0.0f, screenH);
        const float right = std::clamp(focus.x + focus.w + kSpotlightPadding, 0.0f, screenW);
        const float bottom = std::clamp(focus.y + focus.h + kSpotlightPadding, 0.0f, screenH);

        const UiRect panels[4] = {
            { 0.0f, 0.0f, screenW, top },
            { 0.0f, bottom, screenW, screenH - bottom },
            { 0.0f, top, left, bottom - top },
            { right, top, screenW - right, bottom - top },
        };

        for (int i = 0; i < 4; ++i) {
            if (!spotlightPanels_[i]) {
                continue;
            }
            spotlightPanels_[i]->SetPosition({ panels[i].x, panels[i].y });
            spotlightPanels_[i]->SetScale({ panels[i].w, panels[i].h, 1.0f });
            spotlightPanels_[i]->SetColor({ 0.0f, 0.0f, 0.0f, 0.58f });
        }
    } else {
        for (auto& panel : spotlightPanels_) {
            if (!panel) {
                continue;
            }
            panel->SetPosition({ 0.0f, 0.0f });
            panel->SetScale({ 0.0f, 0.0f, 1.0f });
            panel->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
        }
    }

    for (int i = 0; i < kMaxFocusFrames; ++i) {
        if (!focusFrames_[i]) {
            continue;
        }

        if (i < static_cast<int>(focusRects.size())) {
            const UiRect& r = focusRects[i];
            focusFrames_[i]->SetPosition({ r.x, r.y });
            focusFrames_[i]->SetScale({ r.w, r.h, 1.0f });
            focusFrames_[i]->SetColor({ 1.0f, 1.0f, 1.0f, blink });
        } else {
            focusFrames_[i]->SetPosition({ 0.0f, 0.0f });
            focusFrames_[i]->SetScale({ 0.0f, 0.0f, 1.0f });
            focusFrames_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
        }
    }
}

void TutorialUi::DrawGuideCircle_(
    int index,
    const GuideCircleLayout& layout,
    const Matrix4x4& view,
    const Matrix4x4& proj)
{
    if (index < 0 || index >= 3 || !guideCircles_[index]) {
        return;
    }

    guideCircles_[index]->SetPosition({ layout.center.x, layout.center.y });
    guideCircles_[index]->SetScale({ layout.size.x, layout.size.y, 1.0f });
    guideCircles_[index]->SetColor(layout.color);
    guideCircles_[index]->Update(view, proj);
    guideCircles_[index]->Draw();
}

void TutorialUi::DrawGuideArrow_(
    int index,
    const GuideArrowLayout& layout,
    const Matrix4x4& view,
    const Matrix4x4& proj)
{
    if (index < 0 || index >= 3 || !guideArrows_[index]) {
        return;
    }

    guideArrows_[index]->SetPosition({ layout.rect.x, layout.rect.y });

    const float sx = layout.flipX ? -layout.rect.w : layout.rect.w;
    guideArrows_[index]->SetScale({ sx, layout.rect.h, 1.0f });

    guideArrows_[index]->SetColor({ 1.0f, 1.0f, 1.0f, 0.95f });
    guideArrows_[index]->Update(view, proj);
    guideArrows_[index]->Draw();
}

void TutorialUi::DrawGuideBox_(
    int index,
    const GuideBoxLayout& layout,
    const std::wstring& text,
    const Matrix4x4& view,
    const Matrix4x4& proj)
{
    if (index < 0 || index >= 3) {
        return;
    }
    if (!guideBoxBgs_[index] || !guideBoxTexts_[index]) {
        return;
    }

    guideBoxBgs_[index]->SetPosition({ layout.bg.x, layout.bg.y });
    guideBoxBgs_[index]->SetScale({ layout.bg.w, layout.bg.h, 1.0f });
    guideBoxBgs_[index]->SetColor(layout.color);
    guideBoxBgs_[index]->Update(view, proj);
    guideBoxBgs_[index]->Draw();

    guideBoxTexts_[index]->SetText(text);
    guideBoxTexts_[index]->SetPosition({ layout.textPos.x, layout.textPos.y });
    guideBoxTexts_[index]->SetSize({ layout.textScale, layout.textScale, 1.0f });
    guideBoxTexts_[index]->Update(view, proj);
    guideBoxTexts_[index]->Draw();
}

void TutorialUi::DrawCardGuide_(
    const TutorialManager& tutorial,
    const Matrix4x4& view,
    const Matrix4x4& proj)
{
    if (tutorial.GetStep() != TutorialManager::TutorialStep::ExplainCardAll) {
        return;
    }

    if (!cardGuideLayout_.enable) {
        return;
    }

    DrawGuideCircle_(0, cardGuideLayout_.costCircle, view, proj);
    DrawGuideCircle_(1, cardGuideLayout_.suitCircle, view, proj);
    DrawGuideCircle_(2, cardGuideLayout_.numberCircle, view, proj);

    DrawGuideArrow_(0, cardGuideLayout_.costArrow, view, proj);
    DrawGuideArrow_(1, cardGuideLayout_.suitArrow, view, proj);
    DrawGuideArrow_(2, cardGuideLayout_.numberArrow, view, proj);

    DrawGuideBox_(0, cardGuideLayout_.costBox,
        L"この数字が\nカードの使用コストです",
        view, proj);

    DrawGuideBox_(1, cardGuideLayout_.suitBox,
        L"このマークで\nポーカー役を作ります",
        view, proj);

    DrawGuideBox_(2, cardGuideLayout_.numberBox,
        L"この数字が\nポーカーの数字です",
        view, proj);
}

void TutorialUi::DrawDimOverlay(GameApp& app)
{
	if (!dimOverlay_) return;

	Matrix4x4 viewMat = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 projMat = Matrix4x4::MakeOrthographicMatrix((float)WinApp::kClientWidth, (float)WinApp::kClientHeight);
	
	dimOverlay_->Update(viewMat, projMat);
	dimOverlay_->Draw();
}

void TutorialUi::Draw(GameApp& app,
    TutorialManager& tutorial,
    const BattleController& battle)
{
    (void)app;
    (void)battle;

    if (!tutorial.IsActive()) {
        return;
    }

    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0,
        float(WinApp::kClientWidth),
        float(WinApp::kClientHeight),
        0, 100
    );

    if (hasSpotlight_) {
        for (auto& panel : spotlightPanels_) {
            if (!panel) {
                continue;
            }
            panel->Update(view, proj);
            panel->Draw();
        }
    }

    for (auto& frame : focusFrames_) {
        if (!frame) {
            continue;
        }
        frame->Update(view, proj);
        frame->Draw();
    }

    DrawCardGuide_(tutorial, view, proj);

    if (bg_) {
        bg_->Update(view, proj);
        bg_->Draw();
    }

    if (text_) {
        text_->SetAlpha(textAlpha_);
        text_->Update(view, proj);
        text_->Draw();
    }
}

void TutorialUi::DrawDebugCardGuide(GameApp& app)
{
    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0,
        float(WinApp::kClientWidth),
        float(WinApp::kClientHeight),
        0, 100
    );

    if (!cardGuideLayout_.enable) {
        return;
    }

    DrawGuideCircle_(0, cardGuideLayout_.costCircle, view, proj);
    DrawGuideCircle_(1, cardGuideLayout_.suitCircle, view, proj);
    DrawGuideCircle_(2, cardGuideLayout_.numberCircle, view, proj);

    DrawGuideArrow_(0, cardGuideLayout_.costArrow, view, proj);
    DrawGuideArrow_(1, cardGuideLayout_.suitArrow, view, proj);
    DrawGuideArrow_(2, cardGuideLayout_.numberArrow, view, proj);

    DrawGuideBox_(0, cardGuideLayout_.costBox,
        L"この数字が\nカードの使用コストです",
        view, proj);

    DrawGuideBox_(1, cardGuideLayout_.suitBox,
        L"このマークで\nポーカー役を作ります",
        view, proj);

    DrawGuideBox_(2, cardGuideLayout_.numberBox,
        L"この数字が\nポーカーの数字です",
        view, proj);
}

bool TutorialUi::SaveLayout(const std::string& path) const
{
    json j;

    auto writeRect = [](json& dst, const UiRect& r) {
        dst["x"] = r.x;
        dst["y"] = r.y;
        dst["w"] = r.w;
        dst["h"] = r.h;
        };

    auto writeVec2 = [](json& dst, const UiVec2& v) {
        dst["x"] = v.x;
        dst["y"] = v.y;
        };

    auto writeVec4 = [](json& dst, const Vector4& v) {
        dst["x"] = v.x;
        dst["y"] = v.y;
        dst["z"] = v.z;
        dst["w"] = v.w;
        };

    writeRect(j["messageBg"], layout_.messageBg);
    writeVec2(j["messageText"], layout_.messageText);
    j["explainCardMessageOffsetY"] = layout_.explainCardMessageOffsetY;
    writeRect(j["darkOverlay"], layout_.darkOverlay);
    writeRect(j["handArea"], layout_.handArea);
    writeRect(j["fieldArea"], layout_.fieldArea);
    writeRect(j["playerHpArea"], layout_.playerHpArea);
    writeRect(j["enemyHpArea"], layout_.enemyHpArea);
    writeRect(j["turnTextArea"], layout_.turnTextArea);
    writeRect(j["roleTextArea"], layout_.roleTextArea);
    writeRect(j["deckCountArea"], layout_.deckCountArea);
    writeRect(j["pokerHandHelpArea"], layout_.pokerHandHelpArea);
    writeRect(j["playerIncomingDamageArea"], layout_.playerIncomingDamageArea);
    writeRect(j["enemyNextActionArea"], layout_.enemyNextActionArea);

    j["cardGuide"]["enable"] = cardGuideLayout_.enable;

    writeVec2(j["cardGuide"]["costCircle"]["center"], cardGuideLayout_.costCircle.center);
    writeVec2(j["cardGuide"]["costCircle"]["size"], cardGuideLayout_.costCircle.size);
    writeVec4(j["cardGuide"]["costCircle"]["color"], cardGuideLayout_.costCircle.color);

    writeVec2(j["cardGuide"]["suitCircle"]["center"], cardGuideLayout_.suitCircle.center);
    writeVec2(j["cardGuide"]["suitCircle"]["size"], cardGuideLayout_.suitCircle.size);
    writeVec4(j["cardGuide"]["suitCircle"]["color"], cardGuideLayout_.suitCircle.color);

    writeVec2(j["cardGuide"]["numberCircle"]["center"], cardGuideLayout_.numberCircle.center);
    writeVec2(j["cardGuide"]["numberCircle"]["size"], cardGuideLayout_.numberCircle.size);
    writeVec4(j["cardGuide"]["numberCircle"]["color"], cardGuideLayout_.numberCircle.color);

    writeRect(j["cardGuide"]["costBox"]["bg"], cardGuideLayout_.costBox.bg);
    writeVec2(j["cardGuide"]["costBox"]["textPos"], cardGuideLayout_.costBox.textPos);
    j["cardGuide"]["costBox"]["textScale"] = cardGuideLayout_.costBox.textScale;
    writeVec4(j["cardGuide"]["costBox"]["color"], cardGuideLayout_.costBox.color);

    writeRect(j["cardGuide"]["suitBox"]["bg"], cardGuideLayout_.suitBox.bg);
    writeVec2(j["cardGuide"]["suitBox"]["textPos"], cardGuideLayout_.suitBox.textPos);
    j["cardGuide"]["suitBox"]["textScale"] = cardGuideLayout_.suitBox.textScale;
    writeVec4(j["cardGuide"]["suitBox"]["color"], cardGuideLayout_.suitBox.color);

    writeRect(j["cardGuide"]["numberBox"]["bg"], cardGuideLayout_.numberBox.bg);
    writeVec2(j["cardGuide"]["numberBox"]["textPos"], cardGuideLayout_.numberBox.textPos);
    j["cardGuide"]["numberBox"]["textScale"] = cardGuideLayout_.numberBox.textScale;
    writeVec4(j["cardGuide"]["numberBox"]["color"], cardGuideLayout_.numberBox.color);

    writeRect(j["cardGuide"]["costArrow"]["rect"], cardGuideLayout_.costArrow.rect);
    j["cardGuide"]["costArrow"]["flipX"] = cardGuideLayout_.costArrow.flipX;

    writeRect(j["cardGuide"]["suitArrow"]["rect"], cardGuideLayout_.suitArrow.rect);
    j["cardGuide"]["suitArrow"]["flipX"] = cardGuideLayout_.suitArrow.flipX;

    writeRect(j["cardGuide"]["numberArrow"]["rect"], cardGuideLayout_.numberArrow.rect);
    j["cardGuide"]["numberArrow"]["flipX"] = cardGuideLayout_.numberArrow.flipX;

    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        return false;
    }

    ofs << j.dump(2);
    return true;
}

bool TutorialUi::LoadLayout(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return false;
    }

    json j;
    ifs >> j;

    auto readRect = [](const json& src, UiRect& r) {
        r.x = src.value("x", r.x);
        r.y = src.value("y", r.y);
        r.w = src.value("w", r.w);
        r.h = src.value("h", r.h);
        };

    auto readVec2 = [](const json& src, UiVec2& v) {
        v.x = src.value("x", v.x);
        v.y = src.value("y", v.y);
        };

    auto readVec4 = [](const json& src, Vector4& v) {
        v.x = src.value("x", v.x);
        v.y = src.value("y", v.y);
        v.z = src.value("z", v.z);
        v.w = src.value("w", v.w);
        };

    if (j.contains("messageBg")) readRect(j["messageBg"], layout_.messageBg);
    if (j.contains("messageText")) readVec2(j["messageText"], layout_.messageText);
    layout_.explainCardMessageOffsetY = j.value("explainCardMessageOffsetY", layout_.explainCardMessageOffsetY);
    if (j.contains("darkOverlay")) readRect(j["darkOverlay"], layout_.darkOverlay);
    if (j.contains("handArea")) readRect(j["handArea"], layout_.handArea);
    if (j.contains("fieldArea")) readRect(j["fieldArea"], layout_.fieldArea);
    if (j.contains("playerHpArea")) readRect(j["playerHpArea"], layout_.playerHpArea);
    if (j.contains("enemyHpArea")) readRect(j["enemyHpArea"], layout_.enemyHpArea);
    if (j.contains("turnTextArea")) readRect(j["turnTextArea"], layout_.turnTextArea);
    if (j.contains("roleTextArea")) readRect(j["roleTextArea"], layout_.roleTextArea);
    if (j.contains("deckCountArea")) readRect(j["deckCountArea"], layout_.deckCountArea);
    if (j.contains("pokerHandHelpArea")) readRect(j["pokerHandHelpArea"], layout_.pokerHandHelpArea);
    if (j.contains("playerIncomingDamageArea")) {
        readRect(j["playerIncomingDamageArea"], layout_.playerIncomingDamageArea);
    }
    if (j.contains("enemyNextActionArea")) {
        readRect(j["enemyNextActionArea"], layout_.enemyNextActionArea);
    }

    if (j.contains("cardGuide")) {
        const json& g = j["cardGuide"];

        cardGuideLayout_.enable = g.value("enable", cardGuideLayout_.enable);

        if (g.contains("costCircle")) {
            const json& n = g["costCircle"];
            if (n.contains("center")) readVec2(n["center"], cardGuideLayout_.costCircle.center);
            if (n.contains("size")) readVec2(n["size"], cardGuideLayout_.costCircle.size);
            if (n.contains("color")) readVec4(n["color"], cardGuideLayout_.costCircle.color);
        }

        if (g.contains("suitCircle")) {
            const json& n = g["suitCircle"];
            if (n.contains("center")) readVec2(n["center"], cardGuideLayout_.suitCircle.center);
            if (n.contains("size")) readVec2(n["size"], cardGuideLayout_.suitCircle.size);
            if (n.contains("color")) readVec4(n["color"], cardGuideLayout_.suitCircle.color);
        }

        if (g.contains("numberCircle")) {
            const json& n = g["numberCircle"];
            if (n.contains("center")) readVec2(n["center"], cardGuideLayout_.numberCircle.center);
            if (n.contains("size")) readVec2(n["size"], cardGuideLayout_.numberCircle.size);
            if (n.contains("color")) readVec4(n["color"], cardGuideLayout_.numberCircle.color);
        }

        if (g.contains("costBox")) {
            const json& n = g["costBox"];
            if (n.contains("bg")) readRect(n["bg"], cardGuideLayout_.costBox.bg);
            if (n.contains("textPos")) readVec2(n["textPos"], cardGuideLayout_.costBox.textPos);
            cardGuideLayout_.costBox.textScale = n.value("textScale", cardGuideLayout_.costBox.textScale);
            if (n.contains("color")) readVec4(n["color"], cardGuideLayout_.costBox.color);
        }

        if (g.contains("suitBox")) {
            const json& n = g["suitBox"];
            if (n.contains("bg")) readRect(n["bg"], cardGuideLayout_.suitBox.bg);
            if (n.contains("textPos")) readVec2(n["textPos"], cardGuideLayout_.suitBox.textPos);
            cardGuideLayout_.suitBox.textScale = n.value("textScale", cardGuideLayout_.suitBox.textScale);
            if (n.contains("color")) readVec4(n["color"], cardGuideLayout_.suitBox.color);
        }

        if (g.contains("numberBox")) {
            const json& n = g["numberBox"];
            if (n.contains("bg")) readRect(n["bg"], cardGuideLayout_.numberBox.bg);
            if (n.contains("textPos")) readVec2(n["textPos"], cardGuideLayout_.numberBox.textPos);
            cardGuideLayout_.numberBox.textScale = n.value("textScale", cardGuideLayout_.numberBox.textScale);
            if (n.contains("color")) readVec4(n["color"], cardGuideLayout_.numberBox.color);
        }

        if (g.contains("costArrow")) {
            const json& n = g["costArrow"];
            if (n.contains("rect")) readRect(n["rect"], cardGuideLayout_.costArrow.rect);
            cardGuideLayout_.costArrow.flipX = n.value("flipX", cardGuideLayout_.costArrow.flipX);
        }

        if (g.contains("suitArrow")) {
            const json& n = g["suitArrow"];
            if (n.contains("rect")) readRect(n["rect"], cardGuideLayout_.suitArrow.rect);
            cardGuideLayout_.suitArrow.flipX = n.value("flipX", cardGuideLayout_.suitArrow.flipX);
        }

        if (g.contains("numberArrow")) {
            const json& n = g["numberArrow"];
            if (n.contains("rect")) readRect(n["rect"], cardGuideLayout_.numberArrow.rect);
            cardGuideLayout_.numberArrow.flipX = n.value("flipX", cardGuideLayout_.numberArrow.flipX);
        }
    }

    return true;
}

#ifdef USE_IMGUI
void TutorialUi::DrawImGui(TutorialManager& tutorial)
{
    if (!ImGui::Begin("TutorialUi Layout")) {
        ImGui::End();
        return;
    }

    ImGui::Separator();
    ImGui::Text("Message");
    ImGui::DragFloat4("messageBg", &layout_.messageBg.x, 1.0f);
    ImGui::DragFloat2("messageText", &layout_.messageText.x, 1.0f);
    ImGui::DragFloat("explainCardMessageOffsetY", &layout_.explainCardMessageOffsetY, 1.0f);

    ImGui::Separator();
    ImGui::Text("Overlay");
    ImGui::DragFloat4("darkOverlay", &layout_.darkOverlay.x, 1.0f);

    ImGui::Separator();
    ImGui::Text("Manual Areas");
    ImGui::DragFloat4("handArea", &layout_.handArea.x, 1.0f);
    ImGui::DragFloat4("fieldArea", &layout_.fieldArea.x, 1.0f);

    ImGui::DragFloat4("playerHpArea", &layout_.playerHpArea.x, 1.0f);
    ImGui::DragFloat4("enemyHpArea", &layout_.enemyHpArea.x, 1.0f);
    ImGui::DragFloat4("turnTextArea", &layout_.turnTextArea.x, 1.0f);
    ImGui::DragFloat4("roleTextArea", &layout_.roleTextArea.x, 1.0f);
    ImGui::DragFloat4("deckCountArea", &layout_.deckCountArea.x, 1.0f);
    ImGui::DragFloat4("pokerHandHelpArea", &layout_.pokerHandHelpArea.x, 1.0f);

    ImGui::DragFloat4("playerIncomingDamageArea", &layout_.playerIncomingDamageArea.x, 1.0f);
    ImGui::DragFloat4("enemyNextActionArea", &layout_.enemyNextActionArea.x, 1.0f);

    ImGui::Separator();
    ImGui::Text("Card Guide");
    ImGui::Checkbox("cardGuide enable", &cardGuideLayout_.enable);

    if (ImGui::TreeNode("Cost Circle")) {
        ImGui::DragFloat2("costCircle center", &cardGuideLayout_.costCircle.center.x, 1.0f);
        ImGui::DragFloat2("costCircle size", &cardGuideLayout_.costCircle.size.x, 1.0f, 1.0f, 1000.0f);
        ImGui::ColorEdit4("costCircle color", &cardGuideLayout_.costCircle.color.x);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Suit Circle")) {
        ImGui::DragFloat2("suitCircle center", &cardGuideLayout_.suitCircle.center.x, 1.0f);
        ImGui::DragFloat2("suitCircle size", &cardGuideLayout_.suitCircle.size.x, 1.0f, 1.0f, 1000.0f);
        ImGui::ColorEdit4("suitCircle color", &cardGuideLayout_.suitCircle.color.x);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Number Circle")) {
        ImGui::DragFloat2("numberCircle center", &cardGuideLayout_.numberCircle.center.x, 1.0f);
        ImGui::DragFloat2("numberCircle size", &cardGuideLayout_.numberCircle.size.x, 1.0f, 1.0f, 1000.0f);
        ImGui::ColorEdit4("numberCircle color", &cardGuideLayout_.numberCircle.color.x);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Cost Box")) {
        ImGui::DragFloat4("costBox bg", &cardGuideLayout_.costBox.bg.x, 1.0f);
        ImGui::DragFloat2("costBox textPos", &cardGuideLayout_.costBox.textPos.x, 1.0f);
        ImGui::DragFloat("costBox textScale", &cardGuideLayout_.costBox.textScale, 0.01f, 0.1f, 5.0f);
        ImGui::ColorEdit4("costBox color", &cardGuideLayout_.costBox.color.x);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Suit Box")) {
        ImGui::DragFloat4("suitBox bg", &cardGuideLayout_.suitBox.bg.x, 1.0f);
        ImGui::DragFloat2("suitBox textPos", &cardGuideLayout_.suitBox.textPos.x, 1.0f);
        ImGui::DragFloat("suitBox textScale", &cardGuideLayout_.suitBox.textScale, 0.01f, 0.1f, 5.0f);
        ImGui::ColorEdit4("suitBox color", &cardGuideLayout_.suitBox.color.x);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Number Box")) {
        ImGui::DragFloat4("numberBox bg", &cardGuideLayout_.numberBox.bg.x, 1.0f);
        ImGui::DragFloat2("numberBox textPos", &cardGuideLayout_.numberBox.textPos.x, 1.0f);
        ImGui::DragFloat("numberBox textScale", &cardGuideLayout_.numberBox.textScale, 0.01f, 0.1f, 5.0f);
        ImGui::ColorEdit4("numberBox color", &cardGuideLayout_.numberBox.color.x);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Cost Arrow")) {
        ImGui::DragFloat4("costArrow rect", &cardGuideLayout_.costArrow.rect.x, 1.0f);
        ImGui::Checkbox("costArrow flipX", &cardGuideLayout_.costArrow.flipX);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Suit Arrow")) {
        ImGui::DragFloat4("suitArrow rect", &cardGuideLayout_.suitArrow.rect.x, 1.0f);
        ImGui::Checkbox("suitArrow flipX", &cardGuideLayout_.suitArrow.flipX);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Number Arrow")) {
        ImGui::DragFloat4("numberArrow rect", &cardGuideLayout_.numberArrow.rect.x, 1.0f);
        ImGui::Checkbox("numberArrow flipX", &cardGuideLayout_.numberArrow.flipX);
        ImGui::TreePop();
    }

    ImGui::Separator();
    ImGui::Text("Tutorial Message Edit");

    if (ImGui::Button("Save TutorialUiLayout")) {
        SaveLayout(layoutPath_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load TutorialUiLayout")) {
        LoadLayout(layoutPath_);
    }

    static std::string msgReloadResult = "none";

    if (ImGui::Button("Reload Tutorial Messages")) {
        bool ok = tutorial.ReloadMessages();
        msgReloadResult = ok ? "Reload OK" : "Reload FAILED";
    }
    ImGui::Text("MessageReload: %s", msgReloadResult.c_str());

    ImGui::End();
}
#endif
