#include "GameScene.h"
#include "GameApp.h"
#include "Input.h"
#include "ModelParticleManager.h"
#include <random>

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
	camera_->SetTranslate({ 0.0f, 4.0f, -40.0f });
	camera_->SetRotate({ 0.15f, 0.0f, 0.0f });
	app.ObjCom()->SetDefaultCamera(camera_.get());

	animCamera_ = std::make_unique<Camera>();
	animCamera_->SetTranslate({ 0.0f, 4.0f, -40.0f });
	animCamera_->SetRotate({ 0.15f, 0.0f, 0.0f });

	cameraAnim_ = std::make_unique<CameraAnimator>();
	cameraAnim_->Initialize(animCamera_.get());
	ChangeRandomCamera();

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

	// プレイヤーHP数字
	playerHpText_ = std::make_unique<TextSprite>();
	playerHpText_->Initialize(app.SpriteCom(), app.Dx());
	playerHpText_->SetSize({ 1.0f,1.0f,1.0f });
	playerHpText_->SetPosition({ 140.0f, 12.5f });

	// 敵HP数字
	for (int i = 0; i < 3; i++) {
		auto text = std::make_unique<TextSprite>();
		text->Initialize(app.SpriteCom(), app.Dx());
		text->SetSize({ 1.0f,1.0f,1.0f });
		text->SetPosition({ 1000.0f, 40.0f + (i * 30.0f) });
		enemyHpTexts_.push_back(std::move(text));
	}

	// パワーブースト
	powerBoostBg_ = std::make_unique<Sprite>();
	powerBoostBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	powerBoostBg_->SetPosition({ 105.0f, 60.0f });
	powerBoostBg_->SetScale({ 16.0f, 16.0f, 1.0f });
	powerBoostBg_->SetColor({ 1.0f, 0.0f, 0.0f, 0.5f });
	powerBoostText_ = std::make_unique<TextSprite>();
	powerBoostText_->Initialize(app.SpriteCom(), app.Dx());
	powerBoostText_->SetSize({ 0.5f,0.5f,0.5f });
	powerBoostText_->SetPosition({ 100.0f, 50.f });

	// ブロック
	blockBg_ = std::make_unique<Sprite>();
	blockBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	blockBg_->SetPosition({ 145.0f, 60.0f });
	blockBg_->SetScale({ 16.0f, 16.0f, 1.0f });
	blockBg_->SetColor({ 0.0f, 0.0f, 1.0f, 0.5f });
	blockText_ = std::make_unique<TextSprite>();
	blockText_->Initialize(app.SpriteCom(), app.Dx());
	blockText_->SetSize({ 0.5f,0.5f,0.5f });
	blockText_->SetPosition({ 140.0f, 50.f });
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
	if (battle_.IsAllEnemiesDead() || !player_->GetIsAlive()) {
		RequestChangeScene_("Title");
	}

	Input* input = app.GetInput();
	if (!input) return;

	if (cameraAnim_) {
		if (cameraAnim_->Update(dt)) {
			ChangeRandomCamera();
		}
	}

	if (camera_) {
		camera_->Update();     // 固定カメラの更新
	}
	if (animCamera_) {
		animCamera_->Update(); // 動くカメラの更新
	}


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

	// 天球（背景）に動くカメラをセット
	if (skyDome_) {
		skyDome_->SetCamera(animCamera_.get());
	}

	// プレイヤーに動くカメラをセット
	if (player_) {
		player_->SetCamera(animCamera_.get());
	}

	// すべての敵に動くカメラをセット
	for (auto& enemy : enemyMgr_.GetEnemies()) {
		enemy.SetCamera(animCamera_.get());
	}

	battle_.Update(app, *fieldUi_, dt);

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

	if (powerBoostText_) {
		powerBoostText_->SetText(battle_.GetPlayerPowerBoostText());

	}

	if (blockText_) {
		blockText_->SetText(battle_.GetPlayerBlockText());
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
		fieldUi_->Draw(app,battle_);
	}

	if (playerHpText_) {
		playerHpText_->Update(view, proj);
		playerHpText_->Draw();
	}

	if (powerBoostText_) {
		powerBoostText_->Update(view, proj);
		powerBoostText_->Draw();
		powerBoostBg_->Update(view, proj);
		powerBoostBg_->Draw();
	}
	if (blockText_) {
		blockText_->Update(view, proj);
		blockText_->Draw();
		blockBg_->Update(view, proj);
		blockBg_->Draw();
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

	if (cameraAnim_) {
		cameraAnim_->DrawImGui();
	}
	if (ImGui::Button("Test: Change Camera!")) {
		ChangeRandomCamera();
	}
	if (fieldUi_) {
		fieldUi_->DrawImGui();
	}

	ImGui::End();




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

void GameScene::ChangeRandomCamera() {
	if (!cameraAnim_) return;

	// 1〜3のランダムな数字を作る
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(1, 3);
	int randomId = dist(gen);

	// ファイル名を組み立てて読み込む
	std::string filepath = "resources/camera/camera_idle_" + std::to_string(randomId) + ".json";

	std::string msg = ">>> Camera Changed: " + filepath + "\n";
	OutputDebugStringA(msg.c_str());

	cameraAnim_->LoadFromJson(filepath);
}