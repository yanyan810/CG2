#include "StageSelectScene.h"
#include "DeckEditScene.h"

#include "GameApp.h"
#include "Camera.h"
#include "Input.h"
#include "WinApp.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "Matrix4x4.h"
#include "AudioManager.h"
#include "TextureManager.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>

namespace {
	std::string MakeStageConfigPath_(int stageId)
	{
		return "resources/stages/stage" + std::string(stageId < 10 ? "0" : "") + std::to_string(stageId) + ".json";
	}

	std::string MakeStageFieldConfigPath_(int stageId)
	{
		if (stageId <= 0) {
			return "resources/configs/stage_fields/tutorial_field.json";
		}

		const std::string prefix = stageId < 10 ? "stage0" : "stage";
		return "resources/configs/stage_fields/" + prefix + std::to_string(stageId) + "_field.json";
	}

	std::wstring MakeStageLabel_(int stageId)
	{
		if (stageId == 10) {
			return L"ステージ10 ボス戦";
		}

		return L"ステージ" + std::to_wstring(stageId) + L" バトル";
	}

	std::wstring MakeStageDescription_(int stageId)
	{
		if (stageId == 10) {
			return L"強敵とのボス戦です";
		}

		return L"ステージ" + std::to_wstring(stageId) + L"のバトルを開始します";
	}

	BloomParam MakeBossStageWarningParam_(const BloomParam& baseParam, bool isBurst)
	{
		BloomParam param = baseParam;
		param.threshold = 0.0f;
		param.intensity = 0.55f;
		param.vignetteIntensity = 0.0f;
		param.vignetteScale = 0.0f;
		param.chromAbAmount = isBurst ? 0.012f : 0.004f;
		param.distortionAmount = 0.0008f;
		param.noiseIntensity = isBurst ? 0.35f : 0.16f;
		param.scanlineIntensity = isBurst ? 0.30f : 0.12f;
		param.scanlineFrequency = 140.0f;
		param.glitchAmount = isBurst ? 0.04f : 0.008f;
		param.curvature = 0.0f;
		param.borderSharp = 0.0f;
		param.isGrayscale = 0.0f;
		param.isInverted = 0.0f;
		param.dissolveAmount = -1.0f;
		return param;
	}
}

std::vector<std::unique_ptr<PropManager>> StageSelectScene::stageFieldPropsCache_;
bool StageSelectScene::stageFieldPropsCacheReady_ = false;
int StageSelectScene::stageFieldPropsCacheLoadIndex_ = 1;

void StageSelectScene::BeginStageFieldCacheLoading()
{
	if (stageFieldPropsCacheReady_) {
		stageFieldPropsCacheLoadIndex_ = 11;
		return;
	}

	stageFieldPropsCache_.clear();
	stageFieldPropsCache_.resize(11);
	stageFieldPropsCacheLoadIndex_ = 1;
}

bool StageSelectScene::LoadStageFieldCacheStep(GameApp& app)
{
	if (stageFieldPropsCacheReady_) {
		return true;
	}

	if (stageFieldPropsCache_.size() < 11) {
		stageFieldPropsCache_.clear();
		stageFieldPropsCache_.resize(11);
	}

	if (stageFieldPropsCacheLoadIndex_ >= 1 && stageFieldPropsCacheLoadIndex_ <= 10) {
		auto props = std::make_unique<PropManager>();
		props->Initialize(app.ObjCom(), app.Dx(), nullptr);
		props->LoadFromJson(MakeStageFieldConfigPath_(stageFieldPropsCacheLoadIndex_));
		props->Update(0.0f);
		props->WarmupDrawResources();
		stageFieldPropsCache_[stageFieldPropsCacheLoadIndex_] = std::move(props);
		++stageFieldPropsCacheLoadIndex_;
	}

	if (stageFieldPropsCacheLoadIndex_ > 10) {
		stageFieldPropsCacheReady_ = true;
		return true;
	}

	return false;
}

float StageSelectScene::GetStageFieldCacheLoadingProgress()
{
	if (stageFieldPropsCacheReady_) {
		return 1.0f;
	}

	const int loadedCount = std::clamp(stageFieldPropsCacheLoadIndex_ - 1, 0, 10);
	return static_cast<float>(loadedCount) / 10.0f;
}

bool StageSelectScene::PointInRect_(float mx, float my, const Rect& rect) const {
	return mx >= rect.x &&
		mx <= rect.x + rect.w &&
		my >= rect.y &&
		my <= rect.y + rect.h;
}

void StageSelectScene::SelectStageItem_(GameApp& app, const StageItem& item)
{
	if (item.stageId > 0) {
		app.SetSelectedStage(item.stageId, item.stageConfigPath);
		// ステージ決定後は本編用ロードを挟んでからGameSceneへ遷移する。
		app.SetLoadingMode(GameApp::LoadingMode::StageToGame);
		RequestChangeScene_("GameLoading");
		return;
	}

	if (item.sceneName == "DeckEdit") {
		DeckEditScene::returnSceneName_ = "StageSelect";
	}

	RequestChangeScene_(item.sceneName.c_str());
}

void StageSelectScene::ChangeStage_(int delta)
{
	const int prevStageId = currentStageId_;
	const int nextStageId = currentStageId_ + delta;
	if (nextStageId < 1 || nextStageId > 10) {
		return;
	}
	currentStageId_ = nextStageId;
	if (currentStageId_ == 10 && prevStageId != 10) {
		bossWarningBurstTimer_ = bossWarningBurstDuration_;
		bossShakeTimer_ = bossShakeDuration_;
	} else if (currentStageId_ != 10) {
		bossWarningBurstTimer_ = 0.0f;
		bossShakeTimer_ = 0.0f;
	}
	ApplyCurrentStageToBattleItem_();
	UpdateStageFieldBackground_();
	selectIndex_ = 0;
}

void StageSelectScene::ApplyCurrentStageToBattleItem_()
{
	if (stageItems_.empty()) {
		return;
	}

	stageItems_[0].displayText = MakeStageLabel_(currentStageId_);
	stageItems_[0].descText = MakeStageDescription_(currentStageId_);
	stageItems_[0].stageId = currentStageId_;
	stageItems_[0].stageConfigPath = MakeStageConfigPath_(currentStageId_);

	std::string texturePath;
	if (currentStageId_ <= 5) {
		texturePath = "resources/ui/stage_select/Stages_1~5.png";
	} else if (currentStageId_ <= 9) {
		texturePath = "resources/ui/stage_select/Stages_6~9.png";
	} else {
		texturePath = "resources/ui/stage_select/Boss_stage.png";
	}
	if (stageItems_[0].buttonSprite) {
		stageItems_[0].buttonSprite->SetTextureFilePath(texturePath);
	}
}

void StageSelectScene::LoadStageFieldBackgrounds_(GameApp& app)
{
	if (!stageFieldPropsCacheReady_) {
		stageFieldPropsCache_.clear();
		stageFieldPropsCache_.resize(11);

		for (int stageId = 1; stageId <= 10; ++stageId) {
			auto props = std::make_unique<PropManager>();
			props->Initialize(app.ObjCom(), app.Dx(), camera_.get());
			props->LoadFromJson(MakeStageFieldConfigPath_(stageId));
			props->Update(0.0f);
			props->WarmupDrawResources();
			stageFieldPropsCache_[stageId] = std::move(props);
		}

		stageFieldPropsCacheReady_ = true;
		return;
	}

	for (auto& props : stageFieldPropsCache_) {
		if (props) {
			props->SetCamera(camera_.get());
			props->Update(0.0f);
			props->WarmupDrawResources();
		}
	}
}

void StageSelectScene::UpdateStageFieldBackground_()
{
	stageFieldConfigPath_ = MakeStageFieldConfigPath_(currentStageId_);
	stageFieldProps_ = nullptr;
	if (currentStageId_ >= 1 &&
		currentStageId_ < static_cast<int>(stageFieldPropsCache_.size())) {
		stageFieldProps_ = stageFieldPropsCache_[currentStageId_].get();
	}
	if (stageFieldProps_ && camera_) {
		stageFieldProps_->SetCamera(camera_.get());
		stageFieldProps_->Update(0.0f);
		stageFieldProps_->WarmupDrawResources();
	}
}

void StageSelectScene::OnEnter(GameApp& app) {
	hoverIndex_ = -1;
	selectIndex_ = 0;
	isUsingMouse_ = true;
	currentStageId_ = std::clamp(app.GetSelectedStageId(), 1, 10);
	bossWarningBurstTimer_ = 0.0f;
	bossShakeTimer_ = 0.0f;
	circle_ = 0.0f;
	softness_ = 0.6f;
	AudioManager::GetInstance()->PlayBGM("BGM_TitleSelect");

	TextureManager::GetInstance()->LoadTexture("resources/ui/text/Stage_select.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/Deck_editing.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/return.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/stage_select/Stages_1~5.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/stage_select/Stages_6~9.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/stage_select/Boss_stage.png");

	camera_ = std::make_unique<Camera>();
	camera_->SetTranslate({ 0.0f, 4.0f, -40.0f });
	camera_->SetRotate({ 0.15f, 0.0f, 0.0f });
	app.ObjCom()->SetDefaultCamera(camera_.get());

	skyDome_ = std::make_unique<Object3d>();
	skyDome_->Initialize(app.ObjCom(), app.Dx());
	skyDome_->SetModel("skydome/skydome.obj");
	skyDome_->SetCamera(camera_.get());
	skyDome_->SetEnableLighting(0);
	skyDome_->SetTranslate({ 0.0f, 0.0f, 0.0f });
	skyDome_->SetScale({ 100.0f, 100.0f, 100.0f });

	LoadStageFieldBackgrounds_(app);
	UpdateStageFieldBackground_();

	bossStageWarningOverlay_ = std::make_unique<Sprite>();
	bossStageWarningOverlay_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	bossStageWarningOverlay_->SetAnchorPoint({ 0.0f, 0.0f });
	bossStageWarningOverlay_->SetPosition({ 0.0f, 0.0f });
	const DirectX::TexMetadata& whiteMeta =
		TextureManager::GetInstance()->GetMetaData("resources/ui/white.png");
	bossStageWarningOverlay_->SetScale({
		static_cast<float>(WinApp::kClientWidth) / static_cast<float>(whiteMeta.width),
		static_cast<float>(WinApp::kClientHeight) / static_cast<float>(whiteMeta.height),
		1.0f
		});
	bossStageWarningOverlay_->SetColor({ 1.0f, 0.04f, 0.02f, 0.18f });

	titleSprite_ = std::make_unique<Sprite>();
	titleSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/Stage_select.png");
	titleSprite_->SetAnchorPoint({ 0.0f, 0.0f });
	titleSprite_->SetPosition({ 470.0f, 80.0f });

	descBgTop_ = std::make_unique<Sprite>();
	descBgTop_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/stage_select/desc_bg.png");
	descBgTop_->SetAnchorPoint({ 0.0f, 0.0f });
	descBgTop_->SetPosition({ 760.0f, 300.0f });

	descBgBottom_ = std::make_unique<Sprite>();
	descBgBottom_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/stage_select/desc_bg.png");
	descBgBottom_->SetAnchorPoint({ 0.0f, 0.0f });
	descBgBottom_->SetPosition({ 760.0f, 500.0f });

	stageItems_.clear();
	stageItems_.resize(2);

	descTextSprite_ = std::make_unique<TextSprite>();
	descTextSprite_->Initialize(app.SpriteCom(), app.Dx());
	descTextSprite_->SetSize({ 1.0f, 1.0f, 1.0f });
	descTextSprite_->SetFontSize(28);
	descTextSprite_->SetAlpha(1.0f);
	descTextSprite_->SetColor({ 1.0f, 1.0f, 1.0f });
	descTextSprite_->SetOutline(textOutlineColor_, textOutlineWidth_);

	stageItems_[0].sceneName = "Game";
	ApplyCurrentStageToBattleItem_();
	stageItems_[0].buttonRect = {
	layout_.stages1_5Rect.x,
	layout_.stages1_5Rect.y,
	layout_.stages1_5Rect.w,
	layout_.stages1_5Rect.h
	};
	stageItems_[0].buttonSprite = std::make_unique<Sprite>();
	stageItems_[0].buttonSprite->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/stage_select/Stages_1~5.png");
	stageItems_[0].buttonSprite->SetAnchorPoint({ 0.0f, 0.0f });
	stageItems_[0].buttonSprite->SetPosition({ stageItems_[0].buttonRect.x, stageItems_[0].buttonRect.y });

	stageItems_[1].sceneName = "DeckEdit";
	stageItems_[1].displayText = L""; // テキストは画像に含まれるため空
	stageItems_[1].descText = L"使用するデッキを編成します";
	stageItems_[1].buttonRect = {
	layout_.deckEditButtonRect.x,
	layout_.deckEditButtonRect.y,
	layout_.deckEditButtonRect.w,
	layout_.deckEditButtonRect.h
	};
	stageItems_[1].buttonSprite = std::make_unique<Sprite>();
	stageItems_[1].buttonSprite->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/Deck_editing.png");
	stageItems_[1].buttonSprite->SetAnchorPoint({ 0.0f, 0.0f });
	stageItems_[1].buttonSprite->SetPosition({ stageItems_[1].buttonRect.x, stageItems_[1].buttonRect.y });

	debugHitBgs_.resize(stageItems_.size());
	for (int i = 0; i < static_cast<int>(stageItems_.size()); ++i) {
		debugHitBgs_[i] = std::make_unique<Sprite>();
		debugHitBgs_[i]->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
		debugHitBgs_[i]->SetAnchorPoint({ 0.0f, 0.0f });
	}

	itemTextSprites_.resize(stageItems_.size());
	for (int i = 0; i < static_cast<int>(stageItems_.size()); ++i) {
		itemTextSprites_[i] = std::make_unique<TextSprite>();
		itemTextSprites_[i]->Initialize(app.SpriteCom(), app.Dx());
		itemTextSprites_[i]->SetText(stageItems_[i].displayText);
		itemTextSprites_[i]->SetFontSize(stageItems_[i].stageId > 0 ? 40 : 34);
		itemTextSprites_[i]->SetSize({ 1.0f, 1.0f, 1.0f });
		itemTextSprites_[i]->SetAlpha(1.0f);
		itemTextSprites_[i]->SetOutline(textOutlineColor_, textOutlineWidth_);
	}

	leftArrowText_ = std::make_unique<TextSprite>();
	leftArrowText_->Initialize(app.SpriteCom(), app.Dx());
	leftArrowText_->SetText(L"<");
	leftArrowText_->SetFontSize(64);
	leftArrowText_->SetSize({ 1.0f, 1.0f, 1.0f });
	leftArrowText_->SetAlpha(1.0f);
	leftArrowText_->SetOutline(textOutlineColor_, textOutlineWidth_);

	rightArrowText_ = std::make_unique<TextSprite>();
	rightArrowText_->Initialize(app.SpriteCom(), app.Dx());
	rightArrowText_->SetText(L">");
	rightArrowText_->SetFontSize(64);
	rightArrowText_->SetSize({ 1.0f, 1.0f, 1.0f });
	rightArrowText_->SetAlpha(1.0f);
	rightArrowText_->SetOutline(textOutlineColor_, textOutlineWidth_);

	backButtonBg_ = std::make_unique<Sprite>();
	backButtonBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/return.png");
	backButtonBg_->SetAnchorPoint({ 0.0f, 0.0f });
	backButtonBg_->SetPosition({ backButtonRect_.x, backButtonRect_.y });

	// backButtonText_ is no longer needed since return.png has text

	LoadLayout_();
	ApplyLayout_();

	currentDescText_ = stageItems_[0].descText;
	currentDescPos_ = {
		stageItems_[0].descRect.x,
		stageItems_[0].descRect.y
	};

	if (descTextSprite_) {
		descTextSprite_->SetText(currentDescText_);
		descTextSprite_->SetPosition(currentDescPos_);
	}
}

void StageSelectScene::OnExit(GameApp& app) {
	(void)app;
	backButtonText_.reset();
	backButtonBg_.reset();
	rightArrowText_.reset();
	leftArrowText_.reset();
	itemTextSprites_.clear();
	stageItems_.clear();
	descBgBottom_.reset();
	descBgTop_.reset();
	titleSprite_.reset();
	bossStageWarningOverlay_.reset();
	stageFieldProps_ = nullptr;
	for (auto& props : stageFieldPropsCache_) {
		if (props) {
			props->SetCamera(nullptr);
		}
	}
	skyDome_.reset();
	camera_.reset();
	descTextSprite_.reset();
}

void StageSelectScene::Update(GameApp& app, float dt) {
	ApplyLayout_();
	if (skyDome_) {
		skyDome_->SetCamera(camera_.get());
		skyDome_->Update(dt);
	}
	if (stageFieldProps_) {
		stageFieldProps_->SetCamera(camera_.get());
		stageFieldProps_->Update(dt);
	}
	if (currentStageId_ == 10 && bossWarningBurstTimer_ > 0.0f) {
		bossWarningBurstTimer_ -= dt;
		if (bossWarningBurstTimer_ < 0.0f) {
			bossWarningBurstTimer_ = 0.0f;
		}
	} else if (currentStageId_ != 10) {
		bossWarningBurstTimer_ = 0.0f;
	}
	if (currentStageId_ == 10 && bossShakeTimer_ > 0.0f) {
		bossShakeTimer_ -= dt;
		if (bossShakeTimer_ < 0.0f) {
			bossShakeTimer_ = 0.0f;
		}
	} else if (currentStageId_ != 10) {
		bossShakeTimer_ = 0.0f;
	}

	Input* input = app.GetInput();
	if (!input) {
		return;
	}

	POINT mouse = input->GetMousePosition();
	const float mx = static_cast<float>(mouse.x);
	const float my = static_cast<float>(mouse.y);

	POINT mouseDelta = input->GetMouseDelta();
	if (mouseDelta.x != 0 || mouseDelta.y != 0 || input->IsMouseTrigger(0)) {
		isUsingMouse_ = true;
	}

	hoverIndex_ = -1;

	if (PointInRect_(mx, my, backButtonRect_) && input->IsMouseTrigger(0)) {
		AudioManager::GetInstance()->PlaySE("SE_Tap");
		RequestChangeScene_("Select");
		return;
	}

	if (currentStageId_ > 1 && PointInRect_(mx, my, leftArrowRect_) && input->IsMouseTrigger(0)) {
		AudioManager::GetInstance()->PlaySE("SE_Tap");
		ChangeStage_(-1);
		return;
	}

	if (currentStageId_ < 10 && PointInRect_(mx, my, rightArrowRect_) && input->IsMouseTrigger(0)) {
		AudioManager::GetInstance()->PlaySE("SE_Tap");
		ChangeStage_(1);
		return;
	}

	for (int i = 0; i < static_cast<int>(stageItems_.size()); ++i) {
		if (PointInRect_(mx, my, stageItems_[i].buttonRect)) {
			hoverIndex_ = i;
			break;
		}
	}

	if (hoverIndex_ != -1) {
		if (input->IsMouseTrigger(0)) {
			AudioManager::GetInstance()->PlaySE("SE_Tap");
			selectIndex_ = hoverIndex_;
			SelectStageItem_(app, stageItems_[selectIndex_]);
			return;
		}
	}



	const int showIndex = isUsingMouse_ ? hoverIndex_ : selectIndex_;
	if (showIndex >= 0 && showIndex < static_cast<int>(stageItems_.size())) {
		currentDescText_ = stageItems_[showIndex].descText;
		currentDescPos_ = {
			stageItems_[showIndex].descRect.x,
			stageItems_[showIndex].descRect.y
		};
	} else {
		currentDescText_ = L"";
	}
}

void StageSelectScene::Draw3D(GameApp& app) {
	app.ObjCom()->SetGraphicsPipelineState();
	if (stageFieldProps_) {
		stageFieldProps_->Draw3D();
	}
}

void StageSelectScene::DrawDescriptionText_(GameApp& app, const std::wstring& text, float x, float y) {
	if (!descTextSprite_) {
		return;
	}

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
		0.0f, 0.0f,
		static_cast<float>(WinApp::kClientWidth),
		static_cast<float>(WinApp::kClientHeight),
		0.0f, 100.0f
	);

	float scale = 1.2f;
	const int showIndex = isUsingMouse_ ? hoverIndex_ : selectIndex_;
	if (showIndex == 0) {
		scale = layout_.battleDescText.scale;
	} else if (showIndex == 1) {
		//scale = layout_.deckEditDescText.scale;
	}

	descTextSprite_->SetText(text);
	descTextSprite_->SetOutline(textOutlineColor_, textOutlineWidth_);
	descTextSprite_->SetPosition({ x, y });
	descTextSprite_->SetSize({ scale, scale, 1.0f });
	descTextSprite_->Update(view, proj);
	descTextSprite_->Draw();
}

void StageSelectScene::Draw2D(GameApp& app) {
	app.SpriteCom()->SetGraphicsPipelineState();

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
		0.0f, 0.0f,
		static_cast<float>(WinApp::kClientWidth),
		static_cast<float>(WinApp::kClientHeight),
		0.0f, 100.0f
	);

	if (descTextSprite_) {
		descTextSprite_->SetOutline(textOutlineColor_, textOutlineWidth_);
	}
	if (backButtonText_) {
		backButtonText_->SetOutline(textOutlineColor_, textOutlineWidth_);
	}
	if (leftArrowText_) {
		leftArrowText_->SetOutline(textOutlineColor_, textOutlineWidth_);
	}
	if (rightArrowText_) {
		rightArrowText_->SetOutline(textOutlineColor_, textOutlineWidth_);
	}
	for (auto& text : itemTextSprites_) {
		if (text) {
			text->SetOutline(textOutlineColor_, textOutlineWidth_);
		}
	}

	Vector2 shakeOffset{ 0.0f, 0.0f };
	if (currentStageId_ == 10 && bossShakeTimer_ > 0.0f && bossShakeDuration_ > 0.0f) {
		const float elapsed = bossShakeDuration_ - bossShakeTimer_;
		const float strength = bossShakeMagnitude_ * (bossShakeTimer_ / bossShakeDuration_);
		shakeOffset.x = static_cast<float>(std::sin(elapsed * 90.0f)) * strength;
		shakeOffset.y = static_cast<float>(std::cos(elapsed * 72.0f)) * strength * 0.55f;
	}

	if (currentStageId_ == 10 && bossStageWarningOverlay_) {
		bossStageWarningOverlay_->SetPosition({ shakeOffset.x, shakeOffset.y });
		bossStageWarningOverlay_->SetColor({ 1.0f, 0.04f, 0.02f, 0.14f }); // Drawで透過描画
		bossStageWarningOverlay_->Update(view, proj);
		bossStageWarningOverlay_->Draw();
	}


	if (titleSprite_) {
		titleSprite_->SetPosition({ layout_.titlePos.x + shakeOffset.x, layout_.titlePos.y + shakeOffset.y });
		titleSprite_->Update(view, proj);
		titleSprite_->Draw();
	}

	const bool isBackHovered = [&]() {
		Input* input = app.GetInput();
		if (!input) {
			return false;
		}
		const POINT mouse = input->GetMousePosition();
		return PointInRect_(
			static_cast<float>(mouse.x),
			static_cast<float>(mouse.y),
			backButtonRect_);
		}();

	if (backButtonBg_) {
		const DirectX::TexMetadata& meta = TextureManager::GetInstance()->GetMetaData(backButtonBg_->GetTextureFilePath());
		backButtonBg_->SetPosition({ backButtonRect_.x + shakeOffset.x, backButtonRect_.y + shakeOffset.y });
		backButtonBg_->SetScale({ backButtonRect_.w / static_cast<float>(meta.width), backButtonRect_.h / static_cast<float>(meta.height), 1.0f });
		backButtonBg_->SetColor(isBackHovered
			? Vector4{ 1.0f, 1.0f, 1.0f, 1.0f }
			: Vector4{ 0.8f, 0.8f, 0.8f, 1.0f });
		backButtonBg_->Update(view, proj);
		backButtonBg_->Draw();
	}

	if (backButtonText_) {
		backButtonText_->SetText(L"戻る");
		backButtonText_->SetColor(isBackHovered
			? Vector3{ 1.0f, 0.92f, 0.45f }
			: Vector3{ 1.0f, 1.0f, 1.0f });
		backButtonText_->SetPosition({ backButtonRect_.x + 38.0f + shakeOffset.x, backButtonRect_.y + 13.0f + shakeOffset.y });
		backButtonText_->Update(view, proj);
		backButtonText_->Draw();
	}

	for (int i = 0; i < static_cast<int>(stageItems_.size()); ++i) {
		if (!stageItems_[i].buttonSprite) {
			continue;
		}

		const bool isHovered = (i == hoverIndex_);
		const bool isSelected = (i == selectIndex_);
		const bool shouldHighlight = isUsingMouse_ ? isHovered : isSelected;

		float scale = 1.0f;
		if (shouldHighlight) {
			scale = hoverScale_;
		}

		const DirectX::TexMetadata& meta = TextureManager::GetInstance()->GetMetaData(stageItems_[i].buttonSprite->GetTextureFilePath());
		float scaleX = (stageItems_[i].buttonRect.w * scale) / static_cast<float>(meta.width);
		float scaleY = (stageItems_[i].buttonRect.h * scale) / static_cast<float>(meta.height);

		stageItems_[i].buttonSprite->SetPosition({
			stageItems_[i].buttonRect.x + shakeOffset.x,
			stageItems_[i].buttonRect.y + shakeOffset.y
			});
		stageItems_[i].buttonSprite->SetScale({ scaleX, scaleY, 1.0f });

		Vector4 btnColor = shouldHighlight ? Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } : Vector4{ 0.8f, 0.8f, 0.8f, 1.0f };
		stageItems_[i].buttonSprite->SetColor(btnColor);

		stageItems_[i].buttonSprite->Update(view, proj);
		stageItems_[i].buttonSprite->Draw();
	}

	for (int i = 0; i < static_cast<int>(itemTextSprites_.size()); ++i) {
		if (!itemTextSprites_[i]) {
			continue;
		}

		const Rect& r = stageItems_[i].buttonRect;
		const bool isHovered = (i == hoverIndex_);
		const bool isSelected = (i == selectIndex_);
		const bool shouldHighlight = isUsingMouse_ ? isHovered : isSelected;
		
		const bool isBossStage = stageItems_[i].stageId == 10;
		Vector3 textColor = { 1.0f, 1.0f, 1.0f };
		if (isBossStage) {
			textColor = { 1.0f, 0.78f, 0.2f };
		}
		if (shouldHighlight) {
			textColor = { 1.0f, 0.92f, 0.45f };
		}

		itemTextSprites_[i]->SetText(stageItems_[i].displayText);
		itemTextSprites_[i]->SetColor(textColor);
		Vector2 textPos{ r.x + 72.0f, r.y + 42.0f };
		if (i == 1) {
			textPos = { r.x + 82.0f, r.y + 40.0f };
		} else if (stageItems_[i].stageId > 0) {
			textPos = { r.x + layout_.battleTitleTextOffset.x, r.y + layout_.battleTitleTextOffset.y };
		}
		itemTextSprites_[i]->SetPosition({ textPos.x + shakeOffset.x, textPos.y + shakeOffset.y });
		itemTextSprites_[i]->Update(view, proj);
		itemTextSprites_[i]->Draw();
	}

	if (leftArrowText_ && currentStageId_ > 1) {
		leftArrowText_->SetColor({ 1.0f, 1.0f, 1.0f });
		leftArrowText_->SetPosition({ leftArrowRect_.x + shakeOffset.x, leftArrowRect_.y + shakeOffset.y });
		leftArrowText_->Update(view, proj);
		leftArrowText_->Draw();
	}

	if (rightArrowText_ && currentStageId_ < 10) {
		rightArrowText_->SetColor({ 1.0f, 1.0f, 1.0f });
		rightArrowText_->SetPosition({ rightArrowRect_.x + shakeOffset.x, rightArrowRect_.y + shakeOffset.y });
		rightArrowText_->Update(view, proj);
		rightArrowText_->Draw();
	}

	if (descBgBottom_) {
		descBgBottom_->SetPosition({ 360.0f + shakeOffset.x, 560.0f + shakeOffset.y });
		descBgBottom_->Update(view, proj);
		descBgBottom_->Draw();
	}

	DrawDescriptionText_(app, currentDescText_, currentDescPos_.x + shakeOffset.x, currentDescPos_.y + shakeOffset.y);

	// デバッグ用：当たり判定表示
	if (showDebugHitBox_) {
		for (int i = 0; i < static_cast<int>(stageItems_.size()); ++i) {
			if (i >= static_cast<int>(debugHitBgs_.size())) {
				continue;
			}
			const Rect& r = stageItems_[i].buttonRect;

			const bool isHovered = (i == hoverIndex_);
			const bool isSelected = (i == selectIndex_);
			const bool shouldHighlight = isUsingMouse_ ? isHovered : isSelected;

			Vector4 color = { 0.0f, 0.0f, 0.0f, 0.45f };
			if (shouldHighlight) {
				color = { 1.0f, 0.0f, 0.0f, 0.45f };
			}

			debugHitBgs_[i]->SetPosition({ r.x, r.y });
			debugHitBgs_[i]->SetScale({ r.w, r.h, 1.0f });
			debugHitBgs_[i]->SetColor(color);
			debugHitBgs_[i]->Update(view, proj);
			debugHitBgs_[i]->Draw();
		}
	}

	//  app.SpriteCom()->DrawCircleMask(circle_, softness_);
}

void StageSelectScene::DrawImGui(GameApp& app) {
#ifdef USE_IMGUI
	(void)app;
	ImGui::Begin("StageSelectLayout");

	ImGui::Checkbox("ShowDebugHitBox", &showDebugHitBox_);

	ImGui::Separator();
	ImGui::Text("Text Outline");
	float outlineColor[3] = {
		textOutlineColor_.x,
		textOutlineColor_.y,
		textOutlineColor_.z,
	};
	if (ImGui::ColorEdit3("Text Outline Color", outlineColor)) {
		textOutlineColor_ = { outlineColor[0], outlineColor[1], outlineColor[2] };
	}
	ImGui::SliderFloat("Text Outline Width", &textOutlineWidth_, 0.0f, 10.0f);

	ImGui::Separator();
	ImGui::Text("Title");
	ImGui::DragFloat2("titlePos", &layout_.titlePos.x, 1.0f);

	ImGui::Separator();
	ImGui::Text("Tutorial");
	ImGui::DragFloat4("tutorialButtonRect", &layout_.tutorialButtonRect.x, 1.0f);
	ImGui::DragFloat2("tutorialDescBgPos", &layout_.tutorialDescBgPos.x, 1.0f);
	ImGui::DragFloat2("tutorialDescTextPos", &layout_.tutorialDescText.x, 1.0f);
	ImGui::DragFloat("tutorialDescTextScale", &layout_.tutorialDescText.scale, 0.01f, 0.1f, 5.0f);

	ImGui::Separator();
	ImGui::Text("Battle Texts");
	ImGui::DragFloat2("battleTitleTextOffset", &layout_.battleTitleTextOffset.x, 1.0f);
	ImGui::DragFloat2("battleDescBgPos", &layout_.battleDescBgPos.x, 1.0f);
	ImGui::DragFloat2("battleDescTextPos", &layout_.battleDescText.x, 1.0f);
	ImGui::DragFloat("battleDescTextScale", &layout_.battleDescText.scale, 0.01f, 0.1f, 5.0f);

	ImGui::Separator();
	ImGui::Text("Tutorial Visual");
	ImGui::DragFloat2("tutorialButtonVisualPos", &layout_.tutorialButtonVisualPos.x, 1.0f);

	ImGui::Text("Battle Visual");
	ImGui::DragFloat("battleDescTextScale", &layout_.battleDescText.scale, 0.01f, 0.1f, 5.0f);
	ImGui::DragFloat4("tutorialButtonRect", &layout_.tutorialButtonRect.x, 1.0f);

	ImGui::Separator();
	if (ImGui::Button("Save StageSelectLayout")) {
		SaveLayout_();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load StageSelectLayout")) {
		LoadLayout_();
		ApplyLayout_();
	}

	ImGui::Separator();
	if (ImGui::TreeNode("Title Box")) {
		float pos[2] = { layout_.titlePos.x, layout_.titlePos.y };
		if (ImGui::DragFloat2("Pos##Title", pos, 1.0f)) {
			layout_.titlePos = { pos[0], pos[1] };
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Back Button")) {
		float rect[4] = { layout_.backButtonRect.x, layout_.backButtonRect.y, layout_.backButtonRect.w, layout_.backButtonRect.h };
		if (ImGui::DragFloat4("Rect##Back", rect, 1.0f)) {
			layout_.backButtonRect = { rect[0], rect[1], rect[2], rect[3] };
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Stages 1~5 Frame")) {
		float rect[4] = { layout_.stages1_5Rect.x, layout_.stages1_5Rect.y, layout_.stages1_5Rect.w, layout_.stages1_5Rect.h };
		if (ImGui::DragFloat4("Rect##Stages1_5", rect, 1.0f)) {
			layout_.stages1_5Rect = { rect[0], rect[1], rect[2], rect[3] };
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Stages 6~9 Frame")) {
		float rect[4] = { layout_.stages6_9Rect.x, layout_.stages6_9Rect.y, layout_.stages6_9Rect.w, layout_.stages6_9Rect.h };
		if (ImGui::DragFloat4("Rect##Stages6_9", rect, 1.0f)) {
			layout_.stages6_9Rect = { rect[0], rect[1], rect[2], rect[3] };
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Boss Stage Frame")) {
		float rect[4] = { layout_.bossStageRect.x, layout_.bossStageRect.y, layout_.bossStageRect.w, layout_.bossStageRect.h };
		if (ImGui::DragFloat4("Rect##BossStage", rect, 1.0f)) {
			layout_.bossStageRect = { rect[0], rect[1], rect[2], rect[3] };
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Deck Edit Button")) {
		float rect[4] = { layout_.deckEditButtonRect.x, layout_.deckEditButtonRect.y, layout_.deckEditButtonRect.w, layout_.deckEditButtonRect.h };
		if (ImGui::DragFloat4("Rect##DeckEdit", rect, 1.0f)) {
			layout_.deckEditButtonRect = { rect[0], rect[1], rect[2], rect[3] };
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Left Arrow")) {
		float rect[4] = { layout_.leftArrowRect.x, layout_.leftArrowRect.y, layout_.leftArrowRect.w, layout_.leftArrowRect.h };
		if (ImGui::DragFloat4("Rect##LeftArrow", rect, 1.0f)) {
			layout_.leftArrowRect = { rect[0], rect[1], rect[2], rect[3] };
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Right Arrow")) {
		float rect[4] = { layout_.rightArrowRect.x, layout_.rightArrowRect.y, layout_.rightArrowRect.w, layout_.rightArrowRect.h };
		if (ImGui::DragFloat4("Rect##RightArrow", rect, 1.0f)) {
			layout_.rightArrowRect = { rect[0], rect[1], rect[2], rect[3] };
		}
		ImGui::TreePop();
	}
	ImGui::Separator();

	ImGui::Text("hoverIndex = %d", hoverIndex_);
	ImGui::Text("selectIndex = %d", selectIndex_);

	if (stageFieldProps_) {
		stageFieldProps_->DrawImGui("Stage Select Field Props", stageFieldConfigPath_);
	}

	ImGui::End();
#else
	(void)app;
#endif
}

void StageSelectScene::DrawSkydome(GameApp& app) {
	app.ObjCom()->SetGraphicsPipelineState();
	if (skyDome_) {
		skyDome_->Draw();
	}
}

void StageSelectScene::DrawPostEffect3D(GameApp& app) {
	if (stageFieldProps_) {
		stageFieldProps_->DrawPostEffect3D(app);
	}
}

void StageSelectScene::DrawPostEffect2D(GameApp& app) {
	if (!bossStageWarningOverlay_ || currentStageId_ != 10) {
		return;
	}

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
		0.0f, 0.0f,
		static_cast<float>(WinApp::kClientWidth),
		static_cast<float>(WinApp::kClientHeight),
		0.0f, 100.0f
	);

	const bool isBurst = bossWarningBurstTimer_ > 0.0f;
	const float overlayAlpha = isBurst ? 0.22f : 0.14f;
	bossStageWarningOverlay_->SetColor({ 1.0f, 0.04f, 0.02f, overlayAlpha });
	BloomParam warningParam = MakeBossStageWarningParam_(app.ObjectPost()->GetParam(), isBurst);
	app.DrawSpriteObjectPost(bossStageWarningOverlay_.get(), view, proj, warningParam);
}


void StageSelectScene::ApplyLayout_() {
	if (titleSprite_) {
		titleSprite_->SetPosition({ layout_.titlePos.x, layout_.titlePos.y });
	}

	if (descBgBottom_) {
		descBgBottom_->SetPosition({ 360.0f, 560.0f });
	}

	backButtonRect_ = { layout_.backButtonRect.x, layout_.backButtonRect.y, layout_.backButtonRect.w, layout_.backButtonRect.h };
	leftArrowRect_ = { layout_.leftArrowRect.x, layout_.leftArrowRect.y, layout_.leftArrowRect.w, layout_.leftArrowRect.h };
	rightArrowRect_ = { layout_.rightArrowRect.x, layout_.rightArrowRect.y, layout_.rightArrowRect.w, layout_.rightArrowRect.h };

	if (stageItems_.size() >= 2) {
		UiRect activeRect;
		if (currentStageId_ <= 5) {
			activeRect = layout_.stages1_5Rect;
		} else if (currentStageId_ <= 9) {
			activeRect = layout_.stages6_9Rect;
		} else {
			activeRect = layout_.bossStageRect;
		}

		stageItems_[0].buttonRect = {
			activeRect.x,
			activeRect.y,
			activeRect.w,
			activeRect.h
		};

		stageItems_[1].buttonRect = {
			layout_.deckEditButtonRect.x,
			layout_.deckEditButtonRect.y,
			layout_.deckEditButtonRect.w,
			layout_.deckEditButtonRect.h
		};

		const float descX = 420.0f;
		const float descY = 585.0f;
		stageItems_[0].descRect = { descX, descY, stageItems_[0].buttonRect.w, stageItems_[0].buttonRect.h };
		stageItems_[1].descRect = { descX, descY, layout_.deckEditButtonRect.w, layout_.deckEditButtonRect.h };

		for (auto& item : stageItems_) {
			if (item.buttonSprite) {
				item.buttonSprite->SetPosition({
					item.buttonRect.x,
					item.buttonRect.y
					});
			}
		}
	}
}

void StageSelectScene::SaveLayout_() const {
	nlohmann::json j;

	j["titlePos"] = {
		{"x", layout_.titlePos.x},
		{"y", layout_.titlePos.y}
	};

	j["tutorialButtonRect"] = {
		{"x", layout_.tutorialButtonRect.x},
		{"y", layout_.tutorialButtonRect.y},
		{"w", layout_.tutorialButtonRect.w},
		{"h", layout_.tutorialButtonRect.h}
	};

	j["battleTitleTextOffset"] = {
		{"x", layout_.battleTitleTextOffset.x},
		{"y", layout_.battleTitleTextOffset.y}
	};

	j["stages1_5Rect"] = {
		{"x", layout_.stages1_5Rect.x},
		{"y", layout_.stages1_5Rect.y},
		{"w", layout_.stages1_5Rect.w},
		{"h", layout_.stages1_5Rect.h}
	};

	j["stages6_9Rect"] = {
		{"x", layout_.stages6_9Rect.x},
		{"y", layout_.stages6_9Rect.y},
		{"w", layout_.stages6_9Rect.w},
		{"h", layout_.stages6_9Rect.h}
	};

	j["bossStageRect"] = {
		{"x", layout_.bossStageRect.x},
		{"y", layout_.bossStageRect.y},
		{"w", layout_.bossStageRect.w},
		{"h", layout_.bossStageRect.h}
	};

	j["deckEditButtonRect"] = {
		{"x", layout_.deckEditButtonRect.x},
		{"y", layout_.deckEditButtonRect.y},
		{"w", layout_.deckEditButtonRect.w},
		{"h", layout_.deckEditButtonRect.h}
	};

	j["backButtonRect"] = {
		{"x", layout_.backButtonRect.x},
		{"y", layout_.backButtonRect.y},
		{"w", layout_.backButtonRect.w},
		{"h", layout_.backButtonRect.h}
	};

	j["leftArrowRect"] = {
		{"x", layout_.leftArrowRect.x},
		{"y", layout_.leftArrowRect.y},
		{"w", layout_.leftArrowRect.w},
		{"h", layout_.leftArrowRect.h}
	};

	j["rightArrowRect"] = {
		{"x", layout_.rightArrowRect.x},
		{"y", layout_.rightArrowRect.y},
		{"w", layout_.rightArrowRect.w},
		{"h", layout_.rightArrowRect.h}
	};

	j["tutorialDescBgPos"] = {
		{"x", layout_.tutorialDescBgPos.x},
		{"y", layout_.tutorialDescBgPos.y}
	};

	j["battleDescBgPos"] = {
		{"x", layout_.battleDescBgPos.x},
		{"y", layout_.battleDescBgPos.y}
	};

	j["tutorialDescText"] = {
		{"x", layout_.tutorialDescText.x},
		{"y", layout_.tutorialDescText.y},
		{"scale", layout_.tutorialDescText.scale}
	};

	j["battleDescText"] = {
		{"x", layout_.battleDescText.x},
		{"y", layout_.battleDescText.y},
		{"scale", layout_.battleDescText.scale}
	};

	j["tutorialButtonVisualPos"] = {
	{"x", layout_.tutorialButtonVisualPos.x},
	{"y", layout_.tutorialButtonVisualPos.y}
	};

	j["battleButtonVisualPos"] = {
		{"x", layout_.battleButtonVisualPos.x},
		{"y", layout_.battleButtonVisualPos.y}
	};

	std::ofstream ofs(layoutPath_);
	if (ofs.is_open()) {
		ofs << j.dump(4);
	}
}

void StageSelectScene::LoadLayout_() {
	std::ifstream ifs(layoutPath_);
	if (!ifs.is_open()) {
		return;
	}

	nlohmann::json j;
	ifs >> j;

	if (j.contains("titlePos")) {
		layout_.titlePos.x = j["titlePos"].value("x", layout_.titlePos.x);
		layout_.titlePos.y = j["titlePos"].value("y", layout_.titlePos.y);
	}

	if (j.contains("tutorialButtonRect")) {
		layout_.tutorialButtonRect.x = j["tutorialButtonRect"].value("x", layout_.tutorialButtonRect.x);
		layout_.tutorialButtonRect.y = j["tutorialButtonRect"].value("y", layout_.tutorialButtonRect.y);
		layout_.tutorialButtonRect.w = j["tutorialButtonRect"].value("w", layout_.tutorialButtonRect.w);
		layout_.tutorialButtonRect.h = j["tutorialButtonRect"].value("h", layout_.tutorialButtonRect.h);
	}

	if (j.contains("battleTitleTextOffset")) {
		layout_.battleTitleTextOffset.x = j["battleTitleTextOffset"].value("x", layout_.battleTitleTextOffset.x);
		layout_.battleTitleTextOffset.y = j["battleTitleTextOffset"].value("y", layout_.battleTitleTextOffset.y);
	}

	if (j.contains("stages1_5Rect")) {
		layout_.stages1_5Rect.x = j["stages1_5Rect"].value("x", layout_.stages1_5Rect.x);
		layout_.stages1_5Rect.y = j["stages1_5Rect"].value("y", layout_.stages1_5Rect.y);
		layout_.stages1_5Rect.w = j["stages1_5Rect"].value("w", layout_.stages1_5Rect.w);
		layout_.stages1_5Rect.h = j["stages1_5Rect"].value("h", layout_.stages1_5Rect.h);
	}

	if (j.contains("stages6_9Rect")) {
		layout_.stages6_9Rect.x = j["stages6_9Rect"].value("x", layout_.stages6_9Rect.x);
		layout_.stages6_9Rect.y = j["stages6_9Rect"].value("y", layout_.stages6_9Rect.y);
		layout_.stages6_9Rect.w = j["stages6_9Rect"].value("w", layout_.stages6_9Rect.w);
		layout_.stages6_9Rect.h = j["stages6_9Rect"].value("h", layout_.stages6_9Rect.h);
	}

	if (j.contains("bossStageRect")) {
		layout_.bossStageRect.x = j["bossStageRect"].value("x", layout_.bossStageRect.x);
		layout_.bossStageRect.y = j["bossStageRect"].value("y", layout_.bossStageRect.y);
		layout_.bossStageRect.w = j["bossStageRect"].value("w", layout_.bossStageRect.w);
		layout_.bossStageRect.h = j["bossStageRect"].value("h", layout_.bossStageRect.h);
	}

	if (j.contains("deckEditButtonRect")) {
		layout_.deckEditButtonRect.x = j["deckEditButtonRect"].value("x", layout_.deckEditButtonRect.x);
		layout_.deckEditButtonRect.y = j["deckEditButtonRect"].value("y", layout_.deckEditButtonRect.y);
		layout_.deckEditButtonRect.w = j["deckEditButtonRect"].value("w", layout_.deckEditButtonRect.w);
		layout_.deckEditButtonRect.h = j["deckEditButtonRect"].value("h", layout_.deckEditButtonRect.h);
	}

	if (j.contains("backButtonRect")) {
		layout_.backButtonRect.x = j["backButtonRect"].value("x", layout_.backButtonRect.x);
		layout_.backButtonRect.y = j["backButtonRect"].value("y", layout_.backButtonRect.y);
		layout_.backButtonRect.w = j["backButtonRect"].value("w", layout_.backButtonRect.w);
		layout_.backButtonRect.h = j["backButtonRect"].value("h", layout_.backButtonRect.h);
	}

	if (j.contains("leftArrowRect")) {
		layout_.leftArrowRect.x = j["leftArrowRect"].value("x", layout_.leftArrowRect.x);
		layout_.leftArrowRect.y = j["leftArrowRect"].value("y", layout_.leftArrowRect.y);
		layout_.leftArrowRect.w = j["leftArrowRect"].value("w", layout_.leftArrowRect.w);
		layout_.leftArrowRect.h = j["leftArrowRect"].value("h", layout_.leftArrowRect.h);
	}

	if (j.contains("rightArrowRect")) {
		layout_.rightArrowRect.x = j["rightArrowRect"].value("x", layout_.rightArrowRect.x);
		layout_.rightArrowRect.y = j["rightArrowRect"].value("y", layout_.rightArrowRect.y);
		layout_.rightArrowRect.w = j["rightArrowRect"].value("w", layout_.rightArrowRect.w);
		layout_.rightArrowRect.h = j["rightArrowRect"].value("h", layout_.rightArrowRect.h);
	}

	if (j.contains("tutorialDescBgPos")) {
		layout_.tutorialDescBgPos.x = j["tutorialDescBgPos"].value("x", layout_.tutorialDescBgPos.x);
		layout_.tutorialDescBgPos.y = j["tutorialDescBgPos"].value("y", layout_.tutorialDescBgPos.y);
	}

	if (j.contains("battleDescBgPos")) {
		layout_.battleDescBgPos.x = j["battleDescBgPos"].value("x", layout_.battleDescBgPos.x);
		layout_.battleDescBgPos.y = j["battleDescBgPos"].value("y", layout_.battleDescBgPos.y);
	}

	if (j.contains("tutorialDescText")) {
		layout_.tutorialDescText.x = j["tutorialDescText"].value("x", layout_.tutorialDescText.x);
		layout_.tutorialDescText.y = j["tutorialDescText"].value("y", layout_.tutorialDescText.y);
		layout_.tutorialDescText.scale = j["tutorialDescText"].value("scale", layout_.tutorialDescText.scale);
	}

	if (j.contains("battleDescText")) {
		layout_.battleDescText.x = j["battleDescText"].value("x", layout_.battleDescText.x);
		layout_.battleDescText.y = j["battleDescText"].value("y", layout_.battleDescText.y);
		layout_.battleDescText.scale = j["battleDescText"].value("scale", layout_.battleDescText.scale);
	}

	if (j.contains("tutorialButtonVisualPos")) {
		layout_.tutorialButtonVisualPos.x = j["tutorialButtonVisualPos"].value("x", layout_.tutorialButtonVisualPos.x);
		layout_.tutorialButtonVisualPos.y = j["tutorialButtonVisualPos"].value("y", layout_.tutorialButtonVisualPos.y);
	}

	if (j.contains("battleButtonVisualPos")) {
		layout_.battleButtonVisualPos.x = j["battleButtonVisualPos"].value("x", layout_.battleButtonVisualPos.x);
		layout_.battleButtonVisualPos.y = j["battleButtonVisualPos"].value("y", layout_.battleButtonVisualPos.y);
	}
}
