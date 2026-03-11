#pragma once
#include <cstdint>
#include <string>
#include <vector>

// 敵の1ターン分の行動データ
struct EnemyAction {
    std::string type;  // "Attack", "Block", "Heal" など
    int value;         // ダメージ量や回復量
    std::string name;  // UI表示用の技名
};

class Enemy;

// カードバトル用に超シンプルになった BossAI
class BossAI {
public:
    void Reset(int maxHP);
    void Update(Enemy& e, float dt);

    void LoadPattern(const std::string& filePath);
    EnemyAction GetRandomAction();

    int GetMaxHP() const { return maxHP_; }
private:
    int maxHP_ = 300;
    std::vector<EnemyAction> actionList_;
};