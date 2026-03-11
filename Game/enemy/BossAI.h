#pragma once
#include <cstdint>

class Enemy;

// カードバトル用に超シンプルになった BossAI
class BossAI {
public:
    void Reset(int maxHP);
    void Update(Enemy& e, float dt);
    int GetMaxHP() const { return maxHP_; }
private:
    int maxHP_ = 300;
};