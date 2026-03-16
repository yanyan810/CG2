#include "FieldUi.h"
#include "GameApp.h"
#include "BattleController.h"
#include "TextSprite.h"
#include "Sprite.h"
#include "WinApp.h"
#include "CardDatabase.h"

static std::wstring Utf8ToWStringLocal(const std::string& s)
{
    if (s.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), size);
    return out;
}

std::wstring FieldUi::Utf8ToWString_(const std::string& s)
{
    return Utf8ToWStringLocal(s);
}

void FieldUi::Initialize(GameApp& app)
{
    cardDescText_ = std::make_unique<TextSprite>();
    cardDescText_->Initialize(app.SpriteCom(), app.Dx());
    cardDescText_->SetPosition({ 40.0f, 620.0f });

    cardDescBg_ = std::make_unique<Sprite>();
    cardDescBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    cardDescBg_->SetPosition({ 20.0f, 60.0f });
    cardDescBg_->SetScale({ 900.0f, 180.0f, 1.0f });
    cardDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

    deckCountText_ = std::make_unique<TextSprite>();
    deckCountText_->Initialize(app.SpriteCom(), app.Dx());
    deckCountText_->SetPosition({ 1120.0f, 640.0f });
    deckCountText_->SetSize({ 0.9f, 0.9f, 1.0f });

    deckCountBg_ = std::make_unique<Sprite>();
    deckCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    deckCountBg_->SetPosition({ 20.0f, 310.0f });
    deckCountBg_->SetScale({ 0.0f, 0.0f, 1.0f });
    deckCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

    discardCountText_ = std::make_unique<TextSprite>();
    discardCountText_->Initialize(app.SpriteCom(), app.Dx());
    discardCountText_->SetPosition({ 1120.0f, 350.0f });
    discardCountText_->SetSize({ 0.9f, 0.9f, 1.0f });

    discardCountBg_ = std::make_unique<Sprite>();
    discardCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    discardCountBg_->SetPosition({ 1100.0f, 350.0f });
    discardCountBg_->SetScale({ 150.0f, 60.0f, 1.0f });
    discardCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

    handCountText_ = std::make_unique<TextSprite>();
    handCountText_->Initialize(app.SpriteCom(), app.Dx());
    handCountText_->SetPosition({ 1020.0f, 640.0f });
    handCountText_->SetSize({ 0.9f, 0.9f, 1.0f });

    handCountBg_ = std::make_unique<Sprite>();
    handCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    handCountBg_->SetPosition({ 1000.0f, 640.0f });
    handCountBg_->SetScale({ 250.0f, 60.0f, 1.0f });
    handCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

    fieldCountText_ = std::make_unique<TextSprite>();
    fieldCountText_->Initialize(app.SpriteCom(), app.Dx());
    fieldCountText_->SetPosition({ 600.0f, 250.0f });
    fieldCountText_->SetSize({ 0.9f, 0.9f, 1.0f });

    fieldCountBg_ = std::make_unique<Sprite>();
    fieldCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    fieldCountBg_->SetPosition({ 540.0f, 250.0f });
    fieldCountBg_->SetScale({ 250.0f, 60.0f, 1.0f });
    fieldCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });
}

void FieldUi::Update(GameApp& app, const BattleController& battle)
{

    showDescBg_ = false;

    if (battle.HasPokerChoiceUi()) {
        showDescBg_ = true;
        cardDescText_->SetSize({ 1.0f,1.0f,1.0f });
        cardDescText_->SetPosition({ 40.0f, 80.0f });
        cardDescText_->SetText(battle.GetPokerChoiceUiText());

        cardDescBg_->SetPosition({ 20.0f, 52.0f });
        cardDescBg_->SetScale({ 900.0f, 180.0f, 1.0f });
    } else {
        cardDescText_->SetSize({ 1.0f,1.0f,1.0f });
        cardDescText_->SetPosition({ 40.0f, 620.0f });

        const CardDef* def = battle.GetPreviewCardDef();
        if (def) {
            showDescBg_ = true;
            cardDescText_->SetText(Utf8ToWString_(def->desc));

            cardDescBg_->SetPosition({ 20.0f, 600.0f });
            cardDescBg_->SetScale({ 900.0f, 120.0f, 1.0f });
        } else if (battle.ShouldShowOperationUi()) {
            showDescBg_ = true;
            cardDescText_->SetPosition({ 40.0f, 520.0f });
            cardDescText_->SetText(battle.GetOperationUiText());

            cardDescBg_->SetPosition({ 20.0f, 500.0f });
            cardDescBg_->SetScale({ 900.0f, 280.0f, 1.0f });
        } else {
            cardDescText_->SetText(L"");
        }
    }

    deckCountText_->SetText(L"山札:" + std::to_wstring(battle.GetDeckCount()));
    discardCountText_->SetText(L"墓地:" + std::to_wstring(battle.GetDiscardCount()));
    handCountText_->SetText(L"手札:" + std::to_wstring(battle.GetHandCount()));
    fieldCountText_->SetText(battle.GetCurrentPokerHandUiText());
}

void FieldUi::Draw(GameApp& app)
{
    app.SpriteCom()->SetGraphicsPipelineState();

    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0,
        float(WinApp::kClientWidth),
        float(WinApp::kClientHeight),
        0, 100
    );

    if (showDescBg_ && cardDescBg_) {
        cardDescBg_->Update(view, proj);
        cardDescBg_->Draw();
    }

    if (deckCountBg_) {
        deckCountBg_->Update(view, proj);
        deckCountBg_->Draw();
    }
    if (discardCountBg_) {
        discardCountBg_->Update(view, proj);
        discardCountBg_->Draw();
    }
    if (handCountBg_) {
        handCountBg_->Update(view, proj);
        handCountBg_->Draw();
    }
    if (fieldCountBg_) {
        fieldCountBg_->Update(view, proj);
        fieldCountBg_->Draw();
    }

    if (cardDescText_) {
        cardDescText_->Update(view, proj);
        cardDescText_->Draw();
    }

    if (deckCountText_) {
        deckCountText_->Update(view, proj);
        deckCountText_->Draw();
    }
    if (discardCountText_) {
        discardCountText_->Update(view, proj);
        discardCountText_->Draw();
    }
    if (handCountText_) {
        handCountText_->Update(view, proj);
        handCountText_->Draw();
    }
    if (fieldCountText_) {
        fieldCountText_->Update(view, proj);
        fieldCountText_->Draw();
    }
}