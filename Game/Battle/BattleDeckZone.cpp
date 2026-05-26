#include "BattleDeckZone.h"

#include <algorithm>
#include <random>
#include <utility>

void BattleDeckZone::Clear()
{
	deck_.clear();
	hand_.clear();
	discard_.clear();
}

void BattleDeckZone::SetDeck(const std::vector<CardInstance>& deck)
{
	deck_ = deck;
}

void BattleDeckZone::SetDeck(std::vector<CardInstance>&& deck)
{
	deck_ = std::move(deck);
}

void BattleDeckZone::ShuffleDeck()
{
	static std::random_device rd;
	static std::mt19937 mt(rd());
	std::shuffle(deck_.begin(), deck_.end(), mt);
}

bool BattleDeckZone::DrawOne(CardInstance& outCard, bool& reshuffledDiscard)
{
	reshuffledDiscard = false;

	if (deck_.empty()) {
		if (discard_.empty()) {
			return false;
		}

		deck_ = discard_;
		discard_.clear();
		ShuffleDeck();
		reshuffledDiscard = true;
	}

	if (deck_.empty()) {
		return false;
	}

	outCard = deck_.back();
	deck_.pop_back();
	hand_.push_back(outCard);
	return true;
}

std::vector<CardInstance> BattleDeckZone::DrawCards(int count, bool& reshuffledDiscard)
{
	reshuffledDiscard = false;

	std::vector<CardInstance> drawnCards;
	for (int i = 0; i < count; ++i) {
		CardInstance card{};
		bool reshuffledThisDraw = false;
		if (!DrawOne(card, reshuffledThisDraw)) {
			break;
		}

		reshuffledDiscard = reshuffledDiscard || reshuffledThisDraw;
		drawnCards.push_back(card);
	}

	return drawnCards;
}

bool BattleDeckZone::RemoveFirstFromDeckByDefId(int defId)
{
	auto it = std::find_if(deck_.begin(), deck_.end(),
		[defId](const CardInstance& card) {
			return card.defId == defId;
		});

	if (it == deck_.end()) {
		return false;
	}

	deck_.erase(it);
	return true;
}

void BattleDeckZone::PushDeckBack(const CardInstance& card)
{
	deck_.push_back(card);
}

bool BattleDeckZone::RemoveHandAt(std::size_t index, CardInstance* removedCard)
{
	if (index >= hand_.size()) {
		return false;
	}

	if (removedCard) {
		*removedCard = hand_[index];
	}
	hand_.erase(hand_.begin() + static_cast<std::ptrdiff_t>(index));
	return true;
}

void BattleDeckZone::AddToDiscard(const CardInstance& card)
{
	discard_.push_back(card);
}

void BattleDeckZone::AddManyToDiscard(const std::vector<CardInstance>& cards)
{
	discard_.insert(discard_.end(), cards.begin(), cards.end());
}

void BattleDeckZone::ClearHand()
{
	hand_.clear();
}

void BattleDeckZone::ClearDiscard()
{
	discard_.clear();
}
