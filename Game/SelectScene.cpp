#include "SelectScene.h"
#include "DeckEditScene.h"
#include "CardInstance.h"

#include "AudioManager.h"
#include "GameApp.h"
#include "Input.h"
#include "Matrix4x4.h"
#include "WinApp.h"
#include "TextureManager.h"
#include "ModelManager.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include <fstream>
#include <nlohmann/json.hpp>
#include <cmath>

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

	if (menuItems_[selectIndex_].sceneName == "Tutorial") {
		app.SetLoadingMode(GameApp::LoadingMode::SelectToTutorial);
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

	camera_ = std::make_unique<Camera>();
	camera_->SetTranslate({ 0.0f, 2.0f, -10.0f });
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->Update();
	app.ObjCom()->SetDefaultCamera(camera_.get());

	tutorialModel_ = std::make_unique<Object3d>();
	tutorialModel_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());
	tutorialModel_->SetModel("SelectScene/tutorial.obj");
	tutorialModel_->SetCamera(camera_.get());
	tutorialModel_->SetEnableLighting(0);

	stageSelectModel_ = std::make_unique<Object3d>();
	stageSelectModel_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());
	stageSelectModel_->SetModel("SelectScene/stageSelect_Player.obj");
	stageSelectModel_->SetCamera(camera_.get());
	stageSelectModel_->SetEnableLighting(0);

	deckEditModel_ = std::make_unique<Object3d>();
	deckEditModel_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());
	deckEditModel_->SetModel("SelectScene/deck_Question_Player.obj");
	deckEditModel_->SetCamera(camera_.get());
	deckEditModel_->SetEnableLighting(0);

	tutorialFieldModel_ = std::make_unique<Object3d>();
	tutorialFieldModel_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());
	tutorialFieldModel_->SetModel("Field/TutolialField/tutorial.obj");
	tutorialFieldModel_->SetCamera(camera_.get());
	tutorialFieldModel_->SetEnableLighting(0);

	stageSelectGroundModel_ = std::make_unique<Object3d>();
	stageSelectGroundModel_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());
	stageSelectGroundModel_->SetModel("Field/ForestField/glassField.obj");
	stageSelectGroundModel_->SetCamera(camera_.get());
	stageSelectGroundModel_->SetEnableLighting(0);

	backgroundCards_.clear();
	auto cardDB = app.GetCardDB();
	if (cardDB) {
		int cardCount = cardDB->GetCardCount();
		for (int i = 1; i <= cardCount; ++i) {
			const CardDef* def = cardDB->Find(i);
			if (!def) continue;

			auto card = std::make_unique<Card3D>();
			card->Setup(app.ObjCom(), app.Dx(), camera_.get());

			CardInstance inst;
			inst.defId = def->id;
			inst.number = 1;
			inst.suit = CardSuit::Spade;

			card->SetIsPreview(true);
			card->SetCardData(*def, inst);
			card->SetIsHand(false);
			card->SetFrameColor({ bgCardColor_.x, bgCardColor_.y, bgCardColor_.z, 1.0f }); 
			card->SetTransform(bgCardBasePos_, bgCardRot_, bgCardScale_);

			backgroundCards_.push_back(std::move(card));
		}

		fullHouseCards_.clear();
		int fhIds[5] = { 1, 1, 1, 2, 2 }; 
		for (int i = 0; i < 5; ++i) {
			const CardDef* def = cardDB->Find(fhIds[i]);
			if (!def) def = cardDB->Find(1);
			if (def) {
				auto card = std::make_unique<Card3D>();
				card->Setup(app.ObjCom(), app.Dx(), camera_.get());
				CardInstance inst;
				inst.defId = def->id;
				inst.number = 1;
				inst.suit = CardSuit::Spade;
				card->SetIsPreview(true);
				card->SetCardData(*def, inst);
				card->SetIsHand(false);
				card->SetFrameColor({ fhCardColor_.x, fhCardColor_.y, fhCardColor_.z, 1.0f });
				card->SetTransform(fhCardBasePos_, fhCardRot_, fhCardScale_);
				fullHouseCards_.push_back(std::move(card));
			}
		}
	}

	baseFieldModel_ = std::make_unique<Object3d>();
	baseFieldModel_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());
	baseFieldModel_->SetModel("Field/field.obj");
	baseFieldModel_->SetCamera(camera_.get());
	baseFieldModel_->SetEnableLighting(0);

	buttonBgSprite_ = std::make_unique<Sprite>();
	buttonBgSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");

	bg_ = std::make_unique<Sprite>();
	selectIndex_ = 0;
	hoverIndex_ = -1;

	skyDome_ = std::make_unique<Object3d>();
	skyDome_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());
	skyDome_->SetModel("skydome/skydome.obj");
	skyDome_->SetCamera(camera_.get());
	skyDome_->SetEnableLighting(0);
	skyDome_->SetTranslate({ 0.0f, 0.0f, 0.0f });
	skyDome_->SetScale({ 100.0f, 100.0f, 100.0f });

	AudioManager::GetInstance()->PlayBGM("BGM_TitleSelect");

	TextureManager::GetInstance()->LoadTexture("resources/ui/text/return.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/Tutorial.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/Stage_select.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/Deck_editing.png");

	bg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	bg_->SetAnchorPoint({ 0.0f, 0.0f });
	bg_->SetPosition({ 0.0f, 0.0f });
	bg_->SetScale({ static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight), 1.0f });
	bg_->SetColor({ 0.78f, 0.78f, 0.78f, 1.0f });

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
	camera_.reset();
	skyDome_.reset();
}

void SelectScene::Update(GameApp& app, float dt)
{
	ApplyLayout_();

	if (camera_) {
		camera_->Update();
	}

	if (skyDome_) {
		skyDome_->Update(dt);
	}

	if (tutorialModel_) {
		ApplyLighting_(tutorialModel_.get());
		tutorialModel_->SetTranslate(tutorialTransform_.position);
		tutorialModel_->SetRotate(tutorialTransform_.rotation);
		tutorialModel_->SetScale(tutorialTransform_.scale);
		tutorialModel_->Update(dt);
	}
	if (stageSelectModel_) {
		ApplyLighting_(stageSelectModel_.get());
		stageSelectModel_->SetTranslate(stageSelectTransform_.position);
		stageSelectModel_->SetRotate(stageSelectTransform_.rotation);
		stageSelectModel_->SetScale(stageSelectTransform_.scale);
		stageSelectModel_->Update(dt);
	}
	if (deckEditModel_) {
		ApplyLighting_(deckEditModel_.get());
		deckEditModel_->SetTranslate(deckEditTransform_.position);
		deckEditModel_->SetRotate(deckEditTransform_.rotation);
		deckEditModel_->SetScale(deckEditTransform_.scale);
		deckEditModel_->Update(dt);
	}
	if (tutorialFieldModel_) {
		tutorialFieldModel_->SetTranslate(tutorialFieldTransform_.position);
		tutorialFieldModel_->SetRotate(tutorialFieldTransform_.rotation);
		tutorialFieldModel_->SetScale(tutorialFieldTransform_.scale);
		tutorialFieldModel_->Update(dt);
	}
	if (stageSelectGroundModel_) {
		stageSelectGroundModel_->SetTranslate(stageSelectGroundTransform_.position);
		stageSelectGroundModel_->SetRotate(stageSelectGroundTransform_.rotation);
		stageSelectGroundModel_->SetScale(stageSelectGroundTransform_.scale);
		stageSelectGroundModel_->Update(dt);
	}
	if (baseFieldModel_) {
		baseFieldModel_->SetTranslate(baseFieldTransform_.position);
		baseFieldModel_->SetRotate(baseFieldTransform_.rotation);
		baseFieldModel_->SetScale(baseFieldTransform_.scale);
		baseFieldModel_->Update(dt);
	}
	if (buttonBgSprite_) {
		Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
		Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);
		buttonBgSprite_->SetPosition({buttonBgPos_.x, buttonBgPos_.y});
		buttonBgSprite_->SetScale(buttonBgScale_);
		buttonBgSprite_->SetColor(buttonBgColor_);
		buttonBgSprite_->Update(view, proj);
	}

	int targetIndex = lastModelIndex_;
	if (targetIndex == 2) {
		bgCardScrollY_ += bgCardScrollSpeed_ * dt;

		int cols = std::max(1, bgCardCols_);
		int totalRows = ((int)backgroundCards_.size() + cols - 1) / cols;
		float totalHeight = totalRows * bgCardSpacing_.y;

		for (int i = 0; i < (int)backgroundCards_.size(); ++i) {
			int col = i % cols;
			int row = i / cols;

			float offsetX = (col - (cols - 1) * 0.5f) * bgCardSpacing_.x;
			float offsetY = -row * bgCardSpacing_.y + bgCardScrollY_;

			while (offsetY > bgCardSpacing_.y) {
				offsetY -= totalHeight;
			}
			while (offsetY < -(totalHeight - bgCardSpacing_.y)) {
				offsetY += totalHeight;
			}

			Vector3 pos = {
				bgCardBasePos_.x + offsetX,
				bgCardBasePos_.y + offsetY,
				bgCardBasePos_.z
			};

			backgroundCards_[i]->SetFrameColor({ bgCardColor_.x, bgCardColor_.y, bgCardColor_.z, 1.0f });
			backgroundCards_[i]->SetTargetTransform(pos, bgCardRot_, bgCardScale_, true);
			backgroundCards_[i]->Update(dt);
		}
	} else if (targetIndex == 1) {
		for (int i = 0; i < (int)fullHouseCards_.size(); ++i) {
			float offset = i - 2.0f; 
			Vector3 pos = {
				fhCardBasePos_.x + offset * fhCardSpacing_.x,
				fhCardBasePos_.y - (offset * offset) * fhCardArchHeight_, 
				fhCardBasePos_.z + std::abs(offset) * fhCardSpacing_.z
			};
			Vector3 rot = fhCardRot_;
			rot.z -= offset * fhCardFanAngle_;

			fullHouseCards_[i]->SetFrameColor({ fhCardColor_.x, fhCardColor_.y, fhCardColor_.z, 1.0f });
			fullHouseCards_[i]->SetTargetTransform(pos, rot, fhCardScale_, true);
			fullHouseCards_[i]->Update(dt);
		}
	}

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
		lastModelIndex_ = hoverIndex_;
	} else if (!isUsingMouse_) {
		lastModelIndex_ = selectIndex_;
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

void SelectScene::Draw3D(GameApp& app)
{
	app.ObjCom()->SetGraphicsPipelineState();

	if (showBaseField_ && baseFieldModel_) {
		baseFieldModel_->Draw();
	}

	int targetIndexDraw = lastModelIndex_;

	if (targetIndexDraw == 0) {
		if (tutorialFieldModel_) {
			tutorialFieldModel_->Draw();
		}
		if (tutorialModel_) {
			tutorialModel_->Draw();
		}
	} else if (targetIndexDraw == 1) {
		if (stageSelectGroundModel_) {
			stageSelectGroundModel_->Draw();
		}
		if (stageSelectModel_) {
			stageSelectModel_->Draw();
		}
		for (auto& c : fullHouseCards_) {
			c->Draw();
		}
	} else if (targetIndexDraw == 2) {
		if (deckEditModel_) {
			deckEditModel_->Draw();
		}
		for (auto& c : backgroundCards_) {
			c->Draw();
		}
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

	/*if (bg_) {
		bg_->Update(view, proj);
		bg_->Draw();
	}*/

	Input* input = app.GetInput();
	bool isBackHovered = false;
	if (input) {
		const POINT mouse = input->GetMousePosition();
		isBackHovered = PointInRect_(static_cast<float>(mouse.x), static_cast<float>(mouse.y), backButtonRect_);
	}

	if (backButtonBg_) {
		const float backScale = isBackHovered ? 1.03f : 1.0f;
		const float backScaledW = backButtonRect_.w * backScale;
		const float backScaledH = backButtonRect_.h * backScale;
		const float backOffsetX = (backScaledW - backButtonRect_.w) * 0.5f;
		const float backOffsetY = (backScaledH - backButtonRect_.h) * 0.5f;
		const float backX = backButtonRect_.x - backOffsetX;
		const float backY = backButtonRect_.y - backOffsetY;

		const DirectX::TexMetadata& meta = TextureManager::GetInstance()->GetMetaData(backButtonBg_->GetTextureFilePath());
		backButtonBg_->SetPosition({ backX, backY });
		backButtonBg_->SetScale({ backScaledW / static_cast<float>(meta.width), backScaledH / static_cast<float>(meta.height), 1.0f });
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
	if (ImGui::TreeNode("3D Camera")) {
		if (camera_) {
			Vector3 pos = camera_->GetTranslate();
			if (ImGui::DragFloat3("Position##Cam", &pos.x, 0.1f)) {
				camera_->SetTranslate(pos);
			}
			Vector3 rot = camera_->GetRotate();
			if (ImGui::DragFloat3("Rotation##Cam", &rot.x, 0.01f)) {
				camera_->SetRotate(rot);
			}
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("3D Models")) {
		if (ImGui::TreeNode("Tutorial Model")) {
			ImGui::DragFloat3("Position##Tut", &tutorialTransform_.position.x, 0.1f);
			ImGui::DragFloat3("Rotation##Tut", &tutorialTransform_.rotation.x, 0.01f);
			ImGui::DragFloat3("Scale##Tut", &tutorialTransform_.scale.x, 0.01f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Stage Select Model")) {
			ImGui::DragFloat3("Position##Stg", &stageSelectTransform_.position.x, 0.1f);
			ImGui::DragFloat3("Rotation##Stg", &stageSelectTransform_.rotation.x, 0.01f);
			ImGui::DragFloat3("Scale##Stg", &stageSelectTransform_.scale.x, 0.01f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Deck Edit Model")) {
			ImGui::DragFloat3("Position##Dck", &deckEditTransform_.position.x, 0.1f);
			ImGui::DragFloat3("Rotation##Dck", &deckEditTransform_.rotation.x, 0.01f);
			ImGui::DragFloat3("Scale##Dck", &deckEditTransform_.scale.x, 0.01f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Tutorial Field Model")) {
			ImGui::DragFloat3("Position##TutFld", &tutorialFieldTransform_.position.x, 0.1f);
			ImGui::DragFloat3("Rotation##TutFld", &tutorialFieldTransform_.rotation.x, 0.01f);
			ImGui::DragFloat3("Scale##TutFld", &tutorialFieldTransform_.scale.x, 0.01f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Stage Select Ground Model")) {
			ImGui::DragFloat3("Position##StgGnd", &stageSelectGroundTransform_.position.x, 0.1f);
			ImGui::DragFloat3("Rotation##StgGnd", &stageSelectGroundTransform_.rotation.x, 0.01f);
			ImGui::DragFloat3("Scale##StgGnd", &stageSelectGroundTransform_.scale.x, 0.01f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Background Cards (Target 2)")) {
			ImGui::DragFloat3("Base Pos", &bgCardBasePos_.x, 0.1f);
			ImGui::DragFloat3("Spacing", &bgCardSpacing_.x, 0.05f);
			ImGui::DragFloat3("Scale", &bgCardScale_.x, 0.01f);
			ImGui::DragFloat3("Rotation", &bgCardRot_.x, 0.01f);
			ImGui::DragInt("Columns", &bgCardCols_, 1, 1, 20);
			ImGui::ColorEdit3("Frame Color##bg", &bgCardColor_.x);
			ImGui::DragFloat("Scroll Speed", &bgCardScrollSpeed_, 0.1f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Full House Cards (Target 1)")) {
			ImGui::DragFloat3("Base Pos", &fhCardBasePos_.x, 0.1f);
			ImGui::DragFloat3("Spacing", &fhCardSpacing_.x, 0.05f);
			ImGui::DragFloat3("Scale", &fhCardScale_.x, 0.01f);
			ImGui::DragFloat3("Rotation", &fhCardRot_.x, 0.01f);
			ImGui::DragFloat("Fan Angle", &fhCardFanAngle_, 0.01f);
			ImGui::DragFloat("Arch Height", &fhCardArchHeight_, 0.01f);
			ImGui::ColorEdit3("Frame Color##fh", &fhCardColor_.x);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Bottom Background")) {
			ImGui::Checkbox("Show 3D Field", &showBaseField_);
			ImGui::DragFloat3("Field Position", &baseFieldTransform_.position.x, 0.1f);
			ImGui::DragFloat3("Field Rotation", &baseFieldTransform_.rotation.x, 0.01f);
			ImGui::DragFloat3("Field Scale", &baseFieldTransform_.scale.x, 0.01f);
			ImGui::Checkbox("Show 2D UI Background", &showButtonBg_);
			ImGui::DragFloat2("UI Bg Position", &buttonBgPos_.x, 1.0f);
			ImGui::DragFloat2("UI Bg Scale", &buttonBgScale_.x, 1.0f);
			ImGui::ColorEdit4("UI Bg Color", &buttonBgColor_.x);
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}
	ImGui::Separator();
	if (ImGui::TreeNode("Lighting Settings")) {
		ImGui::SliderInt("Lighting Mode", &light_.lightingMode, 0, 2, "%d (0:Off, 1:Lambert, 2:HalfLambert)");
		if (ImGui::TreeNode("Directional Light")) {
			ImGui::ColorEdit4("Color", &light_.dirColor.x);
			ImGui::DragFloat3("Direction", &light_.dir.x, 0.01f, -1.0f, 1.0f);
			if (ImGui::Button("Normalize Dir")) {
				float len = std::sqrt(light_.dir.x*light_.dir.x + light_.dir.y*light_.dir.y + light_.dir.z*light_.dir.z);
				if (len > 1e-6f) { light_.dir = { light_.dir.x/len, light_.dir.y/len, light_.dir.z/len }; }
			}
			ImGui::DragFloat("Intensity", &light_.dirIntensity, 0.05f, 0.0f, 10.0f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Point Light")) {
			ImGui::ColorEdit4("Color", &light_.pointColor.x);
			ImGui::DragFloat3("Position", &light_.pointPos.x, 0.1f);
			ImGui::DragFloat("Intensity", &light_.pointIntensity, 0.05f, 0.0f, 50.0f);
			ImGui::DragFloat("Radius", &light_.pointRadius, 0.1f, 0.0f, 500.0f);
			ImGui::DragFloat("Decay", &light_.pointDecay, 0.01f, 0.0f, 10.0f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Spot Light")) {
			ImGui::ColorEdit3("Color", &light_.spotColor.x);
			ImGui::DragFloat3("Position", &light_.spotPos.x, 0.1f);
			ImGui::DragFloat3("Direction", &light_.spotDir.x, 0.01f, -1.0f, 1.0f);
			if (ImGui::Button("Normalize Spot Dir")) {
				float len = std::sqrt(light_.spotDir.x*light_.spotDir.x + light_.spotDir.y*light_.spotDir.y + light_.spotDir.z*light_.spotDir.z);
				if (len > 1e-6f) { light_.spotDir = { light_.spotDir.x/len, light_.spotDir.y/len, light_.spotDir.z/len }; }
			}
			ImGui::DragFloat("Intensity", &light_.spotIntensity, 0.05f, 0.0f, 200.0f);
			ImGui::DragFloat("Distance", &light_.spotDistance, 0.1f, 0.0f, 500.0f);
			ImGui::DragFloat("Decay", &light_.spotDecay, 0.01f, 0.0f, 10.0f);
			ImGui::DragFloat("Angle", &light_.spotAngleDeg, 0.1f, 0.0f, 89.0f);
			ImGui::DragFloat("Falloff Start", &light_.spotFalloffStartDeg, 0.1f, 0.0f, 89.0f);
			if (light_.spotFalloffStartDeg > light_.spotAngleDeg - 0.1f) {
				light_.spotFalloffStartDeg = light_.spotAngleDeg - 0.1f;
			}
			ImGui::TreePop();
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

	j["tutorialTransform"] = {
		{"posX", tutorialTransform_.position.x}, {"posY", tutorialTransform_.position.y}, {"posZ", tutorialTransform_.position.z},
		{"rotX", tutorialTransform_.rotation.x}, {"rotY", tutorialTransform_.rotation.y}, {"rotZ", tutorialTransform_.rotation.z},
		{"scaleX", tutorialTransform_.scale.x}, {"scaleY", tutorialTransform_.scale.y}, {"scaleZ", tutorialTransform_.scale.z}
	};
	j["stageSelectTransform"] = {
		{"posX", stageSelectTransform_.position.x}, {"posY", stageSelectTransform_.position.y}, {"posZ", stageSelectTransform_.position.z},
		{"rotX", stageSelectTransform_.rotation.x}, {"rotY", stageSelectTransform_.rotation.y}, {"rotZ", stageSelectTransform_.rotation.z},
		{"scaleX", stageSelectTransform_.scale.x}, {"scaleY", stageSelectTransform_.scale.y}, {"scaleZ", stageSelectTransform_.scale.z}
	};
	j["deckEditTransform"] = {
		{"posX", deckEditTransform_.position.x}, {"posY", deckEditTransform_.position.y}, {"posZ", deckEditTransform_.position.z},
		{"rotX", deckEditTransform_.rotation.x}, {"rotY", deckEditTransform_.rotation.y}, {"rotZ", deckEditTransform_.rotation.z},
		{"scaleX", deckEditTransform_.scale.x}, {"scaleY", deckEditTransform_.scale.y}, {"scaleZ", deckEditTransform_.scale.z}
	};
	j["tutorialFieldTransform"] = {
		{"posX", tutorialFieldTransform_.position.x}, {"posY", tutorialFieldTransform_.position.y}, {"posZ", tutorialFieldTransform_.position.z},
		{"rotX", tutorialFieldTransform_.rotation.x}, {"rotY", tutorialFieldTransform_.rotation.y}, {"rotZ", tutorialFieldTransform_.rotation.z},
		{"scaleX", tutorialFieldTransform_.scale.x}, {"scaleY", tutorialFieldTransform_.scale.y}, {"scaleZ", tutorialFieldTransform_.scale.z}
	};
	j["stageSelectGroundTransform"] = {
		{"posX", stageSelectGroundTransform_.position.x}, {"posY", stageSelectGroundTransform_.position.y}, {"posZ", stageSelectGroundTransform_.position.z},
		{"rotX", stageSelectGroundTransform_.rotation.x}, {"rotY", stageSelectGroundTransform_.rotation.y}, {"rotZ", stageSelectGroundTransform_.rotation.z},
		{"scaleX", stageSelectGroundTransform_.scale.x}, {"scaleY", stageSelectGroundTransform_.scale.y}, {"scaleZ", stageSelectGroundTransform_.scale.z}
	};
	j["bgCardLayout"] = {
		{"basePosX", bgCardBasePos_.x}, {"basePosY", bgCardBasePos_.y}, {"basePosZ", bgCardBasePos_.z},
		{"spacingX", bgCardSpacing_.x}, {"spacingY", bgCardSpacing_.y}, {"spacingZ", bgCardSpacing_.z},
		{"scaleX", bgCardScale_.x}, {"scaleY", bgCardScale_.y}, {"scaleZ", bgCardScale_.z},
		{"rotX", bgCardRot_.x}, {"rotY", bgCardRot_.y}, {"rotZ", bgCardRot_.z},
		{"cols", bgCardCols_},
		{"colorR", bgCardColor_.x}, {"colorG", bgCardColor_.y}, {"colorB", bgCardColor_.z},
		{"scrollSpeed", bgCardScrollSpeed_}
	};
	j["fhCardLayout"] = {
		{"basePosX", fhCardBasePos_.x}, {"basePosY", fhCardBasePos_.y}, {"basePosZ", fhCardBasePos_.z},
		{"spacingX", fhCardSpacing_.x}, {"spacingY", fhCardSpacing_.y}, {"spacingZ", fhCardSpacing_.z},
		{"scaleX", fhCardScale_.x}, {"scaleY", fhCardScale_.y}, {"scaleZ", fhCardScale_.z},
		{"rotX", fhCardRot_.x}, {"rotY", fhCardRot_.y}, {"rotZ", fhCardRot_.z},
		{"fanAngle", fhCardFanAngle_},
		{"archHeight", fhCardArchHeight_},
		{"colorR", fhCardColor_.x}, {"colorG", fhCardColor_.y}, {"colorB", fhCardColor_.z}
	};
	j["bottomBgLayout"] = {
		{"showBaseField", showBaseField_},
		{"baseFieldPosX", baseFieldTransform_.position.x}, {"baseFieldPosY", baseFieldTransform_.position.y}, {"baseFieldPosZ", baseFieldTransform_.position.z},
		{"baseFieldRotX", baseFieldTransform_.rotation.x}, {"baseFieldRotY", baseFieldTransform_.rotation.y}, {"baseFieldRotZ", baseFieldTransform_.rotation.z},
		{"baseFieldScaleX", baseFieldTransform_.scale.x}, {"baseFieldScaleY", baseFieldTransform_.scale.y}, {"baseFieldScaleZ", baseFieldTransform_.scale.z},
		{"showButtonBg", showButtonBg_},
		{"buttonBgPosX", buttonBgPos_.x}, {"buttonBgPosY", buttonBgPos_.y},
		{"buttonBgScaleX", buttonBgScale_.x}, {"buttonBgScaleY", buttonBgScale_.y},
		{"buttonBgColorR", buttonBgColor_.x}, {"buttonBgColorG", buttonBgColor_.y}, {"buttonBgColorB", buttonBgColor_.z}, {"buttonBgColorA", buttonBgColor_.w}
	};

	j["lighting"] = {
		{"mode", light_.lightingMode},
		{"dirColorR", light_.dirColor.x}, {"dirColorG", light_.dirColor.y}, {"dirColorB", light_.dirColor.z},
		{"dirX", light_.dir.x}, {"dirY", light_.dir.y}, {"dirZ", light_.dir.z},
		{"dirIntensity", light_.dirIntensity},
		{"pointColorR", light_.pointColor.x}, {"pointColorG", light_.pointColor.y}, {"pointColorB", light_.pointColor.z},
		{"pointPosX", light_.pointPos.x}, {"pointPosY", light_.pointPos.y}, {"pointPosZ", light_.pointPos.z},
		{"pointIntensity", light_.pointIntensity}, {"pointRadius", light_.pointRadius}, {"pointDecay", light_.pointDecay},
		{"spotColorR", light_.spotColor.x}, {"spotColorG", light_.spotColor.y}, {"spotColorB", light_.spotColor.z},
		{"spotPosX", light_.spotPos.x}, {"spotPosY", light_.spotPos.y}, {"spotPosZ", light_.spotPos.z},
		{"spotDirX", light_.spotDir.x}, {"spotDirY", light_.spotDir.y}, {"spotDirZ", light_.spotDir.z},
		{"spotIntensity", light_.spotIntensity}, {"spotDistance", light_.spotDistance}, {"spotDecay", light_.spotDecay},
		{"spotAngle", light_.spotAngleDeg}, {"spotFalloff", light_.spotFalloffStartDeg}
	};

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

	if (j.contains("tutorialTransform")) {
		tutorialTransform_.position.x = j["tutorialTransform"].value("posX", 0.0f);
		tutorialTransform_.position.y = j["tutorialTransform"].value("posY", 0.0f);
		tutorialTransform_.position.z = j["tutorialTransform"].value("posZ", 0.0f);
		tutorialTransform_.rotation.x = j["tutorialTransform"].value("rotX", 0.0f);
		tutorialTransform_.rotation.y = j["tutorialTransform"].value("rotY", 0.0f);
		tutorialTransform_.rotation.z = j["tutorialTransform"].value("rotZ", 0.0f);
		tutorialTransform_.scale.x = j["tutorialTransform"].value("scaleX", 1.0f);
		tutorialTransform_.scale.y = j["tutorialTransform"].value("scaleY", 1.0f);
		tutorialTransform_.scale.z = j["tutorialTransform"].value("scaleZ", 1.0f);
	}
	if (j.contains("stageSelectTransform")) {
		stageSelectTransform_.position.x = j["stageSelectTransform"].value("posX", 0.0f);
		stageSelectTransform_.position.y = j["stageSelectTransform"].value("posY", 0.0f);
		stageSelectTransform_.position.z = j["stageSelectTransform"].value("posZ", 0.0f);
		stageSelectTransform_.rotation.x = j["stageSelectTransform"].value("rotX", 0.0f);
		stageSelectTransform_.rotation.y = j["stageSelectTransform"].value("rotY", 0.0f);
		stageSelectTransform_.rotation.z = j["stageSelectTransform"].value("rotZ", 0.0f);
		stageSelectTransform_.scale.x = j["stageSelectTransform"].value("scaleX", 1.0f);
		stageSelectTransform_.scale.y = j["stageSelectTransform"].value("scaleY", 1.0f);
		stageSelectTransform_.scale.z = j["stageSelectTransform"].value("scaleZ", 1.0f);
	}
	if (j.contains("deckEditTransform")) {
		deckEditTransform_.position.x = j["deckEditTransform"].value("posX", 0.0f);
		deckEditTransform_.position.y = j["deckEditTransform"].value("posY", 0.0f);
		deckEditTransform_.position.z = j["deckEditTransform"].value("posZ", 0.0f);
		deckEditTransform_.rotation.x = j["deckEditTransform"].value("rotX", 0.0f);
		deckEditTransform_.rotation.y = j["deckEditTransform"].value("rotY", 0.0f);
		deckEditTransform_.rotation.z = j["deckEditTransform"].value("rotZ", 0.0f);
		deckEditTransform_.scale.x = j["deckEditTransform"].value("scaleX", 1.0f);
		deckEditTransform_.scale.y = j["deckEditTransform"].value("scaleY", 1.0f);
		deckEditTransform_.scale.z = j["deckEditTransform"].value("scaleZ", 1.0f);
	}
	if (j.contains("tutorialFieldTransform")) {
		tutorialFieldTransform_.position.x = j["tutorialFieldTransform"].value("posX", 0.0f);
		tutorialFieldTransform_.position.y = j["tutorialFieldTransform"].value("posY", 0.0f);
		tutorialFieldTransform_.position.z = j["tutorialFieldTransform"].value("posZ", 0.0f);
		tutorialFieldTransform_.rotation.x = j["tutorialFieldTransform"].value("rotX", 0.0f);
		tutorialFieldTransform_.rotation.y = j["tutorialFieldTransform"].value("rotY", 0.0f);
		tutorialFieldTransform_.rotation.z = j["tutorialFieldTransform"].value("rotZ", 0.0f);
		tutorialFieldTransform_.scale.x = j["tutorialFieldTransform"].value("scaleX", 1.0f);
		tutorialFieldTransform_.scale.y = j["tutorialFieldTransform"].value("scaleY", 1.0f);
		tutorialFieldTransform_.scale.z = j["tutorialFieldTransform"].value("scaleZ", 1.0f);
	}
	if (j.contains("stageSelectGroundTransform")) {
		stageSelectGroundTransform_.position.x = j["stageSelectGroundTransform"].value("posX", 0.0f);
		stageSelectGroundTransform_.position.y = j["stageSelectGroundTransform"].value("posY", 0.0f);
		stageSelectGroundTransform_.position.z = j["stageSelectGroundTransform"].value("posZ", 0.0f);
		stageSelectGroundTransform_.rotation.x = j["stageSelectGroundTransform"].value("rotX", 0.0f);
		stageSelectGroundTransform_.rotation.y = j["stageSelectGroundTransform"].value("rotY", 0.0f);
		stageSelectGroundTransform_.rotation.z = j["stageSelectGroundTransform"].value("rotZ", 0.0f);
		stageSelectGroundTransform_.scale.x = j["stageSelectGroundTransform"].value("scaleX", 1.0f);
		stageSelectGroundTransform_.scale.y = j["stageSelectGroundTransform"].value("scaleY", 1.0f);
		stageSelectGroundTransform_.scale.z = j["stageSelectGroundTransform"].value("scaleZ", 1.0f);
	}
	if (j.contains("bgCardLayout")) {
		bgCardBasePos_.x = j["bgCardLayout"].value("basePosX", 0.0f);
		bgCardBasePos_.y = j["bgCardLayout"].value("basePosY", 5.0f);
		bgCardBasePos_.z = j["bgCardLayout"].value("basePosZ", 10.0f);
		bgCardSpacing_.x = j["bgCardLayout"].value("spacingX", 1.5f);
		bgCardSpacing_.y = j["bgCardLayout"].value("spacingY", 2.0f);
		bgCardSpacing_.z = j["bgCardLayout"].value("spacingZ", 0.0f);
		bgCardScale_.x = j["bgCardLayout"].value("scaleX", 0.25f);
		bgCardScale_.y = j["bgCardLayout"].value("scaleY", 0.25f);
		bgCardScale_.z = j["bgCardLayout"].value("scaleZ", 0.25f);
		bgCardRot_.x = j["bgCardLayout"].value("rotX", 0.0f);
		bgCardRot_.y = j["bgCardLayout"].value("rotY", 0.0f);
		bgCardRot_.z = j["bgCardLayout"].value("rotZ", 0.0f);
		bgCardCols_ = j["bgCardLayout"].value("cols", 6);
		bgCardColor_.x = j["bgCardLayout"].value("colorR", 0.3f);
		bgCardColor_.y = j["bgCardLayout"].value("colorG", 0.3f);
		bgCardColor_.z = j["bgCardLayout"].value("colorB", 0.3f);
		bgCardScrollSpeed_ = j["bgCardLayout"].value("scrollSpeed", 1.0f);
	}
	if (j.contains("fhCardLayout")) {
		fhCardBasePos_.x = j["fhCardLayout"].value("basePosX", 0.0f);
		fhCardBasePos_.y = j["fhCardLayout"].value("basePosY", 4.0f);
		fhCardBasePos_.z = j["fhCardLayout"].value("basePosZ", -2.0f);
		fhCardSpacing_.x = j["fhCardLayout"].value("spacingX", 0.5f);
		fhCardSpacing_.y = j["fhCardLayout"].value("spacingY", 0.0f);
		fhCardSpacing_.z = j["fhCardLayout"].value("spacingZ", 0.01f);
		fhCardScale_.x = j["fhCardLayout"].value("scaleX", 0.1f);
		fhCardScale_.y = j["fhCardLayout"].value("scaleY", 0.1f);
		fhCardScale_.z = j["fhCardLayout"].value("scaleZ", 0.1f);
		fhCardRot_.x = j["fhCardLayout"].value("rotX", 0.0f);
		fhCardRot_.y = j["fhCardLayout"].value("rotY", 0.0f);
		fhCardRot_.z = j["fhCardLayout"].value("rotZ", 0.0f);
		fhCardFanAngle_ = j["fhCardLayout"].value("fanAngle", 0.15f);
		fhCardArchHeight_ = j["fhCardLayout"].value("archHeight", 0.1f);
		fhCardColor_.x = j["fhCardLayout"].value("colorR", 1.0f);
		fhCardColor_.y = j["fhCardLayout"].value("colorG", 0.2f);
		fhCardColor_.z = j["fhCardLayout"].value("colorB", 0.2f);
	}
	if (j.contains("bottomBgLayout")) {
		showBaseField_ = j["bottomBgLayout"].value("showBaseField", true);
		baseFieldTransform_.position.x = j["bottomBgLayout"].value("baseFieldPosX", 0.0f);
		baseFieldTransform_.position.y = j["bottomBgLayout"].value("baseFieldPosY", 0.0f);
		baseFieldTransform_.position.z = j["bottomBgLayout"].value("baseFieldPosZ", 0.0f);
		baseFieldTransform_.rotation.x = j["bottomBgLayout"].value("baseFieldRotX", 0.0f);
		baseFieldTransform_.rotation.y = j["bottomBgLayout"].value("baseFieldRotY", 0.0f);
		baseFieldTransform_.rotation.z = j["bottomBgLayout"].value("baseFieldRotZ", 0.0f);
		baseFieldTransform_.scale.x = j["bottomBgLayout"].value("baseFieldScaleX", 1.0f);
		baseFieldTransform_.scale.y = j["bottomBgLayout"].value("baseFieldScaleY", 1.0f);
		baseFieldTransform_.scale.z = j["bottomBgLayout"].value("baseFieldScaleZ", 1.0f);
		
		showButtonBg_ = j["bottomBgLayout"].value("showButtonBg", false);
		buttonBgPos_.x = j["bottomBgLayout"].value("buttonBgPosX", 640.0f);
		buttonBgPos_.y = j["bottomBgLayout"].value("buttonBgPosY", 600.0f);
		buttonBgScale_.x = j["bottomBgLayout"].value("buttonBgScaleX", 1280.0f);
		buttonBgScale_.y = j["bottomBgLayout"].value("buttonBgScaleY", 200.0f);
		buttonBgColor_.x = j["bottomBgLayout"].value("buttonBgColorR", 1.0f);
		buttonBgColor_.y = j["bottomBgLayout"].value("buttonBgColorG", 1.0f);
		buttonBgColor_.z = j["bottomBgLayout"].value("buttonBgColorB", 1.0f);
		buttonBgColor_.w = j["bottomBgLayout"].value("buttonBgColorA", 0.5f);
	}

	if (j.contains("lighting")) {
		auto& jl = j["lighting"];
		light_.lightingMode = jl.value("mode", 2);
		light_.dirColor.x = jl.value("dirColorR", 1.0f); light_.dirColor.y = jl.value("dirColorG", 1.0f); light_.dirColor.z = jl.value("dirColorB", 1.0f);
		light_.dir.x = jl.value("dirX", 0.3f); light_.dir.y = jl.value("dirY", -1.0f); light_.dir.z = jl.value("dirZ", 0.2f);
		light_.dirIntensity = jl.value("dirIntensity", 1.5f);
		light_.pointColor.x = jl.value("pointColorR", 1.0f); light_.pointColor.y = jl.value("pointColorG", 1.0f); light_.pointColor.z = jl.value("pointColorB", 1.0f);
		light_.pointPos.x = jl.value("pointPosX", 0.0f); light_.pointPos.y = jl.value("pointPosY", 15.0f); light_.pointPos.z = jl.value("pointPosZ", 10.0f);
		light_.pointIntensity = jl.value("pointIntensity", 2.0f); light_.pointRadius = jl.value("pointRadius", 80.0f); light_.pointDecay = jl.value("pointDecay", 1.5f);
		light_.spotColor.x = jl.value("spotColorR", 1.0f); light_.spotColor.y = jl.value("spotColorG", 1.0f); light_.spotColor.z = jl.value("spotColorB", 1.0f);
		light_.spotPos.x = jl.value("spotPosX", 0.0f); light_.spotPos.y = jl.value("spotPosY", 20.0f); light_.spotPos.z = jl.value("spotPosZ", -10.0f);
		light_.spotDir.x = jl.value("spotDirX", 0.0f); light_.spotDir.y = jl.value("spotDirY", -1.0f); light_.spotDir.z = jl.value("spotDirZ", 0.3f);
		light_.spotIntensity = jl.value("spotIntensity", 0.0f); light_.spotDistance = jl.value("spotDistance", 100.0f); light_.spotDecay = jl.value("spotDecay", 2.0f);
		light_.spotAngleDeg = jl.value("spotAngle", 35.0f); light_.spotFalloffStartDeg = jl.value("spotFalloff", 25.0f);
	}
}

void SelectScene::ApplyLighting_(Object3d* obj) {
	if (!obj) return;
	obj->SetEnableLighting(light_.lightingMode);
	
	obj->SetLightColor(light_.dirColor);
	obj->SetDirection(light_.dir);
	obj->SetIntensity(light_.dirIntensity);
	
	obj->SetPointLightColor(light_.pointColor);
	obj->SetPointLightPos(light_.pointPos);
	obj->SetPointLightIntensity(light_.pointIntensity);
	obj->SetPointLightRadius(light_.pointRadius);
	obj->SetPointLightDecay(light_.pointDecay);
	
	obj->SetSpotLightColor({ light_.spotColor.x, light_.spotColor.y, light_.spotColor.z, 1.0f });
	obj->SetSpotLightPos(light_.spotPos);
	obj->SetSpotLightDirection(light_.spotDir);
	obj->SetSpotLightIntensity(light_.spotIntensity);
	obj->SetSpotLightDistance(light_.spotDistance);
	obj->SetSpotLightDecay(light_.spotDecay);
	
	const float cosOuter = std::cosf(light_.spotAngleDeg * (3.141592654f / 180.0f));
	const float cosInner = std::cosf(light_.spotFalloffStartDeg * (3.141592654f / 180.0f));
	obj->SetSpotLightCosAngle(cosOuter);
	obj->SetSpotLightCosFalloffStart(cosInner);
}

void SelectScene::DrawSkydome(GameApp& app) {
	(void)app;
	if (lastModelIndex_ == 1 && skyDome_) {
		skyDome_->Draw();
	}
}
