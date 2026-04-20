#include "TitleScene.h"

#include "GameApp.h"
#include "Camera.h"
#include "WinApp.h"
#include "TextureManager.h"

static std::wstring Utf8ToWStringLocal_Title(const std::string& s)
{
	if (s.empty()) return L"";
	int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring out(size - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), size);
	return out;
}

//------------------------------------------------------------
// シーン開始時の初期化
//------------------------------------------------------------
void TitleScene::OnEnter(GameApp& app) {
	// タイトル状態を初期化
	state_ = State::Idle;
	circle_ = 1.0f;
	softness_ = 0.6f;

	//--------------------------------------------------------
	// カメラ作成
	//--------------------------------------------------------
	camera_ = std::make_unique<Camera>();
	camera_->SetTranslate({ 0.0f, 0.0f, 0.0f });
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	app.ObjCom()->SetDefaultCamera(camera_.get());

	//--------------------------------------------------------
	// 3Dモデルの初期化
	//--------------------------------------------------------
	skyDome_ = std::make_unique<Object3d>();
	skyDome_->Initialize(app.ObjCom(), app.Dx());
	skyDome_->SetModel("skydome/skydome.obj");
	skyDome_->SetCamera(camera_.get());
	skyDome_->SetEnableLighting(0);
	// ★カメラが原点になったので、天球の中心も原点にする
	skyDome_->SetTranslate({ 0.0f, 0.0f, 0.0f });
	skyDome_->SetScale({ 100.0f, 100.0f, 100.0f });

	//--------------------------------------------------------
	// 背景画像
	//--------------------------------------------------------
	bg_ = std::make_unique<Sprite>();
	bg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/char/Title.png");
	bg_->SetAnchorPoint({ 0.0f, 0.0f });
	bg_->SetPosition({ 0.0f, 0.0f });
	bg_->SetScale({ 1.0f, 1.0f, 1.0f });

	//--------------------------------------------------------
	// Press Space画像
	//--------------------------------------------------------
	pressStart_ = std::make_unique<Sprite>();
	pressStart_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/char/pressSpace.png");
	pressStart_->SetAnchorPoint({ 0.0f, 0.0f });

	// 位置はあとで調整しやすいように中央下寄りに置く
	pressStart_->SetPosition({ 430.0f, 560.0f });
	pressStart_->SetScale({ 1.0f, 1.0f, 1.0f });

	//カード慣例を先に読む
	battle_.Preload(app);

	fieldUi_ = std::make_unique<FieldUi>();
	fieldUi_->Initialize(app);

	ApplyDebugPokerPreviewData_();

	//AudioManager::GetInstance()->PlayBGM("machi");

}

//------------------------------------------------------------
// シーン終了時の解放
//------------------------------------------------------------
void TitleScene::OnExit(GameApp&) {
	pressStart_.reset();
	bg_.reset();
	camera_.reset();
}

//------------------------------------------------------------
// 毎フレーム更新
//------------------------------------------------------------
void TitleScene::Update(GameApp& app, float dt) {
	Input* input = app.GetInput();
	if (!input) {
		return;
	}

	//--------------------------------------------------------
	// ESCでアプリ終了
	//--------------------------------------------------------
	if (input->IsKeyPressed(DIK_ESCAPE)) {
		app.RequestQuit();
		return;
	}

	//--------------------------------------------------------
	// SPACEの押した瞬間
	//--------------------------------------------------------
	bool spaceTrig = input->IsKeyTrigger(DIK_SPACE);

	//================
	//Tキーを押したとき
	//================

	bool tutorialTrig = input->IsKeyTrigger(DIK_T);


	//================
	//Dキーを押したとき
	//================

	bool deckEditTrig = input->IsKeyTrigger(DIK_D);

	//--------------------------------------------------------
	// 状態更新
	//--------------------------------------------------------
	switch (state_) {
	case State::Idle:
		// 入力待ち中にSPACEで閉じ演出へ
		if (spaceTrig) {
			state_ = State::ExitClose;
		}

		if (tutorialTrig) {
			RequestChangeScene_("Tutorial");
			return;
		}

		if (deckEditTrig) {
			RequestChangeScene_("DeckEdit");
			return;
		}

		break;

	case State::ExitClose:
		// 円形マスクを閉じていく
		circle_ -= 1.8f * dt;

		if (circle_ <= 0.0f) {
			circle_ = 0.0f;
			RequestChangeScene_(kNextScene_);
		}
		break;
	}


	//3D更新
	skyDome_->Update(dt);

	if (fieldUi_) {
	/*	fieldUi_->SetEditCardId(debugCardId_);

		fieldUi_->SetDebugPokerPreviewVisible(showPokerPreview_);

		const CardDef* debugDef = battle_.FindCardDef(debugCardId_);
		fieldUi_->SetDebugImageCardDescVisible(showDebugCardDesc_);
		fieldUi_->SetDebugImageCardDescCard(debugDef);
		ApplyDebugPokerPreviewData_();
		fieldUi_->Update(app, battle_);*/
	}

}

//------------------------------------------------------------
// 3D描画
//------------------------------------------------------------
void TitleScene::Draw3D(GameApp& app) {
	app.ObjCom()->SetGraphicsPipelineState();

}

//------------------------------------------------------------
// 2D描画
//------------------------------------------------------------
void TitleScene::Draw2D(GameApp& app) {
	app.SpriteCom()->SetGraphicsPipelineState();

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
		0, 0,
		float(WinApp::kClientWidth),
		float(WinApp::kClientHeight),
		0, 100
	);

	//--------------------------------------------------------
	// 背景描画
	//--------------------------------------------------------
	if (bg_) {
		bg_->Update(view, proj);
		bg_->Draw();
	}

	//--------------------------------------------------------
	// Press Space描画
	//--------------------------------------------------------
	if (pressStart_) {
		pressStart_->Update(view, proj);
		pressStart_->Draw();
	}

	//デバッグ用
	//if (fieldUi_) {
	//	fieldUi_->Draw(app, battle_);
	//}

	//--------------------------------------------------------
	// 円形マスク描画
	//--------------------------------------------------------
	app.SpriteCom()->DrawCircleMask(circle_, softness_);
}

//------------------------------------------------------------
// ImGui描画
//------------------------------------------------------------
void TitleScene::DrawImGui(GameApp& app) {
#ifdef USE_IMGUI
	ImGui::Begin("Title Debug");
	ImGui::Text("Simple Title Scene");
	ImGui::Text("SPACE : Start Game");
	ImGui::Text("ESC   : Quit");
	ImGui::SliderFloat("Circle", &circle_, 0.0f, 1.0f);
	ImGui::SliderFloat("Softness", &softness_, 0.0f, 1.0f);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
		1000.0f / ImGui::GetIO().Framerate,
		ImGui::GetIO().Framerate);

	ImGui::Separator();
	ImGui::Checkbox("Show Debug Card Desc", &showDebugCardDesc_);
	ImGui::DragInt("Debug Card ID", &debugCardId_, 1.0f, 1, 999);

	ImGui::Checkbox("Show Poker Preview", &showPokerPreview_);

	ImGui::Separator();
	ImGui::Text("Debug Poker Preview Text");

	bool previewTextChanged = false;

	previewTextChanged |= ImGui::Checkbox("Show Poker Preview", &showPokerPreview_);

	previewTextChanged |= ImGui::DragInt("Activated Line Count", &debugActivatedLineCount_, 1.0f, 0, 5);
	previewTextChanged |= ImGui::DragInt("TurnStart Line Count", &debugTurnStartLineCount_, 1.0f, 0, 5);

	ImGui::Separator();
	ImGui::Text("Activated Lines");

	for (int i = 0; i < 5; ++i) {
		std::string label = "Activated Line " + std::to_string(i + 1);
		previewTextChanged |= ImGui::InputText(
			label.c_str(),
			debugActivatedLinesUtf8_[i].data(),
			debugActivatedLinesUtf8_[i].size()
		);
	}

	if (ImGui::Button("Fill Activated: 10回復 x5")) {
		for (int i = 0; i < 5; ++i) {
			strcpy_s(debugActivatedLinesUtf8_[i].data(), debugActivatedLinesUtf8_[i].size(), "10回復");
		}
		debugActivatedLineCount_ = 5;
		previewTextChanged = true;
	}

	ImGui::Separator();
	ImGui::Text("Turn Start Lines");

	for (int i = 0; i < 5; ++i) {
		std::string label = "TurnStart Line " + std::to_string(i + 1);
		previewTextChanged |= ImGui::InputText(
			label.c_str(),
			debugTurnStartLinesUtf8_[i].data(),
			debugTurnStartLinesUtf8_[i].size()
		);
	}

	if (ImGui::Button("Fill TurnStart: 10回復 x5")) {
		for (int i = 0; i < 5; ++i) {
			strcpy_s(debugTurnStartLinesUtf8_[i].data(), debugTurnStartLinesUtf8_[i].size(), "10回復");
		}
		debugTurnStartLineCount_ = 5;
		previewTextChanged = true;
	}

	if (ImGui::Button("Clear All Preview Lines")) {
		for (int i = 0; i < 5; ++i) {
			debugActivatedLinesUtf8_[i][0] = '\0';
			debugTurnStartLinesUtf8_[i][0] = '\0';
		}
		debugActivatedLineCount_ = 0;
		debugTurnStartLineCount_ = 0;
		previewTextChanged = true;
	}

	if (previewTextChanged) {
		ApplyDebugPokerPreviewData_();
	}

	ImGui::End();

	if (fieldUi_) {
		fieldUi_->DrawImGui();
	}

	//battle_.DrawImGui();

#else
	(void)app;
#endif
}

void TitleScene::DrawSkydome(GameApp& app)
{
	app.ObjCom()->SetGraphicsPipelineState();
	if (skyDome_) skyDome_->Draw();
}

void TitleScene::DrawPostEffect3D(GameApp& app)
{

}

void TitleScene::DrawPostEffect2D(GameApp& app)
{

}


//プレビュー用デバッグ
void TitleScene::ApplyDebugPokerPreviewData_()
{
	if (!fieldUi_) {
		return;
	}

	FieldUi::DebugPokerPreviewData debugPreview{};
	debugPreview.enabled = true;
	debugPreview.rank = BattleController::PokerHandRank::ThreeOfAKind;
	debugPreview.atkUp = 7;
	debugPreview.draw = 2;
	debugPreview.damage = 25;

	debugPreview.turnStartLines.clear();
	for (int i = 0; i < debugTurnStartLineCount_ && i < 5; ++i) {
		if (debugTurnStartLinesUtf8_[i][0] != '\0') {
			debugPreview.turnStartLines.push_back(
				Utf8ToWStringLocal_Title(debugTurnStartLinesUtf8_[i].data())
			);
		}
	}

	debugPreview.activatedLines.clear();
	for (int i = 0; i < debugActivatedLineCount_ && i < 5; ++i) {
		if (debugActivatedLinesUtf8_[i][0] != '\0') {
			debugPreview.activatedLines.push_back(
				Utf8ToWStringLocal_Title(debugActivatedLinesUtf8_[i].data())
			);
		}
	}

	fieldUi_->SetDebugPokerPreviewData(debugPreview);
}