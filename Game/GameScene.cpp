#include "GameScene.h"
#include "GameApp.h"
#include "Input.h"
#include "ModelParticleManager.h"
#include <random>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

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
	cameraAnim_->Initialize(animCamera_.get(), app.GetInput());

	ReloadCameraFileList_();

	if (!cameraFiles_.empty()) {
		ChangeRandomCamera();
	}

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
	powerBoostBg_->SetPosition({ 95.0f, 60.0f });
	powerBoostBg_->SetScale({ 32.0f, 32.0f, 1.0f });
	powerBoostBg_->SetColor({ 1.0f, 0.0f, 0.0f, 0.5f });
	powerBoostText_ = std::make_unique<TextSprite>();
	powerBoostText_->Initialize(app.SpriteCom(), app.Dx());
	powerBoostText_->SetSize({ 1.f,1.f,0.5f });
	powerBoostText_->SetPosition({ 88.f, 40.f });

	// ブロック
	blockBg_ = std::make_unique<Sprite>();
	blockBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	blockBg_->SetPosition({ 145.0f, 60.0f });
	blockBg_->SetScale({ 32.0f, 32.0f, 1.0f });
	blockBg_->SetColor({ 0.0f, 0.0f, 1.0f, 0.5f });
	blockText_ = std::make_unique<TextSprite>();
	blockText_->Initialize(app.SpriteCom(), app.Dx());
	blockText_->SetSize({ 1.f,1.f,0.5f });
	blockText_->SetPosition({ 138.f, 40.f });

	TextureManager::GetInstance()->LoadTexture("resources/gradation.png");

	// 1. TrailManagerの初期化（仮にメンバ変数 std::unique_ptr<TrailManager> trailManager_ を追加）
	trailManager_ = std::make_unique<TrailManager>();
	// 軌跡用のテクスチャを指定（とりあえず既存のものでもOK）
	trailManager_->Initialize(app.Dx(), app.ObjCom(), "resources/gradation.png");


	//

	highlightFilter_ = std::make_unique<Sprite>();
	highlightFilter_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	highlightFilter_->SetPosition({ 0.0f, 0.0f });
	highlightFilter_->SetScale({ 1280.0f, 1280.0f, 1.0f });
	highlightFilter_->SetColor({ 0.0f, 0.0f, 0.0f, 0.8f });

	particleManager_ = ModelParticleManager::GetInstance();
	particleManager_->RegisterEffect("sword_trail", "sword_particle.json");
	particleManager_->RegisterEffect("player_fire", "fire_particle.json");
	// 編集用変数に初期値をコピーしておく
	particleManager_->LoadFromJson("fire_particle.json", attackEffectConfig_);

	//AudioManager::GetInstance()->PlayBGM("toumei");
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

	if (cameraAnim_ && cameraAnim_->IsEditing()) {
		cameraAnim_->Update(dt); // カメラの操作だけは受け付ける
		animCamera_->Update();   // カメラ行列更新

		if (skyDome_) {
			skyDome_->SetCamera(animCamera_.get());
		}
		if (player_) {
			player_->SetCamera(animCamera_.get());
			player_->Update(0.0f);
		}

		enemyMgr_.UpdateCamera(animCamera_.get());
		enemyMgr_.Update(0.0f);

		return;
	}

	//if (battle_.IsPlayerTargeting()) {
	//	// カードを触っている時はアニメーションの時間を止めて初期位置に固定する
	//	animCamera_->SetTranslate({ 0.0f, 4.0f, -40.0f });
	//	animCamera_->SetRotate({ 0.15f, 0.0f, 0.0f });
	//} else {
	//	// 触っていない時は、今まで通りアニメーションを再生する
	//	if (cameraAnim_) {
	//		if (cameraAnim_->Update(dt)) {
	//			ChangeRandomCamera();
	//		}
	//	}
	//}

	//bool isTargeting = battle_.IsPlayerTargeting();

	//// 割合を計算（0.2秒かけて 0.0 と 1.0 の間を移動する）
	//if (isTargeting) {
	//	cameraBlend_ += dt * 5.0f;
	//	if (cameraBlend_ > 1.0f) cameraBlend_ = 1.0f;
	//} else {
	//	cameraBlend_ -= dt * 5.0f;
	//	if (cameraBlend_ < 0.0f) cameraBlend_ = 0.0f;
	//}

	//if (cameraAnim_) {
	//	if (isTargeting) {
	//		// ターゲット中はアニメの時間を止める（現在地をキープ）
	//		cameraAnim_->Update(0.0f);
	//	} else {
	//		// ターゲット解除後はアニメを再開する
	//		if (cameraAnim_->Update(dt)) {
	//			ChangeRandomCamera();
	//		}
	//	}
	//}

	bool isTargeting = battle_.IsPlayerTargeting();

	// 割合を計算
	if (isTargeting) {
		cameraBlend_ += dt * 5.0f;
		if (cameraBlend_ > 1.0f) cameraBlend_ = 1.0f;
	} else {
		cameraBlend_ -= dt * 5.0f;
		if (cameraBlend_ < 0.0f) cameraBlend_ = 0.0f;
	}

	if (cameraAnim_) {
		bool finished = false;

		if (isTargeting) {
			// ターゲット中は時間停止
			finished = cameraAnim_->Update(0.0f);
		} else {
			finished = cameraAnim_->Update(dt);
		}

		if (finished) {
			if (randomCameraEnabled_ && !sameCameraLoopEnabled_) {
				ChangeRandomCamera();
			}
		}
	}

	// 割合が 0.0 より大きいなら、アニメの座標と固定座標を混ぜる（Lerp）
	if (cameraBlend_ > 0.0f) {
		Vector3 animPos = animCamera_->GetTranslate();
		Vector3 animRot = animCamera_->GetRotate();

		Vector3 defaultPos = { 0.0f, 4.0f, -40.0f };
		Vector3 defaultRot = { 0.15f, 0.0f, 0.0f };

		float t = cameraBlend_;
		float easeT = t * t * (3.0f - 2.0f * t);

		// アニメの場所(0.0) から 固定位置(1.0) へブレンド
		Vector3 blendedPos = {
			animPos.x + (defaultPos.x - animPos.x) * easeT,
			animPos.y + (defaultPos.y - animPos.y) * easeT,
			animPos.z + (defaultPos.z - animPos.z) * easeT
		};
		Vector3 blendedRot = {
			animRot.x + (defaultRot.x - animRot.x) * easeT,
			animRot.y + (defaultRot.y - animRot.y) * easeT,
			animRot.z + (defaultRot.z - animRot.z) * easeT
		};

		// 混ざったヌルッとした座標をカメラにセット！
		animCamera_->SetTranslate(blendedPos);
		animCamera_->SetRotate(blendedRot);
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

	//// 1. 剣をぶん回すアニメーション（テスト用）
	//static float timer = 0.0f;
	//timer += 0.05f;
	//
	//// 2. ワールド行列から先端と根元の座標を計算
	//// ※Object3dに GetWorldMatrix() がある前提。なければ計算してください
	//Matrix4x4 worldMat = Matrix4x4::MakeScaleMatrix({ 1.0f, 1.0f, 1.0f }) * Matrix4x4::RotateXYZ(0.0f, 0.0, timer) * Matrix4x4::Translation(player_->GetPos());
	//
	//Vector3 localBase = { 0.0f, 0.0f, 0.0f };   // 剣の根元
	//Vector3 localTip = { 0.0f, 6.0f, 0.0f };  // 剣の先端（Scale.yが10ならこのあたり）
	//
	//// ローカル座標をワールド座標へ変換
	//Vector3 worldBase = trailManager_->Transform(localBase, worldMat);
	//Vector3 worldTip = trailManager_->Transform(localTip, worldMat);
	//
	//// 3. 軌跡を更新！
	//trailManager_->Update(worldTip, worldBase, trailConfig_);
	//
	//// 剣の軌跡上に火花を出す
	//Vector3 swordMid = (worldTip + worldBase) * 0.5f;
	//particleManager_->Emit("sword_trail", swordMid, 1);

	particleManager_->Emit("player_fire", player_->GetPos() + Vector3(0, 1.0f, 0), 100);

	// 最後に1回だけDispatch
	particleManager_->Dispatch(1.0f / 60.0f, animCamera_.get());
}


void GameScene::Draw3D(GameApp& app) {
	app.Dx()->SetBackBuffer();   // RTV + DSV を再バインド
	app.Dx()->SetViewport(WinApp::kClientWidth, WinApp::kClientHeight);

	app.ObjCom()->SetGraphicsPipelineState();

	if (player_) player_->Draw();

	// 敵以外のモデルにFilterを書ける
	if (battle_.GetNowCardInputState() == BattleController::CardInputState::ChoosingEnemyTarget) {
		highlightFilter_->Draw();
	}

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


	highlightFilter_->Update(view, proj);

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

		ImGui::Separator();
		ImGui::Text("=== Camera File Browser ===");

		if (ImGui::Button("Reload Camera Files")) {
			ReloadCameraFileList_();
		}

		ImGui::Checkbox("Random Camera Change", &randomCameraEnabled_);
		ImGui::Checkbox("Same Camera Loop", &sameCameraLoopEnabled_);

		// 両方ONは分かりづらいので排他にする
		if (sameCameraLoopEnabled_) {
			randomCameraEnabled_ = false;
		}

		cameraAnim_->SetLoop(sameCameraLoopEnabled_);

		ImGui::Text("Camera Files: %d", static_cast<int>(cameraFiles_.size()));
		ImGui::Text("Current Index: %d", currentCameraIndex_);

		for (int i = 0; i < static_cast<int>(cameraFiles_.size()); ++i) {
			std::string label = fs::path(cameraFiles_[i]).filename().string();
			bool selected = (i == currentCameraIndex_);

			if (ImGui::Selectable(label.c_str(), selected)) {
				LoadCameraByIndex_(i);
			}
		}

		if (ImGui::Button("Play Random Camera")) {
			ChangeRandomCamera();
		}
		if (player_) {
			player_->DrawAnimationEditorImGui();
		}
	}

	ImGui::End();

	// 例えば "player_fire" を編集したい場合
	static std::string targetEffect = "player_fire";

	// コンボボックスで編集対象を切り替えられるようにすると更に便利
	if (ImGui::BeginCombo("Select Edit Effect", targetEffect.c_str())) {
		if (ImGui::Selectable("player_fire")) targetEffect = "player_fire";
		if (ImGui::Selectable("sword_trail")) targetEffect = "sword_trail";
		ImGui::EndCombo();
	}
	
	// マネージャーから指定したエフェクトの設定を編集・反映
	particleManager_->UpdateImGui(targetEffect, attackEffectConfig_);

	if (fieldUi_) {
		ImGui::Begin("FieldUi Debug");
		fieldUi_->DrawImGui();
		ImGui::End();
	}

	AudioManager::GetInstance()->UpdateImGui();

  #endif
}

void GameScene::DrawSkydome(GameApp& app)
{
	app.ObjCom()->SetGraphicsPipelineState();

	if (skyDome_) skyDome_->Draw();

}

void GameScene::DrawPostEffect3D(GameApp& app)
{

	app.ObjCom()->SetGraphicsPipelineState();

	particleManager_->Draw();
	
	//Matrix4x4 vp = camera_->GetViewProjectionMatrix();
	//trailManager_->Draw(vp);
}

void GameScene::DrawPostEffect2D(GameApp& app)
{

}

//============================
//カメラアニメーション
//============================

void GameScene::ChangeRandomCamera() {
	if (!cameraAnim_) return;

	if (cameraFiles_.empty()) {
		ReloadCameraFileList_();
		if (cameraFiles_.empty()) {
			OutputDebugStringA(">>> No camera json files found.\n");
			return;
		}
	}

	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(0, static_cast<int>(cameraFiles_.size()) - 1);

	int randomIndex = dist(gen);

	// 1個しかないならそのまま
	// 複数あるなら同じもの連続を少し避ける
	if (cameraFiles_.size() > 1 && randomIndex == currentCameraIndex_) {
		randomIndex = (randomIndex + 1) % static_cast<int>(cameraFiles_.size());
	}

	LoadCameraByIndex_(randomIndex);
}

void GameScene::ReloadCameraFileList_() {
	cameraFiles_.clear();

	const fs::path folder = "resources/camera";

	if (!fs::exists(folder) || !fs::is_directory(folder)) {
		OutputDebugStringA(">>> Camera folder not found: resources/camera\n");
		return;
	}

	for (const auto& entry : fs::directory_iterator(folder)) {
		if (!entry.is_regular_file()) {
			continue;
		}

		if (entry.path().extension() != ".json") {
			continue;
		}

		cameraFiles_.push_back(entry.path().string());
	}

	std::sort(cameraFiles_.begin(), cameraFiles_.end());

	std::string msg = ">>> Camera files found: " + std::to_string(cameraFiles_.size()) + "\n";
	OutputDebugStringA(msg.c_str());
}

bool GameScene::LoadCameraByPath_(const std::string& path) {
	if (!cameraAnim_) {
		return false;
	}

	if (!cameraAnim_->LoadFromJson(path)) {
		std::string msg = ">>> Failed to load camera: " + path + "\n";
		OutputDebugStringA(msg.c_str());
		return false;
	}

	auto it = std::find(cameraFiles_.begin(), cameraFiles_.end(), path);
	if (it != cameraFiles_.end()) {
		currentCameraIndex_ = static_cast<int>(std::distance(cameraFiles_.begin(), it));
	}

	// ここで再生モードを反映
	cameraAnim_->SetLoop(sameCameraLoopEnabled_);
	cameraAnim_->SetPlaying(true);

	std::string msg = ">>> Camera Loaded: " + path + "\n";
	OutputDebugStringA(msg.c_str());
	return true;
}

bool GameScene::LoadCameraByIndex_(int index) {
	if (index < 0 || index >= static_cast<int>(cameraFiles_.size())) {
		return false;
	}
	return LoadCameraByPath_(cameraFiles_[index]);
}