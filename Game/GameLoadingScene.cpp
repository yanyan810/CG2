#include "GameLoadingScene.h"

#include "GameScene.h"
#include "Matrix4x4.h"
#include "ModelManager.h"
#include "SceneManager.h"
#include "StageSelectScene.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "WinApp.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <unordered_set>

using json = nlohmann::json;

namespace {
constexpr float kBarX = 240.0f;
constexpr float kBarY = 560.0f;
constexpr float kBarWidth = 800.0f;
constexpr float kBarHeight = 4.0f;
constexpr float kBarGlowHeight = 22.0f;
constexpr float kBarHeadSize = 18.0f;
constexpr float kBarHeadGlowSize = 46.0f;
constexpr float kCircleTextureSize = 512.0f;
constexpr float kMinShowTime = 1.0f;
constexpr int kFlowOrbCount = 5;
constexpr int kMaxDisplayedTipCount = 10;

float Clamp01(float value)
{
	return (std::max)(0.0f, (std::min)(1.0f, value));
}

Vector3 CirclePixelScale(float width, float height)
{
	return { width / kCircleTextureSize, height / kCircleTextureSize, 1.0f };
}

std::wstring Utf8ToWString(const std::string& text)
{
	if (text.empty()) {
		return L"";
	}

	const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
	if (size <= 1) {
		return L"";
	}

	std::wstring result(static_cast<size_t>(size - 1), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), size);
	return result;
}
}

void GameLoadingScene::OnEnter(GameApp& app)
{
	TextureManager::GetInstance()->LoadTexture("resources/ui/white.png");
	TextureManager::GetInstance()->LoadTexture("resources/circle.png");

	mode_ = app.GetLoadingMode();
	loadIndex_ = 0;
	waitFrames_ = 0;
	elapsed_ = 0.0f;
	displayProgress_ = 0.0f;
	loadComplete_ = false;
	stageScenePrepared_ = false;
	stagePrepareFrameShown_ = false;
	preparedGameScene_.reset();
	modelPaths_.clear();
	stageLoadSteps_.clear();
	allTips_.clear();
	displayedTips_.clear();
	currentTipIndex_ = 0;

	if (mode_ == GameApp::LoadingMode::BootToTitle) {
		app.BeginStartupLoading();
	} else if (mode_ == GameApp::LoadingMode::SelectToStageSelect) {
		BuildStageSelectLoadSteps_(app);
	} else {
		BuildStageInfo_(app);
		BuildStageLoadSteps_(app);
	}

	background_ = std::make_unique<Sprite>();
	background_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	background_->SetPosition({ 0.0f, 0.0f });
	background_->SetScale({ static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight), 1.0f });
	background_->SetColor({ 0.015f, 0.018f, 0.024f, 1.0f });

	barFill_ = std::make_unique<Sprite>();
	barFill_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	barFill_->SetPosition({ kBarX, kBarY - kBarHeight * 0.5f });
	barFill_->SetScale({ 1.0f, kBarHeight, 1.0f });
	barFill_->SetColor({ 0.48f, 1.0f, 0.62f, 0.92f });

	barGlow_ = std::make_unique<Sprite>();
	barGlow_->Initialize(app.SpriteCom(), app.Dx(), "resources/circle.png");
	barGlow_->SetPosition({ kBarX, kBarY - kBarGlowHeight * 0.5f });
	barGlow_->SetScale(CirclePixelScale(1.0f, kBarGlowHeight));
	barGlow_->SetColor({ 0.18f, 1.0f, 0.42f, 0.12f });

	// Small light beads chase along the loaded part of the line toward the head orb.
	flowOrbs_.clear();
	flowOrbs_.reserve(kFlowOrbCount);
	for (int i = 0; i < kFlowOrbCount; ++i) {
		auto orb = std::make_unique<Sprite>();
		orb->Initialize(app.SpriteCom(), app.Dx(), "resources/circle.png");
		orb->SetScale(CirclePixelScale(14.0f, 14.0f));
		orb->SetColor({ 0.58f, 1.0f, 0.68f, 0.0f });
		flowOrbs_.push_back(std::move(orb));
	}

	// Stretched orb trail plus a bright leading sphere for the loading line.
	barHeadGlow_ = std::make_unique<Sprite>();
	barHeadGlow_->Initialize(app.SpriteCom(), app.Dx(), "resources/circle.png");
	barHeadGlow_->SetScale(CirclePixelScale(kBarHeadGlowSize, kBarHeadGlowSize));
	barHeadGlow_->SetColor({ 0.18f, 1.0f, 0.42f, 0.28f });

	barHeadCore_ = std::make_unique<Sprite>();
	barHeadCore_->Initialize(app.SpriteCom(), app.Dx(), "resources/circle.png");
	barHeadCore_->SetScale(CirclePixelScale(kBarHeadSize, kBarHeadSize));
	barHeadCore_->SetColor({ 0.90f, 1.0f, 0.90f, 0.95f });

	loadingText_ = std::make_unique<TextSprite>();
	loadingText_->Initialize(app.SpriteCom(), app.Dx());
	loadingText_->SetFontSize(30);
	loadingText_->SetSize({ 1.0f, 1.0f, 1.0f });
	loadingText_->SetPosition({ kBarX, kBarY - 54.0f });
	loadingText_->SetColor({ 0.88f, 0.94f, 1.0f });

	percentText_ = std::make_unique<TextSprite>();
	percentText_->Initialize(app.SpriteCom(), app.Dx());
	percentText_->SetFontSize(24);
	percentText_->SetSize({ 1.0f, 1.0f, 1.0f });
	percentText_->SetPosition({ kBarX + kBarWidth - 58.0f, kBarY - 48.0f });
	percentText_->SetColor({ 0.62f, 0.72f, 0.82f });

	if (mode_ == GameApp::LoadingMode::StageToGame) {
		stageInfoPanel_ = std::make_unique<Sprite>();
		stageInfoPanel_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
		stageInfoPanel_->SetPosition({ 180.0f, 150.0f });
		stageInfoPanel_->SetScale({ 920.0f, 300.0f, 1.0f });
		stageInfoPanel_->SetColor({ 0.04f, 0.055f, 0.075f, 0.92f });

		stageTitleText_ = std::make_unique<TextSprite>();
		stageTitleText_->Initialize(app.SpriteCom(), app.Dx());
		stageTitleText_->SetFontSize(38);
		stageTitleText_->SetSize({ 1.0f, 1.0f, 1.0f });
		stageTitleText_->SetPosition({ 230.0f, 190.0f });
		stageTitleText_->SetColor({ 0.95f, 0.88f, 0.55f });
		stageTitleText_->SetText(stageTitle_);

		enemyInfoText_ = std::make_unique<TextSprite>();
		enemyInfoText_->Initialize(app.SpriteCom(), app.Dx());
		enemyInfoText_->SetFontSize(26);
		enemyInfoText_->SetSize({ 1.0f, 1.0f, 1.0f });
		enemyInfoText_->SetPosition({ 230.0f, 255.0f });
		enemyInfoText_->SetColor({ 0.82f, 0.9f, 1.0f });
		enemyInfoText_->SetText(enemyInfo_);

		tipTitleText_ = std::make_unique<TextSprite>();
		tipTitleText_->Initialize(app.SpriteCom(), app.Dx());
		tipTitleText_->SetFontSize(24);
		tipTitleText_->SetSize({ 1.0f, 1.0f, 1.0f });
		tipTitleText_->SetPosition({ 230.0f, 330.0f });
		tipTitleText_->SetColor({ 0.24f, 1.0f, 0.46f });
		tipTitleText_->SetText(L"TIP");

		tipText_ = std::make_unique<TextSprite>();
		tipText_->Initialize(app.SpriteCom(), app.Dx());
		tipText_->SetFontSize(24);
		tipText_->SetSize({ 1.0f, 1.0f, 1.0f });
		tipText_->SetPosition({ 230.0f, 370.0f });
		tipText_->SetColor({ 0.86f, 0.9f, 0.94f });
		tipText_->SetText(tipTextValue_);

		tipLeftArrowText_ = std::make_unique<TextSprite>();
		tipLeftArrowText_->Initialize(app.SpriteCom(), app.Dx());
		tipLeftArrowText_->SetFontSize(30);
		tipLeftArrowText_->SetSize({ 1.0f, 1.0f, 1.0f });
		tipLeftArrowText_->SetPosition({ 230.0f, 404.0f });
		tipLeftArrowText_->SetColor({ 0.42f, 1.0f, 0.58f });
		tipLeftArrowText_->SetText(L"<");

		tipRightArrowText_ = std::make_unique<TextSprite>();
		tipRightArrowText_->Initialize(app.SpriteCom(), app.Dx());
		tipRightArrowText_->SetFontSize(30);
		tipRightArrowText_->SetSize({ 1.0f, 1.0f, 1.0f });
		tipRightArrowText_->SetPosition({ 332.0f, 404.0f });
		tipRightArrowText_->SetColor({ 0.42f, 1.0f, 0.58f });
		tipRightArrowText_->SetText(L">");

		tipCounterText_ = std::make_unique<TextSprite>();
		tipCounterText_->Initialize(app.SpriteCom(), app.Dx());
		tipCounterText_->SetFontSize(20);
		tipCounterText_->SetSize({ 1.0f, 1.0f, 1.0f });
		tipCounterText_->SetPosition({ 262.0f, 410.0f });
		tipCounterText_->SetColor({ 0.62f, 0.72f, 0.82f });

		readyText_ = std::make_unique<TextSprite>();
		readyText_->Initialize(app.SpriteCom(), app.Dx());
		readyText_->SetFontSize(30);
		readyText_->SetSize({ 1.0f, 1.0f, 1.0f });
		readyText_->SetPosition({ kBarX + kBarWidth - 110.0f, kBarY + 22.0f });
		readyText_->SetColor({ 0.95f, 0.88f, 0.45f });
	}

	UpdateText_();
	UpdateSprites_(app);
}

void GameLoadingScene::OnExit(GameApp& app)
{
	(void)app;
	background_.reset();
	barBack_.reset();
	barFill_.reset();
	barGlow_.reset();
	flowOrbs_.clear();
	barHeadGlow_.reset();
	barHeadCore_.reset();
	stageInfoPanel_.reset();
	loadingText_.reset();
	percentText_.reset();
	stageTitleText_.reset();
	enemyInfoText_.reset();
	tipTitleText_.reset();
	tipText_.reset();
	tipLeftArrowText_.reset();
	tipRightArrowText_.reset();
	tipCounterText_.reset();
	readyText_.reset();
	preparedGameScene_.reset();
	modelPaths_.clear();
	stageLoadSteps_.clear();
	allTips_.clear();
	displayedTips_.clear();
	loadIndex_ = 0;
	waitFrames_ = 0;
}

void GameLoadingScene::BuildStageInfo_(GameApp& app)
{
	stageTitle_ = L"STAGE " + std::to_wstring(app.GetSelectedStageId());
	enemyInfo_ = L"Enemy data loading...";

	LoadTips_();
	SelectRandomTips_(kMaxDisplayedTipCount);
	ApplyCurrentTip_();

	std::ifstream file(app.GetSelectedStageConfigPath());
	if (!file.is_open()) {
		return;
	}

	json root;
	try {
		file >> root;
	} catch (...) {
		return;
	}

	stageTitle_ = Utf8ToWString(root.value("stageName", "Stage " + std::to_string(app.GetSelectedStageId())));
	bool isBossStage = root.value("isBossStage", false);

	std::map<std::string, int> enemyCounts;
	if (root.contains("enemies") && root["enemies"].is_array()) {
		for (const auto& enemy : root["enemies"]) {
			if (!enemy.is_object()) {
				continue;
			}

			const std::string name = enemy.value("enemyName", enemy.value("enemyType", "Enemy"));
			++enemyCounts[name];
			isBossStage = isBossStage || enemy.value("bossFlag", false);
		}
	}

	if (isBossStage) {
		stageTitle_ += L"  BOSS STAGE";
	}

	if (enemyCounts.empty()) {
		return;
	}

	enemyInfo_ = L"Enemies: ";
	bool first = true;
	for (const auto& [name, count] : enemyCounts) {
		if (!first) {
			enemyInfo_ += L" / ";
		}
		first = false;
		enemyInfo_ += Utf8ToWString(name);
		if (count > 1) {
			enemyInfo_ += L" x" + std::to_wstring(count);
		}
	}
}

void GameLoadingScene::LoadTips_()
{
	allTips_.clear();

	std::ifstream file("resources/configs/loading_tips.json");
	if (file.is_open()) {
		json root;
		try {
			file >> root;
			if (root.is_array()) {
				for (const auto& item : root) {
					if (item.is_string()) {
						allTips_.push_back(Utf8ToWString(item.get<std::string>()));
					} else if (item.is_object()) {
						const std::string text = item.value("text", "");
						if (!text.empty()) {
							allTips_.push_back(Utf8ToWString(text));
						}
					}
				}
			}
		} catch (...) {
			allTips_.clear();
		}
	}

	// JSONが読めない時の保険。Tips表示自体は壊さず続行する。
	if (allTips_.empty()) {
		allTips_ = {
			L"コストを残しておくと、次の選択肢を広げられます。",
			L"同じマークを集めると、フラッシュで安定した火力を出せます。",
			L"敵の行動予定を見て、防御と攻撃の順番を決めましょう。"
		};
	}
}

void GameLoadingScene::SelectRandomTips_(int maxCount)
{
	displayedTips_ = allTips_;
	static std::mt19937 rng(std::random_device{}());
	std::shuffle(displayedTips_.begin(), displayedTips_.end(), rng);

	if (maxCount > 0 && displayedTips_.size() > static_cast<size_t>(maxCount)) {
		displayedTips_.resize(static_cast<size_t>(maxCount));
	}
	currentTipIndex_ = 0;
}

void GameLoadingScene::ApplyCurrentTip_()
{
	if (displayedTips_.empty()) {
		tipTextValue_ = L"";
		return;
	}

	currentTipIndex_ %= displayedTips_.size();
	tipTextValue_ = displayedTips_[currentTipIndex_];
}

void GameLoadingScene::ChangeTip_(int direction)
{
	if (displayedTips_.empty()) {
		return;
	}

	const size_t count = displayedTips_.size();
	if (direction < 0) {
		currentTipIndex_ = (currentTipIndex_ + count - 1) % count;
	} else if (direction > 0) {
		currentTipIndex_ = (currentTipIndex_ + 1) % count;
	}
	ApplyCurrentTip_();
	if (tipText_) {
		tipText_->SetText(tipTextValue_);
	}
}

bool GameLoadingScene::IsPointInRect_(const POINT& point, float x, float y, float w, float h) const
{
	return static_cast<float>(point.x) >= x &&
		static_cast<float>(point.x) <= x + w &&
		static_cast<float>(point.y) >= y &&
		static_cast<float>(point.y) <= y + h;
}

bool GameLoadingScene::HandleTipInput_(GameApp& app)
{
	if (mode_ != GameApp::LoadingMode::StageToGame || displayedTips_.size() <= 1) {
		return false;
	}

	Input* input = app.GetInput();
	if (!input) {
		return false;
	}

	if (input->IsKeyTrigger(DIK_LEFT) || input->IsKeyTrigger(DIK_A)) {
		ChangeTip_(-1);
		return true;
	}
	if (input->IsKeyTrigger(DIK_RIGHT) || input->IsKeyTrigger(DIK_D)) {
		ChangeTip_(1);
		return true;
	}

	if (!input->IsMouseTrigger(0)) {
		return false;
	}

	const POINT mouse = input->GetMousePosition();
	if (IsPointInRect_(mouse, 218.0f, 394.0f, 44.0f, 48.0f)) {
		ChangeTip_(-1);
		return true;
	}
	if (IsPointInRect_(mouse, 322.0f, 394.0f, 44.0f, 48.0f)) {
		ChangeTip_(1);
		return true;
	}

	return false;
}

void GameLoadingScene::BuildStageSelectLoadSteps_(GameApp& app)
{
	(void)app;

	StageSelectScene::BeginStageFieldCacheLoading();

	stageLoadSteps_.push_back([]() {
		TextureManager::GetInstance()->LoadTexture("resources/ui/white.png");
		});
	stageLoadSteps_.push_back([]() {
		TextureManager::GetInstance()->LoadTexture("resources/circle.png");
		});
	stageLoadSteps_.push_back([]() {
		TextureManager::GetInstance()->LoadTexture("resources/ui/stage_select/title_stage_select.png");
		});
	stageLoadSteps_.push_back([]() {
		TextureManager::GetInstance()->LoadTexture("resources/ui/stage_select/desc_bg.png");
		});

	GameApp* appPtr = &app;
	for (int i = 0; i < 10; ++i) {
		stageLoadSteps_.push_back([appPtr]() {
			StageSelectScene::LoadStageFieldCacheStep(*appPtr);
			});
	}
}

void GameLoadingScene::BuildStageLoadSteps_(GameApp& app)
{
	std::unordered_set<std::string> seen;

	const auto addModel = [&](const std::string& path) {
		if (!path.empty() && seen.insert(path).second) {
			modelPaths_.push_back(path);
			stageLoadSteps_.push_back([path]() {
				if (!ModelManager::GetInstance()->FindModel(path)) {
					ModelManager::GetInstance()->LoadModel(path);
				}
				});
		}
		};

	// GameSceneで使う汎用テクスチャを先読みして、OnEnter側の実ロードを軽くする。
	stageLoadSteps_.push_back([]() {
		TextureManager::GetInstance()->LoadTexture("resources/gradation.png");
		});
	stageLoadSteps_.push_back([]() {
		TextureManager::GetInstance()->LoadTexture("resources/ui/white.png");
		});

	const std::string stageConfigPath = app.GetSelectedStageConfigPath();
	stageLoadSteps_.push_back([stageConfigPath]() {
		std::ifstream file(stageConfigPath);
		json root;
		if (file.is_open()) {
			try {
				file >> root;
			} catch (...) {
			}
		}
		});

	const std::string fieldConfigPath = app.GetSelectedStageFieldConfigPath();
	stageLoadSteps_.push_back([fieldConfigPath]() {
		std::ifstream file(fieldConfigPath);
		json root;
		if (file.is_open()) {
			try {
				file >> root;
			} catch (...) {
			}
		}
		});

	std::ifstream file(fieldConfigPath);
	if (file.is_open()) {
		json props;
		try {
			file >> props;
			if (props.is_array()) {
				for (const auto& prop : props) {
					addModel(prop.value("modelPath", ""));
				}
			}
		} catch (...) {
			modelPaths_.clear();
		}
	}

	if (modelPaths_.empty() && app.GetSelectedStageId() >= 1 && app.GetSelectedStageId() <= 4) {
		addModel("Field/ForestField/forestFIeld.obj");
		addModel("Field/ForestField/glassField.obj");
	}
}

void GameLoadingScene::UpdateText_()
{
	if (!loadingText_) {
		return;
	}

	const int dotCount = static_cast<int>(elapsed_ * 3.0f) % 4;
	std::wstring text = L"LOADING" + std::wstring(static_cast<size_t>(dotCount), L'.');
	if (mode_ == GameApp::LoadingMode::StageToGame) {
		text = loadComplete_ ? L"READY" : L"LOADING STAGE" + std::wstring(static_cast<size_t>(dotCount), L'.');
		if (!modelPaths_.empty()) {
			text += L" ";
			text += std::to_wstring((std::min)(loadIndex_, stageLoadSteps_.size()));
			text += L"/";
			text += std::to_wstring(stageLoadSteps_.size());
		}
	} else if (mode_ == GameApp::LoadingMode::SelectToStageSelect) {
		text = loadComplete_ ? L"READY" : L"LOADING STAGE SELECT" + std::wstring(static_cast<size_t>(dotCount), L'.');
		if (!stageLoadSteps_.empty()) {
			text += L" ";
			text += std::to_wstring((std::min)(loadIndex_, stageLoadSteps_.size()));
			text += L"/";
			text += std::to_wstring(stageLoadSteps_.size());
		}
	}
	loadingText_->SetText(text);
}

bool GameLoadingScene::LoadCurrentStep_(GameApp& app)
{
	if (mode_ == GameApp::LoadingMode::BootToTitle) {
		const bool complete = app.LoadStartupStep();
		UpdateText_();
		return complete;
	}

	if (loadIndex_ >= stageLoadSteps_.size()) {
		UpdateText_();
		return true;
	}

	stageLoadSteps_[loadIndex_]();
	++loadIndex_;
	UpdateText_();
	return loadIndex_ >= stageLoadSteps_.size();
}

float GameLoadingScene::GetProgress_(GameApp& app) const
{
	if (mode_ == GameApp::LoadingMode::BootToTitle) {
		return app.GetStartupLoadingProgress();
	}

	if (stageLoadSteps_.empty()) {
		return 1.0f;
	}

	return static_cast<float>(loadIndex_) / static_cast<float>(stageLoadSteps_.size());
}

void GameLoadingScene::Update(GameApp& app, float dt)
{
	elapsed_ += dt;
	const bool tipInputConsumed = HandleTipInput_(app);

	// 最低1フレームは描画だけを行い、ロード画面が見える状態にする。
	if (waitFrames_ < 1) {
		++waitFrames_;
		UpdateSprites_(app);
		return;
	}

	if (!loadComplete_) {
		loadComplete_ = LoadCurrentStep_(app);
	}

	const float targetProgress = GetProgress_(app);
	const float followSpeed = loadComplete_ ? 12.0f : 7.0f;
	if (loadComplete_ && mode_ == GameApp::LoadingMode::StageToGame) {
		// Stage loading waits for player confirmation once all resources are ready.
		displayProgress_ = 1.0f;
	} else {
		displayProgress_ += (targetProgress - displayProgress_) * Clamp01(followSpeed * dt);
		if (loadComplete_ && targetProgress >= 1.0f) {
			displayProgress_ = (std::max)(displayProgress_, 0.995f);
		}
	}

	UpdateSprites_(app);

	if (!loadComplete_ || elapsed_ < kMinShowTime || displayProgress_ < 0.99f) {
		return;
	}

	if (mode_ == GameApp::LoadingMode::BootToTitle) {
		RequestChangeScene_("Title");
		return;
	}

	if (mode_ == GameApp::LoadingMode::SelectToStageSelect) {
		RequestChangeScene_("StageSelect");
		return;
	}

	if (!stageScenePrepared_) {
		if (!stagePrepareFrameShown_) {
			stagePrepareFrameShown_ = true;
			return;
		}
		PrepareGameScene_(app);
		UpdateSprites_(app);
		return;
	}

	Input* input = app.GetInput();
	const bool startRequested =
		input && ((input->IsMouseTrigger(0) && !tipInputConsumed) ||
			input->IsKeyTrigger(DIK_RETURN) ||
			input->IsKeyTrigger(DIK_SPACE));
	if (startRequested) {
		app.Scenes().RequestPreparedChange("Game", std::move(preparedGameScene_));
	}
}

void GameLoadingScene::PrepareGameScene_(GameApp& app)
{
	// Build GameScene a little at a time while the loading screen keeps animating.
	if (!preparedGameScene_) {
		auto scene = std::make_unique<GameScene>();
		scene->BeginEnterPreparation(app);
		preparedGameScene_ = std::move(scene);
	}

	GameScene* gameScene = dynamic_cast<GameScene*>(preparedGameScene_.get());
	stageScenePrepared_ = gameScene && gameScene->PrepareEnterStep(app);
}

void GameLoadingScene::UpdateSprites_(GameApp& app)
{
	(void)app;
	const float progress = Clamp01(displayProgress_);
	const float filledWidth = (std::max)(kBarHeadSize, kBarWidth * progress);
	const float glowPulse = 0.5f + 0.5f * std::sin(elapsed_ * 5.0f);
	const float headX = kBarX + kBarWidth * progress;
	const float headPulse = 0.5f + 0.5f * std::sin(elapsed_ * 7.0f);

	if (barFill_) {
		barFill_->SetScale({ filledWidth, kBarHeight, 1.0f });
	}
	if (barGlow_) {
		barGlow_->SetScale(CirclePixelScale(filledWidth, kBarGlowHeight));
		barGlow_->SetColor({ 0.18f, 1.0f, 0.42f, 0.06f + glowPulse * 0.08f });
	}
	for (size_t i = 0; i < flowOrbs_.size(); ++i) {
		Sprite* orb = flowOrbs_[i].get();
		if (!orb) {
			continue;
		}

		const float offset = static_cast<float>(i) / static_cast<float>(flowOrbs_.size());
		const float phase = std::fmod(elapsed_ * 0.55f + offset, 1.0f);
		const float fadeIn = Clamp01(phase * 5.0f);
		const float fadeOut = Clamp01((1.0f - phase) * 4.0f);
		const float alpha = (0.10f + 0.38f * phase) * fadeIn * fadeOut * Clamp01(progress * 3.0f);
		const float orbSize = 8.0f + 8.0f * phase;
		const float x = kBarX + filledWidth * phase;
		const float yWave = std::sin(elapsed_ * 8.0f + static_cast<float>(i) * 1.7f) * 1.2f;

		orb->SetPosition({ x - orbSize * 0.5f, kBarY - orbSize * 0.5f + yWave });
		orb->SetScale(CirclePixelScale(orbSize, orbSize));
		orb->SetColor({ 0.52f, 1.0f, 0.64f, alpha });
	}
	if (barHeadGlow_) {
		const float glowSize = kBarHeadGlowSize + headPulse * 6.0f;
		barHeadGlow_->SetPosition({ headX - glowSize * 0.5f, kBarY - glowSize * 0.5f });
		barHeadGlow_->SetScale(CirclePixelScale(glowSize, glowSize));
		barHeadGlow_->SetColor({ 0.18f, 1.0f, 0.42f, 0.18f + headPulse * 0.14f });
	}
	if (barHeadCore_) {
		const float coreSize = kBarHeadSize + headPulse * 2.0f;
		barHeadCore_->SetPosition({ headX - coreSize * 0.5f, kBarY - coreSize * 0.5f });
		barHeadCore_->SetScale(CirclePixelScale(coreSize, coreSize));
		barHeadCore_->SetColor({ 0.90f, 1.0f, 0.90f, 0.95f });
	}
	if (percentText_) {
		const int percent = static_cast<int>(progress * 100.0f + 0.5f);
		percentText_->SetText(std::to_wstring(percent) + L"%");
	}
	if (loadingText_) {
		UpdateText_();
	}
	if (readyText_) {
		readyText_->SetText(stageScenePrepared_ ? L"READY  CLICK TO START" : L"PREPARING BATTLE");
	}
	if (tipText_) {
		tipText_->SetText(tipTextValue_);
	}
	if (tipCounterText_ && !displayedTips_.empty()) {
		tipCounterText_->SetText(
			std::to_wstring(currentTipIndex_ + 1) + L"/" + std::to_wstring(displayedTips_.size()));
	}
}

void GameLoadingScene::Draw2D(GameApp& app)
{
	app.SpriteCom()->SetGraphicsPipelineState();

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
		0, 0,
		static_cast<float>(WinApp::kClientWidth),
		static_cast<float>(WinApp::kClientHeight),
		0, 100
	);

	if (background_) {
		background_->Update(view, proj);
		background_->Draw();
	}
	if (barGlow_) {
		barGlow_->Update(view, proj);
		barGlow_->DrawAdditive();
	}
	if (barFill_) {
		barFill_->Update(view, proj);
		barFill_->DrawAdditive();
	}
	for (auto& orb : flowOrbs_) {
		if (orb) {
			orb->Update(view, proj);
			orb->DrawAdditive();
		}
	}
	if (barHeadGlow_) {
		barHeadGlow_->Update(view, proj);
		barHeadGlow_->DrawAdditive();
	}
	if (barHeadCore_) {
		barHeadCore_->Update(view, proj);
		barHeadCore_->DrawAdditive();
	}
	if (mode_ == GameApp::LoadingMode::StageToGame && stageInfoPanel_) {
		stageInfoPanel_->Update(view, proj);
		stageInfoPanel_->Draw();
	}
	if (stageTitleText_) {
		stageTitleText_->Update(view, proj);
		stageTitleText_->Draw();
	}
	if (enemyInfoText_) {
		enemyInfoText_->Update(view, proj);
		enemyInfoText_->Draw();
	}
	if (tipTitleText_) {
		tipTitleText_->Update(view, proj);
		tipTitleText_->Draw();
	}
	if (tipText_) {
		tipText_->Update(view, proj);
		tipText_->Draw();
	}
	if (tipLeftArrowText_) {
		tipLeftArrowText_->Update(view, proj);
		tipLeftArrowText_->Draw();
	}
	if (tipRightArrowText_) {
		tipRightArrowText_->Update(view, proj);
		tipRightArrowText_->Draw();
	}
	if (tipCounterText_) {
		tipCounterText_->Update(view, proj);
		tipCounterText_->Draw();
	}
	if (loadingText_) {
		loadingText_->Update(view, proj);
		loadingText_->Draw();
	}
	if (percentText_) {
		percentText_->Update(view, proj);
		percentText_->Draw();
	}
	if (readyText_) {
		readyText_->Update(view, proj);
		readyText_->Draw();
	}
}
