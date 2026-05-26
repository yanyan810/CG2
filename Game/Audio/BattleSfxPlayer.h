#pragma once

class Player;
struct CardDef;

class BattleSfxPlayer {
public:
    static void PlaySE(const char* soundId);
    static void PlayAttackSEForCard(const CardDef& def);
    static void PlayHealSEIfHpIncreased(const Player* player, int beforeHp);
    static void PlayBlockGainSEIfIncreased(int beforeBlock, int afterBlock);
    static void PlayBlockReactionSE(int beforeBlock, int afterBlock, int actualHpDamage, int attemptedDamage);
    static void PlayPowerChargeSE();

private:
    static const char* GetAttackSeIdForCard_(const CardDef& def);
};