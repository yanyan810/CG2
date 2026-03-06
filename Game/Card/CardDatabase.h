#pragma once
#include <unordered_map>
#include "CardDef.h"
#include <string>

class CardDatabase {
public:
    void BuildSample(); // とりあえず数枚作る
    bool LoadFromJson(const std::string& path);
    const CardDef* Find(int id) const;

private:
    std::unordered_map<int, CardDef> defs_;
};