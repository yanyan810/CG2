#pragma once
#include <string>
#include <vector>

struct CardEffectDef {
    std::string type; // "Damage" / "Block" / "Draw" / "Heal" ...
    int value = 0;
    float valueFloat = 0.0f;
    bool valueIsFloat = false;

    // 将来用
    std::string valueText; // "Heart" など文字系を扱いたい時用
};

enum class SubEffectTrigger {
    OnPlayToField,
    OnTurnStartWithPoker,
    OnPokerSkillActivated,
};

enum class SubEffectConditionType {
    None,
    ExactRank,
    AtLeastRank,
    RankFamily,
};

struct SubEffectConditionDef {
    SubEffectConditionType type = SubEffectConditionType::None;

    // 文字で持っておくとJSON読み込みが楽
    std::string rank;      // "OnePair", "Flush", ...
    std::string family;    // "StraightFamily", "FlushFamily", "PairFamily"
};

struct CardSubEffectDef {
    SubEffectTrigger trigger = SubEffectTrigger::OnTurnStartWithPoker;
    SubEffectConditionDef condition;
    std::vector<CardEffectDef> effects;
};

struct CardDef {
    int id = 0;
    std::string name;
    int cost = 0;

    std::string frameModel;
    std::string artModel;
    std::string artTex;

    std::string desc;
    std::vector<CardEffectDef> effects;

    // 追加
    std::vector<CardSubEffectDef> subEffects;
};
