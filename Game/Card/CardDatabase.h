#pragma once
#include <unordered_map>
#include "CardDef.h"

class CardDatabase {
public:
    void BuildSample(); // とりあえず数枚作る
    const CardDef* Find(int id) const;

private:
    std::unordered_map<int, CardDef> defs_;
};