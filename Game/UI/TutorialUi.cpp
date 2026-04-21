#include "TutorialUi.h"
#include "GameApp.h"
#include "TutorialManager.h"
#include "BattleController.h"
#include "FieldUi.h"
#include "WinApp.h"

#include <fstream>
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

    for (auto& frame : focusFrames_) {
        frame = std::make_unique<Sprite>();
        frame->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
        frame->SetColor({ 1.0f, 1.0f, 1.0f, 0.10f });
        frame->SetPosition({ 0.0f, 0.0f });
        frame->SetScale({ 0.0f, 0.0f, 1.0f });
    }

	// ガイド用の円と矢印、説明ボックスを初期化
    guideCircle_ = std::make_unique<Sprite>();
    guideCircle_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    guideCircle_->SetColor({ 1.0f, 1.0f, 0.2f, 0.30f });

    guideArrow_ = std::make_unique<Sprite>();
    guideArrow_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    guideArrow_->SetColor({ 1.0f, 1.0f, 1.0f, 0.95f });

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

    bg_->SetPosition({ layout_.messageBg.x, layout_.messageBg.y });
    bg_->SetScale({ layout_.messageBg.w, layout_.messageBg.h, 1.0f });

    text_->SetPosition({ layout_.messageText.x, layout_.messageText.y });
    text_->SetText(prevText_);

    darkOverlay_->SetPosition({ layout_.darkOverlay.x, layout_.darkOverlay.y });
    darkOverlay_->SetScale({ layout_.darkOverlay.w, layout_.darkOverlay.h, 1.0f });

    std::vector<UiRect> focusRects = ResolveFocusRects_(tutorial, battle, fieldUi);

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
    const GuideCircleLayout& layout,
    const Matrix4x4& view,
    const Matrix4x4& proj)
{
    if (!guideCircle_) {
        return;
    }

    guideCircle_->SetPosition({ layout.center.x, layout.center.y });
    guideCircle_->SetScale({ layout.size.x, layout.size.y, 1.0f });
    guideCircle_->SetColor(layout.color);
    guideCircle_->Update(view, proj);
    guideCircle_->Draw();
}

void TutorialUi::DrawGuideArrow_(
    const GuideArrowLayout& layout,
    const Matrix4x4& view,
    const Matrix4x4& proj)
{
    if (!guideArrow_) {
        return;
    }

    guideArrow_->SetPosition({ layout.rect.x, layout.rect.y });
    guideArrow_->SetScale({ layout.rect.w, layout.rect.h, 1.0f });
    guideArrow_->SetColor({ 1.0f, 1.0f, 1.0f, 0.95f });
    guideArrow_->Update(view, proj);
    guideArrow_->Draw();
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

    DrawGuideCircle_(cardGuideLayout_.costCircle, view, proj);
    DrawGuideCircle_(cardGuideLayout_.suitCircle, view, proj);
    DrawGuideCircle_(cardGuideLayout_.numberCircle, view, proj);

    DrawGuideArrow_(cardGuideLayout_.costArrow, view, proj);
    DrawGuideArrow_(cardGuideLayout_.suitArrow, view, proj);
    DrawGuideArrow_(cardGuideLayout_.numberArrow, view, proj);

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

void TutorialUi::Draw(GameApp& app,
    const TutorialManager& tutorial,
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

    if (darkOverlay_) {
        darkOverlay_->Update(view, proj);
        darkOverlay_->Draw();
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

    writeRect(j["messageBg"], layout_.messageBg);
    writeVec2(j["messageText"], layout_.messageText);
    writeRect(j["darkOverlay"], layout_.darkOverlay);
    writeRect(j["handArea"], layout_.handArea);
    writeRect(j["fieldArea"], layout_.fieldArea);
    writeRect(j["playerHpArea"], layout_.playerHpArea);
    writeRect(j["enemyHpArea"], layout_.enemyHpArea);
    writeRect(j["turnTextArea"], layout_.turnTextArea);
    writeRect(j["roleTextArea"], layout_.roleTextArea);
    writeRect(j["deckCountArea"], layout_.deckCountArea);
    writeRect(j["playerIncomingDamageArea"], layout_.playerIncomingDamageArea);
    writeRect(j["enemyNextActionArea"], layout_.enemyNextActionArea);


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

    if (j.contains("messageBg")) readRect(j["messageBg"], layout_.messageBg);
    if (j.contains("messageText")) readVec2(j["messageText"], layout_.messageText);
    if (j.contains("darkOverlay")) readRect(j["darkOverlay"], layout_.darkOverlay);
    if (j.contains("handArea")) readRect(j["handArea"], layout_.handArea);
    if (j.contains("fieldArea")) readRect(j["fieldArea"], layout_.fieldArea);
    if (j.contains("playerHpArea")) readRect(j["playerHpArea"], layout_.playerHpArea);
    if (j.contains("enemyHpArea")) readRect(j["enemyHpArea"], layout_.enemyHpArea);
    if (j.contains("turnTextArea")) readRect(j["turnTextArea"], layout_.turnTextArea);
    if (j.contains("roleTextArea")) readRect(j["roleTextArea"], layout_.roleTextArea);
    if (j.contains("deckCountArea")) readRect(j["deckCountArea"], layout_.deckCountArea);
    if (j.contains("playerIncomingDamageArea")) {
        readRect(j["playerIncomingDamageArea"], layout_.playerIncomingDamageArea);
    }
    if (j.contains("enemyNextActionArea")) {
        readRect(j["enemyNextActionArea"], layout_.enemyNextActionArea);
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

    ImGui::DragFloat4("playerIncomingDamageArea", &layout_.playerIncomingDamageArea.x, 1.0f);
    ImGui::DragFloat4("enemyNextActionArea", &layout_.enemyNextActionArea.x, 1.0f);

    ImGui::Separator();
    ImGui::Text("Tutorial Message Edit");

    ImGui::Separator();

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