#include "StageSelectScene.h"

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

#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>

namespace {
	std::string MakeStageConfigPath_(int stageId)
	{
		return "resources/stages/stage" + std::string(stageId < 10 ? "0" : "") + std::to_string(stageId) + ".json";
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
		RequestChangeScene_("Game");
		return;
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
	selectIndex_ = 1;
}

void StageSelectScene::ApplyCurrentStageToBattleItem_()
{
	if (stageItems_.size() <= 1) {
		return;
	}

	stageItems_[1].displayText = MakeStageLabel_(currentStageId_);
	stageItems_[1].descText = MakeStageDescription_(currentStageId_);
	stageItems_[1].stageId = currentStageId_;
	stageItems_[1].stageConfigPath = MakeStageConfigPath_(currentStageId_);
}

void StageSelectScene::OnEnter(GameApp& app) {
	hoverIndex_ = -1;
	selectIndex_ = 0;
	currentStageId_ = 1;
	bossWarningBurstTimer_ = 0.0f;
	bossShakeTimer_ = 0.0f;
	circle_ = 0.0f;
	softness_ = 0.6f;
	AudioManager::GetInstance()->PlayBGM("BGM_TitleSelect");

	camera_ = std::make_unique<Camera>();
	camera_->SetTranslate({ 0.0f, 0.0f, 0.0f });
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	app.ObjCom()->SetDefaultCamera(camera_.get());

	bg_ = std::make_unique<Sprite>();
	bg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/stage_select/bg.png");
	bg_->SetAnchorPoint({ 0.0f, 0.0f });
	bg_->SetPosition({ 0.0f, 0.0f });

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
	titleSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/stage_select/title_stage_select.png");
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
	stageItems_.resize(3);

	descTextSprite_ = std::make_unique<TextSprite>();
	descTextSprite_->Initialize(app.SpriteCom(), app.Dx());
	descTextSprite_->SetSize({ 1.0f, 1.0f, 1.0f });
	descTextSprite_->SetFontSize(28);
	descTextSprite_->SetAlpha(1.0f);

	stageItems_[0].sceneName = "Tutorial";
	stageItems_[0].displayText = L"チュートリアル";
	stageItems_[0].descText = L"ゲームの基本操作を確認します";
	stageItems_[0].buttonRect = {
	 layout_.tutorialButtonRect.x,
	 layout_.tutorialButtonRect.y,
	 layout_.tutorialButtonRect.w,
	 layout_.tutorialButtonRect.h
	};
	stageItems_[0].buttonSprite = std::make_unique<Sprite>();
	stageItems_[0].buttonSprite->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	stageItems_[0].buttonSprite->SetAnchorPoint({ 0.0f, 0.0f });
	stageItems_[0].buttonSprite->SetPosition({ stageItems_[0].buttonRect.x, stageItems_[0].buttonRect.y });

	stageItems_[1].sceneName = "Game";
	ApplyCurrentStageToBattleItem_();
	stageItems_[1].buttonRect = {
	layout_.battleButtonRect.x,
	layout_.battleButtonRect.y,
	layout_.battleButtonRect.w,
	layout_.battleButtonRect.h
	};
	stageItems_[1].buttonSprite = std::make_unique<Sprite>();
	stageItems_[1].buttonSprite->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	stageItems_[1].buttonSprite->SetAnchorPoint({ 0.0f, 0.0f });
	stageItems_[1].buttonSprite->SetPosition({ stageItems_[1].buttonRect.x, stageItems_[1].buttonRect.y });

	stageItems_[2].sceneName = "DeckEdit";
	stageItems_[2].displayText = L"デッキ編集";
	stageItems_[2].descText = L"使用するデッキを編成します";
	stageItems_[2].buttonRect = {
	layout_.deckEditButtonRect.x,
	layout_.deckEditButtonRect.y,
	layout_.deckEditButtonRect.w,
	layout_.deckEditButtonRect.h
	};// 仮の座標（後でImGui等で調整）
	stageItems_[2].buttonSprite = std::make_unique<Sprite>();
	stageItems_[2].buttonSprite->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	stageItems_[2].buttonSprite->SetAnchorPoint({ 0.0f, 0.0f });
	stageItems_[2].buttonSprite->SetPosition({ stageItems_[2].buttonRect.x, stageItems_[2].buttonRect.y });

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
	}

	leftArrowText_ = std::make_unique<TextSprite>();
	leftArrowText_->Initialize(app.SpriteCom(), app.Dx());
	leftArrowText_->SetText(L"<");
	leftArrowText_->SetFontSize(64);
	leftArrowText_->SetSize({ 1.0f, 1.0f, 1.0f });
	leftArrowText_->SetAlpha(1.0f);

	rightArrowText_ = std::make_unique<TextSprite>();
	rightArrowText_->Initialize(app.SpriteCom(), app.Dx());
	rightArrowText_->SetText(L">");
	rightArrowText_->SetFontSize(64);
	rightArrowText_->SetSize({ 1.0f, 1.0f, 1.0f });
	rightArrowText_->SetAlpha(1.0f);

	LoadLayout_();
	ApplyLayout_();

	currentDescText_ = stageItems_[0].descText;
	currentDescPos_ = { layout_.tutorialDescText.x, layout_.tutorialDescText.y };

	if (descTextSprite_) {
		descTextSprite_->SetText(currentDescText_);
		descTextSprite_->SetPosition(currentDescPos_);
	}
}

void StageSelectScene::OnExit(GameApp& app) {
	(void)app;
	rightArrowText_.reset();
	leftArrowText_.reset();
	itemTextSprites_.clear();
	stageItems_.clear();
	descBgBottom_.reset();
	descBgTop_.reset();
	titleSprite_.reset();
	bossStageWarningOverlay_.reset();
	bg_.reset();
	camera_.reset();
	descTextSprite_.reset();
}

void StageSelectScene::Update(GameApp& app, float dt) {
	ApplyLayout_();
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

	if (input->IsKeyTrigger(DIK_ESCAPE)) {
		AudioManager::GetInstance()->PlaySE("SE_Tap");
		RequestChangeScene_("Title");
		return;
	}

	if (input->IsKeyTrigger(DIK_A)) {
		RequestChangeScene_("BattleAnimeEditer");
		return;
	}

	hoverIndex_ = -1;

	POINT mouse = input->GetMousePosition();
	const float mx = static_cast<float>(mouse.x);
	const float my = static_cast<float>(mouse.y);

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
		selectIndex_ = hoverIndex_;

		if (input->IsMouseTrigger(0)) {
			AudioManager::GetInstance()->PlaySE("SE_Tap");
			SelectStageItem_(app, stageItems_[selectIndex_]);
			return;
		}
	}

	// キーボード操作
	if (input->IsKeyTrigger(DIK_UP) || input->IsKeyTrigger(DIK_W)) {
		selectIndex_--;
		if (selectIndex_ < 0) {
			selectIndex_ = static_cast<int>(stageItems_.size()) - 1;
		}
	}

	if (input->IsKeyTrigger(DIK_DOWN) || input->IsKeyTrigger(DIK_S)) {
		selectIndex_++;
		if (selectIndex_ >= static_cast<int>(stageItems_.size())) {
			selectIndex_ = 0;
		}
	}

	if (input->IsKeyTrigger(DIK_LEFT) || input->IsKeyTrigger(DIK_A)) {
		if (currentStageId_ > 1) {
			AudioManager::GetInstance()->PlaySE("SE_Tap");
			ChangeStage_(-1);
		}
	}

	if (input->IsKeyTrigger(DIK_RIGHT) || input->IsKeyTrigger(DIK_D)) {
		if (currentStageId_ < 10) {
			AudioManager::GetInstance()->PlaySE("SE_Tap");
			ChangeStage_(1);
		}
	}

	if (input->IsKeyTrigger(DIK_RETURN) || input->IsKeyTrigger(DIK_SPACE)) {
		AudioManager::GetInstance()->PlaySE("SE_Tap");
		SelectStageItem_(app, stageItems_[selectIndex_]);
		return;
	}

	const int showIndex = hoverIndex_ >= 0 ? hoverIndex_ : selectIndex_;
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
	const int showIndex = hoverIndex_ >= 0 ? hoverIndex_ : selectIndex_;
	if (showIndex == 0) {
		scale = layout_.tutorialDescText.scale;
	} else if (showIndex == 1) {
		scale = layout_.battleDescText.scale;
	} else if (showIndex == 2) {
		//scale = layout_.deckEditDescText.scale;
	}

	descTextSprite_->SetText(text);
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

	Vector2 shakeOffset{ 0.0f, 0.0f };
	if (currentStageId_ == 10 && bossShakeTimer_ > 0.0f && bossShakeDuration_ > 0.0f) {
		const float elapsed = bossShakeDuration_ - bossShakeTimer_;
		const float strength = bossShakeMagnitude_ * (bossShakeTimer_ / bossShakeDuration_);
		shakeOffset.x = static_cast<float>(std::sin(elapsed * 90.0f)) * strength;
		shakeOffset.y = static_cast<float>(std::cos(elapsed * 72.0f)) * strength * 0.55f;
	}


	if (bg_) {
		bg_->SetPosition({ shakeOffset.x, shakeOffset.y });
		bg_->Update(view, proj);
		bg_->Draw();
	}

	if (currentStageId_ == 10 && bossStageWarningOverlay_) {
		const bool isBurst = bossWarningBurstTimer_ > 0.0f;
		const float overlayAlpha = isBurst ? 0.22f : 0.14f;
		bossStageWarningOverlay_->SetPosition({ shakeOffset.x, shakeOffset.y });
		bossStageWarningOverlay_->SetColor({ 1.0f, 0.04f, 0.02f, overlayAlpha });
		BloomParam warningParam = MakeBossStageWarningParam_(app.ObjectPost()->GetParam(), isBurst);
		app.DrawSpriteObjectPost(bossStageWarningOverlay_.get(), view, proj, warningParam);
	}


	if (titleSprite_) {
		titleSprite_->SetPosition({ layout_.titlePos.x + shakeOffset.x, layout_.titlePos.y + shakeOffset.y });
		titleSprite_->Update(view, proj);
		titleSprite_->Draw();
	}

	for (int i = 0; i < static_cast<int>(stageItems_.size()); ++i) {
		if (!stageItems_[i].buttonSprite) {
			continue;
		}

		// 2. デッキ編集(index 2)にホバー中なら、バトル(index 1)を表示しない

		const bool isSelected = i == selectIndex_;
		const bool isHovered = i == hoverIndex_;

		float scale = 1.0f;
		if (isSelected || isHovered) {
			scale = hoverScale_;
		}

		stageItems_[i].buttonSprite->SetPosition({
			stageItems_[i].buttonRect.x + shakeOffset.x,
			stageItems_[i].buttonRect.y + shakeOffset.y
			});
		stageItems_[i].buttonSprite->SetScale({ stageItems_[i].buttonRect.w * scale, stageItems_[i].buttonRect.h * scale, 1.0f });
		stageItems_[i].buttonSprite->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		stageItems_[i].buttonSprite->Update(view, proj);
		stageItems_[i].buttonSprite->Draw();
	}

	for (int i = 0; i < static_cast<int>(itemTextSprites_.size()); ++i) {
		if (!itemTextSprites_[i]) {
			continue;
		}

		const Rect& r = stageItems_[i].buttonRect;
		const bool isSelected = i == selectIndex_;
		const bool isHovered = i == hoverIndex_;
		const bool isBossStage = stageItems_[i].stageId == 10;
		Vector3 textColor = { 1.0f, 1.0f, 1.0f };
		if (isBossStage) {
			textColor = { 1.0f, 0.78f, 0.2f };
		}
		if (isHovered && !isSelected) {
			textColor = { 0.68f, 0.9f, 1.0f };
		}
		if (isSelected) {
			textColor = { 1.0f, 0.92f, 0.45f };
		}

		itemTextSprites_[i]->SetText(stageItems_[i].displayText);
		itemTextSprites_[i]->SetColor(textColor);
		Vector2 textPos{ r.x + 72.0f, r.y + 42.0f };
		if (i == 0) {
			textPos = { r.x + 55.0f, r.y + 40.0f };
		} else if (i == 2) {
			textPos = { r.x + 82.0f, r.y + 40.0f };
		} else if (stageItems_[i].stageId > 0) {
			textPos = { r.x + 58.0f, r.y + 38.0f };
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
			const Rect& r = stageItems_[i].buttonRect;

			// 2. デッキ編集(index 2)にホバー中なら、バトル(index 1)を表示しない

			Vector4 color = { 0.0f, 0.0f, 0.0f, 0.45f };
			if (i == hoverIndex_) {
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
	ImGui::Text("Title");
	ImGui::DragFloat2("titlePos", &layout_.titlePos.x, 1.0f);

	ImGui::Separator();
	ImGui::Text("Tutorial");
	ImGui::DragFloat4("tutorialButtonRect", &layout_.tutorialButtonRect.x, 1.0f);
	ImGui::DragFloat2("tutorialDescBgPos", &layout_.tutorialDescBgPos.x, 1.0f);
	ImGui::DragFloat2("tutorialDescTextPos", &layout_.tutorialDescText.x, 1.0f);
	ImGui::DragFloat("tutorialDescTextScale", &layout_.tutorialDescText.scale, 0.01f, 0.1f, 5.0f);

	ImGui::Separator();
	ImGui::Text("Battle");
	ImGui::DragFloat4("battleButtonRect", &layout_.battleButtonRect.x, 1.0f);
	ImGui::DragFloat2("battleDescBgPos", &layout_.battleDescBgPos.x, 1.0f);
	ImGui::DragFloat2("battleDescTextPos", &layout_.battleDescText.x, 1.0f);
	ImGui::DragFloat("battleDescTextScale", &layout_.battleDescText.scale, 0.01f, 0.1f, 5.0f);

	ImGui::Separator();
	ImGui::Text("Tutorial Visual");
	ImGui::DragFloat2("tutorialButtonVisualPos", &layout_.tutorialButtonVisualPos.x, 1.0f);

	ImGui::Text("Battle Visual");
	ImGui::DragFloat2("battleButtonVisualPos", &layout_.battleButtonVisualPos.x, 1.0f);

	ImGui::Text("Tutorial Hit");
	ImGui::DragFloat4("tutorialButtonRect", &layout_.tutorialButtonRect.x, 1.0f);

	ImGui::Text("Battle Hit");
	ImGui::DragFloat4("battleButtonRect", &layout_.battleButtonRect.x, 1.0f);

	ImGui::Separator();
	if (ImGui::Button("Save StageSelectLayout")) {
		SaveLayout_();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load StageSelectLayout")) {
		LoadLayout_();
		ApplyLayout_();
	}

	ImGui::Text("hoverIndex = %d", hoverIndex_);
	ImGui::Text("selectIndex = %d", selectIndex_);

	ImGui::End();
#else
	(void)app;
#endif
}

void StageSelectScene::DrawSkydome(GameApp& app) {
	app.ObjCom()->SetGraphicsPipelineState();
}

void StageSelectScene::DrawPostEffect3D(GameApp& app) {
	(void)app;
}

void StageSelectScene::DrawPostEffect2D(GameApp& app) {
	(void)app;
}


void StageSelectScene::ApplyLayout_() {
	if (titleSprite_) {
		titleSprite_->SetPosition({ layout_.titlePos.x, layout_.titlePos.y });
	}

	if (descBgBottom_) {
		descBgBottom_->SetPosition({ 360.0f, 560.0f });
	}

	if (stageItems_.size() >= 3) {
		const float battleW = 420.0f;
		const float battleH = 120.0f;
		const float battleX = (static_cast<float>(WinApp::kClientWidth) - battleW) * 0.5f;
		const float battleY = 390.0f;
		const float deckGap = 40.0f;
		const float descX = 420.0f;
		const float descY = 585.0f;

		stageItems_[0].buttonRect = {
			layout_.tutorialButtonRect.x,
			layout_.tutorialButtonRect.y,
			layout_.tutorialButtonRect.w,
			layout_.tutorialButtonRect.h
		};

		stageItems_[1].buttonRect = {
			battleX,
			battleY,
			battleW,
			battleH
		};

		stageItems_[2].buttonRect = {
			layout_.tutorialButtonRect.x + layout_.tutorialButtonRect.w + deckGap,
			layout_.tutorialButtonRect.y,
			layout_.deckEditButtonRect.w,
			layout_.deckEditButtonRect.h
		};

		stageItems_[0].descRect = {
			descX,
			descY,
			layout_.tutorialButtonRect.w,
			layout_.tutorialButtonRect.h
		};

		stageItems_[1].descRect = {
			descX,
			descY,
			battleW,
			battleH
		};

		stageItems_[2].descRect = {
			descX,
			descY,
			layout_.deckEditButtonRect.w,
			layout_.deckEditButtonRect.h
		};

		if (stageItems_[0].buttonSprite) {
			stageItems_[0].buttonSprite->SetPosition({
				stageItems_[0].buttonRect.x,
				stageItems_[0].buttonRect.y
				});
		}

		if (stageItems_[1].buttonSprite) {
			stageItems_[1].buttonSprite->SetPosition({
				stageItems_[1].buttonRect.x,
				stageItems_[1].buttonRect.y
				});
		}
		if (stageItems_[2].buttonSprite) {
			stageItems_[2].buttonSprite->SetPosition({
				stageItems_[2].buttonRect.x,
				stageItems_[2].buttonRect.y
				});
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

	j["battleButtonRect"] = {
		{"x", layout_.battleButtonRect.x},
		{"y", layout_.battleButtonRect.y},
		{"w", layout_.battleButtonRect.w},
		{"h", layout_.battleButtonRect.h}
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

	if (j.contains("battleButtonRect")) {
		layout_.battleButtonRect.x = j["battleButtonRect"].value("x", layout_.battleButtonRect.x);
		layout_.battleButtonRect.y = j["battleButtonRect"].value("y", layout_.battleButtonRect.y);
		layout_.battleButtonRect.w = j["battleButtonRect"].value("w", layout_.battleButtonRect.w);
		layout_.battleButtonRect.h = j["battleButtonRect"].value("h", layout_.battleButtonRect.h);
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
