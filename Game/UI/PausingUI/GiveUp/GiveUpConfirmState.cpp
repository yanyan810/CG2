#include "GiveUpConfirmState.h"
#include "../Selection/SelectionState.h"
#include "GameApp.h"
#include "../PausingUI.h"
#include "AudioManager.h"

void GiveUpConfirmState::Initialize(GameApp& app) {
    // 1. 確認用ボード（メッセージ）の初期化
    confirmBoard_ = std::make_unique<Sprite>();
    confirmBoard_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/GiveUpCheck.png");
    confirmBoard_->SetPosition({ 0.0f, 0.0f });
    confirmBoard_->SetScale({ 1.0f, 1.0f, 1.0f });

    // 2. 「はい」ボタン
    auto yesBtn = std::make_unique<Sprite>();
    yesBtn->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    yesBtn->SetPosition({ 360.0f, 425.0f });
    yesBtn->SetScale({ 200.0f, 60.0f, 1.0f });
    yesBtn->SetColor({ 0.8f, 0.0f, 0.0f, 0.9f }); // 注意を引くために赤色
    yesBtn->SetName("Yes");
    sprites_.push_back(std::move(yesBtn));

    // 3. 「いいえ」ボタン
    auto noBtn = std::make_unique<Sprite>();
    noBtn->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    noBtn->SetPosition({ 730.0f, 425.0f });
    noBtn->SetScale({ 200.0f, 60.0f, 1.0f });
    noBtn->SetColor({ 0.0f, 0.0f, 0.0f, 0.9f });
    noBtn->SetName("No");
    sprites_.push_back(std::move(noBtn));

    titleText_ = std::make_unique<TextSprite>();
    titleText_->Initialize(app.SpriteCom(), app.Dx());
    titleText_->SetFontSize(32);
    titleText_->SetSize({ 1.0f, 1.0f, 1.0f });
    titleText_->SetPosition({ 430.0f, 250.0f });

    yesText_ = std::make_unique<TextSprite>();
    yesText_->Initialize(app.SpriteCom(), app.Dx());
    yesText_->SetFontSize(30);
    yesText_->SetSize({ 1.0f, 1.0f, 1.0f });
    yesText_->SetPosition({ 425.0f, 430.0f });
    yesText_->SetText(L"はい");

    noText_ = std::make_unique<TextSprite>();
    noText_->Initialize(app.SpriteCom(), app.Dx());
    noText_->SetFontSize(30);
    noText_->SetSize({ 1.0f, 1.0f, 1.0f });
    noText_->SetPosition({ 795.0f, 430.0f });
    noText_->SetText(L"いいえ");
}

void GiveUpConfirmState::Update(PausingUI* context, GameApp& app, Input* input) {

    if (pushedYes_) {
        return;
    }

    POINT mousePoint = input->GetMousePosition();
    Vector2 mousePos = { (float)mousePoint.x, (float)mousePoint.y };

    // 行列更新
    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);

    if (context->IsTutorialExitMode()) {
        confirmBoard_->SetColor({ 0.04f, 0.04f, 0.05f, 0.96f });
        confirmBoard_->SetPosition({ 280.0f, 205.0f });
        confirmBoard_->SetScale({ 720.0f, 330.0f, 1.0f });
        if (titleText_) {
            titleText_->SetText(L"チュートリアルを終了しますか？");
            titleText_->Update(view, proj);
        }
    } else {
        confirmBoard_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        confirmBoard_->SetPosition({ 0.0f, 0.0f });
        confirmBoard_->SetScale({ 1.0f, 1.0f, 1.0f });
    }
    confirmBoard_->Update(view, proj);
    if (yesText_) yesText_->Update(view, proj);
    if (noText_) noText_->Update(view, proj);

    // ホバー判定
    Sprite* hovered = CheckMouseOverByName(mousePos);

    for (auto& s : sprites_) {
        s->Update(view, proj);

        if (s.get() == hovered) {
            s->SetColor({ 0.5f, 0.5f, 0.5f, 0.9f });

            if (input->IsMouseTrigger(0)) {
                AudioManager::GetInstance()->PlaySE("SE_Tap");
                std::string name = s->GetName();
                if (name == "Yes") {
                    // タイトルシーンへ遷移（GameAppの機能に依存）
					context->RequestSceneChange(true); // シーン変更要求を出す
					pushedYes_ = true;
                } else if (name == "No") {
                    // メインメニュー（Selection）へ戻る
                    context->ChangeState(std::make_unique<SelectionState>(), app);
                }
            }
        } else {
            // 元の色に戻す
            if (s->GetName() == "Yes") s->SetColor({ 0.8f, 0.0f, 0.0f, 0.9f });
            else s->SetColor({ 0.0f, 0.0f, 0.0f, 0.9f });
        }
    }
}

void GiveUpConfirmState::Draw(GameApp& app) {
    confirmBoard_->Draw();
    if (titleText_) {
        titleText_->Draw();
    }
    for (auto& s : sprites_) {
        s->Draw();
    }
    if (yesText_) yesText_->Draw();
    if (noText_) noText_->Draw();
}
