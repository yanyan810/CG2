#include"SelectionState.h"

#include"GameApp.h"

#include"../PausingUI.h"
#include"../GiveUp/GiveUpConfirmState.h"
#include "AudioManager.h"

void SelectionState::Initialize(GameApp& app) {

	title_ = std::make_unique<TextSprite>();
	title_->Initialize(app.SpriteCom(), app.Dx());
	title_->SetFontSize(50);
	title_->SetSize({ 1.f, 1.f, 1.f });
	title_->SetPosition({ 550.f, 100.f });
	title_->SetText(L"ポーズ中");

	// --- 再開ボタン ---
	// 1. Button として生成
	auto resumeBtn = std::make_unique<Button>();
	// 2. 引数から L"再開" を削除（必要に応じて末尾に画像パスを指定してください）
	resumeBtn->Initialize(app, "ResumeButton", {490.f,250.f}, "resources/ui/white.png", "resources/ui/text/Restart.png");//Surrender
	// 3. SetScale から SetBgScale へ変更
	resumeBtn->SetFrameScale({ 1.f,1.f });
	resumeBtn->SetBgScale({288.f, 99.f });
	resumeBtn->SetNormalColor({ 0.1f, 0.1f, 0.1f, 0.9f });
	resumeBtn->SetHoverColor({ 0.4f, 0.4f, 0.4f, 0.9f });
	buttons_.push_back(std::move(resumeBtn));

	// --- 降参ボタン ---
	// 1. Button として生成
	auto giveUpBtn = std::make_unique<Button>();
	// 2. 引数から L"降参" を削除
	giveUpBtn->Initialize(app, "GiveUpButton", { 490.f,400.f }, "resources/ui/white.png", "resources/ui/text/Surrender.png");
	// 3. SetScale から SetBgScale へ変更
	giveUpBtn->SetFrameScale({ 1.f,1.f });
	giveUpBtn->SetBgScale({ 288.f, 99.f });
	giveUpBtn->SetNormalColor({ 0.1f, 0.1f, 0.1f, 0.9f });
	giveUpBtn->SetHoverColor({ 0.9f, 0.0f, 0.0f, 0.9f });
	buttons_.push_back(std::move(giveUpBtn));
	
	statusMenu_.Initialize(app, { 100.f, 300.f });


}

void SelectionState::Update(PausingUI* context, GameApp& app, Input* input) {

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);

	statusMenu_.Update(app, view, proj);

	for (auto& btn : buttons_) {
		if (btn->IsPressed()) {
			if (btn->GetName() == "ResumeButton") {
				context->SetIsPaused(false);
			}else if (btn->GetName() == "GiveUpButton") {
				context->ChangeState(std::make_unique<GiveUpConfirmState>(), app);
				return;
			}

		}
	}

	if (title_) title_->Update(view, proj);

	for (auto& btn : buttons_) {
		btn->Update(app, view, proj);
	}
}

void SelectionState::Draw(GameApp& app) {
	for (auto& btn : buttons_) btn->Draw();
	title_->Draw();
	statusMenu_.Draw();
}
