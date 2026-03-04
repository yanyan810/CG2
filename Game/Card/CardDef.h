#pragma once
#pragma once
#include <string>
#include <vector>

struct CardEffectDef {
    std::string type; // "Damage" / "Block" / "Draw" など
    int value = 0;
};

struct CardDef {
    int id = 0;
    std::string name;
    int cost = 0;

    // 3D見た目
    std::string frameModel;   // 共通でもOK
    std::string artModel;     // 共通でもOK（板ポリ）
    std::string artTex;       // カードごと

    std::string desc;
    std::vector<CardEffectDef> effects;
};