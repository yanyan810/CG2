#include"SelectionState.h"

#include"GameApp.h"

#include"../PausingUI.h"
#include"../GiveUp/GiveUpConfirmState.h"
#include "AudioManager.h"

void SelectionState::Initialize(GameApp& app) {

	title_ = std::make_unique<TextSprite>();
	title_->Initialize(app.SpriteCom(), app.Dx());
	title_->SetFontSize(30);
	title_->SetSize({ 1.f, 1.f, 1.f });
	title_->SetPosition({ 550.f, 100.f });
	title_->SetText(L"ポーズ中");

	// 再開ボタン
	auto resumeBtn = std::make_unique<DebugButton>();
	resumeBtn->Initialize(app, L"再開", "ResumeButton", { 530, 210.f });
	resumeBtn->SetScale({ 200.f,60.f });
	resumeBtn->SetNormalColor({ 0.1f, 0.1f, 0.1f, 0.9f });
	resumeBtn->SetHoverColor({ 0.4f, 0.4f, 0.4f, 0.9f });
	buttons_.push_back(std::move(resumeBtn));

	// 再開ボタン
	auto giveUpBtn = std::make_unique<DebugButton>();
	giveUpBtn->Initialize(app, L"降参", "GiveUpButton", { 530, 320.f });
	giveUpBtn->SetScale({ 200.f,60.f });
	giveUpBtn->SetNormalColor({ 0.1f, 0.1f, 0.1f, 0.9f });
	giveUpBtn->SetHoverColor({ 0.9f, 0.0f, 0.0f, 0.9f });
	buttons_.push_back(std::move(giveUpBtn));

}

void SelectionState::Update(PausingUI* context, GameApp& app, Input* input) {

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);

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
}
