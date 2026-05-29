#include "CardEffectTextBuilder.h"
#include "CardDef.h"
#include "Poker/PokerHandEvaluator.h"

#include <Windows.h>
#include <cmath>

namespace {
	std::wstring Utf8ToWString_(const std::string& s)
	{
		if (s.empty()) {
			return L"";
		}

		int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
		std::wstring result(size - 1, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, result.data(), size);
		return result;
	}

	PokerHandRank ParsePokerRankString_(const std::string& s)
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

	std::wstring GetPokerRankText_(PokerHandRank rank)
	{
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
	}

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
		return std::to_wstring(static_cast<int>(std::lround(value)));
	}
}

std::wstring CardEffectTextBuilder::GetSubEffectTriggerText(SubEffectTrigger trigger)
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

std::wstring CardEffectTextBuilder::GetSubEffectConditionText(const CardSubEffectDef& sub)
{
	switch (sub.condition.type) {
	case SubEffectConditionType::ExactRank:
	{
		PokerHandRank rank = ParsePokerRankString_(sub.condition.rank);
		if (rank != PokerHandRank::None) {
			return GetPokerRankText_(rank) + L"の場合";
		}
		break;
	}

	case SubEffectConditionType::AtLeastRank:
	{
		PokerHandRank rank = ParsePokerRankString_(sub.condition.rank);
		if (rank != PokerHandRank::None) {
			return GetPokerRankText_(rank) + L"以上の場合";
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

std::wstring CardEffectTextBuilder::GetEffectValueText(const CardEffectDef& effect)
{
	if (!effect.valueText.empty()) {
		return Utf8ToWString_(effect.valueText) + L": " + FormatEffectValue_(effect);
	}

	if (effect.type == "Draw") {
		return L"ドロー: " + FormatEffectValue_(effect);
	}
	if (effect.type == "Damage") {
		return L"ダメージ: " + FormatEffectValue_(effect);
	}
	if (effect.type == "DamageAll") {
		return L"全体ダメージ: " + FormatEffectValue_(effect);
	}
	if (effect.type == "Heal") {
		return L"回復: " + FormatEffectValue_(effect);
	}
	if (effect.type == "Block") {
		return L"ブロック: " + FormatEffectValue_(effect);
	}
	if (effect.type == "PowerBoost") {
		return L"パワー: " + FormatEffectValue_(effect);
	}
	if (effect.type == "EnergyCharge") {
		return L"コスト回復: " + FormatEffectValue_(effect);
	}
	if (effect.type == "NextTurnAtkUp") {
		return L"次ターンATK UP: " + FormatEffectValue_(effect);
	}
	if (effect.type == "SelfDamage") {
		return L"自傷: " + FormatEffectValue_(effect);
	}

	return Utf8ToWString_(effect.type) + L": " + FormatEffectValue_(effect);
}

std::wstring CardEffectTextBuilder::GetBaseEffectSummaryText(const CardDef& def)
{
	if (!def.effects.empty()) {
		std::wstring text;

		for (size_t i = 0; i < def.effects.size(); ++i) {
			if (i > 0) {
				text += L"\n";
			}
			text += GetEffectValueText(def.effects[i]);
		}

		return text;
	}

	if (!def.desc.empty()) {
		return Utf8ToWString_(def.desc);
	}

	return L"なし";
}

std::wstring CardEffectTextBuilder::BuildPreviewCardDetailText(const CardDef& def)
{
	std::wstring text = L"基本効果:\n";
	if (!def.desc.empty()) {
		text += Utf8ToWString_(def.desc);
	} else {
		text += L"なし";
	}

	if (!def.subEffects.empty()) {
		text += L"\n\n";
		for (size_t i = 0; i < def.subEffects.size(); ++i) {
			const auto& sub = def.subEffects[i];
			if (i > 0) {
				text += L"\n\n";
			}

			std::wstring triggerText = GetSubEffectTriggerText(sub.trigger);
			if (!triggerText.empty()) {
				text += triggerText + L"\n";
			}

			std::wstring conditionText = GetSubEffectConditionText(sub);
			if (!conditionText.empty()) {
				text += conditionText + L"\n";
			}

			for (size_t j = 0; j < sub.effects.size(); ++j) {
				if (j > 0) text += L"\n";
				text += GetEffectValueText(sub.effects[j]);
			}
		}
	}

	return text;
}
