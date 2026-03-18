#include "FieldUi.h"
#include "GameApp.h"
#include "BattleController.h"
#include "TextSprite.h"
#include "Sprite.h"
#include "WinApp.h"
#include "CardDatabase.h"

#include <fstream>
#include <nlohmann/json.hpp>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

static std::wstring Utf8ToWStringLocal(const std::string& s)
{
	if (s.empty()) return L"";
	int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring out(size - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), size);
	return out;
}

std::wstring FieldUi::Utf8ToWString_(const std::string& s)
{
	return Utf8ToWStringLocal(s);
}

void FieldUi::Initialize(GameApp& app)
{
	cardDescText_ = std::make_unique<TextSprite>();
	cardDescText_->Initialize(app.SpriteCom(), app.Dx());
	cardDescText_->SetPosition({ 40.0f, 620.0f });

	cardDescBg_ = std::make_unique<Sprite>();
	cardDescBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	cardDescBg_->SetPosition({ 20.0f, 60.0f });
	cardDescBg_->SetScale({ 900.0f, 180.0f, 1.0f });
	cardDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

	deckCountText_ = std::make_unique<TextSprite>();
	deckCountText_->Initialize(app.SpriteCom(), app.Dx());
	deckCountText_->SetPosition({ 1120.0f, 640.0f });
	deckCountText_->SetSize({ 0.9f, 0.9f, 1.0f });

	deckCountBg_ = std::make_unique<Sprite>();
	deckCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	deckCountBg_->SetPosition({ 20.0f, 310.0f });
	deckCountBg_->SetScale({ 0.0f, 0.0f, 1.0f });
	deckCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

	discardCountText_ = std::make_unique<TextSprite>();
	discardCountText_->Initialize(app.SpriteCom(), app.Dx());
	discardCountText_->SetPosition({ 1120.0f, 350.0f });
	discardCountText_->SetSize({ 0.9f, 0.9f, 1.0f });

	discardCountBg_ = std::make_unique<Sprite>();
	discardCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	discardCountBg_->SetPosition({ 1100.0f, 350.0f });
	discardCountBg_->SetScale({ 150.0f, 60.0f, 1.0f });
	discardCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

	handCountText_ = std::make_unique<TextSprite>();
	handCountText_->Initialize(app.SpriteCom(), app.Dx());
	handCountText_->SetPosition({ 1020.0f, 640.0f });
	handCountText_->SetSize({ 0.9f, 0.9f, 1.0f });

	handCountBg_ = std::make_unique<Sprite>();
	handCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	handCountBg_->SetPosition({ 1000.0f, 640.0f });
	handCountBg_->SetScale({ 250.0f, 60.0f, 1.0f });
	handCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

	fieldCountText_ = std::make_unique<TextSprite>();
	fieldCountText_->Initialize(app.SpriteCom(), app.Dx());
	fieldCountText_->SetPosition({ 600.0f, 250.0f });
	fieldCountText_->SetSize({ 0.9f, 0.9f, 1.0f });

	fieldCountBg_ = std::make_unique<Sprite>();
	fieldCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	fieldCountBg_->SetPosition({ 540.0f, 250.0f });
	fieldCountBg_->SetScale({ 250.0f, 60.0f, 1.0f });
	fieldCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

	pokerTitleText_ = std::make_unique<TextSprite>();
	pokerTitleText_->Initialize(app.SpriteCom(), app.Dx());
	pokerTitleText_->SetPosition({ 40.0f, 80.0f });
	pokerTitleText_->SetSize({ 1.0f, 1.0f, 1.0f });

	for (int i = 0; i < 4; ++i) {
		pokerOptionBgs_[i] = std::make_unique<Sprite>();
		pokerOptionBgs_[i]->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
		pokerOptionBgs_[i]->SetColor({ 0.0f, 0.0f, 0.0f, 0.65f });
		pokerOptionBgs_[i]->SetScale({ 380.0f, 130.0f, 1.0f });
	}

	for (int i = 0; i < 4; ++i) {
		pokerOptionTexts_[i] = std::make_unique<TextSprite>();
		pokerOptionTexts_[i]->Initialize(app.SpriteCom(), app.Dx());
		pokerOptionTexts_[i]->SetSize({ 1.0f, 1.0f, 1.0f });
		pokerOptionTexts_[i]->SetPosition({ 40.0f, 120.0f + 40.0f * i });
	}

	LoadPokerEffectChoiceLayout(pokerEffectLayoutPath_);

}

void FieldUi::Update(GameApp& app, const BattleController& battle)
{
	showDescBg_ = false;

	showPokerOptions_ = false;
	pokerHoverIndex_ = -1;
	pokerOptionCount_ = 0;

	DescMode newMode = DescMode::None;
	int newPreviewDefId = -1;
	std::wstring newText;

	if (battle.HasPokerChoiceUi()) {
		newMode = DescMode::PokerChoice;
		showDescBg_ = true;
		showPokerOptions_ = true;
		pokerHoverIndex_ = battle.GetPokerMouseChoiceIndex();

		cardDescBg_->SetPosition({ 20.0f, 52.0f });
		cardDescBg_->SetScale({ 900.0f, 220.0f, 1.0f });

		cardDescText_->SetText(L"");

		if (battle.IsWaitingActivateChoice()) {
			pokerOptionCount_ = 2;

			// 上の確認メッセージ
			pokerTitleText_->SetText(L"特殊効果を発動しますか？");
			pokerTitleText_->SetSize({ 1.15f, 1.15f, 1.0f });
			pokerTitleText_->SetPosition({ 470.0f, 185.0f });

			// 左ボタン文字
			pokerOptionTexts_[0]->SetText(L"発動する");
			pokerOptionTexts_[0]->SetPosition({ 225.0f, 478.0f });

			// 右ボタン文字
			pokerOptionTexts_[1]->SetText(L"発動しない");
			pokerOptionTexts_[1]->SetPosition({ 890.0f, 478.0f });

			pokerOptionTexts_[2]->SetText(L"");
			pokerOptionTexts_[3]->SetText(L"");
		} else if (battle.IsWaitingEffectChoice()) {
			pokerOptionCount_ = 4;

			pokerTitleText_->SetText(L"発動する効果を選んでください");
			pokerTitleText_->SetSize({ 1.05f, 1.05f, 1.0f });
			pokerTitleText_->SetPosition({
				pokerEffectLayout_.titleText.x,
				pokerEffectLayout_.titleText.y
				});

			pokerOptionTexts_[0]->SetText(L"戻る");
			pokerOptionTexts_[0]->SetPosition({
				pokerEffectLayout_.backText.x,
				pokerEffectLayout_.backText.y
				});
			pokerOptionTexts_[0]->SetSize({ 1.15f, 1.15f, 1.0f });

			pokerOptionTexts_[1]->SetText(L"次ターンATK UP");
			pokerOptionTexts_[1]->SetPosition({
				pokerEffectLayout_.effectTexts[0].x,
				pokerEffectLayout_.effectTexts[0].y
				});

			pokerOptionTexts_[2]->SetText(L"ドロー");
			pokerOptionTexts_[2]->SetPosition({
				pokerEffectLayout_.effectTexts[1].x,
				pokerEffectLayout_.effectTexts[1].y
				});

			pokerOptionTexts_[3]->SetText(L"ダメージ");
			pokerOptionTexts_[3]->SetPosition({
				pokerEffectLayout_.effectTexts[2].x,
				pokerEffectLayout_.effectTexts[2].y
				});
		}

		for (int i = 0; i < 4; ++i) {
			if (i == pokerHoverIndex_) {
				if (battle.IsWaitingEffectChoice() && i == 0) {
					pokerOptionTexts_[i]->SetSize({ 1.20f, 1.20f, 1.0f });
				} else {
					pokerOptionTexts_[i]->SetSize({ 1.06f, 1.06f, 1.0f });
				}
			} else {
				if (battle.IsWaitingEffectChoice() && i == 0) {
					pokerOptionTexts_[i]->SetSize({ 1.15f, 1.15f, 1.0f });
				} else {
					pokerOptionTexts_[i]->SetSize({ 1.0f, 1.0f, 1.0f });
				}
			}
		}
	} else {
		cardDescText_->SetSize({ 1.0f, 1.0f, 1.0f });
		cardDescText_->SetPosition({ 40.0f, 620.0f });

		const CardDef* def = battle.GetPreviewCardDef();
		if (def) {
			newMode = DescMode::CardDesc;
			newPreviewDefId = def->id;
			showDescBg_ = true;
			newText = Utf8ToWString_(def->desc);

			cardDescBg_->SetPosition({ 20.0f, 600.0f });
			cardDescBg_->SetScale({ 900.0f, 120.0f, 1.0f });
		} else if (battle.ShouldShowOperationUi()) {
			newMode = DescMode::Operation;
			showDescBg_ = true;
			newText = battle.GetOperationUiText();

			cardDescText_->SetPosition({ 40.0f, 520.0f });
			cardDescBg_->SetPosition({ 20.0f, 500.0f });
			cardDescBg_->SetScale({ 900.0f, 280.0f, 1.0f });
		}
	}

	if (newMode == DescMode::None) {
		if (lastDescMode_ != DescMode::None) {
			cardDescText_->SetText(L"");
			lastDescMode_ = DescMode::None;
			lastPreviewDefId_ = -1;
			lastDescText_.clear();
		}
	} else {
		const bool modeChanged = (newMode != lastDescMode_);
		const bool defChanged = (newPreviewDefId != lastPreviewDefId_);
		const bool textChanged = (newText != lastDescText_);

		if (modeChanged || defChanged || textChanged) {
			cardDescText_->SetText(newText);
			lastDescMode_ = newMode;
			lastPreviewDefId_ = newPreviewDefId;
			lastDescText_ = newText;
		}
	}

	deckCountText_->SetText(L"山札:" + std::to_wstring(battle.GetDeckCount()));
	discardCountText_->SetText(L"墓地:" + std::to_wstring(battle.GetDiscardCount()));
	handCountText_->SetText(L"手札:" + std::to_wstring(battle.GetHandCount()));
	fieldCountText_->SetText(battle.GetCurrentPokerHandUiText());
}

void FieldUi::Draw(GameApp& app)
{
	app.SpriteCom()->SetGraphicsPipelineState();

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
		0, 0,
		float(WinApp::kClientWidth),
		float(WinApp::kClientHeight),
		0, 100
	);

	if (showDescBg_ && cardDescBg_ && !showPokerOptions_) {
		cardDescBg_->Update(view, proj);
		cardDescBg_->Draw();
	}

	if (deckCountBg_) {
		deckCountBg_->Update(view, proj);
		deckCountBg_->Draw();
	}
	if (discardCountBg_) {
		discardCountBg_->Update(view, proj);
		discardCountBg_->Draw();
	}
	if (handCountBg_) {
		handCountBg_->Update(view, proj);
		handCountBg_->Draw();
	}
	if (fieldCountBg_) {
		fieldCountBg_->Update(view, proj);
		fieldCountBg_->Draw();
	}

	if (showPokerOptions_) {

		if (pokerOptionCount_ == 2) {
			if (cardDescBg_) {
				cardDescBg_->SetPosition({ 380.0f, 140.0f });
				cardDescBg_->SetScale({ 520.0f, 120.0f, 1.0f });
				cardDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.88f });
				cardDescBg_->Update(view, proj);
				cardDescBg_->Draw();
			}

			if (pokerOptionBgs_[0]) {
				pokerOptionBgs_[0]->SetColor(
					pokerHoverIndex_ == 0 ?
					Vector4{ 0.15f, 0.15f, 0.15f, 0.95f } :
					Vector4{ 0.0f, 0.0f, 0.0f, 0.78f }
				);
				pokerOptionBgs_[0]->SetPosition({ 120.0f, 430.0f });
				pokerOptionBgs_[0]->SetScale({ 360.0f, 120.0f, 1.0f });
				pokerOptionBgs_[0]->Update(view, proj);
				pokerOptionBgs_[0]->Draw();
			}

			if (pokerOptionBgs_[1]) {
				pokerOptionBgs_[1]->SetColor(
					pokerHoverIndex_ == 1 ?
					Vector4{ 0.15f, 0.15f, 0.15f, 0.95f } :
					Vector4{ 0.0f, 0.0f, 0.0f, 0.78f }
				);
				pokerOptionBgs_[1]->SetPosition({ 800.0f, 430.0f });
				pokerOptionBgs_[1]->SetScale({ 360.0f, 120.0f, 1.0f });
				pokerOptionBgs_[1]->Update(view, proj);
				pokerOptionBgs_[1]->Draw();
			}
		}

		// 効果3択UI
		if (pokerOptionCount_ == 4) {
			if (pokerOptionBgs_[0]) {
				pokerOptionBgs_[0]->SetColor(
					pokerHoverIndex_ == 0 ?
					Vector4{ 0.18f, 0.18f, 0.18f, 0.96f } :
					Vector4{ 0.0f, 0.0f, 0.0f, 0.78f }
				);
				pokerOptionBgs_[0]->SetPosition({
					pokerEffectLayout_.backRect.x,
					pokerEffectLayout_.backRect.y
					});
				pokerOptionBgs_[0]->SetScale({
					pokerEffectLayout_.backRect.w,
					pokerEffectLayout_.backRect.h,
					1.0f
					});
				pokerOptionBgs_[0]->Update(view, proj);
				pokerOptionBgs_[0]->Draw();
			}

			for (int i = 0; i < 3; ++i) {
				if (pokerOptionBgs_[i + 1]) {
					pokerOptionBgs_[i + 1]->SetColor(
						pokerHoverIndex_ == (i + 1) ?
						Vector4{ 0.18f, 0.18f, 0.18f, 0.96f } :
						Vector4{ 0.0f, 0.0f, 0.0f, 0.78f }
					);
					pokerOptionBgs_[i + 1]->SetPosition({
						pokerEffectLayout_.effectRects[i].x,
						pokerEffectLayout_.effectRects[i].y
						});
					pokerOptionBgs_[i + 1]->SetScale({
						pokerEffectLayout_.effectRects[i].w,
						pokerEffectLayout_.effectRects[i].h,
						1.0f
						});
					pokerOptionBgs_[i + 1]->Update(view, proj);
					pokerOptionBgs_[i + 1]->Draw();
				}
			}
		}

		if (pokerTitleText_) {
			pokerTitleText_->Update(view, proj);
			pokerTitleText_->Draw();
		}

		for (int i = 0; i < pokerOptionCount_; ++i) {
			if (pokerOptionTexts_[i]) {
				pokerOptionTexts_[i]->Update(view, proj);
				pokerOptionTexts_[i]->Draw();
			}
		}
	}

	if (!showPokerOptions_ && cardDescText_) {
		cardDescText_->Update(view, proj);
		cardDescText_->Draw();
	}

	if (deckCountText_) {
		deckCountText_->Update(view, proj);
		deckCountText_->Draw();
	}
	if (discardCountText_) {
		discardCountText_->Update(view, proj);
		discardCountText_->Draw();
	}
	if (handCountText_) {
		handCountText_->Update(view, proj);
		handCountText_->Draw();
	}
	if (fieldCountText_) {
		fieldCountText_->Update(view, proj);
		fieldCountText_->Draw();
	}
}


#ifdef USE_IMGUI
void FieldUi::DrawImGui()
{
	if (!ImGui::TreeNode("PokerEffectChoiceLayout")) {
		return;
	}

	ImGui::DragFloat2("Title", &pokerEffectLayout_.titleText.x, 1.0f);

	ImGui::Separator();
	ImGui::Text("Back");
	ImGui::DragFloat4("Back Rect", &pokerEffectLayout_.backRect.x, 1.0f);
	ImGui::DragFloat2("Back Text", &pokerEffectLayout_.backText.x, 1.0f);

	ImGui::Separator();
	ImGui::Text("Effect1");
	ImGui::DragFloat4("Effect1 Rect", &pokerEffectLayout_.effectRects[0].x, 1.0f);
	ImGui::DragFloat2("Effect1 Text", &pokerEffectLayout_.effectTexts[0].x, 1.0f);

	ImGui::Separator();
	ImGui::Text("Effect2");
	ImGui::DragFloat4("Effect2 Rect", &pokerEffectLayout_.effectRects[1].x, 1.0f);
	ImGui::DragFloat2("Effect2 Text", &pokerEffectLayout_.effectTexts[1].x, 1.0f);

	ImGui::Separator();
	ImGui::Text("Effect3");
	ImGui::DragFloat4("Effect3 Rect", &pokerEffectLayout_.effectRects[2].x, 1.0f);
	ImGui::DragFloat2("Effect3 Text", &pokerEffectLayout_.effectTexts[2].x, 1.0f);

	ImGui::Separator();
	if (ImGui::Button("Save PokerEffectChoiceLayout")) {
		SavePokerEffectChoiceLayout(pokerEffectLayoutPath_);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load PokerEffectChoiceLayout")) {
		LoadPokerEffectChoiceLayout(pokerEffectLayoutPath_);
	}

	ImGui::TreePop();
}
#endif

bool FieldUi::SavePokerEffectChoiceLayout(const std::string& path) const
{
	nlohmann::json j;

	j["title"]["x"] = pokerEffectLayout_.titleText.x;
	j["title"]["y"] = pokerEffectLayout_.titleText.y;

	j["backButton"]["rect"]["x"] = pokerEffectLayout_.backRect.x;
	j["backButton"]["rect"]["y"] = pokerEffectLayout_.backRect.y;
	j["backButton"]["rect"]["w"] = pokerEffectLayout_.backRect.w;
	j["backButton"]["rect"]["h"] = pokerEffectLayout_.backRect.h;
	j["backButton"]["text"]["x"] = pokerEffectLayout_.backText.x;
	j["backButton"]["text"]["y"] = pokerEffectLayout_.backText.y;

	for (int i = 0; i < 3; ++i) {
		std::string key = "effect" + std::to_string(i + 1);

		j[key]["rect"]["x"] = pokerEffectLayout_.effectRects[i].x;
		j[key]["rect"]["y"] = pokerEffectLayout_.effectRects[i].y;
		j[key]["rect"]["w"] = pokerEffectLayout_.effectRects[i].w;
		j[key]["rect"]["h"] = pokerEffectLayout_.effectRects[i].h;

		j[key]["text"]["x"] = pokerEffectLayout_.effectTexts[i].x;
		j[key]["text"]["y"] = pokerEffectLayout_.effectTexts[i].y;
	}

	std::ofstream ofs(path);
	if (!ofs.is_open()) {
		return false;
	}

	ofs << j.dump(4);
	return true;
}

bool FieldUi::LoadPokerEffectChoiceLayout(const std::string& path)
{
	std::ifstream ifs(path);
	if (!ifs.is_open()) {
		pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();
		return false;
	}

	nlohmann::json j;
	try {
		ifs >> j;
	} catch (...) {
		pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();
		return false;
	}

	pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();

	pokerEffectLayout_.titleText.x = j.value("title", nlohmann::json::object()).value("x", pokerEffectLayout_.titleText.x);
	pokerEffectLayout_.titleText.y = j.value("title", nlohmann::json::object()).value("y", pokerEffectLayout_.titleText.y);

	if (j.contains("backButton")) {
		auto& b = j["backButton"];
		if (b.contains("rect")) {
			pokerEffectLayout_.backRect.x = b["rect"].value("x", pokerEffectLayout_.backRect.x);
			pokerEffectLayout_.backRect.y = b["rect"].value("y", pokerEffectLayout_.backRect.y);
			pokerEffectLayout_.backRect.w = b["rect"].value("w", pokerEffectLayout_.backRect.w);
			pokerEffectLayout_.backRect.h = b["rect"].value("h", pokerEffectLayout_.backRect.h);
		}
		if (b.contains("text")) {
			pokerEffectLayout_.backText.x = b["text"].value("x", pokerEffectLayout_.backText.x);
			pokerEffectLayout_.backText.y = b["text"].value("y", pokerEffectLayout_.backText.y);
		}
	}

	for (int i = 0; i < 3; ++i) {
		std::string key = "effect" + std::to_string(i + 1);
		if (!j.contains(key)) {
			continue;
		}

		auto& e = j[key];
		if (e.contains("rect")) {
			pokerEffectLayout_.effectRects[i].x = e["rect"].value("x", pokerEffectLayout_.effectRects[i].x);
			pokerEffectLayout_.effectRects[i].y = e["rect"].value("y", pokerEffectLayout_.effectRects[i].y);
			pokerEffectLayout_.effectRects[i].w = e["rect"].value("w", pokerEffectLayout_.effectRects[i].w);
			pokerEffectLayout_.effectRects[i].h = e["rect"].value("h", pokerEffectLayout_.effectRects[i].h);
		}
		if (e.contains("text")) {
			pokerEffectLayout_.effectTexts[i].x = e["text"].value("x", pokerEffectLayout_.effectTexts[i].x);
			pokerEffectLayout_.effectTexts[i].y = e["text"].value("y", pokerEffectLayout_.effectTexts[i].y);
		}
	}

	return true;
}