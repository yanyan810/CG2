#pragma once

#include "CardInstance.h"

#include <vector>

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

struct PokerHandResult {
	PokerHandRank rank = PokerHandRank::None;
	int power = 0;
};

class PokerHandEvaluator {
public:
	static PokerHandResult Evaluate(const std::vector<CardInstance>& cards);
	static const char* GetHandName(PokerHandRank rank);
};
