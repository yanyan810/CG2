#include"SelectionState.h"

#include"GameApp.h"

#include"../PausingUI.h"
#include"../GiveUp/GiveUpConfirmState.h"
#include"../Help/HelpState.h"
#include "AudioManager.h"
#include <fstream>
#include <nlohmann/json.hpp>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

void SelectionState::Initialize(GameApp& app) {
	LoadLayout_();

	title_ = std::make_unique<TextSprite>();
	title_->Initialize(app.SpriteCom(), app.Dx());
	title_->SetFontSize(50);
	title_->SetSize({ 1.f, 1.f, 1.f });
	title_->SetPosition({ layout_.titlePos.x, layout_.titlePos.y });
	title_->SetText(L"ポーズ中");

	// --- 再開ボタン ---
	resumeBtn_ = std::make_unique<Button>();
	resumeBtn_->Initialize(app, "ResumeButton", { layout_.resumeBtnPos.x, layout_.resumeBtnPos.y }, "resources/ui/white.png", "resources/ui/text/Restart.png");
	resumeBtn_->SetBgSize({ layout_.resumeBtnBgSize.x, layout_.resumeBtnBgSize.y });
	resumeBtn_->SetFrameSize({ layout_.resumeBtnFrameSize.x, layout_.resumeBtnFrameSize.y });
	resumeBtn_->SetNormalColor({ 0.1f, 0.1f, 0.1f, 0.9f });
	resumeBtn_->SetHoverColor({ 0.4f, 0.4f, 0.4f, 0.9f });

	// --- ヘルプボタン ---
	helpBtn_ = std::make_unique<Button>();
	helpBtn_->Initialize(app, "HelpButton", { layout_.helpBtnPos.x, layout_.helpBtnPos.y }, "resources/ui/white.png", "resources/ui/text/Help.png");
	helpBtn_->SetBgSize({ layout_.helpBtnBgSize.x, layout_.helpBtnBgSize.y });
	helpBtn_->SetFrameSize({ layout_.helpBtnFrameSize.x, layout_.helpBtnFrameSize.y });
	helpBtn_->SetNormalColor({ 0.1f, 0.1f, 0.1f, 0.9f });
	helpBtn_->SetHoverColor({ 0.4f, 0.4f, 0.4f, 0.9f });

	// --- 降参ボタン ---
	std::string giveUpTexture = isTutorialMode_ ? "resources/ui/text/tutorial_Finish.png" : "resources/ui/text/Surrender.png";
	giveUpBtn_ = std::make_unique<Button>();
	giveUpBtn_->Initialize(app, "GiveUpButton", { layout_.giveUpBtnPos.x, layout_.giveUpBtnPos.y }, "resources/ui/white.png", giveUpTexture);
	giveUpBtn_->SetBgSize({ layout_.giveUpBtnBgSize.x, layout_.giveUpBtnBgSize.y });
	giveUpBtn_->SetFrameSize({ layout_.giveUpBtnFrameSize.x, layout_.giveUpBtnFrameSize.y });
	giveUpBtn_->SetNormalColor({ 0.1f, 0.1f, 0.1f, 0.9f });
	giveUpBtn_->SetHoverColor({ 0.9f, 0.0f, 0.0f, 0.9f });
	
	statusMenu_.Initialize(app, { layout_.statusMenuPos.x, layout_.statusMenuPos.y });

	ApplyLayout_();
}

void SelectionState::Update(PausingUI* context, GameApp& app, Input* input) {

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);

	statusMenu_.Update(app, view, proj);

	if (resumeBtn_->IsPressed()) {
		context->SetIsPaused(false);
	} else if (helpBtn_->IsPressed()) {
		context->ChangeState(std::make_unique<HelpState>(), app);
		return;
	} else if (giveUpBtn_->IsPressed()) {
		context->ChangeState(std::make_unique<GiveUpConfirmState>(), app);
		return;
	}

	if (title_) title_->Update(view, proj);

	resumeBtn_->Update(app, view, proj);
	helpBtn_->Update(app, view, proj);
	giveUpBtn_->Update(app, view, proj);
}

void SelectionState::Draw(GameApp& app) {
	resumeBtn_->Draw();
	helpBtn_->Draw();
	giveUpBtn_->Draw();
	title_->Draw();
	statusMenu_.Draw();
}

void SelectionState::SaveLayout_() const {
	nlohmann::json j;
	j["titlePos"] = { layout_.titlePos.x, layout_.titlePos.y };
	j["titleSize"] = layout_.titleSize;
	
	j["resumeBtnPos"] = { layout_.resumeBtnPos.x, layout_.resumeBtnPos.y };
	j["resumeBtnBgSize"] = { layout_.resumeBtnBgSize.x, layout_.resumeBtnBgSize.y };
	j["resumeBtnFrameSize"] = { layout_.resumeBtnFrameSize.x, layout_.resumeBtnFrameSize.y };
	j["resumeBtnFrameOffset"] = { layout_.resumeBtnFrameOffset.x, layout_.resumeBtnFrameOffset.y };
	
	j["helpBtnPos"] = { layout_.helpBtnPos.x, layout_.helpBtnPos.y };
	j["helpBtnBgSize"] = { layout_.helpBtnBgSize.x, layout_.helpBtnBgSize.y };
	j["helpBtnFrameSize"] = { layout_.helpBtnFrameSize.x, layout_.helpBtnFrameSize.y };
	j["helpBtnFrameOffset"] = { layout_.helpBtnFrameOffset.x, layout_.helpBtnFrameOffset.y };
	
	j["giveUpBtnPos"] = { layout_.giveUpBtnPos.x, layout_.giveUpBtnPos.y };
	j["giveUpBtnBgSize"] = { layout_.giveUpBtnBgSize.x, layout_.giveUpBtnBgSize.y };
	j["giveUpBtnFrameSize"] = { layout_.giveUpBtnFrameSize.x, layout_.giveUpBtnFrameSize.y };
	j["giveUpBtnFrameOffset"] = { layout_.giveUpBtnFrameOffset.x, layout_.giveUpBtnFrameOffset.y };
	
	j["statusMenuPos"] = { layout_.statusMenuPos.x, layout_.statusMenuPos.y };

	std::ofstream o(layoutPath_);
	if (o.is_open()) {
		o << j.dump(4);
	}
}

void SelectionState::LoadLayout_() {
	std::ifstream i(layoutPath_);
	if (i.is_open()) {
		nlohmann::json j;
		i >> j;
		if (j.contains("titlePos")) { layout_.titlePos = { j["titlePos"][0], j["titlePos"][1] }; }
		if (j.contains("titleSize")) { layout_.titleSize = j["titleSize"]; }
		
		if (j.contains("resumeBtnPos")) { layout_.resumeBtnPos = { j["resumeBtnPos"][0], j["resumeBtnPos"][1] }; }
		if (j.contains("resumeBtnBgSize")) { layout_.resumeBtnBgSize = { j["resumeBtnBgSize"][0], j["resumeBtnBgSize"][1] }; }
		if (j.contains("resumeBtnFrameSize")) { layout_.resumeBtnFrameSize = { j["resumeBtnFrameSize"][0], j["resumeBtnFrameSize"][1] }; }
		if (j.contains("resumeBtnFrameOffset")) { layout_.resumeBtnFrameOffset = { j["resumeBtnFrameOffset"][0], j["resumeBtnFrameOffset"][1] }; }
		
		if (j.contains("helpBtnPos")) { layout_.helpBtnPos = { j["helpBtnPos"][0], j["helpBtnPos"][1] }; }
		if (j.contains("helpBtnBgSize")) { layout_.helpBtnBgSize = { j["helpBtnBgSize"][0], j["helpBtnBgSize"][1] }; }
		if (j.contains("helpBtnFrameSize")) { layout_.helpBtnFrameSize = { j["helpBtnFrameSize"][0], j["helpBtnFrameSize"][1] }; }
		if (j.contains("helpBtnFrameOffset")) { layout_.helpBtnFrameOffset = { j["helpBtnFrameOffset"][0], j["helpBtnFrameOffset"][1] }; }
		
		if (j.contains("giveUpBtnPos")) { layout_.giveUpBtnPos = { j["giveUpBtnPos"][0], j["giveUpBtnPos"][1] }; }
		if (j.contains("giveUpBtnBgSize")) { layout_.giveUpBtnBgSize = { j["giveUpBtnBgSize"][0], j["giveUpBtnBgSize"][1] }; }
		if (j.contains("giveUpBtnFrameSize")) { layout_.giveUpBtnFrameSize = { j["giveUpBtnFrameSize"][0], j["giveUpBtnFrameSize"][1] }; }
		if (j.contains("giveUpBtnFrameOffset")) { layout_.giveUpBtnFrameOffset = { j["giveUpBtnFrameOffset"][0], j["giveUpBtnFrameOffset"][1] }; }
		
		if (j.contains("statusMenuPos")) { layout_.statusMenuPos = { j["statusMenuPos"][0], j["statusMenuPos"][1] }; }
	}
}

void SelectionState::ApplyLayout_() {
	if (title_) {
		title_->SetPosition({ layout_.titlePos.x, layout_.titlePos.y });
		title_->SetFontSize(static_cast<uint32_t>(layout_.titleSize));
	}
	if (resumeBtn_) {
		resumeBtn_->SetPosition({ layout_.resumeBtnPos.x, layout_.resumeBtnPos.y });
		resumeBtn_->SetBgSize({ layout_.resumeBtnBgSize.x, layout_.resumeBtnBgSize.y });
		resumeBtn_->SetFrameSize({ layout_.resumeBtnFrameSize.x, layout_.resumeBtnFrameSize.y });
		resumeBtn_->SetFrameOffset({ layout_.resumeBtnFrameOffset.x, layout_.resumeBtnFrameOffset.y });
	}
	if (helpBtn_) {
		helpBtn_->SetPosition({ layout_.helpBtnPos.x, layout_.helpBtnPos.y });
		helpBtn_->SetBgSize({ layout_.helpBtnBgSize.x, layout_.helpBtnBgSize.y });
		helpBtn_->SetFrameSize({ layout_.helpBtnFrameSize.x, layout_.helpBtnFrameSize.y });
		helpBtn_->SetFrameOffset({ layout_.helpBtnFrameOffset.x, layout_.helpBtnFrameOffset.y });
	}
	if (giveUpBtn_) {
		giveUpBtn_->SetPosition({ layout_.giveUpBtnPos.x, layout_.giveUpBtnPos.y });
		giveUpBtn_->SetBgSize({ layout_.giveUpBtnBgSize.x, layout_.giveUpBtnBgSize.y });
		giveUpBtn_->SetFrameSize({ layout_.giveUpBtnFrameSize.x, layout_.giveUpBtnFrameSize.y });
		giveUpBtn_->SetFrameOffset({ layout_.giveUpBtnFrameOffset.x, layout_.giveUpBtnFrameOffset.y });
	}
	statusMenu_.SetPosition({ layout_.statusMenuPos.x, layout_.statusMenuPos.y });
}

void SelectionState::DrawImGui() {
#ifdef USE_IMGUI
	bool changed = false;
	if (ImGui::CollapsingHeader("Selection UI Layout")) {
		if (ImGui::TreeNode("Title")) {
			changed |= ImGui::DragFloat2("Pos", &layout_.titlePos.x, 1.0f);
			changed |= ImGui::DragFloat("Size", &layout_.titleSize, 1.0f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Resume Button")) {
			changed |= ImGui::DragFloat2("Pos", &layout_.resumeBtnPos.x, 1.0f);
			changed |= ImGui::DragFloat2("Bg Size", &layout_.resumeBtnBgSize.x, 1.0f);
			changed |= ImGui::DragFloat2("Frame Size", &layout_.resumeBtnFrameSize.x, 1.0f);
			changed |= ImGui::DragFloat2("Frame Offset", &layout_.resumeBtnFrameOffset.x, 1.0f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Help Button")) {
			changed |= ImGui::DragFloat2("Pos", &layout_.helpBtnPos.x, 1.0f);
			changed |= ImGui::DragFloat2("Bg Size", &layout_.helpBtnBgSize.x, 1.0f);
			changed |= ImGui::DragFloat2("Frame Size", &layout_.helpBtnFrameSize.x, 1.0f);
			changed |= ImGui::DragFloat2("Frame Offset", &layout_.helpBtnFrameOffset.x, 1.0f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("GiveUp Button")) {
			changed |= ImGui::DragFloat2("Pos", &layout_.giveUpBtnPos.x, 1.0f);
			changed |= ImGui::DragFloat2("Bg Size", &layout_.giveUpBtnBgSize.x, 1.0f);
			changed |= ImGui::DragFloat2("Frame Size", &layout_.giveUpBtnFrameSize.x, 1.0f);
			changed |= ImGui::DragFloat2("Frame Offset", &layout_.giveUpBtnFrameOffset.x, 1.0f);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Status Menu")) {
			changed |= ImGui::DragFloat2("Pos", &layout_.statusMenuPos.x, 1.0f);
			ImGui::TreePop();
		}

		if (ImGui::Button("Save Layout")) {
			SaveLayout_();
		}
	}

	if (changed) {
		ApplyLayout_();
	}
#endif
}
