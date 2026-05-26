#pragma once

#include <cstddef>
#include <vector>

class Enemy;

class EnemyActionCountSystem {
public:
    void Clear();
    void Resize(std::size_t enemyCount);
    void StartPlayerTurn(const std::vector<Enemy>& enemies);
    std::vector<std::size_t> OnPlayerCardUsed(const std::vector<Enemy>& enemies);

    int GetCount(std::size_t index) const;
    bool IsActedByCount(std::size_t index) const;
    bool ShouldSkipEnemyTurn(std::size_t index) const;
    void MarkActedByCount(std::size_t index);

    const std::vector<int>& GetCounts() const { return counts_; }
    const std::vector<bool>& GetActedFlags() const { return actedFlags_; }

private:
    void EnsureSize_(std::size_t enemyCount);
    int RollActionCount_() const;

    std::vector<int> counts_;
    std::vector<bool> actedFlags_;
};