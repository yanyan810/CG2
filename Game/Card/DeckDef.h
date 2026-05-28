#pragma once
#include <string>
#include <vector>

struct DeckEntryDef {
    int id = 0;
    int count = 0;
};

struct DeckDef {
    std::string name;
    std::string description;
    int minCards = 40;
    int maxCards = 52;
    int maxSameCard = 4;
    std::vector<DeckEntryDef> cards;
};