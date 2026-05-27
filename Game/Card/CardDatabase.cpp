#include "CardDatabase.h"
#include <fstream>
#include <cmath>
#include "externals/nlohmann/json.hpp"
#include "TextureManager.h"

using json = nlohmann::json;

static SubEffectTrigger ParseSubEffectTrigger(const std::string& s)
{
    if (s == "OnPlayToField") return SubEffectTrigger::OnPlayToField;
    if (s == "OnTurnStartWithPoker") return SubEffectTrigger::OnTurnStartWithPoker;
    if (s == "OnPokerSkillActivated") return SubEffectTrigger::OnPokerSkillActivated;
    return SubEffectTrigger::OnTurnStartWithPoker;
}

static SubEffectConditionType ParseSubEffectConditionType(const std::string& s)
{
    if (s == "ExactRank") return SubEffectConditionType::ExactRank;
    if (s == "AtLeastRank") return SubEffectConditionType::AtLeastRank;
    if (s == "RankFamily") return SubEffectConditionType::RankFamily;
    return SubEffectConditionType::None;
}

static CardEffectDef ReadCardEffectDef_(const json& jEffect)
{
    CardEffectDef effect{};
    effect.type = jEffect.value("type", "");

    if (jEffect.contains("value") && jEffect["value"].is_number()) {
        effect.valueFloat = jEffect["value"].get<float>();
        effect.value = static_cast<int>(std::lround(effect.valueFloat));
        effect.valueIsFloat = std::fabs(effect.valueFloat - static_cast<float>(effect.value)) > 0.0001f;
    } else {
        effect.value = jEffect.value("value", 0);
        effect.valueFloat = static_cast<float>(effect.value);
        effect.valueIsFloat = false;
    }

    effect.valueText = jEffect.value("valueText", "");
    return effect;
}

void CardDatabase::BuildSample() {
    defs_.clear();

    CardDef fire{};
    fire.id = 1;
    fire.name = "Fireball";
    fire.cost = 2;
    fire.frameModel = "cards/models/frame.obj";
    fire.artModel = "cards/models/art_plane.obj";
    fire.artTex = "resources/cards/art/fireball.png";
    fire.desc = "敵単体に10ダメージ";
    fire.effects.push_back({ "Damage", 10 });
    defs_[fire.id] = fire;

    CardDef draw{};
    draw.id = 2;
    draw.name = "Quick Draw";
    draw.cost = 1;
    draw.frameModel = "cards/models/frame.obj";
    draw.artModel = "cards/models/art_plane.obj";
    draw.artTex = "resources/cards/art/draw.png";
    draw.desc = "2枚引く";
    draw.effects.push_back({ "Draw", 2 });
    defs_[draw.id] = draw;
}

bool CardDatabase::LoadFromJson(const std::string& path)
{
    //defs_.clear();

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return false;
    }

    json root;
    ifs >> root;

    if (!root.contains("cards") || !root["cards"].is_array()) {
        return false;
    }

    for (const auto& jCard : root["cards"]) {
        CardDef def{};

        def.id = jCard.value("id", 0);
        def.name = jCard.value("name", "");
        def.cost = jCard.value("cost", 0);
        def.frameModel = jCard.value("frameModel", "");
        def.artModel = jCard.value("artModel", "");
        def.artTex = jCard.value("artTex", "");
        def.desc = jCard.value("desc", "");

		//TextureManager::GetInstance()->LoadTexture(def.artTex);

        if (jCard.contains("effects") && jCard["effects"].is_array()) {
            for (const auto& jEffect : jCard["effects"]) {
                def.effects.push_back(ReadCardEffectDef_(jEffect));
            }
        }

        if (jCard.contains("subEffects") && jCard["subEffects"].is_array()) {
            for (const auto& jSub : jCard["subEffects"]) {
                CardSubEffectDef sub{};

                sub.trigger = ParseSubEffectTrigger(jSub.value("trigger", ""));

                if (jSub.contains("condition") && jSub["condition"].is_object()) {
                    const auto& jCond = jSub["condition"];
                    sub.condition.type = ParseSubEffectConditionType(jCond.value("type", ""));
                    sub.condition.rank = jCond.value("rank", "");
                    sub.condition.family = jCond.value("family", "");
                }

                if (jSub.contains("effects") && jSub["effects"].is_array()) {
                    for (const auto& jEffect : jSub["effects"]) {
                        sub.effects.push_back(ReadCardEffectDef_(jEffect));
                    }
                }

                def.subEffects.push_back(sub);
                
            }
        }

        if (def.id != 0) {
            defs_[def.id] = def;
        }
        cardCount_++;
    }

    return true;
}

bool CardDatabase::LoadFromJsons(const std::vector<std::string>& paths)
{
    // データベース全体を初期化（ここでクリアする）
    defs_.clear();
    cardCount_ = 0;

    bool allSuccess = true;

    for (const auto& path : paths) {
        // 1ファイルずつ既存の読み込み関数に渡す

        if (!LoadFromJson(path)) {
            // どこかのファイルが読み込めなかった場合のログ出しなど（任意）
            allSuccess = false;
        }
    }

    return allSuccess;
}

const CardDef* CardDatabase::Find(int id) const {
    auto it = defs_.find(id);
    return (it == defs_.end()) ? nullptr : &it->second;
}
