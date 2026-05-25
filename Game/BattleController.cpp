#include "BattleController.h"
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

#include"Player.h"
#include"Enemy.h"

#include "FieldUi.h"
#include "AudioManager.h"
#include "ModelParticleManager.h"

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

	Vector4 GetPokerFrameColor_(BattleController::PokerHandRank rank, float time)
	{
		switch (rank) {
		case BattleController::PokerHandRank::OnePair:
		case BattleController::PokerHandRank::TwoPair:
			return { 1.0f, 0.85f, 0.20f, 1.0f };

		case BattleController::PokerHandRank::ThreeOfAKind:
		case BattleController::PokerHandRank::Straight:
		case BattleController::PokerHandRank::Flush:
			return { 0.25f, 0.95f, 0.35f, 1.0f };

		case BattleController::PokerHandRank::FullHouse:
			return { 0.25f, 0.60f, 1.0f, 1.0f };

		case BattleController::PokerHandRank::FourOfAKind:
		case BattleController::PokerHandRank::StraightFlush:
			return { 1.0f, 0.25f, 0.20f, 1.0f };

		case BattleController::PokerHandRank::RoyalStraightFlush:
			return HsvToRgb_(time * 120.0f, 0.9f, 1.0f);

		case BattleController::PokerHandRank::None:
		default:
			return { 1.0f, 1.0f, 1.0f, 1.0f };
		}
	}

	Vector4 LerpColor_(const Vector4& a, const Vector4& b, float t)
	{
		t = std::clamp(t, 0.0f, 1.0f);
		return {
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t,
			a.w + (b.w - a.w) * t
		};
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

	int RollEnemyActionCount_()
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dist(3, 6);
		return dist(gen);
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

	if (discard_.empty()) {
		return;
	}

	const CardInstance& top = discard_.back();
	const CardDef* def = db_.Find(top.defId);
	if (!def) {
		return;
	}

	discardView_ = std::make_unique<Card3D>();
	discardView_->Initialize(objCom_, dx_, cam_, *def, top);

	// 鬮｣蜴・ｽｽ・ｴ髯ｷ・･繝ｻ・ｲ郢晢ｽｻ繝ｻ・ｽ郢晢ｽｻ繝ｻ・ｮ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｯ鬮ｯ・ｷ繝ｻ・ｿ郢晢ｽｻ繝ｻ・ｳ鬮｣蛹・ｽｽ・ｳ髯ｷ・ｿ繝ｻ・･郢晢ｽｻ繝ｻ・ｯ驛｢譎｢・ｽ・ｻ郢晢ｽｻ鬯倩ｲｻ・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｸ鬩搾ｽｵ繝ｻ・ｲ驛｢・ｧ郢晢ｽｻ隴鯉ｽｺ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｨ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬯ｮ・ｫ繝ｻ・ｱ郢晢ｽｻ繝ｻ・ｿ鬮ｫ・ｰ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｴ
	Vector3 pos{ 16.0f, -8.0f, 6.0f };
	Vector3 rot{ 0.0f, 0.0f, 0.0f };
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

	for (auto& c : field_) {
		discard_.push_back(c);
	}
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

	void PlaySE_(const char* soundId)
	{
		AudioManager::GetInstance()->PlaySE(soundId);
	}

	const char* GetAttackSeIdForCard_(const CardDef& def)
	{
		const std::string& name = def.name;
		if (name == "Fireball") {
			return "SE_Fireball";
		}
		if (name == "Attack!" || name == "BloodyVengeance") {
			return "SE_NormalAttack";
		}
		if (name == "Power Shot" ||
			name == "Crush" ||
			name == "CrescentMoon" ||
			name == "OverClock" ||
			name == "ShieldBash" ||
			name == "GaeBolg" ||
			name == "Durandal") {
			return "SE_StrongAttack";
		}

		bool hasDamage = false;
		for (const auto& effect : def.effects) {
			if (effect.type == "DamageAll" ||
				effect.type == "DamageCrescent" ||
				effect.type == "DamageByBlock") {
				return "SE_StrongAttack";
			}
			if (effect.type == "Damage") {
				hasDamage = true;
			}
		}

		return hasDamage ? "SE_NormalAttack" : nullptr;
	}

	void PlayAttackSEForCard_(const CardDef& def)
	{
		if (const char* soundId = GetAttackSeIdForCard_(def)) {
			PlaySE_(soundId);
		}
	}

	void PlayHealSEIfHpIncreased_(Player* player, int beforeHp)
	{
		if (player && player->GetHP() > beforeHp) {
			PlaySE_("SE_Heal");
		}
	}

	void PlayBlockGainSEIfIncreased_(int beforeBlock, int afterBlock)
	{
		if (afterBlock > beforeBlock) {
			PlaySE_("SE_Block");
		}
	}

	void PlayBlockReactionSE_(int beforeBlock, int afterBlock, int actualHpDamage, int attemptedDamage)
	{
		if (attemptedDamage <= 0 || beforeBlock <= 0) {
			return;
		}

		if (afterBlock <= 0) {
			PlaySE_("SE_BlockBreak");
			return;
		}

		if (actualHpDamage <= 0) {
			PlaySE_("SE_ShieldGuard");
		}
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
	static std::random_device rd;
	static std::mt19937 mt(rd());
	std::shuffle(deck_.begin(), deck_.end(), mt);
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
		std::vector<std::string> cardFiles = {
	  "resources/cards/data/UtilityAttack.json",
	  "resources/cards/data/UtilitySupport.json",
	  "resources/cards/data/Poison.json",
	  "resources/cards/data/Frost.json"
		};

		db_.LoadFromJsons(cardFiles);
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
	deck_ = prebuiltDeck_;
	ShuffleDeck_();

	if (useTutorialOpeningHand_) {
		// 鬮ｫ・ｴ陝・｢・つ鬮ｯ蜈ｷ・ｽ・ｻ髫ｴ謫ｾ・ｽ・ｴ驕ｶ鬆托ｽ･・｢繝ｻ・ｰ鬯倬ｯ会ｽｽ・ｼ髮具ｽｻ繝ｻ・ｿ繝ｻ・･5鬮ｫ・ｴ繝ｻ・ｫ髯橸ｽ｢繝ｻ・ｹ郢晢ｽｻ郢晢ｽｻdeck 鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮ｫ・ｴ陝ｷ・｢繝ｻ・ｽ繝ｻ・ｫ鬮ｯ譏ｴ繝ｻ繝ｻ・ｽ繝ｻ・ｾ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬯ｩ蛹・ｽｽ・ｨ鬯ｮ・ｦ繝ｻ・ｪ驛｢譎｢・ｽ・ｻ鬩搾ｽｵ繝ｻ・ｺ髮九・竏槭・・ｽ遶擾ｽｫ繝ｻ・ｸ繝ｻ・ｲ驛｢譎｢・ｽ・ｻ
		// 鬮ｯ・ｷ闔・･霑ｴ・ｾ驕ｶ鬆托ｽ･・｢隲・ｺ髯溷供ﾂ蜈ｷ・ｽ・ｧ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ驕ｯ・ｶ繝ｻ・ｲ鬩搾ｽｵ繝ｻ・ｺ驛｢・ｧ郢晢ｽｻ繝ｻ・ｽ隶呵ｶ｣・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬯ｮ・ｴ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｽ鬩搾ｽｵ繝ｻ・ｺ髣包ｽｳ隶抵ｽｫ陟募ｮ｣ﾎ斐・・ｧ髣費ｽｨ遶乗刋・ｱ繧九＠繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
		for (const auto& fixedCard : tutorialOpeningHand_) {
			auto it = std::find_if(deck_.begin(), deck_.end(),
				[&](const CardInstance& c) {
					return c.defId == fixedCard.defId;
				});
			if (it != deck_.end()) {
				deck_.erase(it);
			}
		}

		// DrawOne_ 鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｯ back() 鬩幢ｽ｢繝ｻ・ｧ鬮ｮ蛹ｺ・ｩ・ｸ繝ｻ・ｽ繝ｻ・ｼ鬮ｴ驛・ｽｲ・ｻ繝ｻ・ｿ繝ｻ・･鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬩搾ｽｵ繝ｻ・ｲ驛｢譎｢・ｽ・ｻ繝ｻ縺､ﾂ驛｢譎｢・ｽ・ｻ郢晢ｽｻ繝ｻ・ｰ驛｢譎｢・ｽ・ｻ驍ｵ・ｲ陝ｶ譏ｶ繝ｻ鬯ｮ・ｦ繝ｻ・ｪ驛｢譎｢・ｽ・ｻ
		for (auto it = tutorialOpeningHand_.rbegin(); it != tutorialOpeningHand_.rend(); ++it) {
			deck_.push_back(*it);
		}
	}

	hand_.clear();
	discard_.clear();
	field_.clear();
	fieldViews_.clear();
	damagePopupUi_.Clear();

	hasPendingCard_ = false;
	pendingCard_ = {};
	currentEnemyIndex_ = 0;
	nextTurnAtkUp_ = 0;
	currentTurnAtkUp_ = 0;

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
	if (deck_.empty()) {
		if (discard_.empty()) return false;
		deck_ = discard_;
		discard_.clear();
		ShuffleDeck_();
		RebuildDiscardView_();
	}

	if (deck_.empty()) return false;

	CardInstance card = deck_.back();
	deck_.pop_back();
	hand_.push_back(card);

	// 鬩搾ｽｵ繝ｻ・ｺ鬮ｦ・ｮ陷ｻ・ｻ繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ髯橸ｽｳ陞滂ｽｲ繝ｻ・ｽ繝ｻ・ｿ郢晢ｽｻ繝ｻ・ｽ鬮ｯ・ｷ闔ｨ螟ｲ・ｽ・｣繝ｻ・ｰ
	handView_.AddCard(card);

	return true;
}

void BattleController::DrawUntilFive_()
{
	while ((int)hand_.size() < 5) {
		if (!DrawOne_()) break;
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

void BattleController::ApplyEffectsList_(const std::vector<CardEffectDef>& effects, int targetIndex, bool applyAttackBuff)
{
	for (const auto& effect : effects) {
		if (effect.type == "Draw") {
			DrawCards_(effect.value);

		} else if (effect.type == "Damage") {
			if (enemyMgr_) {
				if (targetIndex >= 0 && targetIndex < enemyMgr_->GetEnemies().size()) {
					auto& e = enemyMgr_->GetEnemies()[targetIndex];
					if (e.IsAlive()) {
						int totalDamage = applyAttackBuff ? CalcFinalAttackDamage_(effect.value) : std::max(0, effect.value);

						if (e.GetBC() == Enemy::BadCondition::kFrost) {
							totalDamage += e.GetBCPoint();
						}

						if (player_->GetFrostBiteActive()) {
							e.SetBC(Enemy::BadCondition::kFrost);
							e.AddBC(1);
						}

						const int actualDamage = ApplyDamageToEnemy_(e, totalDamage);
						if (totalDamage > 0) SpawnDamagePopup(e.GetPos(), actualDamage, false);
					}
				} else
				{
					for (auto& e : enemyMgr_->GetEnemies()) {
						if (!e.IsAlive()) continue;
						int totalDamage = applyAttackBuff ? CalcFinalAttackDamage_(effect.value) : std::max(0, effect.value);
						const int actualDamage = ApplyDamageToEnemy_(e, totalDamage);
						if (player_->GetFrostBiteActive()) {
							e.SetBC(Enemy::BadCondition::kFrost);
							e.AddBC(1);
						}
						if (totalDamage > 0) SpawnDamagePopup(e.GetPos(), actualDamage, false);
						break;
					}
				}

				/*	if (applyAttackBuff) {
						nextTurnAtkUp_ = 0;
					}*/
			}

		} else if (effect.type == "DamageCrescent") {
			if (enemyMgr_) {
				int baseVal = effect.value;
				if (playerTurnCount_ % 2 != 0) {
					baseVal += 3;
				}

				int finalDamage = applyAttackBuff ? CalcFinalAttackDamage_(baseVal) : std::max(0, baseVal);

				if (targetIndex >= 0 && targetIndex < enemyMgr_->GetEnemies().size()) {
					auto& e = enemyMgr_->GetEnemies()[targetIndex];
					if (e.IsAlive()) {
						const int actualDamage = ApplyDamageToEnemy_(e, finalDamage);
						if (finalDamage > 0) SpawnDamagePopup(e.GetPos(), actualDamage, false);
						if (player_->GetFrostBiteActive()) {
							e.SetBC(Enemy::BadCondition::kFrost);
							e.AddBC(1);
						}
					}
				} else {
					for (auto& e : enemyMgr_->GetEnemies()) {
						if (!e.IsAlive()) continue;
						const int actualDamage = ApplyDamageToEnemy_(e, finalDamage);
						if (finalDamage > 0) SpawnDamagePopup(e.GetPos(), actualDamage, false);
						if (player_->GetFrostBiteActive()) {
							e.SetBC(Enemy::BadCondition::kFrost);
							e.AddBC(1);
						}
						break;
					}
				}

				/*	if (applyAttackBuff) {
						nextTurnAtkUp_ = 0;
					}*/
			}

		} else if (effect.type == "DamageByBlock") {
			if (enemyMgr_) {
				int baseVal = (player_ ? player_->GetBlock() : 0) * effect.value;
				int finalDamage = applyAttackBuff ? CalcFinalAttackDamage_(baseVal) : std::max(0, baseVal);

				if (targetIndex >= 0 && targetIndex < enemyMgr_->GetEnemies().size()) {
					auto& e = enemyMgr_->GetEnemies()[targetIndex];
					if (e.IsAlive()) {
						const int actualDamage = ApplyDamageToEnemy_(e, finalDamage);
						if (finalDamage > 0) SpawnDamagePopup(e.GetPos(), actualDamage, false);
						if (player_->GetFrostBiteActive()) {
							e.SetBC(Enemy::BadCondition::kFrost);
							e.AddBC(1);
						}
					}
				} else {
					for (auto& e : enemyMgr_->GetEnemies()) {
						if (!e.IsAlive()) continue;
						const int actualDamage = ApplyDamageToEnemy_(e, finalDamage);
						if (finalDamage > 0) SpawnDamagePopup(e.GetPos(), actualDamage, false);
						if (player_->GetFrostBiteActive()) {
							e.SetBC(Enemy::BadCondition::kFrost);
							e.AddBC(1);
						}
						break;
					}
				}

				/*	if (applyAttackBuff) {
						nextTurnAtkUp_ = 0;
					}*/
			}

		} else if (effect.type == "Block") {
			if (player_ && effect.value > 0) {
				const int beforeBlock = player_->GetBlock();
				player_->AddBlock(effect.value);
				PlayBlockGainSEIfIncreased_(beforeBlock, player_->GetBlock());
			}

		} else if (effect.type == "DamageAll") {
			if (enemyMgr_ && player_) {
				int totalDamage = applyAttackBuff ? CalcFinalAttackDamage_(effect.value) : std::max(0, effect.value);

				player_->PlayAttackAnimWithEffect(player_->GetPos(), -1);

				int hitCount = 0;
				for (auto& e : enemyMgr_->GetEnemies()) {
					if (e.IsAlive()) {
						e.TriggerHitFlash(0.2f);
						e.PlayDamageAnim();
						const int beforeBlock = e.GetBlock();
						const int actualDamage = e.Damage(totalDamage);
						PlayBlockReactionSE_(beforeBlock, e.GetBlock(), actualDamage, totalDamage);
						if (totalDamage > 0) {
							SpawnDamagePopup(e.GetPos(), actualDamage, false);
						}
						if (player_->GetFrostBiteActive()) {
							e.SetBC(Enemy::BadCondition::kFrost);
							e.AddBC(1);
						}
						hitCount++;
					}
				}

				if (hitCount > 0 && player_->GetVampireHeal() > 0) {
					int beforeHp = player_->GetHP();
					player_->Heal(player_->GetVampireHeal() * hitCount);
					PlayHealSEIfHpIncreased_(player_, beforeHp);
				}

				//if (applyAttackBuff) {
				//	nextTurnAtkUp_ = 0;
				//}
			}

		} else if (effect.type == "PowerBoost") {
			if (player_ && effect.value > 0) {
				const int beforePower = player_->GetBoostedPower();
				player_->PowerBoost(effect.value);
				if (player_->GetBoostedPower() > beforePower) {
					PlaySE_("SE_PowerCharge");
				}
			}
		} else if (effect.type == "NextTurnAtkUp") {
			if (effect.value > 0) {
				PlaySE_("SE_PowerCharge");
			}
			nextTurnAtkUp_ += effect.value;

		} else if (effect.type == "Heal") {
			if (player_) {
				int beforeHp = player_->GetHP();
				player_->Heal(effect.value);
				PlayHealSEIfHpIncreased_(player_, beforeHp);
			}

		} else if (effect.type == "HealByBlock") {
			if (player_) {
				int healAmount = player_->GetBlock() * effect.value; // 鬩幢ｽ｢隴弱・ﾂｧ繝ｻ蜿厄ｽｺ・ｽ繝ｻ・ｹ隴擾ｽｴ郢晢ｽｻ驍ｵ・ｺ鬯倬ｯ会ｽｽ・ｬ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｰ 郢晢ｽｻ郢晢ｽｻ郢晢ｽｻ鬮ｯ蛹ｺ・ｺ・ｷ髫ｱ・ｿ鬩肴得・ｽ・ｫ
				int beforeHp = player_->GetHP();
				player_->Heal(healAmount);
				PlayHealSEIfHpIncreased_(player_, beforeHp);
			}
		} else if (effect.type == "HealByLowCostInHand") {
			if (player_) {
				int count = 0;
				// 鬮｣遒代・繝ｻ・ｿ繝ｻ・ｫ驛｢譎｢・ｽ・ｻ鬮ｫ・ｰ郢晢ｽｻ驍・・・ｫ・ｰ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ驛｢譎｢・ｽ・ｻ鬮ｫ・ｴ繝ｻ・ｫ髯橸ｽ｢繝ｻ・ｹ髫ｨ蛟･繝ｻ繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・､鬯ｩ蠅捺・繝ｻ・ｽ繝ｻ・ｺ鬯ｮ・ｫ繝ｻ・ｱ鬯ｮ・ｦ繝ｻ・ｪ髫ｨ蛟･繝ｻ繝ｻ・ｹ繝ｻ・ｧ驛｢譎｢・ｽ・ｻ
				for (const auto& cardInst : hand_) {
					const CardDef* cDef = db_.Find(cardInst.defId);
					if (cDef && cDef->cost == 1) {
						count++; // 1鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｳ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩幢ｽ｢隴主・讓滄Δ譎｢・ｽ・ｻ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ髫ｨ繝ｻ・ｽ・｡鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・｣鬩搾ｽｵ繝ｻ・ｺ髮九・竏槭・・ｽ髢ｾ・･繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｦ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩幢ｽ｢隴主・讓溽ｹ晢ｽｻ陞ｳ螟ｲ・｣・ｰ郢晢ｽｻ陝ｷ・ｲ郢晢ｽｻ郢晢ｽｻ繝ｻ・ｸ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
					}
				}
				int healAmount = count * effect.value;
				if (healAmount > 0) {
					int beforeHp = player_->GetHP();
					player_->Heal(healAmount);
					PlayHealSEIfHpIncreased_(player_, beforeHp);
				}
			}
		} else if (effect.type == "VampireBuff") {
			if (player_) {
				player_->AddVampireHeal(effect.value);
			}
		} else if (effect.type == "SelfDamage") {
			if (player_) {
				player_->TriggerHitFlash(0.2f);
				player_->PlayDamageAnim();
				player_->Damage(effect.value);
			}

		} else if (effect.type == "Poison") {
			if (enemyMgr_) {
				// 鬮ｯ・ｷ鬮ｮ繝ｻﾂ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｽ鬮ｦ・ｮ陷ｷ・ｶ隨・｣ｰ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｲ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驛｢譎｢・ｽ・ｨ鬩搾ｽｵ繝ｻ・ｺ髫ｴ・ｴ繝ｻ・ｧ髫ｹ・ｺ繝ｻ・ｰ鬮ｯ讖ｸ・ｽ・ｳ髯橸ｽ｢繝ｻ・ｹ郢晢ｽｻ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ髯溷供・ｨ・ｯ・つ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ驍・戟謐礼ｹ晢ｽｻ繝ｻ・ｴ鬮ｯ・ｷ繝ｻ・ｷ驛｢譎｢・ｽ・ｻ
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
				DrawCards_(1); // 鬩幢ｽ｢隴弱・・ｺ・｢驍ｵ・ｺ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｺ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ繝ｻ蜿厄ｽｺ・ｽ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬮ｴ謇假ｽｽ・･郢晢ｽｻ繝ｻ・ｶ鬮ｫ・ｲ繝ｻ・ｷ髣包ｽｵ隴擾ｽｶ郢晢ｽｻ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ鬮ｫ・ｴ繝ｻ・ｫ髯橸ｽ｢繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｼ鬮ｴ驛・ｽｲ・ｻ繝ｻ・ｿ繝ｻ・･
			}
		} else if (effect.type == "PoisonAll") {
			if (enemyMgr_) {
				for (auto& e : enemyMgr_->GetEnemies()) {
					if (e.IsAlive()) {
						e.SetBC(Enemy::BadCondition::kPoison);
						e.AddBC(effect.value);
					}
				}
			}
			if (player_->GetPoisonDrawActive()) {
				DrawCards_(1); // 鬩幢ｽ｢隴弱・・ｺ・｢驍ｵ・ｺ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｺ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ繝ｻ蜿厄ｽｺ・ｽ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬮ｴ謇假ｽｽ・･郢晢ｽｻ繝ｻ・ｶ鬮ｫ・ｲ繝ｻ・ｷ髣包ｽｵ隴擾ｽｶ郢晢ｽｻ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ鬮ｫ・ｴ繝ｻ・ｫ髯橸ｽ｢繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｼ鬮ｴ驛・ｽｲ・ｻ繝ｻ・ｿ繝ｻ・･
			}
		} else if (effect.type == "PoisonAmplify") {
			if (enemyMgr_) {
				for (auto& e : enemyMgr_->GetEnemies()) {
					if (e.IsAlive()) {
						if (e.GetBC() == Enemy::BadCondition::kPoison) {
							e.AmplifyBC(effect.value);
						}
					}
				}
			}
		} else if (effect.type == "PoisonDamage") {
			if (enemyMgr_) {
				for (auto& e : enemyMgr_->GetEnemies()) {
					if (e.IsAlive()) {
						e.SetBC(Enemy::BadCondition::kPoison);
						e.DamageBC(effect.value);
					}
				}
			}
		} else if (effect.type == "PoisonDraw") {

			bool isActivated = player_->GetPoisonDrawActive();

			if (!isActivated) {
				player_->SetPoisonDrawActive(true);
			} else {
				if (enemyMgr_) {
					for (auto& e : enemyMgr_->GetEnemies()) {
						if (e.IsAlive()) {
							e.SetBC(Enemy::BadCondition::kPoison);
							e.AddBC(effect.value);
						}
					}
				}
				if (player_->GetPoisonDrawActive()) {
					DrawCards_(1); // 鬩幢ｽ｢隴弱・・ｺ・｢驍ｵ・ｺ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｺ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩幢ｽ｢隴取得・ｽ・ｳ繝ｻ・ｨ繝ｻ蜿厄ｽｺ・ｽ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬮ｴ謇假ｽｽ・･郢晢ｽｻ繝ｻ・ｶ鬮ｫ・ｲ繝ｻ・ｷ髣包ｽｵ隴擾ｽｶ郢晢ｽｻ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ鬮ｫ・ｴ繝ｻ・ｫ髯橸ｽ｢繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｼ鬮ｴ驛・ｽｲ・ｻ繝ｻ・ｿ繝ｻ・･
				}
			}

		} else if (effect.type == "PoisonRemove") {

			if (enemyMgr_) {
				for (auto& e : enemyMgr_->GetEnemies()) {
					if (e.IsAlive()) {
						e.RemoveBC();
					}
				}
			}

		} else if (effect.type == "PoisonHeal") {

			int healAmount = 0;

			if (enemyMgr_) {
				for (auto& e : enemyMgr_->GetEnemies()) {
					// 鬮ｮ諠ｹ・ｺ・･繝ｻ・ｰ繝ｻ・､髫ｲ讖ｸ・ｽ・ｾ鬮ｫ・ｲ繝ｻ・ｷ髣包ｽｵ隴擾ｽｶ郢晢ｽｻ鬩幢ｽ｢繝ｻ・ｧ髯晢ｽｲ繝ｻ・ｨ髫ｨ・ｳ陞ｳ闌ｨ・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬮ｯ蜈ｷ・ｽ・ｻ驛｢譎｢・ｽ・ｻ髯橸ｽｻ隶鯉ｽ｢繝ｻ・ｰ陋ｹ繝ｻ・ｽ・ｽ繝ｻ・ｩ
					if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kPoison) {
						e.SetBC(Enemy::BadCondition::kPoison);
						healAmount += e.GetBCPoint();
					}
				}
			}

			int beforeHp = player_->GetHP();
			player_->Heal(healAmount);
			PlayHealSEIfHpIncreased_(player_, beforeHp);

		} else if (effect.type == "Frost") {
			if (enemyMgr_) {
				// 鬮ｯ・ｷ鬮ｮ繝ｻﾂ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｽ鬮ｦ・ｮ陷ｷ・ｶ隨・｣ｰ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｲ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驛｢譎｢・ｽ・ｨ鬩搾ｽｵ繝ｻ・ｺ髫ｴ・ｴ繝ｻ・ｧ髫ｹ・ｺ繝ｻ・ｰ鬮ｯ讖ｸ・ｽ・ｳ髯橸ｽ｢繝ｻ・ｹ郢晢ｽｻ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ髯溷供・ｨ・ｯ・つ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ驍・戟謐礼ｹ晢ｽｻ繝ｻ・ｴ鬮ｯ・ｷ繝ｻ・ｷ驛｢譎｢・ｽ・ｻ
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

		} else if (effect.type == "FrostAll") {
			if (enemyMgr_) {
				for (auto& e : enemyMgr_->GetEnemies()) {
					if (e.IsAlive()) {
						e.SetBC(Enemy::BadCondition::kFrost);
						e.AddBC(effect.value);
					}
				}
			}

		} else if (effect.type == "FrostBlock") {
			int count = 0;

			if (enemyMgr_) {
				for (auto& e : enemyMgr_->GetEnemies()) {
					if (e.IsAlive()&&e.GetBC() == Enemy::BadCondition::kFrost) {
						count += e.GetBCPoint();
					}
				}
			}

		} else if (effect.type == "FrostDamage") {
			if (enemyMgr_) {
				for (auto& e : enemyMgr_->GetEnemies()) {
					if (e.IsAlive()) {
						e.DamageBC(effect.value);
					}
				}
			}

		} else if (effect.type == "FrostSubtract") {
			if (enemyMgr_) {
				for (auto& e : enemyMgr_->GetEnemies()) {
					if (e.IsAlive()) {
						e.SubtractBC(effect.value);
					}
				}
			}

		} else if (effect.type == "FrostBite") {
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
				//if (player_->GetFrostBiteActive()) {
				//	DrawCards_(1); // 鬩幢ｽ｢隴弱・・ｽ・ｼ鬩･繝ｻ・ｺ・ｽ繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩幢ｽ｢隴主・讓滄Δ譎｢・ｽ・ｰ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬩幢ｽ｢隴寂・・・坿讖ｸ・ｽ・ｾ鬮ｫ・ｲ繝ｻ・ｷ髣包ｽｵ隴擾ｽｶ郢晢ｽｻ鬩幢ｽ｢繝ｻ・ｧ驛｢譎｢・ｽ・ｻ鬮ｫ・ｴ繝ｻ・ｫ髯橸ｽ｢繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｼ鬮ｴ驛・ｽｲ・ｻ繝ｻ・ｿ繝ｻ・･
				//}
			}

		} else if (effect.type == "FrostAmplify") {
			if (enemyMgr_) {
				for (auto& e : enemyMgr_->GetEnemies()) {
					if (e.IsAlive()) {
						if (e.GetBC() == Enemy::BadCondition::kFrost) {
							e.AmplifyBC(effect.value);
						}
					}
				}
			}

		} else if (effect.type == "FrostDraw") {
			
			if (enemyMgr_) {
				// 鬮ｯ・ｷ鬮ｮ繝ｻﾂ郢晢ｽｻ繝ｻ・ｽ繝ｻ・ｽ鬮ｦ・ｮ陷ｷ・ｶ隨・｣ｰ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｲ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驛｢譎｢・ｽ・ｨ鬩搾ｽｵ繝ｻ・ｺ髫ｴ・ｴ繝ｻ・ｧ髫ｹ・ｺ繝ｻ・ｰ鬮ｯ讖ｸ・ｽ・ｳ髯橸ｽ｢繝ｻ・ｹ郢晢ｽｻ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ髯溷供・ｨ・ｯ・つ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ驍・戟謐礼ｹ晢ｽｻ繝ｻ・ｴ鬮ｯ・ｷ繝ｻ・ｷ驛｢譎｢・ｽ・ｻ
				if (targetIndex >= 0 && targetIndex < enemyMgr_->GetEnemies().size()) {
					auto& e = enemyMgr_->GetEnemies()[targetIndex];
					if (e.IsAlive()&& e.GetBC() == Enemy::BadCondition::kFrost) {
						DrawCards_(e.GetBCPoint()/2);
					}
				} else {
					for (auto& e : enemyMgr_->GetEnemies()) {
						if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kFrost) {
							DrawCards_(e.GetBCPoint()/2);
							break;
						}
					}
				}
			}

		} else if (effect.type == "ChangeNumber") {
			// 鬮ｯ貅ｷ萓帙・・ｾ陟募仰陞ｳ螢ｽ・ｱ讒ｭ繝ｻ繝ｻ・ｾ鬯ｮ・ｮ髮懶ｽ｣繝ｻ・ｽ繝ｻ・｡鬮ｫ・ｰ隰費ｽｶ郢晢ｽｻ郢晢ｽｻ繝ｻ・ｮ髯橸ｽ｢繝ｻ・ｹ驕ｯ・ｶ繝ｻ・ｲ鬮ｯ貊ゑｽｽ・｢驛｢譎｢・ｽ・ｻ郢晢ｽｻ繝ｻ・ｦ驛｢譎｢・ｽ・ｻ

		} else if (effect.type == "ChangeSuit") {
			// 鬮ｯ貅ｷ萓帙・・ｾ陟募仰陞ｳ螢ｽ・ｱ讒ｭ繝ｻ繝ｻ・ｾ鬯ｮ・ｮ髮懶ｽ｣繝ｻ・ｽ繝ｻ・｡鬮ｫ・ｰ隰費ｽｶ郢晢ｽｻ郢晢ｽｻ繝ｻ・ｮ髯橸ｽ｢繝ｻ・ｹ驕ｯ・ｶ繝ｻ・ｲ鬮ｯ貊ゑｽｽ・｢驛｢譎｢・ｽ・ｻ郢晢ｽｻ繝ｻ・ｦ驛｢譎｢・ｽ・ｻ
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
		player_->ResetBlock();
		player_->ResetVampireHeal();
		player_->ResetPowerBoost();
	}
	playerTurnCount_++;

	energy_ = energyMax_;
	DrawUntilFive_();

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
			PlaySE_("SE_Pop");
		}
	}
	if (enemyMgr_) {
		auto& enemies = enemyMgr_->GetEnemies();
		enemyActionCounts_.assign(enemies.size(), 0);
		enemyActedByCountThisTurn_.assign(enemies.size(), false);
		for (size_t i = 0; i < enemies.size(); ++i) {
			if (enemies[i].IsAlive()) {
				enemies[i].GetBossAI().DecideNextAction();
				enemyActionCounts_[i] = RollEnemyActionCount_();
			}
		}
	} else {
		enemyActionCounts_.clear();
		enemyActedByCountThisTurn_.clear();
	}
}

BattleController::PokerBonus BattleController::GetPokerBonus_(PokerHandRank rank) const
{
	PokerBonus b{};

	switch (rank) {
	case PokerHandRank::OnePair:
		b.atkUp = 10;
		b.drawCount = 2;
		b.damage = 15;
		break;

	case PokerHandRank::TwoPair:
		b.atkUp = 15;
		b.drawCount = 3;
		b.damage = 25;
		break;

	case PokerHandRank::ThreeOfAKind:
		b.atkUp = 20;
		b.drawCount = 3;
		b.damage = 35;
		break;

	case PokerHandRank::Straight:
		b.atkUp = 25;
		b.drawCount = 4;
		b.damage = 45;
		break;

	case PokerHandRank::Flush:
		b.atkUp = 30;
		b.drawCount = 4;
		b.damage = 55;
		break;

	case PokerHandRank::FullHouse:
		b.atkUp = 35;
		b.drawCount = 5;
		b.damage = 70;
		break;

	case PokerHandRank::FourOfAKind:
		b.atkUp = 40;
		b.drawCount = 5;
		b.damage = 85;
		break;

	case PokerHandRank::StraightFlush:
		b.atkUp = 50;
		b.drawCount = 6;
		b.damage = 110;
		break;

	case PokerHandRank::RoyalStraightFlush:
		b.atkUp = 70;
		b.drawCount = 7;
		b.damage = 150;
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
	switch (trigger) {
	case SubEffectTrigger::OnTurnStartWithPoker:
		return L"ターン開始時";

	case SubEffectTrigger::OnPokerSkillActivated:
		return L"特殊効果発動時";

	case SubEffectTrigger::OnPlayToField:
		return L"場に出した時";

	default:
		return L"";
	}
}

std::wstring BattleController::GetSubEffectConditionText_(const CardSubEffectDef& sub) const
{
	auto rankToText = [](PokerHandRank rank) -> std::wstring {
		switch (rank) {
		case PokerHandRank::OnePair:            return L"ワンペア";
		case PokerHandRank::TwoPair:            return L"ツーペア";
		case PokerHandRank::ThreeOfAKind:       return L"スリーカード";
		case PokerHandRank::Straight:           return L"ストレート";
		case PokerHandRank::Flush:              return L"フラッシュ";
		case PokerHandRank::FullHouse:          return L"フルハウス";
		case PokerHandRank::FourOfAKind:        return L"フォーカード";
		case PokerHandRank::StraightFlush:      return L"ストレートフラッシュ";
		case PokerHandRank::RoyalStraightFlush: return L"ロイヤルストレートフラッシュ";
		default:                                return L"";
		}
		};

	switch (sub.condition.type) {
	case SubEffectConditionType::ExactRank:
	{
		PokerHandRank rank = ParsePokerRankString_(sub.condition.rank);
		if (rank != PokerHandRank::None) {
			return rankToText(rank) + L"の場合";
		}
		break;
	}

	case SubEffectConditionType::AtLeastRank:
	{
		PokerHandRank rank = ParsePokerRankString_(sub.condition.rank);
		if (rank != PokerHandRank::None) {
			return rankToText(rank) + L"以上の場合";
		}
		break;
	}

	case SubEffectConditionType::RankFamily:
		if (sub.condition.family == "StraightFamily") return L"ストレート系の場合";
		if (sub.condition.family == "FlushFamily")    return L"フラッシュ系の場合";
		if (sub.condition.family == "PairFamily")     return L"ペア系の場合";
		return L"役条件あり";

	default:
		break;
	}

	return L"";
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
	if (!def.effects.empty()) {
		std::wstring text;

		for (size_t i = 0; i < def.effects.size(); ++i) {
			if (i > 0) {
				text += L"\n";
			}
			text += GetEffectValueText_(def.effects[i]);
		}

		return text;
	}

	if (!def.desc.empty()) {
		return Utf8ToWString(def.desc);
	}

	return L"なし";
}

std::wstring BattleController::GetPreviewCardDetailText() const
{
	const CardDef* def = GetPreviewCardDef();
	if (!def) {
		return L"";
	}

	std::wstring text = L"基本効果:\n";
	if (!def->desc.empty()) {
		text += Utf8ToWString(def->desc);
	} else {
		text += L"なし";
	}

	if (!def->subEffects.empty()) {
		text += L"\n\n";
		for (size_t i = 0; i < def->subEffects.size(); ++i) {
			const auto& sub = def->subEffects[i];
			if (i > 0) {
				text += L"\n\n";
			}

			std::wstring triggerText = GetSubEffectTriggerText_(sub.trigger);
			if (!triggerText.empty()) {
				text += triggerText + L"\n";
			}

			std::wstring conditionText = GetSubEffectConditionText_(sub);
			if (!conditionText.empty()) {
				text += conditionText + L"\n";
			}

			for (size_t j = 0; j < sub.effects.size(); ++j) {
				if (j > 0) text += L"\n";
				text += GetEffectValueText_(sub.effects[j]);
			}
		}
	}

	return text;
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
					line += L"カードを" + std::to_wstring(effect.value) + L"枚引く";
				} else if (effect.type == "Damage") {
					line += L"敵単体に" + std::to_wstring(effect.value) + L"ダメージ";
				} else if (effect.type == "DamageAll") {
					line += L"敵全体に" + std::to_wstring(effect.value) + L"ダメージ";
				} else if (effect.type == "Heal") {
					line += L"体力を" + std::to_wstring(effect.value) + L"回復";
				} else if (effect.type == "Block") {
					line += L"ブロックを" + std::to_wstring(effect.value) + L"獲得";
				} else if (effect.type == "PowerBoost") {
					line += L"パワーを" + std::to_wstring(effect.value) + L"獲得";
				} else if (effect.type == "EnergyCharge") {
					line += L"コストを" + std::to_wstring(effect.value) + L"回復";
				} else {
					line += Utf8ToWString(effect.type) + L" : " + std::to_wstring(effect.value);
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
	std::wstring text;

	PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);

	text += L"選択効果:\n";
	text += L"・このあと1つ選びます\n";
	text += L"  1. 次ターンATK UP +" + std::to_wstring(bonus.atkUp) + L"\n";
	text += L"  2. " + std::to_wstring(bonus.drawCount) + L"枚ドロー\n";
	text += L"  3. 敵単体に" + std::to_wstring(bonus.damage) + L"ダメージ\n";
	text += L"\n";

	auto turnStartLines = CollectSubEffectPreviewLines_(
		SubEffectTrigger::OnTurnStartWithPoker,
		currentPoker_.rank
	);

	text += L"ターン開始時:\n";
	if (turnStartLines.empty()) {
		text += L"・なし\n";
	} else {
		for (const auto& line : turnStartLines) {
			text += line + L"\n";
		}
	}
	text += L"\n";

	auto pokerActivatedLines = CollectSubEffectPreviewLines_(
		SubEffectTrigger::OnPokerSkillActivated,
		currentPoker_.rank
	);

	text += L"特殊効果発動時:\n";
	if (pokerActivatedLines.empty()) {
		text += L"・なし\n";
	} else {
		for (const auto& line : pokerActivatedLines) {
			text += line + L"\n";
		}
	}

	return text;
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
	if (index < 0 || index >= (int)fieldViews_.size()) {
		return;
	}

	const int fieldCount = (int)fieldViews_.size();
	if (fieldCount <= 0) {
		return;
	}

	const float y = fieldCardLayout_.y;
	const float z = fieldCardLayout_.z;
	const float gap = fieldCardLayout_.gap;
	const float startX = -gap * 0.5f * (fieldCount - 1);

	Vector3 pos{ startX + gap * index, y, z };
	Vector3 rot{ 0.0f, 0.0f, 0.0f };
	Vector3 scl{ fieldCardLayout_.scale, fieldCardLayout_.scale, fieldCardLayout_.scale };

	if (hovered) {

		pos.y += fieldCardLayout_.hoverYOffset;
		pos.z += fieldCardLayout_.hoverZOffset;
		scl = { fieldCardLayout_.hoverScale, fieldCardLayout_.hoverScale, fieldCardLayout_.hoverScale };

	}

	fieldViews_[index]->SetTargetTransform(pos, rot, scl, false);

	Vector3 curPos = fieldViews_[index]->GetWorldPos();
	float distSq = (curPos.x - pos.x) * (curPos.x - pos.x) +
		(curPos.y - pos.y) * (curPos.y - pos.y) +
		(curPos.z - pos.z) * (curPos.z - pos.z);
	if (distSq > 0.00001f) {
		fieldLayoutDirty_ = true;
	}
}

Vector4 GetPokerTransitionColor_(
	BattleController::PokerHandRank beforeRank,
	BattleController::PokerHandRank afterRank,
	float time)
{
	const Vector4 beforeColor = GetPokerFrameColor_(beforeRank, time);
	const Vector4 afterColor = GetPokerFrameColor_(afterRank, time);
	const float pulse = 0.5f + 0.5f * std::sin(time * 1.6f);
	return LerpColor_(beforeColor, afterColor, pulse);
}

float GetPokerGlitterIntensity_(BattleController::PokerHandRank rank)
{
	if (rank == BattleController::PokerHandRank::None) {
		return 0.0f;
	}
	if (rank <= BattleController::PokerHandRank::TwoPair) {
		return 5.0f;
	}
	if (rank <= BattleController::PokerHandRank::FullHouse) {
		return 10.0f;
	}
	return 15.0f;
}

void BattleController::RefreshAllFieldCardTransforms_(float dt)
{
	for (int i = 0; i < (int)fieldViews_.size(); ++i) {
		const bool hovered =
			(cardState_ == CardInputState::ChoosingFieldReplace && i == fieldReplaceHoverIndex_) ||
			(pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi && i == fieldReplaceHoverIndex_);

		UpdateFieldCardTransform_(i, hovered, dt);
	}

	if (field_.size() == 5) {
		currentPoker_ = EvaluatePokerHand_();
	} else {
		currentPoker_.rank = PokerHandRank::None;
		currentPoker_.power = 0;
	}

	UpdateFieldReplacePreviewEffects_();

	const std::array<bool, 5> highlightMask = GetPokerHighlightMask_();
	const Vector4 frameColor = GetPokerFrameColor_(currentPoker_.rank, sPokerGlowRainbowTime);
	const bool inReplacePreview = cardState_ == CardInputState::ChoosingFieldReplace && hasPendingCard_;
	for (int i = 0; i < (int)fieldViews_.size(); ++i) {
		if (!fieldViews_[i]) {
			continue;
		}

		PokerHandRank replacePreviewRank = PokerHandRank::None;
		if (i < static_cast<int>(fieldReplacePreviewRanks_.size())) {
			replacePreviewRank = fieldReplacePreviewRanks_[i];
		}
		const bool replacePreviewActive =
			i < static_cast<int>(fieldReplacePreviewActive_.size()) &&
			fieldReplacePreviewActive_[i];

		if (replacePreviewActive) {
			fieldViews_[i]->SetFrameColor(GetPokerTransitionColor_(currentPoker_.rank, replacePreviewRank, sPokerGlowRainbowTime));
			const float previewIntensity = std::max(
				GetPokerGlitterIntensity_(currentPoker_.rank),
				GetPokerGlitterIntensity_(replacePreviewRank));
			fieldViews_[i]->SetGlitter(previewIntensity > 0.0f ? previewIntensity : 4.0f);
		} else if (!inReplacePreview && i < 5 && highlightMask[i] && currentPoker_.rank != PokerHandRank::None) {
			fieldViews_[i]->SetFrameColor(frameColor);
			fieldViews_[i]->SetGlitter(GetPokerGlitterIntensity_(currentPoker_.rank));
		} else {
			fieldViews_[i]->ResetFrameColor();
			fieldViews_[i]->SetGlitter(0.0f);
		}
	}
}

void BattleController::UpdateFieldReplacePreviewEffects_()
{
	fieldReplacePreviewRanks_.assign(fieldViews_.size(), PokerHandRank::None);
	fieldReplacePreviewActive_.assign(fieldViews_.size(), false);

	if (cardState_ != CardInputState::ChoosingFieldReplace ||
		!hasPendingCard_ ||
		field_.size() != 5 ||
		fieldViews_.size() < 5) {
		return;
	}

	PokerHandRank bestRank = PokerHandRank::None;
	std::array<PokerHandRank, 5> ranks{};
	ranks.fill(PokerHandRank::None);

	for (int replaceIndex = 0; replaceIndex < 5; ++replaceIndex) {
		std::vector<CardInstance> candidate = field_;
		candidate[replaceIndex] = pendingCard_;
		const PokerHandRank rank = EvaluatePokerHandForCards_(candidate).rank;
		ranks[replaceIndex] = rank;
		if (rank > bestRank) {
			bestRank = rank;
		}
	}

	for (int replaceIndex = 0; replaceIndex < 5; ++replaceIndex) {
		fieldReplacePreviewRanks_[replaceIndex] = ranks[replaceIndex];
	}

	if (fieldReplaceHoverIndex_ >= 0 && fieldReplaceHoverIndex_ < 5) {
		fieldReplacePreviewActive_[fieldReplaceHoverIndex_] = true;
		return;
	}

	for (int replaceIndex = 0; replaceIndex < 5; ++replaceIndex) {
		if (ranks[replaceIndex] == bestRank && bestRank != PokerHandRank::None) {
			fieldReplacePreviewActive_[replaceIndex] = true;
		}
	}
}

void BattleController::EmitFieldCardGlitter_(float dt)
{
	if (fieldViews_.empty()) {
		fieldCardGlitterEmitTimer_ = 0.0f;
		return;
	}

	fieldCardGlitterEmitTimer_ += dt;
	if (fieldCardGlitterEmitTimer_ < sFieldCardGlitterEmitInterval) {
		return;
	}
	fieldCardGlitterEmitTimer_ = 0.0f;

	const bool hasPoker = currentPoker_.rank != PokerHandRank::None;
	const std::array<bool, 5> highlightMask = GetPokerHighlightMask_();
	const Vector4 pokerColor = GetPokerFrameColor_(currentPoker_.rank, sPokerGlowRainbowTime);
	const bool useReplacePreview = cardState_ == CardInputState::ChoosingFieldReplace && !fieldReplacePreviewRanks_.empty();
	ModelParticleManager* particles = fieldParticleManager_;
	if (!particles) {
		return;
	}

	for (int i = 0; i < (int)fieldViews_.size(); ++i) {
		if (!fieldViews_[i]) {
			continue;
		}

		PokerHandRank replacePreviewRank = PokerHandRank::None;
		if (i < static_cast<int>(fieldReplacePreviewRanks_.size())) {
			replacePreviewRank = fieldReplacePreviewRanks_[i];
		}
		const bool replacePreviewActive =
			i < static_cast<int>(fieldReplacePreviewActive_.size()) &&
			fieldReplacePreviewActive_[i];

		const bool highlighted = replacePreviewActive || (hasPoker && i < 5 && highlightMask[i]);
		if (useReplacePreview && !replacePreviewActive) {
			continue;
		}

		const uint32_t emitCount = static_cast<uint32_t>(std::max(0, highlighted ? sFieldCardGlitterHighlightCount : sFieldCardGlitterNormalCount));
		const Vector4 highlightColor = replacePreviewActive
			? GetPokerTransitionColor_(currentPoker_.rank, replacePreviewRank, sPokerGlowRainbowTime)
			: pokerColor;

		for (uint32_t emitIndex = 0; emitIndex < emitCount; ++emitIndex) {
			const Vector3 localOffset = {
				sFieldCardGlitterLocalOffset.x + Rand(-sFieldCardGlitterSpreadX, sFieldCardGlitterSpreadX),
				sFieldCardGlitterLocalOffset.y + Rand(-sFieldCardGlitterSpreadY, sFieldCardGlitterSpreadY),
				sFieldCardGlitterLocalOffset.z
			};
			Vector3 pos = fieldViews_[i]->GetWorldPointFromLocal(localOffset);
			const Vector4 color = highlighted ? highlightColor : Vector4{ 1.0f, 1.0f, 1.0f, 0.55f };
			particles->Emit("card_glitter", pos, 1u, color);
		}
	}
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

	mix(static_cast<uint64_t>(hand_.size()));
	for (const auto& card : hand_) {
		mix(static_cast<uint64_t>(card.defId));
		mix(static_cast<uint64_t>(card.number));
		mix(static_cast<uint64_t>(card.suit));
	}

	mix(static_cast<uint64_t>(handView_.GetCardCount()));
	return hash;
}

void BattleController::UpdateHandPokerPreviewEffects_()
{
	const uint64_t signature = BuildHandPokerPreviewSignature_();
	if (signature == handPreviewSignature_) {
		return;
	}
	handPreviewSignature_ = signature;

	handPreviewRanks_.assign(hand_.size(), PokerHandRank::None);

	if (!sHandPokerPreviewEnabled || hand_.empty()) {
		handView_.ClearCardEffects();
		return;
	}

	bool anyPreview = false;
	const int handCount = std::min<int>(static_cast<int>(hand_.size()), handView_.GetCardCount());

	for (int handIndex = 0; handIndex < handCount; ++handIndex) {
		PokerHandRank bestRank = PokerHandRank::None;

		if (field_.size() < 5) {
			std::vector<CardInstance> candidate = field_;
			candidate.push_back(hand_[handIndex]);
			if (candidate.size() == 5) {
				bestRank = EvaluatePokerHandForCards_(candidate).rank;
			}
		} else if (field_.size() == 5) {
			for (int replaceIndex = 0; replaceIndex < 5; ++replaceIndex) {
				std::vector<CardInstance> candidate = field_;
				candidate[replaceIndex] = hand_[handIndex];
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
			const Vector4 color = GetPokerFrameColor_(bestRank, sPokerGlowRainbowTime);
			handView_.SetCardEffect(handIndex, color, GetPokerGlitterIntensity_(bestRank));
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

		const Vector4 color = GetPokerFrameColor_(rank, sPokerGlowRainbowTime);
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
	const Matrix4x4& vp = cam_->GetViewProjectionMatrix();
	const float sw = (float)WinApp::kClientWidth;
	const float sh = (float)WinApp::kClientHeight;

	int best = -1;
	float bestD2 = 80.0f * 80.0f;

	for (int i = 0; i < (int)fieldViews_.size(); ++i) {
		Vector3 w = fieldViews_[i]->GetWorldPos();

		Vector4 clip{};
		clip.x = w.x * vp.m[0][0] + w.y * vp.m[1][0] + w.z * vp.m[2][0] + 1.0f * vp.m[3][0];
		clip.y = w.x * vp.m[0][1] + w.y * vp.m[1][1] + w.z * vp.m[2][1] + 1.0f * vp.m[3][1];
		clip.z = w.x * vp.m[0][2] + w.y * vp.m[1][2] + w.z * vp.m[2][2] + 1.0f * vp.m[3][2];
		clip.w = w.x * vp.m[0][3] + w.y * vp.m[1][3] + w.z * vp.m[2][3] + 1.0f * vp.m[3][3];

		if (clip.w <= 0.0f) {
			continue;
		}

		const float ndcX = clip.x / clip.w;
		const float ndcY = clip.y / clip.w;

		const float sx = (ndcX * 0.5f + 0.5f) * sw;
		const float sy = (-ndcY * 0.5f + 0.5f) * sh;

		const float dx = sx - (float)mouseX;
		const float dy = sy - (float)mouseY;
		const float d2 = dx * dx + dy * dy;

		if (d2 < bestD2) {
			bestD2 = d2;
			best = i;
		}
	}

	return best;
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

	operationUiVisible_ = input->IsKeyPressed(DIK_TAB);

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

						// 鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・｢鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ繝ｻ蜿門旭繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｱ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｫ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬮ｯ貊ゑｽｽ・｢髫ｲ蟶ｷ閻ｸ繝ｻ・ｧ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬮ｯ貅ｷ繝ｻ關難ｽｭ髫ｨ・ｳ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ鬯ｮ・ｮ遶擾ｽｵ郢晢ｽｻ鬮ｯ讖ｸ・ｽ・ｳ髯橸ｽ｢繝ｻ・ｹ驛｢譎｢・ｽ・ｻ鬮ｯ・ｷ闔ｨ竏晢ｽｮ・ｦ郢晢ｽｻ繝ｻ・ｾ驛｢譎｢・ｽ・ｻ郢晢ｽｻ陝ｶ譎乗凄鬮｢・ｧ繝ｻ・ｲ郢晢ｽｻ繝ｻ・ｮ髫ｴ莨夲ｽｽ・ｦ郢晢ｽｻ繝ｻ・ｼ鬨ｾ雜｣・ｽ・ｯ驕ｶ髮・・・・ｰ髮懶ｽ｣繝ｻ・ｽ繝ｻ・ｦ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬮ｯ讓奇ｽｻ繧托ｽｽ・ｽ繝ｻ・ｧ鬩搾ｽｵ繝ｻ・ｺ鬯ｮ・ｦ繝ｻ・ｪ郢晢ｽｻ遶丞｣ｹ繝ｻ驛｢譎｢・ｽ・ｻ
						float radius = 60.0f * prop.scale.x;
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
			int hover = handView_.PickIndexByMouse(
				mouse.x, mouse.y,
				cam_->GetViewProjectionMatrix(),
				(float)WinApp::kClientWidth, (float)WinApp::kClientHeight
			);
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

			OutputDebugStringA(("Before EndTurn hand=" + std::to_string(hand_.size()) +
				" deck=" + std::to_string(deck_.size()) +
				" discard=" + std::to_string(discard_.size()) +
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
				int hover = handView_.PickIndexByMouse(
					mouse.x, mouse.y,
					cam_->GetViewProjectionMatrix(),
					(float)WinApp::kClientWidth, (float)WinApp::kClientHeight
				);
				handView_.SetHoverIndex(hover);
			} else {
				handView_.SetHoverIndex(-1);
			}

			switch (cardState_) {
			case CardInputState::Idle:
				handView_.SetDrag(-1, 0, 0, false);
				handView_.SetPreviewIndex(-1);

				if (lTrig) {
					int idx = handView_.PickIndexByMouse(
						mouse.x, mouse.y,
						cam_->GetViewProjectionMatrix(),
						(float)WinApp::kClientWidth, (float)WinApp::kClientHeight
					);
					if (idx >= 0) {
						selectedIndex_ = idx;
						dragStartMouse_ = mouse;
						dragDx_ = dragDy_ = 0.0f;
						cardState_ = CardInputState::Dragging;
					}
				}
				break;

			case CardInputState::Dragging:
			{
				dragDx_ = float(mouse.x - dragStartMouse_.x);
				dragDy_ = float(mouse.y - dragStartMouse_.y);

				handView_.SetDrag(selectedIndex_, dragDx_, dragDy_, true);

				const float threshold = 80.0f;

				if (lRel) {
					handView_.SetDrag(-1, 0, 0, false);

					if (dragDy_ <= -threshold) {
						PlaySE_("SE_CardFlick");
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
				handView_.SetPreviewIndex(selectedIndex_);

				if (lTrig) {
					int idx = selectedIndex_;
					if (idx >= 0 && idx < (int)hand_.size()) {
						CardInstance inst = hand_[idx];
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
									dmgVal += (player_ ? player_->GetBlock() : 0) * effect.value;
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
							PlaySE_("SE_CardPlay");
							PlayAttackSEForCard_(*def);
							auto usedCardView = handView_.ExtractCardAt(idx);
							hand_.erase(hand_.begin() + idx);
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

				if (rTrig) {
					cardState_ = CardInputState::Idle;
					selectedIndex_ = -1;
					handView_.SetPreviewIndex(-1);
				}
				break;

			case CardInputState::ChoosingFieldReplace:
			{
				if (pendingCardView_) {

					Vector3 previewPos = { -10.f, 2.0f, 3.0 };
					pendingCardView_->SetTransform(previewPos, { 0.0f, 0.0f, 0.0f }, { 1.f, 1.f, 1.f });
					pendingCardView_->Update(dt); // 鬮ｫ・ｰ繝ｻ・ｰ髯ｷﾂ隲､諛医・鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩搾ｽｵ繝ｻ・ｺ髮九・竏槭・・ｽ遶擾ｽｫ繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫUpdate鬩幢ｽ｢繝ｻ・ｧ鬮ｮ蛹ｺ・ｨ雋ｻ・ｽ・ｻ闕ｵ貊ゑｽｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｶ
				}

				int newHover = PickFieldIndexByMouse_(mouse.x, mouse.y);

				if (newHover != fieldReplaceHoverIndex_) {
					fieldReplaceHoverIndex_ = newHover;
					fieldLayoutDirty_ = true;
				}

				if (lTrig) {
					int replaceIndex = fieldReplaceHoverIndex_;
					if (replaceIndex >= 0 && replaceIndex < (int)field_.size() && hasPendingCard_) {
						PlaySE_("SE_CardFlick");
						if (fieldViews_[replaceIndex]) {
							handView_.AddDiscardingCard(std::move(fieldViews_[replaceIndex]));
						}
						discard_.push_back(field_[replaceIndex]);
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

				if (rTrig) {
					if (hasPendingCard_) {
						PlaySE_("SE_CardFlick");
						discard_.push_back(pendingCard_);
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
				int hoverIndex = enemyMgr_->PickEnemyByMouse(
					mouse.x, mouse.y,
					cam_->GetViewProjectionMatrix(),
					(float)WinApp::kClientWidth, (float)WinApp::kClientHeight
				);

				Vector4 defaultColor{ 1.0f, 1.f, 1.f, 1.0f };

				// 鬩幢ｽ｢隴弱・・ｽ・ｧ繝ｻ・ｭ驍ｵ・ｺ髢ｧ・ｲ繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩搾ｽｵ繝ｻ・ｺ鬩募●繝ｻ髯ｬ貊・＠繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｪ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・｣鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ郢晢ｽｻ驍・私・ｽ・ｬ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｵ鬩幢ｽ｢繝ｻ・ｧ髯晢ｽｶ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｻ驛｢譎｢・ｽ・ｻ髴大､ｲ・ｽ・｡鬩搾ｽｵ繝ｻ・ｺ髣包ｽｳ隶抵ｽｭ郢晢ｽｻ鬩幢ｽ｢繝ｻ・ｧ髯晢ｽｲ繝ｻ・ｨ髫ｨ・ｳ霑｢證ｦ・ｽ・ｹ繝ｻ・ｧ驛｢譎｢・ｽ・ｻ
				for (auto& enemy : enemyMgr_->GetEnemies()) {
					enemy.SetHighlight(false);
				}
				if (hoverIndex >= 0) {
					enemyMgr_->GetEnemies()[hoverIndex].SetHighlight(true);
				}

				// 鬮ｯ譎｢・ｽ・ｾ郢晢ｽｻ繝ｻ・ｦ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｯ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｪ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驍ｵ・ｺ鬩｢謳ｾ・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬮ｮ謇具ｽｶ・｣繝ｻ・ｽ繝ｻ・ｺ鬮ｯ讖ｸ・ｽ・ｳ髯橸ｽ｢繝ｻ・ｹ郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｦ鬮ｫ・ｰ繝ｻ・ｾ郢晢ｽｻ繝ｻ・ｻ鬮ｫ・ｰ繝ｻ・ｦ驛｢譎｢・ｽ・ｻ
				if (lTrig) {
					if (hoverIndex >= 0) {
						Enemy& targetEnemy = enemyMgr_->GetEnemies()[hoverIndex];
						currentEnemyIndex_ = hoverIndex;

						if (isPokerDamageTargeting_) {
							actionSequenceTarget_ = &targetEnemy;
							ExecutePendingAttack_(targetEnemy);
							return;
						}

						const CardInstance& inst = hand_[pendingCardHandIndex_];
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
				if (rTrig) {
					if (isPokerDamageTargeting_ && tutorialLockPokerTargetingCancel_) {
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
						(currentEnemyIndex_ < enemyActedByCountThisTurn_.size() &&
							enemyActedByCountThisTurn_[currentEnemyIndex_]))) {
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
		enemyStatusUi_.UpdateLayout(enemies, enemyActionCounts_, enemyActedByCountThisTurn_, viewMat, projMat);
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

	if (PointInRect(mouse.x, mouse.y,
		layout.infoButtonRect.x, layout.infoButtonRect.y,
		layout.infoButtonRect.w, layout.infoButtonRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::None;
		if (lTrig) {
			pokerQuickPreviewVisible_ = !pokerQuickPreviewVisible_;
			return;
		}
	} else if (PointInRect(mouse.x, mouse.y,
		layout.activateYesRect.x, layout.activateYesRect.y,
		layout.activateYesRect.w, layout.activateYesRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::ActivateYes;
	} else if (!tutorialActivateOnly_ &&
		PointInRect(mouse.x, mouse.y,
			layout.activateNoRect.x, layout.activateNoRect.y,
			layout.activateNoRect.w, layout.activateNoRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::ActivateNo;
	} else if (!tutorialActivateOnly_ &&
		PointInRect(mouse.x, mouse.y,
			layout.activateViewBoardRect.x, layout.activateViewBoardRect.y,
			layout.activateViewBoardRect.w, layout.activateViewBoardRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::ActivateViewBoard;
	}

	if (yTrig || (lTrig && pokerMouseChoice_ == PokerMouseChoice::ActivateYes)) {
		pokerQuickPreviewVisible_ = false;
		pokerChoiceState_ = PokerChoiceState::WaitingEffectChoice;
		pokerChoiceJustOpened_ = true;
		return;
	}

	if (!tutorialActivateOnly_ &&
		(nTrig || (lTrig && pokerMouseChoice_ == PokerMouseChoice::ActivateNo))) {
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Skipped;
		pokerChoiceState_ = PokerChoiceState::None;
		return;
	}

	if (!tutorialActivateOnly_ &&
		lTrig && pokerMouseChoice_ == PokerMouseChoice::ActivateViewBoard) {
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
	}

	// 鬩幢ｽ｢隴擾ｽｶ郢晢ｽｻ繝ｻ螳茨ｽ､・ｼ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴主・讓溘・蜿悶渚繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・｢鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｫ鬮｣蛹・ｽｽ・ｳ郢晢ｽｻ繝ｻ・ｭ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｯ鬩搾ｽｵ繝ｻ・ｲ髫ｶ蜷ｩ・ｸ・ｻ鬯ｨ鬥ｴ蛹夊泛雜｣・ｽ・ｼ隰夲ｽｫ郢晢ｽｻ鬩幢ｽ｢繝ｻ・ｧ髣包ｽｵ隰ｨ魑ｴﾂ鬯ｮ・ｦ繝ｻ・ｪ郢晢ｽｻ陞ｳ螟ｲ・｣・ｰ隰・∞・ｽ・ｽ繝ｻ・ｷ鬮ｯ蜈ｷ・ｽ・ｻ郢晢ｽｻ繝ｻ・ｶ鬩幢ｽ｢隴乗・・ｽ・ｸ驗呻ｽｫ遶包ｽｧ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｩ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ
	if (tutorialActivateOnly_) {
		if (!(PointInRect(mouse.x, mouse.y,
			layout.infoButtonRect.x, layout.infoButtonRect.y,
			layout.infoButtonRect.w, layout.infoButtonRect.h))) {
			pokerMouseChoice_ = PokerMouseChoice::ActivateYes;
		}
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

	// 鬮ｮ諠ｹ・ｺ・･繝ｻ・ｼ繝ｻ・ｱ驛｢譎｢・ｽ・ｵ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｬ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・｣繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ髫ｨ繝ｻ・ｽ・ｲ鬩搾ｽｵ繝ｻ・ｺ髮九・竏槭・・ｽ鬪ｰ蜈ｷ・ｽ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｪ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｻ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ驛｢譎｢・ｽ・ｨ
	pokerMouseChoice_ = PokerMouseChoice::None;

	// -----------------------------
	// 鬩幢ｽ｢隴擾ｽｶ郢晢ｽｻ繝ｻ螳茨ｽ､・ｼ繝ｻ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴主・讓溘・蜿悶渚繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・｢鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｫ鬮｣蛹・ｽｽ・ｳ郢晢ｽｻ繝ｻ・ｭ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｯ鬩搾ｽｵ繝ｻ・ｲ髯溷供・ｾ貉門ｸ斟碑ｭ趣ｽ｢繝ｻ・ｽ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｸ鬩搾ｽｵ繝ｻ・ｲ鬯ｮ・ｦ繝ｻ・ｪ髫ｨ繝ｻ・ｽ・｡鬩搾ｽｵ繝ｻ・ｺ髯樊ｻゑｽｽ・ｧ驛｢譎｢・ｽ・ｻ鬩幢ｽ｢繝ｻ・ｧ髯晢ｽｲ繝ｻ・ｨ髫ｨ・ｳ霑｢證ｦ・ｽ・ｹ繝ｻ・ｧ驛｢譎｢・ｽ・ｻ
	// -----------------------------
	if (tutorialDamageOnly_) {
		// info鬩幢ｽ｢隴弱・魃ｵ驍ｵ・ｺ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ髣比ｼ夲ｽｽ・｣驛｢譎｢・ｽ・ｻ鬯ｯ・ｨ繝ｻ・ｾ髯橸ｽ｢繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｸ郢晢ｽｻ繝ｻ・ｸ鬯ｯ・ｨ繝ｻ・ｾ髯橸ｽ｢繝ｻ・ｹ郢晢ｽｻ鬯伜∞・ｽ・ｬ陞滂ｽｲ繝ｻ・ｽ繝ｻ・ｼ鬩搾ｽｵ繝ｻ・ｺ髯晢ｽｶ陷ｻ・ｻ繝ｻ・ｽ郢晢ｽｻ
		if (PointInRect(mouse.x, mouse.y,
			layout.infoButtonRect.x, layout.infoButtonRect.y,
			layout.infoButtonRect.w, layout.infoButtonRect.h)) {
			if (lTrig) {
				pokerQuickPreviewVisible_ = !pokerQuickPreviewVisible_;
				return;
			}
		} else {
			// 鬮ｯ譎｢・ｽ・ｶ郢晢ｽｻ繝ｻ・ｸ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢隰ｨ魑ｴﾂ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｸ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ髣比ｼ夲ｽｽ・｣驛｢譎｢・ｽ・ｯ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｩ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ
			pokerMouseChoice_ = PokerMouseChoice::EffectDamage;
		}

		if (lTrig && pokerMouseChoice_ == PokerMouseChoice::EffectDamage) {
			pendingDamage_ = CalcFinalAttackDamage_(bonus.damage);
			isPokerDamageTargeting_ = true;
			tutorialLockPokerTargetingCancel_ = true;
			pokerQuickPreviewVisible_ = false;

			// 鬩搾ｽｵ繝ｻ・ｺ鬮ｦ・ｮ陷ｻ・ｻ繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｯ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｾ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬯ｨ・ｾ陷茨ｽｷ繝ｻ・ｽ繝ｻ・ｺ鬮ｯ・ｷ隶手ｴ具ｽｾ蟶吶・繝ｻ・ｮ髯溷桁・ｽ・｡郢晢ｽｻ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ驕ｶ莨∬ｱｪ繝ｻ・ｸ繝ｻ・ｺ髯ｷ莨夲ｽｽ・ｱ驕ｶ莨√・繝ｻ・ｸ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
			lastPokerTutorialResult_ = PokerTutorialResult::None;

			cardState_ = CardInputState::ChoosingEnemyTarget;
			pokerChoiceState_ = PokerChoiceState::None;
			return;
		}

		return;
	}

	// -----------------------------
	// 鬯ｯ・ｨ繝ｻ・ｾ髯橸ｽ｢繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｸ郢晢ｽｻ繝ｻ・ｸ鬮ｫ・ｴ陟托ｽｱ郢晢ｽｻ
	// -----------------------------
	if (PointInRect(mouse.x, mouse.y,
		layout.infoButtonRect.x, layout.infoButtonRect.y,
		layout.infoButtonRect.w, layout.infoButtonRect.h)) {
		if (lTrig) {
			pokerQuickPreviewVisible_ = !pokerQuickPreviewVisible_;
			return;
		}
	} else if (PointInRect(mouse.x, mouse.y,
		layout.backRect.x, layout.backRect.y,
		layout.backRect.w, layout.backRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::EffectBack;
	} else if (PointInRect(mouse.x, mouse.y,
		layout.effectRects[0].x, layout.effectRects[0].y,
		layout.effectRects[0].w, layout.effectRects[0].h)) {
		pokerMouseChoice_ = PokerMouseChoice::EffectAtkUp;
	} else if (PointInRect(mouse.x, mouse.y,
		layout.effectRects[1].x, layout.effectRects[1].y,
		layout.effectRects[1].w, layout.effectRects[1].h)) {
		pokerMouseChoice_ = PokerMouseChoice::EffectDamage;
	} else if (PointInRect(mouse.x, mouse.y,
		layout.effectRects[2].x, layout.effectRects[2].y,
		layout.effectRects[2].w, layout.effectRects[2].h)) {
		pokerMouseChoice_ = PokerMouseChoice::EffectDraw;
	} else if (PointInRect(mouse.x, mouse.y,
		layout.effectViewBoardRect.x, layout.effectViewBoardRect.y,
		layout.effectViewBoardRect.w, layout.effectViewBoardRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::EffectViewBoard;
	}

	if (lTrig && pokerMouseChoice_ == PokerMouseChoice::EffectAtkUp) {
		nextTurnAtkUp_ += bonus.atkUp;
		TriggerSubEffectsForField_(SubEffectTrigger::OnPokerSkillActivated, currentPoker_.rank);
		ConsumeFieldCards_();
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Activated;
		pokerChoiceState_ = PokerChoiceState::None;
		turn_ = TurnState::Enemy;
		enemyWait_ = 1.0f;
		return;
	}

	if (lTrig && pokerMouseChoice_ == PokerMouseChoice::EffectDraw) {
		DrawCards_(bonus.drawCount);
		TriggerSubEffectsForField_(SubEffectTrigger::OnPokerSkillActivated, currentPoker_.rank);
		ConsumeFieldCards_();
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Activated;
		pokerChoiceState_ = PokerChoiceState::None;
		turn_ = TurnState::Enemy;
		enemyWait_ = 1.0f;
		return;
	}

	if (lTrig && pokerMouseChoice_ == PokerMouseChoice::EffectDamage) {
		pendingDamage_ = CalcFinalAttackDamage_(bonus.damage);
		isPokerDamageTargeting_ = true;
		pokerQuickPreviewVisible_ = false;

		// 鬩搾ｽｵ繝ｻ・ｺ鬮ｦ・ｮ陷ｻ・ｻ繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｯ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｾ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬯ｩ蠅捺・繝ｻ・ｽ繝ｻ・ｺ鬮ｯ讖ｸ・ｽ・ｳ髯橸ｽ｢繝ｻ・ｹ郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｪ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
		lastPokerTutorialResult_ = PokerTutorialResult::None;

		cardState_ = CardInputState::ChoosingEnemyTarget;
		pokerChoiceState_ = PokerChoiceState::None;
		return;
	}

	if (nTrig || (lTrig && pokerMouseChoice_ == PokerMouseChoice::EffectBack)) {
		pokerQuickPreviewVisible_ = false;
		pokerChoiceState_ = PokerChoiceState::WaitingActivateChoice;
		return;
	}

	if (lTrig && pokerMouseChoice_ == PokerMouseChoice::EffectViewBoard) {
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
	}
}

void BattleController::HandlePokerViewBoard_(FieldUi& fieldUi, POINT mouse, bool lTrig, float dt)
{
	handView_.SetPreviewIndex(-1);
	handView_.SetDrag(-1, 0.0f, 0.0f, false);
	handView_.SetFocusIndex(-1);

	const auto& layout = fieldUi.GetPokerEffectChoiceLayout();

	if (PointInRect(mouse.x, mouse.y,
		layout.backRect.x, layout.backRect.y,
		layout.backRect.w, layout.backRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::ReturnFromBoard;
	} else {
		pokerMouseChoice_ = PokerMouseChoice::None;
	}

	int hover = handView_.PickIndexByMouse(
		mouse.x, mouse.y,
		cam_->GetViewProjectionMatrix(),
		(float)WinApp::kClientWidth, (float)WinApp::kClientHeight
	);
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
	actionDirector_.Draw3D();
	damagePopupUi_.Draw3D();
}

void BattleController::DrawPlayerBattleStatusUI(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj)
{
	(void)app;
	playerStatusUi_.SetTexts(
		GetPlayerHpTexts(),
		GetPlayerBlockText(),
		GetPlayerPowerBoostText());
	playerStatusUi_.DrawStatus2D(view, proj);
}

void BattleController::DrawEnemyBattleStatusHpTexts(const Matrix4x4& view, const Matrix4x4& proj)
{
	enemyStatusUi_.DrawHpTexts2D(view, proj);
}

void BattleController::DrawEnemyBattleStatusBcTexts(const Matrix4x4& view, const Matrix4x4& proj)
{
	enemyStatusUi_.DrawBcTexts2D(view, proj);
}

void BattleController::DrawCardArea3D(GameApp& app)
{
	(void)app;
	// 鬮ｯ貅倥・・ゑｽｧ髫ｲ・ｷ郢晢ｽｻ
	if (discardView_) {
		discardView_->Draw();
	}

	// 鬮ｫ・ｰ郢晢ｽｻ驍・・・ｫ・ｰ郢晢ｽｻ
	handView_.Draw();
	handView_.DrawPreviewCard();

	// 鬩幢ｽ｢隰ｨ魑ｴﾂ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・｡鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｸ鬩幢ｽ｢隴弱・・ｺ・｢驛｢譎｢・ｽ・｣鬩幢ｽ｢隴惹ｸ橸ｽｹ・ｲ驍ｵ・ｺ郢晢ｽｻ繝ｻ・ｹ隴擾ｽｴ郢晢ｽｻ驛｢譎｢・ｽ・ｻ
	damagePopupUi_.Draw3D();

	// 鬮ｯ諛ｶ・ｽ・｣郢晢ｽｻ繝ｻ・ｴ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｨ鬮｣雋ｻ・｣・ｰ郢晢ｽｻ繝ｻ・､鬮ｫ・ｰ繝ｻ・ｰ髯晢ｽｶ繝ｻ・ｶ髯ｷ繝ｻ・ｽ・ｾ鬩幢ｽ｢隴弱・・ｽ・ｼ隴∫ｵｶ隘夜ｩ幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｫ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｿ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ
	if (cardState_ == CardInputState::ChoosingFieldReplace) {
		highlightFilter_->Draw();
		pendingCardView_->Draw();
	}

	// 鬮ｯ諛ｶ・ｽ・｣郢晢ｽｻ繝ｻ・ｴ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｫ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴擾ｽｴ郢晢ｽｻ
	for (auto& c : fieldViews_) {
		c->Draw();
	}
}

void BattleController::DrawField3D(GameApp& app)
{
	(void)app;
	if (propManager_) {
		propManager_->Draw3D();
	}
}

void BattleController::DrawBattleOverlay3D(GameApp& app)
{
	if (cardState_ == CardInputState::ChoosingEnemyTarget) {
		highlightFilter_->Draw();

		if (enemyMgr_) {
			// 3D鬮ｫ・ｰ繝ｻ・ｰ髯ｷﾂ隲､諛医・鬯ｨ・ｾ陋ｹ繝ｻ・ｽ・ｽ繝ｻ・ｨ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩幢ｽ｢隴乗・・ｽ・ｻ繝ｻ・｣驍ｵ・ｺ郢晢ｽｻ繝ｻ・ｹ隴惹ｸ橸ｽｹ・ｲ繝ｻ荳ｻ・ｸ・ｷ繝ｻ・ｹ繝ｻ・ｧ郢晢ｽｻ繝ｻ・､鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬮ｫ・ｰ鬲・ｼ夲ｽｽ・ｽ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ
			app.ObjCom()->SetGraphicsPipelineState();
			// Z鬩幢ｽ｢隴寂・繝ｻ驛｢譎｢・ｽ・｣鬩幢ｽ｢隴弱・・ｽ・ｼ隴∵腸・ｼ諞ｺﾎ斐・・ｧ髯句ｹ｢・ｽ・ｵ驍ｵ・ｺ鬩｢謳ｾ・ｽ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｪ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・｢鬩搾ｽｵ繝ｻ・ｺ髯ｷ莨夲ｽｽ・ｱ驕ｯ・ｶ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｲ驛｢譎｢・ｽ・ｻ郢晢ｽｻ繝ｻ・ｻ髯橸ｽｳ陞｢・ｹ・取ｨ｣蝙馴屆・｣繝ｻ・ｽ繝ｻ・ｯ驛｢譎｢・ｽ・ｻ驛｢譎｢・ｽ・ｻighlightFilter_驛｢譎｢・ｽ・ｻ髯晢ｽｲ繝ｻ・ｨ郢晢ｽｻ髢ｧ・ｲ繝ｻ・ｹ繝ｻ・ｧ鬩怜遜・ｽ・ｫ郢晢ｽｻ郢ｧ螂・ｽｽ・ｬ郢晢ｽｻ陷夲ｽｱ髴趣ｽｯ隲ｷ蛹・ｽｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬮ｫ・ｰ繝ｻ・ｰ髯ｷﾂ隲､諛医・鬩搾ｽｵ繝ｻ・ｺ鬮ｴ驛・ｽｲ・ｻ繝ｻ・ｽ隶呵ｶ｣・ｽ・ｹ繝ｻ・ｧ髣包ｽｵ隴趣ｽ｢繝ｻ・ｽ髢ｧ・ｲ繝ｻ・ｸ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ驕ｶ莨∬ｱｪ繝ｻ・ｸ繝ｻ・ｺ髯ｷ・ｷ繝ｻ・ｶ郢晢ｽｻ郢晢ｽｻ
			app.Dx()->ClearDepthBuffer();

			bool hasHighlightedEnemy = false;
			for (const auto& enemy : enemyMgr_->GetEnemies()) {
				hasHighlightedEnemy = hasHighlightedEnemy || enemy.IsHighlighted();
			}

			if (sEnemyTargetBloomEnabled && hasHighlightedEnemy) {
				BloomParam targetParam = MakeEnemyTargetBloomParam_(app.ObjectPost()->GetParam(), sPokerGlowRainbowTime);
				app.ObjectPost()->SetParam(targetParam);
				app.BeginObjectPostEffect();
				for (auto& enemy : enemyMgr_->GetEnemies()) {
					if (enemy.IsHighlighted()) {
						enemy.Draw();
					}
				}
				app.EndObjectPostEffect();
				app.ObjCom()->SetGraphicsPipelineState();
			}

			for (auto& enemy : enemyMgr_->GetEnemies()) {
				if (enemy.IsHighlighted()) {
					enemy.Draw();
				}
			}
		}
	}

}

void BattleController::DrawPostEffect3D(GameApp& app)
{
	handView_.DrawDiscardingCardsObjectPost(app);
}

void BattleController::DrawFieldFrameBloom(GameApp& app)
{
	if (!sFieldFrameBloomEnabled) {
		return;
	}

	const std::array<bool, 5> highlightMask = GetPokerHighlightMask_();
	bool hasBloomTarget = false;
	const bool inReplacePreview = cardState_ == CardInputState::ChoosingFieldReplace && hasPendingCard_;
	for (int i = 0; i < (int)fieldViews_.size(); ++i) {
		const bool replacePreview = i < static_cast<int>(fieldReplacePreviewActive_.size()) &&
			fieldReplacePreviewActive_[i];
		if (fieldViews_[i] && (replacePreview || (!inReplacePreview && i < 5 && highlightMask[i] && currentPoker_.rank != PokerHandRank::None))) {
			hasBloomTarget = true;
			break;
		}
	}
	for (PokerHandRank rank : handPreviewRanks_) {
		if (rank != PokerHandRank::None) {
			hasBloomTarget = true;
			break;
		}
	}
	if (!hasBloomTarget) {
		return;
	}

	BloomParam param = app.ObjectPost()->GetParam();
	param.threshold = sFieldFrameBloomThreshold;
	const float pulse = sFieldFrameBloomMinPulse + (1.0f - sFieldFrameBloomMinPulse) * (0.5f + 0.5f * std::sin(sPokerGlowRainbowTime * 1.6f));
	param.intensity = std::max(sFieldFrameBloomIntensity, sHandFrameBloomIntensity) * pulse;
	param.vignetteIntensity = 0.0f;
	param.vignetteScale = 0.0f;
	param.distortionAmount = 0.0f;
	param.chromAbAmount = sFieldFrameBloomChromAb;
	param.isGrayscale = 0.0f;
	param.isInverted = 0.0f;
	param.noiseIntensity = 0.0f;
	param.scanlineIntensity = 0.0f;
	param.curvature = 0.0f;
	param.borderSharp = 0.0f;
	param.glitchAmount = 0.0f;
	param.dissolveAmount = -1.0f;

	app.ObjectPost()->SetParam(param);
	app.BeginObjectPostEffect();
	for (int i = 0; i < (int)fieldViews_.size(); ++i) {
		const bool replacePreview = i < static_cast<int>(fieldReplacePreviewActive_.size()) &&
			fieldReplacePreviewActive_[i];
		const bool currentHighlight = !inReplacePreview && i < 5 && highlightMask[i] && currentPoker_.rank != PokerHandRank::None;
		if (!fieldViews_[i] || (!replacePreview && !currentHighlight)) {
			continue;
		}
		fieldViews_[i]->DrawFrameOnly();
	}
	const int handCount = std::min<int>(static_cast<int>(handPreviewRanks_.size()), handView_.GetCardCount());
	for (int i = 0; i < handCount; ++i) {
		if (handPreviewRanks_[i] == PokerHandRank::None) {
			continue;
		}
		Card3D* card = handView_.GetCard(i);
		if (card) {
			card->DrawFrameOnly();
		}
	}
	app.EndObjectPostEffect();
	app.ObjCom()->SetGraphicsPipelineState();
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
	if (actionDirector_.IsPlaying()) {
		actionDirector_.Draw2D();
		return;
	}

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
		0, 0,
		float(WinApp::kClientWidth),
		float(WinApp::kClientHeight),
		0, 100
	);
	DrawHpGaugeBloom_(app, view, proj);
	if (player_) {
		playerStatusUi_.DrawHpGauge(
			player_->GetHP(),
			player_->GetMaxHP(),
			player_->GetBlock(),
			std::max(0, CalcTotalIncomingDamage()),
			sPokerGlowRainbowTime,
			sHpDamageBlinkSpeed);
	}

	if (enemyMgr_) {
		auto& enemies = enemyMgr_->GetEnemies();
		EnemyBattleStatusUI::EnemyBloomSettings bloomSettings{};
		bloomSettings.intentEnabled = sEnemyIntentBloomEnabled;
		bloomSettings.intentIntensity = sEnemyIntentBloomIntensity;
		bloomSettings.intentMinPulse = sEnemyIntentBloomMinPulse;
		enemyStatusUi_.DrawGaugeAndIntent2D(
			app,
			enemies,
			enemyActedByCountThisTurn_,
			sPokerGlowRainbowTime,
			view,
			proj,
			bloomSettings);
	}
	actionDirector_.Draw2D();
}

void BattleController::DrawHpGaugeBloom_(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj)
{
	if (!sHpGaugeBloomEnabled) {
		return;
	}

	const float baseIntensity = sHpGaugeBloomIntensity;
	if (player_) {
		playerStatusUi_.DrawHpGaugeBloom(
			app,
			view,
			proj,
			player_->GetHP(),
			player_->GetMaxHP(),
			player_->GetBlock(),
			std::max(0, CalcTotalIncomingDamage()),
			sPokerGlowRainbowTime,
			baseIntensity,
			sHpDamageBloomIntensity,
			sHpDamageBlinkSpeed);
	}

	if (enemyMgr_) {
		auto& enemies = enemyMgr_->GetEnemies();
		enemyStatusUi_.DrawGaugeBloom(app, enemies, view, proj, baseIntensity);
	}
}

#ifdef USE_IMGUI
#include <imgui.h>
void BattleController::DrawPlayerHudImGuiControls()
{
	playerStatusUi_.DrawImGuiControls();
}

void BattleController::DrawImGui()
{
	Card3D::DrawAdjustImGui();

	ImGui::Text("turn: %s", turn_ == TurnState::Player ? "Player" : "Enemy");
	ImGui::Text("PlayerTurnCount : %d", playerTurnCount_);
	ImGui::Text("EnemyTurnCount : %d", enemyTurnCount_);
	ImGui::Text("energy: %d / %d", energy_, energyMax_);
	ImGui::Text("hand: %d  discard: %d", (int)hand_.size(), (int)discard_.size());
	ImGui::Text("field: %d", (int)field_.size());

	if (ImGui::CollapsingHeader("Field Card Glitter")) {
		ImGui::DragFloat3("Emitter Local Offset", &sFieldCardGlitterLocalOffset.x, 0.01f);
		ImGui::DragFloat("Emitter Spread X", &sFieldCardGlitterSpreadX, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Emitter Spread Y", &sFieldCardGlitterSpreadY, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Emit Interval", &sFieldCardGlitterEmitInterval, 0.01f, 0.01f, 2.0f);
		ImGui::SliderInt("Normal Count", &sFieldCardGlitterNormalCount, 0, 30);
		ImGui::SliderInt("Highlight Count", &sFieldCardGlitterHighlightCount, 0, 60);
		ImGui::Checkbox("Hand Poker Preview", &sHandPokerPreviewEnabled);
		ImGui::DragFloat("Hand Emit Interval", &sHandCardGlitterEmitInterval, 0.01f, 0.01f, 2.0f);
		ImGui::SliderInt("Hand Count", &sHandCardGlitterCount, 0, 60);
		if (ImGui::Button("Reset Field Card Glitter")) {
			sFieldCardGlitterLocalOffset = { 0.0f, 2.2f, 0.12f };
			sFieldCardGlitterSpreadX = 1.05f;
			sFieldCardGlitterSpreadY = 0.08f;
			sFieldCardGlitterEmitInterval = 0.12f;
			sFieldCardGlitterNormalCount = 2;
			sFieldCardGlitterHighlightCount = 5;
			sHandPokerPreviewEnabled = true;
			sHandCardGlitterEmitInterval = 0.12f;
			sHandCardGlitterCount = 3;
		}
	}

	if (ImGui::CollapsingHeader("Field Frame Bloom")) {
		ImGui::Checkbox("Enable Frame Bloom", &sFieldFrameBloomEnabled);
		ImGui::DragFloat("Frame Bloom Threshold", &sFieldFrameBloomThreshold, 0.01f, 0.0f, 3.0f);
		ImGui::DragFloat("Frame Bloom Intensity", &sFieldFrameBloomIntensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Hand Bloom Intensity", &sHandFrameBloomIntensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Frame Bloom Min Pulse", &sFieldFrameBloomMinPulse, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Frame Bloom ChromAb", &sFieldFrameBloomChromAb, 0.0005f, 0.0f, 0.05f);
		if (ImGui::Button("Reset Field Frame Bloom")) {
			sFieldFrameBloomEnabled = true;
			sFieldFrameBloomThreshold = 0.35f;
			sFieldFrameBloomIntensity = 1.45f;
			sHandFrameBloomIntensity = 1.15f;
			sFieldFrameBloomMinPulse = 0.15f;
			sFieldFrameBloomChromAb = 0.0015f;
		}
	}

	if (ImGui::CollapsingHeader("Enemy Bloom")) {
		ImGui::Checkbox("Intent Bloom", &sEnemyIntentBloomEnabled);
		ImGui::DragFloat("Intent Intensity", &sEnemyIntentBloomIntensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Intent Min Pulse", &sEnemyIntentBloomMinPulse, 0.01f, 0.0f, 1.0f);
		ImGui::Checkbox("Target Bloom", &sEnemyTargetBloomEnabled);
		ImGui::DragFloat("Target Intensity", &sEnemyTargetBloomIntensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Target ChromAb", &sEnemyTargetBloomChromAb, 0.0005f, 0.0f, 0.05f);
		if (ImGui::Button("Reset Enemy Bloom")) {
			sEnemyIntentBloomEnabled = true;
			sEnemyIntentBloomIntensity = 1.8f;
			sEnemyIntentBloomMinPulse = 0.45f;
			sEnemyTargetBloomEnabled = true;
			sEnemyTargetBloomIntensity = 2.1f;
			sEnemyTargetBloomChromAb = 0.002f;
		}
	}

	if (ImGui::CollapsingHeader("HP Gauge Bloom")) {
		ImGui::Checkbox("HP Bloom", &sHpGaugeBloomEnabled);
		ImGui::DragFloat("HP Bloom Intensity", &sHpGaugeBloomIntensity, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("HP Bloom Min Pulse", &sHpGaugeBloomMinPulse, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Damage Blink Speed", &sHpDamageBlinkSpeed, 0.1f, 0.0f, 20.0f);
		ImGui::DragFloat("Damage Bloom Intensity", &sHpDamageBloomIntensity, 0.01f, 0.0f, 5.0f);
		if (ImGui::Button("Reset HP Gauge Bloom")) {
			sHpGaugeBloomEnabled = true;
			sHpGaugeBloomIntensity = 0.55f;
			sHpGaugeBloomMinPulse = 0.65f;
			sHpDamageBlinkSpeed = 6.0f;
			sHpDamageBloomIntensity = 1.05f;
		}
	}

	if (ImGui::CollapsingHeader("Character Scale")) {
		if (player_ && player_->GetObject3d()) {
			Vector3 pScale = player_->GetObject3d()->GetScale();
			if (ImGui::DragFloat3("Player Scale", &pScale.x, 0.01f)) {
				player_->GetObject3d()->SetScale(pScale);
			}
		}
		if (enemyMgr_) {
			int idx = 0;
			for (auto& enemy : enemyMgr_->GetEnemies()) {
				if (!enemy.IsAlive() || !enemy.GetObject3d()) {
					idx++;
					continue;
				}
				ImGui::PushID(idx);
				Vector3 eScale = enemy.GetObject3d()->GetScale();
				if (ImGui::DragFloat3(("Enemy " + std::to_string(idx) + " Scale").c_str(), &eScale.x, 0.01f)) {
					enemy.GetObject3d()->SetScale(eScale);
				}
				ImGui::PopID();
				idx++;
			}
		}
	}

	handView_.DrawImGui();

	const char* stateName = "";
	switch (cardState_) {
	case CardInputState::Idle: stateName = "Idle"; break;
	case CardInputState::Dragging: stateName = "Dragging"; break;
	case CardInputState::Preview: stateName = "Preview"; break;
	case CardInputState::ChoosingFieldReplace: stateName = "ChoosingFieldReplace"; break;
	}
	ImGui::Text("cardState: %s", stateName);

	if (hasPendingCard_) {
		ImGui::Text("pending: defId=%d number=%d suit=%s",
			pendingCard_.defId,
			pendingCard_.number,
			SuitToString(pendingCard_.suit));
	} else {
		ImGui::Text("pending: none");
	}

	ImGui::Separator();
	ImGui::Text("Hand Cards");
	for (int i = 0; i < (int)hand_.size(); ++i) {
		ImGui::Text("hand[%d] defId=%d number=%d suit=%s",
			i,
			hand_[i].defId,
			hand_[i].number,
			SuitToString(hand_[i].suit));
	}

	ImGui::Separator();
	ImGui::Text("Field Cards");
	for (int i = 0; i < (int)field_.size(); ++i) {
		ImGui::Text("field[%d] defId=%d number=%d suit=%s",
			i,
			field_[i].defId,
			field_[i].number,
			SuitToString(field_[i].suit));
	}

	ImGui::Separator();
	PokerHandResult poker = EvaluatePokerHand_();
	ImGui::Text("Poker Hand: %s", GetPokerHandName_(poker.rank));
	ImGui::Text("Poker Power: %d", poker.power);

	if (pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice)
	{
		ImGui::Separator();
		ImGui::Text("Poker Skill Available!");
		ImGui::Text("Hand : %s", GetPokerHandName_(currentPoker_.rank));
		ImGui::Text("Press Y = Activate");
		ImGui::Text("Press N = Skip");
	}

	if (pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice)
	{
		PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);

		ImGui::Separator();
		ImGui::Text("Choose Poker Effect");
		ImGui::Text("Hand : %s", GetPokerHandName_(currentPoker_.rank));
		ImGui::Text("1 : Next Turn ATK UP (+%d)", bonus.atkUp);
		ImGui::Text("2 : Draw %d", bonus.drawCount);
		ImGui::Text("3 : Damage %d", bonus.damage);
		ImGui::Text("N : Back");
	}
	ImGui::Separator();

	if (player_) {
		ImGui::Text("Player Hp: %d", player_->GetHP());
		ImGui::Text("Player Hp: %d (Block: %d)", player_->GetHP(), player_->GetBlock());
		ImGui::Text("Player Power: %d (CurrentTurnAtkUp: %d / NextTurnAtkUp: %d)",
			player_->GetBoostedPower(),
			currentTurnAtkUp_,
			nextTurnAtkUp_);
	} else {
		ImGui::Text("Player: null");
		ImGui::Text("Player Power: preview mode (CurrentTurnAtkUp: %d / NextTurnAtkUp: %d)",
			currentTurnAtkUp_,
			nextTurnAtkUp_);
	}

	if (enemyMgr_ && !enemyMgr_->GetEnemies().empty()) {
		ImGui::Text("Enemy Hp: %d", enemyMgr_->GetEnemies()[0].GetHP());
	} else {
		ImGui::Text("Enemy: null");
	}

	ImGui::Separator();
	if (propManager_) {
		propManager_->DrawImGui();
	}

	ImGui::Separator();
	ImGui::Text("Attack Debug");

	ImGui::Checkbox("Use Debug Preview Buff", &useDebugPreviewBuff_);

	if (player_) {
		ImGui::Text("Runtime Player Connected");

		int previewPower = player_->GetBoostedPower();
		if (ImGui::DragInt("Player PowerBoost", &previewPower, 1.0f, -999, 999)) {
			player_->ResetPowerBoost();
			if (previewPower > 0) {
				player_->PowerBoost(previewPower);
			}
		}

		ImGui::DragInt("CurrentTurnAtkUp", &currentTurnAtkUp_, 1.0f, -999, 999);
		ImGui::DragInt("NextTurnAtkUp", &nextTurnAtkUp_, 1.0f, -999, 999);
	} else {
		ImGui::Text("Preview Only (No Player Connected)");
		ImGui::DragInt("Debug PowerBoost", &debugPreviewPowerBoost_, 1.0f, -999, 999);
		ImGui::DragInt("Debug CurrentTurnAtkUp", &debugPreviewCurrentTurnAtkUp_, 1.0f, -999, 999);
		ImGui::DragInt("Debug NextTurnAtkUp", &debugPreviewNextTurnAtkUp_, 1.0f, -999, 999);
	}

	actionDirector_.DrawImGuiEditor(cam_);
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
			return (player_ ? player_->GetBlock() : 0) * effect.value;
		}
		return effect.value;
	}

	if (effect.type == "Damage") {
		return CalcFinalAttackDamage_(effect.value);
	}

	if (effect.type == "DamageAll") {
		return CalcFinalAttackDamage_(effect.value);
	}

	if (effect.type == "DamageCrescent") {
		int value = effect.value;
		if (playerTurnCount_ % 2 != 0) {
			value += 3;
		}
		return CalcFinalAttackDamage_(value);
	}

	if (effect.type == "DamageByBlock") {
		int value = (player_ ? player_->GetBlock() : 0) * effect.value;
		return CalcFinalAttackDamage_(value);
	}

	return effect.value;
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
	PlayBlockReactionSE_(beforeBlock, enemy.GetBlock(), actualDamage, damage);

	if (player_->GetVampireHeal() > 0) {
		int beforeHp = player_->GetHP();
		player_->Heal(player_->GetVampireHeal());
		PlayHealSEIfHpIncreased_(player_, beforeHp);
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
		if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(hand_.size())) {
			return db_.Find(hand_[selectedIndex_].defId);
		}
	}

	if (cardState_ == CardInputState::Preview) {
		if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(hand_.size())) {
			return db_.Find(hand_[selectedIndex_].defId);
		}
	}

	if (cardState_ == CardInputState::Idle) {
		int handHover = handView_.GetHoverIndex();
		if (handHover >= 0 && handHover < static_cast<int>(hand_.size())) {
			return db_.Find(hand_[handHover].defId);
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
		if (handHover >= 0 && handHover < static_cast<int>(hand_.size())) {
			return db_.Find(hand_[handHover].defId);
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
	return pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice ||
		pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice;
}

std::wstring BattleController::GetPokerChoiceUiText() const
{
	if (pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice) {
		std::wstring text = L"";
		text += L"ポーカー効果が発動可能です\n";
		text += L"役: ";
		text += std::wstring(GetPokerHandName_(currentPoker_.rank),
			GetPokerHandName_(currentPoker_.rank) + std::strlen(GetPokerHandName_(currentPoker_.rank)));
		text += L"\n";
		text += L"左クリック : 発動する\n";
		text += L"左クリック : 発動しない\n";
		text += L"左クリック : 場を見る\n";
		return text;
	}

	if (pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice) {
		PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);

		std::wstring text = L"";
		text += L"発動する効果を選んでください\n";
		text += L"左クリック : 戻る\n";
		text += L"左クリック : 次ターンATK UP (+" + std::to_wstring(bonus.atkUp) + L")\n";
		text += L"左クリック : " + std::to_wstring(bonus.drawCount) + L"枚ドロー\n";
		text += L"左クリック : " + std::to_wstring(bonus.damage) + L"ダメージ\n";
		text += L"左クリック : 場を見る\n";
		return text;
	}

	if (pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi) {
		std::wstring text;
		text += L"場確認中\n";
		text += L"カードにマウスを乗せて確認できます\n";
		text += L"左クリック : 特殊効果選択に戻る\n";
		return text;
	}

	return L"";
}

int BattleController::GetPokerMouseChoiceIndex() const
{
	switch (pokerMouseChoice_) {
	case PokerMouseChoice::ActivateYes:       return 0;
	case PokerMouseChoice::ActivateNo:        return 1;
	case PokerMouseChoice::ActivateViewBoard: return 2;

	case PokerMouseChoice::EffectBack:        return 0;
	case PokerMouseChoice::EffectAtkUp:       return 1;
	case PokerMouseChoice::EffectDamage:      return 2;
	case PokerMouseChoice::EffectDraw:        return 3;
	case PokerMouseChoice::EffectViewBoard:   return 4;

	case PokerMouseChoice::ReturnFromBoard:   return 0;

	default:                                  return -1;
	}
}

bool BattleController::IsWaitingActivateChoice() const
{
	return pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice;
}

bool BattleController::IsWaitingEffectChoice() const
{
	return pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice;
}

bool BattleController::IsViewingBoardFromPokerUi() const
{
	return pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi;
}

bool BattleController::ShouldShowOperationUi() const
{
	return operationUiVisible_;
}

std::wstring BattleController::GetOperationUiText() const
{
	if (cardState_ == CardInputState::ChoosingFieldReplace) {
		std::wstring text;
		text += L"カード交換\n";
		text += L"左クリック : 選択中の場カードと入れ替える\n";
		text += L"右クリック : 入れ替えず墓地へ送る\n";
		return text;
	}

	if (cardState_ == CardInputState::Preview) {
		std::wstring text;
		text += L"カード選択中\n";
		text += L"左クリック : 使用する\n";
		text += L"右クリック : キャンセル\n";
		text += L"Tab : 操作説明を表示\n";
		return text;
	}

	std::wstring text;
	text += L"基本操作\n";
	text += L"左クリック＋上ドラッグ : カードをプレビュー\n";
	text += L"プレビュー中に左クリック : カードを使用\n";
	text += L"プレビュー中に右クリック : キャンセル\n";
	text += L"Enter : ターン終了\n";
	text += L"Tab : 操作説明を表示\n";
	return text;
}

std::wstring BattleController::GetZoneCountUiText() const
{
	std::wstring text;
	text += L"山札 : " + std::to_wstring(deck_.size()) + L"\n";
	text += L"手札 : " + std::to_wstring(hand_.size()) + L"\n";
	text += L"墓地 : " + std::to_wstring(discard_.size()) + L"\n";
	text += L"場   : " + std::to_wstring(field_.size()) + L"\n";
	return text;
}

std::wstring BattleController::GetCurrentPokerHandUiText() const
{
	PokerHandResult poker = EvaluatePokerHand_();

	if (field_.size() < 5) {
		return L"役:       判定中";
	}

	if (poker.rank == PokerHandRank::None) {
		return L"役:       なし";
	}

	switch (poker.rank) {
	case PokerHandRank::OnePair: return L"役: ワンペア";
	case PokerHandRank::TwoPair: return L"役: ツーペア";
	case PokerHandRank::ThreeOfAKind: return L"役: スリーカード";
	case PokerHandRank::Straight: return L"役: ストレート";
	case PokerHandRank::Flush: return L"役: フラッシュ";
	case PokerHandRank::FullHouse: return L"役: フルハウス";
	case PokerHandRank::FourOfAKind: return L"役: フォーカード";
	case PokerHandRank::StraightFlush: return L"役: ストレートフラッシュ";
	case PokerHandRank::RoyalStraightFlush: return L"役: ロイヤルストレートフラッシュ";
	default: return L"役: ?";
	}
}

std::wstring BattleController::GetTurnUiText() const
{
	std::wstring text;

	switch (turn_) {
	case TurnState::Player: return L"あなたのターン : " + std::to_wstring(playerTurnCount_);
	case TurnState::Enemy: return L"あいてのターン : " + std::to_wstring(enemyTurnCount_);
	}

	return text;
}

std::wstring BattleController::GetEnergyText() const {

	std::wstring text;

	text += std::to_wstring(energy_) + L" / " + std::to_wstring(energyMax_);

	return text;

}

std::vector<std::wstring> BattleController::GetEnemyHpTexts() const
{
	std::vector<std::wstring> hpTexts;
	if (!enemyMgr_) {
		return hpTexts;
	}

	auto& enemies = enemyMgr_->GetEnemies();
	for (const auto& enemy : enemies) {
		if (!enemy.IsAlive()) {
			continue;
		}
		std::wstring text = std::to_wstring(enemy.GetHP()) + L" / " + std::to_wstring(enemy.GetMaxHP());
		if (enemy.GetBlock() > 0) {
			text += L"  B " + std::to_wstring(enemy.GetBlock());
		}
		hpTexts.push_back(text);
	}
	return hpTexts;
}

std::vector<std::wstring> BattleController::GetEnemyBCTexts() const
{
	std::vector<std::wstring> bcTexts;
	if (!enemyMgr_) {
		return bcTexts;
	}

	auto& enemies = enemyMgr_->GetEnemies();
	for (const auto& enemy : enemies) {
		if (!enemy.IsAlive()) {
			continue;
		}
		const int bcPoint = enemy.GetBCPoint();
		bcTexts.push_back(bcPoint == 0 ? L"" : std::to_wstring(bcPoint));
	}
	return bcTexts;
}

BattleController::PokerBonus BattleController::GetCurrentPokerBonusForUi() const
{
	return GetPokerBonus_(currentPoker_.rank);
}


std::wstring BattleController::GetPlayerHpTexts() const {
	std::wstring text;

	text = std::to_wstring(player_->GetHP()) + L" / " + std::to_wstring(player_->GetMaxHP());

	return text;
}

bool BattleController::IsAllEnemiesDead() const {
	auto& enemies = enemyMgr_->GetEnemies();
	for (auto& e : enemies) {
		if (e.IsAlive()) return false; // 鬮｣蛹・ｽｽ・ｳ繝ｻ縺､ﾂ鬮｣雋ｻ・｣・ｰ郢晢ｽｻ繝ｻ・ｺ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬩幢ｽ｢繝ｻ・ｧ驛｢・ｧ霑壼生繝ｻ鬩搾ｽｵ繝ｻ・ｺ鬯ｮ・ｦ繝ｻ・ｪ驕ｯ・ｶ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ髫ｨ・ｳ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ髫ｴ謫ｾ・｣・ｰalse
	}
	return true; // 鬮ｯ・ｷ髣鯉ｽｨ繝ｻ・ｽ繝ｻ・ｨ鬮ｯ・ｷ繝ｻ・ｩ郢晢ｽｻ繝ｻ・｡鬮ｮ蠑ｱ繝ｻ繝ｻ・ｽ繝ｻ・ｻ鬩幢ｽ｢繝ｻ・ｧ鬮ｦ・ｮ陷ｷ・ｶ・つ陜｣・､繝ｻ・ｸ繝ｻ・ｺ驛｢譎｢・ｽ・ｻ髫ｨ・ｳ郢晢ｽｻ繝ｻ・ｹ繝ｻ・ｧ髮趣ｽｸ繝ｻ・ｲrue
}

std::wstring BattleController::GetPlayerPowerBoostText()const {
	std::wstring text;

	text = std::to_wstring(player_->GetBoostedPower());

	return text;
}

std::wstring BattleController::GetPlayerBlockText()const {
	std::wstring text;

	text = std::to_wstring(player_->GetBlock());

	return text;
}

int BattleController::CalcTotalIncomingDamage() const {
	int total = 0;
	if (!enemyMgr_) return 0;

	for (auto& enemy : enemyMgr_->GetEnemies()) {
		if (enemy.IsAlive()) {

			// 鬮ｫ・ｰ繝ｻ・ｨ郢晢ｽｻ繝ｻ・ｵ鬩搾ｽｵ繝ｻ・ｺ髫ｴ・ｴ繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｬ郢晢ｽｻ繝ｻ・｡鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｮ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｿ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｳ鬩搾ｽｵ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｫ鬯ｮ・ｯ繝ｻ・ｦ髯溷供・ｨ・ｯ鬲假ｽｬ鬮ｫ・ｰ繝ｻ・ｾ郢晢ｽｻ繝ｻ・ｻ鬮ｫ・ｰ繝ｻ・ｦ驛｢譎｢・ｽ・ｻ鬯ｲ蛛・ｽｽ・ｨ鬩幢ｽ｢繝ｻ・ｧ鬮ｮ蛹ｺ・ｧ・ｫ陟募ｮ｣霎ｧ陟・§諠ｧ郢晢ｽｻ繝ｻ・ｼ髯具ｽｹ繝ｻ・ｻ驍ｵ・ｺ陷･謫ｾ・ｽ・ｹ隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｼ鬩幢ｽ｢隴趣ｽ｢繝ｻ・ｽ繝ｻ・ｫ鬩幢ｽ｢隴寂・繝ｻ郢晢ｽｻ繝ｻ・ｭ髯晢ｽｲ繝ｻ・ｨ驕ｯ・ｶ繝ｻ・ｲ鬩搾ｽｵ繝ｻ・ｺ驛｢・ｧ郢晢ｽｻ繝ｻ・ｽ隶呵ｶ｣・ｽ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｰ鬩搾ｽｵ繝ｻ・ｺ鬮ｦ・ｮ陷ｻ・ｻ繝ｻ・ｼ郢晢ｽｻ繝ｻ・ｸ繝ｻ・ｺ郢晢ｽｻ繝ｻ・ｧ鬮ｮ荵晢ｽ・・・ｸ繝ｻ・ｷ郢晢ｽｻ繝ｻ・ｮ髫ｲ・､隲帷ｿｫ繝ｻ鬯ｨ・ｾ郢晢ｽｻ郢晢ｽｻ郢晢ｽｻ繝ｻ・ｼ驛｢譎｢・ｽ・ｻ
			total += enemy.GetIncomingDamage() - player_->GetBlock();
		}
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

void BattleController::ExecuteEnemyAction_(Enemy& enemy, const EnemyAction& action)
{
	if (action.type == "Attack") {
		if (!player_) {
			return;
		}
		enemy.PlayAttackAnim(player_->GetPos());
		player_->TriggerHitFlash(0.2f);
		player_->PlayDamageAnim();
		const int beforeBlock = player_->GetBlock();
		const int beforeHp = player_->GetHP();
		player_->Damage(action.value);
		const int actualHpDamage = beforeHp - player_->GetHP();
		PlayBlockReactionSE_(beforeBlock, player_->GetBlock(), actualHpDamage, action.value);
		if (action.value > 0) {
			SpawnDamagePopup(player_->GetPos(), actualHpDamage, true);
		}
	} else if (action.type == "Heal") {
		const int beforeHp = enemy.GetHP();
		enemy.Heal(action.value);
		if (enemy.GetHP() > beforeHp) {
			PlaySE_("SE_Heal");
		}
	} else if (action.type == "Block") {
		const int beforeBlock = enemy.GetBlock();
		enemy.AddBlock(action.value);
		PlayBlockGainSEIfIncreased_(beforeBlock, enemy.GetBlock());
	}
}

void BattleController::OnPlayerCardUsed_()
{
	if (!enemyMgr_) {
		return;
	}

	auto& enemies = enemyMgr_->GetEnemies();
	if (enemyActionCounts_.size() != enemies.size()) {
		enemyActionCounts_.assign(enemies.size(), 0);
	}
	if (enemyActedByCountThisTurn_.size() != enemies.size()) {
		enemyActedByCountThisTurn_.assign(enemies.size(), false);
	}

	for (size_t i = 0; i < enemies.size(); ++i) {
		if (!enemies[i].IsAlive() || enemyActedByCountThisTurn_[i]) {
			continue;
		}
		if (enemyActionCounts_[i] <= 0) {
			continue;
		}

		enemyActionCounts_[i]--;
		if (enemyActionCounts_[i] <= 0) {
			ExecuteEnemyAction_(enemies[i], enemies[i].GetBossAI().GetNextAction());
			enemyActedByCountThisTurn_[i] = true;
			enemyActionCounts_[i] = 0;
		}
	}
}

void BattleController::ExecutePendingAttack_(Enemy& targetEnemy)
{
	if (isPokerDamageTargeting_) {
		PlaySE_("SE_StrongAttack");
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
		CardInstance inst = hand_[idx];
		const CardDef* def = db_.Find(inst.defId);

		// 鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｳ鬩幢ｽ｢繝ｻ・ｧ郢晢ｽｻ繝ｻ・ｹ鬩幢ｽ｢隴主・讓溽ｹ晢ｽｻ陞ｳ螟ｲ・ｽ・ｱ繝ｻ・ｸ鬯ｩ蟶吶・繝ｻ・ｽ繝ｻ・ｲ郢晢ｽｻ繝ｻ・ｻ鬩搾ｽｵ繝ｻ・ｺ髯ｷ莨夲ｽｽ・ｱ驕ｯ・ｶ繝ｻ・ｻ鬮ｫ・ｰ郢晢ｽｻ驍・・・ｫ・ｰ郢晢ｽｻ繝ｻ・ｸ繝ｻ・ｺ髣包ｽｵ隴趣ｽ｢繝ｻ・ｽ髣・ｽｽ繝ｻ・ｱ繝ｻ・ｸ髯具ｽｹ繝ｻ・ｻ髫ｨ蛟･繝ｻ
		energy_ -= def->cost;
		PlaySE_("SE_CardPlay");
		PlayAttackSEForCard_(*def);
		auto usedCardView = handView_.ExtractCardAt(idx);
		hand_.erase(hand_.begin() + idx);

		handView_.Rebuild(hand_);

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

