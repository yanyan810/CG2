#include "GameScene.h"
#include "GameApp.h"
#include "Input.h"
#include "ModelParticleManager.h"

static std::wstring Utf8ToWString(const std::string& s)
{
	if (s.empty()) return L"";
	int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring out(size - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), size);
	return out;
}

void GameScene::OnEnter(GameApp& app) {
	// --------------------------------------------------
	// 1. カメラの初期化と設定
	// --------------------------------------------------
	camera_ = std::make_unique<Camera>();

	// ★カメラを原点(0, 0, 0)に配置し、少しだけ見下ろす角度に
	camera_->SetTranslate({ 0.0f, 4.0f, -40.0f }); // 高さを4.0fにして見下ろす
	camera_->SetRotate({ 0.15f, 0.0f, 0.0f });     // 軽く見下ろす角度
	app.ObjCom()->SetDefaultCamera(camera_.get());

	// --------------------------------------------------
	// 2. 背景（天球）の初期化
	// --------------------------------------------------
	skyDome_ = std::make_unique<Object3d>();
	skyDome_->Initialize(app.ObjCom(), app.Dx());
	skyDome_->SetModel("skydome/skydome.obj");
	skyDome_->SetCamera(camera_.get());
	skyDome_->SetEnableLighting(0);
	// ★カメラが原点になったので、天球の中心も原点にする
	skyDome_->SetTranslate({ 0.0f, 0.0f, 0.0f });
	skyDome_->SetScale({ 100.0f, 100.0f, 100.0f });

	// --------------------------------------------------
	// 3. プレイヤーとエネミーの初期化・配置
	// --------------------------------------------------
	// カメラが原点なので、キャラクターは Z 方向に押し出してカメラの前に置く
	const float charZ = 15.0f; // カメラから15.0f 奥

	// プレイヤーの配置（左側・右向き）
	player_ = std::make_unique<Player>();
	player_->Initialize(app.ObjCom(), app.Dx(), camera_.get());
	player_->SetSpawnPos({ -7.0f, 0.0f, charZ });
	player_->SetRotation({ 0.0f, 1.5708f, 0.0f });

	// エネミーの配置（右側・左向き）
	enemyMgr_.Initialize(app.ObjCom(), app.Dx(), camera_.get());
	enemyMgr_.Spawn(EnemyType::Slime, { 7.0f, 0.0f, 5.0f }); // 奥にスライム
	enemyMgr_.Spawn(EnemyType::Boss, { 7.0f, 0.0f,  15.0f }); // 真ん中にボス
	enemyMgr_.Spawn(EnemyType::Slime, { 7.0f, 0.0f,  25.0f }); // 手前にスライム
	// --------------------------------------------------
	// 4. ライトの初期設定
	// --------------------------------------------------
	light_.lightingMode = 1;
	light_.dir = { 0.3f, -1.0f, 0.2f };
	light_.dirIntensity = 1.5f;
	light_.dirColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	// --------------------------------------------------
   //  5. バトルコントローラー
   // --------------------------------------------------
	battle_.SetPlayer(player_.get());
	battle_.SetEnemyManager(&enemyMgr_);
	battle_.Initialize(app, camera_.get());

	// --------------------------------------------------
	// 6. 文字描画の初期化
	// --------------------------------------------------

	cardDescText_ = std::make_unique<TextSprite>();
	cardDescText_->Initialize(app.SpriteCom(), app.Dx());
	cardDescText_->SetPosition({ 40.0f, 620.0f });

	//白テクスチャ
	cardDescBg_ = std::make_unique<Sprite>();
	cardDescBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	cardDescBg_->SetPosition({ 20.0f, 60.0f });
	cardDescBg_->SetScale({ 900.0f, 180.0f, 1.0f });
	cardDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

	fieldUi_ = std::make_unique<FieldUi>();
	fieldUi_->Initialize(app);

	//// ターン数描画関連
	//turnText_ = std::make_unique<TextSprite>();
	//turnText_->Initialize(app.SpriteCom(), app.Dx());
	//turnText_->SetSize({ 1.0f,1.0f,1.0f });
	//turnText_->SetPosition({ 500.0f, 20.0f });
	//turnTextBg_ = std::make_unique<Sprite>();
	//turnTextBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	//turnTextBg_->SetPosition({ 490.f,25.f });
	//turnTextBg_->SetScale({ 250.f,60.f,1.f });
	//turnTextBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

	//// コスト描画関連
	//position_ = { 90.f,420.f };
	//scale_ = { 900.f,180.f,1.f };

	//costText_ = std::make_unique<TextSprite>();
	//costText_->Initialize(app.SpriteCom(), app.Dx());
	//costText_->SetSize({ 1.0f,1.0f,1.0f });
	//costText_->SetPosition({ 90.0f, 400.0f });
	//costTextBg_ = std::make_unique<Sprite>();
	//costTextBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	//costTextBg_->SetPosition({ 75.f,405.f });
	//costTextBg_->SetScale({ 170.f,55.f,1.f });
	//costTextBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

	// プレイヤーHP数字
	playerHpText_ = std::make_unique<TextSprite>();
	playerHpText_->Initialize(app.SpriteCom(), app.Dx());
	playerHpText_->SetSize({ 1.0f,1.0f,1.0f });
	playerHpText_->SetPosition({ 140.0f, 12.5f});

	// 敵HP数字
	for (int i = 0; i < 3; i++) {
		auto text = std::make_unique<TextSprite>();
		text->Initialize(app.SpriteCom(), app.Dx());
		text->SetSize({ 1.0f,1.0f,1.0f });
		text->SetPosition({ 1000.0f, 40.0f + (i * 30.0f) });
		enemyHpTexts_.push_back(std::move(text));
	}
}

void GameScene::OnExit(GameApp& app) {
	fieldUi_.reset();
	cardDescBg_.reset();
	cardDescText_.reset();

	player_.reset();
	skyDome_.reset();
	camera_.reset();
	battle_.Finalize();
	// EnemyManager に Clear() があるなら呼ぶ
	// enemyMgr_.Clear();

	// battle_ に明示的な解放関数を作るのが理想
	// battle_.Finalize();
}
void GameScene::Update(GameApp& app, float dt) {
	if (battle_.IsAllEnemiesDead()||!player_->GetIsAlive()) {
		RequestChangeScene_("Title");
	}

	Input* input = app.GetInput();
	if (!input) return;

	

	camera_->Update();

	// ESCキーでタイトルへ戻る
	bool currEsc = input->IsKeyPressed(DIK_ESCAPE);
	if (currEsc && !prevEsc_) {
		RequestChangeScene_("Title");
	}
	prevEsc_ = currEsc;

	if (skyDome_) {
		skyDome_->Update(dt);
	}

	if (player_) {
		player_->Update(dt);
	}

	enemyMgr_.Update(dt);
	enemyMgr_.SetLighting(light_);

	battle_.Update(app, *fieldUi_,dt);

	if (battle_.HasPokerChoiceUi()) {
		cardDescText_->SetSize({ 1.0f,1.0f,1.0f });
		cardDescText_->SetPosition({ 40.0f, 80.0f });
		cardDescText_->SetText(battle_.GetPokerChoiceUiText());

		cardDescBg_->SetPosition({ 20.0f, 52.0f });
		cardDescBg_->SetScale({ 900.0f, 180.0f, 1.0f });
	} else {
		cardDescText_->SetSize({ 1.0f,1.0f,1.0f });
		cardDescText_->SetPosition({ 40.0f, 620.0f });

		const CardDef* def = battle_.GetPreviewCardDef();
		if (def) {
			cardDescText_->SetText(Utf8ToWString(def->desc));

			cardDescBg_->SetPosition({ 20.0f, 600.0f });
			cardDescBg_->SetScale({ 900.0f, 120.0f, 1.0f });
		} else if (battle_.ShouldShowOperationUi()) {
			cardDescText_->SetPosition({ 40.0f, 520.0f });
			cardDescText_->SetText(battle_.GetOperationUiText());

			cardDescBg_->SetPosition({ 20.0f, 500.0f });
			cardDescBg_->SetScale({ 900.0f, 280.0f, 1.0f });
		} else {
			cardDescText_->SetText(L"");
		}
	}

	if (fieldUi_) {
		fieldUi_->Update(app, battle_);
	}



	if (costText_) {
		costText_->SetText(battle_.GetEnergyText());
	}

	if (playerHpText_) {
		playerHpText_->SetText(battle_.GetPlayerHpTexts());
	}

	std::vector<std::wstring> hpData = battle_.GetEnemyHpTexts();

	for (size_t i = 0; i < enemyHpTexts_.size(); i++) {
		if (i < hpData.size()) {
			enemyHpTexts_[i]->SetText(hpData[i]);

			enemyHpTexts_[i]->SetPosition({ 1025.0f, 10.0f + (i * 30.0f) });
		} else {
			
			enemyHpTexts_[i]->SetText(L"");
		}
	}

	ModelParticleManager::GetInstance()->Update(1.0f / 60.0f, camera_.get());
}


void GameScene::Draw3D(GameApp& app) {
	app.ObjCom()->SetGraphicsPipelineState();

	battle_.Draw3D(app);

	enemyMgr_.Draw();

}

void GameScene::Draw2D(GameApp& app) {
	app.SpriteCom()->SetGraphicsPipelineState();

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
		0, 0,
		float(WinApp::kClientWidth),
		float(WinApp::kClientHeight),
		0, 100
	);

	bool showDescBg = false;

	if (battle_.HasPokerChoiceUi()) {
		showDescBg = true;
	} else {
		const CardDef* def = battle_.GetPreviewCardDef();
		showDescBg = (def != nullptr) || battle_.ShouldShowOperationUi();
	}

	battle_.Draw2D(app);

	if (fieldUi_) {
		fieldUi_->Draw(app);
	}

	if (playerHpText_) {
		playerHpText_->Update(view, proj);
		playerHpText_->Draw();
	}

	for (auto& text : enemyHpTexts_) {
		text->Update(view, proj);
		text->Draw();
	}
}

void GameScene::DrawImGui(GameApp& app) {
#ifdef USE_IMGUI
	ImGui::Begin("Battle Debug");

	battle_.DrawImGui();

	ImGui::DragFloat2("position", &position_.x);
	ImGui::DragFloat3("scale", &scale_.x);

	//playerHpText_->SetPosition(position_);

	ImGui::End();

	if (fieldUi_) {
		fieldUi_->DrawImGui();
	}


#endif
}

void GameScene::DrawSkydome(GameApp& app)
{
	app.ObjCom()->SetGraphicsPipelineState();

	if (skyDome_) skyDome_->Draw();

	if (player_) player_->Draw();

}

void GameScene::DrawPostEffect3D(GameApp& app)
{

	app.ObjCom()->SetGraphicsPipelineState();


	ModelParticleManager::GetInstance()->Draw();




}

void GameScene::DrawPostEffect2D(GameApp& app)
{

}
