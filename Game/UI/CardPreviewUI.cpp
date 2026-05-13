#include "CardPreviewUI.h"

#include<Windows.h>

#include"CardDef.h"

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

std::wstring CardPreviewUI::GetPreviewCardDetailText(const CardDef* def)
{
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

std::wstring CardPreviewUI::GetSubEffectTriggerText(SubEffectTrigger trigger)
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

CardPreviewUI::PokerHandRank CardPreviewUI::ParsePokerRankString(const std::string& s)
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


std::wstring CardPreviewUI::GetSubEffectConditionText(const CardSubEffectDef& sub)
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
		PokerHandRank rank = ParsePokerRankString(sub.condition.rank);
		if (rank != PokerHandRank::None) {
			return rankToText(rank) + L"の場合";
		}
		break;
	}

	case SubEffectConditionType::AtLeastRank:
	{
		PokerHandRank rank = ParsePokerRankString(sub.condition.rank);
		if (rank != PokerHandRank::None) {
			return rankToText(rank) + L"以上の場合";
		}
		break;
	}

	case SubEffectConditionType::RankFamily:
	{
		if (sub.condition.family == "StraightFamily") return L"ストレート系の場合";
		if (sub.condition.family == "FlushFamily")    return L"フラッシュ系の場合";
		if (sub.condition.family == "PairFamily")     return L"ペア系の場合";
		return L"役条件あり";
	}
	default:
		break;
	}

	return L"";
}

std::wstring CardPreviewUI::GetEffectValueText(const CardEffectDef& effect)
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
