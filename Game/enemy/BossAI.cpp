#include "BossAI.h"
#include "Enemy.h"
#include <algorithm>
#include <fstream>
#include <random>
#include <nlohmann/json.hpp> 
#include <Windows.h>
using json = nlohmann::json;

namespace {
    bool IsSupportedActionType_(const std::string& type)
    {
        return type == "Attack" || type == "Block" || type == "Heal";
    }
}

void BossAI::Reset(int maxHP) {
    maxHP_ = std::max(1, maxHP);
}

// JSONからの読み込み
void BossAI::LoadPattern(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::string errorMsg = "[BossAI] ERROR: JSONファイルが見つかりません！探した場所: " + filePath + "\n";
        OutputDebugStringA(errorMsg.c_str());
        return;
    }
    json j;
    try {
        file >> j;
    } catch (...) {
        std::string errorMsg = "[BossAI] ERROR: JSON parse failed: " + filePath + "\n";
        OutputDebugStringA(errorMsg.c_str());
        return;
    }

    // 最大HPの読み込み
    if (j.contains("maxHp") && j["maxHp"].is_number_integer()) {
        maxHP_ = j["maxHp"].get<int>();
    }

    // 行動リストの読み込み
    actionList_.clear();
    if (j.contains("actions") && j["actions"].is_array()) {
        for (const auto& item : j["actions"]) {
            if (!item.is_object() ||
                !item.contains("type") ||
                !item.contains("value") ||
                !item.contains("name") ||
                !item["type"].is_string() ||
                !item["value"].is_number_integer() ||
                !item["name"].is_string()) {
                OutputDebugStringA("[BossAI] WARN: invalid action skipped.\n");
                continue;
            }

            EnemyAction action;
            action.type = item["type"].get<std::string>();
            action.value = item["value"].get<int>();
            action.name = item["name"].get<std::string>();
            if (!IsSupportedActionType_(action.type)) {
                std::string warnMsg = "[BossAI] WARN: unknown action type skipped: " + action.type + "\n";
                OutputDebugStringA(warnMsg.c_str());
                continue;
            }

            actionList_.push_back(action);
        }
    } else {
        OutputDebugStringA("[BossAI] WARN: actions array not found.\n");
    }
    std::string successMsg = "[BossAI] SUCCESS: JSONを読み込みました！技の数: " + std::to_string(actionList_.size()) + "\n";
    OutputDebugStringA(successMsg.c_str());
}

// ランダムに行動を選択する
void BossAI::DecideNextAction() {
    if (actionList_.empty()) {
        nextAction_ = { "Attack", 10, "通常攻撃" };
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, static_cast<int>(actionList_.size()) - 1);

    int randomIndex = dist(gen);
    nextAction_ = actionList_[randomIndex];
}


void BossAI::Update(Enemy& e, float dt) {
    // 現在のカードバトルではAIが勝手に動いたり攻撃したりすることは無いので、
    // ここは基本的に空っぽでOKです。
    // 将来、ターンごとの特殊な演出や、死んだ時のエフェクトを入れたい場合はここに書きます。
}
