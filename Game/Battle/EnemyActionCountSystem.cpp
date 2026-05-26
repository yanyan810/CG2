#include "EnemyActionCountSystem.h"

#include "enemy/Enemy.h"

#include <random>

void EnemyActionCountSystem::Clear()
{
    counts_.clear();
    actedFlags_.clear();
}

void EnemyActionCountSystem::Resize(std::size_t enemyCount)
{
    counts_.assign(enemyCount, 0);
    actedFlags_.assign(enemyCount, false);
}

void EnemyActionCountSystem::StartPlayerTurn(const std::vector<Enemy>& enemies)
{
    Resize(enemies.size());
    for (std::size_t i = 0; i < enemies.size(); ++i) {
        if (enemies[i].IsAlive()) {
            counts_[i] = RollActionCount_();
        }
    }
}

std::vector<std::size_t> EnemyActionCountSystem::OnPlayerCardUsed(const std::vector<Enemy>& enemies)
{
    EnsureSize_(enemies.size());

    std::vector<std::size_t> triggeredEnemies;
    for (std::size_t i = 0; i < enemies.size(); ++i) {
        if (!enemies[i].IsAlive() || IsActedByCount(i)) {
            continue;
        }
        if (counts_[i] <= 0) {
            continue;
        }

        counts_[i]--;
        if (counts_[i] <= 0) {
            counts_[i] = 0;
            triggeredEnemies.push_back(i);
        }
    }

    return triggeredEnemies;
}

int EnemyActionCountSystem::GetCount(std::size_t index) const
{
    if (index >= counts_.size()) {
        return 0;
    }
    return counts_[index];
}

bool EnemyActionCountSystem::IsActedByCount(std::size_t index) const
{
    return index < actedFlags_.size() && actedFlags_[index];
}

bool EnemyActionCountSystem::ShouldSkipEnemyTurn(std::size_t index) const
{
    return IsActedByCount(index);
}

void EnemyActionCountSystem::MarkActedByCount(std::size_t index)
{
    EnsureSize_(index + 1);
    actedFlags_[index] = true;
    counts_[index] = 0;
}

void EnemyActionCountSystem::EnsureSize_(std::size_t enemyCount)
{
    if (counts_.size() == enemyCount && actedFlags_.size() == enemyCount) {
        return;
    }
    Resize(enemyCount);
}

int EnemyActionCountSystem::RollActionCount_() const
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(3, 6);
    return dist(gen);
}