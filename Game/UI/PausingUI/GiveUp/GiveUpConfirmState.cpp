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
}

void GiveUpConfirmState::Update(PausingUI* context, GameApp& app, Input* input) {
    POINT mousePoint = input->GetMousePosition();
    Vector2 mousePos = { (float)mousePoint.x, (float)mousePoint.y };

    // 行列更新
    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);

    confirmBoard_->Update(view, proj);

    // ESCキーで戻る（「いいえ」と同じ挙動）
    if (input->IsKeyTrigger(DIK_ESCAPE)) {
        AudioManager::GetInstance()->PlaySE("SE_Tap");
        context->ChangeState(std::make_unique<SelectionState>(), app);
        return;
    }

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
    for (auto& s : sprites_) {
        s->Draw();
    }
    confirmBoard_->Draw();
}
