#include "TitleScene.h"

#include "GameApp.h"
#include "Camera.h"
#include "WinApp.h"
#include "TextureManager.h"
#include "TutorialManager.h"
#include "Card3D.h"
#include "CardDatabase.h"
#include "AudioManager.h"



//------------------------------------------------------------
// シーン開始時の初期化
//------------------------------------------------------------
void TitleScene::OnEnter(GameApp& app) {
	// タイトル状態を初期化
	state_ = State::Idle;
	circle_ = 1.0f;
	softness_ = 0.6f;
	openingDissolveTimer_ = 0.0f;
	openingDissolveDone_ = false;

	//--------------------------------------------------------
	// カメラ作成
	//--------------------------------------------------------
	camera_ = std::make_unique<Camera>();
	camera_->SetTranslate({ 0.0f, 4.0f, -40.0f });
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
	// Title Logo画像
	//--------------------------------------------------------
	titleLogo_ = std::make_unique<Sprite>();
	titleLogo_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/resonance_title.png");
	titleLogo_->SetAnchorPoint({ 0.0f, 0.0f });
	titleLogo_->SetPosition({ 150.0f, 100.0f }); // あとでImGuiで調整可能
	titleLogo_->SetScale({ 1.0f, 1.0f, 1.0f });

	//--------------------------------------------------------
	// Click Start画像
	//--------------------------------------------------------
	clickStart_ = std::make_unique<Sprite>();
	clickStart_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/clickStart.png");
	clickStart_->SetAnchorPoint({ 0.0f, 0.0f });

	// 位置はあとで調整しやすいように中央下寄りに置く
	clickStart_->SetPosition({ 430.0f, 560.0f });
	clickStart_->SetScale({ 1.0f, 1.0f, 1.0f });


	AudioManager::GetInstance()->PlaySE("SE_SoundLogo");

	//--------------------------------------------------------
	// 起動時ディソルブ用の黒い全面スプライト
	//--------------------------------------------------------
	dissolveFade_ = std::make_unique<Sprite>();
	dissolveFade_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	dissolveFade_->SetAnchorPoint({ 0.0f, 0.0f });
	dissolveFade_->SetPosition({ 0.0f, 0.0f });
	const DirectX::TexMetadata& whiteMeta =
		TextureManager::GetInstance()->GetMetaData("resources/ui/white.png");
	dissolveFade_->SetScale({
		float(WinApp::kClientWidth) / float(whiteMeta.width),
		float(WinApp::kClientHeight) / float(whiteMeta.height),
		1.0f
		});
	dissolveFade_->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });

	//AudioManager::GetInstance()->PlayBGM("machi");

}

//------------------------------------------------------------
// シーン終了時の解放
//------------------------------------------------------------
void TitleScene::OnExit(GameApp&) {
	dissolveFade_.reset();
	clickStart_.reset();
	titleLogo_.reset();
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
	// クリックの瞬間 (左または右)
	//--------------------------------------------------------
	bool clickTrig = input->IsMouseTrigger(0) || input->IsMouseTrigger(1);
	
	// 画面内をクリックしたか判定
	bool isInsideWindow = false;
	POINT mousePos = input->GetMousePosition();
	if (mousePos.x >= 0 && mousePos.x < WinApp::kClientWidth && 
		mousePos.y >= 0 && mousePos.y < WinApp::kClientHeight) {
		isInsideWindow = true;
	}

	bool tutorialTrig = input->IsKeyTrigger(DIK_T);
	bool deckEditTrig = input->IsKeyTrigger(DIK_D);
	bool objectPostTestTrig = input->IsKeyTrigger(DIK_O);

	if (!openingDissolveDone_) {
		openingDissolveTimer_ += dt;
		if (openingDissolveTimer_ >= openingDissolveDuration_) {
			openingDissolveTimer_ = openingDissolveDuration_;
			openingDissolveDone_ = true;
		}

		auto& param = app.ObjectPost()->GetParam();
		param.dissolveAmount = openingDissolveTimer_ / openingDissolveDuration_;
		param.dissolveEdgeWidth = 0.09f;
		param.dissolveEdgeIntensity = 2.6f;
		param.dissolveNoiseScale = 34.0f;
		param.dissolveEdgeColor = { 0.10f, 0.95f, 1.0f, 1.0f };
		param.intensity = 1.0f;
		param.chromAbAmount = 0.004f;
		param.distortionAmount = 0.0015f;
		param.noiseIntensity = 0.0f;
	} else {
		app.ObjectPost()->GetParam().dissolveAmount = -1.0f;
	}

	//--------------------------------------------------------
	// 状態更新
	//--------------------------------------------------------
	switch (state_) {
	case State::Idle:
		// 画面内をクリックしたら閉じ演出へ
		if (clickTrig && isInsideWindow) {
			AudioManager::GetInstance()->PlaySE("SE_Tap");
			state_ = State::ExitClose;
		}

		if (tutorialTrig) {
			AudioManager::GetInstance()->PlaySE("SE_Tap");
			RequestChangeScene_("Tutorial");
			return;
		}

		if (deckEditTrig) {
			AudioManager::GetInstance()->PlaySE("SE_Tap");
			RequestChangeScene_("DeckEdit");
			return;
		}

		if (objectPostTestTrig) {
			RequestChangeScene_("Test");
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
	// Title Logo描画
	//--------------------------------------------------------
	if (titleLogo_) {
		titleLogo_->Update(view, proj);
		titleLogo_->Draw();
	}

	//--------------------------------------------------------
	// Click Start描画
	//--------------------------------------------------------
	if (clickStart_) {
		clickStart_->Update(view, proj);
		clickStart_->Draw();
	}

	//--------------------------------------------------------
	// 起動時ディソルブフェード
	//--------------------------------------------------------
	if (!openingDissolveDone_ && dissolveFade_) {
		dissolveFade_->Update(view, proj);

		app.BeginObjectPostEffect();
		dissolveFade_->Draw();
		app.EndObjectPostEffect();

		app.SpriteCom()->SetGraphicsPipelineState();
	}

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
