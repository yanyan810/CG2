#include "BattleSfxPlayer.h"

#include "AudioManager.h"
#include "Card/CardDef.h"
#include "player/Player.h"

#include <string>

void BattleSfxPlayer::PlaySE(const char* soundId)
{
    if (!soundId || soundId[0] == '\0') {
        return;
    }
    AudioManager::GetInstance()->PlaySE(soundId);
}

void BattleSfxPlayer::PlayAttackSEForCard(const CardDef& def)
{
    if (const char* soundId = GetAttackSeIdForCard_(def)) {
        PlaySE(soundId);
    }
}

void BattleSfxPlayer::PlayHealSEIfHpIncreased(const Player* player, int beforeHp)
{
    if (player && player->GetHP() > beforeHp) {
        PlaySE("SE_Heal");
    }
}

void BattleSfxPlayer::PlayBlockGainSEIfIncreased(int beforeBlock, int afterBlock)
{
    if (afterBlock > beforeBlock) {
        PlaySE("SE_Block");
    }
}

void BattleSfxPlayer::PlayBlockReactionSE(int beforeBlock, int afterBlock, int actualHpDamage, int attemptedDamage)
{
    if (attemptedDamage <= 0 || beforeBlock <= 0) {
        return;
    }

    if (afterBlock <= 0) {
        PlaySE("SE_BlockBreak");
        return;
    }

    if (actualHpDamage <= 0) {
        PlaySE("SE_ShieldGuard");
    }
}

void BattleSfxPlayer::PlayPowerChargeSE()
{
    PlaySE("SE_PowerCharge");
}

const char* BattleSfxPlayer::GetAttackSeIdForCard_(const CardDef& def)
{
    const std::string& name = def.name;
    if (name == "Fireball") {
        return "SE_Fireball";
    }
    if (name == "Attack!" || name == "BloodyVengeance") {
        return "SE_NormalAttack";
    }
    if (name == "Power Shot" ||
        name == "Crush" ||
        name == "CrescentMoon" ||
        name == "OverClock" ||
        name == "ShieldBash" ||
        name == "GaeBolg" ||
        name == "Durandal") {
        return "SE_StrongAttack";
    }

    bool hasDamage = false;
    for (const auto& effect : def.effects) {
        if (effect.type == "DamageAll" ||
            effect.type == "DamageCrescent" ||
            effect.type == "DamageByBlock") {
            return "SE_StrongAttack";
        }
        if (effect.type == "Damage") {
            hasDamage = true;
        }
    }

    return hasDamage ? "SE_NormalAttack" : nullptr;
}