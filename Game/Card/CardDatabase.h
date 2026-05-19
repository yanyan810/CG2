#pragma once
#include <unordered_map>
#include "CardDef.h"
#include <string>
#include <vector> // 追加

class CardDatabase {
public:
    void BuildSample();

    // 単一ファイルの読み込みも残しておくと便利（互換性のため）
    bool LoadFromJson(const std::string& path);

    // ★追加：複数のJSONファイルをまとめて読み込む関数
    bool LoadFromJsons(const std::vector<std::string>& paths);

    const CardDef* Find(int id) const;
    int GetCardCount() const { return cardCount_; }

private:
    std::unordered_map<int, CardDef> defs_;
    int cardCount_ = 0;
};