#include "SelectScene.h"
#include "DeckEditScene.h"

#include "AudioManager.h"
#include "GameApp.h"
#include "Input.h"
#include "Matrix4x4.h"
#include "WinApp.h"
#include "TextureManager.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include <fstream>
#include <nlohmann/json.hpp>

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

	if (menuItems_[selectIndex_].sceneName == "DeckEdit") {
		DeckEditScene::returnSceneName_ = "Select";
	}

	RequestChangeScene_(menuItems_[selectIndex_].sceneName.c_str());
}

void SelectScene::OnEnter(GameApp& app)
{
	LoadLayout_();
	ApplyLayout_();

	bg_ = std::make_unique<Sprite>();
	selectIndex_ = 0;
	hoverIndex_ = -1;

	AudioManager::GetInstance()->PlayBGM("BGM_TitleSelect");

	TextureManager::GetInstance()->LoadTexture("resources/ui/text/Select_scene.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/return.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/Tutorial.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/Stage_select.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/Deck_editing.png");

	bg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	bg_->SetAnchorPoint({ 0.0f, 0.0f });
	bg_->SetPosition({ 0.0f, 0.0f });
	bg_->SetScale({ static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight), 1.0f });
	bg_->SetColor({ 0.78f, 0.78f, 0.78f, 1.0f });

	titleBoxBg_ = std::make_unique<Sprite>();
	titleBoxBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/Select_scene.png");
	titleBoxBg_->SetAnchorPoint({ 0.0f, 0.0f });

	backButtonBg_ = std::make_unique<Sprite>();
	backButtonBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/return.png");
	backButtonBg_->SetAnchorPoint({ 0.0f, 0.0f });

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

	menuItems_[0].bg = std::make_unique<Sprite>();
	menuItems_[0].bg->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/Tutorial.png");
	menuItems_[0].bg->SetAnchorPoint({ 0.0f, 0.0f });

	menuItems_[1].bg = std::make_unique<Sprite>();
	menuItems_[1].bg->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/Stage_select.png");
	menuItems_[1].bg->SetAnchorPoint({ 0.0f, 0.0f });

	menuItems_[2].bg = std::make_unique<Sprite>();
	menuItems_[2].bg->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/Deck_editing.png");
	menuItems_[2].bg->SetAnchorPoint({ 0.0f, 0.0f });
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
	ApplyLayout_();

	(void)dt;
	Input* input = app.GetInput();
	if (!input) {
		return;
	}

	hoverIndex_ = -1;

	POINT mouse = input->GetMousePosition();
	const float mx = static_cast<float>(mouse.x);
	const float my = static_cast<float>(mouse.y);

	POINT mouseDelta = input->GetMouseDelta();
	if (mouseDelta.x != 0 || mouseDelta.y != 0 || input->IsMouseTrigger(0)) {
		isUsingMouse_ = true;
	}

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
		if (input->IsMouseTrigger(0)) {
			selectIndex_ = hoverIndex_;
			SelectCurrent_(app);
			return;
		}
	}

	if (input->IsKeyTrigger(DIK_LEFT) || input->IsKeyTrigger(DIK_UP) ||
		input->IsKeyTrigger(DIK_A) || input->IsKeyTrigger(DIK_W)) {
		isUsingMouse_ = false;
		--selectIndex_;
		if (selectIndex_ < 0) {
			selectIndex_ = static_cast<int>(menuItems_.size()) - 1;
		}
		AudioManager::GetInstance()->PlaySE("SE_Tap");
	}

	if (input->IsKeyTrigger(DIK_RIGHT) || input->IsKeyTrigger(DIK_DOWN) ||
		input->IsKeyTrigger(DIK_D) || input->IsKeyTrigger(DIK_S)) {
		isUsingMouse_ = false;
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
	const Vector4 normalPanelColor{ 0.8f, 0.8f, 0.8f, 1.0f };
	const Vector4 activePanelColor{ 1.0f, 1.0f, 1.0f, 1.0f };
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

	if (titleBoxBg_) {
		const DirectX::TexMetadata& meta = TextureManager::GetInstance()->GetMetaData(titleBoxBg_->GetTextureFilePath());
		titleBoxBg_->SetPosition({ titleBoxRect_.x, titleBoxRect_.y });
		titleBoxBg_->SetScale({ titleBoxRect_.w / static_cast<float>(meta.width), titleBoxRect_.h / static_cast<float>(meta.height), 1.0f });
		titleBoxBg_->SetColor(normalPanelColor);
		titleBoxBg_->Update(view, proj);
		titleBoxBg_->Draw();
	}

	Input* input = app.GetInput();
	bool isBackHovered = false;
	if (input) {
		const POINT mouse = input->GetMousePosition();
		isBackHovered = PointInRect_(static_cast<float>(mouse.x), static_cast<float>(mouse.y), backButtonRect_);
	}

	if (backButtonBg_) {
		const DirectX::TexMetadata& meta = TextureManager::GetInstance()->GetMetaData(backButtonBg_->GetTextureFilePath());
		backButtonBg_->SetPosition({ backButtonRect_.x, backButtonRect_.y });
		backButtonBg_->SetScale({ backButtonRect_.w / static_cast<float>(meta.width), backButtonRect_.h / static_cast<float>(meta.height), 1.0f });
		backButtonBg_->SetColor(isBackHovered ? activePanelColor : normalPanelColor);
		backButtonBg_->Update(view, proj);
		backButtonBg_->Draw();
	}

	for (int i = 0; i < static_cast<int>(menuItems_.size()); ++i) {
		auto& item = menuItems_[i];
		const bool isHovered = (i == hoverIndex_);
		const bool isSelected = (i == selectIndex_);
		const bool shouldHighlight = isUsingMouse_ ? isHovered : isSelected;
		
		const float scale = shouldHighlight ? 1.03f : 1.0f;
		const float scaledW = item.rect.w * scale;
		const float scaledH = item.rect.h * scale;
		const float offsetX = (scaledW - item.rect.w) * 0.5f;
		const float offsetY = (scaledH - item.rect.h) * 0.5f;
		const float x = item.rect.x - offsetX;
		const float y = item.rect.y - offsetY;

		if (item.bg) {
			const DirectX::TexMetadata& meta = TextureManager::GetInstance()->GetMetaData(item.bg->GetTextureFilePath());
			item.bg->SetPosition({ x, y });
			item.bg->SetScale({ scaledW / static_cast<float>(meta.width), scaledH / static_cast<float>(meta.height), 1.0f });
			item.bg->SetColor(shouldHighlight ? activePanelColor : normalPanelColor);
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
	ImGui::Begin("SelectSceneLayout");
	
	if (ImGui::Button("Save SelectSceneLayout")) {
		SaveLayout_();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load SelectSceneLayout")) {
		LoadLayout_();
		ApplyLayout_();
	}

	ImGui::Separator();
	if (ImGui::TreeNode("Title Box")) {
		float rect[4] = { layout_.titleBoxRect.x, layout_.titleBoxRect.y, layout_.titleBoxRect.w, layout_.titleBoxRect.h };
		if (ImGui::DragFloat4("Rect##Title", rect, 1.0f)) {
			layout_.titleBoxRect = { rect[0], rect[1], rect[2], rect[3] };
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
	if (ImGui::TreeNode("Tutorial Button")) {
		float rect[4] = { layout_.tutorialButtonRect.x, layout_.tutorialButtonRect.y, layout_.tutorialButtonRect.w, layout_.tutorialButtonRect.h };
		if (ImGui::DragFloat4("Rect##Tutorial", rect, 1.0f)) {
			layout_.tutorialButtonRect = { rect[0], rect[1], rect[2], rect[3] };
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Battle Button")) {
		float rect[4] = { layout_.battleButtonRect.x, layout_.battleButtonRect.y, layout_.battleButtonRect.w, layout_.battleButtonRect.h };
		if (ImGui::DragFloat4("Rect##Battle", rect, 1.0f)) {
			layout_.battleButtonRect = { rect[0], rect[1], rect[2], rect[3] };
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
	ImGui::Separator();

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

void SelectScene::ApplyLayout_() {
	titleBoxRect_ = { layout_.titleBoxRect.x, layout_.titleBoxRect.y, layout_.titleBoxRect.w, layout_.titleBoxRect.h };
	backButtonRect_ = { layout_.backButtonRect.x, layout_.backButtonRect.y, layout_.backButtonRect.w, layout_.backButtonRect.h };
	
	if (menuItems_.size() >= 3) {
		menuItems_[0].rect = { layout_.tutorialButtonRect.x, layout_.tutorialButtonRect.y, layout_.tutorialButtonRect.w, layout_.tutorialButtonRect.h };
		menuItems_[1].rect = { layout_.battleButtonRect.x, layout_.battleButtonRect.y, layout_.battleButtonRect.w, layout_.battleButtonRect.h };
		menuItems_[2].rect = { layout_.deckEditButtonRect.x, layout_.deckEditButtonRect.y, layout_.deckEditButtonRect.w, layout_.deckEditButtonRect.h };
	}
}

void SelectScene::SaveLayout_() const {
	nlohmann::json j;

	j["titleBoxRect"] = { {"x", layout_.titleBoxRect.x}, {"y", layout_.titleBoxRect.y}, {"w", layout_.titleBoxRect.w}, {"h", layout_.titleBoxRect.h} };
	j["backButtonRect"] = { {"x", layout_.backButtonRect.x}, {"y", layout_.backButtonRect.y}, {"w", layout_.backButtonRect.w}, {"h", layout_.backButtonRect.h} };
	j["tutorialButtonRect"] = { {"x", layout_.tutorialButtonRect.x}, {"y", layout_.tutorialButtonRect.y}, {"w", layout_.tutorialButtonRect.w}, {"h", layout_.tutorialButtonRect.h} };
	j["battleButtonRect"] = { {"x", layout_.battleButtonRect.x}, {"y", layout_.battleButtonRect.y}, {"w", layout_.battleButtonRect.w}, {"h", layout_.battleButtonRect.h} };
	j["deckEditButtonRect"] = { {"x", layout_.deckEditButtonRect.x}, {"y", layout_.deckEditButtonRect.y}, {"w", layout_.deckEditButtonRect.w}, {"h", layout_.deckEditButtonRect.h} };

	std::ofstream ofs(layoutPath_);
	if (ofs.is_open()) {
		ofs << j.dump(4);
	}
}

void SelectScene::LoadLayout_() {
	std::ifstream ifs(layoutPath_);
	if (!ifs.is_open()) {
		return;
	}

	nlohmann::json j;
	ifs >> j;

	if (j.contains("titleBoxRect")) {
		layout_.titleBoxRect.x = j["titleBoxRect"].value("x", layout_.titleBoxRect.x);
		layout_.titleBoxRect.y = j["titleBoxRect"].value("y", layout_.titleBoxRect.y);
		layout_.titleBoxRect.w = j["titleBoxRect"].value("w", layout_.titleBoxRect.w);
		layout_.titleBoxRect.h = j["titleBoxRect"].value("h", layout_.titleBoxRect.h);
	}
	if (j.contains("backButtonRect")) {
		layout_.backButtonRect.x = j["backButtonRect"].value("x", layout_.backButtonRect.x);
		layout_.backButtonRect.y = j["backButtonRect"].value("y", layout_.backButtonRect.y);
		layout_.backButtonRect.w = j["backButtonRect"].value("w", layout_.backButtonRect.w);
		layout_.backButtonRect.h = j["backButtonRect"].value("h", layout_.backButtonRect.h);
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
	if (j.contains("deckEditButtonRect")) {
		layout_.deckEditButtonRect.x = j["deckEditButtonRect"].value("x", layout_.deckEditButtonRect.x);
		layout_.deckEditButtonRect.y = j["deckEditButtonRect"].value("y", layout_.deckEditButtonRect.y);
		layout_.deckEditButtonRect.w = j["deckEditButtonRect"].value("w", layout_.deckEditButtonRect.w);
		layout_.deckEditButtonRect.h = j["deckEditButtonRect"].value("h", layout_.deckEditButtonRect.h);
	}
}
