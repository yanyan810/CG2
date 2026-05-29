#include"PausingUI.h"

#include"GameApp.h"
#include"selection/SelectionState.h"
#include "AudioManager.h"

void PausingUI::Initialize(GameApp& app) {
	// 全状態共通の背景
	pausingBg_ = std::make_unique<Sprite>();
	pausingBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	pausingBg_->SetPosition({ 0.0f, 0.0f });
	pausingBg_->SetScale({ 1280.0f, 720.0f, 1.0f });
	pausingBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.8f });

	pauseButton_ = std::make_unique<Sprite>();
	pauseButton_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	pauseButton_->SetPosition({ 15.0f, 60.0f });
	pauseButton_->SetScale({ 60.0f, 60.0f, 1.0f });
	pauseButton_->SetColor({ 0.0f, 0.0f, 0.0f, 1.f });

	pauseButtonText_ = std::make_unique<TextSprite>();
	pauseButtonText_->Initialize(app.SpriteCom(), app.Dx());
	pauseButtonText_->SetFontSize(28);
	pauseButtonText_->SetSize({ 1.0f,1.0f,1.0f });
	pauseButtonText_->SetPosition({ 0.f, 60.f });
	pauseButtonText_->SetText(L"Pause");

	
}

void PausingUI::Update(GameApp& app, Input* input) {

	POINT mouse = input->GetMousePosition();
	Vector2 mousePos = { (float)mouse.x, (float)mouse.y };

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);

	
	pauseButton_->Update(view, proj);
	pauseButtonText_->Update(view, proj);

	bool buttonPressed = false;

	if (!isPaused_) {
		buttonPressed = input->IsMouseTrigger(0) && pauseButton_->IsMouseOver(mousePos);
	}

	if (pauseButton_->IsMouseOver(mousePos)) {
		pauseButton_->SetColor({ 0.5f, 0.5f, 0.5f, 0.8f });
	} else {
		pauseButton_->SetColor({ 0.0f, 0.0f, 0.0f, 0.8f });
	}

	if (input->IsKeyTrigger(DIK_TAB) || buttonPressed) {
		isPaused_ = !isPaused_;
		if (isPaused_) {
			AudioManager::GetInstance()->PlaySE("SE_Pop");
			// ポーズ開始時に初期ステートへ
			ChangeState(std::make_unique<SelectionState>(), app);
		}
	}

	if (!isPaused_ || !currentState_) return;

	// 共通背景の更新
	pausingBg_->Update(view, proj);

	// 現在のステートを更新
	currentState_->Update(this, app, input);
}

void PausingUI::Draw(GameApp& app) {

	if (!isPaused_) {
		pauseButton_->Draw();
		pauseButtonText_->Draw();
	}

	if (!isPaused_ || !currentState_) return;

	pausingBg_->Draw();
	currentState_->Draw(app);

}

void PausingUI::ChangeState(std::unique_ptr<IPauseState> newState, GameApp& app) {
	currentState_ = std::move(newState);
	currentState_->Initialize(app);
}
