#include "PokerHandEvaluator.h"

#include <algorithm>
#include <array>

const char* PokerHandEvaluator::GetHandName(PokerHandRank rank)
{
	switch (rank) {
	case PokerHandRank::None:                 return "None";
	case PokerHandRank::OnePair:              return "One Pair";
	case PokerHandRank::TwoPair:              return "Two Pair";
	case PokerHandRank::ThreeOfAKind:         return "Three of a Kind";
	case PokerHandRank::Straight:             return "Straight";
	case PokerHandRank::Flush:                return "Flush";
	case PokerHandRank::FullHouse:            return "Full House";
	case PokerHandRank::FourOfAKind:          return "Four of a Kind";
	case PokerHandRank::StraightFlush:        return "Straight Flush";
	case PokerHandRank::RoyalStraightFlush:   return "Royal Straight Flush";
	default:                                  return "?";
	}
}

PokerHandResult PokerHandEvaluator::Evaluate(const std::vector<CardInstance>& cards)
{
	PokerHandResult result{};

	if (cards.size() != 5) {
		result.rank = PokerHandRank::None;
		result.power = 0;
		return result;
	}

	std::array<int, 14> countNumber{};
	std::array<int, 4> countSuit{};

	std::vector<int> numbers;
	numbers.reserve(5);

	for (const auto& c : cards) {
		if (c.number >= 1 && c.number <= 13) {
			countNumber[c.number]++;
			numbers.push_back(c.number);
		}

		int suitIndex = static_cast<int>(c.suit);
		if (suitIndex >= 0 && suitIndex < 4) {
			countSuit[suitIndex]++;
		}
	}

	std::sort(numbers.begin(), numbers.end());

	bool isFlush = false;
	for (int s : countSuit) {
		if (s == 5) {
			isFlush = true;
			break;
		}
	}

	bool isStraight = false;

	{
		bool straight = true;
		for (int i = 0; i < 4; ++i) {
			if (numbers[i] + 1 != numbers[i + 1]) {
				straight = false;
				break;
			}
		}
		if (straight) {
			isStraight = true;
		}
	}

	if (!isStraight) {
		std::vector<int> royal = numbers;
		std::sort(royal.begin(), royal.end());
		if (royal.size() == 5 &&
			royal[0] == 1 &&
			royal[1] == 10 &&
			royal[2] == 11 &&
			royal[3] == 12 &&
			royal[4] == 13) {
			isStraight = true;
		}
	}

	if (!isStraight) {
		std::vector<int> lowA = numbers;
		std::sort(lowA.begin(), lowA.end());
		if (lowA.size() == 5 &&
			lowA[0] == 1 &&
			lowA[1] == 2 &&
			lowA[2] == 3 &&
			lowA[3] == 4 &&
			lowA[4] == 5) {
			isStraight = true;
		}
	}

	int pairCount = 0;
	bool hasThree = false;
	bool hasFour = false;

	for (int n = 1; n <= 13; ++n) {
		if (countNumber[n] == 2) pairCount++;
		if (countNumber[n] == 3) hasThree = true;
		if (countNumber[n] == 4) hasFour = true;
	}

	bool isRoyal = (numbers[0] == 1 &&
		numbers[1] == 10 &&
		numbers[2] == 11 &&
		numbers[3] == 12 &&
		numbers[4] == 13);

	if (isStraight && isFlush && isRoyal) {
		result.rank = PokerHandRank::RoyalStraightFlush;
		result.power = 100;
	} else if (isStraight && isFlush) {
		result.rank = PokerHandRank::StraightFlush;
		result.power = 80;
	} else if (hasFour) {
		result.rank = PokerHandRank::FourOfAKind;
		result.power = 70;
	} else if (hasThree && pairCount == 1) {
		result.rank = PokerHandRank::FullHouse;
		result.power = 60;
	} else if (isFlush) {
		result.rank = PokerHandRank::Flush;
		result.power = 50;
	} else if (isStraight) {
		result.rank = PokerHandRank::Straight;
		result.power = 40;
	} else if (hasThree) {
		result.rank = PokerHandRank::ThreeOfAKind;
		result.power = 30;
	} else if (pairCount == 2) {
		result.rank = PokerHandRank::TwoPair;
		result.power = 20;
	} else if (pairCount == 1) {
		result.rank = PokerHandRank::OnePair;
		result.power = 10;
	} else {
		result.rank = PokerHandRank::None;
		result.power = 0;
	}

	return result;
}
