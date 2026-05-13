#include "StageSelectScene.h"

#include "GameApp.h"
#include "Camera.h"
#include "Input.h"
#include "WinApp.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "Matrix4x4.h"
#include "AudioManager.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include <fstream>
#include <nlohmann/json.hpp>

bool StageSelectScene::PointInRect_(float mx, float my, const Rect& rect) const {
	return mx >= rect.x &&
		mx <= rect.x + rect.w &&
		my >= rect.y &&
		my <= rect.y + rect.h;
}

void StageSelectScene::OnEnter(GameApp& app) {
	hoverIndex_ = -1;
	selectIndex_ = 0;
	circle_ = 0.0f;
	softness_ = 0.6f;

	camera_ = std::make_unique<Camera>();
	camera_->SetTranslate({ 0.0f, 0.0f, 0.0f });
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	app.ObjCom()->SetDefaultCamera(camera_.get());

	bg_ = std::make_unique<Sprite>();
	bg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/stage_select/bg.png");
	bg_->SetAnchorPoint({ 0.0f, 0.0f });
	bg_->SetPosition({ 0.0f, 0.0f });

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
	stageItems_[0].descText = L"ゲームの基本操作や\n特殊効果の流れを学べます";
	stageItems_[0].buttonRect = {
	 layout_.tutorialButtonRect.x,
	 layout_.tutorialButtonRect.y,
	 layout_.tutorialButtonRect.w,
	 layout_.tutorialButtonRect.h
	};
	stageItems_[0].buttonSprite = std::make_unique<Sprite>();
	stageItems_[0].buttonSprite->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/stage_select/button_tutorial.png");
	stageItems_[0].buttonSprite->SetAnchorPoint({ 0.0f, 0.0f });
	stageItems_[0].buttonSprite->SetPosition({ stageItems_[0].buttonRect.x, stageItems_[0].buttonRect.y });

	stageItems_[1].sceneName = "Game";
	stageItems_[1].descText = L"通常のバトルを開始します\n実戦用のモードです";
	stageItems_[1].buttonRect = {
	layout_.battleButtonRect.x,
	layout_.battleButtonRect.y,
	layout_.battleButtonRect.w,
	layout_.battleButtonRect.h
	};
	stageItems_[1].buttonSprite = std::make_unique<Sprite>();
	stageItems_[1].buttonSprite->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/stage_select/button_battle.png");
	stageItems_[1].buttonSprite->SetAnchorPoint({ 0.0f, 0.0f });
	stageItems_[1].buttonSprite->SetPosition({ stageItems_[1].buttonRect.x, stageItems_[1].buttonRect.y });

	stageItems_[2].sceneName = "DeckEdit";
	stageItems_[2].descText = L"使用するデッキを\n編成します";
	stageItems_[2].buttonRect = {
	layout_.deckEditButtonRect.x,
	layout_.deckEditButtonRect.y,
	layout_.deckEditButtonRect.w,
	layout_.deckEditButtonRect.h
	};// 仮の座標（後でImGui等で調整）
	stageItems_[2].buttonSprite = std::make_unique<Sprite>();
	stageItems_[2].buttonSprite->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/stage_select/button_deckEdit.png");
	stageItems_[2].buttonSprite->SetAnchorPoint({ 0.0f, 0.0f });
	stageItems_[2].buttonSprite->SetPosition({ stageItems_[2].buttonRect.x, stageItems_[2].buttonRect.y });

	debugHitBgs_.resize(stageItems_.size());
	for (int i = 0; i < static_cast<int>(stageItems_.size()); ++i) {
		debugHitBgs_[i] = std::make_unique<Sprite>();
		debugHitBgs_[i]->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
		debugHitBgs_[i]->SetAnchorPoint({ 0.0f, 0.0f });
	}

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
	stageItems_.clear();
	descBgBottom_.reset();
	descBgTop_.reset();
	titleSprite_.reset();
	bg_.reset();
	camera_.reset();
	descTextSprite_.reset();
}

void StageSelectScene::Update(GameApp& app, float dt) {
	(void)dt;

	ApplyLayout_();

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
			RequestChangeScene_(stageItems_[selectIndex_].sceneName.c_str());
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

	if (input->IsKeyTrigger(DIK_RETURN) || input->IsKeyTrigger(DIK_SPACE)) {
		AudioManager::GetInstance()->PlaySE("SE_Tap");
		RequestChangeScene_(stageItems_[selectIndex_].sceneName.c_str());
		return;
	}

	const int showIndex = hoverIndex_;
	if (showIndex == 0) {
		currentDescText_ = stageItems_[0].descText;
		currentDescPos_ = { layout_.tutorialDescText.x, layout_.tutorialDescText.y };
	} else if (showIndex == 1) {
		currentDescText_ = stageItems_[1].descText;
		currentDescPos_ = { layout_.battleDescText.x, layout_.battleDescText.y };
	} else if (showIndex == 2) {
		currentDescText_ = stageItems_[2].descText;
		currentDescPos_ = { layout_.deckEditDescText.x, layout_.deckEditDescText.y };
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

	float scale = 1.5f;
	if (selectIndex_ == 0) {
		scale = layout_.tutorialDescText.scale;
	} else if (selectIndex_ == 1) {
		scale = layout_.battleDescText.scale;
	} else if (selectIndex_ == 2) {
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



	if (bg_) {
		bg_->Update(view, proj);
		bg_->Draw();
	}

	


	if (titleSprite_) {
		titleSprite_->Update(view, proj);
		titleSprite_->Draw();
	}

	for (int i = 0; i < static_cast<int>(stageItems_.size()); ++i) {
		if (!stageItems_[i].buttonSprite) {
			continue;
		}

		if (hoverIndex_ == 1 && i == 2) continue;

		// 2. デッキ編集(index 2)にホバー中なら、バトル(index 1)を表示しない
		if (hoverIndex_ == 2 && i == 1) continue;

		float scale = 1.0f;
		if (i == hoverIndex_) {
			scale = hoverScale_;
		}

		stageItems_[i].buttonSprite->SetScale({ scale, scale, 1.0f });
		stageItems_[i].buttonSprite->Update(view, proj);
		stageItems_[i].buttonSprite->Draw();
	}

	if (descBgTop_) {
		descBgTop_->Update(view, proj);
		descBgTop_->Draw();
	}

	if (descBgBottom_) {
		descBgBottom_->Update(view, proj);
		descBgBottom_->Draw();
	}

	DrawDescriptionText_(app, currentDescText_, currentDescPos_.x, currentDescPos_.y);

	// デバッグ用：当たり判定表示
	if (showDebugHitBox_) {
		for (int i = 0; i < static_cast<int>(stageItems_.size()); ++i) {
			const Rect& r = stageItems_[i].buttonRect;

			if (hoverIndex_ == 1 && i == 2) continue;

			// 2. デッキ編集(index 2)にホバー中なら、バトル(index 1)を表示しない
			if (hoverIndex_ == 2 && i == 1) continue;

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

	if (descBgTop_) {
		descBgTop_->SetPosition({ layout_.tutorialDescBgPos.x, layout_.tutorialDescBgPos.y });
	}

	if (descBgBottom_) {
		descBgBottom_->SetPosition({ layout_.battleDescBgPos.x, layout_.battleDescBgPos.y });
	}

	if (stageItems_.size() >= 3) {
		stageItems_[0].buttonRect = {
			layout_.tutorialButtonRect.x,
			layout_.tutorialButtonRect.y,
			layout_.tutorialButtonRect.w,
			layout_.tutorialButtonRect.h
		};

		stageItems_[1].buttonRect = {
			layout_.battleButtonRect.x,
			layout_.battleButtonRect.y,
			layout_.battleButtonRect.w,
			layout_.battleButtonRect.h
		};

		stageItems_[2].buttonRect = {
			layout_.deckEditButtonRect.x,
			layout_.deckEditButtonRect.y,
			layout_.deckEditButtonRect.w,
			layout_.deckEditButtonRect.h
		};

		stageItems_[0].descRect = {
			layout_.tutorialDescBgPos.x,
			layout_.tutorialDescBgPos.y,
			layout_.tutorialButtonRect.w,
			layout_.tutorialButtonRect.h
		};

		stageItems_[1].descRect = {
			layout_.battleDescBgPos.x,
			layout_.battleDescBgPos.y,
			layout_.battleButtonRect.w,
			layout_.battleButtonRect.h
		};

		stageItems_[2].descRect = {
			layout_.deckEditDescBgPos.x,
			layout_.deckEditDescBgPos.y,
			layout_.deckEditButtonRect.w,
			layout_.deckEditButtonRect.h
		};

		if (stageItems_[0].buttonSprite) {
			stageItems_[0].buttonSprite->SetPosition({
				layout_.tutorialButtonVisualPos.x,
				layout_.tutorialButtonVisualPos.y
				});
		}

		if (stageItems_[1].buttonSprite) {
			stageItems_[1].buttonSprite->SetPosition({
				layout_.battleButtonVisualPos.x,
				layout_.battleButtonVisualPos.y
				});
		}
		if (stageItems_[2].buttonSprite) {
			stageItems_[2].buttonSprite->SetPosition({
				layout_.deckEditButtonVisualPos.x,
				layout_.deckEditButtonVisualPos.y
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
