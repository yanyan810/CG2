#include "BattleController.h"
#include "Battle/BattleFieldViewController.h"
#include "Battle/BattleDebugImGui.h"
#include "Battle/BattleRenderView.h"
#include "Battle/BattleInfoTextProvider.h"
#include "Battle/BattleCardInputController.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include "GameApp.h"
#include "WinApp.h"
#include <Windows.h>
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "MathStruct.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <set>
#include <random>
#include <filesystem>
#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include"Player.h"
#include"Enemy.h"

#include "Card/CardEffectTextBuilder.h"
#include "Card/CardEffectExecutor.h"
#include "FieldUi.h"
#include "Audio/BattleSfxPlayer.h"
#include "ModelParticleManager.h"
#include "Poker/PokerChoiceQuery.h"
#include "Poker/PokerChoiceController.h"
#include "Poker/PokerChoiceTextBuilder.h"

namespace {
	float sPokerGlowRainbowTime = 0.0f;
	Vector3 sFieldCardGlitterLocalOffset = { 0.0f, 3.0f, 0.0f };
	float sFieldCardGlitterSpreadX = 2.0f;
	float sFieldCardGlitterSpreadY = 0.0f;
	float sFieldCardGlitterEmitInterval = 0.12f;
	int sFieldCardGlitterNormalCount = 0;
	int sFieldCardGlitterHighlightCount = 10;
	bool sFieldFrameBloomEnabled = true;
	float sFieldFrameBloomThreshold = 0.0f;
	float sFieldFrameBloomIntensity = 1.4f;
	float sFieldFrameBloomMinPulse = 0.0f;
	float sFieldFrameBloomChromAb = 0.0f;
	bool sHandPokerPreviewEnabled = true;
	float sHandCardGlitterEmitInterval = 0.12f;
	int sHandCardGlitterCount = 3;
	float sHandFrameBloomIntensity = 1.4f;
	bool sEnemyIntentBloomEnabled = true;
	float sEnemyIntentBloomIntensity = 1.8f;
	float sEnemyIntentBloomMinPulse = 0.45f;
	bool sEnemyTargetBloomEnabled = true;
	float sEnemyTargetBloomIntensity = 2.1f;
	float sEnemyTargetBloomChromAb = 0.002f;
	bool sHpGaugeBloomEnabled = true;
	float sHpGaugeBloomIntensity = 0.55f;
	float sHpGaugeBloomMinPulse = 0.65f;
	float sHpDamageBlinkSpeed = 6.0f;
	float sHpDamageBloomIntensity = 1.05f;
	bool sPlayerBlockCarryOverEnabled = true;
	float sPlayerBlockTurnDecayRate = 0.35f;
	int sFrostBurstThreshold = 15;
	int sFrostBurstMultiplier = 3;

	float EffectValueFloat_(const CardEffectDef& effect)
	{
		if (effect.valueIsFloat) {
			return effect.valueFloat;
		}
		if (effect.valueFloat != 0.0f || effect.value == 0) {
			return effect.valueFloat;
		}
		return static_cast<float>(effect.value);
	}

	int EffectValueInt_(const CardEffectDef& effect)
	{
		return std::max(0, static_cast<int>(std::lround(EffectValueFloat_(effect))));
	}

	int ScaleEffectAmount_(int baseValue, const CardEffectDef& effect)
	{
		return std::max(0, static_cast<int>(std::lround(static_cast<float>(baseValue) * EffectValueFloat_(effect))));
	}

	std::wstring FormatEffectValue_(const CardEffectDef& effect)
	{
		const float value = EffectValueFloat_(effect);
		if (effect.valueIsFloat) {
			wchar_t buffer[32]{};
			swprintf_s(buffer, L"%.2f", value);
			std::wstring text = buffer;
			while (!text.empty() && text.back() == L'0') {
				text.pop_back();
			}
			if (!text.empty() && text.back() == L'.') {
				text.pop_back();
			}
			return text;
		}
		return std::to_wstring(EffectValueInt_(effect));
	}

	Vector4 HsvToRgb_(float hue, float saturation, float value)
	{
		hue = std::fmod(hue, 360.0f);
		if (hue < 0.0f) {
			hue += 360.0f;
		}

		const float c = value * saturation;
		const float x = c * (1.0f - std::abs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
		const float m = value - c;

		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;

		if (hue < 60.0f) {
			r = c; g = x; b = 0.0f;
		} else if (hue < 120.0f) {
			r = x; g = c; b = 0.0f;
		} else if (hue < 180.0f) {
			r = 0.0f; g = c; b = x;
		} else if (hue < 240.0f) {
			r = 0.0f; g = x; b = c;
		} else if (hue < 300.0f) {
			r = x; g = 0.0f; b = c;
		} else {
			r = c; g = 0.0f; b = x;
		}

		return { r + m, g + m, b + m, 1.0f };
	}

	BattleFieldViewController::FieldLayoutParams MakeFieldLayoutParams_(
		const BattleController::FieldCardLayout& layout)
	{
		BattleFieldViewController::FieldLayoutParams params{};
		params.y = layout.y;
		params.z = layout.z;
		params.gap = layout.gap;
		params.scale = layout.scale;
		params.hoverYOffset = layout.hoverYOffset;
		params.hoverZOffset = layout.hoverZOffset;
		params.hoverScale = layout.hoverScale;
		return params;
	}

	BattleController::PokerMouseChoice ToPokerMouseChoice_(PokerChoiceController::Choice choice)
	{
		switch (choice) {
		case PokerChoiceController::Choice::ActivateYes:       return BattleController::PokerMouseChoice::ActivateYes;
		case PokerChoiceController::Choice::ActivateNo:        return BattleController::PokerMouseChoice::ActivateNo;
		case PokerChoiceController::Choice::ActivateViewBoard: return BattleController::PokerMouseChoice::ActivateViewBoard;
		case PokerChoiceController::Choice::EffectAtkUp:       return BattleController::PokerMouseChoice::EffectAtkUp;
		case PokerChoiceController::Choice::EffectDraw:        return BattleController::PokerMouseChoice::EffectDraw;
		case PokerChoiceController::Choice::EffectDamage:      return BattleController::PokerMouseChoice::EffectDamage;
		case PokerChoiceController::Choice::EffectBack:        return BattleController::PokerMouseChoice::EffectBack;
		case PokerChoiceController::Choice::EffectViewBoard:   return BattleController::PokerMouseChoice::EffectViewBoard;
		case PokerChoiceController::Choice::ReturnFromBoard:   return BattleController::PokerMouseChoice::ReturnFromBoard;
		case PokerChoiceController::Choice::None:
		default:                                               return BattleController::PokerMouseChoice::None;
		}
	}

	BloomParam MakeEnemyTargetBloomParam_(const BloomParam& baseParam, float time)
	{
		const float pulse = 0.72f + 0.28f * (0.5f + 0.5f * std::sin(time * 5.0f));
		BloomParam param = baseParam;
		param.threshold = 0.0f;
		param.intensity = sEnemyTargetBloomIntensity * pulse;
		param.vignetteIntensity = 0.0f;
		param.vignetteScale = 0.0f;
		param.chromAbAmount = sEnemyTargetBloomChromAb;
		param.distortionAmount = 0.0f;
		param.noiseIntensity = 0.0f;
		param.scanlineIntensity = 0.0f;
		param.curvature = 0.0f;
		param.borderSharp = 0.0f;
		param.glitchAmount = 0.0f;
		param.dissolveAmount = -1.0f;
		return param;
	}

}

//===============================
//鬮ｯ貅ｷ遘√・・ｽ繝ｻ・ｹ
//===============================

const char* BattleController::GetPokerHandName_(PokerHandRank rank) const
{
	return PokerHandEvaluator::GetHandName(rank);
}

BattleController::PokerHandResult BattleController::EvaluatePokerHand_() const
{
	return PokerHandEvaluator::Evaluate(field_);
}

BattleController::PokerHandResult BattleController::EvaluatePokerHandForCards_(const std::vector<CardInstance>& cards) const
{
	return PokerHandEvaluator::Evaluate(cards);
}

void BattleController::RebuildDiscardView_()
{
	discardView_.reset();

	const auto& discard = deckZone_.GetDiscard();
	if (discard.empty()) {
		return;
	}

	const CardInstance& top = discard.back();
	const CardDef* def = db_.Find(top.defId);
	if (!def) {
		return;
	}

	discardView_ = std::make_unique<Card3D>();
	discardView_->Initialize(objCom_, dx_, cam_, *def, top);

	// 鬮｣蜴・ｽｽ・ｴ髯ｷ・･繝ｻ・ｲ郢晢ｽｻ繝ｻ・ｽ郢晢ｽｻ繝ｻ・ｮ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｯ鬮ｯ・ｷ繝ｻ・ｿ郢晢ｽｻ繝ｻ・ｳ鬮｣蛹・ｽｽ・ｳ髯ｷ・ｿ繝ｻ・･郢晢ｽｻ繝ｻ・ｯ驛｢譎｢・ｽ・ｻ郢晢ｽｻ鬯倩ｲｻ・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｸ鬩搾ｽｵ繝ｻ・ｲ驛｢・ｧ郢晢ｽｻ隴鯉ｽｺ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｨ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬯ｮ・ｫ繝ｻ・ｱ郢晢ｽｻ繝ｻ・ｿ鬮ｫ・ｰ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｴ
	Vector3 pos{ 16.0f, -8.0f, 6.0f };
	Vector3 rot{ 0.0f, 0.0f, 1.5707963f };
	Vector3 scl{ 1.0f, 1.0f, 1.0f };

	discardView_->SetTransform(pos, rot, scl);
}

void BattleController::ConsumeFieldCards_()
{
	for (auto& view : fieldViews_) {
		if (view) {
			handView_.AddDiscardingCard(std::move(view));
		}
	}
	fieldViews_.clear();

	deckZone_.AddManyToDiscard(field_);
	field_.clear();
	RebuildDiscardView_();
	fieldLayoutDirty_ = true;
}

namespace {
	int RandomRangeInt(int minValue, int maxValue)
	{
		static std::random_device rd;
		static std::mt19937 mt(rd());
		std::uniform_int_distribution<int> dist(minValue, maxValue);
		return dist(mt);
	}

	CardSuit RandomSuit()
	{
		int v = RandomRangeInt(0, 3);
		return static_cast<CardSuit>(v);
	}

	CardInstance MakeCardInstance(int defId)
	{
		CardInstance c{};
		c.defId = defId;
		c.number = RandomRangeInt(1, 13);
		c.suit = RandomSuit();
		return c;
	}

	const char* SuitToString(CardSuit suit)
	{
		switch (suit) {
		case CardSuit::Spade:   return "Spade";
		case CardSuit::Heart:   return "Heart";
		case CardSuit::Diamond: return "Diamond";
		case CardSuit::Club:    return "Club";
		default:                return "?";
		}
	}

	bool PointInRect(int mx, int my, float x, float y, float w, float h)
	{
		return mx >= x && mx <= x + w &&
			my >= y && my <= y + h;
	}

	std::wstring Utf8ToWString(const std::string& s) {
		if (s.empty()) {
			return L"";
		}

		int sizeNeeded = MultiByteToWideChar(
			CP_UTF8, 0, s.c_str(), -1, nullptr, 0
		);
		if (sizeNeeded <= 0) {
			return L"";
		}

		std::wstring result(sizeNeeded - 1, L'\0');
		MultiByteToWideChar(
			CP_UTF8, 0, s.c_str(), -1, result.data(), sizeNeeded
		);
		return result;
	}

}

void BattleController::UpdateCostViewTransform_(float dt)
{
	const int count = (int)costDigitModels_.size();
	if (count <= 0) return;

	const float gap = 1.2f;
	const float startX = -gap * 0.5f * (count - 1);

	const float baseX = -14.5f;  // 鬮ｯ譎｢・ｽ・ｾ郢晢ｽｻ繝ｻ・ｦ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｸ
	const float baseY = -6.8f;   // 鬮ｯ譏ｴ繝ｻ繝ｻ・ｻ繝ｻ・｣郢晢ｽｻ繝ｻ・ｰ鬮｣蛹・ｽｽ・ｳ驛｢譎｢・ｽ・ｻ
	const float baseZ = 6.0f;    // 鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｾ鬩搾ｽｵ繝ｻ・ｺ髯橸ｽ｢繝ｻ・ｹ驛｢譎｢・ｽ・ｻ鬩搾ｽｵ繝ｻ・ｺ髫ｴ謫ｾ・ｽ・ｴ驛｢譎｢・ｽ・ｻ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｾ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｾ

	for (int i = 0; i < count; ++i) {
		Vector3 pos{ baseX + startX + gap * i, baseY, baseZ };
		Vector3 rot{ 0.0f, 0.0f, 0.0f };
		Vector3 scl{ 0.8f, 0.8f, 0.8f };

		costDigitModels_[i]->SetRotate(rot);
		costDigitModels_[i]->SetTranslate(pos);
		costDigitModels_[i]->SetScale(scl);

		costDigitModels_[i]->Update(dt);
	}
}

void BattleController::ShuffleDeck_()
{
	deckZone_.ShuffleDeck();
}

void BattleController::RebuildCostView_(float dt)
{
	costDigitModels_.clear();

	std::string text = std::to_string(energy_) + "/" + std::to_string(energyMax_);

	for (char ch : text) {
		std::string path;

		if (ch >= '0' && ch <= '9') {
			path = "cards/models/";
			path += ch;
			path += ".obj";
		} else if (ch == '/') {
			path = "cards/models/slash.obj";
		} else {
			continue;
		}

		auto obj = std::make_unique<Object3d>();
		obj->Initialize(objCom_, dx_);

		obj->SetModel(path);
		obj->SetCamera(cam_);

		costDigitModels_.push_back(std::move(obj));
	}

	prevEnergy_ = energy_;
	prevEnergyMax_ = energyMax_;

	UpdateCostViewTransform_(dt);
}

std::array<bool, 5> BattleController::GetPokerHighlightMask_() const
{
	std::array<bool, 5> mask{};
	mask.fill(false);

	if (field_.size() != 5) {
		return mask;
	}

	PokerHandResult result = EvaluatePokerHand_();
	if (result.rank == PokerHandRank::None) {
		return mask;
	}

	std::array<int, 14> countNumber{};
	std::array<int, 4> countSuit{};

	for (const auto& c : field_) {
		if (c.number >= 1 && c.number <= 13) {
			countNumber[c.number]++;
		}
		int suitIndex = static_cast<int>(c.suit);
		if (suitIndex >= 0 && suitIndex < 4) {
			countSuit[suitIndex]++;
		}
	}

	auto markNumber = [&](int number) {
		for (int i = 0; i < 5; ++i) {
			if (field_[i].number == number) {
				mask[i] = true;
			}
		}
		};

	auto markSuit = [&](CardSuit suit) {
		for (int i = 0; i < 5; ++i) {
			if (field_[i].suit == suit) {
				mask[i] = true;
			}
		}
		};

	switch (result.rank) {
	case PokerHandRank::OnePair:
		for (int n = 1; n <= 13; ++n) {
			if (countNumber[n] == 2) {
				markNumber(n);
				break;
			}
		}
		break;

	case PokerHandRank::TwoPair:
		for (int n = 1; n <= 13; ++n) {
			if (countNumber[n] == 2) {
				markNumber(n);
			}
		}
		break;

	case PokerHandRank::ThreeOfAKind:
		for (int n = 1; n <= 13; ++n) {
			if (countNumber[n] == 3) {
				markNumber(n);
				break;
			}
		}
		break;

	case PokerHandRank::FourOfAKind:
		for (int n = 1; n <= 13; ++n) {
			if (countNumber[n] == 4) {
				markNumber(n);
				break;
			}
		}
		break;

	case PokerHandRank::FullHouse:
		for (int n = 1; n <= 13; ++n) {
			if (countNumber[n] == 3 || countNumber[n] == 2) {
				markNumber(n);
			}
		}
		break;

	case PokerHandRank::Flush:
	case PokerHandRank::StraightFlush:
	case PokerHandRank::RoyalStraightFlush:
		for (int s = 0; s < 4; ++s) {
			if (countSuit[s] == 5) {
				markSuit(static_cast<CardSuit>(s));
				break;
			}
		}
		break;

	case PokerHandRank::Straight:
		// 鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩幢ｽ｢隴主・讓溘・蜿厄ｽｨ謚ｵ・ｽ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴主・讓滄Δ譎｢・ｽ・ｻ5鬮ｫ・ｴ繝ｻ・ｫ髯橸ｽ｢繝ｻ・ｼ驛｢譎｢・ｽ・ｻ鬯ｯ・ｩ陝ｷ・｢繝ｻ・ｽ繝ｻ・ｨ鬮｣蜴・ｽｽ・ｴ郢晢ｽｻ繝ｻ・ｿ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
		for (int i = 0; i < 5; ++i) {
			mask[i] = true;
		}
		break;

	default:
		break;
	}

	return mask;
}

const CardDef* BattleController::FindCardDef(int id) const
{
	return db_.Find(id);
}

void BattleController::PreloadCardAssets_()
{
	// 鬮ｯ・ｷ髣鯉ｽｨ繝ｻ・ｽ繝ｻ・ｱ鬯ｯ・ｨ繝ｻ・ｾ髯橸ｽ｢繝ｻ・ｹ驍ｵ・ｲ陞ｳ螢ｽ蜑ｲ郢晢ｽｻ繝ｻ・ｿ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ繝ｻ螳亥擠繝ｻ・ｹ隴擾ｽｴ郢晢ｽｻ繝ｻ蜿門旭繝ｻ・ｹ繝ｻ・ｧ鬮ｮ蛹ｺ・ｧ・ｭ郢晢ｽｻ鬯ｮ・ｫ繝ｻ・ｱ郢晢ｽｻ繝ｻ・ｭ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｿ
	ModelManager::GetInstance()->LoadModel("cards/models/frame.obj");
	ModelManager::GetInstance()->LoadModel("cards/models/art_plane.obj");

	// 鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ
	ModelManager::GetInstance()->LoadModel("cards/models/spade.obj");
	ModelManager::GetInstance()->LoadModel("cards/models/heart.obj");
	ModelManager::GetInstance()->LoadModel("cards/models/daiya.obj");
	ModelManager::GetInstance()->LoadModel("cards/models/clover.obj");

	// 鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｳ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩幢ｽ｢隴惹ｹ暦ｽｲ・ｺ髴取ｺｷ・､鬆托ｽｰ蟶ｷ・ｹ譎｢・ｽ・ｻ
	for (int i = 0; i <= 9; ++i) {
		ModelManager::GetInstance()->LoadModel("cards/models/" + std::to_string(i) + ".obj");
	}
	ModelManager::GetInstance()->LoadModel("cards/models/slash.obj");

	// cards.json 鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬯ｮ・ｴ闔・･繝ｻ・ｳ繝ｻ・ｨ髫ｨ繝ｻ・ｽ・ｲ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ霑｢證ｦ・ｽ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ驛｢譎｢・ｽ・ｻ鬯ｨ・ｾ陋ｹ繝ｻ・ｽ・ｽ繝ｻ・ｻ鬮ｯ・ｷ陷代・・ｽ・ｸ陝ｯ・ｩ郢晢ｽｻ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・｢鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ繝ｻ蜿門旭繝ｻ・ｹ繝ｻ・ｧ鬮ｮ蛹ｺ・ｧ・ｭ郢晢ｽｻ鬯ｯ・ｩ陝ｷ・｢繝ｻ・ｽ繝ｻ・ｨ鬮ｯ・ｷ騾趣ｽｯ郢晢ｽ｡郢晢ｽｻ繝ｻ・ｪ郢晢ｽｻ繝ｻ・ｭ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｿ
	for (int id = 1; id <= 100; ++id) {
		const CardDef* def = db_.Find(id);
		if (!def) {
			continue;
		}

		ModelManager::GetInstance()->LoadModel(def->frameModel);
		ModelManager::GetInstance()->LoadModel(def->artModel);
		TextureManager::GetInstance()->LoadTexture(def->artTex);
	}
}

void BattleController::Preload(GameApp& app)
{
	objCom_ = app.ObjCom();
	dx_ = app.Dx();
	spriteCom_ = app.SpriteCom();

	if (!cardDbLoaded_) {
		db_ = *app.GetCardDB();
		cardDbLoaded_ = true;
	}

	if (!assetsPreloaded_) {
		PreloadCardAssets_();

		// 鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驛｢譎｢・ｽ・｣鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｭ鬮ｯ讖ｸ・ｽ・ｳ髯樊ｺ假ｽ代・・ｽ繝ｻ・ｾ郢晢ｽｻ繝ｻ・ｩ鬩幢ｽ｢繝ｻ・ｧ驛｢・ｧ郢晢ｽｻ繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｸ繝ｻ・ｺ鬮ｦ・ｮ陷ｷ・ｶ・つ陝ｶ譎乗套郢晢ｽｻ繝ｻ・ｭ鬩幢ｽ｢繝ｻ・ｧ鬮ｦ・ｮ陷ｷ・ｶ・つ陜｣・､繝ｻ・ｸ繝ｻ・ｺ鬩怜遜・ｽ・ｫ郢晢ｽｻ繝ｻ・･鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｨ鬩搾ｽｵ繝ｻ・ｲ驕ｶ荳橸ｽ｣・ｹ繝ｻ繝ｻﾎ碑ｭ趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・｣繝ｻ・ｰ鬯ｯ・ｮ繝ｻ・｢髯ｷ・ｿ繝ｻ・･郢晢ｽｻ繝ｻ・ｧ髯具ｽｹ繝ｻ・ｺ髯ｷ繝ｻ・ｽ・ｾ鬩搾ｽｵ繝ｻ・ｺ髯滓坩・ｯ莨夲ｽｽ・ｼ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ髯晢ｽｲ繝ｻ・ｨ驕ｶ莨・ｽｦ・ｴ隲､霈斐・繝ｻ・ｽ鬩搾ｽｵ繝ｻ・ｺ髣包ｽｳ陝ｯ・ｩ郢晢ｽｻ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ
		DeckDef deckDef{};
		std::string err;

		prebuiltDeck_.clear();

		/*if (DeckLoader::LoadFromJson("resources/cards/deck/deck.json", deckDef) &&
			DeckLoader::ValidateDeck(deckDef, db_, err)) {

			for (const auto& e : deckDef.cards) {
				for (int i = 0; i < e.count; ++i) {
					prebuiltDeck_.push_back(MakeCardInstance(e.id));
				}
			}
		} else {
			for (int i = 0; i < 4; ++i) {
				prebuiltDeck_.push_back(MakeCardInstance(9));
				prebuiltDeck_.push_back(MakeCardInstance(8));
				prebuiltDeck_.push_back(MakeCardInstance(7));
				prebuiltDeck_.push_back(MakeCardInstance(6));
				prebuiltDeck_.push_back(MakeCardInstance(5));
				prebuiltDeck_.push_back(MakeCardInstance(4));
				prebuiltDeck_.push_back(MakeCardInstance(3));
				prebuiltDeck_.push_back(MakeCardInstance(2));
				prebuiltDeck_.push_back(MakeCardInstance(1));
				prebuiltDeck_.push_back(MakeCardInstance(20));
				prebuiltDeck_.push_back(MakeCardInstance(19));
				prebuiltDeck_.push_back(MakeCardInstance(18));
				prebuiltDeck_.push_back(MakeCardInstance(17));
				prebuiltDeck_.push_back(MakeCardInstance(16));
				prebuiltDeck_.push_back(MakeCardInstance(15));
				prebuiltDeck_.push_back(MakeCardInstance(14));
				prebuiltDeck_.push_back(MakeCardInstance(13));
				prebuiltDeck_.push_back(MakeCardInstance(12));
				prebuiltDeck_.push_back(MakeCardInstance(11));
				prebuiltDeck_.push_back(MakeCardInstance(10));
			}
		}*/

		for (CardInstance instance : app.GetDeckInstances()) {
			prebuiltDeck_.push_back(instance);
		}

		assetsPreloaded_ = true;
	}
}

void BattleController::Initialize(GameApp& app, Camera* camera)
{
	cam_ = camera;
	objCom_ = app.ObjCom();
	dx_ = app.Dx();
	spriteCom_ = app.SpriteCom();

	// 鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｾ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬮ｯ・ｷ騾趣ｽｯ郢晢ｽ｡郢晢ｽｻ繝ｻ・ｪ郢晢ｽｻ繝ｻ・ｭ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｿ鬩搾ｽｵ繝ｻ・ｺ鬮ｴ驛・ｽｲ・ｻ繝ｻ・ｽ隶呵ｶ｣・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ驕ｶ莨√・繝ｻ・ｸ繝ｻ・ｺ髣比ｼ夲ｽｽ・｣郢晢ｽｻ隶呵ｶ｣・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ鬮ｦ・ｮ陷ｻ・ｻ繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬮｣蜴・ｽｽ・ｫ髫ｴ蜿厄ｽｧ・ｫ鬯ｨ鬥ｴ縺励・・ｺ郢晢ｽｻ繝ｻ・ｨ鬩搾ｽｵ繝ｻ・ｺ髯ｷ莨夲ｽｽ・ｱ驕ｯ・ｶ繝ｻ・ｻ鬮ｯ讖ｸ・ｽ・ｳ髮九・・ｽ・ｯ郢晢ｽｻ繝ｻ・｡驛｢譎｢・ｽ・ｻ
	Preload(app);
	LoadFieldCardLayout(fieldCardLayoutPath_);

	actionDirector_.Initialize(spriteCom_, dx_, objCom_);

	damagePopupUi_.Initialize(objCom_, dx_, cam_);
	playerStatusUi_.Initialize(app);
	enemyStatusUi_.Initialize(app, 3);
	playerLastHp_ = player_ ? player_->GetHP() : -1;

	// 鬩搾ｽｵ繝ｻ・ｺ鬮ｦ・ｮ陷ｻ・ｻ繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｯ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｳ鬩幢ｽ｢隴弱・・ｱ蝣､・ｹ譎｢・ｽ・ｻ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
	deckZone_.SetDeck(prebuiltDeck_);
	ShuffleDeck_();

	if (useTutorialOpeningHand_) {
		// 鬮ｫ・ｴ陝・｢・つ鬮ｯ蜈ｷ・ｽ・ｻ髫ｴ謫ｾ・ｽ・ｴ驕ｶ鬆托ｽ･・｢繝ｻ・ｰ鬯倬ｯ会ｽｽ・ｼ髮具ｽｻ繝ｻ・ｿ繝ｻ・･5鬮ｫ・ｴ繝ｻ・ｫ髯橸ｽ｢繝ｻ・ｹ郢晢ｽｻ郢晢ｽｻdeck 鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮ｫ・ｴ陝ｷ・｢繝ｻ・ｽ繝ｻ・ｫ鬮ｯ譏ｴ繝ｻ繝ｻ・ｽ繝ｻ・ｾ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬯ｩ蛹・ｽｽ・ｨ鬯ｮ・ｦ繝ｻ・ｪ驛｢譎｢・ｽ・ｻ鬩搾ｽｵ繝ｻ・ｺ髮九・竏槭・・ｽ遶擾ｽｫ繝ｻ・ｸ繝ｻ・ｲ驛｢譎｢・ｽ・ｻ
		// 鬮ｯ・ｷ闔・･霑ｴ・ｾ驕ｶ鬆托ｽ･・｢隲・ｺ髯溷供ﾂ蜈ｷ・ｽ・ｧ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ驕ｯ・ｶ繝ｻ・ｲ鬩搾ｽｵ繝ｻ・ｺ驛｢・ｧ郢晢ｽｻ繝ｻ・ｽ隶呵ｶ｣・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬯ｮ・ｴ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｽ鬩搾ｽｵ繝ｻ・ｺ髣包ｽｳ隶抵ｽｫ陟募ｮ｣ﾎ斐・・ｧ髣費ｽｨ遶乗刋・ｱ繧九＠繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
		for (const auto& fixedCard : tutorialOpeningHand_) {
			deckZone_.RemoveFirstFromDeckByDefId(fixedCard.defId);
		}

		// DrawOne_ 鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｯ back() 鬩幢ｽ｢繝ｻ・ｧ鬮ｮ蛹ｺ・ｩ・ｸ繝ｻ・ｽ繝ｻ・ｼ鬮ｴ驛・ｽｲ・ｻ繝ｻ・ｿ繝ｻ・･鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬩搾ｽｵ繝ｻ・ｲ驛｢譎｢・ｽ・ｻ繝ｻ縺､ﾂ驛｢譎｢・ｽ・ｻ郢晢ｽｻ繝ｻ・ｰ驛｢譎｢・ｽ・ｻ驍ｵ・ｲ陝ｶ譏ｶ繝ｻ鬯ｮ・ｦ繝ｻ・ｪ驛｢譎｢・ｽ・ｻ
		for (auto it = tutorialOpeningHand_.rbegin(); it != tutorialOpeningHand_.rend(); ++it) {
			deckZone_.PushDeckBack(*it);
		}
	}

	deckZone_.ClearHand();
	deckZone_.ClearDiscard();
	field_.clear();
	fieldViews_.clear();
	damagePopupUi_.Clear();

	hasPendingCard_ = false;
	pendingCard_ = {};
	currentEnemyIndex_ = 0;
	nextTurnAtkUp_ = 0;
	currentTurnAtkUp_ = 0;
	playerTurnCount_ = 0;
	enemyTurnCount_ = 0;

	energy_ = energyMax_;

	handView_.Initialize(objCom_, dx_, cam_, &db_);
	handView_.Clear();

	StartPlayerTurn_();
	RebuildDiscardView_();
	RebuildCostView_(deltaTime_);

	// 鬩幢ｽ｢隴乗・・ｽ・ｸ驗呻ｽｫ遶包ｽｧ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｩ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬩幢ｽ｢隴寂・・・ｨｾ・｡髫ｧ・ｮilter
	highlightFilter_ = std::make_unique<Sprite>();
	highlightFilter_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	highlightFilter_->SetPosition({ 0.0f, 0.0f });
	highlightFilter_->SetScale({ 1280.0f, 1280.0f, 1.0f });
	highlightFilter_->SetColor({ 0.0f, 0.0f, 0.0f, 0.8f });

	propManager_ = std::make_unique<PropManager>();
	propManager_->Initialize(objCom_, dx_, cam_);
	propManager_->LoadFromJson("resources/configs/sceneProps.json");

	tutorialLockPokerTargetingCancel_ = false;

}

bool BattleController::DrawOne_()
{
	CardInstance card{};
	bool reshuffledDiscard = false;
	if (!deckZone_.DrawOne(card, reshuffledDiscard)) {
		return false;
	}

	if (reshuffledDiscard) {
		RebuildDiscardView_();
	}

	// 鬩搾ｽｵ繝ｻ・ｺ鬮ｦ・ｮ陷ｻ・ｻ繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ髯橸ｽｳ陞滂ｽｲ繝ｻ・ｽ繝ｻ・ｿ郢晢ｽｻ繝ｻ・ｽ鬮ｯ・ｷ闔ｨ螟ｲ・ｽ・｣繝ｻ・ｰ
	handView_.AddCard(card);

	return true;
}

void BattleController::DrawTurnStartCards_()
{
	const int drawCount = (playerTurnCount_ == 1) ? 5 : 3;
	for (int i = 0; i < drawCount; ++i) {
		if (!DrawOne_()) {
			break;
		}
	}
	//handView_.Rebuild(hand_);
}

void BattleController::DrawCards_(int count)
{
	for (int i = 0; i < count; ++i) {
		if (!DrawOne_()) {
			break;
		}
	}
	//	handView_.Rebuild(hand_);
}

Enemy* BattleController::ResolveTargetEnemy_(int targetIndex) const
{
	if (!enemyMgr_) {
		return nullptr;
	}

	auto& enemies = enemyMgr_->GetEnemies();
	if (targetIndex >= 0 && targetIndex < static_cast<int>(enemies.size())) {
		return &enemies[static_cast<std::size_t>(targetIndex)];
	}

	for (auto& enemy : enemies) {
		if (enemy.IsAlive()) {
			return &enemy;
		}
	}

	return nullptr;
}

void BattleController::ApplyFrostBiteToEnemyIfActive_(Enemy& enemy)
{
	if (player_ && player_->GetFrostBiteActive()) {
		enemy.SetBC(Enemy::BadCondition::kFrost);
		enemy.AddBC(1);
	}
}

void BattleController::ApplyDamageEffect_(const CardEffectDef& effect, int targetIndex, bool applyAttackBuff)
{
	if (enemyMgr_) {
		const bool hasExplicitTarget =
			targetIndex >= 0 && targetIndex < static_cast<int>(enemyMgr_->GetEnemies().size());
		Enemy* targetEnemy = ResolveTargetEnemy_(targetIndex);
		if (!targetEnemy || !targetEnemy->IsAlive()) return;

		int totalDamage = applyAttackBuff ? CalcFinalAttackDamage_(EffectValueInt_(effect)) : EffectValueInt_(effect);

		if (hasExplicitTarget && targetEnemy->GetBC() == Enemy::BadCondition::kFrost) {
			totalDamage += targetEnemy->GetBCPoint();
		}

		if (hasExplicitTarget) {
			ApplyFrostBiteToEnemyIfActive_(*targetEnemy);
		}

		const int actualDamage = ApplyDamageToEnemy_(*targetEnemy, totalDamage);
		if (!hasExplicitTarget) {
			ApplyFrostBiteToEnemyIfActive_(*targetEnemy);
		}
		if (totalDamage > 0) SpawnDamagePopup(targetEnemy->GetPos(), actualDamage, false);

	}
}

void BattleController::ApplyDamageCrescentEffect_(const CardEffectDef& effect, int targetIndex, bool applyAttackBuff)
{
	if (enemyMgr_) {
		int baseVal = EffectValueInt_(effect);
		if (playerTurnCount_ % 2 != 0) {
			baseVal += 3;
		}

		int finalDamage = applyAttackBuff ? CalcFinalAttackDamage_(baseVal) : std::max(0, baseVal);

		Enemy* targetEnemy = ResolveTargetEnemy_(targetIndex);
		if (targetEnemy && targetEnemy->IsAlive()) {
			const int actualDamage = ApplyDamageToEnemy_(*targetEnemy, finalDamage);
			if (finalDamage > 0) SpawnDamagePopup(targetEnemy->GetPos(), actualDamage, false);
			ApplyFrostBiteToEnemyIfActive_(*targetEnemy);
		}

	}
}

void BattleController::ApplyDamageByBlockEffect_(const CardEffectDef& effect, int targetIndex, bool applyAttackBuff)
{
	if (enemyMgr_) {
		int baseVal = ScaleEffectAmount_(player_ ? player_->GetBlock() : 0, effect);
		int finalDamage = applyAttackBuff ? CalcFinalAttackDamage_(baseVal) : baseVal;
		Enemy* targetEnemy = ResolveTargetEnemy_(targetIndex);
		if (targetEnemy && targetEnemy->IsAlive()) {
			const int actualDamage = ApplyDamageToEnemy_(*targetEnemy, finalDamage);
			if (finalDamage > 0) SpawnDamagePopup(targetEnemy->GetPos(), actualDamage, false);
			ApplyFrostBiteToEnemyIfActive_(*targetEnemy);
		}

	}
}

void BattleController::ApplyDamageAllEffect_(const CardEffectDef& effect, bool applyAttackBuff)
{
	if (enemyMgr_ && player_) {
		int totalDamage = applyAttackBuff ? CalcFinalAttackDamage_(EffectValueInt_(effect)) : EffectValueInt_(effect);

		player_->PlayAttackAnimWithEffect(player_->GetPos(), -1);

		int hitCount = 0;
		for (auto& e : enemyMgr_->GetEnemies()) {
			if (e.IsAlive()) {
				e.TriggerHitFlash(0.2f);
				e.PlayDamageAnim();
				const int beforeBlock = e.GetBlock();
				const int actualDamage = e.Damage(totalDamage);
				BattleSfxPlayer::PlayBlockReactionSE(beforeBlock, e.GetBlock(), actualDamage, totalDamage);
				if (totalDamage > 0) {
					SpawnDamagePopup(e.GetPos(), actualDamage, false);
				}
				ApplyFrostBiteToEnemyIfActive_(e);
				hitCount++;
			}
		}

		if (hitCount > 0 && player_->GetVampireHeal() > 0) {
			int beforeHp = player_->GetHP();
			player_->Heal(player_->GetVampireHeal() * hitCount);
			BattleSfxPlayer::PlayHealSEIfHpIncreased(player_, beforeHp);
		}

	}
}

void BattleController::ApplyDrawEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.drawCards = [this](int count) {
		DrawCards_(count);
	};
	CardEffectExecutor::ApplyDraw(context, effect);
}

void BattleController::ApplyBlockEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.player = player_;
	CardEffectExecutor::ApplyBlock(context, effect);
}

void BattleController::ApplyPowerBoostEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.player = player_;
	CardEffectExecutor::ApplyPowerBoost(context, effect);
}

void BattleController::ApplyNextTurnAtkUpEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.nextTurnAtkUp = &nextTurnAtkUp_;
	CardEffectExecutor::ApplyNextTurnAtkUp(context, effect);
}

void BattleController::ApplyHealEffect_(const CardEffectDef& effect)
{
	if (!player_) {
		return;
	}
	int beforeHp = player_->GetHP();
	player_->Heal(EffectValueInt_(effect));
	BattleSfxPlayer::PlayHealSEIfHpIncreased(player_, beforeHp);
}

void BattleController::ApplyHealByBlockEffect_(const CardEffectDef& effect)
{
	if (!player_) {
		return;
	}
	int healAmount = ScaleEffectAmount_(player_->GetBlock(), effect);
	int beforeHp = player_->GetHP();
	player_->Heal(healAmount);
	BattleSfxPlayer::PlayHealSEIfHpIncreased(player_, beforeHp);
}

void BattleController::ApplyHealByLowCostInHandEffect_(const CardEffectDef& effect)
{
	if (!player_) {
		return;
	}

	int count = 0;
	for (const auto& cardInst : deckZone_.GetHand()) {
		const CardDef* cDef = db_.Find(cardInst.defId);
		if (cDef && cDef->cost == 1) {
			count++;
		}
	}

	const int healAmount = ScaleEffectAmount_(count, effect);
	if (healAmount > 0) {
		int beforeHp = player_->GetHP();
		player_->Heal(healAmount);
		BattleSfxPlayer::PlayHealSEIfHpIncreased(player_, beforeHp);
	}
}

void BattleController::ApplyVampireBuffEffect_(const CardEffectDef& effect)
{
	if (player_) {
		player_->AddVampireHeal(EffectValueInt_(effect));
	}
}

void BattleController::ApplySelfDamageEffect_(const CardEffectDef& effect)
{
	if (player_) {
		player_->TriggerHitFlash(0.2f);
		player_->PlayDamageAnim();
		player_->Damage(EffectValueInt_(effect));
	}
}

void BattleController::ApplyPoisonEffect_(const CardEffectDef& effect, int targetIndex)
{
	if (enemyMgr_) {
		if (targetIndex >= 0 && targetIndex < enemyMgr_->GetEnemies().size()) {
			auto& e = enemyMgr_->GetEnemies()[targetIndex];
			if (e.IsAlive()) {
				e.SetBC(Enemy::BadCondition::kPoison);
				e.AddBC(effect.value);
			}
		} else {
			for (auto& e : enemyMgr_->GetEnemies()) {
				if (e.IsAlive()) {
					e.SetBC(Enemy::BadCondition::kPoison);
					e.AddBC(effect.value);
					break;
				}
			}
		}
	}
	if (player_->GetPoisonDrawActive()) {
		DrawCards_(1);
	}
}

void BattleController::ApplyPoisonAllEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.player = player_;
	context.enemyMgr = enemyMgr_;
	context.drawCards = [this](int count) {
		DrawCards_(count);
	};
	CardEffectExecutor::ApplyPoisonAll(context, effect);
}

void BattleController::ApplyPoisonAmplifyEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyPoisonAmplify(context, effect);
}

void BattleController::ApplyPoisonDamageEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyPoisonDamage(context, effect);
}

void BattleController::ApplyPoisonDrawEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.player = player_;
	context.enemyMgr = enemyMgr_;
	context.drawCards = [this](int count) {
		DrawCards_(count);
	};
	CardEffectExecutor::ApplyPoisonDraw(context, effect);
}

void BattleController::ApplyPoisonRemoveEffect_()
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyPoisonRemove(context);
}

void BattleController::ApplyPoisonHealEffect_()
{
	int healAmount = 0;

	if (enemyMgr_) {
		for (auto& e : enemyMgr_->GetEnemies()) {
			if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kPoison) {
				e.SetBC(Enemy::BadCondition::kPoison);
				healAmount += e.GetBCPoint();
			}
		}
	}

	int beforeHp = player_->GetHP();
	player_->Heal(healAmount);
	BattleSfxPlayer::PlayHealSEIfHpIncreased(player_, beforeHp);
}

void BattleController::ApplyFrostEffect_(const CardEffectDef& effect, int targetIndex)
{
	if (enemyMgr_) {
		if (targetIndex >= 0 && targetIndex < enemyMgr_->GetEnemies().size()) {
			auto& e = enemyMgr_->GetEnemies()[targetIndex];
			if (e.IsAlive()) {
				e.SetBC(Enemy::BadCondition::kFrost);
				e.AddBC(effect.value);
			}
		} else {
			for (auto& e : enemyMgr_->GetEnemies()) {
				if (e.IsAlive()) {
					e.SetBC(Enemy::BadCondition::kFrost);
					e.AddBC(effect.value);
					break;
				}
			}
		}
	}
}

void BattleController::ApplyFrostAllEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyFrostAll(context, effect);
}

void BattleController::ApplyFrostBlockEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.player = player_;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyFrostBlock(context, effect);
}

void BattleController::ApplyFrostDamageEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyFrostDamage(context, effect);
}

void BattleController::ApplyFrostSubtractEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyFrostSubtract(context, effect);
}

void BattleController::ApplyFrostBiteEffect_(const CardEffectDef& effect)
{
	bool isActivated = player_->GetFrostBiteActive();

	if (!isActivated) {
		player_->SetFrostBiteActive(true);
	} else {
		if (enemyMgr_) {
			for (auto& e : enemyMgr_->GetEnemies()) {
				if (e.IsAlive()) {
					e.SetBC(Enemy::BadCondition::kFrost);
					e.AddBC(effect.value);
				}
			}
		}
	}
}

void BattleController::ApplyFrostAmplifyEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyFrostAmplify(context, effect);
}

void BattleController::ApplyChangeNumberEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::ApplyChangeNumber(effect);
}

void BattleController::ApplyChangeSuitEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::ApplyChangeSuit(effect);
}

void BattleController::ApplyFrostDrawEffect_(int targetIndex)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	context.drawCards = [this](int count) {
		DrawCards_(count);
	};
	CardEffectExecutor::ApplyFrostDraw(context, targetIndex);
}

void BattleController::ApplyEffectsList_(const std::vector<CardEffectDef>& effects, int targetIndex, bool applyAttackBuff)
{
	for (const auto& effect : effects) {
		if (effect.type == "Draw") {
			ApplyDrawEffect_(effect);

		} else if (effect.type == "Damage") {
			ApplyDamageEffect_(effect, targetIndex, applyAttackBuff);

		} else if (effect.type == "DamageCrescent") {
			ApplyDamageCrescentEffect_(effect, targetIndex, applyAttackBuff);

		} else if (effect.type == "DamageByBlock") {
			ApplyDamageByBlockEffect_(effect, targetIndex, applyAttackBuff);

		} else if (effect.type == "Block") {
			ApplyBlockEffect_(effect);

		} else if (effect.type == "DamageAll") {
			ApplyDamageAllEffect_(effect, applyAttackBuff);

		} else if (effect.type == "PowerBoost") {
			ApplyPowerBoostEffect_(effect);
		} else if (effect.type == "NextTurnAtkUp") {
			ApplyNextTurnAtkUpEffect_(effect);

		} else if (effect.type == "Heal") {
			ApplyHealEffect_(effect);

		} else if (effect.type == "HealByBlock") {
			ApplyHealByBlockEffect_(effect);
		} else if (effect.type == "HealByLowCostInHand") {
			ApplyHealByLowCostInHandEffect_(effect);
		} else if (effect.type == "VampireBuff") {
			ApplyVampireBuffEffect_(effect);
		} else if (effect.type == "SelfDamage") {
			ApplySelfDamageEffect_(effect);

		} else if (effect.type == "Poison") {
			ApplyPoisonEffect_(effect, targetIndex);
		} else if (effect.type == "PoisonAll") {
			ApplyPoisonAllEffect_(effect);
		} else if (effect.type == "PoisonAmplify") {
			ApplyPoisonAmplifyEffect_(effect);
		} else if (effect.type == "PoisonDamage") {
			ApplyPoisonDamageEffect_(effect);
		} else if (effect.type == "PoisonDraw") {
			ApplyPoisonDrawEffect_(effect);

		} else if (effect.type == "PoisonRemove") {
			ApplyPoisonRemoveEffect_();

		} else if (effect.type == "PoisonHeal") {
			ApplyPoisonHealEffect_();

		} else if (effect.type == "Frost") {
			ApplyFrostEffect_(effect, targetIndex);

		} else if (effect.type == "FrostAll") {
			ApplyFrostAllEffect_(effect);

		} else if (effect.type == "FrostBlock") {
			ApplyFrostBlockEffect_(effect);

		} else if (effect.type == "FrostDamage") {
			ApplyFrostDamageEffect_(effect);

		} else if (effect.type == "FrostSubtract") {
			ApplyFrostSubtractEffect_(effect);

		} else if (effect.type == "FrostBite") {
			ApplyFrostBiteEffect_(effect);

		} else if (effect.type == "FrostAmplify") {
			ApplyFrostAmplifyEffect_(effect);

		} else if (effect.type == "FrostDraw") {
			ApplyFrostDrawEffect_(targetIndex);

		} else if (effect.type == "ChangeNumber") {
			ApplyChangeNumberEffect_(effect);

		} else if (effect.type == "ChangeSuit") {
			ApplyChangeSuitEffect_(effect);
		}
	}
}

void BattleController::ApplyCardEffects_(const CardDef& def, int targetIndex)
{
	ApplyEffectsList_(def.effects, targetIndex, true);
}

BattleController::PokerHandRank BattleController::ParsePokerRankString_(const std::string& s) const
{
	if (s == "OnePair") return PokerHandRank::OnePair;
	if (s == "TwoPair") return PokerHandRank::TwoPair;
	if (s == "ThreeOfAKind") return PokerHandRank::ThreeOfAKind;
	if (s == "Straight") return PokerHandRank::Straight;
	if (s == "Flush") return PokerHandRank::Flush;
	if (s == "FullHouse") return PokerHandRank::FullHouse;
	if (s == "FourOfAKind") return PokerHandRank::FourOfAKind;
	if (s == "StraightFlush") return PokerHandRank::StraightFlush;
	if (s == "RoyalStraightFlush") return PokerHandRank::RoyalStraightFlush;
	return PokerHandRank::None;
}

bool BattleController::IsRankAtLeast_(PokerHandRank a, PokerHandRank b) const
{
	return static_cast<int>(a) >= static_cast<int>(b);
}

bool BattleController::IsRankInFamily_(PokerHandRank rank, const std::string& family) const
{
	if (family == "StraightFamily") {
		return rank == PokerHandRank::Straight ||
			rank == PokerHandRank::StraightFlush ||
			rank == PokerHandRank::RoyalStraightFlush;
	}

	if (family == "FlushFamily") {
		return rank == PokerHandRank::Flush ||
			rank == PokerHandRank::StraightFlush ||
			rank == PokerHandRank::RoyalStraightFlush;
	}

	if (family == "PairFamily") {
		return rank == PokerHandRank::OnePair ||
			rank == PokerHandRank::TwoPair ||
			rank == PokerHandRank::ThreeOfAKind ||
			rank == PokerHandRank::FullHouse ||
			rank == PokerHandRank::FourOfAKind;
	}

	return false;
}

bool BattleController::DoesSubEffectConditionMatch_(const CardSubEffectDef& sub, PokerHandRank rank) const
{
	switch (sub.condition.type) {
	case SubEffectConditionType::ExactRank:
		return rank == ParsePokerRankString_(sub.condition.rank);

	case SubEffectConditionType::AtLeastRank:
		return IsRankAtLeast_(rank, ParsePokerRankString_(sub.condition.rank));

	case SubEffectConditionType::RankFamily:
		return IsRankInFamily_(rank, sub.condition.family);

	default:
		return false;
	}
}


void BattleController::StartPlayerTurn_()
{
	currentTurnAtkUp_ = nextTurnAtkUp_;
	nextTurnAtkUp_ = 0;

	if (player_) {
		if (sPlayerBlockCarryOverEnabled) {
			player_->DecayBlock(sPlayerBlockTurnDecayRate);
		} else {
			player_->ResetBlock();
		}
		player_->ResetVampireHeal();
		player_->ResetPowerBoost();
	}
	playerTurnCount_++;

	energy_ = energyMax_;
	DrawTurnStartCards_();

	if (!field_.empty()) {
		RebuildFieldView_();
	}

	if (field_.size() == 5) {
		currentPoker_ = EvaluatePokerHand_();

		if (currentPoker_.rank != PokerHandRank::None) {
			TriggerSubEffectsForField_(
				SubEffectTrigger::OnTurnStartWithPoker,
				currentPoker_.rank
			);

			lastPokerTutorialResult_ = PokerTutorialResult::None;
			pokerChoiceState_ = PokerChoiceState::WaitingActivateChoice;
			pokerChoiceJustOpened_ = true;
			BattleSfxPlayer::PlaySE("SE_Pop");
		}
	}
	if (enemyMgr_) {
		auto& enemies = enemyMgr_->GetEnemies();
		for (auto& enemy : enemies) {
			if (enemy.IsAlive()) {
				enemy.GetBossAI().DecideNextAction();
			}
		}
		enemyActionCountSystem_.StartPlayerTurn(enemies);
	} else {
		enemyActionCountSystem_.Clear();
	}
}

BattleController::PokerBonus BattleController::GetPokerBonus_(PokerHandRank rank) const
{
	PokerBonus b{};

	switch (rank) {
	case PokerHandRank::OnePair:
		b.atkUp = 3;
		b.drawCount = 2;
		b.damage = 55;
		break;

	case PokerHandRank::TwoPair:
		b.atkUp = 5;
		b.drawCount = 3;
		b.damage = 65;
		break;

	case PokerHandRank::ThreeOfAKind:
		b.atkUp = 10;
		b.drawCount = 3;
		b.damage = 80;
		break;

	case PokerHandRank::Straight:
		b.atkUp = 15;
		b.drawCount = 4;
		b.damage = 95;
		break;

	case PokerHandRank::Flush:
		b.atkUp = 20;
		b.drawCount = 4;
		b.damage = 105;
		break;

	case PokerHandRank::FullHouse:
		b.atkUp = 25;
		b.drawCount = 5;
		b.damage = 120;
		break;

	case PokerHandRank::FourOfAKind:
		b.atkUp = 30;
		b.drawCount = 5;
		b.damage = 135;
		break;

	case PokerHandRank::StraightFlush:
		b.atkUp = 40;
		b.drawCount = 6;
		b.damage = 160;
		break;

	case PokerHandRank::RoyalStraightFlush:
		b.atkUp = 60;
		b.drawCount = 7;
		b.damage = 200;
		break;

	default:
		break;
	}

	return b;
}

void BattleController::SetPokerQuickPreviewVisible(bool visible)
{
	pokerQuickPreviewVisible_ = visible;
}

std::wstring BattleController::GetSubEffectTriggerText_(SubEffectTrigger trigger) const
{
	return CardEffectTextBuilder::GetSubEffectTriggerText(trigger);
}

std::wstring BattleController::GetSubEffectConditionText_(const CardSubEffectDef& sub) const
{
	return CardEffectTextBuilder::GetSubEffectConditionText(sub);
}

std::wstring BattleController::GetEffectValueText_(const CardEffectDef& effect) const
{
	if (!effect.valueText.empty()) {
		return Utf8ToWString(effect.valueText) + L": " + std::to_wstring(effect.value);
	}

	if (effect.type == "Draw") {
		return L"ドロー: " + std::to_wstring(effect.value);
	}
	if (effect.type == "Damage") {
		return L"ダメージ: " + std::to_wstring(effect.value);
	}
	if (effect.type == "DamageAll") {
		return L"全体ダメージ: " + std::to_wstring(effect.value);
	}
	if (effect.type == "Heal") {
		return L"回復: " + std::to_wstring(effect.value);
	}
	if (effect.type == "Block") {
		return L"ブロック: " + std::to_wstring(effect.value);
	}
	if (effect.type == "PowerBoost") {
		return L"パワー: " + std::to_wstring(effect.value);
	}
	if (effect.type == "EnergyCharge") {
		return L"コスト回復: " + std::to_wstring(effect.value);
	}
	if (effect.type == "NextTurnAtkUp") {
		return L"次ターンATK UP: " + std::to_wstring(effect.value);
	}
	if (effect.type == "SelfDamage") {
		return L"自傷: " + std::to_wstring(effect.value);
	}

	return Utf8ToWString(effect.type) + L": " + std::to_wstring(effect.value);
}

std::wstring BattleController::GetBaseEffectSummaryText_(const CardDef& def) const
{
	return CardEffectTextBuilder::GetBaseEffectSummaryText(def);
}

std::wstring BattleController::GetPreviewCardDetailText() const
{
	return BattleInfoTextProvider::BuildPreviewCardDetailText(GetPreviewCardDef());
}

std::vector<std::wstring> BattleController::CollectSubEffectPreviewLines_(
	SubEffectTrigger trigger,
	PokerHandRank rank
) const
{
	std::vector<std::wstring> lines;
	std::set<std::wstring> uniqueLines;

	for (const auto& card : field_) {
		const CardDef* def = db_.Find(card.defId);
		if (!def) continue;

		for (const auto& sub : def->subEffects) {
			if (sub.trigger != trigger) continue;
			if (!DoesSubEffectConditionMatch_(sub, rank)) continue;

			for (const auto& effect : sub.effects) {
				std::wstring line = L"・";

				if (!def->name.empty()) {
					int size = MultiByteToWideChar(CP_UTF8, 0, def->name.c_str(), -1, nullptr, 0);
					std::wstring cardName(size - 1, L'\0');
					MultiByteToWideChar(CP_UTF8, 0, def->name.c_str(), -1, cardName.data(), size);
					line += cardName + L" : ";
				}

				if (effect.type == "Draw") {
					line += L"カードを" + FormatEffectValue_(effect) + L"枚引く";
				} else if (effect.type == "Damage") {
					line += L"敵単体に" + FormatEffectValue_(effect) + L"ダメージ";
				} else if (effect.type == "DamageAll") {
					line += L"敵全体に" + FormatEffectValue_(effect) + L"ダメージ";
				} else if (effect.type == "Heal") {
					line += L"体力を" + FormatEffectValue_(effect) + L"回復";
				} else if (effect.type == "Block") {
					line += L"ブロックを" + FormatEffectValue_(effect) + L"獲得";
				} else if (effect.type == "PowerBoost") {
					line += L"パワーを" + FormatEffectValue_(effect) + L"獲得";
				} else if (effect.type == "EnergyCharge") {
					line += L"コストを" + FormatEffectValue_(effect) + L"回復";
				} else {
					line += Utf8ToWString(effect.type) + L" : " + FormatEffectValue_(effect);
				}

				if (uniqueLines.insert(line).second) {
					lines.push_back(line);
				}
			}
		}
	}

	return lines;
}

std::wstring BattleController::GetPokerEffectPreviewText() const
{
	const PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);

	auto turnStartLines = CollectSubEffectPreviewLines_(
		SubEffectTrigger::OnTurnStartWithPoker,
		currentPoker_.rank
	);

	auto pokerActivatedLines = CollectSubEffectPreviewLines_(
		SubEffectTrigger::OnPokerSkillActivated,
		currentPoker_.rank
	);

	return BattleInfoTextProvider::BuildPokerEffectPreviewText(
		{ bonus.atkUp, bonus.drawCount, bonus.damage },
		turnStartLines,
		pokerActivatedLines);
}
void BattleController::TriggerSubEffectsForField_(SubEffectTrigger trigger, PokerHandRank rank)
{
	if (field_.size() != 5) return;
	if (rank == PokerHandRank::None) return;

	for (const auto& card : field_) {
		const CardDef* def = db_.Find(card.defId);
		if (!def) continue;

		for (const auto& sub : def->subEffects) {
			if (sub.trigger != trigger) continue;
			if (!DoesSubEffectConditionMatch_(sub, rank)) continue;

			ApplyEffectsList_(sub.effects, -1, false);
		}
	}
}

void BattleController::TriggerSubEffectsForCard_(
	const CardInstance& card,
	SubEffectTrigger trigger,
	PokerHandRank rank)
{
	const CardDef* def = db_.Find(card.defId);
	if (!def) return;
	if (rank == PokerHandRank::None) return;

	for (const auto& sub : def->subEffects) {
		if (sub.trigger != trigger) continue;
		if (!DoesSubEffectConditionMatch_(sub, rank)) continue;

		ApplyEffectsList_(sub.effects, -1, false);
	}
}

void BattleController::RebuildFieldView_()
{
	const int n = (int)field_.size();
	if (n <= 0) {
		fieldViews_.clear();
		return;
	}

	while (fieldViews_.size() < field_.size()) {
		int i = (int)fieldViews_.size();
		const CardDef* def = db_.Find(field_[i].defId);

		auto card = std::make_unique<Card3D>();
		if (def) card->Initialize(objCom_, dx_, cam_, *def, field_[i]);
		card->SetIsHand(false);
		card->SetTransform({ 0.0f, -10.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.15f, 1.15f, 1.15f }); // 鬯ｮ・ｫ驕ｨ繧托ｽｽ・ｹ隴擾ｽｶ隴・ｽ｡鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｪ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ繝ｻ・ｰ郢晢ｽｻ繝ｻ・ｴ鬮ｫ・ｰ郢晢ｽｻ・つ鬩搾ｽｵ繝ｻ・ｺ髣包ｽｵ隴趣ｽ｢繝ｻ・ｽ郢晢ｽｻ
		fieldViews_.push_back(std::move(card));
	}

	const float y = fieldCardLayout_.y;
	const float z = fieldCardLayout_.z;
	const float gap = fieldCardLayout_.gap;
	const float startX = -gap * 0.5f * (n - 1);

	for (int i = 0; i < n; ++i) {
		if (!fieldViews_[i]) continue;

		Vector3 pos{ startX + gap * i, y, z };
		Vector3 rot{ 0.0f, 0.0f, 0.0f };
		Vector3 scl{ fieldCardLayout_.scale, fieldCardLayout_.scale, fieldCardLayout_.scale };

		fieldViews_[i]->SetTargetTransform(pos, rot, scl, false);

	}

	// 1. 鬮｣遒代・繝ｻ・ｿ繝ｻ・ｫ驛｢譎｢・ｽ・ｻ鬮ｯ貅ｷ遘√・・ｽ繝ｻ・ｹ鬩幢ｽ｢繝ｻ・ｧ髯橸ｽｳ陞滂ｽｲ繝ｻ・ｽ繝ｻ・ｩ鬩包ｽｨ郢ｧ謇假ｽｽ・ｽ繝ｻ・ｾ郢晢ｽｻ繝ｻ・｡
	if (field_.size() == 5) {
		currentPoker_ = EvaluatePokerHand_();
	} else {
		currentPoker_.rank = PokerHandRank::None;
		currentPoker_.power = 0;
	}

	// 2. 鬮ｯ貅ｷ遘√・・ｽ繝ｻ・ｹ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮ｯ貊捺ｱ壹・・ｽ繝ｻ・ｷ鬩搾ｽｵ繝ｻ・ｺ鬮ｴ蝓溷繭郢晢ｽｻ鬮ｯ貊ゑｽｽ・｢髫ｲ蟶ｷ閻ｸ繝ｻ・ｧ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｭ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｩ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｭ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｩ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮ｯ貊捺ｱ壹・・ｽ繝ｻ・ｷ鬩搾ｽｵ繝ｻ・ｺ鬮ｴ驛・ｽｲ・ｻ繝ｻ・ｽ陞ｳ螟ｲ・ｽ・ｱ髮懶ｽ｣繝ｻ・ｽ繝ｻ・ｺ鬩幢ｽ｢繝ｻ・ｧ驕ｶ荳橸ｽ､・ｲ繝ｻ・ｽ郢晢ｽｻ
	float intensity = 0.0f;
	if (currentPoker_.rank == PokerHandRank::None) {
		intensity = 0.0f;
	} else if (currentPoker_.rank <= PokerHandRank::TwoPair) {
		intensity = 5.0f;  // 鬮ｯ貊捺ｱ壹・・ｽ繝ｻ・ｱ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ繝ｻ・ｽ郢晢ｽｻ繝ｻ・ｹ驛｢譎｢・ｽ・ｻ髯橸ｽ｢繝ｻ・ｹ驕ｶ蛹・ｽｽ・ｧ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・｣鬩搾ｽｵ繝ｻ・ｺ髯ｷ・ｷ繝ｻ・ｶ郢晢ｽｻ郢晢ｽｻ
	} else if (currentPoker_.rank <= PokerHandRank::FullHouse) {
		intensity = 10.0f;  // 鬮｣蛹・ｽｽ・ｳ郢晢ｽｻ繝ｻ・ｭ鬮ｯ諛ｶ・ｽ・｣驛｢譎｢・ｽ・ｻ驛｢譎｢・ｽ・ｻ鬮ｯ貅ｷ遘√・・ｽ繝ｻ・ｹ驛｢譎｢・ｽ・ｻ髯橸ｽ｢繝ｻ・ｹ驛｢譎｢・ｽ・ｻ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・｣鬩搾ｽｵ繝ｻ・ｺ鬯ｮ・ｦ繝ｻ・ｪ郢晢ｽｻ郢晢ｽｻ
	} else {
		intensity = 15.0f;  // 鬮ｯ貊捺ｱ壹・・ｽ繝ｻ・ｷ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ繝ｻ・ｽ郢晢ｽｻ繝ｻ・ｹ驛｢譎｢・ｽ・ｻ髯橸ｽ｢繝ｻ・ｹ驕ｶ謫ｾ・ｽ・ｪ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ郢晢ｽｻ隶抵ｽｭ郢晢ｽｻ驛｢譎｢・ｽ・ｻ
	}

	// 3. 鬮ｯ貅ｷ遘√・・ｽ繝ｻ・ｹ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬯ｯ・ｮ繝ｻ・｢郢晢ｽｻ繝ｻ・｢鬮｣蜴・ｽｽ・ｫ驛｢・ｧ郢晢ｽｻ繝ｻ・ｼ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ霑｢證ｦ・ｽ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ髫ｨ繝ｻ・ｽ・｡鬩搾ｽｵ繝ｻ・ｺ髣比ｼ夲ｽｽ・｣郢晢ｽｻ陜｣・､繝ｻ・ｹ隴乗・・ｽ・ｸ驗呻ｽｫ遶包ｽｧ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｩ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ
	std::array<bool, 5> mask = GetPokerHighlightMask_();

	for (int i = 0; i < (int)fieldViews_.size(); ++i) {
		if (i < 5 && mask[i]) {
			fieldViews_[i]->SetGlitter(intensity);

			// 鬮ｯ貊捺ｱ壹・・ｽ繝ｻ・ｷ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ繝ｻ・ｽ郢晢ｽｻ繝ｻ・ｹ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｪ鬩幢ｽ｢繝ｻ・ｧ鬨ｾ蛹・ｽｽ・ｻ髫ｴ・ｽ繝ｻ・ｧ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ髮趣ｽｼ繝ｻ・ｶ郢晢ｽｻ繝ｻ・ｲ鬩幢ｽ｢繝ｻ・ｧ驛｢・ｧ郢晢ｽｻ繝ｻ・ｽ繝ｻ・､鬨ｾ蛹・ｽｽ・ｻ髯晢ｽｲ繝ｻ・ｩ
			if (currentPoker_.rank != PokerHandRank::None) {
				// final frame color is applied in RefreshAllFieldCardTransforms_()
			}
		} else {
			// 鬮ｯ貅ｷ遘√・・ｽ繝ｻ・ｹ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬯ｯ・ｮ繝ｻ・｢郢晢ｽｻ繝ｻ・｢鬮｣蜴・ｽｽ・ｫ驛｢・ｧ郢晢ｽｻ郢晢ｽｻ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ驍ｵ・ｺ陷･・ｲ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ驛｢譎｢・ｽ・ｻ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｪ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｻ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驛｢譎｢・ｽ・ｨ
			fieldViews_[i]->SetGlitter(0.0f);
			// final frame color is applied in RefreshAllFieldCardTransforms_()
		}
	}

	fieldLayoutDirty_ = true;
	RefreshAllFieldCardTransforms_(0.0f);
}

void BattleController::UpdateFieldCardTransform_(int index, bool hovered, float dt)
{
	if (BattleFieldViewController::UpdateFieldCardTransform(
		fieldViews_,
		MakeFieldLayoutParams_(fieldCardLayout_),
		index,
		hovered,
		dt)) {
		fieldLayoutDirty_ = true;
	}
}

void BattleController::RefreshAllFieldCardTransforms_(float dt)
{
	BattleFieldViewController::TransformContext transformContext{};
	transformContext.fieldViews = &fieldViews_;
	transformContext.layout = MakeFieldLayoutParams_(fieldCardLayout_);
	transformContext.hoverIndex = fieldReplaceHoverIndex_;
	transformContext.choosingFieldReplace = cardState_ == CardInputState::ChoosingFieldReplace;
	transformContext.viewingBoardFromPokerUi = pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi;
	if (BattleFieldViewController::RefreshFieldCardTransforms(transformContext, dt)) {
		fieldLayoutDirty_ = true;
	}

	if (field_.size() == 5) {
		currentPoker_ = EvaluatePokerHand_();
	} else {
		currentPoker_.rank = PokerHandRank::None;
		currentPoker_.power = 0;
	}

	UpdateFieldReplacePreviewEffects_();

	BattleFieldViewController::FrameEffectContext frameContext{};
	frameContext.fieldViews = &fieldViews_;
	frameContext.currentRank = currentPoker_.rank;
	frameContext.pokerGlowTime = sPokerGlowRainbowTime;
	frameContext.highlightMask = GetPokerHighlightMask_();
	frameContext.inReplacePreview = cardState_ == CardInputState::ChoosingFieldReplace && hasPendingCard_;
	frameContext.replacePreviewRanks = &fieldReplacePreviewRanks_;
	frameContext.replacePreviewActive = &fieldReplacePreviewActive_;
	BattleFieldViewController::ApplyFieldFrameEffects(frameContext);
}

void BattleController::UpdateFieldReplacePreviewEffects_()
{
	BattleFieldViewController::ReplacePreviewContext context{};
	context.field = &field_;
	context.fieldViewCount = fieldViews_.size();
	context.pendingCard = &pendingCard_;
	context.choosingFieldReplace = cardState_ == CardInputState::ChoosingFieldReplace;
	context.hasPendingCard = hasPendingCard_;
	context.hoverIndex = fieldReplaceHoverIndex_;

	const BattleFieldViewController::ReplacePreviewResult result =
		BattleFieldViewController::BuildFieldReplacePreview(context);
	fieldReplacePreviewRanks_ = result.ranks;
	fieldReplacePreviewActive_ = result.active;
}

void BattleController::EmitFieldCardGlitter_(float dt)
{
	BattleFieldViewController::GlitterContext context{};
	context.fieldViews = &fieldViews_;
	context.particleManager = fieldParticleManager_;
	context.emitTimer = &fieldCardGlitterEmitTimer_;
	context.dt = dt;
	context.emitInterval = sFieldCardGlitterEmitInterval;
	context.normalCount = sFieldCardGlitterNormalCount;
	context.highlightCount = sFieldCardGlitterHighlightCount;
	context.localOffset = &sFieldCardGlitterLocalOffset;
	context.spreadX = sFieldCardGlitterSpreadX;
	context.spreadY = sFieldCardGlitterSpreadY;
	context.currentRank = currentPoker_.rank;
	context.pokerGlowTime = sPokerGlowRainbowTime;
	context.highlightMask = GetPokerHighlightMask_();
	context.choosingFieldReplace = cardState_ == CardInputState::ChoosingFieldReplace;
	context.replacePreviewRanks = &fieldReplacePreviewRanks_;
	context.replacePreviewActive = &fieldReplacePreviewActive_;
	BattleFieldViewController::EmitFieldCardGlitter(context);
}
void BattleController::UpdateFieldEditorPreview(float dt)
{
	sPokerGlowRainbowTime += dt;
	RefreshAllFieldCardTransforms_(dt);

	for (auto& card : fieldViews_) {
		if (card) {
			card->Update(dt);
		}
	}

	if (propManager_) {
		propManager_->Update(dt);
	}
}

void BattleController::BuildFieldEditorPreview(int cardCount, int firstCardId)
{
	cardCount = std::clamp(cardCount, 0, 5);
	firstCardId = std::max(1, firstCardId);

	field_.clear();
	fieldViews_.clear();
	currentPoker_ = {};
	fieldReplacePreviewRanks_.clear();
	fieldReplacePreviewActive_.clear();
	cardState_ = CardInputState::Idle;
	pokerChoiceState_ = PokerChoiceState::None;

	for (int i = 0; i < cardCount; ++i) {
		int defId = firstCardId + i;
		if (!db_.Find(defId)) {
			defId = 1;
		}

		CardInstance card{};
		card.defId = defId;
		card.number = (i % 13) + 1;
		card.suit = static_cast<CardSuit>(i % 4);
		field_.push_back(card);
	}

	RebuildFieldView_();
}

void BattleController::DrawFieldEditorPreview3D(GameApp& app)
{
	(void)app;
	for (auto& card : fieldViews_) {
		if (card) {
			card->Draw();
		}
	}

	if (propManager_) {
		propManager_->Draw3D();
	}
}

void BattleController::DrawFieldEditorPostEffect3D(GameApp& app)
{
	if (propManager_) {
		propManager_->DrawPostEffect3D(app);
	}
}

bool BattleController::LoadFieldCardLayout(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open()) {
		return false;
	}

	nlohmann::json json;
	file >> json;

	if (json.contains("y")) fieldCardLayout_.y = json["y"].get<float>();
	if (json.contains("z")) fieldCardLayout_.z = json["z"].get<float>();
	if (json.contains("gap")) fieldCardLayout_.gap = json["gap"].get<float>();
	if (json.contains("scale")) fieldCardLayout_.scale = json["scale"].get<float>();
	if (json.contains("hoverYOffset")) fieldCardLayout_.hoverYOffset = json["hoverYOffset"].get<float>();
	if (json.contains("hoverZOffset")) fieldCardLayout_.hoverZOffset = json["hoverZOffset"].get<float>();
	if (json.contains("hoverScale")) fieldCardLayout_.hoverScale = json["hoverScale"].get<float>();

	fieldCardLayout_.gap = std::max(0.0f, fieldCardLayout_.gap);
	fieldCardLayout_.scale = std::max(0.01f, fieldCardLayout_.scale);
	fieldCardLayout_.hoverScale = std::max(0.01f, fieldCardLayout_.hoverScale);
	fieldLayoutDirty_ = true;
	RefreshAllFieldCardTransforms_(0.0f);
	return true;
}

bool BattleController::SaveFieldCardLayout(const std::string& path) const
{
	const std::filesystem::path filePath(path);
	const std::filesystem::path dir = filePath.parent_path();
	if (!dir.empty() && !std::filesystem::exists(dir)) {
		std::filesystem::create_directories(dir);
	}

	nlohmann::json json;
	json["y"] = fieldCardLayout_.y;
	json["z"] = fieldCardLayout_.z;
	json["gap"] = fieldCardLayout_.gap;
	json["scale"] = fieldCardLayout_.scale;
	json["hoverYOffset"] = fieldCardLayout_.hoverYOffset;
	json["hoverZOffset"] = fieldCardLayout_.hoverZOffset;
	json["hoverScale"] = fieldCardLayout_.hoverScale;

	std::ofstream file(path);
	if (!file.is_open()) {
		return false;
	}

	file << json.dump(4);
	return true;
}

#ifdef USE_IMGUI
void BattleController::DrawFieldSceneEditerImGui()
{
	ImGui::Begin("FieldSceneEditer");

	bool layoutChanged = false;
	layoutChanged |= ImGui::DragFloat("Field Y", &fieldCardLayout_.y, 0.05f);
	layoutChanged |= ImGui::DragFloat("Field Z", &fieldCardLayout_.z, 0.05f);
	layoutChanged |= ImGui::DragFloat("Card Gap", &fieldCardLayout_.gap, 0.05f, 0.0f, 20.0f);
	layoutChanged |= ImGui::DragFloat("Card Scale", &fieldCardLayout_.scale, 0.01f, 0.01f, 5.0f);
	layoutChanged |= ImGui::DragFloat("Hover Y Offset", &fieldCardLayout_.hoverYOffset, 0.01f);
	layoutChanged |= ImGui::DragFloat("Hover Z Offset", &fieldCardLayout_.hoverZOffset, 0.01f);
	layoutChanged |= ImGui::DragFloat("Hover Scale", &fieldCardLayout_.hoverScale, 0.01f, 0.01f, 5.0f);

	fieldCardLayout_.gap = std::max(0.0f, fieldCardLayout_.gap);
	fieldCardLayout_.scale = std::max(0.01f, fieldCardLayout_.scale);
	fieldCardLayout_.hoverScale = std::max(0.01f, fieldCardLayout_.hoverScale);

	if (layoutChanged) {
		fieldLayoutDirty_ = true;
		RefreshAllFieldCardTransforms_(0.0f);
	}

	if (ImGui::Button("Save Field Layout")) {
		SaveFieldCardLayout(fieldCardLayoutPath_);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Field Layout")) {
		LoadFieldCardLayout(fieldCardLayoutPath_);
	}

	ImGui::Separator();
	ImGui::Text("Preview Cards: %d", static_cast<int>(field_.size()));
	ImGui::Text("Layout File: %s", fieldCardLayoutPath_.c_str());

	if (propManager_) {
		ImGui::Separator();
		propManager_->DrawImGui();
	}

	ImGui::End();
}
#endif

uint64_t BattleController::BuildHandPokerPreviewSignature_() const
{
	uint64_t hash = 1469598103934665603ull;
	auto mix = [&hash](uint64_t value) {
		hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
		hash *= 1099511628211ull;
	};

	mix(sHandPokerPreviewEnabled ? 1ull : 0ull);
	mix(static_cast<uint64_t>(currentPoker_.rank));
	mix(static_cast<uint64_t>(field_.size()));
	for (const auto& card : field_) {
		mix(static_cast<uint64_t>(card.defId));
		mix(static_cast<uint64_t>(card.number));
		mix(static_cast<uint64_t>(card.suit));
	}

	const auto& hand = deckZone_.GetHand();
	mix(static_cast<uint64_t>(hand.size()));
	for (const auto& card : hand) {
		mix(static_cast<uint64_t>(card.defId));
		mix(static_cast<uint64_t>(card.number));
		mix(static_cast<uint64_t>(card.suit));
	}

	mix(static_cast<uint64_t>(handView_.GetCardCount()));
	mix(static_cast<uint64_t>(tutorialForcedEnemyTargetCardId_ + 1));
	return hash;
}

bool BattleController::IsTutorialForcedCardActive_() const
{
	return tutorialForcedEnemyTargetCardId_ > 0;
}

bool BattleController::IsTutorialForcedCardAllowed_(int handIndex) const
{
	if (!IsTutorialForcedCardActive_()) {
		return true;
	}

	const auto& hand = deckZone_.GetHand();
	if (handIndex < 0 || handIndex >= static_cast<int>(hand.size())) {
		return false;
	}

	return hand[handIndex].defId == tutorialForcedEnemyTargetCardId_;
}

void BattleController::UpdateHandPokerPreviewEffects_()
{
	const uint64_t signature = BuildHandPokerPreviewSignature_();
	if (signature == handPreviewSignature_) {
		return;
	}
	handPreviewSignature_ = signature;

	const auto& hand = deckZone_.GetHand();
	handPreviewRanks_.assign(hand.size(), PokerHandRank::None);

	if (IsTutorialForcedCardActive_()) {
		const int handCount = std::min<int>(static_cast<int>(hand.size()), handView_.GetCardCount());
		for (int handIndex = 0; handIndex < handCount; ++handIndex) {
			if (IsTutorialForcedCardAllowed_(handIndex)) {
				handView_.SetCardEffect(handIndex, { 1.0f, 0.86f, 0.18f, 1.0f }, 1.8f);
			} else {
				handView_.SetCardEffect(handIndex, { 0.18f, 0.18f, 0.20f, 0.55f }, 0.0f);
			}
		}
		return;
	}

	if (!sHandPokerPreviewEnabled || hand.empty()) {
		handView_.ClearCardEffects();
		return;
	}

	bool anyPreview = false;
	const int handCount = std::min<int>(static_cast<int>(hand.size()), handView_.GetCardCount());

	for (int handIndex = 0; handIndex < handCount; ++handIndex) {
		PokerHandRank bestRank = PokerHandRank::None;

		if (field_.size() < 5) {
			std::vector<CardInstance> candidate = field_;
			candidate.push_back(hand[handIndex]);
			if (candidate.size() == 5) {
				bestRank = EvaluatePokerHandForCards_(candidate).rank;
			}
		} else if (field_.size() == 5) {
			for (int replaceIndex = 0; replaceIndex < 5; ++replaceIndex) {
				std::vector<CardInstance> candidate = field_;
				candidate[replaceIndex] = hand[handIndex];
				const PokerHandRank rank = EvaluatePokerHandForCards_(candidate).rank;
				if (rank > bestRank) {
					bestRank = rank;
				}
			}
		}

		if (bestRank <= currentPoker_.rank) {
			bestRank = PokerHandRank::None;
		}

		handPreviewRanks_[handIndex] = bestRank;

		if (bestRank != PokerHandRank::None) {
			const Vector4 color = BattleFieldViewController::GetPokerFrameColor(bestRank, sPokerGlowRainbowTime);
			handView_.SetCardEffect(handIndex, color, BattleFieldViewController::GetPokerGlitterIntensity(bestRank));
			anyPreview = true;
		} else {
			handView_.ResetCardEffect(handIndex);
		}
	}

	if (!anyPreview) {
		handView_.ClearCardEffects();
	}
}

void BattleController::EmitHandCardGlitter_(float dt)
{
	if (!sHandPokerPreviewEnabled || handPreviewRanks_.empty()) {
		handCardGlitterEmitTimer_ = 0.0f;
		return;
	}

	handCardGlitterEmitTimer_ += dt;
	if (handCardGlitterEmitTimer_ < sHandCardGlitterEmitInterval) {
		return;
	}
	handCardGlitterEmitTimer_ = 0.0f;

	ModelParticleManager* particles = fieldParticleManager_;
	if (!particles) {
		return;
	}

	const int handCount = std::min<int>(static_cast<int>(handPreviewRanks_.size()), handView_.GetCardCount());
	for (int i = 0; i < handCount; ++i) {
		const PokerHandRank rank = handPreviewRanks_[i];
		if (rank == PokerHandRank::None) {
			continue;
		}

		Card3D* card = handView_.GetCard(i);
		if (!card) {
			continue;
		}

		const Vector4 color = BattleFieldViewController::GetPokerFrameColor(rank, sPokerGlowRainbowTime);
		const uint32_t emitCount = static_cast<uint32_t>(std::max(0, sHandCardGlitterCount));
		for (uint32_t emitIndex = 0; emitIndex < emitCount; ++emitIndex) {
			const Vector3 localOffset = {
				sFieldCardGlitterLocalOffset.x + Rand(-sFieldCardGlitterSpreadX, sFieldCardGlitterSpreadX),
				sFieldCardGlitterLocalOffset.y + Rand(-sFieldCardGlitterSpreadY, sFieldCardGlitterSpreadY),
				sFieldCardGlitterLocalOffset.z
			};
			particles->Emit("card_glitter", card->GetWorldPointFromLocal(localOffset), 1u, color);
		}
	}
}

int BattleController::PickFieldIndexByMouse_(int mouseX, int mouseY) const
{
	return BattleFieldViewController::PickFieldIndexByMouse(
		fieldViews_,
		*cam_,
		mouseX,
		mouseY,
		(float)WinApp::kClientWidth,
		(float)WinApp::kClientHeight);
}

void BattleController::Update(GameApp& app, FieldUi& fieldUi, float dt)
{
	if (actionDirector_.IsPlaying()) {
		Camera* actionCamera = GetActionCamera();
		if (actionCamera) {
			app.ObjCom()->SetDefaultCamera(actionCamera);
			if (player_) {
				player_->SetCamera(actionCamera);
			}
			if (enemyMgr_) {
				enemyMgr_->UpdateCamera(actionCamera);
			}
		}

		const bool sequenceFinished = actionDirector_.Update(dt, app.GetInput());
		const bool isFinalActionSequence =
			cardState_ == CardInputState::ExecutingSequence &&
			actionSequenceIndex_ >= actionSequenceQueue_.size();
		if (isFinalActionSequence &&
			!actionSequenceDamageApplied_ &&
			actionDirector_.HasReachedImpact()) {
			Enemy& targetEnemy = actionSequenceTarget_
				? *actionSequenceTarget_
				: enemyMgr_->GetEnemies()[currentEnemyIndex_];
			ExecutePendingAttack_(targetEnemy);
			actionSequenceDamageApplied_ = true;
		}

		if (sequenceFinished) {
			app.ObjCom()->SetDefaultCamera(cam_);
			if (player_) {
				player_->SetCamera(cam_);
			}
			if (enemyMgr_) {
				enemyMgr_->UpdateCamera(cam_);
			}
			if (cardState_ == CardInputState::ExecutingSequence) {
				if (StartNextActionSequence_()) {
					return;
				}
				Enemy& targetEnemy = actionSequenceTarget_
					? *actionSequenceTarget_
					: enemyMgr_->GetEnemies()[currentEnemyIndex_];
				if (!actionSequenceDamageApplied_) {
					ExecutePendingAttack_(targetEnemy);
				}
			} else if (actionSequenceDamageApplied_) {
				actionSequenceQueue_.clear();
				actionSequenceIndex_ = 0;
				actionSequenceCardDef_ = nullptr;
				actionSequenceDamageApplied_ = false;
			}
		}
		// Skip logic but update visuals
		UpdateVisuals_(dt);
		return;
	}

	UpdateLogic_(app, fieldUi, dt);

	UpdateVisuals_(dt);
	EmitFieldCardGlitter_(dt);
	EmitHandCardGlitter_(dt);
}

void BattleController::UpdateClearTransitionVisuals(float dt)
{
	UpdateVisuals_(dt);
}

void BattleController::PrepareForClearTransition()
{
	cardState_ = CardInputState::Idle;
	pokerChoiceState_ = PokerChoiceState::None;
	pokerReturnState_ = PokerChoiceState::None;
	pokerQuickPreviewVisible_ = false;
	hasPendingCard_ = false;
	pendingCard_ = {};
	pendingCardView_.reset();
	selectedIndex_ = -1;
	fieldReplaceHoverIndex_ = -1;
	prevFieldReplaceHoverIndex_ = -1;
	isPokerDamageTargeting_ = false;
	tutorialLockPokerTargetingCancel_ = false;
	pendingDamage_ = 0;
	handView_.SetHoverIndex(-1);
	handView_.SetPreviewIndex(-1);
	handView_.SetDrag(-1, 0.0f, 0.0f, false);
	handView_.SetFocusIndex(-1);
	if (enemyMgr_) {
		for (auto& enemy : enemyMgr_->GetEnemies()) {
			enemy.SetHighlight(false);
		}
	}
}

Camera* BattleController::GetActionCamera() const
{
	if (!actionDirector_.IsPlaying()) {
		return nullptr;
	}
	if (!actionDirector_.GetProfile().enableCameraWork) {
		return nullptr;
	}
	return actionDirector_.GetCinematicCamera();
}

Enemy* BattleController::GetActionTarget() const
{
	if (actionDirector_.IsPlaying()) {
		return actionDirector_.GetTarget();
	}
	return actionSequenceTarget_;
}

void BattleController::UpdateLogic_(GameApp& app, FieldUi& fieldUi, float dt)
{
	Input* input = app.GetInput();
	if (!input) {
		return;
	}

	bool yTrig = input->IsKeyTrigger(DIK_Y);
	bool nTrig = input->IsKeyTrigger(DIK_N);

	POINT mouse = input->GetMousePosition();

	bool lNow = input->IsMousePressed(0);
	bool lTrig = input->IsMouseTrigger(0);
	bool lRel = input->IsMouseReleased(0);

	bool rTrig = input->IsMouseTrigger(1);
	// ---------------------------------
	// 鬩幢ｽ｢隴擾ｽｶ郢晢ｽｻ繝ｻ螳茨ｽ､・ｼ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴主・讓溘・蜿悶渚繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・｢鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｫ鬮｣蛹・ｽｽ・ｳ郢晢ｽｻ繝ｻ・ｭ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮUI鬯ｮ・ｫ繝ｻ・ｱ郢晢ｽｻ繝ｻ・ｬ鬮ｫ・ｴ闕ｳ讖ｸ・ｽ・ｼ繝ｻ・ｱ驍ｵ・ｲ陜｣・､繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｯ
	// 鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｲ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・｣繝ｻ・ｰ鬩幢ｽ｢隴惹ｸ橸ｽｹ・ｲ繝ｻ蜿厄ｽｨ謚ｵ・ｽ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬮ｯ・ｷ髣鯉ｽｨ繝ｻ・ｽ繝ｻ・･鬮ｯ・ｷ霑壼遜・ｽ・ｸ陷ｷ・ｮ陷ｻ・ｳ鬩搾ｽｵ繝ｻ・ｺ鬯ｩ・｢陜ｮ繧・ｽｼ・ｯ鬮ｯ・ｷ闔ｨ螟ｲ・ｽ・ｽ繝ｻ・ｹ鬮ｯ蜈ｷ・ｽ・ｹ髫ｰ雋ｻ・ｽ・ｶ髫ｨ蛟･繝ｻ繝ｻ・ｹ繝ｻ・ｧ驛｢譎｢・ｽ・ｻ
	// ---------------------------------
	if (tutorialInputLocked_) {
		yTrig = false;
		nTrig = false;
		lTrig = false;
		lRel = false;
		rTrig = false;
	}

	const BattleCardInputController::CardInputSnapshot cardInput{ mouse.x, mouse.y, lTrig, lRel, rTrig, lNow };

	pokerMouseChoice_ = PokerMouseChoice::None;

	// -----------------------------
	// 鬩幢ｽ｢隴弱・・ｺ・｢驛｢譎｢・ｽ・ｻ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬯ｨ・ｾ陷茨ｽｷ繝ｻ・ｽ繝ｻ・ｺ鬮ｯ・ｷ陝雜｣・ｽ・ｼ隰夲ｽｫ郢晢ｽｻ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ鬩搾ｽｵ繝ｻ・ｺ髯ｷ莨夲ｽｽ・ｱ驕ｶ莨√・繝ｻ・ｸ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ鬯ｯ・ｩ陋ｹ繝ｻ・ｽ・ｽ繝ｻ・ｸ鬮ｫ・ｰ陞｢・ｹ郢晢ｽｻ
	// -----------------------------
	if (pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice)
	{
		HandlePokerActivateChoice_(fieldUi, mouse, lTrig, yTrig, nTrig);
		return;
	}

	// -----------------------------
	// 鬩幢ｽ｢隴弱・・ｺ・｢驛｢譎｢・ｽ・ｻ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬮ｯ・ｷ闔ｨ螟ｲ・ｽ・ｽ繝ｻ・ｹ鬮ｫ・ｴ繝ｻ・ｫ髫ｲ讖ｸ・ｽ・ｺ驕ｶ蝓弱Γ繝ｻ・ｬ陞｢・ｹ郢晢ｽｻ
	// -----------------------------
	if (pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice)
	{
		HandlePokerEffectChoice_(fieldUi, mouse, lTrig, nTrig);
		return;
	}

	if (pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi)
	{
		HandlePokerViewBoard_(fieldUi, mouse, lTrig, dt);
		return;
	}

	bool enterTrig = input->IsKeyTrigger(DIK_RETURN);
	if (tutorialInputLocked_) {
		enterTrig = false;
	}

	operationUiVisible_ = !tutorialInputLocked_ && input->IsKeyPressed(DIK_TAB);

	bool endTurnButtonClicked = false;
	endTurnButtonHovered_ = false;

	if (turn_ == TurnState::Player &&
		cardState_ == CardInputState::Idle &&
		pokerChoiceState_ == PokerChoiceState::None &&
		!tutorialEndTurnLocked_) {

		endTurnButtonHovered_ = false;

		if (propManager_) {
			Matrix4x4 vp = cam_->GetViewProjectionMatrix();
			for (const auto& prop : propManager_->GetProps()) {
				// Scene Editor鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬮｣逧ｮ逕･・つ繝ｻ・･郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ髮九・・ｽ・ｷ鬯ｪ・ｭ陷奇｣ｰ隲ｱ繝ｻ・ｫ・ｦ繝ｻ・ｪ驕ｯ・ｶ繝ｻ・ｲ "Button" 鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩幢ｽ｢繝ｻ・ｧ驛｢・ｧ郢晢ｽｻ郢晢ｽｻ鬩幢ｽ｢繝ｻ・ｧ髯ｷ・ｻ髢ｧ・ｲ驍頑ｻ・＠繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
				if (prop.name == "Button" || prop.name == "EndTurnButton") {
					Vector4 clip = MulRowVec4Mat4({ prop.pos.x, prop.pos.y, prop.pos.z, 1.0f }, vp);
					if (clip.w > 0.0f) {
						float sx = (clip.x / clip.w + 1.0f) * 0.5f * WinApp::kClientWidth;
						float sy = (1.0f - clip.y / clip.w) * 0.5f * WinApp::kClientHeight;

						constexpr float kEndTurnButtonHitRadiusBase = 35.0f;
						float radius = kEndTurnButtonHitRadiusBase * prop.scale.x;
						float dx = mouse.x - sx;
						float dy = mouse.y - sy;
						if (dx * dx + dy * dy <= radius * radius) {
							endTurnButtonHovered_ = true;
							break;
						}
					}
				}
			}
		}

		if (endTurnButtonHovered_ && lTrig) {
			endTurnButtonClicked = true;
		}
	}

	if (turn_ == TurnState::Player) {

		if (cardState_ != CardInputState::ChoosingFieldReplace) {
			fieldReplaceHoverIndex_ = -1;
			prevFieldReplaceHoverIndex_ = -1;
		}

		if (cardState_ == CardInputState::ChoosingFieldReplace ||
			cardState_ == CardInputState::Preview) {
			handView_.SetHoverIndex(-1);
		} else {
			int hover = BattleCardInputController::PickHandIndexByMouse(handView_, cam_->GetViewProjectionMatrix(), mouse.x, mouse.y, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight);
			if (!IsTutorialForcedCardAllowed_(hover)) {
				hover = -1;
			}
			handView_.SetHoverIndex(hover);
		}
	} else {
		handView_.SetHoverIndex(-1);
	}

	if (turn_ == TurnState::Player) {

		if (cardState_ != CardInputState::ChoosingFieldReplace) {
			fieldReplaceHoverIndex_ = -1;
			prevFieldReplaceHoverIndex_ = -1;
		}

		if ((/*enterTrig ||*/ endTurnButtonClicked) && cardState_ == CardInputState::Idle) {

			OutputDebugStringA(("Before EndTurn hand=" + std::to_string(deckZone_.GetHandCount()) +
				" deck=" + std::to_string(deckZone_.GetDeckCount()) +
				" discard=" + std::to_string(deckZone_.GetDiscardCount()) +
				" field=" + std::to_string(field_.size()) + "\n").c_str());

			turn_ = TurnState::Enemy;
			player_->SetPoisonDrawActive(false);
			enemyTurnCount_++;
			hasPendingCard_ = false;
			pendingCard_ = {};
			enemyWait_ = 1.0f;
			handView_.SetHoverIndex(-1);
			handView_.SetDrag(-1, 0, 0, false);
			handView_.SetPreviewIndex(-1);
			selectedIndex_ = -1;
			cardState_ = CardInputState::Idle;
			fieldReplaceHoverIndex_ = -1;
			prevFieldReplaceHoverIndex_ = -1;
		} else {
			if (cardState_ != CardInputState::Preview &&
				cardState_ != CardInputState::ChoosingFieldReplace) {
			int hover = BattleCardInputController::PickHandIndexByMouse(handView_, cam_->GetViewProjectionMatrix(), mouse.x, mouse.y, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight);
			if (!IsTutorialForcedCardAllowed_(hover)) {
				hover = -1;
			}
			handView_.SetHoverIndex(hover);
			} else {
				handView_.SetHoverIndex(-1);
			}

			switch (cardState_) {
			case CardInputState::Idle:
				handView_.SetDrag(-1, 0, 0, false);
				handView_.SetPreviewIndex(-1);

				if (lTrig) {
					int idx = BattleCardInputController::PickHandIndexByMouse(handView_, cam_->GetViewProjectionMatrix(), mouse.x, mouse.y, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight);
					if (!IsTutorialForcedCardAllowed_(idx)) {
						idx = -1;
					}
					const auto handDecision = BattleCardInputController::ResolveIdle(cardInput, idx);
					if (handDecision.action == BattleCardInputController::HandInputAction::StartDrag) {
						selectedIndex_ = handDecision.handIndex;
						dragStartMouse_ = mouse;
						dragDx_ = dragDy_ = 0.0f;
						cardState_ = CardInputState::Dragging;
					}
				}
				break;

			case CardInputState::Dragging:
			{
				const float threshold = 80.0f;
				const auto handDecision = BattleCardInputController::ResolveDragging(cardInput, selectedIndex_, dragStartMouse_.x, dragStartMouse_.y, threshold);

				dragDx_ = handDecision.dragDx;
				dragDy_ = handDecision.dragDy;

				handView_.SetDrag(selectedIndex_, dragDx_, dragDy_, true);

				if (handDecision.action == BattleCardInputController::HandInputAction::OpenPreview ||
					handDecision.action == BattleCardInputController::HandInputAction::ReturnToIdle) {
					handView_.SetDrag(-1, 0, 0, false);

					if (handDecision.action == BattleCardInputController::HandInputAction::OpenPreview) {
						BattleSfxPlayer::PlaySE("SE_CardFlick");
						cardState_ = CardInputState::Preview;
						handView_.SetPreviewIndex(selectedIndex_);
					} else {
						cardState_ = CardInputState::Idle;
						selectedIndex_ = -1;
						handView_.SetPreviewIndex(-1);
					}
				}
			}
			break;

			case CardInputState::Preview:

			{
				handView_.SetPreviewIndex(selectedIndex_);

				if (!IsTutorialForcedCardAllowed_(selectedIndex_)) {
					cardState_ = CardInputState::Idle;
					selectedIndex_ = -1;
					handView_.SetPreviewIndex(-1);
					break;
				}

				if (lTrig) {
					int idx = selectedIndex_;
					if (idx >= 0 && idx < static_cast<int>(deckZone_.GetHandCount())) {
						CardInstance inst = deckZone_.GetHand()[idx];
						const CardDef* def = db_.Find(inst.defId);

						if (def && def->cost <= energy_) {

							bool needsTarget = false;
							int dmgVal = 0;
							int hitCount = 0; // 鬮ｫ・ｰ繝ｻ・ｾ郢晢ｽｻ繝ｻ・ｻ鬮ｫ・ｰ繝ｻ・ｦ驛｢譎｢・ｽ・ｻ髯橸ｽｻ隶鯉ｽ｢繝ｻ・ｬ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｰ驛｢譎｢・ｽ・ｻ髯具ｽｹ繝ｻ・ｻ驛｢譎｢・ｽ・ｰ鬩幢ｽ｢隴弱・・ｽ・ｼ髮具ｽｻ繝ｻ・ｽ陞ｳ螢ｼ・ｵ・ｯ髯ｷ莨夲ｽｽ・ｱ髫ｨ・ｳ霑｢證ｦ・ｽ・ｹ繝ｻ・ｧ髯ｷ・ｿ繝ｻ・･髯橸ｽｻ隶鯉ｽ｢繝ｻ・ｬ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｰ驛｢譎｢・ｽ・ｻ驛｢譎｢・ｽ・ｻ

							for (const auto& effect : def->effects) {
								// 鬯ｯ・ｨ繝ｻ・ｾ髯橸ｽ｢繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｸ郢晢ｽｻ繝ｻ・ｸ鬩幢ｽ｢隰ｨ魑ｴﾂ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｸ驛｢譎｢・ｽ・ｻ驛｢譎｢・ｽ・ｻverClock鬯ｩ蛹・ｽｽ・ｲ髯晢ｽｲ繝ｻ・ｨ驍ｵ・ｲ陝ｶ譎丞楜驛｢譎｢・ｽ・ｻ髴取ｺｷ・､繧托ｽｽ・ｸ繝ｻ・ｺ驛｢・ｧ郢晢ｽｻ繝ｻ・ｽ驍・戟謐礼ｹ晢ｽｻ繝ｻ・ｴ鬮ｯ・ｷ繝ｻ・ｷ髯具ｽｹ繝ｻ・ｻ驛｢譎｢・ｽ・ｻ鬮ｯ・ｷ闔ｨ螟ｲ・ｽ・｣繝ｻ・ｰ鬯ｩ髦ｪ・・濤・ｲ郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ繝ｻ・･驛｢譎｢・ｽ・ｻ驛｢譎｢・ｽ・ｻ
								if (effect.type == "Damage") {
									needsTarget = true;
									dmgVal += effect.value;
									hitCount++;
								}
								// 鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｯ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｬ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｻ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩幢ｽ｢隴主・讓溘・荳ｻ・ｰ・､繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ驛｢譎｢・ｽ・ｻ髣費｣ｰ繝ｻ・･郢晢ｽｻ繝ｻ・･驛｢譎｢・ｽ・ｻ髴取ｺｷ・､繧托ｽｽ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｿ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｪ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ3鬩幢ｽ｢隰ｨ魑ｴﾂ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｸ驛｢譎｢・ｽ・ｻ驛｢譎｢・ｽ・ｻ
								else if (effect.type == "DamageCrescent") {
									needsTarget = true;
									int val = effect.value;
									if (playerTurnCount_ % 2 != 0) {
										val += 3; // 鬮ｯ讒ｭ・・ｹ晢ｽｻ髴取ｺｷ・､繧托ｽｽ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｿ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｪ鬩幢ｽ｢繝ｻ・ｧ鬮｣繝ｻ・ｽ・ｽ郢晢ｽｻ繝ｻ・ｿ郢晢ｽｻ繝ｻ・ｽ鬮ｯ・ｷ闔ｨ螟ｲ・ｽ・｣繝ｻ・ｰ鬩幢ｽ｢隰ｨ魑ｴﾂ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｸ
									}
									dmgVal += val;
									hitCount++;
								}
								// 鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｷ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｫ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ驛｢譎｢・ｽ・ｰ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驍ｵ・ｺ陷･謫ｾ・ｽ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・･
								else if (effect.type == "DamageByBlock") {
									needsTarget = true;
									dmgVal += ScaleEffectAmount_(player_ ? player_->GetBlock() : 0, effect);
									hitCount++;
								}
								if (effect.type == "Poison") {
									needsTarget = true;
									hitCount++;
								}
								if (effect.type == "Frost") {
									needsTarget = true;
									hitCount++;
								}
								if (effect.type == "FrostDraw") {
									needsTarget = true;
								}
							}

							// 鬩幢ｽ｢繝ｻ・ｧ驛｢・ｧ郢晢ｽｻ繝ｻ・ｼ繝ｻ・ｰ鬮ｫ・ｰ繝ｻ・ｾ郢晢ｽｻ繝ｻ・ｻ鬮ｫ・ｰ繝ｻ・ｦ驛｢譎｢・ｽ・ｻ驍ｵ・ｺ陷･・ｲ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ驕ｶ莨√・繝ｻ・ｹ繝ｻ・ｧ髯晢ｽｲ繝ｻ・ｨ繝ｻ縺､ﾂ驕ｶ謫ｾ・ｽ・ｫ髯具ｽｹ繝ｻ・ｱ鬮ｯ・ｷ陝雜｣・ｽ・ｼ隰夲ｽｫ鬮ｮ・ｷ鬩搾ｽｵ繝ｻ・ｺ髯橸ｽ｢繝ｻ・ｹ驕ｶ莨∬ｱｪ繝ｻ・ｸ繝ｻ・ｲ髫ｴ・ｴ繝ｻ・ｧ鬯ｮ・ｮ繝ｻ・ｰ鬩幢ｽ｢繝ｻ・ｧ髯晢ｽｶ隴擾ｽｶ郢晢ｽｻ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｶ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・｢鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ繝ｻ縺､ﾂ鬯ｮ・ｦ繝ｻ・ｪ驕ｶ蜀暦ｽ｣・ｯ・ゑｽｧ郢晢ｽｻ繝ｻ・ｻ鬯ｮ・ｯ繝ｻ・ｦ鬯ｲ繝ｻ・ｼ螟ｲ・ｽ・ｽ繝ｻ・ｼ驛｢譎｢・ｽ・ｻ
							if (needsTarget) {
								int buff = currentTurnAtkUp_ + (player_ ? player_->GetBoostedPower() : 0);
								pendingDamage_ = dmgVal + (buff * hitCount);

								isPokerDamageTargeting_ = false;                 // 鬮ｫ・ｰ郢晢ｽｻ驍・・・ｫ・ｰ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴寂・繝ｻ鬩励ｑ・ｽ・ｰ鬮ｫ・ｴ陞滂ｽｲ繝ｻ・ｽ繝ｻ・･
								pendingCardHandIndex_ = idx;                     // 鬮｣蜴・ｽｽ・ｴ郢晢ｽｻ繝ｻ・ｿ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・｣鬩搾ｽｵ繝ｻ・ｺ髮倶ｼ∬ｱｪ郢晢ｽｻ鬮ｫ・ｴ陝ｷ・｢繝ｻ・ｽ繝ｻ・ｭ鬮｣蜴・ｽｽ・ｴ髯ｷ・･繝ｻ・ｲ郢晢ｽｻ繝ｻ・ｽ郢晢ｽｻ繝ｻ・ｮ鬩幢ｽ｢繝ｻ・ｧ髯橸ｽｳ陞滂ｽｲ繝ｻ・ｽ繝ｻ・ｦ髯橸ｽ｢繝ｻ・ｹ驕ｶ謫ｾ・ｽ・ｴ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ
								handView_.SetFocusIndex(idx);
								cardState_ = CardInputState::ChoosingEnemyTarget; // 鬮ｫ・ｰ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｵ鬯ｯ・ｩ陋ｹ繝ｻ・ｽ・ｽ繝ｻ・ｸ鬮ｫ・ｰ陞｢・ｽ繝ｻ・ｧ繝ｻ・ｭ驕ｶ荳翫・

								selectedIndex_ = -1;
								handView_.SetPreviewIndex(-1);
								return;
							}

							pendingCardHandIndex_ = idx;
							Enemy* sequenceTarget = nullptr;
							if (enemyMgr_) {
								for (auto& enemy : enemyMgr_->GetEnemies()) {
									if (enemy.IsAlive()) {
										sequenceTarget = &enemy;
										break;
									}
								}
							}

							if (sequenceTarget && BeginCardActionSequence_(app, *def, inst, *sequenceTarget)) {
								handView_.SetFocusIndex(idx);
								cardState_ = CardInputState::ExecutingSequence;

								selectedIndex_ = -1;
								handView_.SetPreviewIndex(-1);
								return;
							}

							energy_ -= def->cost;
							BattleSfxPlayer::PlaySE("SE_CardPlay");
							BattleSfxPlayer::PlayAttackSEForCard(*def);
							auto usedCardView = handView_.ExtractCardAt(idx);
							deckZone_.RemoveHandAt(static_cast<std::size_t>(idx));
							//handView_.Rebuild(hand_);



							ApplyCardEffects_(*def);

							if ((int)field_.size() < 5) {
								field_.push_back(inst);
								if (usedCardView) {
									usedCardView->SetIsHand(false);
									fieldViews_.push_back(std::move(usedCardView));
								}
								RebuildFieldView_();
								if ((int)field_.size() == 5) {
									PokerHandResult poker = EvaluatePokerHand_();
									TriggerSubEffectsForCard_(
										inst,
										SubEffectTrigger::OnPlayToField,
										poker.rank
									);
								}

								cardState_ = CardInputState::Idle;
								hasPendingCard_ = false;
								pendingCard_ = {};
							} else {
								pendingCard_ = inst;
								hasPendingCard_ = true;
								pendingCardView_ = std::move(usedCardView);
								cardState_ = CardInputState::ChoosingFieldReplace;
							}
							OnPlayerCardUsed_();
						} else {
							cardState_ = CardInputState::Idle;
						}
					} else {
						cardState_ = CardInputState::Idle;
					}

					selectedIndex_ = -1;
					handView_.SetPreviewIndex(-1);
				}
				const auto previewDecision = BattleCardInputController::ResolvePreview(cardInput, selectedIndex_);
				if (previewDecision.action == BattleCardInputController::HandInputAction::CancelPreview) {
					cardState_ = CardInputState::Idle;
					selectedIndex_ = -1;
					handView_.SetPreviewIndex(-1);
				}
				break;
			}

			case CardInputState::ChoosingFieldReplace:
			{
				if (pendingCardView_) {

					Vector3 previewPos = { -10.f, 2.0f, 3.0 };
					pendingCardView_->SetTransform(previewPos, { 0.0f, 0.0f, 0.0f }, { 1.f, 1.f, 1.f });
					pendingCardView_->Update(dt); // 鬮ｫ・ｰ繝ｻ・ｰ髯ｷﾂ隲､諛医・鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩搾ｽｵ繝ｻ・ｺ髮九・竏槭・・ｽ遶擾ｽｫ繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫUpdate鬩幢ｽ｢繝ｻ・ｧ鬮ｮ蛹ｺ・ｨ雋ｻ・ｽ・ｻ闕ｵ貊ゑｽｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｶ
				}

				int newHover = PickFieldIndexByMouse_(mouse.x, mouse.y);
				const auto fieldReplaceDecision = BattleCardInputController::ResolveFieldReplaceInput(cardInput, newHover);

				if (fieldReplaceDecision.hoverIndex != fieldReplaceHoverIndex_) {
					fieldReplaceHoverIndex_ = fieldReplaceDecision.hoverIndex;
					fieldLayoutDirty_ = true;
				}

				if (fieldReplaceDecision.replaceRequested) {
					int replaceIndex = fieldReplaceHoverIndex_;
					if (replaceIndex >= 0 && replaceIndex < (int)field_.size() && hasPendingCard_) {
						BattleSfxPlayer::PlaySE("SE_CardFlick");
						if (fieldViews_[replaceIndex]) {
							handView_.AddDiscardingCard(std::move(fieldViews_[replaceIndex]));
						}
						deckZone_.AddToDiscard(field_[replaceIndex]);
						field_[replaceIndex] = pendingCard_;
						if (pendingCardView_) {
							pendingCardView_->SetIsHand(false);
							fieldViews_[replaceIndex] = std::move(pendingCardView_);
						}
						RebuildFieldView_();
						RebuildDiscardView_();

						PokerHandResult poker = EvaluatePokerHand_();
						TriggerSubEffectsForCard_(
							pendingCard_,
							SubEffectTrigger::OnPlayToField,
							poker.rank
						);

						hasPendingCard_ = false;
						pendingCard_ = {};
						fieldReplaceHoverIndex_ = -1;
						prevFieldReplaceHoverIndex_ = -1;
						cardState_ = CardInputState::Idle;
						fieldLayoutDirty_ = true;
					}
				}

				if (fieldReplaceDecision.cancelRequested) {
					if (hasPendingCard_) {
						BattleSfxPlayer::PlaySE("SE_CardFlick");
						deckZone_.AddToDiscard(pendingCard_);
					}
					if (pendingCardView_) {
						handView_.AddDiscardingCard(std::move(pendingCardView_));
					}
					hasPendingCard_ = false;
					pendingCard_ = {};

					fieldReplaceHoverIndex_ = -1;
					prevFieldReplaceHoverIndex_ = -1;
					cardState_ = CardInputState::Idle;
					RebuildDiscardView_();
					fieldLayoutDirty_ = true;
				}

				handView_.SetHoverIndex(-1);
				handView_.SetDrag(-1, 0, 0, false);
				handView_.SetPreviewIndex(-1);
			}
			break;
			case CardInputState::ExecutingSequence:
			{
				// Handled in BattleController::Update
			}
			break;
			case CardInputState::ChoosingEnemyTarget:
			{
				// 鬩幢ｽ｢隴弱・・ｽ・ｧ繝ｻ・ｭ驍ｵ・ｺ髢ｧ・ｲ繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮｣蜴・ｽｽ・ｴ髯ｷ・･繝ｻ・ｲ郢晢ｽｻ繝ｻ・ｽ郢晢ｽｻ繝ｻ・ｮ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ驍・私・ｽ・ｬ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｵ鬩幢ｽ｢繝ｻ・ｧ髯ｷ・ｻ髢ｧ・ｲ驍頑ｻ・＠繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
				int hoverIndex = cardTargetingController_.PickHoveredEnemy(
					enemyMgr_,
					cam_,
					mouse.x,
					mouse.y,
					static_cast<float>(WinApp::kClientWidth),
					static_cast<float>(WinApp::kClientHeight));

				// 鬩幢ｽ｢隴弱・・ｽ・ｧ繝ｻ・ｭ驍ｵ・ｺ髢ｧ・ｲ繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩搾ｽｵ繝ｻ・ｺ鬩募●繝ｻ髯ｬ貊・＠繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｪ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・｣鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ驍・私・ｽ・ｬ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｵ鬩幢ｽ｢繝ｻ・ｧ髯晢ｽｶ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｻ驛｢譎｢・ｽ・ｻ髴大､ｲ・ｽ・｡鬩搾ｽｵ繝ｻ・ｺ髣包ｽｳ隶抵ｽｭ郢晢ｽｻ鬩幢ｽ｢繝ｻ・ｧ髯晢ｽｲ繝ｻ・ｨ髫ｨ・ｳ霑｢證ｦ・ｽ・ｹ繝ｻ・ｧ驛｢譎｢・ｽ・ｻ
				cardTargetingController_.ClearHighlights(enemyMgr_);
				cardTargetingController_.ApplyHoverHighlight(enemyMgr_, hoverIndex);
                const auto targetDecision = BattleCardInputController::ResolveEnemyTargetInput(
                    hoverIndex,
                    lTrig,
                    rTrig,
                    isPokerDamageTargeting_ && tutorialLockPokerTargetingCancel_);

				// 鬮ｯ譎｢・ｽ・ｾ郢晢ｽｻ繝ｻ・ｦ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｯ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｪ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驍ｵ・ｺ鬩｢謳ｾ・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬮ｮ謇具ｽｶ・｣繝ｻ・ｽ繝ｻ・ｺ鬮ｯ讖ｸ・ｽ・ｳ髯橸ｽ｢繝ｻ・ｹ郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬮ｫ・ｰ繝ｻ・ｾ郢晢ｽｻ繝ｻ・ｻ鬮ｫ・ｰ繝ｻ・ｦ驛｢譎｢・ｽ・ｻ
				if (targetDecision.confirmRequested) {
					if (cardTargetingController_.IsValidTarget(enemyMgr_, targetDecision.targetIndex)) {
						Enemy& targetEnemy = enemyMgr_->GetEnemies()[targetDecision.targetIndex];
						currentEnemyIndex_ = targetDecision.targetIndex;

						if (isPokerDamageTargeting_) {
							actionSequenceTarget_ = &targetEnemy;
							ExecutePendingAttack_(targetEnemy);
							return;
						}

						const CardInstance& inst = deckZone_.GetHand()[pendingCardHandIndex_];
						const CardDef* def = db_.Find(inst.defId);
						if (def && BeginCardActionSequence_(app, *def, inst, targetEnemy)) {
							cardState_ = CardInputState::ExecutingSequence;
						} else {
							actionSequenceTarget_ = &targetEnemy;
							ExecutePendingAttack_(targetEnemy);
						}
					}
				}

				// 鬮ｯ・ｷ繝ｻ・ｿ郢晢ｽｻ繝ｻ・ｳ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｯ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｪ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驍ｵ・ｺ闔会ｽ｣郢晢ｽｻ髯橸ｽ｢繝ｻ・ｹ驍ｵ・ｺ陷證ｦ・ｽ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・｣鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｻ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｫ鬩搾ｽｵ繝ｻ・ｺ髯ｷ莨夲ｽｽ・ｱ驕ｯ・ｶ繝ｻ・ｻ鬮ｯ・ｷ陋ｹ・ｻ郢晢ｽｻ驕ｶ鬆托ｽ･・｢繝ｻ・ｬ鬲・ｼ夲ｽｽ・ｽ繝ｻ・ｻ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ
				if (targetDecision.cancelRequested) {
					if (targetDecision.action == BattleCardInputController::TargetAction::LockedCancel) {
						handView_.SetFocusIndex(-1);
						handView_.SetHoverIndex(-1);
						handView_.SetPreviewIndex(-1);
						return;
					}

					handView_.SetFocusIndex(-1);
					cardState_ = CardInputState::Idle;

					if (isPokerDamageTargeting_) {
						pokerChoiceState_ = PokerChoiceState::WaitingEffectChoice; // 鬩幢ｽ｢隴弱・・ｺ・｢驛｢譎｢・ｽ・ｻ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬯ｯ・ｩ陋ｹ繝ｻ・ｽ・ｽ繝ｻ・ｸ鬮ｫ・ｰ陞｢・ｽ繝ｻ・ｧ繝ｻ・ｭ驕ｶ蝓弱Γ繝ｻ・ｬ鬲・ｼ夲ｽｽ・ｽ繝ｻ・ｻ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ
					}
				}
			}
			break;
			}
		}

	} else {
		// --------------------------------------------------
		// 鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｨ鬩幢ｽ｢隴取ｨ費ｽｺ繧托ｽｾ譛ｱ繝ｻ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｿ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮ｯ・ｷ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｦ鬯ｨ・ｾ郢晢ｽｻ郢晢ｽｻ
		// --------------------------------------------------
		handView_.SetHoverIndex(-1);
		handView_.SetDrag(-1, 0, 0, false);
		handView_.SetPreviewIndex(-1);
		cardState_ = CardInputState::Idle;
		selectedIndex_ = -1;
		enemyMgr_&& player_;
		hasPendingCard_ = false;
		pendingCard_ = {};

		enemyWait_ -= dt;

		if (enemyWait_ <= 0.0f) {

			if (enemyMgr_ && player_) {
				auto& enemies = enemyMgr_->GetEnemies();
				while (currentEnemyIndex_ < enemies.size() &&
					(!enemies[currentEnemyIndex_].IsAlive() ||
						enemyActionCountSystem_.ShouldSkipEnemyTurn(currentEnemyIndex_))) {
					currentEnemyIndex_++;
				}

				// 鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｾ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬯ｮ・ｯ繝ｻ・ｦ髫ｰ逍ｲ・ｺ・ｯ陷牙ｸ昴＠繝ｻ・ｺ髯ｷ莨夲ｽｽ・ｱ驕ｯ・ｶ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ驕ｶ莨√・繝ｻ・ｸ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ鬯ｮ・ｮ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ髯滓坩・ｯ莨夲ｽｽ・ｼ隶捺慣・ｽ・ｹ繝ｻ・ｧ髯ｷ・ｿ繝ｻ・･郢晢ｽｻ繝ｻ・ｰ郢晢ｽｻ繝ｻ・ｴ鬮ｯ・ｷ繝ｻ・ｷ驛｢譎｢・ｽ・ｻ
				if (currentEnemyIndex_ < enemies.size()) {

					// 鬮｣遒大ｴ溯楜・ｦ髯橸ｽｻ鬯･・ｴ陷崎挙・ｬ逍ｲ・ｺ・ｯ陷牙ｸ昴＠繝ｻ・ｺ髯ｷ・ｷ繝ｻ・ｶ郢晢ｽｻ驍・私・ｽ・ｬ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｵ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ鬮｣蜴・ｽｽ・ｴ鬮ｦ・ｮ陷ｷ・ｮ陷ｻ・ｳ鬩搾ｽｵ繝ｻ・ｺ髯樊ｻゑｽｽ・ｧ髯ｷ・ｿ陷ｻ・ｵ繝ｻ・ｰ髴域鱒繝ｻ
					Enemy& e = enemies[currentEnemyIndex_];
					EnemyAction action = e.GetBossAI().GetNextAction();

					ExecuteEnemyAction_(e, action);

					enemyWait_ = 1.0f;

					// 鬮ｫ・ｹ繝ｻ・ｺ郢晢ｽｻ繝ｻ・｡鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮ｫ・ｰ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｵ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬯ｨ・ｾ繝ｻ・｡郢晢ｽｻ繝ｻ・ｪ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｸ鬯ｯ・ｨ繝ｻ・ｾ郢晢ｽｻ繝ｻ・ｲ鬩幢ｽ｢繝ｻ・ｧ驕ｶ荳橸ｽ｣・ｺ・つ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ鬩怜遜・ｽ・ｫ郢晢ｽｻ繝ｻ・･
					currentEnemyIndex_++;

				} else {
					// 鬮ｯ・ｷ髣鯉ｽｨ繝ｻ・ｽ繝ｻ・ｨ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮ｫ・ｰ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｵ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬯ｮ・ｯ繝ｻ・ｦ髫ｰ逍ｲ・ｺ・ｯ陷牙ｸ昴＠繝ｻ・ｺ髫ｶ蜻ｵ・ｶ・｣繝ｻ・ｽ繝ｻ・ｵ驛｢・ｧ郢晢ｽｻ繝ｻ・ｽ陷證ｦ・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・｣鬩搾ｽｵ繝ｻ・ｺ髮九・・ｽ・ｷ郢晢ｽｻ繝ｻ・ｰ郢晢ｽｻ繝ｻ・ｴ鬮ｯ・ｷ繝ｻ・ｷ驛｢譎｢・ｽ・ｻ

					// 鬮ｴ謇假ｽｽ・･郢晢ｽｻ繝ｻ・ｶ鬮ｫ・ｲ繝ｻ・ｷ髴托ｽ｢驕会ｽｼ郢晢ｽｻ鬮ｯ譎｢・ｽ・ｶ郢晢ｽｻ繝ｻ・ｸ鬩幢ｽ｢隰ｨ魑ｴﾂ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｸ鬮ｯ・ｷ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｦ鬯ｨ・ｾ郢晢ｽｻ郢晢ｽｻ
					if (enemyMgr_) {
						// 鬮ｯ・ｷ髣鯉ｽｨ繝ｻ・ｽ繝ｻ・ｨ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮ｫ・ｰ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｵ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬮ｯ譏ｴ繝ｻ繝ｻ・ｽ繝ｻ・ｾ鬩搾ｽｵ繝ｻ・ｺ髯ｷ莨夲ｽｽ・ｱ驕ｯ・ｶ繝ｻ・ｻ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｫ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴弱・ﾂ隲帷ｿｫ繝ｻ鬯ｨ・ｾ郢晢ｽｻ郢晢ｽｻ郢晢ｽｻ陝ｶ譎剰ｷ晞辧蜍滂ｽｨ・ｯ鬲假ｽｬ
						for (auto& enemy : enemyMgr_->GetEnemies()) {
							// 鬯ｨ・ｾ陟・屮・ｽ・ｺ陋滂ｽｪ・つ繝ｻ・ｳ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ驍・私・ｽ・ｬ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｵ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｿ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬮｣逧ｮ逕･・つ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｸ驛｢譎｢・ｽ・ｻ
							if (enemy.IsAlive()) {
								enemy.TurnEndApplyBC();

								// 鬮ｯ貊ゑｽｽ・｢驛｢譎｢・ｽ・ｻ郢晢ｽｻ繝ｻ・ｦ驕ｶ荳橸ｽ｣・ｹ・つ陜｣・､繝ｻ・ｸ繝ｻ・ｺ驛｢・ｧ郢晢ｽｻ繝ｻ・ｽ隶呵ｶ｣・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬮ｯ・ｷ繝ｻ・ｷ驛｢譎｢・ｽ・ｻ鬯ｮ・ｮ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬯ｯ・ｯ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｭ鬮｣蛹・ｽｽ・ｳ鬩怜遜・ｽ・ｫ驕ｶ莨∬ｱｪ繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｨ鬩幢ｽ｢隴弱・・ｽ・ｼ隴∫ｵｶ蜃ｾ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｯ鬩幢ｽ｢隴主・讓溽ｹ晢ｽｻ郢晢ｽｻ繝ｻ・ｬ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｰ鬮ｯ蛹ｺ・ｻ繧托ｽｽ・ｽ繝ｻ・､鬩幢ｽ｢繝ｻ・ｧ鬮ｮ蛹ｺ・ｧ・ｭ郢晢ｽｻ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
								// SpawnDamagePopup(enemy.GetPos(), effect.value, false); 
							}
						}
					}

					currentEnemyIndex_ = 0; // 鬮ｫ・ｹ繝ｻ・ｺ郢晢ｽｻ繝ｻ・｡鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｿ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬮ｯ・ｷ繝ｻ・ｷ髣比ｼ夲ｽｽ・｣郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｪ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｻ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驛｢譎｢・ｽ・ｨ鬩搾ｽｵ繝ｻ・ｺ髯ｷ莨夲ｽｽ・ｱ驕ｯ・ｶ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ鬩怜遜・ｽ・ｫ郢晢ｽｻ繝ｻ・･

					// 鬩幢ｽ｢隴惹ｸ橸ｽｹ・ｲ繝ｻ蜿厄ｽｨ謚ｵ・ｽ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・､鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｿ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｸ鬯ｩ蜍溪・繝ｻ・ｽ繝ｻ・ｻ鬯ｮ・ｯ繝ｻ・ｦ驛｢譎｢・ｽ・ｻ
					turn_ = TurnState::Player;
					StartPlayerTurn_();
				}
			}
		}
	}

	if (fieldLayoutDirty_ || cardState_ == CardInputState::ChoosingFieldReplace || pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi) {
		RefreshAllFieldCardTransforms_(dt);
		fieldLayoutDirty_ = false;
	}
	for (auto& cardView : fieldViews_) {
		if (cardView) {
			cardView->Update(dt);
		}
	}
	if (discardView_) {
		discardView_->Update(dt);
	}
	if (player_) {
		playerLastHp_ = player_->GetHP();
	}

}

void BattleController::UpdateVisuals_(float dt)
{
	sPokerGlowRainbowTime += dt;

	const bool actionOrEnemyAttack =
		actionDirector_.IsPlaying() ||
		cardState_ == CardInputState::ExecutingSequence ||
		turn_ == TurnState::Enemy;

	if (actionOrEnemyAttack) {
		handView_.Update(dt);
		if (discardView_) {
			discardView_->Update(dt);
		}

		damagePopupUi_.Update(dt);
		return;
	}

	// 1. 鬩幢ｽ｢隴弱・・ｽ・ｼ隴∫ｵｶ隘夜ｩ幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｫ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ驍ｵ・ｺ陷･・ｲ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ驛｢譎｢・ｽ・ｻ鬮ｫ・ｴ陷ｴ繝ｻ・ｽ・ｽ繝ｻ・ｴ鬮ｫ・ｴ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｰ驛｢譎｢・ｽ・ｻ髯具ｽｹ繝ｻ・ｻ繝ｻ荳ｻ・ｸ・ｷ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｻ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・｢鬩幢ｽ｢隴乗・・ｽ・ｹ隴∵ｻ・ｱｪ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｷ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｧ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬯ｯ・ｨ繝ｻ・ｾ郢晢ｽｻ繝ｻ・ｲ鬯ｮ・ｯ繝ｻ・ｦ鬯ｲ繝ｻ・ｼ螟ｲ・ｽ・ｽ繝ｻ・ｼ驛｢譎｢・ｽ・ｻ
	for (auto& cardView : fieldViews_) {
		if (cardView) {
			cardView->Update(dt); // 鬩搾ｽｵ繝ｻ・ｺ鬮ｦ・ｮ陷ｻ・ｻ繝ｻ・ｽ隶呵ｶ｣・ｽ・ｸ繝ｻ・ｺ髫ｰ逍ｲ・ｻ莨夲ｽｽ・ｻ闕ｵ貊ゑｽｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬩幢ｽ｢繝ｻ・ｧ髯滓坩・ｯ莨夲ｽｽ・ｽ隶呵ｶ｣・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｩ鬩幢ｽ｢繝ｻ・ｧ鬮ｦ・ｮ陷ｷ・ｮ郢晢ｽｻ鬮ｴ謇假ｽｽ・･郢晢ｽｻ繝ｻ・ｶ鬮ｫ・ｲ繝ｻ・ｷ髣包ｽｵ隴擾ｽｴ・つ陜｣・､繝ｻ・ｹ繝ｻ・ｧ驛｢・ｧ郢晢ｽｻ陝ｶ・ｷ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・｡鬩搾ｽｵ繝ｻ・ｺ髫ｰ逍ｲ・ｺ・ｯ陷牙ｸ昴＠繝ｻ・ｺ鬯ｮ・ｦ繝ｻ・ｪ驕ｶ謫ｾ・ｽ・ｪ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
		}
	}

	// 2. 鬮ｯ貅ｯ・ｶ・｣繝ｻ・ｽ繝ｻ・ｧ鬮ｫ・ｶ霓｣蛟｡蜃ｽ驛｢譎｢・ｽ・ｻ鬯ｩ蜍溪・繝ｻ・ｽ繝ｻ・ｻ鬮ｯ・ｷ隶主･・ｽｽ・｢霓｣蛛・ｽｽ・ｳ繝ｻ・ｩ鬮ｫ・ｴ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｰ驛｢譎｢・ｽ・ｻ鬮｢・ｧ繝ｻ・ｲ髯晢ｽｯ繝ｻ・ｼ鬮ｫ・ｶ霓｣蛟ｬ螳・坿・ｷ陝雜｣・ｽ・ｽ繝ｻ・､郢晢ｽｻ繝ｻ・ｹ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｸ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩幢ｽ｢隴趣ｽ｢繝ｻ・｣繝ｻ・ｰ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｺ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬮ｯ・ｷ陝雜｣・ｽ・ｼ隴夲ｽｿ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ髯ｷ・ｻ繝ｻ・ｻ郢晢ｽｻ繝ｻ・ｼ驛｢譎｢・ｽ・ｻ
	if (fieldLayoutDirty_ ||
		cardState_ == CardInputState::ChoosingFieldReplace ||
		pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi)
	{
		RefreshAllFieldCardTransforms_(dt);
		// 鬩包ｽｯ繝ｻ・ｶ郢晢ｽｻ繝ｻ・ｻ Refresh~ 鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮｣蛹・ｽｽ・ｳ郢晢ｽｻ繝ｻ・ｭ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ fieldLayoutDirty_ = false; 鬩幢ｽ｢繝ｻ・ｧ髯句ｹ｢・ｽ・ｵ郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ霑｢證ｦ・ｽ・ｸ繝ｻ・ｺ鬮ｦ・ｮ陷ｷ・ｮ郢晢ｽｻ鬩幢ｽ｢繝ｻ・ｧ髯懶ｽ｣繝ｻ・､郢晢ｽｻ繝ｻ・｢郢晢ｽｻ繝ｻ・ｺ鬯ｮ・ｫ繝ｻ・ｱ驛｢譎｢・ｽ・ｻ
	}

	// 3. 鬮ｫ・ｰ郢晢ｽｻ驍・・・ｫ・ｰ郢晢ｽｻ繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩搾ｽｵ繝ｻ・ｲ髫ｰ逍ｲ・ｺ蛟･繝ｻ鬩搾ｽｵ繝ｻ・ｺ髯ｷ・ｷ繝ｻ・ｶ驕ｶ髮・ｽｮ螢ｽ蜑ｲ髫ｲ蟶幢ｽ･繝ｻ・ｽ・ｽ隶呵ｶ｣・ｽ・ｹ繝ｻ・ｧ髯具ｽｹ繝ｻ・ｺ髫ｲ・､陷･雜｣・ｽ・ｬ繝ｻ・ｮ髣費ｽｨ隲幢ｽｶ繝ｻ・ｽ繝ｻ・ｽ郢晢ｽｻ繝ｻ・ｹ鬩搾ｽｵ繝ｻ・ｲ鬯ｮ・ｦ繝ｻ・ｪ驛｢譎｢・ｽ・ｻ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｬ鬩幢ｽ｢隴寂或・ｾ・ｭ繝ｻ螳茨ｽ､・ｼ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ鬮ｮ蛹ｺ・ｨ螂・ｽｽ・ｸ陞溷･・ｽｽ・ｭ隰ｫ・ｾ繝ｻ・｣繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ髯ｷ莨夲ｽｽ・ｱ驕ｯ・ｶ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ髣包ｽｵ隴趣ｽ｢繝ｻ・ｽ髣・ｽｽ繝ｻ・ｭ陷ｴ繝ｻ・ｽ・ｽ繝ｻ・ｴ鬮ｫ・ｴ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｰ
	UpdateHandPokerPreviewEffects_();
	handView_.Update(dt);

	// 4. 鬮ｯ貅倥・・ゑｽｧ髫ｲ・ｷ陷･・ｲ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｻ鬮ｫ・ｰ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｰ鬮ｯ譏ｴ繝ｻ陝ｷ・ｲ驛｢譎｢・ｽ・ｻ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驛｢譎｢・ｽ・ｻ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・｢鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驛｢譎｢・ｽ・ｻ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮ｫ・ｴ陷ｴ繝ｻ・ｽ・ｽ繝ｻ・ｴ鬮ｫ・ｴ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｰ
	if (discardView_) discardView_->Update(dt);

	damagePopupUi_.Update(dt);

	// 5. HP鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｲ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｸ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｻUI鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｻ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩幢ｽ｢隴惹ｸ橸ｽｹ・ｲ繝ｻ荳ｻ・ｸ・ｷ繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬩幢ｽ｢隴主・讓滄Δ譎｢・ｽ・ｻ鬯ｮ・ｯ繝ｻ・ｦ髫ｰ逍ｲ・ｺ蛟･繝ｻ鬮ｫ・ｴ陷ｴ繝ｻ・ｽ・ｽ繝ｻ・ｴ鬮ｫ・ｴ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｰ
	UpdateHpGauges();

	Matrix4x4 viewMat = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 projMat = Matrix4x4::MakeOrthographicMatrix((float)WinApp::kClientWidth, (float)WinApp::kClientHeight);

	if (enemyMgr_) {
		auto& enemies = enemyMgr_->GetEnemies();
		enemyStatusUi_.UpdateLayout(enemies, enemyActionCountSystem_.GetCounts(), enemyActionCountSystem_.GetActedFlags(), viewMat, projMat);
	}
	if (highlightFilter_)highlightFilter_->Update(viewMat, projMat);

	if (propManager_) {
		propManager_->Update(dt);

		// EndTurn鬩幢ｽ｢隴弱・魃ｵ驍ｵ・ｺ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ驛｢譎｢・ｽ・ｻ驛｢譎｢・ｽ・ｻrop驛｢譎｢・ｽ・ｻ髯晢ｽｲ繝ｻ・ｨ驛｢譎｢・ｽ・ｻ鬩幢ｽ｢隴取得・ｽ・ｸ陷ｷ・ｶ・趣ｽ｣鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬮ｫ・ｴ陟托ｽｱ繝ｻ繝ｻ・ｹ譎｢・ｽ・ｻ鬩幢ｽ｢隴弱・・ｽ・ｼ隴∫ｵｶ隘夜ｩ幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ驛｢譎｢・ｽ・ｰ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驍ｵ・ｺ郢晢ｽｻ
		for (auto& prop : propManager_->GetPropsMutable()) {
			if (prop.name == "Button" || prop.name == "EndTurnButton") {
				if (endTurnButtonHovered_) {
					prop.object->SetIntensity(prop.lightIntensity * 0.3f); // 鬮ｫ・ｴ霑壼雀・ｹ・ｲ郢晢ｽｻ繝ｻ・･鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｪ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ
				} else {
					// 鬩幢ｽ｢隴取得・ｽ・ｸ陷ｷ・ｶ・趣ｽ｣鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩搾ｽｵ繝ｻ・ｺ髯ｷ莨夲ｽｽ・ｱ驕ｯ・ｶ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ驕ｶ莨√・繝ｻ・ｸ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ髯ｷ繝ｻ・ｽ・ｾ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｯ鬮ｯ・ｷ陋ｹ・ｻ郢晢ｽｻ驛｢譎｢・ｽ・ｻ鬮ｴ謇假ｽｽ・･郢晢ｽｻ繝ｻ・ｶ鬮ｫ・ｲ繝ｻ・ｷ髣包ｽｵ隴擾ｽｶ郢晢ｽｻ鬮ｫ・ｰ鬲・ｼ夲ｽｽ・ｽ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
					prop.object->SetIntensity(prop.lightIntensity);
				}
				prop.object->Update(dt); // 鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｩ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬩幢ｽ｢隴主・讓滄Δ譎｢・ｽ・ｻ鬮ｯ讓奇ｽｺ・ｽ陋ｻ・､髯晢ｽｲ繝ｻ・ｩ鬩幢ｽ｢繝ｻ・ｧ髯懶ｽ｣繝ｻ・､郢晢ｽｻ繝ｻ・｢郢晢ｽｻ繝ｻ・ｺ鬮ｯ讖ｸ・ｽ・ｳ驛｢譎｢・ｽ・ｻ
			}
		}
	}
}

void BattleController::HandlePokerActivateChoice_(FieldUi& fieldUi, POINT mouse, bool lTrig, bool yTrig, bool nTrig)
{
	if (pokerChoiceJustOpened_) {
		pokerChoiceJustOpened_ = false;
		return;
	}

	const auto& layout = fieldUi.GetPokerEffectChoiceLayout();
	const auto decision = PokerChoiceController::ResolveActivateChoice(
		layout,
		mouse.x,
		mouse.y,
		lTrig,
		yTrig,
		nTrig,
		tutorialActivateOnly_);
	const auto& hover = decision.hover;

	if (hover.infoHovered) {
		pokerMouseChoice_ = PokerMouseChoice::None;
	} else if (hover.choice != PokerChoiceController::Choice::None) {
		pokerMouseChoice_ = ToPokerMouseChoice_(hover.choice);
	}

	switch (decision.action) {
	case PokerChoiceController::ActivateAction::ToggleInfo:
		pokerQuickPreviewVisible_ = !pokerQuickPreviewVisible_;
		return;

	case PokerChoiceController::ActivateAction::Activate:
		pokerQuickPreviewVisible_ = false;
		pokerChoiceState_ = PokerChoiceState::WaitingEffectChoice;
		pokerChoiceJustOpened_ = true;
		return;

	case PokerChoiceController::ActivateAction::Skip:
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Skipped;
		pokerChoiceState_ = PokerChoiceState::None;
		return;

	case PokerChoiceController::ActivateAction::ViewBoard:
		pokerQuickPreviewVisible_ = false;
		pokerReturnState_ = PokerChoiceState::WaitingActivateChoice;
		pokerChoiceState_ = PokerChoiceState::ViewingBoardFromPokerUi;

		handView_.SetPreviewIndex(-1);
		handView_.SetDrag(-1, 0.0f, 0.0f, false);
		handView_.SetFocusIndex(-1);
		handView_.SetHoverIndex(-1);

		fieldReplaceHoverIndex_ = -1;
		fieldLayoutDirty_ = true;
		return;

	case PokerChoiceController::ActivateAction::None:
	default:
		break;
	}

	if (tutorialActivateOnly_ && !hover.infoHovered) {
		pokerMouseChoice_ = PokerMouseChoice::ActivateYes;
	}
}

void BattleController::HandlePokerEffectChoice_(FieldUi& fieldUi, POINT mouse, bool lTrig, bool nTrig)
{
	if (pokerChoiceJustOpened_) {
		pokerChoiceJustOpened_ = false;
		return;
	}

	PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);
	const auto& layout = fieldUi.GetPokerEffectChoiceLayout();

	pokerMouseChoice_ = PokerMouseChoice::None;
	const auto decision = PokerChoiceController::ResolveEffectChoice(
		layout,
		mouse.x,
		mouse.y,
		lTrig,
		nTrig,
		tutorialDamageOnly_);
	const auto& hover = decision.hover;

	if (hover.infoHovered) {
		pokerMouseChoice_ = PokerMouseChoice::None;
	} else {
		pokerMouseChoice_ = ToPokerMouseChoice_(hover.choice);
	}

	switch (decision.action) {
	case PokerChoiceController::EffectAction::ToggleInfo:
		pokerQuickPreviewVisible_ = !pokerQuickPreviewVisible_;
		return;

	case PokerChoiceController::EffectAction::AtkUp:
		nextTurnAtkUp_ += bonus.atkUp;
		TriggerSubEffectsForField_(SubEffectTrigger::OnPokerSkillActivated, currentPoker_.rank);
		ConsumeFieldCards_();
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Activated;
		pokerChoiceState_ = PokerChoiceState::None;
		turn_ = TurnState::Enemy;
		enemyWait_ = 1.0f;
		return;

	case PokerChoiceController::EffectAction::Draw:
		DrawCards_(bonus.drawCount);
		TriggerSubEffectsForField_(SubEffectTrigger::OnPokerSkillActivated, currentPoker_.rank);
		ConsumeFieldCards_();
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Activated;
		pokerChoiceState_ = PokerChoiceState::None;
		turn_ = TurnState::Enemy;
		enemyWait_ = 1.0f;
		return;

	case PokerChoiceController::EffectAction::Damage:
		pendingDamage_ = CalcFinalAttackDamage_(bonus.damage);
		isPokerDamageTargeting_ = true;
		if (tutorialDamageOnly_) {
			tutorialLockPokerTargetingCancel_ = true;
		}
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::None;

		cardState_ = CardInputState::ChoosingEnemyTarget;
		pokerChoiceState_ = PokerChoiceState::None;
		return;

	case PokerChoiceController::EffectAction::Back:
		pokerQuickPreviewVisible_ = false;
		pokerChoiceState_ = PokerChoiceState::WaitingActivateChoice;
		return;

	case PokerChoiceController::EffectAction::ViewBoard:
		pokerQuickPreviewVisible_ = false;
		pokerReturnState_ = PokerChoiceState::WaitingEffectChoice;
		pokerChoiceState_ = PokerChoiceState::ViewingBoardFromPokerUi;

		handView_.SetPreviewIndex(-1);
		handView_.SetDrag(-1, 0.0f, 0.0f, false);
		handView_.SetFocusIndex(-1);
		handView_.SetHoverIndex(-1);

		fieldReplaceHoverIndex_ = -1;
		fieldLayoutDirty_ = true;
		return;

	case PokerChoiceController::EffectAction::None:
	default:
		break;
	}

	if (tutorialDamageOnly_) {
		return;
	}
}

void BattleController::HandlePokerViewBoard_(FieldUi& fieldUi, POINT mouse, bool lTrig, float dt)
{
	handView_.SetPreviewIndex(-1);
	handView_.SetDrag(-1, 0.0f, 0.0f, false);
	handView_.SetFocusIndex(-1);

	const auto& layout = fieldUi.GetPokerEffectChoiceLayout();
	pokerMouseChoice_ = ToPokerMouseChoice_(
		PokerChoiceController::ResolveViewBoardHover(layout, mouse.x, mouse.y));

	int hover = BattleCardInputController::PickHandIndexByMouse(handView_, cam_->GetViewProjectionMatrix(), mouse.x, mouse.y, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight);
	handView_.SetHoverIndex(hover);

	if (hover < 0) {
		int newHover = PickFieldIndexByMouse_(mouse.x, mouse.y);
		if (newHover != fieldReplaceHoverIndex_) {
			fieldReplaceHoverIndex_ = newHover;
			fieldLayoutDirty_ = true;
		}
	} else {
		if (fieldReplaceHoverIndex_ != -1) {
			fieldReplaceHoverIndex_ = -1;
			fieldLayoutDirty_ = true;
		}
	}

	handView_.Update(dt);
	RefreshAllFieldCardTransforms_(dt);
	fieldLayoutDirty_ = false;

	if (lTrig && pokerMouseChoice_ == PokerMouseChoice::ReturnFromBoard) {
		handView_.SetHoverIndex(-1);
		fieldReplaceHoverIndex_ = -1;
		fieldLayoutDirty_ = true;

		pokerChoiceState_ = pokerReturnState_;
		pokerReturnState_ = PokerChoiceState::None;
		pokerChoiceJustOpened_ = true;
		return;
	}
}

void BattleController::Draw3D(GameApp& app)
{
	DrawDamagePopups3D(app);
	DrawCardArea3D(app);
	DrawField3D(app);
	DrawBattleOverlay3D(app);
}

void BattleController::DrawDamagePopups3D(GameApp& app)
{
	(void)app;
	BattleRenderView::DrawDamagePopups3D(actionDirector_, damagePopupUi_);
}

void BattleController::DrawPlayerBattleStatusUI(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj)
{
	(void)app;
	BattleRenderView::DrawPlayerBattleStatusUI(
		playerStatusUi_,
		GetPlayerHpTexts(),
		GetPlayerBlockText(),
		GetPlayerPowerBoostText(),
		view,
		proj);
}

void BattleController::DrawEnemyBattleStatusHpTexts(const Matrix4x4& view, const Matrix4x4& proj)
{
	BattleRenderView::DrawEnemyBattleStatusHpTexts(enemyStatusUi_, view, proj);
}

void BattleController::DrawEnemyBattleStatusBcTexts(const Matrix4x4& view, const Matrix4x4& proj)
{
	BattleRenderView::DrawEnemyBattleStatusBcTexts(enemyStatusUi_, view, proj);
}

void BattleController::DrawCardArea3D(GameApp& app)
{
	(void)app;
	BattleRenderView::CardAreaContext context{};
	context.discardView = discardView_.get();
	context.handView = &handView_;
	context.damagePopupUi = &damagePopupUi_;
	context.choosingFieldReplace = cardState_ == CardInputState::ChoosingFieldReplace;
	context.highlightFilter = highlightFilter_.get();
	context.pendingCardView = pendingCardView_.get();
	context.fieldViews = &fieldViews_;
	BattleRenderView::DrawCardArea3D(context);
}

void BattleController::DrawField3D(GameApp& app)
{
	(void)app;
	BattleRenderView::DrawField3D(propManager_.get());
}

void BattleController::DrawBattleOverlay3D(GameApp& app)
{
	BloomParam targetParam = MakeEnemyTargetBloomParam_(app.ObjectPost()->GetParam(), sPokerGlowRainbowTime);
	BattleRenderView::BattleOverlayContext context{};
	context.app = &app;
	context.highlightFilter = highlightFilter_.get();
	context.enemyMgr = enemyMgr_;
	context.choosingEnemyTarget = cardState_ == CardInputState::ChoosingEnemyTarget;
	context.enemyTargetBloomEnabled = sEnemyTargetBloomEnabled;
	context.enemyTargetBloomParam = &targetParam;
	BattleRenderView::DrawBattleOverlay3D(context);
}

void BattleController::DrawPostEffect3D(GameApp& app)
{
	BattleRenderView::DrawPostEffect3D(handView_, app);
}

void BattleController::DrawFieldFrameBloom(GameApp& app)
{
	BattleRenderView::FieldFrameBloomContext context{};
	context.app = &app;
	context.fieldViews = &fieldViews_;
	context.handView = &handView_;
	context.fieldReplacePreviewActive = &fieldReplacePreviewActive_;
	context.handPreviewRanks = &handPreviewRanks_;
	context.pokerHighlightMask = GetPokerHighlightMask_();
	context.currentPokerRank = currentPoker_.rank;
	context.inReplacePreview = cardState_ == CardInputState::ChoosingFieldReplace && hasPendingCard_;
	context.enabled = sFieldFrameBloomEnabled;
	context.threshold = sFieldFrameBloomThreshold;
	context.intensity = sFieldFrameBloomIntensity;
	context.handIntensity = sHandFrameBloomIntensity;
	context.minPulse = sFieldFrameBloomMinPulse;
	context.chromAb = sFieldFrameBloomChromAb;
	context.time = sPokerGlowRainbowTime;
	BattleRenderView::DrawFieldFrameBloom(context);
}
void BattleController::DrawPreviewCard3D(GameApp& app) {
	app.ObjCom()->SetGraphicsPipelineState();
	if (cardState_ == CardInputState::Preview && pendingCardView_) {
		pendingCardView_->Draw();
	}
	handView_.DrawPreviewCard();
}

void BattleController::Draw2D(GameApp& app)
{
	BattleRenderView::Draw2DContext context{};
	context.app = &app;
	context.actionDirector = &actionDirector_;
	context.playerStatusUi = &playerStatusUi_;
	context.enemyStatusUi = &enemyStatusUi_;
	context.player = player_;
	context.enemyMgr = enemyMgr_;
	context.enemyActedFlags = &enemyActionCountSystem_.GetActedFlags();
	context.incomingDamage = std::max(0, CalcTotalIncomingDamage());
	context.time = sPokerGlowRainbowTime;
	context.hpDamageBlinkSpeed = sHpDamageBlinkSpeed;
	context.hpGaugeBloomEnabled = sHpGaugeBloomEnabled;
	context.hpGaugeBloomIntensity = sHpGaugeBloomIntensity;
	context.hpDamageBloomIntensity = sHpDamageBloomIntensity;
	context.enemyIntentBloomEnabled = sEnemyIntentBloomEnabled;
	context.enemyIntentBloomIntensity = sEnemyIntentBloomIntensity;
	context.enemyIntentBloomMinPulse = sEnemyIntentBloomMinPulse;
	BattleRenderView::Draw2D(context);
}

void BattleController::DrawHpGaugeBloom_(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj)
{
	BattleRenderView::Draw2DContext context{};
	context.app = &app;
	context.playerStatusUi = &playerStatusUi_;
	context.enemyStatusUi = &enemyStatusUi_;
	context.player = player_;
	context.enemyMgr = enemyMgr_;
	context.incomingDamage = std::max(0, CalcTotalIncomingDamage());
	context.time = sPokerGlowRainbowTime;
	context.hpDamageBlinkSpeed = sHpDamageBlinkSpeed;
	context.hpGaugeBloomEnabled = sHpGaugeBloomEnabled;
	context.hpGaugeBloomIntensity = sHpGaugeBloomIntensity;
	context.hpDamageBloomIntensity = sHpDamageBloomIntensity;
	BattleRenderView::DrawHpGaugeBloom(context, view, proj);
}
#ifdef USE_IMGUI
void BattleController::DrawPlayerHudImGuiControls()
{
	BattleDebugImGui::DrawPlayerHudControls(playerStatusUi_);
}

void BattleController::DrawImGui()
{
	const char* cardStateName = "";
	switch (cardState_) {
	case CardInputState::Idle: cardStateName = "Idle"; break;
	case CardInputState::Dragging: cardStateName = "Dragging"; break;
	case CardInputState::Preview: cardStateName = "Preview"; break;
	case CardInputState::ChoosingFieldReplace: cardStateName = "ChoosingFieldReplace"; break;
	}

	const PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);

	BattleDebugImGui::Context context{};
	context.playerStatusUi = &playerStatusUi_;
	context.turnName = turn_ == TurnState::Player ? "Player" : "Enemy";
	context.playerTurnCount = playerTurnCount_;
	context.enemyTurnCount = enemyTurnCount_;
	context.energy = energy_;
	context.energyMax = energyMax_;
	context.deckZone = &deckZone_;
	context.field = &field_;
	context.handView = &handView_;
	context.cardStateName = cardStateName;
	context.hasPendingCard = hasPendingCard_;
	context.pendingCard = pendingCard_;
	context.evaluatedPoker = EvaluatePokerHand_();
	context.currentPoker = currentPoker_;
	context.waitingActivateChoice = pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice;
	context.waitingEffectChoice = pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice;
	context.currentPokerBonus = { bonus.atkUp, bonus.drawCount, bonus.damage };
	context.player = player_;
	context.enemyMgr = enemyMgr_;
	context.propManager = propManager_.get();
	context.useDebugPreviewBuff = &useDebugPreviewBuff_;
	context.currentTurnAtkUp = &currentTurnAtkUp_;
	context.nextTurnAtkUp = &nextTurnAtkUp_;
	context.debugPreviewPowerBoost = &debugPreviewPowerBoost_;
	context.debugPreviewCurrentTurnAtkUp = &debugPreviewCurrentTurnAtkUp_;
	context.debugPreviewNextTurnAtkUp = &debugPreviewNextTurnAtkUp_;
	context.actionDirector = &actionDirector_;
	context.camera = cam_;

	context.fieldCardGlitter.localOffset = &sFieldCardGlitterLocalOffset;
	context.fieldCardGlitter.spreadX = &sFieldCardGlitterSpreadX;
	context.fieldCardGlitter.spreadY = &sFieldCardGlitterSpreadY;
	context.fieldCardGlitter.emitInterval = &sFieldCardGlitterEmitInterval;
	context.fieldCardGlitter.normalCount = &sFieldCardGlitterNormalCount;
	context.fieldCardGlitter.highlightCount = &sFieldCardGlitterHighlightCount;
	context.fieldCardGlitter.handPreviewEnabled = &sHandPokerPreviewEnabled;
	context.fieldCardGlitter.handEmitInterval = &sHandCardGlitterEmitInterval;
	context.fieldCardGlitter.handCount = &sHandCardGlitterCount;

	context.fieldFrameBloom.enabled = &sFieldFrameBloomEnabled;
	context.fieldFrameBloom.threshold = &sFieldFrameBloomThreshold;
	context.fieldFrameBloom.intensity = &sFieldFrameBloomIntensity;
	context.fieldFrameBloom.handIntensity = &sHandFrameBloomIntensity;
	context.fieldFrameBloom.minPulse = &sFieldFrameBloomMinPulse;
	context.fieldFrameBloom.chromAb = &sFieldFrameBloomChromAb;

	context.enemyBloom.intentEnabled = &sEnemyIntentBloomEnabled;
	context.enemyBloom.intentIntensity = &sEnemyIntentBloomIntensity;
	context.enemyBloom.intentMinPulse = &sEnemyIntentBloomMinPulse;
	context.enemyBloom.targetEnabled = &sEnemyTargetBloomEnabled;
	context.enemyBloom.targetIntensity = &sEnemyTargetBloomIntensity;
	context.enemyBloom.targetChromAb = &sEnemyTargetBloomChromAb;

	context.hpGaugeBloom.enabled = &sHpGaugeBloomEnabled;
	context.hpGaugeBloom.intensity = &sHpGaugeBloomIntensity;
	context.hpGaugeBloom.minPulse = &sHpGaugeBloomMinPulse;
	context.hpGaugeBloom.damageBlinkSpeed = &sHpDamageBlinkSpeed;
	context.hpGaugeBloom.damageIntensity = &sHpDamageBloomIntensity;
	context.frostAction.threshold = &sFrostBurstThreshold;
	context.frostAction.burstMultiplier = &sFrostBurstMultiplier;

	BattleDebugImGui::Draw(context);
}
#endif

int BattleController::CalcFinalAttackDamage_(int baseDamage) const
{
	int total = baseDamage;

	if (player_) {
		total += player_->GetBoostedPower();
	} else if (useDebugPreviewBuff_) {
		total += debugPreviewPowerBoost_;
	}

	if (player_) {
		total += currentTurnAtkUp_;
	} else if (useDebugPreviewBuff_) {
		total += debugPreviewCurrentTurnAtkUp_;
	} else {
		total += currentTurnAtkUp_;
	}

	if (total < 0) {
		total = 0;
	}

	return total;
}

int BattleController::GetDisplayEffectValue(const CardEffectDef& effect, bool applyAttackBuff) const
{
	if (!applyAttackBuff) {
		if (effect.type == "DamageCrescent") {
			int value = effect.value;
			if (playerTurnCount_ % 2 != 0) {
				value += 3;
			}
			return value;
		}
		if (effect.type == "DamageByBlock") {
			return ScaleEffectAmount_(player_ ? player_->GetBlock() : 0, effect);
		}
		return EffectValueInt_(effect);
	}

	if (effect.type == "Damage") {
		return CalcFinalAttackDamage_(EffectValueInt_(effect));
	}

	if (effect.type == "DamageAll") {
		return CalcFinalAttackDamage_(EffectValueInt_(effect));
	}

	if (effect.type == "DamageCrescent") {
		int value = effect.value;
		if (playerTurnCount_ % 2 != 0) {
			value += 3;
		}
		return CalcFinalAttackDamage_(value);
	}

	if (effect.type == "DamageByBlock") {
		int value = ScaleEffectAmount_(player_ ? player_->GetBlock() : 0, effect);
		return CalcFinalAttackDamage_(value);
	}

	return EffectValueInt_(effect);
}

int BattleController::ApplyDamageToEnemy_(Enemy& enemy, int damage)
{
	if (!player_) {
		return 0;
	}

	player_->PlayAttackAnimWithEffect(enemy.GetPos(), -1);
	enemy.TriggerHitFlash(0.2f);
	enemy.PlayDamageAnim();
	const int beforeBlock = enemy.GetBlock();
	const int actualDamage = enemy.Damage(damage);
	BattleSfxPlayer::PlayBlockReactionSE(beforeBlock, enemy.GetBlock(), actualDamage, damage);

	if (player_->GetVampireHeal() > 0) {
		int beforeHp = player_->GetHP();
		player_->Heal(player_->GetVampireHeal());
		BattleSfxPlayer::PlayHealSEIfHpIncreased(player_, beforeHp);
	}

	return actualDamage;
}

void BattleController::SetPlayer(Player* player) {
	player_ = player;
}

void BattleController::SetEnemyManager(EnemyManager* enemyMgr) {
	enemyMgr_ = enemyMgr;
}

void BattleController::SpawnDamagePopup(const Vector3& pos, int damage, bool isPlayer)
{
	damagePopupUi_.SpawnDamage(pos, damage, isPlayer);
}

const CardDef* BattleController::GetPreviewCardDef() const
{
	if (cardState_ == CardInputState::Dragging) {
		if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(deckZone_.GetHandCount())) {
			return db_.Find(deckZone_.GetHand()[selectedIndex_].defId);
		}
	}

	if (cardState_ == CardInputState::Preview) {
		if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(deckZone_.GetHandCount())) {
			return db_.Find(deckZone_.GetHand()[selectedIndex_].defId);
		}
	}

	if (cardState_ == CardInputState::Idle) {
		int handHover = handView_.GetHoverIndex();
		if (handHover >= 0 && handHover < static_cast<int>(deckZone_.GetHandCount())) {
			return db_.Find(deckZone_.GetHand()[handHover].defId);
		}
	}

	if (cardState_ == CardInputState::ChoosingFieldReplace) {
		if (fieldReplaceHoverIndex_ >= 0 &&
			fieldReplaceHoverIndex_ < static_cast<int>(field_.size())) {
			return db_.Find(field_[fieldReplaceHoverIndex_].defId);
		}
	}

	if (pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi) {
		int handHover = handView_.GetHoverIndex();
		if (handHover >= 0 && handHover < static_cast<int>(deckZone_.GetHandCount())) {
			return db_.Find(deckZone_.GetHand()[handHover].defId);
		}

		if (fieldReplaceHoverIndex_ >= 0 &&
			fieldReplaceHoverIndex_ < static_cast<int>(field_.size())) {
			return db_.Find(field_[fieldReplaceHoverIndex_].defId);
		}
	}

	return nullptr;
}

bool BattleController::HasPokerChoiceUi() const
{
	return PokerChoiceQuery::HasChoiceUi(pokerChoiceState_);
}

std::wstring BattleController::GetPokerChoiceUiText() const
{
	const PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);
	BattleInfoTextProvider::PokerChoiceState choiceState = BattleInfoTextProvider::PokerChoiceState::None;
	switch (pokerChoiceState_) {
	case PokerChoiceState::WaitingActivateChoice:
		choiceState = BattleInfoTextProvider::PokerChoiceState::WaitingActivateChoice;
		break;
	case PokerChoiceState::WaitingEffectChoice:
		choiceState = BattleInfoTextProvider::PokerChoiceState::WaitingEffectChoice;
		break;
	case PokerChoiceState::ViewingBoardFromPokerUi:
		choiceState = BattleInfoTextProvider::PokerChoiceState::ViewingBoard;
		break;
	case PokerChoiceState::None:
	default:
		break;
	}
	return BattleInfoTextProvider::BuildPokerChoiceUiText(
		choiceState,
		currentPoker_.rank,
		{ bonus.atkUp, bonus.drawCount, bonus.damage });
}

int BattleController::GetPokerMouseChoiceIndex() const
{
	return PokerChoiceQuery::GetMouseChoiceIndex(pokerMouseChoice_);
}

bool BattleController::IsWaitingActivateChoice() const
{
	return PokerChoiceQuery::IsWaitingActivate(pokerChoiceState_);
}

bool BattleController::IsWaitingEffectChoice() const
{
	return PokerChoiceQuery::IsWaitingEffect(pokerChoiceState_);
}

bool BattleController::IsViewingBoardFromPokerUi() const
{
	return PokerChoiceQuery::IsViewingBoard(pokerChoiceState_);
}

bool BattleController::ShouldShowOperationUi() const
{
	return operationUiVisible_;
}

std::wstring BattleController::GetOperationUiText() const
{
	BattleInfoTextProvider::OperationState operationState = BattleInfoTextProvider::OperationState::Basic;
	if (cardState_ == CardInputState::ChoosingFieldReplace) {
		operationState = BattleInfoTextProvider::OperationState::ChoosingFieldReplace;
	} else if (cardState_ == CardInputState::Preview) {
		operationState = BattleInfoTextProvider::OperationState::Preview;
	}

	return BattleInfoTextProvider::BuildOperationText(operationState);
}

std::wstring BattleController::GetZoneCountUiText() const
{
	return BattleInfoTextProvider::BuildZoneCountText({
		static_cast<int>(deckZone_.GetDeckCount()),
		static_cast<int>(deckZone_.GetHandCount()),
		static_cast<int>(deckZone_.GetDiscardCount()),
		static_cast<int>(field_.size()) });
}

std::wstring BattleController::GetCurrentPokerHandUiText() const
{
	PokerHandResult poker = EvaluatePokerHand_();
	return BattleInfoTextProvider::BuildCurrentPokerHandText(poker.rank, field_.size());
}

std::wstring BattleController::GetTurnUiText() const
{
	return BattleInfoTextProvider::BuildTurnText({
		turn_ == TurnState::Player,
		playerTurnCount_,
		enemyTurnCount_ });
}

std::wstring BattleController::GetEnergyText() const
{
	return BattleInfoTextProvider::BuildEnergyText(energy_, energyMax_);
}

std::vector<std::wstring> BattleController::GetEnemyHpTexts() const
{
	return BattleInfoTextProvider::BuildEnemyHpTexts(enemyMgr_);
}

std::vector<std::wstring> BattleController::GetEnemyBCTexts() const
{
	return BattleInfoTextProvider::BuildEnemyBcTexts(enemyMgr_);
}

BattleController::PokerBonus BattleController::GetCurrentPokerBonusForUi() const
{
	return GetPokerBonus_(currentPoker_.rank);
}


std::wstring BattleController::GetPlayerHpTexts() const
{
	return BattleInfoTextProvider::BuildPlayerHpText(*player_);
}

bool BattleController::IsAllEnemiesDead() const {
	auto& enemies = enemyMgr_->GetEnemies();
	for (auto& e : enemies) {
		if (e.IsAlive()) return false; // 鬮｣蛹・ｽｽ・ｳ繝ｻ縺､ﾂ鬮｣雋ｻ・｣・ｰ郢晢ｽｻ繝ｻ・ｺ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬩幢ｽ｢繝ｻ・ｧ驛｢・ｧ霑壼生繝ｻ鬩搾ｽｵ繝ｻ・ｺ鬯ｮ・ｦ繝ｻ・ｪ驕ｯ・ｶ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ髫ｨ・ｳ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ髫ｴ謫ｾ・｣・ｰalse
	}
	return true; // 鬮ｯ・ｷ髣鯉ｽｨ繝ｻ・ｽ繝ｻ・ｨ鬮ｯ・ｷ繝ｻ・ｩ郢晢ｽｻ繝ｻ・｡鬮ｮ蠑ｱ繝ｻ繝ｻ・ｽ繝ｻ・ｻ鬩幢ｽ｢繝ｻ・ｧ鬮ｦ・ｮ陷ｷ・ｶ・つ陜｣・､繝ｻ・ｸ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ髫ｨ・ｳ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ髮趣ｽｸ繝ｻ・ｲrue
}

std::wstring BattleController::GetPlayerPowerBoostText() const
{
	return BattleInfoTextProvider::BuildPlayerPowerBoostText(*player_);
}

std::wstring BattleController::GetPlayerBlockText() const
{
	return BattleInfoTextProvider::BuildPlayerBlockText(*player_);
}
int BattleController::CalcTotalIncomingDamage() const {
	int total = 0;
	if (!enemyMgr_ || !player_) return 0;

	auto& enemies = enemyMgr_->GetEnemies();
	for (size_t i = 0; i < enemies.size(); ++i) {
		const auto& enemy = enemies[i];
		if (!enemy.IsAlive() || enemyActionCountSystem_.IsActedByCount(i)) {
			continue;
		}
		total += enemy.GetIncomingDamage() - player_->GetBlock();
	}
	return total;
}

void BattleController::UpdateHpGauges() {}

//=====================
//鬩幢ｽ｢隴擾ｽｶ郢晢ｽｻ繝ｻ螳茨ｽ､・ｼ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴主・讓溘・蜿悶渚繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・｢鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｫ鬯ｨ・ｾ陋ｹ繝ｻ・ｽ・ｽ繝ｻ・ｨ
//=====================
void BattleController::SetTutorialOpeningHand(const std::vector<CardInstance>& cards)
{
	tutorialOpeningHand_ = cards;
	useTutorialOpeningHand_ = !cards.empty();
}

void BattleController::SetTutorialPokerRestriction(bool activateOnly, bool damageOnly) {
	tutorialActivateOnly_ = activateOnly;
	tutorialDamageOnly_ = damageOnly;
}

void BattleController::SetTutorialForcedEnemyTargetCardId(int cardDefId)
{
	const int newId = cardDefId > 0 ? cardDefId : -1;
	if (tutorialForcedEnemyTargetCardId_ == newId) {
		return;
	}

	tutorialForcedEnemyTargetCardId_ = newId;
	handPreviewSignature_ = 0;
	if (!IsTutorialForcedCardActive_()) {
		handView_.ClearCardEffects();
	}
}

std::vector<std::string> BattleController::CollectEffectTypes_(const CardDef& def) const
{
	std::vector<std::string> types;
	types.reserve(def.effects.size());

	for (const auto& effect : def.effects) {
		if (!effect.type.empty()) {
			types.push_back(effect.type);
		}
	}

	return types;
}

bool BattleController::BeginCardActionSequence_(GameApp& app, const CardDef& def, const CardInstance& card, Enemy& targetEnemy)
{
	actionSequenceQueue_.clear();
	actionSequenceIndex_ = 0;
	actionSequenceTarget_ = &targetEnemy;
	actionSequenceCardDef_ = &def;
	actionSequenceCard_ = card;
	actionSequenceDamageApplied_ = false;

	if (const ActionSequenceProfile* useProfile = app.PickCardUseSequenceProfile()) {
		actionSequenceQueue_.push_back(useProfile);
	}

	return StartNextActionSequence_();
}

bool BattleController::StartNextActionSequence_()
{
	if (!actionSequenceTarget_ || actionSequenceIndex_ >= actionSequenceQueue_.size()) {
		actionSequenceQueue_.clear();
		actionSequenceIndex_ = 0;
		actionSequenceCardDef_ = nullptr;
		actionSequenceDamageApplied_ = false;
		return false;
	}

	const ActionSequenceProfile* profile = actionSequenceQueue_[actionSequenceIndex_++];
	if (!profile) {
		return StartNextActionSequence_();
	}

	actionDirector_.SetProfile(*profile);
	actionSequenceDamageApplied_ = false;
	if (actionSequenceCardDef_) {
		actionDirector_.StartAction(player_, actionSequenceTarget_, *actionSequenceCardDef_, actionSequenceCard_);
	} else {
		actionDirector_.StartAction(player_, actionSequenceTarget_);
	}
	return true;
}

bool BattleController::ApplyFrostBeforeEnemyAction_(Enemy& enemy)
{
	if (!enemy.IsAlive() ||
		enemy.GetBC() != Enemy::BadCondition::kFrost ||
		enemy.GetBCPoint() <= 0) {
		return false;
	}

	const int frostPoint = enemy.GetBCPoint();
	const bool burst = frostPoint >= std::max(1, sFrostBurstThreshold);
	const int damage = burst
		? frostPoint * std::max(1, sFrostBurstMultiplier)
		: frostPoint;
	const int actualDamage = enemy.Damage(damage);

	enemy.TriggerHitFlash(0.2f);
	enemy.PlayDamageAnim();
	if (actualDamage > 0) {
		SpawnDamagePopup(enemy.GetPos(), actualDamage, false);
	}

	if (burst) {
		enemy.RemoveBC();
		return true;
	}

	return !enemy.IsAlive();
}

void BattleController::ExecuteEnemyAction_(Enemy& enemy, const EnemyAction& action)
{
	if (ApplyFrostBeforeEnemyAction_(enemy)) {
		return;
	}

	auto findLowestHpAlly = [&]() -> Enemy* {
		if (!enemyMgr_) {
			return nullptr;
		}

		Enemy* target = nullptr;
		for (auto& ally : enemyMgr_->GetEnemies()) {
			if (!ally.IsAlive()) {
				continue;
			}
			if (!target || ally.GetHP() < target->GetHP()) {
				target = &ally;
			}
		}
		return target;
	};

	if (action.type == "Attack") {
		if (!player_) {
			return;
		}
		enemy.PlayAttackAnim(player_->GetPos());
		const int beforeHp = player_->GetHP();
		const int beforeBlock = player_->GetBlock();
		player_->Damage(action.value);
		const int actualDamage = std::max(0, beforeHp - player_->GetHP());
		const int afterBlock = player_->GetBlock();
		BattleSfxPlayer::PlayBlockReactionSE(beforeBlock, afterBlock, actualDamage, action.value);
	} else if (action.type == "Heal") {
		const int beforeHp = enemy.GetHP();
		enemy.Heal(action.value);
		if (enemy.GetHP() > beforeHp) {
			BattleSfxPlayer::PlaySE("SE_Heal");
		}
	} else if (action.type == "HealLowestAlly") {
		Enemy* target = findLowestHpAlly();
		if (!target) {
			return;
		}
		const int beforeHp = target->GetHP();
		target->Heal(action.value);
		if (target->GetHP() > beforeHp) {
			BattleSfxPlayer::PlaySE("SE_Heal");
		}
	} else if (action.type == "HealAll") {
		if (!enemyMgr_) {
			return;
		}
		bool healedAny = false;
		for (auto& ally : enemyMgr_->GetEnemies()) {
			if (!ally.IsAlive()) {
				continue;
			}
			const int beforeHp = ally.GetHP();
			ally.Heal(action.value);
			healedAny = healedAny || ally.GetHP() > beforeHp;
		}
		if (healedAny) {
			BattleSfxPlayer::PlaySE("SE_Heal");
		}
	} else if (action.type == "Block") {
		const int beforeBlock = enemy.GetBlock();
		enemy.AddBlock(action.value);
		BattleSfxPlayer::PlayBlockGainSEIfIncreased(beforeBlock, enemy.GetBlock());
	} else if (action.type == "BlockLowestAlly") {
		Enemy* target = findLowestHpAlly();
		if (!target) {
			return;
		}
		const int beforeBlock = target->GetBlock();
		target->AddBlock(action.value);
		BattleSfxPlayer::PlayBlockGainSEIfIncreased(beforeBlock, target->GetBlock());
	} else if (action.type == "BlockAll") {
		if (!enemyMgr_) {
			return;
		}
		bool blockedAny = false;
		for (auto& ally : enemyMgr_->GetEnemies()) {
			if (!ally.IsAlive()) {
				continue;
			}
			const int beforeBlock = ally.GetBlock();
			ally.AddBlock(action.value);
			blockedAny = blockedAny || ally.GetBlock() > beforeBlock;
		}
		if (blockedAny) {
			BattleSfxPlayer::PlaySE("SE_Block");
		}
	}
}

void BattleController::OnPlayerCardUsed_()
{
	if (!enemyMgr_) {
		return;
	}

	auto& enemies = enemyMgr_->GetEnemies();
	const auto triggeredEnemies = enemyActionCountSystem_.OnPlayerCardUsed(enemies);
	for (size_t index : triggeredEnemies) {
		if (index >= enemies.size() || !enemies[index].IsAlive()) {
			continue;
		}
		ExecuteEnemyAction_(enemies[index], enemies[index].GetBossAI().GetNextAction());
		enemyActionCountSystem_.MarkActedByCount(index);
	}
}

void BattleController::ExecutePendingAttack_(Enemy& targetEnemy)
{
	if (isPokerDamageTargeting_) {
		BattleSfxPlayer::PlaySE("SE_StrongAttack");
		const int actualDamage = ApplyDamageToEnemy_(targetEnemy, pendingDamage_);
		if (pendingDamage_ > 0) {
			SpawnDamagePopup(targetEnemy.GetPos(), actualDamage, false);
		}
		TriggerSubEffectsForField_(SubEffectTrigger::OnPokerSkillActivated, currentPoker_.rank);

		lastPokerTutorialResult_ = PokerTutorialResult::Activated;

		isPokerDamageTargeting_ = false;
		tutorialLockPokerTargetingCancel_ = false;
		pendingDamage_ = 0;

		ConsumeFieldCards_();
		cardState_ = CardInputState::Idle;
		turn_ = TurnState::Enemy;
		enemyTurnCount_++;
		enemyWait_ = 1.0f;
	} else {
		// 鬮ｫ・ｰ郢晢ｽｻ驍・・・ｫ・ｰ郢晢ｽｻ繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ驍ｵ・ｲ陜｣・､繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮ｫ・ｰ繝ｻ・ｾ郢晢ｽｻ繝ｻ・ｻ鬮ｫ・ｰ繝ｻ・ｦ驛｢譎｢・ｽ・ｻ髫ｨ繝ｻ・ｽ・｡鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・｣鬩搾ｽｵ繝ｻ・ｺ髮九・・ｽ・ｷ郢晢ｽｻ繝ｻ・ｰ郢晢ｽｻ繝ｻ・ｴ鬮ｯ・ｷ繝ｻ・ｷ驛｢譎｢・ｽ・ｻ
		int idx = pendingCardHandIndex_;
		CardInstance inst = deckZone_.GetHand()[idx];
		const CardDef* def = db_.Find(inst.defId);

		// 鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｳ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩幢ｽ｢隴主・讓溽ｹ晢ｽｻ陞ｳ螟ｲ・ｽ・ｱ繝ｻ・ｸ鬯ｩ蟶吶・繝ｻ・ｽ繝ｻ・ｲ郢晢ｽｻ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ髯ｷ莨夲ｽｽ・ｱ驕ｯ・ｶ繝ｻ・ｻ鬮ｫ・ｰ郢晢ｽｻ驍・・・ｫ・ｰ郢晢ｽｻ繝ｻ・ｸ繝ｻ・ｺ髣包ｽｵ隴趣ｽ｢繝ｻ・ｽ髣・ｽｽ繝ｻ・ｱ繝ｻ・ｸ髯具ｽｹ繝ｻ・ｻ髫ｨ蛟･繝ｻ
		energy_ -= def->cost;
		BattleSfxPlayer::PlaySE("SE_CardPlay");
		BattleSfxPlayer::PlayAttackSEForCard(*def);
		auto usedCardView = handView_.ExtractCardAt(idx);
		deckZone_.RemoveHandAt(static_cast<std::size_t>(idx));

		handView_.Rebuild(deckZone_.GetHand());

		// 鬩幢ｽ｢隰ｨ魑ｴﾂ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｸ鬮｣豈費ｽｼ螟ｲ・ｽ・ｽ繝ｻ・･鬮ｯ讓奇ｽｻ阮卍ｧ驛｢譎｢・ｽ・ｻ鬮ｯ・ｷ闔ｨ螟ｲ・ｽ・ｽ繝ｻ・ｹ鬮ｫ・ｴ繝ｻ・ｫ髫ｲ蟷｢・ｽ・ｶ郢晢ｽｻ繝ｻ・ｼ髯具ｽｹ繝ｻ・ｻ驛｢譎｢・ｽ・ｩ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｭ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｪ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｩ驛｢譎｢・ｽ・ｻ髯晢ｽｲ繝ｻ・ｨ郢晢ｽｻ陝ｶ謨鳴陷茨ｽｷ繝ｻ・ｽ繝ｻ・ｺ鬮ｯ・ｷ鬮ｦ・ｪ郢晢ｽｻ
		ApplyCardEffects_(*def, currentEnemyIndex_);

		handView_.SetFocusIndex(-1);

		// 鬮ｯ諛ｶ・ｽ・｣郢晢ｽｻ繝ｻ・ｴ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬮ｯ・ｷ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｺ鬩搾ｽｵ繝ｻ・ｺ髯ｷ・ｷ隴擾ｽｴ郢晢ｽｻ鬯ｨ・ｾ郢晢ｽｻ郢晢ｽｻ
		if ((int)field_.size() < 5) {
			field_.push_back(inst);
			if (usedCardView) {
				usedCardView->SetIsHand(false);
				fieldViews_.push_back(std::move(usedCardView));
			}
			RebuildFieldView_();
			if ((int)field_.size() == 5) {
				PokerHandResult poker = EvaluatePokerHand_();
				TriggerSubEffectsForCard_(inst, SubEffectTrigger::OnPlayToField, poker.rank);
			}
			cardState_ = CardInputState::Idle;
			hasPendingCard_ = false;
			pendingCard_ = {};
		} else {
			pendingCard_ = inst;
			hasPendingCard_ = true;
			pendingCardView_ = std::move(usedCardView);
			cardState_ = CardInputState::ChoosingFieldReplace;
		}
		OnPlayerCardUsed_();
	}
}
