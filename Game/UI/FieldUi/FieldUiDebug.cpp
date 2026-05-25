#include "../FieldUi.h"
#include "GameApp.h"
#include "TextSprite.h"
#include "Sprite.h"
#include "WinApp.h"
#include "CardDatabase.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

using json = nlohmann::json;
#ifdef USE_IMGUI
void FieldUi::DrawImGui()
{
	if (ImGui::TreeNode("PokerEffectChoiceLayout")) {
		bool changed = false;

		changed |= ImGui::DragFloat2("Title Image", &pokerEffectLayout_.titleImage.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("Activate Choice");
		changed |= ImGui::DragFloat4("Activate Title Bg", &pokerEffectLayout_.activateTitleBg.x, 1.0f);
		changed |= ImGui::DragFloat4("Yes Rect", &pokerEffectLayout_.activateYesRect.x, 1.0f);
		changed |= ImGui::DragFloat2("Yes Image", &pokerEffectLayout_.activateYesImage.x, 1.0f);
		changed |= ImGui::DragFloat4("No Rect", &pokerEffectLayout_.activateNoRect.x, 1.0f);
		changed |= ImGui::DragFloat2("No Image", &pokerEffectLayout_.activateNoImage.x, 1.0f);
		changed |= ImGui::DragFloat4("ViewBoard Rect", &pokerEffectLayout_.activateViewBoardRect.x, 1.0f);
		changed |= ImGui::DragFloat2("ViewBoard Image", &pokerEffectLayout_.activateViewBoardImage.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("Effect Choice");
		changed |= ImGui::DragFloat4("Effect Title Bg", &pokerEffectLayout_.effectTitleBg.x, 1.0f);

		changed |= ImGui::DragFloat4("Back Rect", &pokerEffectLayout_.backRect.x, 1.0f);
		changed |= ImGui::DragFloat2("Back Image", &pokerEffectLayout_.backImage.x, 1.0f);

		changed |= ImGui::DragFloat4("Effect1 Rect", &pokerEffectLayout_.effectRects[0].x, 1.0f);
		changed |= ImGui::DragFloat2("Effect1 Image", &pokerEffectLayout_.effectImages[0].x, 1.0f);

		changed |= ImGui::DragFloat4("Effect2 Rect", &pokerEffectLayout_.effectRects[1].x, 1.0f);
		changed |= ImGui::DragFloat2("Effect2 Image", &pokerEffectLayout_.effectImages[1].x, 1.0f);

		changed |= ImGui::DragFloat4("Effect3 Rect", &pokerEffectLayout_.effectRects[2].x, 1.0f);
		changed |= ImGui::DragFloat2("Effect3 Image", &pokerEffectLayout_.effectImages[2].x, 1.0f);

		changed |= ImGui::DragFloat4("Effect ViewBoard Rect", &pokerEffectLayout_.effectViewBoardRect.x, 1.0f);
		changed |= ImGui::DragFloat2("Effect ViewBoard Image", &pokerEffectLayout_.effectViewBoardImage.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("InfoButton");
		changed |= ImGui::DragFloat4("InfoButton Rect", &pokerEffectLayout_.infoButtonRect.x, 1.0f);
		changed |= ImGui::DragFloat3("InfoButton Image", &pokerEffectLayout_.infoButtonImage.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("PreviewPanel");
		changed |= ImGui::DragFloat4("PreviewPanel Bg", &pokerEffectLayout_.previewPanelBg.x, 1.0f);
		changed |= ImGui::DragFloat3("PreviewPanel Title Image", &pokerEffectLayout_.previewPanelTitleImage.x, 1.0f);
		changed |= ImGui::DragFloat3("PreviewPanel Text", &pokerEffectLayout_.previewPanelText.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("PreviewPanel Images");

		changed |= ImGui::DragFloat2("Preview Rank Pos", &pokerEffectLayout_.previewImages.rank.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Rank Scale", &pokerEffectLayout_.previewImages.rank.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat2("Preview ATK Label Pos", &pokerEffectLayout_.previewImages.atkLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview ATK Label Scale", &pokerEffectLayout_.previewImages.atkLabel.scale, 0.01f, 0.1f, 5.0f);
		changed |= ImGui::DragFloat2("Preview ATK Value Pos", &pokerEffectLayout_.previewImages.atkValue.x, 1.0f);
		changed |= ImGui::DragFloat("Preview ATK Value Scale", &pokerEffectLayout_.previewImages.atkValue.scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Preview ATK Value Spacing", &pokerEffectLayout_.previewImages.atkValue.spacing, 1.0f, 0.0f, 200.0f);

		changed |= ImGui::DragFloat2("Preview Draw Label Pos", &pokerEffectLayout_.previewImages.drawLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Draw Label Scale", &pokerEffectLayout_.previewImages.drawLabel.scale, 0.01f, 0.1f, 5.0f);
		changed |= ImGui::DragFloat2("Preview Draw Value Pos", &pokerEffectLayout_.previewImages.drawValue.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Draw Value Scale", &pokerEffectLayout_.previewImages.drawValue.scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Preview Draw Value Spacing", &pokerEffectLayout_.previewImages.drawValue.spacing, 1.0f, 0.0f, 200.0f);

		changed |= ImGui::DragFloat2("Preview Damage Label Pos", &pokerEffectLayout_.previewImages.damageLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Damage Label Scale", &pokerEffectLayout_.previewImages.damageLabel.scale, 0.01f, 0.1f, 5.0f);
		changed |= ImGui::DragFloat2("Preview Damage Value Pos", &pokerEffectLayout_.previewImages.damageValue.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Damage Value Scale", &pokerEffectLayout_.previewImages.damageValue.scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Preview Damage Value Spacing", &pokerEffectLayout_.previewImages.damageValue.spacing, 1.0f, 0.0f, 200.0f);

		changed |= ImGui::DragFloat2("Preview TurnStart Pos", &pokerEffectLayout_.previewImages.turnStartLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview TurnStart Scale", &pokerEffectLayout_.previewImages.turnStartLabel.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat2("Preview TurnStart None Pos", &pokerEffectLayout_.previewImages.turnStartNoneLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview TurnStart None Scale", &pokerEffectLayout_.previewImages.turnStartNoneLabel.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat2("Preview Activated Pos", &pokerEffectLayout_.previewImages.activatedLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Activated Scale", &pokerEffectLayout_.previewImages.activatedLabel.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat2("Preview Activated None Pos", &pokerEffectLayout_.previewImages.activatedNoneLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Activated None Scale", &pokerEffectLayout_.previewImages.activatedNoneLabel.scale, 0.01f, 0.1f, 5.0f);

		auto DrawLinesEditor = [&](const char* title, UiPokerPreviewLinesLayout& lines, const char* suffix) {
			if (ImGui::TreeNode(title)) {
				for (int i = 0; i < 5; ++i) {
					std::string label = "Lane" + std::to_string(i + 1) + "##" + suffix;
					changed |= ImGui::DragFloat2(label.c_str(), &lines.lanes[i].x, 1.0f);
				}
				ImGui::TreePop();
			}
			};

		auto DrawEffectAnchorsEditor = [&](const char* title, UiPokerPreviewEffectAnchors& anchors, const char* suffix) {
			if (ImGui::TreeNode(title)) {
				DrawLinesEditor("SingleDamage", anchors.singleDamage, (std::string(suffix) + "_sd").c_str());
				DrawLinesEditor("AllDamage", anchors.allDamage, (std::string(suffix) + "_ad").c_str());
				DrawLinesEditor("Draw", anchors.draw, (std::string(suffix) + "_dr").c_str());
				DrawLinesEditor("Block", anchors.block, (std::string(suffix) + "_bl").c_str());
				DrawLinesEditor("Heal", anchors.heal, (std::string(suffix) + "_he").c_str());
				DrawLinesEditor("None", anchors.none, (std::string(suffix) + "_no").c_str());
				ImGui::TreePop();
			}
			};

		ImGui::Separator();
		ImGui::Text("Preview Effect Anchors");

		DrawEffectAnchorsEditor(
			"TurnStart Effect Anchors",
			pokerEffectLayout_.previewImages.turnStartEffectAnchors,
			"ts_anchor");

		DrawEffectAnchorsEditor(
			"Activated Effect Anchors",
			pokerEffectLayout_.previewImages.activatedEffectAnchors,
			"ac_anchor");

		auto DrawPatternEditor = [&](const char* name, UiPokerPreviewPatternLayout& pat, const char* suffix) {
			if (ImGui::TreeNode(name)) {
				changed |= ImGui::DragFloat(("LabelScale##" + std::string(suffix)).c_str(), &pat.labelScale, 0.01f);
				changed |= ImGui::DragFloat2(("PrefixOffset##" + std::string(suffix)).c_str(), &pat.prefixOffsetX, 1.0f);
				changed |= ImGui::DragFloat2(("NumberOffset##" + std::string(suffix)).c_str(), &pat.numberOffsetX, 1.0f);
				changed |= ImGui::DragFloat(("NumberScale##" + std::string(suffix)).c_str(), &pat.numberScale, 0.01f);
				changed |= ImGui::DragFloat(("NumberSpacing##" + std::string(suffix)).c_str(), &pat.numberSpacing, 1.0f);
				changed |= ImGui::DragFloat2(("SuffixOffset##" + std::string(suffix)).c_str(), &pat.suffixOffsetX, 1.0f);
				ImGui::TreePop();
			}
			};

		ImGui::Separator();
		ImGui::Text("TurnStart Patterns");
		DrawPatternEditor("SingleDamage TS", pokerEffectLayout_.previewImages.turnStartPatterns.singleDamage, "ts_sd");
		DrawPatternEditor("AllDamage TS", pokerEffectLayout_.previewImages.turnStartPatterns.allDamage, "ts_ad");
		DrawPatternEditor("Draw TS", pokerEffectLayout_.previewImages.turnStartPatterns.draw, "ts_dr");
		DrawPatternEditor("Block TS", pokerEffectLayout_.previewImages.turnStartPatterns.block, "ts_bl");
		DrawPatternEditor("Heal TS", pokerEffectLayout_.previewImages.turnStartPatterns.heal, "ts_he");

		ImGui::Separator();
		ImGui::Text("Activated Patterns");
		DrawPatternEditor("SingleDamage AC", pokerEffectLayout_.previewImages.activatedPatterns.singleDamage, "ac_sd");
		DrawPatternEditor("AllDamage AC", pokerEffectLayout_.previewImages.activatedPatterns.allDamage, "ac_ad");
		DrawPatternEditor("Draw AC", pokerEffectLayout_.previewImages.activatedPatterns.draw, "ac_dr");
		DrawPatternEditor("Block AC", pokerEffectLayout_.previewImages.activatedPatterns.block, "ac_bl");
		DrawPatternEditor("Heal AC", pokerEffectLayout_.previewImages.activatedPatterns.heal, "ac_he");

		if (ImGui::Button("Save PokerEffectChoiceLayout")) {
			SavePokerEffectChoiceLayout(pokerEffectLayoutPath_);
		}
		ImGui::SameLine();
		if (ImGui::Button("Load PokerEffectChoiceLayout")) {
			LoadPokerEffectChoiceLayout(pokerEffectLayoutPath_);
		}


		ImGui::Separator();
		ImGui::Text("Activate Choice Text");

		changed |= ImGui::DragFloat3("ActivateTitleText", &pokerEffectLayout_.activateTitleText.x, 1.0f);
		changed |= ImGui::DragFloat3("ActivateYesText", &pokerEffectLayout_.activateYesText.x, 1.0f);
		changed |= ImGui::DragFloat3("ActivateNoText", &pokerEffectLayout_.activateNoText.x, 1.0f);
		changed |= ImGui::DragFloat3("ActivateViewBoardText", &pokerEffectLayout_.activateViewBoardText.x, 1.0f);
		changed |= ImGui::DragFloat3("InfoButtonText", &pokerEffectLayout_.infoButtonText.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("Effect Choice Text");

		changed |= ImGui::DragFloat3("EffectTitleText", &pokerEffectLayout_.effectTitleText.x, 1.0f);
		changed |= ImGui::DragFloat3("BackText", &pokerEffectLayout_.backText.x, 1.0f);
		changed |= ImGui::DragFloat3("EffectText0", &pokerEffectLayout_.effectTexts[0].x, 1.0f);
		changed |= ImGui::DragFloat3("EffectText1", &pokerEffectLayout_.effectTexts[1].x, 1.0f);
		changed |= ImGui::DragFloat3("EffectText2", &pokerEffectLayout_.effectTexts[2].x, 1.0f);
		changed |= ImGui::DragFloat3("EffectViewBoardText", &pokerEffectLayout_.effectViewBoardText.x, 1.0f);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("FieldUiLayout")) {
		bool changed = false;

		changed |= ImGui::DragFloat4("deckBg", &layout_.deckBg.x, 1.0f);
		changed |= ImGui::DragFloat3("deckText", &layout_.deckText.x, 1.0f);
		changed |= ImGui::DragFloat2("deckLabelPos", &layout_.deckLabelImage.x, 1.0f);
		changed |= ImGui::DragFloat("deckLabelScale", &layout_.deckLabelImage.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat4("discardBg", &layout_.discardBg.x, 1.0f);
		changed |= ImGui::DragFloat3("discardText", &layout_.discardText.x, 1.0f);
		changed |= ImGui::DragFloat2("discardLabelPos", &layout_.discardLabelImage.x, 1.0f);
		changed |= ImGui::DragFloat("discardLabelScale", &layout_.discardLabelImage.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat4("handBg", &layout_.handBg.x, 1.0f);
		changed |= ImGui::DragFloat3("handText", &layout_.handText.x, 1.0f);
		changed |= ImGui::DragFloat2("handLabelPos", &layout_.handLabelImage.x, 1.0f);
		changed |= ImGui::DragFloat("handLabelScale", &layout_.handLabelImage.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat4("fieldBg", &layout_.fieldBg.x, 1.0f);
		changed |= ImGui::DragFloat3("fieldText", &layout_.fieldText.x, 1.0f);

		changed |= ImGui::DragFloat4("turnBg", &layout_.turnBg.x, 1.0f);
		changed |= ImGui::DragFloat3("turnText", &layout_.turnText.x, 1.0f);

		changed |= ImGui::DragFloat4("costBg", &layout_.costBg.x, 1.0f);
		changed |= ImGui::DragFloat3("costText", &layout_.costText.x, 1.0f);

		changed |= ImGui::DragFloat4("endTurnBg", &layout_.endTurnBg.x, 1.0f);
		changed |= ImGui::DragFloat3("endTurnText", &layout_.endTurnText.x, 1.0f);

		changed |= ImGui::DragFloat4("overlay", &layout_.overlay.x, 1.0f);

		if (changed) {
			ApplyFieldUiLayout_();
		}

		if (ImGui::Button("Save FieldUiLayout")) {
			SaveFieldUiLayout(layoutPath_);
		}
		ImGui::SameLine();
		if (ImGui::Button("Load FieldUiLayout")) {
			LoadFieldUiLayout(layoutPath_);
			ApplyFieldUiLayout_();
		}

		ImGui::Separator();
		ImGui::Text("=== Count Text Layout ===");

		bool countChanged = false;

		// デッキ
		ImGui::Text("Deck");
		countChanged |= ImGui::DragFloat2("Deck Offset", &deckCountTextLayout_.offsetX, 1.0f);
		countChanged |= ImGui::DragFloat("Deck Scale", &deckCountTextLayout_.scale, 0.01f, 0.1f, 5.0f);

		// 墓地
		ImGui::Text("Discard");
		countChanged |= ImGui::DragFloat2("Discard Offset", &discardCountTextLayout_.offsetX, 1.0f);
		countChanged |= ImGui::DragFloat("Discard Scale", &discardCountTextLayout_.scale, 0.01f, 0.1f, 5.0f);

		// 手札
		ImGui::Text("Hand");
		countChanged |= ImGui::DragFloat2("Hand Offset", &handCountTextLayout_.offsetX, 1.0f);
		countChanged |= ImGui::DragFloat("Hand Scale", &handCountTextLayout_.scale, 0.01f, 0.1f, 5.0f);

		if (countChanged) {
			ApplyFieldUiLayout_();
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("CardShowUiLayout")) {
		bool changed = false;

		changed |= ImGui::DragFloat4("cardDescBg", &layout_.cardDescBg.x, 1.0f);
		changed |= ImGui::DragFloat2("cardDescText Pos", &layout_.cardDescText.x, 1.0f);
		changed |= ImGui::DragFloat("cardDescText Scale", &layout_.cardDescText.scale, 0.1f);

		ImGui::Separator();
		ImGui::Text("CardDescCustomLayout");

		static bool editDefaultLayout = false;
		ImGui::Checkbox("Edit Default Layout", &editDefaultLayout);

		ImGui::BeginDisabled();
		ImGui::DragInt("Edit Card ID", &editCardId_, 1.0f, 1, 999);
		ImGui::EndDisabled();
		ImGui::Text("※ Title Debug の Debug Card ID と連動");

		UiCardDescCustomLayout* editLayoutPtr = nullptr;

		if (editDefaultLayout) {
			editLayoutPtr = &layout_.cardDescCustom;
			ImGui::Text("Editing : Default Layout");
		} else {
			editLayoutPtr = &GetOrCreateCardDescCustomLayout_(editCardId_);
			ImGui::Text("Editing : Card ID = %d", editCardId_);

			bool hasPerCard = perCardDescCustomLayouts_.find(editCardId_) != perCardDescCustomLayouts_.end();
			ImGui::Text("Per Card Exists : %s", hasPerCard ? "Yes" : "No");

			if (hasPerCard) {
				if (ImGui::Button("Remove Per Card Layout")) {
					perCardDescCustomLayouts_.erase(editCardId_);
					editLayoutPtr = &GetOrCreateCardDescCustomLayout_(editCardId_);
					changed = true;
				}
			} else {
				ImGui::BeginDisabled();
				ImGui::Button("Remove Per Card Layout");
				ImGui::EndDisabled();
			}
		}

		UiCardDescCustomLayout& editLayout = *editLayoutPtr;

		ImGui::Separator();

		ImGui::Text("Custom Desc Image Layout");

		if (!editDefaultLayout && IsCustomDescCardId_(editCardId_)) {
			auto& customImage = GetOrCreateCustomDescImageLayout_(editCardId_);

			changed |= ImGui::DragFloat2("CustomDescImage Pos", &customImage.x, 1.0f);
			changed |= ImGui::DragFloat2("CustomDescImage Scale", &customImage.scaleX, 0.01f, 0.01f, 10.0f);

			ImGui::Text("Custom Desc Image Target Card ID = %d", editCardId_);
		} else {
			ImGui::BeginDisabled();
			static float dummyPos[2] = { 0.0f, 0.0f };
			static float dummyScale[2] = { 1.0f, 1.0f };
			ImGui::DragFloat2("CustomDescImage Pos", dummyPos, 1.0f);
			ImGui::DragFloat2("CustomDescImage Scale", dummyScale, 0.01f, 0.01f, 10.0f);
			ImGui::EndDisabled();

			if (editDefaultLayout) {
				ImGui::Text("※ Default Layout編集中は専用画像レイアウトは編集できません");
			} else {
				ImGui::Text("※ このCard IDは専用画像カードではありません");
			}
		}

		ImGui::Separator();

		ImGui::Text("titleBasicEffect");
		changed |= ImGui::DragFloat2("titleBasicEffect Pos", &editLayout.titleBasicEffect.x, 1.0f);
		changed |= ImGui::DragFloat("titleBasicEffect Scale", &editLayout.titleBasicEffect.scale, 0.01f, 0.1f, 10.0f);

		ImGui::Text("separatorCustom");
		changed |= ImGui::DragFloat2("separatorCustom Pos", &editLayout.separator.x, 1.0f);
		changed |= ImGui::DragFloat("separatorCustom Scale", &editLayout.separator.scale, 0.01f, 0.1f, 10.0f);

		for (int i = 0; i < 3; ++i) {
			std::string label = "BaseRow" + std::to_string(i);
			if (ImGui::TreeNode(label.c_str())) {
				ImGui::Text("target");
				changed |= ImGui::DragFloat2(("target Pos##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].target.x, 1.0f);
				changed |= ImGui::DragFloat(("target Scale##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].target.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("particle");
				changed |= ImGui::DragFloat2(("particle Pos##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].particle.x, 1.0f);
				changed |= ImGui::DragFloat(("particle Scale##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].particle.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("effectType");
				changed |= ImGui::DragFloat2(("effectType Pos##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].effectType.x, 1.0f);
				changed |= ImGui::DragFloat(("effectType Scale##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].effectType.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("value");
				changed |= ImGui::DragFloat2(("value Pos##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].value.x, 1.0f);
				changed |= ImGui::DragFloat(("value Scale##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].value.scale, 0.01f, 0.01f, 10.0f);
				changed |= ImGui::DragFloat(("value Spacing##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].value.spacing, 0.1f, 0.0f, 200.0f);

				ImGui::Text("special1");
				changed |= ImGui::DragFloat2(("special1 Pos##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].special1.x, 1.0f);
				changed |= ImGui::DragFloat(("special1 Scale##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].special1.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("special2");
				changed |= ImGui::DragFloat2(("special2 Pos##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].special2.x, 1.0f);
				changed |= ImGui::DragFloat(("special2 Scale##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].special2.scale, 0.01f, 0.1f, 10.0f);

				changed |= ImGui::DragFloat(("specialAdvance##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].specialAdvance, 1.0f, 0.0f, 1000.0f);

				ImGui::TreePop();
			}
		}

		for (int i = 0; i < 3; ++i) {
			std::string label = "SubBlock" + std::to_string(i);
			if (ImGui::TreeNode(label.c_str())) {
				ImGui::Text("trigger");
				changed |= ImGui::DragFloat2(("trigger Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].trigger.x, 1.0f);
				changed |= ImGui::DragFloat(("trigger Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].trigger.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("rank");
				changed |= ImGui::DragFloat2(("rank Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].rank.x, 1.0f);
				changed |= ImGui::DragFloat(("rank Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].rank.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("suffix");
				changed |= ImGui::DragFloat2(("suffix Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].suffix.x, 1.0f);
				changed |= ImGui::DragFloat(("suffix Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].suffix.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("target");
				changed |= ImGui::DragFloat2(("target Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].target.x, 1.0f);
				changed |= ImGui::DragFloat(("target Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].target.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("particle");
				changed |= ImGui::DragFloat2(("particle Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].particle.x, 1.0f);
				changed |= ImGui::DragFloat(("particle Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].particle.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("effectType");
				changed |= ImGui::DragFloat2(("effectType Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].effectType.x, 1.0f);
				changed |= ImGui::DragFloat(("effectType Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].effectType.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("value");
				changed |= ImGui::DragFloat2(("value Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].value.x, 1.0f);
				changed |= ImGui::DragFloat(("value Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].value.scale, 0.01f, 0.01f, 10.0f);
				changed |= ImGui::DragFloat(("value Spacing##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].value.spacing, 0.1f, 0.0f, 200.0f);

				ImGui::TreePop();
			}
		}

		if (changed) {
			ApplyFieldUiLayout_();
		}

		if (ImGui::Button("Save CardShowUiLayout")) {
			SaveCardShowUiLayout(cardShowLayoutPath_);
		}
		ImGui::SameLine();
		if (ImGui::Button("Load CardShowUiLayout")) {
			LoadCardShowUiLayout(cardShowLayoutPath_);
			ApplyFieldUiLayout_();
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("UiNumberLayout")) {
		bool changed = false;

		ImGui::Text("Deck Count");
		changed |= ImGui::DragFloat2("Deck Pos", &numberLayout_.deckCount.x, 1.0f);
		changed |= ImGui::DragFloat("Deck Scale", &numberLayout_.deckCount.scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Deck Spacing", &numberLayout_.deckCount.spacing, 1.0f, 0.0f, 200.0f);

		ImGui::Separator();
		ImGui::Text("Discard Count");
		changed |= ImGui::DragFloat2("Discard Pos", &numberLayout_.discardCount.x, 1.0f);
		changed |= ImGui::DragFloat("Discard Scale", &numberLayout_.discardCount.scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Discard Spacing", &numberLayout_.discardCount.spacing, 1.0f, 0.0f, 200.0f);

		ImGui::Separator();
		ImGui::Text("Hand Count");
		changed |= ImGui::DragFloat2("Hand Pos", &numberLayout_.handCount.x, 1.0f);
		changed |= ImGui::DragFloat("Hand Scale", &numberLayout_.handCount.scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Hand Spacing", &numberLayout_.handCount.spacing, 1.0f, 0.0f, 200.0f);

		ImGui::Separator();
		ImGui::Text("Effect1 Value");
		changed |= ImGui::DragFloat2("Effect1 Offset", &numberLayout_.effectValue[0].offsetX, 1.0f);
		changed |= ImGui::DragFloat("Effect1 Scale", &numberLayout_.effectValue[0].scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Effect1 Spacing", &numberLayout_.effectValue[0].spacing, 1.0f, 0.0f, 200.0f);

		ImGui::Separator();
		ImGui::Text("Effect2 Value");
		changed |= ImGui::DragFloat2("Effect2 Offset", &numberLayout_.effectValue[1].offsetX, 1.0f);
		changed |= ImGui::DragFloat("Effect2 Scale", &numberLayout_.effectValue[1].scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Effect2 Spacing", &numberLayout_.effectValue[1].spacing, 1.0f, 0.0f, 200.0f);

		ImGui::Separator();
		ImGui::Text("Effect3 Value");
		changed |= ImGui::DragFloat2("Effect3 Offset", &numberLayout_.effectValue[2].offsetX, 1.0f);
		changed |= ImGui::DragFloat("Effect3 Scale", &numberLayout_.effectValue[2].scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Effect3 Spacing", &numberLayout_.effectValue[2].spacing, 1.0f, 0.0f, 200.0f);

		if (ImGui::Button("Save UiNumberLayout")) {
			SaveUiNumberLayout(numberLayoutPath_);
		}
		ImGui::SameLine();
		if (ImGui::Button("Load UiNumberLayout")) {
			LoadUiNumberLayout(numberLayoutPath_);
		}

		ImGui::TreePop();
	}
}

#endif


