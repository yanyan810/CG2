#pragma once
#include <string>
#include "DeckDef.h"
#include "CardDatabase.h"


class CardDatabase;

class DeckLoader {
public:
    static bool LoadFromJson(const std::string& path, DeckDef& outDeck);
    static bool ValidateDeck(const DeckDef& deck, const CardDatabase& cardDb, std::string& outError);
    static bool SaveToJson(const std::string& path, const DeckDef& deck);

};