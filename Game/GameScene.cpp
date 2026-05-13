#include "GameScene.h"
#include "GameApp.h"
#include "Input.h"
#include "ModelParticleManager.h"
#include "AnimationJsonSerializer.h"
#include "AudioManager.h"
#include <fstream>
#include <random>
#include <filesystem>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

static std::wstring Utf8ToWString(const std::string& s)
{
	if (s.empty()) return L"";
	int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring out(size - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), size);
	return out;
}

namespace {
	EnemyType ParseEnemyType_(const std::string& type)
	{
		if (type == "Boss" || type == "boss") {
			return EnemyType::Boss;
		}
		return EnemyType::Slime;
	}

	Vector3 ReadEnemyPosition_(const json& enemyJson)
	{
		Vector3 pos{ 7.0f, 0.0f, 15.0f };
		if (!enemyJson.contains("position") || !enemyJson["position"].is_object()) {
			return pos;
		}

		const auto& jPos = enemyJson["position"];
		pos.x = jPos.value("x", pos.x);
		pos.y = jPos.value("y", pos.y);
		pos.z = jPos.value("z", pos.z);
		return pos;
	}

	bool LoadStageEnemyConfigs_(
		const std::string& path,
		std::vector<StageEnemyConfig>& outEnemies,
		std::string& outBgmId,
		bool& outIsBossStage)
	{
		outEnemies.clear();
		outBgmId.clear();
		outIsBossStage = false;
		if (path.empty()) {
			return false;
		}

		std::ifstream ifs(path);
		if (!ifs.is_open()) {
			return false;
		}

		json root;
		try {
			ifs >> root;
		} catch (...) {
			outEnemies.clear();
			return false;
		}

		if (!root.contains("enemies") || !root["enemies"].is_array()) {
			return false;
		}

		outBgmId = root.value("bgmId", "");
		bool hasBossEnemy = false;

		for (const auto& enemyJson : root["enemies"]) {
			if (!enemyJson.is_object()) {
				continue;
			}

			StageEnemyConfig config{};
			config.type = ParseEnemyType_(enemyJson.value("enemyType", "Slime"));
			config.position = ReadEnemyPosition_(enemyJson);
			config.maxHp = enemyJson.value("maxHp", -1);
			config.hp = enemyJson.value("hp", -1);
			config.behaviorJson = enemyJson.value("behaviorJson", "");
			hasBossEnemy = hasBossEnemy || enemyJson.value("bossFlag", false);
			outEnemies.push_back(config);
		}

		outIsBossStage = root.value("isBossStage", false) || hasBossEnemy;
		return !outEnemies.empty();
	}

	void SpawnDefaultEnemies_(EnemyManager& enemyMgr)
	{
		enemyMgr.Spawn(EnemyType::Slime, { 7.0f, 0.0f, 5.0f });
		enemyMgr.Spawn(EnemyType::Boss, { 7.0f, 0.0f, 15.0f });
		enemyMgr.Spawn(EnemyType::Slime, { 7.0f, 0.0f, 25.0f });
	}
}

void GameScene::OnEnter(GameApp& app) {
	isBossStage_ = false;
	bossStageBannerTimer_ = 0.0f;

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

#ifndef _DEBUG
	if (player_ && player_->GetObject3d() && player_->GetObject3d()->GetModel()) {
		struct CustomAnimationFile {
			const char* path;
			const char* name;
		};

		static const CustomAnimationFile kCustomAnimationFiles[] = {
			{ "resources/CustomAnim/CustomAnim.json", "CustomAnim" },
			{ "resources/CustomAnim/CustomAnim_attack_1.json", "CustomAnim_attack_1" },
			{ "resources/CustomAnim/CustomAnim_attack_2.json", "CustomAnim_attack_2" },
			{ "resources/CustomAnim/CustomAnim_attack_3.json", "CustomAnim_attack_3" },
			{ "resources/CustomAnim/CustomAnim_attack_received_1.json", "CustomAnim_attack_received_1" },
			{ "resources/CustomAnim/CustomAnim_attack_received_2.json", "CustomAnim_attack_received_2" },
		};

		bool loadedDefaultCustomAnim = false;
		for (const auto& customAnimationFile : kCustomAnimationFiles) {
			Animation animation{};
			if (!AnimationJsonSerializer::LoadFromJson(customAnimationFile.path, animation)) {
				continue;
			}

			player_->GetObject3d()->GetModel()->AddAnimation(customAnimationFile.name, animation);
			if (std::string(customAnimationFile.name) == "CustomAnim") {
				loadedDefaultCustomAnim = true;
			}
		}

		if (loadedDefaultCustomAnim) {
			player_->GetObject3d()->PlayAnimation("CustomAnim", true);
		}
	}
#endif

	animationEditTarget_ = player_ ? player_->GetObject3d() : nullptr;
	cameraEditTarget_ = animCamera_.get();

	// エネミーの配置（右側・左向き）
	enemyMgr_.Initialize(app.ObjCom(), app.Dx(), camera_.get());
	std::vector<StageEnemyConfig> stageEnemies;
	std::string stageBgmId;
	bool stageIsBoss = false;
	if (LoadStageEnemyConfigs_(app.GetSelectedStageConfigPath(), stageEnemies, stageBgmId, stageIsBoss)) {
		isBossStage_ = stageIsBoss;
		for (const StageEnemyConfig& config : stageEnemies) {
			enemyMgr_.SpawnWithConfig(config);
		}
	} else {
		isBossStage_ = false;
		SpawnDefaultEnemies_(enemyMgr_);
	}
	bossStageBannerTimer_ = isBossStage_ ? 3.0f : 0.0f;

	AudioManager::GetInstance()->PlayBGM(stageBgmId.empty() ? "BGM_Game" : stageBgmId);
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

	// フィールドUI
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

		auto poisonText = std::make_unique<TextSprite>();
		poisonText->Initialize(app.SpriteCom(), app.Dx());
		poisonText->SetSize({ 1.0f,1.0f,1.0f });
		poisonText->SetColor({ 0.5f, 0.0f, 0.5f }); // 紫色
		enemyPoisonTexts_.push_back(std::move(poisonText));

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
	// 軌跡インスタンスを作成
	testTrail_ = trailManager_->CreateInstance();
	testTrail_->SetIsPermanent(true);
	player_->SetTrailInstance(testTrail_);

	highlightFilter_ = std::make_unique<Sprite>();
	highlightFilter_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	highlightFilter_->SetPosition({ 0.0f, 0.0f });
	highlightFilter_->SetScale({ 1280.0f, 1280.0f, 1.0f });
	highlightFilter_->SetColor({ 0.0f, 0.0f, 0.0f, 0.8f });

	bossStageBannerBg_ = std::make_unique<Sprite>();
	bossStageBannerBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	bossStageBannerBg_->SetPosition({ 380.0f, 105.0f });
	bossStageBannerBg_->SetScale({ 520.0f, 86.0f, 1.0f });
	bossStageBannerBg_->SetColor({ 0.18f, 0.02f, 0.02f, 0.78f });

	bossStageBannerText_ = std::make_unique<TextSprite>();
	bossStageBannerText_->Initialize(app.SpriteCom(), app.Dx());
	bossStageBannerText_->SetText(L"BOSS STAGE");
	bossStageBannerText_->SetFontSize(56);
	bossStageBannerText_->SetColor({ 1.0f, 0.78f, 0.2f });
	bossStageBannerText_->SetPosition({ 430.0f, 120.0f });
	bossStageBannerText_->SetSize({ 1.0f, 1.0f, 1.0f });

	particleManager_ = ModelParticleManager::GetInstance();
	particleManager_->RegisterEffect("sword_trail", "sword_particle.json");
	particleManager_->RegisterEffect("player_fire", "fire_particle.json");
	particleManager_->RegisterEffect("fireExplosive", "fireExplosive.json");
	particleManager_->RegisterEffect("particle_image", "0.json");
	
	particleManager_->RegisterEffect("Vacuum_Fly", "Vacuum_Fly.json");
	particleManager_->RegisterEffect("Vacuum_Hit", "Vacuum_Hit.json");
	
	particleManager_->RegisterEffect("Flare_Fly", "Flare_Fly.json");
	particleManager_->RegisterEffect("Flare_Hit", "Flare_Hit.json");
	
	particleManager_->RegisterEffect("Air_Fly", "Air_Fly.json");
	particleManager_->RegisterEffect("Air_Hit", "Air_Hit.json");

	// 編集用変数に初期値をコピーしておく
	particleManager_->LoadFromJson("fire_particle.json", attackEffectConfig_);
	ResetParticleObjectPostParam_();

	// 軌跡の見た目の設定
	TrailConfig config;
	// 軌跡の設定を大幅に強化
	trailConfig_.maxPoints = 200;           // 記録数を増やす（50だと一瞬で終わります）
	trailConfig_.interpolationSteps = 8;     // 補間を増やして密度を上げる
	trailConfig_.startColor = { 1, 1, 1, 1 };  // 最初はハッキリ白
	trailConfig_.endColor = { 1, 0, 0, 0.2f }; // 最後まで少し色を残す
	player_->SetTrailConfig(config);

	// エフェクトシーケンサーの初期化（GameScene用）
	effectSequencer_ = std::make_unique<EffectSequencer>();
	effectSequencer_->Initialize(
		app.ObjCom(), app.Dx(), animCamera_.get(),
		particleManager_, trailManager_.get()
	);

	// プレイヤーのEffectSequencerを再初期化（TrailManager/particleManagerが揃った後）
	if (player_) {
		player_->GetEffectSequencer().Initialize(
			app.ObjCom(), app.Dx(), animCamera_.get(),
			particleManager_, trailManager_.get()
		);

		// デフォルトの攻撃技を登録（JSONファイル名は実際のファイルに合わせて変更）
		player_->AddAttackMove({ "CustomAnim_attack_1", "attack_1.json", 0.1f });
		player_->AddAttackMove({ "CustomAnim_attack_2", "attack_2.json", 0.15f });
		player_->AddAttackMove({ "CustomAnim_attack_3", "attack_3.json", 0.2f });
	}

	pausingUI_ = std::make_unique<PausingUI>();
	pausingUI_->Initialize(app);
}

void GameScene::OnExit(GameApp& app) {
	fieldUi_.reset();
	bossStageBannerText_.reset();
	bossStageBannerBg_.reset();
	cardDescBg_.reset();
	cardDescText_.reset();
	isBossStage_ = false;
	bossStageBannerTimer_ = 0.0f;

	animationEditTarget_ = nullptr;
	cameraEditTarget_ = nullptr;
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
	if (battle_.IsAllEnemiesDead() || !player_->GetIsAlive() || pausingUI_->GetIsSceneChangeRequested()) {
		battleEndTimer_--;
	}

	if (battleEndTimer_ <= 0) {
		if (battle_.IsAllEnemiesDead()) {
			RequestChangeScene_("GameClear");
		}
		if (!player_->GetIsAlive()) {
			RequestChangeScene_("GameOver");
		}
		if (pausingUI_->GetIsSceneChangeRequested()) {
			RequestChangeScene_("Title");
		}
	}

	Input* input = app.GetInput();
	if (!input) return;

	pausingUI_->Update(app, input);

	if (pausingUI_->GetIsPaused()) {
		return;
	}

	if (bossStageBannerTimer_ > 0.0f) {
		bossStageBannerTimer_ -= dt;
		if (bossStageBannerTimer_ < 0.0f) {
			bossStageBannerTimer_ = 0.0f;
		}
	}

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
		int windowW = WinApp::kClientWidth;
		int windowH = WinApp::kClientHeight;
		int fieldHeight = windowH - static_cast<int>(windowH * splitRatio_);
		camera_->SetAspect((float)windowW / fieldHeight);

		// FOVと角度の補正 (元のサイズ感を維持しつつ見切れないようにする)
		float origFovY = 0.45f;
		float zoomRatio = ((float)fieldHeight / windowH) / fieldCameraZoom_;
		float newFovY = 2.0f * std::atan(zoomRatio * std::tan(origFovY / 2.0f));
		camera_->SetFovY(newFovY);

		// 少し下を向けてカードが見切れないようにする
		Vector3 rot = camera_->GetRotate();
		rot.x = 0.15f + fieldCameraRotXOffset_;
		camera_->SetRotate(rot);

		Matrix4x4 shiftField = Matrix4x4::MakeIdentity4x4();
		shiftField.m[1][1] = 1.0f - splitRatio_;
		shiftField.m[3][1] = -splitRatio_;
		camera_->SetProjectionShift(shiftField);

		camera_->Update();     // 固定カメラの更新
	}
	if (animCamera_) {
		int windowW = WinApp::kClientWidth;
		int windowH = WinApp::kClientHeight;
		int battleHeight = static_cast<int>(windowH * splitRatio_);
		animCamera_->SetAspect((float)windowW / battleHeight);

		// FOVと角度の補正
		float origFovY = 0.45f;
		float zoomRatio = ((float)battleHeight / windowH) / battleCameraZoom_;
		float newFovY = 2.0f * std::atan(zoomRatio * std::tan(origFovY / 2.0f));
		animCamera_->SetFovY(newFovY);

		// 少し上を向けてキャラクターの頭が見切れないようにする
		Vector3 rot = animCamera_->GetRotate();
		rot.x += battleCameraRotXOffset_;
		animCamera_->SetRotate(rot);

		Matrix4x4 shiftBattle = Matrix4x4::MakeIdentity4x4();
		shiftBattle.m[1][1] = splitRatio_;
		shiftBattle.m[3][1] = 1.0f - splitRatio_;
		animCamera_->SetProjectionShift(shiftBattle);

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
	if (Camera* actionCamera = battle_.GetActionCamera()) {
		int windowW = WinApp::kClientWidth;
		int windowH = WinApp::kClientHeight;
		int battleHeight = static_cast<int>(windowH * splitRatio_);
		actionCamera->SetAspect((float)windowW / battleHeight);

		float zoomRatio = ((float)battleHeight / windowH) / battleCameraZoom_;
		float correctedFovY = 2.0f * std::atan(zoomRatio * std::tan(actionCamera->GetFovY() / 2.0f));
		actionCamera->SetFovY(correctedFovY);

		Matrix4x4 shiftBattle = Matrix4x4::MakeIdentity4x4();
		shiftBattle.m[1][1] = splitRatio_;
		shiftBattle.m[3][1] = 1.0f - splitRatio_;
		actionCamera->SetProjectionShift(shiftBattle);
		actionCamera->Update();
	}

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
	std::vector<std::wstring> poisonData = battle_.GetEnemyPoisonTexts();

	for (size_t i = 0; i < enemyHpTexts_.size(); i++) {
		if (i < hpData.size()) {
			enemyHpTexts_[i]->SetText(hpData[i]);

			enemyHpTexts_[i]->SetPosition({ 1025.0f, 10.0f + (i * 30.0f) });
		} else {

			enemyHpTexts_[i]->SetText(L"");
		}
	}


	for (size_t i = 0; i < enemyPoisonTexts_.size(); i++) {
		if (i < poisonData.size()) {
			enemyPoisonTexts_[i]->SetText(poisonData[i]);

			enemyPoisonTexts_[i]->SetPosition({ 1200.0f, 10.0f + (i * 30.0f) });
		} else {

			enemyPoisonTexts_[i]->SetText(L"");
		}
	}

	// 1. マネージャ自体の更新（不要になったインスタンスの自動削除など）
	trailManager_->Update(dt);

	//particleManager_->Emit("particle_image", Vector3(0, 0.0f, 0), 10);

	// エフェクトシーケンサーの更新
	if (effectSequencer_) {
		effectSequencer_->Update(dt);
	}

	// 最後に1回だけDispatch
	particleManager_->Dispatch(1.0f / 60.0f, animCamera_.get());
}


void GameScene::Draw3D(GameApp& app) {
	app.Dx()->SetBackBuffer();

	int windowW = WinApp::kClientWidth;
	int windowH = WinApp::kClientHeight;
	int battleHeight = static_cast<int>(windowH * splitRatio_);

	app.Dx()->SetViewport(0, 0, windowW, windowH);

	app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);
	app.ObjCom()->SetGraphicsPipelineState();

	if (player_) player_->Draw();
	enemyMgr_.Draw();

	// 敵以外のモデルにFilterを書ける (バトル画面側)
	if (battle_.GetNowCardInputState() == BattleController::CardInputState::ChoosingEnemyTarget) {
		highlightFilter_->Draw();
	}

	app.Dx()->SetScissorRect(0, 0, windowW, windowH);
	app.ObjCom()->SetGraphicsPipelineState();

	battle_.Draw3D(app);
	battle_.DrawPostEffect3D(app);

	// 最後にビューポートを元に戻す（2D描画等のため）
	app.Dx()->SetViewport(windowW, windowH);
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

	pausingUI_->Draw(app);

	if (pausingUI_->GetIsPaused()) {
		return;
	}

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
		fieldUi_->Draw(app, battle_);
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

	if (isBossStage_ && bossStageBannerTimer_ > 0.0f) {
		const float alpha = bossStageBannerTimer_ < 1.0f ? bossStageBannerTimer_ : 1.0f;
		if (bossStageBannerBg_) {
			bossStageBannerBg_->SetColor({ 0.18f, 0.02f, 0.02f, 0.78f * alpha });
			bossStageBannerBg_->Update(view, proj);
			bossStageBannerBg_->Draw();
		}
		if (bossStageBannerText_) {
			bossStageBannerText_->SetAlpha(alpha);
			bossStageBannerText_->Update(view, proj);
			bossStageBannerText_->Draw();
		}
	}


	for (auto& text : enemyPoisonTexts_) {
		text->Update(view, proj);
		text->Draw();
	}
}

void GameScene::DrawImGui(GameApp& app) {
#ifdef USE_IMGUI
	ImGui::Begin("UI Visibility", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Checkbox("Battle Debug", &battleDebugVisible_);
	ImGui::Checkbox("Battle Effects", &battleEffectsDebugVisible_);
	ImGui::Separator();
	ImGui::TextUnformatted("Animation Editor");
	auto& editorWindows = animationEditor_.GetWindowVisibility();
	ImGui::Checkbox("Toolbar", &editorWindows.toolbar);
	ImGui::Checkbox("Hierarchy", &editorWindows.hierarchy);
	ImGui::Checkbox("Inspector", &editorWindows.inspector);
	ImGui::Checkbox("Timeline", &editorWindows.timeline);
	ImGui::Checkbox("Preview", &editorWindows.preview);
	ImGui::End();

	ImGui::Begin("Camera Setup");
	ImGui::SliderFloat("CameraBlend", &cameraBlend_, 0.0f, 1.0f);
	ImGui::SliderFloat("Split Ratio", &splitRatio_, 0.1f, 0.9f);
	ImGui::SliderFloat("Field Camera Zoom", &fieldCameraZoom_, 0.1f, 3.0f);
	ImGui::SliderFloat("Field Camera RotX Offset", &fieldCameraRotXOffset_, -0.5f, 0.5f);
	ImGui::SliderFloat("Battle Camera Zoom", &battleCameraZoom_, 0.1f, 3.0f);
	ImGui::SliderFloat("Battle Camera RotX Offset", &battleCameraRotXOffset_, -0.5f, 0.5f);
	ImGui::End();

	ImGui::Begin("Battle Debug", &battleDebugVisible_);
	battle_.DrawImGui();

	ImGui::DragFloat2("position", &position_.x);
	ImGui::DragFloat3("scale", &scale_.x);

	//playerHpText_->SetPosition(position_);

	if (false && cameraAnim_) {
		ImGui::Separator();
		ImGui::Text("=== Editor Target ===");

		const bool editAnimation = (editorTargetKind_ == EditorTargetKind::Animation);
		if (ImGui::RadioButton("Animation", editAnimation)) {
			editorTargetKind_ = EditorTargetKind::Animation;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Camera", !editAnimation)) {
			editorTargetKind_ = EditorTargetKind::Camera;
		}

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
	}

	if (animationEditTarget_ || cameraEditTarget_) {
		animationEditor_.DrawImGui(BuildEditorContext_());
	}

	ImGui::End();

	// 例えば "player_fire" を編集したい場合
	if (battleEffectsDebugVisible_) {
		ImGui::Begin("Battle Effects", &battleEffectsDebugVisible_);
		static std::string targetEffect = "player_fire";

		// コンボボックスで編集対象を切り替えられるようにすると更に便利
		if (ImGui::BeginCombo("Select Edit Effect", targetEffect.c_str())) {
			if (ImGui::Selectable("player_fire")) targetEffect = "player_fire";
			if (ImGui::Selectable("sword_trail")) targetEffect = "sword_trail";
			ImGui::EndCombo();
		}

		// マネージャーから指定したエフェクトの設定を編集・反映
		particleManager_->UpdateImGui(targetEffect, attackEffectConfig_);
		DrawParticleObjectPostEditor_();
		ImGui::End();
	}

	if (fieldUi_) {
		ImGui::Begin("FieldUi Debug");
		fieldUi_->DrawImGui();
		ImGui::End();
	}

	AudioManager::GetInstance()->UpdateImGui();

	ImGui::Begin("Trail Debug");
	if (ImGui::CollapsingHeader("Test Trail Settings")) {
		ImGui::ColorEdit4("Start Color", &trailConfig_.startColor.x);
		ImGui::ColorEdit4("End Color", &trailConfig_.endColor.x);
		ImGui::SliderInt("Max Points", (int*)&trailConfig_.maxPoints, 10, 200);
		ImGui::SliderInt("Steps", (int*)&trailConfig_.interpolationSteps, 1, 10);
	}
	ImGui::End();

	// エフェクトシーケンサーエディター
	if (effectSequencer_) {
		// デフォルトの発射位置と着弾位置（プレイヤー → 最初の敵）
		Vector3 startPos = player_ ? player_->GetPos() + Vector3(0, 1.0f, 0) : Vector3{ -7.0f, 1.0f, 15.0f };
		Vector3 targetPos = { 7.0f, 1.0f, 15.0f }; // デフォルト敵位置
		if (enemyMgr_.GetEnemies().size() > 0) {
			const auto& firstEnemy = enemyMgr_.GetEnemies()[0];
			targetPos = firstEnemy.GetPos() + Vector3(0, 1.0f, 0);
		}
		effectSequencer_->DrawImGuiEditor(startPos, targetPos);
	}

#endif
}

void GameScene::DrawSkydome(GameApp& app)
{
	int windowW = WinApp::kClientWidth;
	int windowH = WinApp::kClientHeight;
	int battleHeight = static_cast<int>(windowH * splitRatio_);
	app.Dx()->SetViewport(0, 0, windowW, windowH);
	app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);

	app.ObjCom()->SetGraphicsPipelineState();

	if (skyDome_) skyDome_->Draw();

	app.Dx()->SetScissorRect(0, 0, windowW, windowH);
}

void GameScene::DrawPostEffect3D(GameApp& app)
{
	int windowW = WinApp::kClientWidth;
	int windowH = WinApp::kClientHeight;
	int battleHeight = static_cast<int>(windowH * splitRatio_);
	app.Dx()->SetViewport(0, 0, windowW, windowH);
	app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);

	app.ObjCom()->SetGraphicsPipelineState();

	if (particleObjectPostEnabled_) {
		app.DrawModelParticlesObjectPostToBloomScene(particleManager_, particleObjectPostParam_, battleHeight);
		app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);
	} else {
		particleManager_->Draw();
	}
	app.ObjCom()->SetGraphicsPipelineState();

	// エフェクトシーケンサーの弾を描画
	if (effectSequencer_) {
		effectSequencer_->Draw();
	}

	if (trailManager_) {
		trailManager_->DrawAll(animCamera_->GetViewProjectionMatrix());
	}

	app.Dx()->SetScissorRect(0, 0, windowW, windowH);
}

void GameScene::DrawPostEffect2D(GameApp& app)
{

}

void GameScene::ResetParticleObjectPostParam_()
{
	particleObjectPostParam_ = {};
	particleObjectPostParam_.threshold = 0.0f;
	particleObjectPostParam_.intensity = 1.7f;
	particleObjectPostParam_.vignetteIntensity = 0.0f;
	particleObjectPostParam_.vignetteScale = 0.0f;
	particleObjectPostParam_.distortionAmount = 0.0f;
	particleObjectPostParam_.chromAbAmount = 0.003f;
	particleObjectPostParam_.isGrayscale = 0.0f;
	particleObjectPostParam_.isInverted = 0.0f;
	particleObjectPostParam_.noiseIntensity = 0.0f;
	particleObjectPostParam_.scanlineIntensity = 0.0f;
	particleObjectPostParam_.scanlineFrequency = 100.0f;
	particleObjectPostParam_.curvature = 0.0f;
	particleObjectPostParam_.borderSharp = 0.0f;
	particleObjectPostParam_.glitchAmount = 0.0f;
	particleObjectPostParam_.dissolveAmount = -1.0f;
	particleObjectPostParam_.dissolveEdgeWidth = 0.08f;
	particleObjectPostParam_.dissolveEdgeIntensity = 2.0f;
	particleObjectPostParam_.dissolveNoiseScale = 36.0f;
	particleObjectPostParam_.dissolveEdgeColor = { 0.15f, 0.8f, 1.0f, 1.0f };
}

void GameScene::DrawParticleObjectPostEditor_()
{
#ifdef USE_IMGUI
	if (!ImGui::CollapsingHeader("Particle Object Post", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	ImGui::Checkbox("Enable Particle Object Post", &particleObjectPostEnabled_);
	ImGui::DragFloat("Post Threshold", &particleObjectPostParam_.threshold, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("Post Intensity", &particleObjectPostParam_.intensity, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("Chromatic Aberration", &particleObjectPostParam_.chromAbAmount, 0.001f, 0.0f, 0.1f);
	ImGui::DragFloat("Distortion", &particleObjectPostParam_.distortionAmount, 0.001f, 0.0f, 0.2f);
	ImGui::DragFloat("Noise", &particleObjectPostParam_.noiseIntensity, 0.001f, 0.0f, 1.0f);
	ImGui::DragFloat("Glitch", &particleObjectPostParam_.glitchAmount, 0.001f, 0.0f, 0.2f);
	ImGui::DragFloat("Vignette Intensity", &particleObjectPostParam_.vignetteIntensity, 0.01f, 0.0f, 2.0f);
	ImGui::DragFloat("Vignette Scale", &particleObjectPostParam_.vignetteScale, 0.01f, 0.0f, 2.0f);

	bool grayscale = particleObjectPostParam_.isGrayscale > 0.5f;
	bool inverted = particleObjectPostParam_.isInverted > 0.5f;
	if (ImGui::Checkbox("Post Grayscale", &grayscale)) {
		particleObjectPostParam_.isGrayscale = grayscale ? 1.0f : 0.0f;
	}
	if (ImGui::Checkbox("Post Invert", &inverted)) {
		particleObjectPostParam_.isInverted = inverted ? 1.0f : 0.0f;
	}

	ImGui::SeparatorText("Dissolve");
	ImGui::DragFloat("Dissolve Amount", &particleObjectPostParam_.dissolveAmount, 0.01f, -1.0f, 1.0f);
	ImGui::DragFloat("Dissolve Edge Width", &particleObjectPostParam_.dissolveEdgeWidth, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Dissolve Edge Intensity", &particleObjectPostParam_.dissolveEdgeIntensity, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("Dissolve Noise Scale", &particleObjectPostParam_.dissolveNoiseScale, 0.1f, 1.0f, 200.0f);
	ImGui::ColorEdit4("Dissolve Edge Color", &particleObjectPostParam_.dissolveEdgeColor.x);

	if (ImGui::Button("Reset Particle Object Post")) {
		ResetParticleObjectPostParam_();
	}
#endif
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

AnimationEditorSession::EditorContext GameScene::BuildEditorContext_() {
	AnimationEditorSession::EditorContext context{};
	context.editorCamera = cameraEditTarget_ ? cameraEditTarget_ : camera_.get();
	context.cameraAnimator = cameraAnim_.get();
	context.canEditAnimation = (animationEditTarget_ != nullptr);
	context.canEditCamera = (cameraEditTarget_ != nullptr && cameraAnim_ != nullptr);
	context.editCameraMode = (editorTargetKind_ == EditorTargetKind::Camera);
	context.cameraFiles = &cameraFiles_;
	context.currentCameraIndex = currentCameraIndex_;
	context.randomCameraEnabled = &randomCameraEnabled_;
	context.sameCameraLoopEnabled = &sameCameraLoopEnabled_;
	context.switchToAnimation = [this]() {
		editorTargetKind_ = EditorTargetKind::Animation;
		};
	context.switchToCamera = [this]() {
		editorTargetKind_ = EditorTargetKind::Camera;
		};
	context.reloadCameraFiles = [this]() {
		ReloadCameraFileList_();
		};
	context.loadCameraByIndex = [this](int index) {
		LoadCameraByIndex_(index);
		};
	context.playRandomCamera = [this]() {
		ChangeRandomCamera();
		};

	if (editorTargetKind_ == EditorTargetKind::Camera) {
		context.cameraTarget = cameraEditTarget_;
		context.animationTarget = nullptr;
	} else {
		context.animationTarget = animationEditTarget_;
		context.cameraTarget = nullptr;
	}

	return context;
}
