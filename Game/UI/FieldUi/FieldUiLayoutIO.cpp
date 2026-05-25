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
bool FieldUi::SavePokerEffectChoiceLayout(const std::string& path) const
{
	json j;

	auto writePattern = [&](json& dst, const UiPokerPreviewPatternLayout& pat) {
		dst["labelScale"] = pat.labelScale;
		dst["prefixOffsetX"] = pat.prefixOffsetX;
		dst["prefixOffsetY"] = pat.prefixOffsetY;
		dst["numberOffsetX"] = pat.numberOffsetX;
		dst["numberOffsetY"] = pat.numberOffsetY;
		dst["numberScale"] = pat.numberScale;
		dst["numberSpacing"] = pat.numberSpacing;
		dst["suffixOffsetX"] = pat.suffixOffsetX;
		dst["suffixOffsetY"] = pat.suffixOffsetY;
		dst["leadingLabelOffsetX"] = pat.leadingLabelOffsetX;
		dst["leadingLabelOffsetY"] = pat.leadingLabelOffsetY;
		dst["leadingAdvanceX"] = pat.leadingAdvanceX;
		};

	j["title"]["x"] = pokerEffectLayout_.titleImage.x;
	j["title"]["y"] = pokerEffectLayout_.titleImage.y;

	j["backButton"]["rect"]["x"] = pokerEffectLayout_.backRect.x;
	j["backButton"]["rect"]["y"] = pokerEffectLayout_.backRect.y;
	j["backButton"]["rect"]["w"] = pokerEffectLayout_.backRect.w;
	j["backButton"]["rect"]["h"] = pokerEffectLayout_.backRect.h;
	j["backButton"]["image"]["x"] = pokerEffectLayout_.backImage.x;
	j["backButton"]["image"]["y"] = pokerEffectLayout_.backImage.y;

	for (int i = 0; i < 3; ++i) {
		std::string key = "effect" + std::to_string(i + 1);

		j[key]["rect"]["x"] = pokerEffectLayout_.effectRects[i].x;
		j[key]["rect"]["y"] = pokerEffectLayout_.effectRects[i].y;
		j[key]["rect"]["w"] = pokerEffectLayout_.effectRects[i].w;
		j[key]["rect"]["h"] = pokerEffectLayout_.effectRects[i].h;

		j[key]["image"]["x"] = pokerEffectLayout_.effectImages[i].x;
		j[key]["image"]["y"] = pokerEffectLayout_.effectImages[i].y;
	}

	j["infoButton"]["rect"]["x"] = pokerEffectLayout_.infoButtonRect.x;
	j["infoButton"]["rect"]["y"] = pokerEffectLayout_.infoButtonRect.y;
	j["infoButton"]["rect"]["w"] = pokerEffectLayout_.infoButtonRect.w;
	j["infoButton"]["rect"]["h"] = pokerEffectLayout_.infoButtonRect.h;
	j["infoButton"]["image"]["x"] = pokerEffectLayout_.infoButtonImage.x;
	j["infoButton"]["image"]["y"] = pokerEffectLayout_.infoButtonImage.y;
	j["infoButton"]["image"]["scale"] = pokerEffectLayout_.infoButtonImage.scale;

	j["previewPanelBg"]["x"] = pokerEffectLayout_.
		previewPanelBg.x;
	j["previewPanelBg"]["y"] = pokerEffectLayout_.previewPanelBg.y;
	j["previewPanelBg"]["w"] = pokerEffectLayout_.previewPanelBg.w;
	j["previewPanelBg"]["h"] = pokerEffectLayout_.previewPanelBg.h;

	j["previewPanelTitle"]["x"] = pokerEffectLayout_.previewPanelTitleImage.x;
	j["previewPanelTitle"]["y"] = pokerEffectLayout_.previewPanelTitleImage.y;
	j["previewPanelTitle"]["scale"] = pokerEffectLayout_.previewPanelTitleImage.scale;

	j["previewPanelText"]["x"] = pokerEffectLayout_.previewPanelText.x;
	j["previewPanelText"]["y"] = pokerEffectLayout_.previewPanelText.y;
	j["previewPanelText"]["scale"] = pokerEffectLayout_.previewPanelText.scale;

	j["previewImages"]["rank"]["x"] = pokerEffectLayout_.previewImages.rank.x;
	j["previewImages"]["rank"]["y"] = pokerEffectLayout_.previewImages.rank.y;
	j["previewImages"]["rank"]["scale"] = pokerEffectLayout_.previewImages.rank.scale;

	j["previewImages"]["atkLabel"]["x"] = pokerEffectLayout_.previewImages.atkLabel.x;
	j["previewImages"]["atkLabel"]["y"] = pokerEffectLayout_.previewImages.atkLabel.y;
	j["previewImages"]["atkLabel"]["scale"] = pokerEffectLayout_.previewImages.atkLabel.scale;

	j["previewImages"]["atkValue"]["x"] = pokerEffectLayout_.previewImages.atkValue.x;
	j["previewImages"]["atkValue"]["y"] = pokerEffectLayout_.previewImages.atkValue.y;
	j["previewImages"]["atkValue"]["scale"] = pokerEffectLayout_.previewImages.atkValue.scale;
	j["previewImages"]["atkValue"]["spacing"] = pokerEffectLayout_.previewImages.atkValue.spacing;

	j["previewImages"]["drawLabel"]["x"] = pokerEffectLayout_.previewImages.drawLabel.x;
	j["previewImages"]["drawLabel"]["y"] = pokerEffectLayout_.previewImages.drawLabel.y;
	j["previewImages"]["drawLabel"]["scale"] = pokerEffectLayout_.previewImages.drawLabel.scale;

	j["previewImages"]["drawValue"]["x"] = pokerEffectLayout_.previewImages.drawValue.x;
	j["previewImages"]["drawValue"]["y"] = pokerEffectLayout_.previewImages.drawValue.y;
	j["previewImages"]["drawValue"]["scale"] = pokerEffectLayout_.previewImages.drawValue.scale;
	j["previewImages"]["drawValue"]["spacing"] = pokerEffectLayout_.previewImages.drawValue.spacing;

	j["previewImages"]["damageLabel"]["x"] = pokerEffectLayout_.previewImages.damageLabel.x;
	j["previewImages"]["damageLabel"]["y"] = pokerEffectLayout_.previewImages.damageLabel.y;
	j["previewImages"]["damageLabel"]["scale"] = pokerEffectLayout_.previewImages.damageLabel.scale;

	j["previewImages"]["damageValue"]["x"] = pokerEffectLayout_.previewImages.damageValue.x;
	j["previewImages"]["damageValue"]["y"] = pokerEffectLayout_.previewImages.damageValue.y;
	j["previewImages"]["damageValue"]["scale"] = pokerEffectLayout_.previewImages.damageValue.scale;
	j["previewImages"]["damageValue"]["spacing"] = pokerEffectLayout_.previewImages.damageValue.spacing;

	j["previewImages"]["turnStartLabel"]["x"] = pokerEffectLayout_.previewImages.turnStartLabel.x;
	j["previewImages"]["turnStartLabel"]["y"] = pokerEffectLayout_.previewImages.turnStartLabel.y;
	j["previewImages"]["turnStartLabel"]["scale"] = pokerEffectLayout_.previewImages.turnStartLabel.scale;

	j["previewImages"]["turnStartNoneLabel"]["x"] = pokerEffectLayout_.previewImages.turnStartNoneLabel.x;
	j["previewImages"]["turnStartNoneLabel"]["y"] = pokerEffectLayout_.previewImages.turnStartNoneLabel.y;
	j["previewImages"]["turnStartNoneLabel"]["scale"] = pokerEffectLayout_.previewImages.turnStartNoneLabel.scale;

	j["previewImages"]["activatedNoneLabel"]["x"] = pokerEffectLayout_.previewImages.activatedNoneLabel.x;
	j["previewImages"]["activatedNoneLabel"]["y"] = pokerEffectLayout_.previewImages.activatedNoneLabel.y;
	j["previewImages"]["activatedNoneLabel"]["scale"] = pokerEffectLayout_.previewImages.activatedNoneLabel.scale;

	

	writePattern(j["previewImages"]["turnStartPatterns"]["singleDamage"],
		pokerEffectLayout_.previewImages.turnStartPatterns.singleDamage);
	writePattern(j["previewImages"]["turnStartPatterns"]["allDamage"],
		pokerEffectLayout_.previewImages.turnStartPatterns.allDamage);
	writePattern(j["previewImages"]["turnStartPatterns"]["draw"],
		pokerEffectLayout_.previewImages.turnStartPatterns.draw);
	writePattern(j["previewImages"]["turnStartPatterns"]["block"],
		pokerEffectLayout_.previewImages.turnStartPatterns.block);
	writePattern(j["previewImages"]["turnStartPatterns"]["heal"],
		pokerEffectLayout_.previewImages.turnStartPatterns.heal);

	writePattern(j["previewImages"]["activatedPatterns"]["singleDamage"],
		pokerEffectLayout_.previewImages.activatedPatterns.singleDamage);
	writePattern(j["previewImages"]["activatedPatterns"]["allDamage"],
		pokerEffectLayout_.previewImages.activatedPatterns.allDamage);
	writePattern(j["previewImages"]["activatedPatterns"]["draw"],
		pokerEffectLayout_.previewImages.activatedPatterns.draw);
	writePattern(j["previewImages"]["activatedPatterns"]["block"],
		pokerEffectLayout_.previewImages.activatedPatterns.block);
	writePattern(j["previewImages"]["activatedPatterns"]["heal"],
		pokerEffectLayout_.previewImages.activatedPatterns.heal);

	auto writeLines = [&](json& dst, const UiPokerPreviewLinesLayout& lines) {
		for (int i = 0; i < 5; ++i) {
			dst[i]["x"] = lines.lanes[i].x;
			dst[i]["y"] = lines.lanes[i].y;
		}
		};

	auto writeEffectAnchors = [&](json& dst, const UiPokerPreviewEffectAnchors& a) {
		writeLines(dst["singleDamage"], a.singleDamage);
		writeLines(dst["allDamage"], a.allDamage);
		writeLines(dst["draw"], a.draw);
		writeLines(dst["block"], a.block);
		writeLines(dst["heal"], a.heal);
		writeLines(dst["none"], a.none);
		};

	writeEffectAnchors(
		j["previewImages"]["turnStartEffectAnchors"],
		pokerEffectLayout_.previewImages.turnStartEffectAnchors);

	writeEffectAnchors(
		j["previewImages"]["activatedEffectAnchors"],
		pokerEffectLayout_.previewImages.activatedEffectAnchors);

	j["previewImages"]["activatedLabel"]["x"] = pokerEffectLayout_.previewImages.activatedLabel.x;
	j["previewImages"]["activatedLabel"]["y"] = pokerEffectLayout_.previewImages.activatedLabel.y;
	j["previewImages"]["activatedLabel"]["scale"] = pokerEffectLayout_.previewImages.activatedLabel.scale;

	j["activateTitleBg"]["x"] = pokerEffectLayout_.activateTitleBg.x;
	j["activateTitleBg"]["y"] = pokerEffectLayout_.activateTitleBg.y;
	j["activateTitleBg"]["w"] = pokerEffectLayout_.activateTitleBg.w;
	j["activateTitleBg"]["h"] = pokerEffectLayout_.activateTitleBg.h;

	j["activateYes"]["rect"]["x"] = pokerEffectLayout_.activateYesRect.x;
	j["activateYes"]["rect"]["y"] = pokerEffectLayout_.activateYesRect.y;
	j["activateYes"]["rect"]["w"] = pokerEffectLayout_.activateYesRect.w;
	j["activateYes"]["rect"]["h"] = pokerEffectLayout_.activateYesRect.h;
	j["activateYes"]["image"]["x"] = pokerEffectLayout_.activateYesImage.x;
	j["activateYes"]["image"]["y"] = pokerEffectLayout_.activateYesImage.y;

	j["activateNo"]["rect"]["x"] = pokerEffectLayout_.activateNoRect.x;
	j["activateNo"]["rect"]["y"] = pokerEffectLayout_.activateNoRect.y;
	j["activateNo"]["rect"]["w"] = pokerEffectLayout_.activateNoRect.w;
	j["activateNo"]["rect"]["h"] = pokerEffectLayout_.activateNoRect.h;
	j["activateNo"]["image"]["x"] = pokerEffectLayout_.activateNoImage.x;
	j["activateNo"]["image"]["y"] = pokerEffectLayout_.activateNoImage.y;

	j["activateViewBoard"]["rect"]["x"] = pokerEffectLayout_.activateViewBoardRect.x;
	j["activateViewBoard"]["rect"]["y"] = pokerEffectLayout_.activateViewBoardRect.y;
	j["activateViewBoard"]["rect"]["w"] = pokerEffectLayout_.activateViewBoardRect.w;
	j["activateViewBoard"]["rect"]["h"] = pokerEffectLayout_.activateViewBoardRect.h;
	j["activateViewBoard"]["image"]["x"] = pokerEffectLayout_.activateViewBoardImage.x;
	j["activateViewBoard"]["image"]["y"] = pokerEffectLayout_.activateViewBoardImage.y;

	j["effectTitleBg"]["x"] = pokerEffectLayout_.effectTitleBg.x;
	j["effectTitleBg"]["y"] = pokerEffectLayout_.effectTitleBg.y;
	j["effectTitleBg"]["w"] = pokerEffectLayout_.effectTitleBg.w;
	j["effectTitleBg"]["h"] = pokerEffectLayout_.effectTitleBg.h;

	j["effectViewBoard"]["rect"]["x"] = pokerEffectLayout_.effectViewBoardRect.x;
	j["effectViewBoard"]["rect"]["y"] = pokerEffectLayout_.effectViewBoardRect.y;
	j["effectViewBoard"]["rect"]["w"] = pokerEffectLayout_.effectViewBoardRect.w;
	j["effectViewBoard"]["rect"]["h"] = pokerEffectLayout_.effectViewBoardRect.h;
	j["effectViewBoard"]["image"]["x"] = pokerEffectLayout_.effectViewBoardImage.x;
	j["effectViewBoard"]["image"]["y"] = pokerEffectLayout_.effectViewBoardImage.y;

	auto writeText = [&](json& dst, const UiText& t) {
		dst["x"] = t.x;
		dst["y"] = t.y;
		dst["scale"] = t.scale;
		};

	writeText(j["activateTitleText"], pokerEffectLayout_.activateTitleText);
	writeText(j["activateYesText"], pokerEffectLayout_.activateYesText);
	writeText(j["activateNoText"], pokerEffectLayout_.activateNoText);
	writeText(j["activateViewBoardText"], pokerEffectLayout_.activateViewBoardText);
	writeText(j["infoButtonText"], pokerEffectLayout_.infoButtonText);

	writeText(j["effectTitleText"], pokerEffectLayout_.effectTitleText);
	writeText(j["backText"], pokerEffectLayout_.backText);
	writeText(j["effectTexts"][0], pokerEffectLayout_.effectTexts[0]);
	writeText(j["effectTexts"][1], pokerEffectLayout_.effectTexts[1]);
	writeText(j["effectTexts"][2], pokerEffectLayout_.effectTexts[2]);
	writeText(j["effectViewBoardText"], pokerEffectLayout_.effectViewBoardText);

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

	json j;
	try {
		ifs >> j;
	} catch (...) {
		pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();
		return false;
	}

	pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();

	auto readPattern = [&](const json& src, UiPokerPreviewPatternLayout& pat) {
		pat.labelScale = src.value("labelScale", pat.labelScale);
		pat.prefixOffsetX = src.value("prefixOffsetX", pat.prefixOffsetX);
		pat.prefixOffsetY = src.value("prefixOffsetY", pat.prefixOffsetY);
		pat.numberOffsetX = src.value("numberOffsetX", pat.numberOffsetX);
		pat.numberOffsetY = src.value("numberOffsetY", pat.numberOffsetY);
		pat.numberScale = src.value("numberScale", pat.numberScale);
		pat.numberSpacing = src.value("numberSpacing", pat.numberSpacing);
		pat.suffixOffsetX = src.value("suffixOffsetX", pat.suffixOffsetX);
		pat.suffixOffsetY = src.value("suffixOffsetY", pat.suffixOffsetY);
		
		pat.leadingLabelOffsetX = src.value("leadingLabelOffsetX", pat.leadingLabelOffsetX);
		pat.leadingLabelOffsetY = src.value("leadingLabelOffsetY", pat.leadingLabelOffsetY);
		pat.leadingAdvanceX = src.value("leadingAdvanceX", pat.leadingAdvanceX);
		
		};

	auto readLines = [&](const json& src, UiPokerPreviewLinesLayout& lines) {
		if (!src.is_array()) return;

		for (int i = 0; i < static_cast<int>(src.size()) && i < 5; ++i) {
			lines.lanes[i].x = src[i].value("x", lines.lanes[i].x);
			lines.lanes[i].y = src[i].value("y", lines.lanes[i].y);
		}
		};

	auto readEffectAnchors = [&](const json& src, UiPokerPreviewEffectAnchors& a) {
		if (src.contains("singleDamage")) readLines(src["singleDamage"], a.singleDamage);
		if (src.contains("allDamage"))    readLines(src["allDamage"], a.allDamage);
		if (src.contains("draw"))         readLines(src["draw"], a.draw);
		if (src.contains("block"))        readLines(src["block"], a.block);
		if (src.contains("heal"))         readLines(src["heal"], a.heal);
		if (src.contains("none"))         readLines(src["none"], a.none);
		};

	// title
	pokerEffectLayout_.titleImage.x = j.value("title", json::object()).value("x", pokerEffectLayout_.titleImage.x);
	pokerEffectLayout_.titleImage.y = j.value("title", json::object()).value("y", pokerEffectLayout_.titleImage.y);

	// backButton
	if (j.contains("backButton")) {
		auto& b = j["backButton"];
		if (b.contains("rect")) {
			pokerEffectLayout_.backRect.x = b["rect"].value("x", pokerEffectLayout_.backRect.x);
			pokerEffectLayout_.backRect.y = b["rect"].value("y", pokerEffectLayout_.backRect.y);
			pokerEffectLayout_.backRect.w = b["rect"].value("w", pokerEffectLayout_.backRect.w);
			pokerEffectLayout_.backRect.h = b["rect"].value("h", pokerEffectLayout_.backRect.h);
		}
		if (b.contains("image")) {
			pokerEffectLayout_.backImage.x = b["image"].value("x", pokerEffectLayout_.backImage.x);
			pokerEffectLayout_.backImage.y = b["image"].value("y", pokerEffectLayout_.backImage.y);
		}
	}

	// effect1 ~ effect3
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
		if (e.contains("image")) {
			pokerEffectLayout_.effectImages[i].x = e["image"].value("x", pokerEffectLayout_.effectImages[i].x);
			pokerEffectLayout_.effectImages[i].y = e["image"].value("y", pokerEffectLayout_.effectImages[i].y);
		}
	}

	// infoButton
	if (j.contains("infoButton")) {
		auto& ib = j["infoButton"];
		if (ib.contains("rect")) {
			pokerEffectLayout_.infoButtonRect.x = ib["rect"].value("x", pokerEffectLayout_.infoButtonRect.x);
			pokerEffectLayout_.infoButtonRect.y = ib["rect"].value("y", pokerEffectLayout_.infoButtonRect.y);
			pokerEffectLayout_.infoButtonRect.w = ib["rect"].value("w", pokerEffectLayout_.infoButtonRect.w);
			pokerEffectLayout_.infoButtonRect.h = ib["rect"].value("h", pokerEffectLayout_.infoButtonRect.h);
		}
		if (ib.contains("image")) {
			pokerEffectLayout_.infoButtonImage.x = ib["image"].value("x", pokerEffectLayout_.infoButtonImage.x);
			pokerEffectLayout_.infoButtonImage.y = ib["image"].value("y", pokerEffectLayout_.infoButtonImage.y);
			pokerEffectLayout_.infoButtonImage.scale = ib["image"].value("scale", pokerEffectLayout_.infoButtonImage.scale);
		}
	}

	// previewPanelBg
	if (j.contains("previewPanelBg")) {
		pokerEffectLayout_.previewPanelBg.x = j["previewPanelBg"].value("x", pokerEffectLayout_.previewPanelBg.x);
		pokerEffectLayout_.previewPanelBg.y = j["previewPanelBg"].value("y", pokerEffectLayout_.previewPanelBg.y);
		pokerEffectLayout_.previewPanelBg.w = j["previewPanelBg"].value("w", pokerEffectLayout_.previewPanelBg.w);
		pokerEffectLayout_.previewPanelBg.h = j["previewPanelBg"].value("h", pokerEffectLayout_.previewPanelBg.h);
	}

	// previewPanelTitle
	if (j.contains("previewPanelTitle")) {
		pokerEffectLayout_.previewPanelTitleImage.x = j["previewPanelTitle"].value("x", pokerEffectLayout_.previewPanelTitleImage.x);
		pokerEffectLayout_.previewPanelTitleImage.y = j["previewPanelTitle"].value("y", pokerEffectLayout_.previewPanelTitleImage.y);
		pokerEffectLayout_.previewPanelTitleImage.scale = j["previewPanelTitle"].value("scale", pokerEffectLayout_.previewPanelTitleImage.scale);
	}

	// previewPanelText
	if (j.contains("previewPanelText")) {
		pokerEffectLayout_.previewPanelText.x = j["previewPanelText"].value("x", pokerEffectLayout_.previewPanelText.x);
		pokerEffectLayout_.previewPanelText.y = j["previewPanelText"].value("y", pokerEffectLayout_.previewPanelText.y);
		pokerEffectLayout_.previewPanelText.scale = j["previewPanelText"].value("scale", pokerEffectLayout_.previewPanelText.scale);
	}

	if (j.contains("previewImages")) {
		auto& p = j["previewImages"];

		if (p.contains("rank")) {
			pokerEffectLayout_.previewImages.rank.x = p["rank"].value("x", pokerEffectLayout_.previewImages.rank.x);
			pokerEffectLayout_.previewImages.rank.y = p["rank"].value("y", pokerEffectLayout_.previewImages.rank.y);
			pokerEffectLayout_.previewImages.rank.scale = p["rank"].value("scale", pokerEffectLayout_.previewImages.rank.scale);
		}

		if (p.contains("atkLabel")) {
			pokerEffectLayout_.previewImages.atkLabel.x = p["atkLabel"].value("x", pokerEffectLayout_.previewImages.atkLabel.x);
			pokerEffectLayout_.previewImages.atkLabel.y = p["atkLabel"].value("y", pokerEffectLayout_.previewImages.atkLabel.y);
			pokerEffectLayout_.previewImages.atkLabel.scale = p["atkLabel"].value("scale", pokerEffectLayout_.previewImages.atkLabel.scale);
		}
		if (p.contains("atkValue")) {
			pokerEffectLayout_.previewImages.atkValue.x = p["atkValue"].value("x", pokerEffectLayout_.previewImages.atkValue.x);
			pokerEffectLayout_.previewImages.atkValue.y = p["atkValue"].value("y", pokerEffectLayout_.previewImages.atkValue.y);
			pokerEffectLayout_.previewImages.atkValue.scale = p["atkValue"].value("scale", pokerEffectLayout_.previewImages.atkValue.scale);
			pokerEffectLayout_.previewImages.atkValue.spacing = p["atkValue"].value("spacing", pokerEffectLayout_.previewImages.atkValue.spacing);
		}

		if (p.contains("drawLabel")) {
			pokerEffectLayout_.previewImages.drawLabel.x = p["drawLabel"].value("x", pokerEffectLayout_.previewImages.drawLabel.x);
			pokerEffectLayout_.previewImages.drawLabel.y = p["drawLabel"].value("y", pokerEffectLayout_.previewImages.drawLabel.y);
			pokerEffectLayout_.previewImages.drawLabel.scale = p["drawLabel"].value("scale", pokerEffectLayout_.previewImages.drawLabel.scale);
		}
		if (p.contains("drawValue")) {
			pokerEffectLayout_.previewImages.drawValue.x = p["drawValue"].value("x", pokerEffectLayout_.previewImages.drawValue.x);
			pokerEffectLayout_.previewImages.drawValue.y = p["drawValue"].value("y", pokerEffectLayout_.previewImages.drawValue.y);
			pokerEffectLayout_.previewImages.drawValue.scale = p["drawValue"].value("scale", pokerEffectLayout_.previewImages.drawValue.scale);
			pokerEffectLayout_.previewImages.drawValue.spacing = p["drawValue"].value("spacing", pokerEffectLayout_.previewImages.drawValue.spacing);
		}

		if (p.contains("damageLabel")) {
			pokerEffectLayout_.previewImages.damageLabel.x = p["damageLabel"].value("x", pokerEffectLayout_.previewImages.damageLabel.x);
			pokerEffectLayout_.previewImages.damageLabel.y = p["damageLabel"].value("y", pokerEffectLayout_.previewImages.damageLabel.y);
			pokerEffectLayout_.previewImages.damageLabel.scale = p["damageLabel"].value("scale", pokerEffectLayout_.previewImages.damageLabel.scale);
		}
		if (p.contains("damageValue")) {
			pokerEffectLayout_.previewImages.damageValue.x = p["damageValue"].value("x", pokerEffectLayout_.previewImages.damageValue.x);
			pokerEffectLayout_.previewImages.damageValue.y = p["damageValue"].value("y", pokerEffectLayout_.previewImages.damageValue.y);
			pokerEffectLayout_.previewImages.damageValue.scale = p["damageValue"].value("scale", pokerEffectLayout_.previewImages.damageValue.scale);
			pokerEffectLayout_.previewImages.damageValue.spacing = p["damageValue"].value("spacing", pokerEffectLayout_.previewImages.damageValue.spacing);
		}

		if (p.contains("turnStartLabel")) {
			pokerEffectLayout_.previewImages.turnStartLabel.x = p["turnStartLabel"].value("x", pokerEffectLayout_.previewImages.turnStartLabel.x);
			pokerEffectLayout_.previewImages.turnStartLabel.y = p["turnStartLabel"].value("y", pokerEffectLayout_.previewImages.turnStartLabel.y);
			pokerEffectLayout_.previewImages.turnStartLabel.scale = p["turnStartLabel"].value("scale", pokerEffectLayout_.previewImages.turnStartLabel.scale);
		}

		if (p.contains("turnStartNoneLabel")) {
			pokerEffectLayout_.previewImages.turnStartNoneLabel.x = p["turnStartNoneLabel"].value("x", pokerEffectLayout_.previewImages.turnStartNoneLabel.x);
			pokerEffectLayout_.previewImages.turnStartNoneLabel.y = p["turnStartNoneLabel"].value("y", pokerEffectLayout_.previewImages.turnStartNoneLabel.y);
			pokerEffectLayout_.previewImages.turnStartNoneLabel.scale = p["turnStartNoneLabel"].value("scale", pokerEffectLayout_.previewImages.turnStartNoneLabel.scale);
		}

		if (p.contains("activatedLabel")) {
			pokerEffectLayout_.previewImages.activatedLabel.x = p["activatedLabel"].value("x", pokerEffectLayout_.previewImages.activatedLabel.x);
			pokerEffectLayout_.previewImages.activatedLabel.y = p["activatedLabel"].value("y", pokerEffectLayout_.previewImages.activatedLabel.y);
			pokerEffectLayout_.previewImages.activatedLabel.scale = p["activatedLabel"].value("scale", pokerEffectLayout_.previewImages.activatedLabel.scale);
		}

		if (p.contains("activatedNoneLabel")) {
			pokerEffectLayout_.previewImages.activatedNoneLabel.x = p["activatedNoneLabel"].value("x", pokerEffectLayout_.previewImages.activatedNoneLabel.x);
			pokerEffectLayout_.previewImages.activatedNoneLabel.y = p["activatedNoneLabel"].value("y", pokerEffectLayout_.previewImages.activatedNoneLabel.y);
			pokerEffectLayout_.previewImages.activatedNoneLabel.scale = p["activatedNoneLabel"].value("scale", pokerEffectLayout_.previewImages.activatedNoneLabel.scale);
		}

		if (p.contains("turnStartEffectAnchors")) {
			readEffectAnchors(
				p["turnStartEffectAnchors"],
				pokerEffectLayout_.previewImages.turnStartEffectAnchors);
		}

		if (p.contains("activatedEffectAnchors")) {
			readEffectAnchors(
				p["activatedEffectAnchors"],
				pokerEffectLayout_.previewImages.activatedEffectAnchors);
		}

		if (p.contains("turnStartPatterns")) {
			auto& pp = p["turnStartPatterns"];

			if (pp.contains("singleDamage")) {
				readPattern(pp["singleDamage"],
					pokerEffectLayout_.previewImages.turnStartPatterns.singleDamage);
			}
			if (pp.contains("allDamage")) {
				readPattern(pp["allDamage"],
					pokerEffectLayout_.previewImages.turnStartPatterns.allDamage);
			}
			if (pp.contains("draw")) {
				readPattern(pp["draw"],
					pokerEffectLayout_.previewImages.turnStartPatterns.draw);
			}
			if (pp.contains("block")) {
				readPattern(pp["block"],
					pokerEffectLayout_.previewImages.turnStartPatterns.block);
			}
			if (pp.contains("heal")) {
				readPattern(pp["heal"],
					pokerEffectLayout_.previewImages.turnStartPatterns.heal);
			}
		}

		if (p.contains("activatedPatterns")) {
			auto& pp = p["activatedPatterns"];

			if (pp.contains("singleDamage")) {
				readPattern(pp["singleDamage"],
					pokerEffectLayout_.previewImages.activatedPatterns.singleDamage);
			}
			if (pp.contains("allDamage")) {
				readPattern(pp["allDamage"],
					pokerEffectLayout_.previewImages.activatedPatterns.allDamage);
			}
			if (pp.contains("draw")) {
				readPattern(pp["draw"],
					pokerEffectLayout_.previewImages.activatedPatterns.draw);
			}
			if (pp.contains("block")) {
				readPattern(pp["block"],
					pokerEffectLayout_.previewImages.activatedPatterns.block);
			}
			if (pp.contains("heal")) {
				readPattern(pp["heal"],
					pokerEffectLayout_.previewImages.activatedPatterns.heal);
			}
		}

	}



	// activateTitleBg
	if (j.contains("activateTitleBg")) {
		pokerEffectLayout_.activateTitleBg.x = j["activateTitleBg"].value("x", pokerEffectLayout_.activateTitleBg.x);
		pokerEffectLayout_.activateTitleBg.y = j["activateTitleBg"].value("y", pokerEffectLayout_.activateTitleBg.y);
		pokerEffectLayout_.activateTitleBg.w = j["activateTitleBg"].value("w", pokerEffectLayout_.activateTitleBg.w);
		pokerEffectLayout_.activateTitleBg.h = j["activateTitleBg"].value("h", pokerEffectLayout_.activateTitleBg.h);
	}

	// activateYes
	if (j.contains("activateYes")) {
		auto& a = j["activateYes"];
		if (a.contains("rect")) {
			pokerEffectLayout_.activateYesRect.x = a["rect"].value("x", pokerEffectLayout_.activateYesRect.x);
			pokerEffectLayout_.activateYesRect.y = a["rect"].value("y", pokerEffectLayout_.activateYesRect.y);
			pokerEffectLayout_.activateYesRect.w = a["rect"].value("w", pokerEffectLayout_.activateYesRect.w);
			pokerEffectLayout_.activateYesRect.h = a["rect"].value("h", pokerEffectLayout_.activateYesRect.h);
		}
		if (a.contains("image")) {
			pokerEffectLayout_.activateYesImage.x = a["image"].value("x", pokerEffectLayout_.activateYesImage.x);
			pokerEffectLayout_.activateYesImage.y = a["image"].value("y", pokerEffectLayout_.activateYesImage.y);
		}
	}

	// activateNo
	if (j.contains("activateNo")) {
		auto& a = j["activateNo"];
		if (a.contains("rect")) {
			pokerEffectLayout_.activateNoRect.x = a["rect"].value("x", pokerEffectLayout_.activateNoRect.x);
			pokerEffectLayout_.activateNoRect.y = a["rect"].value("y", pokerEffectLayout_.activateNoRect.y);
			pokerEffectLayout_.activateNoRect.w = a["rect"].value("w", pokerEffectLayout_.activateNoRect.w);
			pokerEffectLayout_.activateNoRect.h = a["rect"].value("h", pokerEffectLayout_.activateNoRect.h);
		}
		if (a.contains("image")) {
			pokerEffectLayout_.activateNoImage.x = a["image"].value("x", pokerEffectLayout_.activateNoImage.x);
			pokerEffectLayout_.activateNoImage.y = a["image"].value("y", pokerEffectLayout_.activateNoImage.y);
		}
	}

	// activateViewBoard
	if (j.contains("activateViewBoard")) {
		auto& a = j["activateViewBoard"];
		if (a.contains("rect")) {
			pokerEffectLayout_.activateViewBoardRect.x = a["rect"].value("x", pokerEffectLayout_.activateViewBoardRect.x);
			pokerEffectLayout_.activateViewBoardRect.y = a["rect"].value("y", pokerEffectLayout_.activateViewBoardRect.y);
			pokerEffectLayout_.activateViewBoardRect.w = a["rect"].value("w", pokerEffectLayout_.activateViewBoardRect.w);
			pokerEffectLayout_.activateViewBoardRect.h = a["rect"].value("h", pokerEffectLayout_.activateViewBoardRect.h);
		}
		if (a.contains("image")) {
			pokerEffectLayout_.activateViewBoardImage.x = a["image"].value("x", pokerEffectLayout_.activateViewBoardImage.x);
			pokerEffectLayout_.activateViewBoardImage.y = a["image"].value("y", pokerEffectLayout_.activateViewBoardImage.y);
		}
	}

	// effectTitleBg
	if (j.contains("effectTitleBg")) {
		pokerEffectLayout_.effectTitleBg.x = j["effectTitleBg"].value("x", pokerEffectLayout_.effectTitleBg.x);
		pokerEffectLayout_.effectTitleBg.y = j["effectTitleBg"].value("y", pokerEffectLayout_.effectTitleBg.y);
		pokerEffectLayout_.effectTitleBg.w = j["effectTitleBg"].value("w", pokerEffectLayout_.effectTitleBg.w);
		pokerEffectLayout_.effectTitleBg.h = j["effectTitleBg"].value("h", pokerEffectLayout_.effectTitleBg.h);
	}

	// effectViewBoard
	if (j.contains("effectViewBoard")) {
		auto& e = j["effectViewBoard"];
		if (e.contains("rect")) {
			pokerEffectLayout_.effectViewBoardRect.x = e["rect"].value("x", pokerEffectLayout_.effectViewBoardRect.x);
			pokerEffectLayout_.effectViewBoardRect.y = e["rect"].value("y", pokerEffectLayout_.effectViewBoardRect.y);
			pokerEffectLayout_.effectViewBoardRect.w = e["rect"].value("w", pokerEffectLayout_.effectViewBoardRect.w);
			pokerEffectLayout_.effectViewBoardRect.h = e["rect"].value("h", pokerEffectLayout_.effectViewBoardRect.h);
		}
		if (e.contains("image")) {
			pokerEffectLayout_.effectViewBoardImage.x = e["image"].value("x", pokerEffectLayout_.effectViewBoardImage.x);
			pokerEffectLayout_.effectViewBoardImage.y = e["image"].value("y", pokerEffectLayout_.effectViewBoardImage.y);
		}
	}

	auto readText = [&](const json& src, UiText& t) {
		t.x = src.value("x", t.x);
		t.y = src.value("y", t.y);
		t.scale = src.value("scale", t.scale);
		};

	if (j.contains("activateTitleText")) {
		readText(j["activateTitleText"], pokerEffectLayout_.activateTitleText);
	}
	if (j.contains("activateYesText")) {
		readText(j["activateYesText"], pokerEffectLayout_.activateYesText);
	}
	if (j.contains("activateNoText")) {
		readText(j["activateNoText"], pokerEffectLayout_.activateNoText);
	}
	if (j.contains("activateViewBoardText")) {
		readText(j["activateViewBoardText"], pokerEffectLayout_.activateViewBoardText);
	}
	if (j.contains("infoButtonText")) {
		readText(j["infoButtonText"], pokerEffectLayout_.infoButtonText);
	}

	if (j.contains("effectTitleText")) {
		readText(j["effectTitleText"], pokerEffectLayout_.effectTitleText);
	}
	if (j.contains("backText")) {
		readText(j["backText"], pokerEffectLayout_.backText);
	}
	if (j.contains("effectTexts") && j["effectTexts"].is_array()) {
		for (int i = 0; i < 3 && i < static_cast<int>(j["effectTexts"].size()); ++i) {
			readText(j["effectTexts"][i], pokerEffectLayout_.effectTexts[i]);
		}
	}
	if (j.contains("effectViewBoardText")) {
		readText(j["effectViewBoardText"], pokerEffectLayout_.effectViewBoardText);
	}

	return true;
}

bool FieldUi::LoadFieldUiLayout(const std::string& path)
{
	std::ifstream f(path);
	if (!f.is_open()) {
		layout_.cardDescBg = { 20.0f, 600.0f, 900.0f, 120.0f };
		layout_.cardDescText = { 40.0f, 620.0f, 1.0f };

		layout_.deckBg = { 20.0f, 310.0f, 150.0f, 60.0f };
		layout_.deckText = { 40.0f, 320.0f, 0.9f };

		layout_.discardBg = { 1100.0f, 350.0f, 150.0f, 60.0f };
		layout_.discardText = { 1120.0f, 350.0f, 0.9f };

		layout_.handBg = { 1000.0f, 640.0f, 250.0f, 60.0f };
		layout_.handText = { 1020.0f, 640.0f, 0.9f };

		layout_.fieldBg = { 540.0f, 250.0f, 250.0f, 60.0f };
		layout_.fieldText = { 600.0f, 250.0f, 0.9f };

		layout_.turnBg = { 490.0f, 25.0f, 250.0f, 60.0f };
		layout_.turnText = { 500.0f, 20.0f, 1.0f };

		layout_.costBg = { 75.0f, 405.0f, 170.0f, 55.0f };
		layout_.costText = { 90.0f, 400.0f, 1.0f };

		layout_.deckLabelImage = { 40.0f, 320.0f, 1.0f };
		layout_.discardLabelImage = { 1120.0f, 350.0f, 1.0f };
		layout_.handLabelImage = { 1020.0f, 640.0f, 1.0f };

		layout_.overlay = { 0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight) };
		
		layout_.cardDescImage.baseEffectOffsetY = 0.0f;

		layout_.cardDescImage.baseEffectTypeOffsetX = 0.0f;
		layout_.cardDescImage.baseEffectTypeOffsetY = 0.0f;

		layout_.cardDescImage.baseColonOffsetX = 170.0f;
		layout_.cardDescImage.baseColonOffsetY = 0.0f;

		layout_.cardDescImage.baseValueOffsetX = 210.0f;
		layout_.cardDescImage.baseValueOffsetY = 0.0f;
		layout_.cardDescImage.baseValueScale = 0.35f;
		layout_.cardDescImage.baseValueSpacing = 28.0f;

		layout_.cardDescImage.separatorOffsetX = 0.0f;
		layout_.cardDescImage.separatorOffsetY = 0.0f;
		layout_.cardDescImage.separatorWidth = 320.0f;
		layout_.cardDescImage.separatorHeight = 2.0f;

		layout_.cardDescImage.triggerOffsetX = 0.0f;
		layout_.cardDescImage.triggerOffsetY = 112.0f;

		layout_.cardDescImage.rankOffsetX = 0.0f;
		layout_.cardDescImage.rankOffsetY = 0.0f;

		layout_.cardDescImage.suffixOffsetX = 170.0f;
		layout_.cardDescImage.suffixOffsetY = 0.0f;

		layout_.cardDescImage.subEffectTypeOffsetX = 0.0f;
		layout_.cardDescImage.subEffectTypeOffsetY = 0.0f;

		layout_.cardDescImage.subColonOffsetX = 170.0f;
		layout_.cardDescImage.subColonOffsetY = 0.0f;

		layout_.cardDescImage.subValueOffsetX = 210.0f;
		layout_.cardDescImage.subValueOffsetY = 0.0f;
		layout_.cardDescImage.subValueScale = 0.35f;
		layout_.cardDescImage.subValueSpacing = 28.0f;
		
		// =========================
        // cardDescCustom default
        // =========================
 		layout_.cardDescCustom.titleBasicEffect = { 0.0f, 0.0f, 1.0f };
		layout_.cardDescCustom.separator = { 0.0f, 130.0f, 1.0f };

		layout_.cardDescCustom.baseRows[0].target = { -150.0f, 42.0f, 0.75f };
		layout_.cardDescCustom.baseRows[0].particle = { -10.0f, 42.0f, 0.75f };
		layout_.cardDescCustom.baseRows[0].effectType = { 90.0f, 42.0f, 0.75f };
		layout_.cardDescCustom.baseRows[0].value = { 40.0f, 72.0f, 0.28f, 28.0f };

		layout_.cardDescCustom.baseRows[1].target = { -150.0f, 112.0f, 0.75f };
		layout_.cardDescCustom.baseRows[1].particle = { -10.0f, 112.0f, 0.75f };
		layout_.cardDescCustom.baseRows[1].effectType = { 90.0f, 112.0f, 0.75f };
		layout_.cardDescCustom.baseRows[1].value = { 40.0f, 142.0f, 0.28f, 28.0f };

		layout_.cardDescCustom.baseRows[2].target = { -150.0f, 182.0f, 0.75f };
		layout_.cardDescCustom.baseRows[2].particle = { -10.0f, 182.0f, 0.75f };
		layout_.cardDescCustom.baseRows[2].effectType = { 90.0f, 182.0f, 0.75f };
		layout_.cardDescCustom.baseRows[2].value = { 40.0f, 212.0f, 0.28f, 28.0f };
		
		layout_.cardDescCustom.baseRows[0].special1 = { 90.0f, 42.0f, 0.75f };
		layout_.cardDescCustom.baseRows[0].special2 = { 260.0f, 42.0f, 0.75f };
		layout_.cardDescCustom.baseRows[0].specialAdvance = 250.0f;

		layout_.cardDescCustom.baseRows[1].special1 = { 90.0f, 112.0f, 0.75f };
		layout_.cardDescCustom.baseRows[1].special2 = { 260.0f, 112.0f, 0.75f };
		layout_.cardDescCustom.baseRows[1].specialAdvance = 250.0f;

		layout_.cardDescCustom.baseRows[2].special1 = { 90.0f, 182.0f, 0.75f };
		layout_.cardDescCustom.baseRows[2].special2 = { 260.0f, 182.0f, 0.75f };
		layout_.cardDescCustom.baseRows[2].specialAdvance = 250.0f;

		layout_.cardDescCustom.subBlocks[0].trigger = { -140.0f, 250.0f, 0.74f };
		layout_.cardDescCustom.subBlocks[0].rank = { -141.0f, 295.0f, 0.73f };
		layout_.cardDescCustom.subBlocks[0].suffix = { 34.0f, 293.0f, 0.67f };
		layout_.cardDescCustom.subBlocks[0].target = { -120.0f, 331.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[0].particle = { 15.0f, 331.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[0].effectType = { 110.0f, 331.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[0].value = { 40.0f, 363.0f, 0.23f, 28.0f };

		layout_.cardDescCustom.subBlocks[1].trigger = { -140.0f, 410.0f, 0.74f };
		layout_.cardDescCustom.subBlocks[1].rank = { -141.0f, 455.0f, 0.73f };
		layout_.cardDescCustom.subBlocks[1].suffix = { 34.0f, 453.0f, 0.67f };
		layout_.cardDescCustom.subBlocks[1].target = { -120.0f, 491.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[1].particle = { 15.0f, 491.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[1].effectType = { 110.0f, 491.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[1].value = { 40.0f, 523.0f, 0.23f, 28.0f };

		layout_.cardDescCustom.subBlocks[2].trigger = { -140.0f, 570.0f, 0.74f };
		layout_.cardDescCustom.subBlocks[2].rank = { -141.0f, 615.0f, 0.73f };
		layout_.cardDescCustom.subBlocks[2].suffix = { 34.0f, 613.0f, 0.67f };
		layout_.cardDescCustom.subBlocks[2].target = { -120.0f, 651.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[2].particle = { 15.0f, 651.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[2].effectType = { 110.0f, 651.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[2].value = { 40.0f, 683.0f, 0.23f, 28.0f };

		return false;
	}

	json j;
	try {
		f >> j;
	} catch (...) {
		return false;
	}

	auto readRect = [&](const char* key, UiRect& out) {
		if (!j.contains(key)) return;
		auto& v = j[key];
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.w = v.value("w", out.w);
		out.h = v.value("h", out.h);
		};

	auto readCardDescImage = [&](const char* key, UiCardDescImageLayout& out) {
		if (!j.contains(key)) return;
		auto& v = j[key];

		out.baseEffectOffsetY = v.value("baseEffectOffsetY", out.baseEffectOffsetY);

		out.baseEffectTypeOffsetX = v.value("baseEffectTypeOffsetX", out.baseEffectTypeOffsetX);
		out.baseEffectTypeOffsetY = v.value("baseEffectTypeOffsetY", out.baseEffectTypeOffsetY);

		out.baseColonOffsetX = v.value("baseColonOffsetX", out.baseColonOffsetX);
		out.baseColonOffsetY = v.value("baseColonOffsetY", out.baseColonOffsetY);

		out.baseValueOffsetX = v.value("baseValueOffsetX", out.baseValueOffsetX);
		out.baseValueOffsetY = v.value("baseValueOffsetY", out.baseValueOffsetY);
		out.baseValueScale = v.value("baseValueScale", out.baseValueScale);
		out.baseValueSpacing = v.value("baseValueSpacing", out.baseValueSpacing);

		out.separatorOffsetX = v.value("separatorOffsetX", out.separatorOffsetX);
		out.separatorOffsetY = v.value("separatorOffsetY", out.separatorOffsetY);
		out.separatorWidth = v.value("separatorWidth", out.separatorWidth);
		out.separatorHeight = v.value("separatorHeight", out.separatorHeight);

		out.triggerOffsetX = v.value("triggerOffsetX", out.triggerOffsetX);
		out.triggerOffsetY = v.value("triggerOffsetY", out.triggerOffsetY);

		out.rankOffsetX = v.value("rankOffsetX", out.rankOffsetX);
		out.rankOffsetY = v.value("rankOffsetY", out.rankOffsetY);

		out.suffixOffsetX = v.value("suffixOffsetX", out.suffixOffsetX);
		out.suffixOffsetY = v.value("suffixOffsetY", out.suffixOffsetY);

		out.subEffectTypeOffsetX = v.value("subEffectTypeOffsetX", out.subEffectTypeOffsetX);
		out.subEffectTypeOffsetY = v.value("subEffectTypeOffsetY", out.subEffectTypeOffsetY);

		out.subColonOffsetX = v.value("subColonOffsetX", out.subColonOffsetX);
		out.subColonOffsetY = v.value("subColonOffsetY", out.subColonOffsetY);

		out.subValueOffsetX = v.value("subValueOffsetX", out.subValueOffsetX);
		out.subValueOffsetY = v.value("subValueOffsetY", out.subValueOffsetY);
		out.subValueScale = v.value("subValueScale", out.subValueScale);
		out.subValueSpacing = v.value("subValueSpacing", out.subValueSpacing);

		out.baseEffectOffsetX = v.value("baseEffectOffsetX", out.baseEffectOffsetX);
		out.baseEffectOffsetY = v.value("baseEffectOffsetY", out.baseEffectOffsetY);
		out.baseEffectScale = v.value("baseEffectScale", out.baseEffectScale);

		out.baseEffectTypeScale = v.value("baseEffectTypeScale", out.baseEffectTypeScale);
		out.baseColonScale = v.value("baseColonScale", out.baseColonScale);

		out.triggerScale = v.value("triggerScale", out.triggerScale);
		out.rankScale = v.value("rankScale", out.rankScale);
		out.suffixScale = v.value("suffixScale", out.suffixScale);

		out.subEffectTypeScale = v.value("subEffectTypeScale", out.subEffectTypeScale);
		out.subColonScale = v.value("subColonScale", out.subColonScale);

		};

	auto readText = [&](const char* key, UiText& out) {
		if (!j.contains(key)) return;
		auto& v = j[key];
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.scale = v.value("scale", out.scale);
		};

	auto readImageItem = [&](const json& v, UiImageItem& out) {
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.scale = v.value("scale", out.scale);
		};

	auto readNumberItem = [&](const json& v, UiNumberItem& out) {
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.scale = v.value("scale", out.scale);
		out.spacing = v.value("spacing", out.spacing);
		};

	auto readCardDescCustom = [&](const char* key, UiCardDescCustomLayout& out) {
		if (!j.contains(key)) return;
		auto& v = j[key];

		if (v.contains("titleBasicEffect")) readImageItem(v["titleBasicEffect"], out.titleBasicEffect);
		if (v.contains("separator"))       readImageItem(v["separator"], out.separator);

		if (v.contains("baseRows") && v["baseRows"].is_array()) {
			for (int i = 0; i < static_cast<int>(v["baseRows"].size()) && i < 3; ++i) {
				auto& row = v["baseRows"][i];
				if (row.contains("target"))    readImageItem(row["target"], out.baseRows[i].target);
				if (row.contains("particle"))  readImageItem(row["particle"], out.baseRows[i].particle);
				if (row.contains("effectType"))readImageItem(row["effectType"], out.baseRows[i].effectType);
				if (row.contains("value"))     readNumberItem(row["value"], out.baseRows[i].value);
				if (row.contains("special1")) readImageItem(row["special1"], out.baseRows[i].special1);
				if (row.contains("special2")) readImageItem(row["special2"], out.baseRows[i].special2);
				out.baseRows[i].specialAdvance = row.value("specialAdvance", out.baseRows[i].specialAdvance);
			}
		}

		if (v.contains("subBlocks") && v["subBlocks"].is_array()) {
			for (int i = 0; i < static_cast<int>(v["subBlocks"].size()) && i < 3; ++i) {
				auto& block = v["subBlocks"][i];
				if (block.contains("trigger"))    readImageItem(block["trigger"], out.subBlocks[i].trigger);
				if (block.contains("rank"))       readImageItem(block["rank"], out.subBlocks[i].rank);
				if (block.contains("suffix"))     readImageItem(block["suffix"], out.subBlocks[i].suffix);
				if (block.contains("target"))    readImageItem(block["target"], out.subBlocks[i].target);
				if (block.contains("particle"))  readImageItem(block["particle"], out.subBlocks[i].particle);
				if (block.contains("effectType"))readImageItem(block["effectType"], out.subBlocks[i].effectType);
				if (block.contains("value"))     readNumberItem(block["value"], out.subBlocks[i].value);
			}
		}
		};

	layout_.cardDescBg = { 20.0f, 600.0f, 900.0f, 120.0f };
	layout_.cardDescText = { 40.0f, 620.0f, 1.0f };
	layout_.deckBg = { 20.0f, 310.0f, 150.0f, 60.0f };
	layout_.deckText = { 40.0f, 320.0f, 0.9f };
	layout_.discardBg = { 1100.0f, 350.0f, 150.0f, 60.0f };
	layout_.discardText = { 1120.0f, 350.0f, 0.9f };
	layout_.handBg = { 1000.0f, 640.0f, 250.0f, 60.0f };
	layout_.handText = { 1020.0f, 640.0f, 0.9f };
	layout_.deckLabelImage = { 40.0f, 320.0f, 1.0f };
	layout_.discardLabelImage = { 1120.0f, 350.0f, 1.0f };
	layout_.handLabelImage = { 1020.0f, 640.0f, 1.0f };
	layout_.fieldBg = { 540.0f, 250.0f, 250.0f, 60.0f };
	layout_.fieldText = { 600.0f, 250.0f, 0.9f };
	layout_.turnBg = { 490.0f, 25.0f, 250.0f, 60.0f };
	layout_.turnText = { 500.0f, 20.0f, 1.0f };
	layout_.costBg = { 75.0f, 405.0f, 170.0f, 55.0f };
	layout_.costText = { 90.0f, 400.0f, 1.0f };
	layout_.overlay = { 0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight) };
	layout_.cardDescImage.baseEffectOffsetY = 0.0f;

	layout_.cardDescImage.baseEffectTypeOffsetX = 0.0f;
	layout_.cardDescImage.baseEffectTypeOffsetY = 0.0f;

	layout_.cardDescImage.baseColonOffsetX = 170.0f;
	layout_.cardDescImage.baseColonOffsetY = 0.0f;

	layout_.cardDescImage.baseValueOffsetX = 210.0f;
	layout_.cardDescImage.baseValueOffsetY = 0.0f;
	layout_.cardDescImage.baseValueScale = 0.35f;
	layout_.cardDescImage.baseValueSpacing = 28.0f;

	layout_.cardDescImage.separatorOffsetY = 0.0f;
	layout_.cardDescImage.separatorWidth = 320.0f;
	layout_.cardDescImage.separatorHeight = 2.0f;

	layout_.cardDescImage.triggerOffsetX = 0.0f;
	layout_.cardDescImage.triggerOffsetY = 0.0f;

	layout_.cardDescImage.rankOffsetX = 0.0f;
	layout_.cardDescImage.rankOffsetY = 0.0f;

	layout_.cardDescImage.suffixOffsetX = 170.0f;
	layout_.cardDescImage.suffixOffsetY = 0.0f;

	layout_.cardDescImage.subEffectTypeOffsetX = 0.0f;
	layout_.cardDescImage.subEffectTypeOffsetY = 0.0f;

	layout_.cardDescImage.subColonOffsetX = 170.0f;
	layout_.cardDescImage.subColonOffsetY = 0.0f;

	layout_.cardDescImage.subValueOffsetX = 210.0f;
	layout_.cardDescImage.subValueOffsetY = 0.0f;
	layout_.cardDescImage.subValueScale = 0.35f;
	layout_.cardDescImage.subValueSpacing = 28.0f;

	// =========================
    // cardDescCustom default
    // ========================= 
	layout_.cardDescCustom.titleBasicEffect = { 0.0f, 0.0f, 1.0f };
	layout_.cardDescCustom.separator = { 0.0f, 130.0f, 1.0f };

	layout_.cardDescCustom.baseRows[0].target = { -150.0f, 42.0f, 0.75f };
	layout_.cardDescCustom.baseRows[0].particle = { -10.0f, 42.0f, 0.75f };
	layout_.cardDescCustom.baseRows[0].effectType = { 90.0f, 42.0f, 0.75f };
	layout_.cardDescCustom.baseRows[0].value = { 40.0f, 72.0f, 0.28f, 28.0f };
	layout_.cardDescCustom.baseRows[0].special1 = { 90.0f, 42.0f, 0.75f };
	layout_.cardDescCustom.baseRows[0].special2 = { 260.0f, 42.0f, 0.75f };
	layout_.cardDescCustom.baseRows[0].specialAdvance = 250.0f;

	layout_.cardDescCustom.baseRows[1].target = { -150.0f, 112.0f, 0.75f };
	layout_.cardDescCustom.baseRows[1].particle = { -10.0f, 112.0f, 0.75f };
	layout_.cardDescCustom.baseRows[1].effectType = { 90.0f, 112.0f, 0.75f };
	layout_.cardDescCustom.baseRows[1].value = { 40.0f, 142.0f, 0.28f, 28.0f };
	layout_.cardDescCustom.baseRows[1].special1 = { 90.0f, 112.0f, 0.75f };
	layout_.cardDescCustom.baseRows[1].special2 = { 260.0f, 112.0f, 0.75f };
	layout_.cardDescCustom.baseRows[1].specialAdvance = 250.0f;

	layout_.cardDescCustom.baseRows[2].target = { -150.0f, 182.0f, 0.75f };
	layout_.cardDescCustom.baseRows[2].particle = { -10.0f, 182.0f, 0.75f };
	layout_.cardDescCustom.baseRows[2].effectType = { 90.0f, 182.0f, 0.75f };
	layout_.cardDescCustom.baseRows[2].value = { 40.0f, 212.0f, 0.28f, 28.0f };
	layout_.cardDescCustom.baseRows[2].special1 = { 90.0f, 182.0f, 0.75f };
	layout_.cardDescCustom.baseRows[2].special2 = { 260.0f, 182.0f, 0.75f };
	layout_.cardDescCustom.baseRows[2].specialAdvance = 250.0f;

	layout_.cardDescCustom.subBlocks[0].trigger = { -140.0f, 250.0f, 0.74f };
	layout_.cardDescCustom.subBlocks[0].rank = { -141.0f, 295.0f, 0.73f };
	layout_.cardDescCustom.subBlocks[0].suffix = { 34.0f, 293.0f, 0.67f };
	layout_.cardDescCustom.subBlocks[0].target = { -120.0f, 331.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[0].particle = { 15.0f, 331.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[0].effectType = { 110.0f, 331.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[0].value = { 40.0f, 363.0f, 0.23f, 28.0f };

	layout_.cardDescCustom.subBlocks[1].trigger = { -140.0f, 410.0f, 0.74f };
	layout_.cardDescCustom.subBlocks[1].rank = { -141.0f, 455.0f, 0.73f };
	layout_.cardDescCustom.subBlocks[1].suffix = { 34.0f, 453.0f, 0.67f };
	layout_.cardDescCustom.subBlocks[1].target = { -120.0f, 491.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[1].particle = { 15.0f, 491.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[1].effectType = { 110.0f, 491.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[1].value = { 40.0f, 523.0f, 0.23f, 28.0f };

	layout_.cardDescCustom.subBlocks[2].trigger = { -140.0f, 570.0f, 0.74f };
	layout_.cardDescCustom.subBlocks[2].rank = { -141.0f, 615.0f, 0.73f };
	layout_.cardDescCustom.subBlocks[2].suffix = { 34.0f, 613.0f, 0.67f };
	layout_.cardDescCustom.subBlocks[2].target = { -120.0f, 651.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[2].particle = { 15.0f, 651.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[2].effectType = { 110.0f, 651.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[2].value = { 40.0f, 683.0f, 0.23f, 28.0f };

	readRect("cardDescBg", layout_.cardDescBg);
	readText("cardDescText", layout_.cardDescText);

	readRect("deckBg", layout_.deckBg);
	readText("deckText", layout_.deckText);
	readText("deckLabelImage", layout_.deckLabelImage);

	readRect("discardBg", layout_.discardBg);
	readText("discardText", layout_.discardText);
	readText("discardLabelImage", layout_.discardLabelImage);

	readRect("handBg", layout_.handBg);
	readText("handText", layout_.handText);
	readText("handLabelImage", layout_.handLabelImage);

	readRect("fieldBg", layout_.fieldBg);
	readText("fieldText", layout_.fieldText);

	readRect("turnBg", layout_.turnBg);
	readText("turnText", layout_.turnText);

	readRect("costBg", layout_.costBg);
	readText("costText", layout_.costText);

	readRect("endTurnBg", layout_.endTurnBg);
	readText("endTurnText", layout_.endTurnText);

	readRect("overlay", layout_.overlay);

	readCardDescImage("cardDescImage", layout_.cardDescImage);

	readCardDescCustom("cardDescCustom", layout_.cardDescCustom);

	return true;
}
bool FieldUi::LoadCardShowUiLayout(const std::string& path)
{
	std::ifstream f(path);
	if (!f.is_open()) {
		return false;
	}

	json j;
	try {
		f >> j;
	} catch (...) {
		return false;
	}

	auto readRect = [&](const char* key, UiRect& out) {
		if (!j.contains(key)) return;
		auto& v = j[key];
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.w = v.value("w", out.w);
		out.h = v.value("h", out.h);
		};

	auto readText = [&](const char* key, UiText& out) {
		if (!j.contains(key)) return;
		auto& v = j[key];
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.scale = v.value("scale", out.scale);
		};

	auto readImageItem = [&](const json& v, UiImageItem& out) {
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.scale = v.value("scale", out.scale);
		};

	auto readNumberItem = [&](const json& v, UiNumberItem& out) {
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.scale = v.value("scale", out.scale);
		out.spacing = v.value("spacing", out.spacing);
		};

	auto readCardDescCustomFromJson = [&](const json& v, UiCardDescCustomLayout& out) {
		if (v.contains("titleBasicEffect")) readImageItem(v["titleBasicEffect"], out.titleBasicEffect);
		if (v.contains("separator")) readImageItem(v["separator"], out.separator);

		if (v.contains("baseRows") && v["baseRows"].is_array()) {
			for (int i = 0; i < static_cast<int>(v["baseRows"].size()) && i < 3; ++i) {
				auto& row = v["baseRows"][i];
				if (row.contains("target")) readImageItem(row["target"], out.baseRows[i].target);
				if (row.contains("particle")) readImageItem(row["particle"], out.baseRows[i].particle);
				if (row.contains("effectType")) readImageItem(row["effectType"], out.baseRows[i].effectType);
				if (row.contains("value")) readNumberItem(row["value"], out.baseRows[i].value);
				if (row.contains("special1")) readImageItem(row["special1"], out.baseRows[i].special1);
				if (row.contains("special2")) readImageItem(row["special2"], out.baseRows[i].special2);
				out.baseRows[i].specialAdvance = row.value("specialAdvance", out.baseRows[i].specialAdvance);
			}
		}

		if (v.contains("subBlocks") && v["subBlocks"].is_array()) {
			for (int i = 0; i < static_cast<int>(v["subBlocks"].size()) && i < 3; ++i) {
				auto& block = v["subBlocks"][i];
				if (block.contains("trigger")) readImageItem(block["trigger"], out.subBlocks[i].trigger);
				if (block.contains("rank")) readImageItem(block["rank"], out.subBlocks[i].rank);
				if (block.contains("suffix")) readImageItem(block["suffix"], out.subBlocks[i].suffix);
				if (block.contains("target")) readImageItem(block["target"], out.subBlocks[i].target);
				if (block.contains("particle")) readImageItem(block["particle"], out.subBlocks[i].particle);
				if (block.contains("effectType")) readImageItem(block["effectType"], out.subBlocks[i].effectType);
				if (block.contains("value")) readNumberItem(block["value"], out.subBlocks[i].value);
			}
		}
		};

	readRect("cardDescBg", layout_.cardDescBg);
	readText("cardDescText", layout_.cardDescText);

	if (j.contains("defaultCardDescCustom")) {
		readCardDescCustomFromJson(j["defaultCardDescCustom"], layout_.cardDescCustom);
	} else if (j.contains("cardDescCustom")) {
		// 旧形式互換
		readCardDescCustomFromJson(j["cardDescCustom"], layout_.cardDescCustom);
	}

	perCardDescCustomLayouts_.clear();
	if (j.contains("perCard") && j["perCard"].is_object()) {
		for (auto it = j["perCard"].begin(); it != j["perCard"].end(); ++it) {
			int cardId = std::stoi(it.key());
			UiCardDescCustomLayout custom = layout_.cardDescCustom;
			readCardDescCustomFromJson(it.value(), custom);
			perCardDescCustomLayouts_[cardId] = custom;
		}
	}

	perCardCustomDescImageLayouts_.clear();
	if (j.contains("perCardCustomDescImage") && j["perCardCustomDescImage"].is_object()) {
		for (auto it = j["perCardCustomDescImage"].begin(); it != j["perCardCustomDescImage"].end(); ++it) {
			int cardId = std::stoi(it.key());

			UiCustomDescImageLayout layout{};
			layout.x = it.value().value("x", layout_.cardDescBg.x);
			layout.y = it.value().value("y", layout_.cardDescBg.y);
			layout.scaleX = it.value().value("scaleX", 1.0f);
			layout.scaleY = it.value().value("scaleY", 1.0f);

			perCardCustomDescImageLayouts_[cardId] = layout;
		}
	}

	return true;
}

bool FieldUi::SaveCardShowUiLayout(const std::string& path) const
{
	json j;

	auto writeRect = [&](const char* key, const UiRect& r) {
		j[key]["x"] = r.x;
		j[key]["y"] = r.y;
		j[key]["w"] = r.w;
		j[key]["h"] = r.h;
		};

	auto writeText = [&](const char* key, const UiText& t) {
		j[key]["x"] = t.x;
		j[key]["y"] = t.y;
		j[key]["scale"] = t.scale;
		};

	auto writeImageItem = [&](json& dst, const UiImageItem& v) {
		dst["x"] = v.x;
		dst["y"] = v.y;
		dst["scale"] = v.scale;
		};

	auto writeNumberItem = [&](json& dst, const UiNumberItem& v) {
		dst["x"] = v.x;
		dst["y"] = v.y;
		dst["scale"] = v.scale;
		dst["spacing"] = v.spacing;
		};

	auto writeCardDescCustomToJson = [&](json& dst, const UiCardDescCustomLayout& v) {
		writeImageItem(dst["titleBasicEffect"], v.titleBasicEffect);
		writeImageItem(dst["separator"], v.separator);

		for (int i = 0; i < 3; ++i) {
			writeImageItem(dst["baseRows"][i]["target"], v.baseRows[i].target);
			writeImageItem(dst["baseRows"][i]["particle"], v.baseRows[i].particle);
			writeImageItem(dst["baseRows"][i]["effectType"], v.baseRows[i].effectType);
			writeNumberItem(dst["baseRows"][i]["value"], v.baseRows[i].value);
			writeImageItem(dst["baseRows"][i]["special1"], v.baseRows[i].special1);
			writeImageItem(dst["baseRows"][i]["special2"], v.baseRows[i].special2);
			dst["baseRows"][i]["specialAdvance"] = v.baseRows[i].specialAdvance;
		}

		for (int i = 0; i < 3; ++i) {
			writeImageItem(dst["subBlocks"][i]["trigger"], v.subBlocks[i].trigger);
			writeImageItem(dst["subBlocks"][i]["rank"], v.subBlocks[i].rank);
			writeImageItem(dst["subBlocks"][i]["suffix"], v.subBlocks[i].suffix);
			writeImageItem(dst["subBlocks"][i]["target"], v.subBlocks[i].target);
			writeImageItem(dst["subBlocks"][i]["particle"], v.subBlocks[i].particle);
			writeImageItem(dst["subBlocks"][i]["effectType"], v.subBlocks[i].effectType);
			writeNumberItem(dst["subBlocks"][i]["value"], v.subBlocks[i].value);
		}
		};

	writeRect("cardDescBg", layout_.cardDescBg);
	writeText("cardDescText", layout_.cardDescText);

	writeCardDescCustomToJson(j["defaultCardDescCustom"], layout_.cardDescCustom);

	for (const auto& [cardId, layout] : perCardDescCustomLayouts_) {
		writeCardDescCustomToJson(j["perCard"][std::to_string(cardId)], layout);
	}

	for (const auto& [cardId, layout] : perCardCustomDescImageLayouts_) {
		j["perCardCustomDescImage"][std::to_string(cardId)]["x"] = layout.x;
		j["perCardCustomDescImage"][std::to_string(cardId)]["y"] = layout.y;
		j["perCardCustomDescImage"][std::to_string(cardId)]["scaleX"] = layout.scaleX;
		j["perCardCustomDescImage"][std::to_string(cardId)]["scaleY"] = layout.scaleY;
	}

	std::ofstream ofs(path);
	if (!ofs.is_open()) return false;
	ofs << j.dump(4);
	return true;
}

bool FieldUi::SaveFieldUiLayout(const std::string& path) const
{
	json j;

	auto writeRect = [&](const char* key, const UiRect& r) {
		j[key]["x"] = r.x;
		j[key]["y"] = r.y;
		j[key]["w"] = r.w;
		j[key]["h"] = r.h;
		};

	auto writeText = [&](const char* key, const UiText& t) {
		j[key]["x"] = t.x;
		j[key]["y"] = t.y;
		j[key]["scale"] = t.scale;
		};

	writeRect("cardDescBg", layout_.cardDescBg);
	writeText("cardDescText", layout_.cardDescText);

	writeRect("deckBg", layout_.deckBg);
	writeText("deckText", layout_.deckText);
	writeText("deckLabelImage", layout_.deckLabelImage);

	writeRect("discardBg", layout_.discardBg);
	writeText("discardText", layout_.discardText);
	writeText("discardLabelImage", layout_.discardLabelImage);

	writeRect("handBg", layout_.handBg);
	writeText("handText", layout_.handText);
	writeText("handLabelImage", layout_.handLabelImage);

	writeRect("fieldBg", layout_.fieldBg);
	writeText("fieldText", layout_.fieldText);

	writeRect("turnBg", layout_.turnBg);
	writeText("turnText", layout_.turnText);

	writeRect("costBg", layout_.costBg);
	writeText("costText", layout_.costText);

	writeRect("endTurnBg", layout_.endTurnBg);
	writeText("endTurnText", layout_.endTurnText);

	
	auto writeCardDescImage = [&](const char* key, const UiCardDescImageLayout& v) {
		j[key]["baseEffectOffsetY"] = v.baseEffectOffsetY;

		j[key]["baseEffectTypeOffsetX"] = v.baseEffectTypeOffsetX;
		j[key]["baseEffectTypeOffsetY"] = v.baseEffectTypeOffsetY;

		j[key]["baseColonOffsetX"] = v.baseColonOffsetX;
		j[key]["baseColonOffsetY"] = v.baseColonOffsetY;

		j[key]["baseValueOffsetX"] = v.baseValueOffsetX;
		j[key]["baseValueOffsetY"] = v.baseValueOffsetY;
		j[key]["baseValueScale"] = v.baseValueScale;
		j[key]["baseValueSpacing"] = v.baseValueSpacing;

		j[key]["separatorOffsetX"] = v.separatorOffsetX;
		j[key]["separatorOffsetY"] = v.separatorOffsetY;
		j[key]["separatorWidth"] = v.separatorWidth;
		j[key]["separatorHeight"] = v.separatorHeight;

		j[key]["triggerOffsetX"] = v.triggerOffsetX;
		j[key]["triggerOffsetY"] = v.triggerOffsetY;

		j[key]["rankOffsetX"] = v.rankOffsetX;
		j[key]["rankOffsetY"] = v.rankOffsetY;

		j[key]["suffixOffsetX"] = v.suffixOffsetX;
		j[key]["suffixOffsetY"] = v.suffixOffsetY;

		j[key]["subEffectTypeOffsetX"] = v.subEffectTypeOffsetX;
		j[key]["subEffectTypeOffsetY"] = v.subEffectTypeOffsetY;

		j[key]["subColonOffsetX"] = v.subColonOffsetX;
		j[key]["subColonOffsetY"] = v.subColonOffsetY;

		j[key]["subValueOffsetX"] = v.subValueOffsetX;
		j[key]["subValueOffsetY"] = v.subValueOffsetY;
		j[key]["subValueScale"] = v.subValueScale;
		j[key]["subValueSpacing"] = v.subValueSpacing;

		j[key]["baseEffectOffsetX"] = v.baseEffectOffsetX;
		j[key]["baseEffectOffsetY"] = v.baseEffectOffsetY;
		j[key]["baseEffectScale"] = v.baseEffectScale;

		j[key]["baseEffectTypeScale"] = v.baseEffectTypeScale;
		j[key]["baseColonScale"] = v.baseColonScale;

		j[key]["triggerScale"] = v.triggerScale;
		j[key]["rankScale"] = v.rankScale;
		j[key]["suffixScale"] = v.suffixScale;

		j[key]["subEffectTypeScale"] = v.subEffectTypeScale;
		j[key]["subColonScale"] = v.subColonScale;

		};

	auto writeImageItem = [&](json& dst, const UiImageItem& v) {
		dst["x"] = v.x;
		dst["y"] = v.y;
		dst["scale"] = v.scale;
		};

	auto writeNumberItem = [&](json& dst, const UiNumberItem& v) {
		dst["x"] = v.x;
		dst["y"] = v.y;
		dst["scale"] = v.scale;
		dst["spacing"] = v.spacing;
		};

	auto writeCardDescCustom = [&](const char* key, const UiCardDescCustomLayout& v) {
		writeImageItem(j[key]["titleBasicEffect"], v.titleBasicEffect);
		writeImageItem(j[key]["separator"], v.separator);

		for (int i = 0; i < 3; ++i) {
			writeImageItem(j[key]["baseRows"][i]["target"], v.baseRows[i].target);
			writeImageItem(j[key]["baseRows"][i]["particle"], v.baseRows[i].particle);
			writeImageItem(j[key]["baseRows"][i]["effectType"], v.baseRows[i].effectType);
			writeNumberItem(j[key]["baseRows"][i]["value"], v.baseRows[i].value);
			writeImageItem(j[key]["baseRows"][i]["special1"], v.baseRows[i].special1);
			writeImageItem(j[key]["baseRows"][i]["special2"], v.baseRows[i].special2);
			j[key]["baseRows"][i]["specialAdvance"] = v.baseRows[i].specialAdvance;
		}

		for (int i = 0; i < 3; ++i) {
			writeImageItem(j[key]["subBlocks"][i]["trigger"], v.subBlocks[i].trigger);
			writeImageItem(j[key]["subBlocks"][i]["rank"], v.subBlocks[i].rank);
			writeImageItem(j[key]["subBlocks"][i]["suffix"], v.subBlocks[i].suffix);
			writeImageItem(j[key]["subBlocks"][i]["target"], v.subBlocks[i].target);
			writeImageItem(j[key]["subBlocks"][i]["particle"], v.subBlocks[i].particle);
			writeImageItem(j[key]["subBlocks"][i]["effectType"], v.subBlocks[i].effectType);
			writeNumberItem(j[key]["subBlocks"][i]["value"], v.subBlocks[i].value);
		}
		};
	

	writeRect("overlay", layout_.overlay);

	writeCardDescImage("cardDescImage", layout_.cardDescImage);

	writeCardDescCustom("cardDescCustom", layout_.cardDescCustom);

	std::ofstream ofs(path);
	if (!ofs.is_open()) {
		return false;
	}

	ofs << j.dump(4);
	return true;
}


bool FieldUi::SaveUiNumberLayout(const std::string& path) const
{
	json j;

	auto writeNumber = [&](const char* key, const UiNumber& n) {
		j[key]["x"] = n.x;
		j[key]["y"] = n.y;
		j[key]["scale"] = n.scale;
		j[key]["spacing"] = n.spacing;
		};

	auto writeRelative = [&](const char* key, const UiNumberRelative& n) {
		j[key]["offsetX"] = n.offsetX;
		j[key]["offsetY"] = n.offsetY;
		j[key]["scale"] = n.scale;
		j[key]["spacing"] = n.spacing;
		};

	writeNumber("deckCount", numberLayout_.deckCount);
	writeNumber("discardCount", numberLayout_.discardCount);
	writeNumber("handCount", numberLayout_.handCount);

	writeRelative("effect1", numberLayout_.effectValue[0]);
	writeRelative("effect2", numberLayout_.effectValue[1]);
	writeRelative("effect3", numberLayout_.effectValue[2]);

	
	std::ofstream ofs(path);
	if (!ofs.is_open()) return false;
	ofs << j.dump(4);



	return true;
}

bool FieldUi::LoadUiNumberLayout(const std::string& path)
{
	numberLayout_.deckCount = { 120.0f, 340.0f, 0.45f, 34.0f };
	numberLayout_.discardCount = { 1200.0f, 370.0f, 0.45f, 34.0f };
	numberLayout_.handCount = { 1120.0f, 660.0f, 0.45f, 34.0f };

	numberLayout_.effectValue[0] = { 150.0f, 0.0f, 0.35f, 28.0f };
	numberLayout_.effectValue[1] = { 170.0f, 0.0f, 0.35f, 28.0f };
	numberLayout_.effectValue[2] = { 170.0f, 0.0f, 0.35f, 28.0f };

	std::ifstream ifs(path);
	if (!ifs.is_open()) return false;

	json j;
	try {
		ifs >> j;
	} catch (...) {
		return false;
	}

	auto readNumber = [&](const char* key, UiNumber& n) {
		if (!j.contains(key)) return;
		auto& v = j[key];
		n.x = v.value("x", n.x);
		n.y = v.value("y", n.y);
		n.scale = v.value("scale", n.scale);
		n.spacing = v.value("spacing", n.spacing);
		};

	auto readRelative = [&](const char* key, UiNumberRelative& n) {
		if (!j.contains(key)) return;
		auto& v = j[key];
		n.offsetX = v.value("offsetX", n.offsetX);
		n.offsetY = v.value("offsetY", n.offsetY);
		n.scale = v.value("scale", n.scale);
		n.spacing = v.value("spacing", n.spacing);
		};

	readNumber("deckCount", numberLayout_.deckCount);
	readNumber("discardCount", numberLayout_.discardCount);
	readNumber("handCount", numberLayout_.handCount);

	readRelative("effect1", numberLayout_.effectValue[0]);
	readRelative("effect2", numberLayout_.effectValue[1]);
	readRelative("effect3", numberLayout_.effectValue[2]);

	return true;
}

