#pragma once

#include <cstddef>
#include <vector>

#include "Card/CardInstance.h"

class BattleDeckZone {
public:
	void Clear();
	void SetDeck(const std::vector<CardInstance>& deck);
	void SetDeck(std::vector<CardInstance>&& deck);
	void ShuffleDeck();

	bool DrawOne(CardInstance& outCard, bool& reshuffledDiscard);
	std::vector<CardInstance> DrawCards(int count, bool& reshuffledDiscard);

	bool RemoveFirstFromDeckByDefId(int defId);
	void PushDeckBack(const CardInstance& card);
	bool RemoveHandAt(std::size_t index, CardInstance* removedCard = nullptr);

	void AddToDiscard(const CardInstance& card);
	void AddManyToDiscard(const std::vector<CardInstance>& cards);
	void ClearHand();
	void ClearDiscard();

	const std::vector<CardInstance>& GetDeck() const { return deck_; }
	const std::vector<CardInstance>& GetHand() const { return hand_; }
	const std::vector<CardInstance>& GetDiscard() const { return discard_; }

	std::size_t GetDeckCount() const { return deck_.size(); }
	std::size_t GetHandCount() const { return hand_.size(); }
	std::size_t GetDiscardCount() const { return discard_.size(); }

private:
	std::vector<CardInstance> deck_;
	std::vector<CardInstance> hand_;
	std::vector<CardInstance> discard_;
};
