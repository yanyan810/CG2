#pragma once
#include<string>

struct CardDef;
enum class SubEffectTrigger;
struct CardEffectDef;
struct CardSubEffectDef;

class CardPreviewUI
{
public:

	enum class PokerHandRank {
		None = 0,
		OnePair,
		TwoPair,
		ThreeOfAKind,
		Straight,
		Flush,
		FullHouse,
		FourOfAKind,
		StraightFlush,
		RoyalStraightFlush,
	};

	static std::wstring GetPreviewCardDetailText(const CardDef* def);

private:

	static std::wstring GetSubEffectTriggerText(SubEffectTrigger trigger);

	static std::wstring GetSubEffectConditionText(const CardSubEffectDef& sub);

	static PokerHandRank ParsePokerRankString(const std::string& s);

	static std::wstring GetEffectValueText(const CardEffectDef& effect);
};

