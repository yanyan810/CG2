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
#include <cmath>
#include <cwchar>
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
	constexpr Vector2 kDefenseUiTextureSize{ 64.0f, 64.0f };
	constexpr Vector2 kPowerupUiTextureSize{ 48.0f, 48.0f };
	constexpr float kClearTransitionDuration = 2.0f;
	constexpr float kFinisherHitStopDuration = 0.12f;
	constexpr float kFinisherSlowDuration = 0.85f;
	constexpr float kFinisherShakeDuration = 0.65f;
	constexpr float kFinisherShakeMagnitude = 0.34f;
	constexpr std::array<Vector2, 8> kOutlineDirections{
		Vector2{ -1.0f, 0.0f },
		Vector2{ 1.0f, 0.0f },
		Vector2{ 0.0f, -1.0f },
		Vector2{ 0.0f, 1.0f },
		Vector2{ -1.0f, -1.0f },
		Vector2{ 1.0f, -1.0f },
		Vector2{ -1.0f, 1.0f },
		Vector2{ 1.0f, 1.0f },
	};

	void DrawVector3Debug_(const char* label, const Vector3& v)
	{
#ifdef USE_IMGUI
		ImGui::Text("%s: %.3f, %.3f, %.3f", label, v.x, v.y, v.z);
#else
		(void)label;
		(void)v;
#endif
	}

	void DrawCameraDebug_(const char* label, Camera* camera)
	{
#ifdef USE_IMGUI
		if (!camera) {
			ImGui::Text("%s: null", label);
			return;
		}

		ImGui::Text("%s: %p", label, static_cast<void*>(camera));
		DrawVector3Debug_("  pos", camera->GetTranslate());
		DrawVector3Debug_("  rot", camera->GetRotate());
		ImGui::Text("  matrixOverride: %s", camera->UsesWorldMatrixOverride() ? "true" : "false");
		ImGui::Text("  fov/aspect: %.3f / %.3f", camera->GetFovY(), camera->GetAspect());
#else
		(void)label;
		(void)camera;
#endif
	}

	void DrawClipDebug_(const char* label, const Vector3& worldPos, Camera* camera)
	{
#ifdef USE_IMGUI
		if (!camera) {
			ImGui::Text("%s clip: camera null", label);
			return;
		}

		const Matrix4x4& vp = camera->GetViewProjectionMatrix();
		Vector4 clip{};
		clip.x = worldPos.x * vp.m[0][0] + worldPos.y * vp.m[1][0] + worldPos.z * vp.m[2][0] + vp.m[3][0];
		clip.y = worldPos.x * vp.m[0][1] + worldPos.y * vp.m[1][1] + worldPos.z * vp.m[2][1] + vp.m[3][1];
		clip.z = worldPos.x * vp.m[0][2] + worldPos.y * vp.m[1][2] + worldPos.z * vp.m[2][2] + vp.m[3][2];
		clip.w = worldPos.x * vp.m[0][3] + worldPos.y * vp.m[1][3] + worldPos.z * vp.m[2][3] + vp.m[3][3];

		if (std::abs(clip.w) < 0.0001f) {
			ImGui::Text("%s clip: w is near zero", label);
			return;
		}

		const float ndcX = clip.x / clip.w;
		const float ndcY = clip.y / clip.w;
		const float ndcZ = clip.z / clip.w;
		const bool visible = clip.w > 0.0f &&
			ndcX >= -1.0f && ndcX <= 1.0f &&
			ndcY >= -1.0f && ndcY <= 1.0f &&
			ndcZ >= 0.0f && ndcZ <= 1.0f;

		ImGui::Text("%s clip: %.3f, %.3f, %.3f, %.3f", label, clip.x, clip.y, clip.z, clip.w);
		ImGui::Text("%s ndc : %.3f, %.3f, %.3f visible=%s", label, ndcX, ndcY, ndcZ, visible ? "yes" : "no");
#else
		(void)label;
		(void)worldPos;
		(void)camera;
#endif
	}

	Vector3 MakeLookAtRotation_(const Vector3& eye, const Vector3& target)
	{
		Vector3 dir = target - eye;
		const float horizontal = std::sqrt(dir.x * dir.x + dir.z * dir.z);
		if (horizontal < 0.0001f && std::abs(dir.y) < 0.0001f) {
			return {};
		}

		const float yaw = std::atan2(dir.x, dir.z);
		const float pitch = std::atan2(-dir.y, horizontal);
		return { pitch, yaw, 0.0f };
	}

	void AppendVec3DebugLine_(std::wstring& text, const wchar_t* label, const Vector3& v)
	{
		wchar_t line[160]{};
		swprintf_s(line, L"%s: %.3f, %.3f, %.3f\n", label, v.x, v.y, v.z);
		text += line;
	}

	void AppendCameraDebugLines_(std::wstring& text, const wchar_t* label, Camera* camera)
	{
		text += label;
		text += L"\n";
		if (!camera) {
			text += L"  null\n";
			return;
		}

		AppendVec3DebugLine_(text, L"  pos", camera->GetTranslate());
		AppendVec3DebugLine_(text, L"  rot", camera->GetRotate());

		wchar_t line[160]{};
		swprintf_s(line, L"  matrixOverride: %s\n", camera->UsesWorldMatrixOverride() ? L"true" : L"false");
		text += line;
		swprintf_s(line, L"  fov/aspect: %.3f / %.3f\n", camera->GetFovY(), camera->GetAspect());
		text += line;
	}

	constexpr float kBossStageBannerDuration = 2.0f;
	constexpr float kBossStageBannerX = 380.0f;
	constexpr float kBossStageBannerY = 105.0f;
	constexpr float kBossStageBannerWidth = 520.0f;
	constexpr float kBossStageBannerHeight = 86.0f;
	constexpr float kBossStageTextX = kBossStageBannerX + 110.0f;
	constexpr float kBossStageTextY = kBossStageBannerY;

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

	BloomParam MakeBossStageBannerEffectParam_(const BloomParam& baseParam, float strength)
	{
		const float t = std::clamp(strength, 0.0f, 1.0f);
		BloomParam param = baseParam;
		param.threshold = 0.0f;
		param.intensity = 0.75f + 0.25f * t;
		param.vignetteIntensity = 0.0f;
		param.vignetteScale = 0.0f;
		param.chromAbAmount = 0.004f + 0.008f * t;
		param.distortionAmount = 0.0006f + 0.0008f * t;
		param.noiseIntensity = 0.10f + 0.20f * t;
		param.scanlineIntensity = 0.08f + 0.18f * t;
		param.scanlineFrequency = 120.0f;
		param.curvature = 0.0f;
		param.borderSharp = 0.0f;
		param.glitchAmount = 0.004f + 0.016f * t;
		param.dissolveAmount = -1.0f;
		return param;
	}
}

void GameScene::OnEnter(GameApp& app) {
	isBossStage_ = false;
	bossStageBannerTimer_ = 0.0f;
	battleEndTimer_ = 120;
	clearTransitionActive_ = false;
	clearTransitionTimer_ = 0.0f;
	clearBaseFieldCameraPos_ = {};
	clearBaseFieldCameraRot_ = {};
	clearBaseBattleCameraPos_ = {};
	clearBaseBattleCameraRot_ = {};
	app.ResetRadialBlur();

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
	bossStageBannerTimer_ = isBossStage_ ? kBossStageBannerDuration : 0.0f;

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
	playerHpText_->SetFontSize(playerHpTextFontSize_);
	playerHpText_->SetSize({ 1.0f,1.0f,1.0f });
	playerHpText_->SetPosition(playerHpTextPosition_);
	for (auto& outlineText : playerHpOutlineTexts_) {
		outlineText = std::make_unique<TextSprite>();
		outlineText->Initialize(app.SpriteCom(), app.Dx());
		outlineText->SetFontSize(playerHpTextFontSize_);
		outlineText->SetSize({ 1.0f,1.0f,1.0f });
	}

	releaseDebugText_ = std::make_unique<TextSprite>();
	releaseDebugText_->Initialize(app.SpriteCom(), app.Dx());
	releaseDebugText_->SetFontSize(18);
	releaseDebugText_->SetSize({ 1.0f, 1.0f, 1.0f });
	releaseDebugText_->SetPosition({ 12.0f, 120.0f });
	releaseDebugText_->SetColor({ 0.9f, 1.0f, 0.45f });
	releaseDebugText_->SetText(L"");

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
	TextureManager::GetInstance()->LoadTexture("resources/ui/gauge/Powerup_UI.png");
	powerBoostBg_ = std::make_unique<Sprite>();
	powerBoostBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/gauge/Powerup_UI.png");
	powerBoostBg_->SetPosition(powerupUiPosition_);
	powerBoostBg_->SetScale({
		powerupUiSize_.x / kPowerupUiTextureSize.x,
		powerupUiSize_.y / kPowerupUiTextureSize.y,
		1.0f
		});
	powerBoostBg_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	powerBoostText_ = std::make_unique<TextSprite>();
	powerBoostText_->Initialize(app.SpriteCom(), app.Dx());
	powerBoostText_->SetSize({ 1.f,1.f,0.5f });
	powerBoostText_->SetPosition(powerBoostTextPosition_);

	// ブロック
	TextureManager::GetInstance()->LoadTexture("resources/ui/gauge/Defense_UI.png");
	blockBg_ = std::make_unique<Sprite>();
	blockBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/gauge/Defense_UI.png");
	blockBg_->SetPosition(defenseUiPosition_);
	blockBg_->SetScale({
		defenseUiSize_.x / kDefenseUiTextureSize.x,
		defenseUiSize_.y / kDefenseUiTextureSize.y,
		1.0f
		});
	blockBg_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	blockText_ = std::make_unique<TextSprite>();
	blockText_->Initialize(app.SpriteCom(), app.Dx());
	blockText_->SetFontSize(blockTextFontSize_);
	blockText_->SetSize({ 1.f,1.f,0.5f });
	blockText_->SetPosition(blockTextPosition_);
	for (auto& outlineText : blockOutlineTexts_) {
		outlineText = std::make_unique<TextSprite>();
		outlineText->Initialize(app.SpriteCom(), app.Dx());
		outlineText->SetFontSize(blockTextFontSize_);
		outlineText->SetSize({ 1.0f,1.0f,0.5f });
	}

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
	bossStageBannerBg_->SetPosition({ kBossStageBannerX, kBossStageBannerY });
	bossStageBannerBg_->SetScale({ kBossStageBannerWidth, kBossStageBannerHeight, 1.0f });
	bossStageBannerBg_->SetColor({ 0.18f, 0.02f, 0.02f, 0.78f });

	bossStageBannerEffectOverlay_ = std::make_unique<Sprite>();
	bossStageBannerEffectOverlay_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	bossStageBannerEffectOverlay_->SetPosition({ kBossStageBannerX - 24.0f, kBossStageBannerY - 12.0f });
	bossStageBannerEffectOverlay_->SetScale({ kBossStageBannerWidth + 48.0f, kBossStageBannerHeight + 24.0f, 1.0f });
	bossStageBannerEffectOverlay_->SetColor({ 1.0f, 0.02f, 0.0f, 0.18f });

	bossStageBannerGlowText_ = std::make_unique<TextSprite>();
	bossStageBannerGlowText_->Initialize(app.SpriteCom(), app.Dx());
	bossStageBannerGlowText_->SetText(L"BOSS STAGE");
	bossStageBannerGlowText_->SetFontSize(56);
	bossStageBannerGlowText_->SetColor({ 1.0f, 0.18f, 0.04f });
	bossStageBannerGlowText_->SetPosition({ kBossStageTextX, kBossStageTextY });
	bossStageBannerGlowText_->SetSize({ 1.0f, 1.0f, 1.0f });

	bossStageBannerText_ = std::make_unique<TextSprite>();
	bossStageBannerText_->Initialize(app.SpriteCom(), app.Dx());
	bossStageBannerText_->SetText(L"BOSS STAGE");
	bossStageBannerText_->SetFontSize(56);
	bossStageBannerText_->SetColor({ 1.0f, 0.78f, 0.2f });
	bossStageBannerText_->SetPosition({ kBossStageTextX, kBossStageTextY });
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

	fieldParticleManager_ = std::make_unique<ModelParticleManager>();
	fieldParticleManager_->Initialize(app.Dx(), app.Srv(), 20000);
	fieldParticleManager_->RegisterEffect("card_glitter", "card_glitter.json");
	battle_.SetFieldParticleManager(fieldParticleManager_.get());

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
	app.ResetRadialBlur();
	clearTransitionActive_ = false;
	clearTransitionTimer_ = 0.0f;
	fieldUi_.reset();
	bossStageBannerGlowText_.reset();
	bossStageBannerText_.reset();
	bossStageBannerBg_.reset();
	bossStageBannerEffectOverlay_.reset();
	cardDescBg_.reset();
	cardDescText_.reset();
	isBossStage_ = false;
	bossStageBannerTimer_ = 0.0f;

	animationEditTarget_ = nullptr;
	cameraEditTarget_ = nullptr;
	battle_.Finalize();
	player_.reset();
	skyDome_.reset();
	fieldParticleManager_.reset();
	camera_.reset();

	// EnemyManager に Clear() があるなら呼ぶ
	// enemyMgr_.Clear();

	// battle_ に明示的な解放関数を作るのが理想
	// battle_.Finalize();
}
void GameScene::Update(GameApp& app, float dt) {
	const auto isPlayerDead = [this]() {
		return player_ && (player_->GetHP() <= 0 || !player_->GetIsAlive());
	};

	if (isPlayerDead()) {
		RequestChangeScene_("GameOver");
		return;
	}

	const bool isBattleClear = battle_.IsAllEnemiesDead();
	const bool isTitleRequested = pausingUI_ && pausingUI_->GetIsSceneChangeRequested();
	if (isBattleClear) {
		Camera* clearBattleCamera = battle_.GetActionCamera();
		if (!clearBattleCamera) {
			clearBattleCamera = animCamera_.get();
		}
		if (!clearTransitionActive_) {
			clearTransitionActive_ = true;
			clearTransitionTimer_ = 0.0f;
			clearBaseFieldCameraPos_ = camera_ ? camera_->GetTranslate() : Vector3{};
			clearBaseFieldCameraRot_ = camera_ ? camera_->GetRotate() : Vector3{};
			clearBaseBattleCameraPos_ = clearBattleCamera ? clearBattleCamera->GetTranslate() : Vector3{};
			clearBaseBattleCameraRot_ = clearBattleCamera ? clearBattleCamera->GetRotate() : Vector3{};
		}

		clearTransitionTimer_ += dt;
		const float t = std::clamp(clearTransitionTimer_ / kClearTransitionDuration, 0.0f, 1.0f);
		const bool inHitStop = clearTransitionTimer_ < kFinisherHitStopDuration;
		const float slowScale = inHitStop
			? 0.0f
			: (clearTransitionTimer_ < kFinisherSlowDuration ? 0.18f : 0.62f);
		const float visualDt = dt * slowScale;

		float shakePower = 0.0f;
		if (clearTransitionTimer_ < kFinisherShakeDuration) {
			const float shakeT = std::clamp(clearTransitionTimer_ / kFinisherShakeDuration, 0.0f, 1.0f);
			const float falloff = 1.0f - shakeT;
			shakePower = kFinisherShakeMagnitude * falloff * falloff;
			if (inHitStop) {
				shakePower *= 1.25f;
			}
		}
		const Vector3 shakeOffset{
			std::sin(clearTransitionTimer_ * 91.0f) * shakePower,
			std::cos(clearTransitionTimer_ * 67.0f) * shakePower * 0.55f,
			std::sin(clearTransitionTimer_ * 49.0f) * shakePower * 0.18f
		};

		if (camera_) {
			const int windowW = WinApp::kClientWidth;
			const int windowH = WinApp::kClientHeight;
			const int fieldHeight = windowH - static_cast<int>(windowH * splitRatio_);
			camera_->SetAspect((float)windowW / fieldHeight);

			float origFovY = 0.45f;
			float zoomRatio = ((float)fieldHeight / windowH) / fieldCameraZoom_;
			float newFovY = 2.0f * std::atan(zoomRatio * std::tan(origFovY / 2.0f));
			camera_->SetFovY(newFovY);

			Matrix4x4 shiftField = Matrix4x4::MakeIdentity4x4();
			shiftField.m[1][1] = 1.0f - splitRatio_;
			shiftField.m[3][1] = -splitRatio_;
			camera_->SetProjectionShift(shiftField);
			camera_->SetTranslate(clearBaseFieldCameraPos_ + shakeOffset);
			camera_->SetRotate(clearBaseFieldCameraRot_);
			camera_->Update();
		}

		if (clearBattleCamera) {
			const int windowW = WinApp::kClientWidth;
			const int windowH = WinApp::kClientHeight;
			const bool isBattleAnimationPlaying = battle_.IsActionSequencePlaying();
			const int battleHeight = isBattleAnimationPlaying
				? windowH
				: static_cast<int>(windowH * splitRatio_);
			clearBattleCamera->SetAspect((float)windowW / battleHeight);

			float origFovY = 0.45f;
			float zoomRatio = ((float)battleHeight / windowH) / battleCameraZoom_;
			float newFovY = 2.0f * std::atan(zoomRatio * std::tan(origFovY / 2.0f));
			if (!isBattleAnimationPlaying) {
				clearBattleCamera->SetFovY(newFovY);
			}

			Matrix4x4 shiftBattle = Matrix4x4::MakeIdentity4x4();
			if (!isBattleAnimationPlaying) {
				shiftBattle.m[1][1] = splitRatio_;
				shiftBattle.m[3][1] = 1.0f - splitRatio_;
			}
			clearBattleCamera->SetProjectionShift(shiftBattle);
			clearBattleCamera->SetTranslate(clearBaseBattleCameraPos_ + shakeOffset);
			clearBattleCamera->SetRotate(clearBaseBattleCameraRot_);
			clearBattleCamera->Update();
		}

		app.ObjCom()->SetDefaultCamera(clearBattleCamera);
		if (skyDome_) {
			skyDome_->SetCamera(clearBattleCamera);
			skyDome_->Update(visualDt);
		}
		if (player_) {
			player_->SetCamera(clearBattleCamera);
			player_->Update(visualDt);
		}
		for (auto& enemy : enemyMgr_.GetEnemies()) {
			enemy.SetCamera(clearBattleCamera);
			if (enemy.GetObject3d()) {
				enemy.GetObject3d()->Update(visualDt);
			}
		}

		battle_.UpdateClearTransitionVisuals(visualDt);
		if (trailManager_) {
			trailManager_->Update(visualDt);
		}
		if (effectSequencer_) {
			effectSequencer_->Update(visualDt);
		}
		if (particleManager_) {
			particleManager_->Dispatch(visualDt, clearBattleCamera);
		}

		app.SetRadialBlur((0.09f + (inHitStop ? 0.035f : 0.0f)) * (1.0f - t));
		UpdateReleaseDebugText_();

		if (clearTransitionTimer_ >= kClearTransitionDuration) {
			app.ResetRadialBlur();
			RequestChangeScene_("GameClear");
		}
		return;
	}

	if (isTitleRequested) {
		battleEndTimer_--;
		if (battleEndTimer_ <= 0) {
			app.ResetRadialBlur();
			RequestChangeScene_("Title");
		}
		return;
	}

	Input* input = app.GetInput();
	if (!input) {
		app.ResetRadialBlur();
		return;
	}

	pausingUI_->Update(app, input);

	if (pausingUI_->GetIsPaused()) {
		app.ResetRadialBlur();
		return;
	}

	if (bossStageBannerTimer_ > 0.0f) {
		bossStageBannerTimer_ -= dt;
		if (bossStageBannerTimer_ < 0.0f) {
			bossStageBannerTimer_ = 0.0f;
		}
	}

	if (isBossStage_ && bossStageBannerTimer_ > 0.0f) {
		const float t = std::clamp(bossStageBannerTimer_ / kBossStageBannerDuration, 0.0f, 1.0f);
		app.SetRadialBlur(0.055f * t);
	} else {
		app.ResetRadialBlur();
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
		if (!cameraAnim_ || cameraAnim_->GetKeyframes().empty()) {
			rot.x = 0.15f;
		}
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
		const bool isBattleAnimationPlaying = battle_.IsActionSequencePlaying();
		int battleHeight = isBattleAnimationPlaying ? windowH : static_cast<int>(windowH * splitRatio_);
		actionCamera->SetAspect((float)windowW / battleHeight);

		Matrix4x4 shiftBattle = Matrix4x4::MakeIdentity4x4();
		if (!isBattleAnimationPlaying) {
			float zoomRatio = ((float)battleHeight / windowH) / battleCameraZoom_;
			float correctedFovY = 2.0f * std::atan(zoomRatio * std::tan(actionCamera->GetFovY() / 2.0f));
			actionCamera->SetFovY(correctedFovY);
			shiftBattle.m[1][1] = splitRatio_;
			shiftBattle.m[3][1] = 1.0f - splitRatio_;
		}
		actionCamera->SetProjectionShift(shiftBattle);

		if (isBattleAnimationPlaying && forceActionCameraLookAt_ && player_) {
			Vector3 target = player_->GetPos() + Vector3{ 0.0f, 1.0f, 0.0f };
			actionCamera->SetRotate(MakeLookAtRotation_(actionCamera->GetTranslate(), target));
		}
		actionCamera->Update();

		if (isBattleAnimationPlaying) {
			if (skyDome_) {
				skyDome_->SetCamera(actionCamera);
				skyDome_->Update(0.0f);
			}
			if (player_) {
				player_->SetCamera(actionCamera);
				if (player_->GetObject3d()) {
					player_->GetObject3d()->Update(0.0f);
				}
			}
			for (auto& enemy : enemyMgr_.GetEnemies()) {
				enemy.SetCamera(actionCamera);
				if (enemy.GetObject3d()) {
					enemy.GetObject3d()->Update(0.0f);
				}
			}
		}
	}

	if (isPlayerDead()) {
		RequestChangeScene_("GameOver");
		return;
	}

	if (battle_.IsActionSequencePlaying()) {
		trailManager_->Update(dt);
		if (effectSequencer_) {
			effectSequencer_->Update(dt);
		}
		particleManager_->Dispatch(1.0f / 60.0f, animCamera_.get());
		UpdateReleaseDebugText_();
		return;
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
		const std::wstring playerHpText = battle_.GetPlayerHpTexts();
		playerHpText_->SetText(playerHpText);
		for (auto& outlineText : playerHpOutlineTexts_) {
			if (outlineText) {
				outlineText->SetText(playerHpText);
			}
		}
	}

	if (powerBoostText_) {
		powerBoostText_->SetText(battle_.GetPlayerPowerBoostText());
	}

	if (blockText_) {
		const std::wstring blockText = battle_.GetPlayerBlockText();
		blockText_->SetText(blockText);
		for (auto& outlineText : blockOutlineTexts_) {
			if (outlineText) {
				outlineText->SetText(blockText);
			}
		}
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

	particleManager_->Dispatch(1.0f / 60.0f, animCamera_.get());
	if (fieldParticleManager_) {
		fieldParticleManager_->Dispatch(1.0f / 60.0f, camera_.get());
	}

}

void GameScene::UpdateReleaseDebugText_()
{
	if (!releaseDebugVisible_ || !releaseDebugText_) {
		return;
	}

	const bool isActionPlaying = battle_.IsActionSequencePlaying();
	Camera* actionCamera = battle_.GetActionCamera();
	Camera* activeCamera = isActionPlaying && actionCamera ? actionCamera : camera_.get();

	std::wstring text;
	text.reserve(1024);
	text += L"[Release Pos Debug]\n";
	text += isActionPlaying ? L"ActionSequence: true\n" : L"ActionSequence: false\n";

	if (player_) {
		AppendVec3DebugLine_(text, L"Player logic", player_->GetPos());
		AppendVec3DebugLine_(text, L"Player world", player_->GetWorldPos());
		if (Object3d* playerObj = player_->GetObject3d()) {
			AppendVec3DebugLine_(text, L"Player obj pos", playerObj->GetTranslate());
			AppendVec3DebugLine_(text, L"Player obj rot", playerObj->GetRotate());
		}
	} else {
		text += L"Player: null\n";
	}

	if (!enemyMgr_.GetEnemies().empty()) {
		Enemy& enemy = enemyMgr_.GetEnemies().front();
		AppendVec3DebugLine_(text, L"Enemy[0] logic", enemy.GetPos());
		if (Object3d* enemyObj = enemy.GetObject3d()) {
			AppendVec3DebugLine_(text, L"Enemy[0] obj pos", enemyObj->GetTranslate());
		}
	} else {
		text += L"Enemy[0]: null\n";
	}

	AppendCameraDebugLines_(text, L"Active camera", activeCamera);
	AppendCameraDebugLines_(text, L"Field camera", camera_.get());
	AppendCameraDebugLines_(text, L"Action camera", actionCamera);

	releaseDebugText_->SetText(text);
}

void GameScene::Draw3D(GameApp& app) {
	app.Dx()->SetBackBuffer();

	int windowW = WinApp::kClientWidth;
	int windowH = WinApp::kClientHeight;
	const bool isBattleAnimationPlaying = battle_.IsActionSequencePlaying();
	int battleHeight = isBattleAnimationPlaying ? windowH : static_cast<int>(windowH * splitRatio_);

	app.Dx()->SetViewport(0, 0, windowW, windowH);

	app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);
	app.ObjCom()->SetGraphicsPipelineState();
	app.Dx()->ClearDepthBuffer();

	if (player_) player_->Draw();
	enemyMgr_.Draw();

	// 敵以外のモデルにFilterを書ける (バトル画面側)
	if (battle_.GetNowCardInputState() == BattleController::CardInputState::ChoosingEnemyTarget) {
		highlightFilter_->Draw();
	}
	battle_.DrawDamagePopups3D(app);

	if (!isBattleAnimationPlaying) {
		app.Dx()->SetScissorRect(0, battleHeight, windowW, windowH);
		app.ObjCom()->SetGraphicsPipelineState();
		app.Dx()->ClearDepthBuffer();
		battle_.DrawField3D(app);

		app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);
		app.ObjCom()->SetGraphicsPipelineState();
		battle_.DrawBattleOverlay3D(app);

		app.Dx()->SetScissorRect(0, 0, windowW, windowH);
		app.ObjCom()->SetGraphicsPipelineState();
		app.Dx()->ClearDepthBuffer();
		battle_.DrawCardArea3D(app);

	battle_.DrawPostEffect3D(app);
	battle_.DrawFieldFrameBloom(app);

	app.Dx()->SetScissorRect(0, 0, windowW, windowH);
	app.Dx()->ClearDepthBuffer();
	if (fieldParticleManager_) {
		if (particleObjectPostEnabled_) {
			app.DrawModelParticlesObjectPost(fieldParticleManager_.get(), particleObjectPostParam_);
		} else {
			fieldParticleManager_->Draw();
			app.ObjCom()->SetGraphicsPipelineState();
		}
	}
		battle_.DrawPostEffect3D(app);
	}

	// 最後にビューポートを元に戻す（2D描画等のため）
	app.Dx()->SetViewport(windowW, windowH);
	app.ObjCom()->SetGraphicsPipelineState();

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

	if (battle_.IsActionSequencePlaying()) {
		battle_.Draw2D(app);
		if (releaseDebugVisible_ && releaseDebugText_) {
			releaseDebugText_->Update(view, proj);
			releaseDebugText_->Draw();
		}
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
		playerHpText_->SetPosition(playerHpTextPosition_);
		playerHpText_->SetColor({ playerHpTextColor_.x, playerHpTextColor_.y, playerHpTextColor_.z });
		playerHpText_->SetAlpha(playerHpTextColor_.w);
		if (playerHpOutlineEnabled_) {
			for (size_t i = 0; i < playerHpOutlineTexts_.size(); ++i) {
				auto& outlineText = playerHpOutlineTexts_[i];
				if (!outlineText) {
					continue;
				}
				outlineText->SetPosition({
					playerHpTextPosition_.x + kOutlineDirections[i].x * playerHpOutlineThickness_,
					playerHpTextPosition_.y + kOutlineDirections[i].y * playerHpOutlineThickness_
					});
				outlineText->SetColor({
					playerHpOutlineColor_.x,
					playerHpOutlineColor_.y,
					playerHpOutlineColor_.z
					});
				outlineText->SetAlpha(playerHpOutlineColor_.w);
				outlineText->Update(view, proj);
				outlineText->Draw();
			}
		}
		playerHpText_->Update(view, proj);
		playerHpText_->Draw();
	}

	if (powerupUiVisible_ && powerBoostText_) {
		if (powerBoostBg_) {
			powerBoostBg_->SetPosition(powerupUiPosition_);
			powerBoostBg_->SetScale({
				powerupUiSize_.x / kPowerupUiTextureSize.x,
				powerupUiSize_.y / kPowerupUiTextureSize.y,
				1.0f
				});
			powerBoostBg_->Update(view, proj);
			powerBoostBg_->Draw();
		}
		powerBoostText_->SetPosition(powerBoostTextPosition_);
		powerBoostText_->Update(view, proj);
		powerBoostText_->Draw();
	}
	if (blockText_) {
		if (blockBg_) {
			blockBg_->SetPosition(defenseUiPosition_);
			blockBg_->SetScale({
				defenseUiSize_.x / kDefenseUiTextureSize.x,
				defenseUiSize_.y / kDefenseUiTextureSize.y,
				1.0f
				});
			blockBg_->Update(view, proj);
			blockBg_->Draw();
		}
		blockText_->SetPosition(blockTextPosition_);
		blockText_->SetColor({ blockTextColor_.x, blockTextColor_.y, blockTextColor_.z });
		blockText_->SetAlpha(blockTextColor_.w);
		if (blockOutlineEnabled_) {
			for (size_t i = 0; i < blockOutlineTexts_.size(); ++i) {
				auto& outlineText = blockOutlineTexts_[i];
				if (!outlineText) {
					continue;
				}
				outlineText->SetPosition({
					blockTextPosition_.x + kOutlineDirections[i].x * blockOutlineThickness_,
					blockTextPosition_.y + kOutlineDirections[i].y * blockOutlineThickness_
					});
				outlineText->SetColor({
					blockOutlineColor_.x,
					blockOutlineColor_.y,
					blockOutlineColor_.z
					});
				outlineText->SetAlpha(blockOutlineColor_.w);
				outlineText->Update(view, proj);
				outlineText->Draw();
			}
		}
		blockText_->Update(view, proj);
		blockText_->Draw();
	}

	for (auto& text : enemyHpTexts_) {
		text->Update(view, proj);
		text->Draw();
	}

	if (isBossStage_ && bossStageBannerTimer_ > 0.0f) {
		const float alpha = bossStageBannerTimer_ < 1.0f ? bossStageBannerTimer_ : 1.0f;
		const float effectStrength = std::clamp(bossStageBannerTimer_ / kBossStageBannerDuration, 0.0f, 1.0f);
		if (bossStageBannerEffectOverlay_) {
			const float effectAlpha = (0.10f + 0.18f * effectStrength) * alpha;
			bossStageBannerEffectOverlay_->SetColor({ 1.0f, 0.02f, 0.0f, effectAlpha });
			BloomParam bannerParam = MakeBossStageBannerEffectParam_(app.ObjectPost()->GetParam(), effectStrength);
			app.DrawSpriteObjectPost(bossStageBannerEffectOverlay_.get(), view, proj, bannerParam);
		}
		if (bossStageBannerBg_) {
			bossStageBannerBg_->SetColor({ 0.18f, 0.02f, 0.02f, 0.78f * alpha });
			bossStageBannerBg_->Update(view, proj);
			bossStageBannerBg_->Draw();
		}
		if (bossStageBannerGlowText_) {
			bossStageBannerGlowText_->SetAlpha(0.16f * alpha);
			bossStageBannerGlowText_->Update(view, proj);
			bossStageBannerGlowText_->Draw();
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

	if (releaseDebugVisible_ && releaseDebugText_) {
		releaseDebugText_->Update(view, proj);
		releaseDebugText_->Draw();
	}
}

void GameScene::DrawImGui(GameApp& app) {
#ifdef USE_IMGUI
	DrawPlayerHudImGui_();

	ImGui::Begin("UI Visibility", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Checkbox("Battle Debug", &battleDebugVisible_);
	ImGui::Checkbox("Battle Effects", &battleEffectsDebugVisible_);
	ImGui::Checkbox("Release Pos Debug Text", &releaseDebugVisible_);
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

	DrawBattleAnimationDebugWindow_();

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

void GameScene::DrawPlayerHudImGui_()
{
#ifdef USE_IMGUI
	ImGui::Begin("Player UI Adjust", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	if (ImGui::CollapsingHeader("HP Gauge", ImGuiTreeNodeFlags_DefaultOpen)) {
		battle_.DrawPlayerHudImGuiControls();
	}

	if (ImGui::CollapsingHeader("HP Number", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat2("HP Number Position", &playerHpTextPosition_.x, 1.0f);
		if (ImGui::DragInt("HP Number Font Size", &playerHpTextFontSize_, 1.0f, 8, 128)) {
			playerHpTextFontSize_ = std::max(8, playerHpTextFontSize_);
			if (playerHpText_) {
				playerHpText_->SetFontSize(playerHpTextFontSize_);
			}
			for (auto& outlineText : playerHpOutlineTexts_) {
				if (outlineText) {
					outlineText->SetFontSize(playerHpTextFontSize_);
				}
			}
		}
		ImGui::ColorEdit4("HP Number Color", &playerHpTextColor_.x);
		ImGui::Checkbox("HP Outline Enabled", &playerHpOutlineEnabled_);
		ImGui::DragFloat("HP Outline Thickness", &playerHpOutlineThickness_, 0.1f, 0.0f, 12.0f);
		ImGui::ColorEdit4("HP Outline Color", &playerHpOutlineColor_.x);
	}

	if (ImGui::CollapsingHeader("Defense UI", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat2("Defense Image Position", &defenseUiPosition_.x, 1.0f);
		ImGui::DragFloat2("Defense Image Size", &defenseUiSize_.x, 1.0f, 1.0f, 512.0f);
		defenseUiSize_.x = std::max(1.0f, defenseUiSize_.x);
		defenseUiSize_.y = std::max(1.0f, defenseUiSize_.y);
		ImGui::DragFloat2("Defense Number Position", &blockTextPosition_.x, 1.0f);
		if (ImGui::DragInt("Defense Number Font Size", &blockTextFontSize_, 1.0f, 8, 128)) {
			blockTextFontSize_ = std::max(8, blockTextFontSize_);
			if (blockText_) {
				blockText_->SetFontSize(blockTextFontSize_);
			}
			for (auto& outlineText : blockOutlineTexts_) {
				if (outlineText) {
					outlineText->SetFontSize(blockTextFontSize_);
				}
			}
		}
		ImGui::ColorEdit4("Defense Number Color", &blockTextColor_.x);
		ImGui::Checkbox("Defense Outline Enabled", &blockOutlineEnabled_);
		ImGui::DragFloat("Defense Outline Thickness", &blockOutlineThickness_, 0.1f, 0.0f, 12.0f);
		ImGui::ColorEdit4("Defense Outline Color", &blockOutlineColor_.x);
	}

	if (ImGui::CollapsingHeader("Powerup UI", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Powerup UI Visible", &powerupUiVisible_);
		ImGui::DragFloat2("Powerup Image Position", &powerupUiPosition_.x, 1.0f);
		ImGui::DragFloat2("Powerup Image Size", &powerupUiSize_.x, 1.0f, 1.0f, 512.0f);
		powerupUiSize_.x = std::max(1.0f, powerupUiSize_.x);
		powerupUiSize_.y = std::max(1.0f, powerupUiSize_.y);
	}

	ImGui::End();
#endif
}

void GameScene::DrawBattleAnimationDebugWindow_()
{
#ifdef USE_IMGUI
	ImGui::Begin("Battle Animation Scene Debug");

	const bool isActionPlaying = battle_.IsActionSequencePlaying();
	Camera* actionCamera = battle_.GetActionCamera();
	Camera* editorCamera = isActionPlaying && actionCamera
		? actionCamera
		: (cameraEditTarget_ ? cameraEditTarget_ : camera_.get());

	ImGui::Text("ActionSequencePlaying: %s", isActionPlaying ? "true" : "false");
	ImGui::Checkbox("Force Action Camera LookAt", &forceActionCameraLookAt_);
	ImGui::Text("SplitRatio: %.3f", splitRatio_);
	ImGui::Text("Window: %d x %d", WinApp::kClientWidth, WinApp::kClientHeight);
	ImGui::Text("BattleHeight: %d", isActionPlaying
		? WinApp::kClientHeight
		: static_cast<int>(WinApp::kClientHeight * splitRatio_));

	ImGui::Separator();
	ImGui::TextUnformatted("Player");
	if (player_) {
		DrawVector3Debug_("Player logic pos", player_->GetPos());
		DrawVector3Debug_("Player world pos", player_->GetWorldPos());
		ImGui::Text("Player HP/alive: %d / %s", player_->GetHP(), player_->GetIsAlive() ? "true" : "false");
		bool releaseAnimationEnabled = player_->GetReleaseAnimationEnabled();
		if (ImGui::Checkbox("Player Release Animations", &releaseAnimationEnabled)) {
			player_->SetReleaseAnimationEnabled(releaseAnimationEnabled);
		}
		if (Object3d* playerObj = player_->GetObject3d()) {
			DrawVector3Debug_("Player object pos", playerObj->GetTranslate());
			DrawVector3Debug_("Player object rot", playerObj->GetRotate());
			DrawVector3Debug_("Player object scale", playerObj->GetScale());
			ImGui::Text("Player object camera: %p", static_cast<void*>(playerObj->GetCamera()));
			ImGui::Text("Player model: %s", playerObj->GetModel() ? "loaded" : "null");
			DrawClipDebug_("Player/actionCam", playerObj->GetTranslate(), actionCamera);
			DrawClipDebug_("Player/editorCam", playerObj->GetTranslate(), editorCamera);

			if (ImGui::Button("Refresh Player WVP")) {
				playerObj->Update(0.0f);
			}
			ImGui::SameLine();
			if (ImGui::Button("Set Player Camera = Action") && actionCamera) {
				player_->SetCamera(actionCamera);
				playerObj->Update(0.0f);
			}
			if (ImGui::Button("LookAt Player/Enemy Now") && actionCamera) {
				Vector3 target = player_->GetPos() + Vector3{ 0.0f, 1.0f, 0.0f };
				actionCamera->SetRotate(MakeLookAtRotation_(actionCamera->GetTranslate(), target));
				actionCamera->Update();
				player_->SetCamera(actionCamera);
				playerObj->Update(0.0f);
			}
		}
	} else {
		ImGui::TextUnformatted("player_: null");
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Enemy");
	ImGui::Text("Enemy count: %d", static_cast<int>(enemyMgr_.GetEnemies().size()));
	if (!enemyMgr_.GetEnemies().empty()) {
		Enemy& enemy = enemyMgr_.GetEnemies().front();
		DrawVector3Debug_("Enemy[0] pos", enemy.GetPos());
		if (Object3d* enemyObj = enemy.GetObject3d()) {
			DrawVector3Debug_("Enemy[0] object pos", enemyObj->GetTranslate());
			ImGui::Text("Enemy[0] object camera: %p", static_cast<void*>(enemyObj->GetCamera()));
			DrawClipDebug_("Enemy[0]/actionCam", enemyObj->GetTranslate(), actionCamera);
		}
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Cameras");
	DrawCameraDebug_("Field camera", camera_.get());
	DrawCameraDebug_("Anim camera", animCamera_.get());
	DrawCameraDebug_("Action camera", actionCamera);
	DrawCameraDebug_("Editor camera", editorCamera);

	ImGui::End();
#endif
}

void GameScene::DrawSkydome(GameApp& app)
{
	int windowW = WinApp::kClientWidth;
	int windowH = WinApp::kClientHeight;
	int battleHeight = battle_.IsActionSequencePlaying() ? windowH : static_cast<int>(windowH * splitRatio_);
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
	int battleHeight = battle_.IsActionSequencePlaying() ? windowH : static_cast<int>(windowH * splitRatio_);
	app.Dx()->SetViewport(0, 0, windowW, windowH);
	app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);

	if (particleObjectPostEnabled_) {
		app.DrawModelParticlesObjectPostToBloomScene(particleManager_, particleObjectPostParam_);
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
	Camera* actionCamera = battle_.IsActionSequencePlaying() ? battle_.GetActionCamera() : nullptr;
	context.editorCamera = actionCamera ? actionCamera : (cameraEditTarget_ ? cameraEditTarget_ : camera_.get());
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
