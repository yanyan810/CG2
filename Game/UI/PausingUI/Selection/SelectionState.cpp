#include"SelectionState.h"

#include"GameApp.h"

#include"../PausingUI.h"
#include"../GiveUp/GiveUpConfirmState.h"

void SelectionState::Initialize(GameApp& app) {

	baseSprite_ = std::make_unique<Sprite>();
	baseSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/PauseMenu.png");
	baseSprite_->SetPosition({ 0.0f, 0.0f });
	baseSprite_->SetScale({ 1.0f, 1.0f, 1.0f });
	baseSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	baseSprite_->SetName("Base");

	// 1. スプライトの生成と名前の設定
	auto resume = std::make_unique<Sprite>();
	resume->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	resume->SetPosition({ 530.0f, 210.0f });
	resume->SetScale({ 200.0f, 60.0f, 1.0f });
	resume->SetColor({ 0.0f, 0.0f, 0.0f, 0.9f });
	resume->SetName("Resume");
	sprites_.push_back(std::move(resume));

	// デッキ確認の背景とテキスト
	auto deckCheck = std::make_unique<Sprite>();
	deckCheck->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	deckCheck->SetPosition({ 472.0f, 325.0f });
	deckCheck->SetScale({ 320.0f, 60.0f, 1.0f });
	deckCheck->SetColor({ 0.0f, 0.0f, 0.0f, 0.9f });
	deckCheck->SetName("DeckCheck");
	sprites_.push_back(std::move(deckCheck));

	auto giveUp = std::make_unique<Sprite>();
	giveUp->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	giveUp->SetPosition({ 530.0f, 440.0f });
	giveUp->SetScale({ 200.0f, 60.0f, 1.0f });
	giveUp->SetColor({ 0.0f, 0.0f, 0.0f, 0.9f });
	giveUp->SetName("GiveUp");
	sprites_.push_back(std::move(giveUp));

	// チュートリアル確認の背景とテキスト
	auto  tutrialCheck = std::make_unique<Sprite>();
	tutrialCheck->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	tutrialCheck->SetPosition({ 383.0f, 554.0f });
	tutrialCheck->SetScale({ 500.0f, 60.0f, 1.0f });
	tutrialCheck->SetColor({ 0.0f, 0.0f, 0.0f, 0.9f });
	tutrialCheck->SetName("Tutorial");
	sprites_.push_back(std::move(tutrialCheck));

}

void SelectionState::Update(PausingUI* context, GameApp& app, Input* input){
	POINT mousePoint = input->GetMousePosition();
	Vector2 mousePos = { (float)mousePoint.x, (float)mousePoint.y };

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);

	// 共通関数でホバー中のスプライトを取得
	Sprite* hovered = CheckMouseOverByName(mousePos);

	// 全ボタンを一旦デフォルト色に
	for (auto& s : sprites_) s->SetColor({ 0.0f, 0.0f, 0.0f, 0.7f });


	if (hovered) {
		hovered->SetColor({ 0.5f, 0.5f, 0.5f, 0.6f }); // ホバー強調

		std::string name = hovered->GetName();

		if (name == "GiveUp")hovered->SetColor({ 1.f, 0.2f, 0.2f, 0.6f });

		if (input->IsMouseTrigger(0)) {
			if (name == "Resume") context->SetIsPaused(false);
			//if (name == "DeckCheck")
			if (name == "GiveUp") {
				context->ChangeState(std::make_unique<GiveUpConfirmState>(), app); 
				return;
			}
			//if (name == "Tutrial")
		}
	}

	if (baseSprite_) baseSprite_->Update(view, proj);

	// ★重要：各ボタンの行列を更新
	for (auto& s : sprites_) {
		s->Update(view, proj);
	}
}

void SelectionState::Draw(GameApp& app) {
	for (auto& s : sprites_) s->Draw();
	baseSprite_->Draw();
}