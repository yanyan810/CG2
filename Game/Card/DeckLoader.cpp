#include "DeckLoader.h"
#include "externals/nlohmann/json.hpp"
#include <fstream>

using json = nlohmann::json;


#include <unordered_map>

bool DeckLoader::ValidateDeck(const DeckDef& deck, const CardDatabase& cardDb, std::string& outError)
{
    int total = 0;
    std::unordered_map<int, int> countById;

    for (const auto& e : deck.cards) {
        if (e.id <= 0) {
            outError = "カードIDが不正です";
            return false;
        }

        if (e.count <= 0) {
            outError = "カード枚数が不正です";
            return false;
        }

        if (!cardDb.Find(e.id)) {
            outError = "cards.json に存在しないカードIDがあります";
            return false;
        }

        countById[e.id] += e.count;
        if (countById[e.id] > deck.maxSameCard) {
            outError = "同じカードは最大4枚までです";
            return false;
        }

        total += e.count;
    }

    if (total < deck.minCards) {
        outError = "デッキ枚数が少なすぎます";
        return false;
    }

    if (total > deck.maxCards) {
        outError = "デッキ枚数が多すぎます";
        return false;
    }

    return true;
}

bool DeckLoader::LoadFromJson(const std::string& path, DeckDef& outDeck)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return false;
    }

    json root;
    ifs >> root;

    outDeck = {};

    outDeck.name = root.value("name", "");
    outDeck.description = root.value("description", "");
    outDeck.minCards = root.value("minCards", 40);
    outDeck.maxCards = root.value("maxCards", 52);
    outDeck.maxSameCard = root.value("maxSameCard", 4);

    if (root.contains("cards") && root["cards"].is_array()) {
        for (const auto& j : root["cards"]) {
            DeckEntryDef e{};
            e.id = j.value("id", 0);
            e.count = j.value("count", 0);
            outDeck.cards.push_back(e);
        }
    }

    return true;
}

bool DeckLoader::SaveToJson(const std::string& path, const DeckDef& deck)
{
    json root;
    root["name"] = deck.name;
    root["minCards"] = deck.minCards;
    root["maxCards"] = deck.maxCards;
    root["maxSameCard"] = deck.maxSameCard;
    root["cards"] = json::array();

    for (const auto& e : deck.cards) {
        root["cards"].push_back({
            { "id", e.id },
            { "count", e.count }
            });
    }

    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;

    ofs << root.dump(2);
    return true;
}