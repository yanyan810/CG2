#include "SelectScene.h"

#include "AudioManager.h"
#include "GameApp.h"
#include "Input.h"
#include "Matrix4x4.h"
#include "WinApp.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

bool SelectScene::PointInRect_(float mx, float my, const Rect& rect) const
{
	return mx >= rect.x &&
		mx <= rect.x + rect.w &&
		my >= rect.y &&
		my <= rect.y + rect.h;
}

void SelectScene::SelectCurrent_(GameApp& app)
{
	if (selectIndex_ < 0 || selectIndex_ >= static_cast<int>(menuItems_.size())) {
		return;
	}

	AudioManager::GetInstance()->PlaySE("SE_Tap");
	if (menuItems_[selectIndex_].sceneName == "StageSelect") {
		app.SetLoadingMode(GameApp::LoadingMode::SelectToStageSelect);
		RequestChangeScene_("GameLoading");
		return;
	}

	RequestChangeScene_(menuItems_[selectIndex_].sceneName.c_str());
}

void SelectScene::OnEnter(GameApp& app)
{
	hoverIndex_ = -1;
	selectIndex_ = 0;

	AudioManager::GetInstance()->PlayBGM("BGM_TitleSelect");

	bg_ = std::make_unique<Sprite>();
	bg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	bg_->SetAnchorPoint({ 0.0f, 0.0f });
	bg_->SetPosition({ 0.0f, 0.0f });
	bg_->SetScale({ static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight), 1.0f });
	bg_->SetColor({ 0.78f, 0.78f, 0.78f, 1.0f });

	titleBoxBorder_ = std::make_unique<Sprite>();
	titleBoxBorder_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	titleBoxBorder_->SetAnchorPoint({ 0.0f, 0.0f });

	titleBoxBg_ = std::make_unique<Sprite>();
	titleBoxBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	titleBoxBg_->SetAnchorPoint({ 0.0f, 0.0f });

	titleText_ = std::make_unique<TextSprite>();
	titleText_->Initialize(app.SpriteCom(), app.Dx());
	titleText_->SetFontSize(30);
	titleText_->SetSize({ 1.0f, 1.0f, 1.0f });
	titleText_->SetAlpha(1.0f);
	titleText_->SetColor({ 1.0f, 1.0f, 1.0f });
	titleText_->SetOutline(selectTextOutlineColor_, selectTextOutlineWidth_);

	backButtonBorder_ = std::make_unique<Sprite>();
	backButtonBorder_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	backButtonBorder_->SetAnchorPoint({ 0.0f, 0.0f });

	backButtonBg_ = std::make_unique<Sprite>();
	backButtonBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	backButtonBg_->SetAnchorPoint({ 0.0f, 0.0f });

	backButtonText_ = std::make_unique<TextSprite>();
	backButtonText_->Initialize(app.SpriteCom(), app.Dx());
	backButtonText_->SetFontSize(30);
	backButtonText_->SetSize({ 1.0f, 1.0f, 1.0f });
	backButtonText_->SetAlpha(1.0f);
	backButtonText_->SetColor({ 1.0f, 1.0f, 1.0f });
	backButtonText_->SetOutline(selectTextOutlineColor_, selectTextOutlineWidth_);

	descText_ = std::make_unique<TextSprite>();
	descText_->Initialize(app.SpriteCom(), app.Dx());
	descText_->SetFontSize(24);
	descText_->SetSize({ 1.0f, 1.0f, 1.0f });
	descText_->SetAlpha(1.0f);

	menuItems_[0].sceneName = "Tutorial";
	menuItems_[0].label = L"チュートリアル";
	menuItems_[0].description = L"基本操作を確認します";
	menuItems_[0].rect = { 160.0f, 440.0f, 300.0f, 90.0f };

	menuItems_[1].sceneName = "StageSelect";
	menuItems_[1].label = L"ステージセレクト";
	menuItems_[1].description = L"挑戦するステージを選びます";
	menuItems_[1].rect = { 490.0f, 440.0f, 300.0f, 90.0f };

	menuItems_[2].sceneName = "DeckEdit";
	menuItems_[2].label = L"デッキ編集";
	menuItems_[2].description = L"使用するデッキを編集します";
	menuItems_[2].rect = { 820.0f, 440.0f, 300.0f, 90.0f };

	for (auto& item : menuItems_) {
		item.border = std::make_unique<Sprite>();
		item.border->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
		item.border->SetAnchorPoint({ 0.0f, 0.0f });

		item.bg = std::make_unique<Sprite>();
		item.bg->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
		item.bg->SetAnchorPoint({ 0.0f, 0.0f });

		item.text = std::make_unique<TextSprite>();
		item.text->Initialize(app.SpriteCom(), app.Dx());
		item.text->SetFontSize(28);
		item.text->SetSize({ 1.0f, 1.0f, 1.0f });
		item.text->SetAlpha(1.0f);
		item.text->SetColor({ 1.0f, 1.0f, 1.0f });
		item.text->SetOutline(selectTextOutlineColor_, selectTextOutlineWidth_);
	}
}

void SelectScene::OnExit(GameApp& app)
{
	(void)app;
	for (auto& item : menuItems_) {
		item.text.reset();
		item.bg.reset();
		item.border.reset();
	}
	descText_.reset();
	titleText_.reset();
	backButtonText_.reset();
	backButtonBg_.reset();
	backButtonBorder_.reset();
	titleBoxBg_.reset();
	titleBoxBorder_.reset();
	bg_.reset();
}

void SelectScene::Update(GameApp& app, float dt)
{
	(void)dt;
	Input* input = app.GetInput();
	if (!input) {
		return;
	}

	hoverIndex_ = -1;

	POINT mouse = input->GetMousePosition();
	const float mx = static_cast<float>(mouse.x);
	const float my = static_cast<float>(mouse.y);

	if (input->IsKeyTrigger(DIK_ESCAPE) ||
		(PointInRect_(mx, my, backButtonRect_) && input->IsMouseTrigger(0))) {
		AudioManager::GetInstance()->PlaySE("SE_Tap");
		RequestChangeScene_("Title");
		return;
	}

	for (int i = 0; i < static_cast<int>(menuItems_.size()); ++i) {
		if (PointInRect_(mx, my, menuItems_[i].rect)) {
			hoverIndex_ = i;
			break;
		}
	}

	if (hoverIndex_ >= 0) {
		selectIndex_ = hoverIndex_;
		if (input->IsMouseTrigger(0)) {
			SelectCurrent_(app);
			return;
		}
	}

	if (input->IsKeyTrigger(DIK_LEFT) || input->IsKeyTrigger(DIK_UP) ||
		input->IsKeyTrigger(DIK_A) || input->IsKeyTrigger(DIK_W)) {
		--selectIndex_;
		if (selectIndex_ < 0) {
			selectIndex_ = static_cast<int>(menuItems_.size()) - 1;
		}
		AudioManager::GetInstance()->PlaySE("SE_Tap");
	}

	if (input->IsKeyTrigger(DIK_RIGHT) || input->IsKeyTrigger(DIK_DOWN) ||
		input->IsKeyTrigger(DIK_D) || input->IsKeyTrigger(DIK_S)) {
		++selectIndex_;
		if (selectIndex_ >= static_cast<int>(menuItems_.size())) {
			selectIndex_ = 0;
		}
		AudioManager::GetInstance()->PlaySE("SE_Tap");
	}

	if (input->IsKeyTrigger(DIK_RETURN) || input->IsKeyTrigger(DIK_SPACE)) {
		SelectCurrent_(app);
		return;
	}
}

void SelectScene::Draw2D(GameApp& app)
{
	app.SpriteCom()->SetGraphicsPipelineState();

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
		0.0f, 0.0f,
		static_cast<float>(WinApp::kClientWidth),
		static_cast<float>(WinApp::kClientHeight),
		0.0f, 100.0f
	);

	constexpr float kBorder = 3.0f;
	const Vector4 normalPanelColor{ 0.60f, 0.60f, 0.60f, 1.0f };
	const Vector4 activePanelColor{ 0.70f, 0.70f, 0.70f, 1.0f };
	const Vector4 borderColor{ 0.0f, 0.0f, 0.0f, 1.0f };

	if (titleText_) {
		titleText_->SetOutline(selectTextOutlineColor_, selectTextOutlineWidth_);
	}
	if (backButtonText_) {
		backButtonText_->SetOutline(selectTextOutlineColor_, selectTextOutlineWidth_);
	}
	for (auto& item : menuItems_) {
		if (item.text) {
			item.text->SetOutline(selectTextOutlineColor_, selectTextOutlineWidth_);
		}
	}

	if (bg_) {
		bg_->Update(view, proj);
		bg_->Draw();
	}

	if (titleBoxBorder_) {
		titleBoxBorder_->SetPosition({ titleBoxRect_.x - kBorder, titleBoxRect_.y - kBorder });
		titleBoxBorder_->SetScale({ titleBoxRect_.w + kBorder * 2.0f, titleBoxRect_.h + kBorder * 2.0f, 1.0f });
		titleBoxBorder_->SetColor(borderColor);
		titleBoxBorder_->Update(view, proj);
		titleBoxBorder_->Draw();
	}

	if (titleBoxBg_) {
		titleBoxBg_->SetPosition({ titleBoxRect_.x, titleBoxRect_.y });
		titleBoxBg_->SetScale({ titleBoxRect_.w, titleBoxRect_.h, 1.0f });
		titleBoxBg_->SetColor(normalPanelColor);
		titleBoxBg_->Update(view, proj);
		titleBoxBg_->Draw();
	}

	if (titleText_) {
		titleText_->SetText(L"セレクト");
		titleText_->SetPosition({ titleBoxRect_.x + 110.0f, titleBoxRect_.y + 28.0f });
		titleText_->Update(view, proj);
		titleText_->Draw();
	}

	Input* input = app.GetInput();
	bool isBackHovered = false;
	if (input) {
		const POINT mouse = input->GetMousePosition();
		isBackHovered = PointInRect_(static_cast<float>(mouse.x), static_cast<float>(mouse.y), backButtonRect_);
	}

	if (backButtonBorder_) {
		backButtonBorder_->SetPosition({ backButtonRect_.x - kBorder, backButtonRect_.y - kBorder });
		backButtonBorder_->SetScale({ backButtonRect_.w + kBorder * 2.0f, backButtonRect_.h + kBorder * 2.0f, 1.0f });
		backButtonBorder_->SetColor(borderColor);
		backButtonBorder_->Update(view, proj);
		backButtonBorder_->Draw();
	}

	if (backButtonBg_) {
		backButtonBg_->SetPosition({ backButtonRect_.x, backButtonRect_.y });
		backButtonBg_->SetScale({ backButtonRect_.w, backButtonRect_.h, 1.0f });
		backButtonBg_->SetColor(isBackHovered ? activePanelColor : normalPanelColor);
		backButtonBg_->Update(view, proj);
		backButtonBg_->Draw();
	}

	if (backButtonText_) {
		backButtonText_->SetText(L"戻る");
		backButtonText_->SetPosition({ backButtonRect_.x + 62.0f, backButtonRect_.y + 22.0f });
		backButtonText_->Update(view, proj);
		backButtonText_->Draw();
	}

	for (int i = 0; i < static_cast<int>(menuItems_.size()); ++i) {
		auto& item = menuItems_[i];
		const bool isSelected = i == selectIndex_;
		const bool isHovered = i == hoverIndex_;
		const float scale = (isSelected || isHovered) ? 1.03f : 1.0f;
		const float scaledW = item.rect.w * scale;
		const float scaledH = item.rect.h * scale;
		const float offsetX = (scaledW - item.rect.w) * 0.5f;
		const float offsetY = (scaledH - item.rect.h) * 0.5f;
		const float x = item.rect.x - offsetX;
		const float y = item.rect.y - offsetY;

		if (item.border) {
			item.border->SetPosition({ x - kBorder, y - kBorder });
			item.border->SetScale({ scaledW + kBorder * 2.0f, scaledH + kBorder * 2.0f, 1.0f });
			item.border->SetColor(borderColor);
			item.border->Update(view, proj);
			item.border->Draw();
		}

		if (item.bg) {
			item.bg->SetPosition({ x, y });
			item.bg->SetScale({ scaledW, scaledH, 1.0f });
			item.bg->SetColor(isSelected ? activePanelColor : normalPanelColor);
			item.bg->Update(view, proj);
			item.bg->Draw();
		}

		Vector2 textPos{ item.rect.x + 77.0f, item.rect.y + 31.0f };
		if (i == 1) {
			textPos = { item.rect.x + 58.0f, item.rect.y + 31.0f };
		} else if (i == 2) {
			textPos = { item.rect.x + 91.0f, item.rect.y + 31.0f };
		}
		if (item.text) {
			item.text->SetText(item.label);
			item.text->SetPosition(textPos);
			item.text->Update(view, proj);
			item.text->Draw();
		}
	}
}

void SelectScene::DrawImGui(GameApp& app)
{
#ifdef USE_IMGUI
	(void)app;
	ImGui::Begin("SelectScene");
	ImGui::Text("hoverIndex = %d", hoverIndex_);
	ImGui::Text("selectIndex = %d", selectIndex_);
	float outlineColor[3] = {
		selectTextOutlineColor_.x,
		selectTextOutlineColor_.y,
		selectTextOutlineColor_.z,
	};
	if (ImGui::ColorEdit3("Text Outline Color", outlineColor)) {
		selectTextOutlineColor_ = { outlineColor[0], outlineColor[1], outlineColor[2] };
	}
	ImGui::SliderFloat("Text Outline Width", &selectTextOutlineWidth_, 0.0f, 10.0f);
	ImGui::End();
#else
	(void)app;
#endif
}
