#include "StartupLoadingScene.h"

#include "GameApp.h"
#include "Matrix4x4.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "WinApp.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr float kBarX = 240.0f;
constexpr float kBarY = 560.0f;
constexpr float kBarWidth = 800.0f;
constexpr float kBarHeight = 4.0f;
constexpr float kMinShowTime = 1.2f;

float Clamp01(float value)
{
	return (std::max)(0.0f, (std::min)(1.0f, value));
}
}

void StartupLoadingScene::OnEnter(GameApp& app)
{
	TextureManager::GetInstance()->LoadTexture("resources/ui/white.png");
	app.BeginStartupLoading();

	displayProgress_ = 0.0f;
	elapsed_ = 0.0f;
	loadComplete_ = false;

	background_ = std::make_unique<Sprite>();
	background_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	background_->SetPosition({ 0.0f, 0.0f });
	background_->SetScale({ static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight), 1.0f });
	background_->SetColor({ 0.015f, 0.018f, 0.024f, 1.0f });

	barBack_ = std::make_unique<Sprite>();
	barBack_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	barBack_->SetPosition({ kBarX, kBarY });
	barBack_->SetScale({ kBarWidth, kBarHeight, 1.0f });
	barBack_->SetColor({ 0.18f, 0.21f, 0.27f, 1.0f });

	barFill_ = std::make_unique<Sprite>();
	barFill_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	barFill_->SetPosition({ kBarX, kBarY });
	barFill_->SetScale({ 1.0f, kBarHeight, 1.0f });
	barFill_->SetColor({ 0.22f, 0.86f, 1.0f, 1.0f });

	barGlow_ = std::make_unique<Sprite>();
	barGlow_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	barGlow_->SetPosition({ kBarX, kBarY - 7.0f });
	barGlow_->SetScale({ 1.0f, 18.0f, 1.0f });
	barGlow_->SetColor({ 0.12f, 0.68f, 1.0f, 0.20f });

	loadingText_ = std::make_unique<TextSprite>();
	loadingText_->Initialize(app.SpriteCom(), app.Dx());
	loadingText_->SetFontSize(30);
	loadingText_->SetSize({ 1.0f, 1.0f, 1.0f });
	loadingText_->SetPosition({ kBarX, kBarY - 54.0f });
	loadingText_->SetColor({ 0.88f, 0.94f, 1.0f });
	loadingText_->SetText(L"LOADING");

	percentText_ = std::make_unique<TextSprite>();
	percentText_->Initialize(app.SpriteCom(), app.Dx());
	percentText_->SetFontSize(24);
	percentText_->SetSize({ 1.0f, 1.0f, 1.0f });
	percentText_->SetPosition({ kBarX + kBarWidth - 58.0f, kBarY - 48.0f });
	percentText_->SetColor({ 0.62f, 0.72f, 0.82f });

	UpdateSprites_(app);
}

void StartupLoadingScene::OnExit(GameApp& app)
{
	(void)app;
	background_.reset();
	barBack_.reset();
	barFill_.reset();
	barGlow_.reset();
	loadingText_.reset();
	percentText_.reset();
}

void StartupLoadingScene::Update(GameApp& app, float dt)
{
	elapsed_ += dt;

	if (!loadComplete_) {
		loadComplete_ = app.LoadStartupStep();
	}

	const float targetProgress = app.GetStartupLoadingProgress();
	const float followSpeed = loadComplete_ ? 12.0f : 7.0f;
	displayProgress_ += (targetProgress - displayProgress_) * Clamp01(followSpeed * dt);
	if (loadComplete_ && targetProgress >= 1.0f) {
		displayProgress_ = (std::max)(displayProgress_, 0.995f);
	}

	UpdateSprites_(app);

	if (loadComplete_ && elapsed_ >= kMinShowTime && displayProgress_ >= 0.99f) {
		RequestChangeScene_("Title");
	}
}

void StartupLoadingScene::UpdateSprites_(GameApp& app)
{
	(void)app;
	const float progress = Clamp01(displayProgress_);
	const float filledWidth = (std::max)(1.0f, kBarWidth * progress);
	const float glowPulse = 0.5f + 0.5f * std::sin(elapsed_ * 5.0f);

	if (barFill_) {
		barFill_->SetScale({ filledWidth, kBarHeight, 1.0f });
	}
	if (barGlow_) {
		barGlow_->SetScale({ filledWidth, 18.0f, 1.0f });
		barGlow_->SetColor({ 0.12f, 0.68f, 1.0f, 0.12f + glowPulse * 0.10f });
	}
	if (percentText_) {
		const int percent = static_cast<int>(Clamp01(progress) * 100.0f + 0.5f);
		percentText_->SetText(std::to_wstring(percent) + L"%");
	}
}

void StartupLoadingScene::Draw2D(GameApp& app)
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
	if (barBack_) {
		barBack_->Update(view, proj);
		barBack_->Draw();
	}
	if (barGlow_) {
		barGlow_->Update(view, proj);
		barGlow_->Draw();
	}
	if (barFill_) {
		barFill_->Update(view, proj);
		barFill_->Draw();
	}
	if (loadingText_) {
		loadingText_->Update(view, proj);
		loadingText_->Draw();
	}
	if (percentText_) {
		percentText_->Update(view, proj);
		percentText_->Draw();
	}
}
