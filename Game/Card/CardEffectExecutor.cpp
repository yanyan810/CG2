#include "CardEffectExecutor.h"

#include "Audio/BattleSfxPlayer.h"
#include "Battle/BattleDeckZone.h"
#include "CardDatabase.h"
#include "CardDef.h"
#include "Enemy.h"
#include "Player.h"

#include <algorithm>
#include <cmath>

namespace {
	float EffectValueFloat_(const CardEffectDef& effect)
	{
		if (effect.valueIsFloat) {
			return effect.valueFloat;
		}
		if (effect.valueFloat != 0.0f || effect.value == 0) {
			return effect.valueFloat;
		}
		return static_cast<float>(effect.value);
	}

	int EffectValueInt_(const CardEffectDef& effect)
	{
		return std::max(0, static_cast<int>(std::lround(EffectValueFloat_(effect))));
	}

	int ScaleEffectAmount_(int baseValue, const CardEffectDef& effect)
	{
		return std::max(0, static_cast<int>(std::lround(static_cast<float>(baseValue) * EffectValueFloat_(effect))));
	}
}

void CardEffectExecutor::ApplyDraw(const Context& context, const CardEffectDef& effect)
{
	if (context.drawCards) {
		context.drawCards(EffectValueInt_(effect));
	}
}

void CardEffectExecutor::ApplyBlock(const Context& context, const CardEffectDef& effect)
{
	const int value = EffectValueInt_(effect);
	if (context.player && value > 0) {
		const int beforeBlock = context.player->GetBlock();
		context.player->AddBlock(value);
		BattleSfxPlayer::PlayBlockGainSEIfIncreased(beforeBlock, context.player->GetBlock());
	}
}

void CardEffectExecutor::ApplyPowerBoost(const Context& context, const CardEffectDef& effect)
{
	const int value = EffectValueInt_(effect);
	if (context.player && value > 0) {
		const int beforePower = context.player->GetBoostedPower();
		context.player->PowerBoost(value);
		if (context.player->GetBoostedPower() > beforePower) {
			BattleSfxPlayer::PlayPowerChargeSE();
		}
	}
}

void CardEffectExecutor::ApplyNextTurnAtkUp(const Context& context, const CardEffectDef& effect)
{
	const int value = EffectValueInt_(effect);
	if (value > 0) {
		BattleSfxPlayer::PlayPowerChargeSE();
	}
	if (context.nextTurnAtkUp) {
		*context.nextTurnAtkUp += value;
	}
}

void CardEffectExecutor::ApplyHeal(const Context& context, const CardEffectDef& effect)
{
	if (context.player) {
		int beforeHp = context.player->GetHP();
		context.player->Heal(EffectValueInt_(effect));
		BattleSfxPlayer::PlayHealSEIfHpIncreased(context.player, beforeHp);
	}
}

void CardEffectExecutor::ApplyHealByBlock(const Context& context, const CardEffectDef& effect)
{
	if (context.player) {
		int healAmount = ScaleEffectAmount_(context.player->GetBlock(), effect);
		int beforeHp = context.player->GetHP();
		context.player->Heal(healAmount);
		BattleSfxPlayer::PlayHealSEIfHpIncreased(context.player, beforeHp);
	}
}

void CardEffectExecutor::ApplyHealByLowCostInHand(const Context& context, const CardEffectDef& effect)
{
	if (context.player && context.deckZone && context.cardDb) {
		int count = 0;
		for (const auto& cardInst : context.deckZone->GetHand()) {
			const CardDef* cDef = context.cardDb->Find(cardInst.defId);
			if (cDef && cDef->cost == 1) {
				count++;
			}
		}
		int healAmount = ScaleEffectAmount_(count, effect);
		if (healAmount > 0) {
			int beforeHp = context.player->GetHP();
			context.player->Heal(healAmount);
			BattleSfxPlayer::PlayHealSEIfHpIncreased(context.player, beforeHp);
		}
	}
}

void CardEffectExecutor::ApplyVampireBuff(const Context& context, const CardEffectDef& effect)
{
	if (context.player) {
		context.player->AddVampireHeal(EffectValueInt_(effect));
	}
}

void CardEffectExecutor::ApplySelfDamage(const Context& context, const CardEffectDef& effect)
{
	if (context.player) {
		context.player->TriggerHitFlash(0.2f);
		context.player->PlayDamageAnim();
		context.player->Damage(EffectValueInt_(effect));
	}
}

void CardEffectExecutor::ApplyPoisonAmplify(const Context& context, const CardEffectDef& effect)
{
	if (context.enemyMgr) {
		for (auto& e : context.enemyMgr->GetEnemies()) {
			if (e.IsAlive()) {
				if (e.GetBC() == Enemy::BadCondition::kPoison) {
					e.AmplifyBC(EffectValueFloat_(effect));
				}
			}
		}
	}
}

void CardEffectExecutor::ApplyPoisonDamage(const Context& context, const CardEffectDef& effect)
{
	if (context.enemyMgr) {
		for (auto& e : context.enemyMgr->GetEnemies()) {
			if (e.IsAlive()) {
				e.SetBC(Enemy::BadCondition::kPoison);
				e.DamageBC(EffectValueInt_(effect));
			}
		}
	}
}

void CardEffectExecutor::ApplyPoisonAll(const Context& context, const CardEffectDef& effect)
{
	if (context.enemyMgr) {
		for (auto& e : context.enemyMgr->GetEnemies()) {
			if (e.IsAlive()) {
				e.SetBC(Enemy::BadCondition::kPoison);
				e.AddBC(EffectValueInt_(effect));
			}
		}
	}
	if (context.player && context.player->GetPoisonDrawActive() && context.drawCards) {
		context.drawCards(1);
	}
}

void CardEffectExecutor::ApplyPoisonDraw(const Context& context, const CardEffectDef& effect)
{
	if (!context.player) {
		return;
	}

	bool isActivated = context.player->GetPoisonDrawActive();

	if (!isActivated) {
		context.player->SetPoisonDrawActive(true);
	} else {
		if (context.enemyMgr) {
			for (auto& e : context.enemyMgr->GetEnemies()) {
				if (e.IsAlive()) {
					e.SetBC(Enemy::BadCondition::kPoison);
					e.AddBC(EffectValueInt_(effect));
				}
			}
		}
		if (context.player->GetPoisonDrawActive() && context.drawCards) {
			context.drawCards(1);
		}
	}
}

void CardEffectExecutor::ApplyPoisonRemove(const Context& context)
{
	if (context.enemyMgr) {
		for (auto& e : context.enemyMgr->GetEnemies()) {
			if (e.IsAlive()) {
				e.RemoveBC();
			}
		}
	}
}

void CardEffectExecutor::ApplyFrostAll(const Context& context, const CardEffectDef& effect)
{
	if (context.enemyMgr) {
		for (auto& e : context.enemyMgr->GetEnemies()) {
			if (e.IsAlive()) {
				e.SetBC(Enemy::BadCondition::kFrost);
				e.AddBC(EffectValueInt_(effect));
			}
		}
	}
}

void CardEffectExecutor::ApplyFrostBlock(const Context& context, const CardEffectDef& effect)
{
	int count = 0;

	if (context.enemyMgr) {
		for (auto& e : context.enemyMgr->GetEnemies()) {
			if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kFrost) {
				count += e.GetBCPoint();
			}
		}
	}

	int blockAmount = ScaleEffectAmount_(count, effect);
	if (blockAmount > 0 && context.player) {
		int beforeBlock = context.player->GetBlock();
		context.player->AddBlock(blockAmount);
		BattleSfxPlayer::PlayBlockGainSEIfIncreased(beforeBlock, context.player->GetBlock());
	}
}

void CardEffectExecutor::ApplyFrostDamage(const Context& context, const CardEffectDef& effect)
{
	if (context.enemyMgr) {
		for (auto& e : context.enemyMgr->GetEnemies()) {
			if (e.IsAlive()) {
				e.DamageBC(EffectValueInt_(effect));
			}
		}
	}
}

void CardEffectExecutor::ApplyFrostDraw(const Context& context, const CardEffectDef& effect, int targetIndex)
{
	if (context.enemyMgr) {
		if (targetIndex >= 0 && targetIndex < context.enemyMgr->GetEnemies().size()) {
			auto& e = context.enemyMgr->GetEnemies()[targetIndex];
			if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kFrost) {
				if (context.drawCards) {
					context.drawCards(ScaleEffectAmount_(e.GetBCPoint(), effect));
				}
			}
		} else {
			for (auto& e : context.enemyMgr->GetEnemies()) {
				if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kFrost) {
					if (context.drawCards) {
						context.drawCards(ScaleEffectAmount_(e.GetBCPoint(), effect));
					}
					break;
				}
			}
		}
	}
}

void CardEffectExecutor::ApplyFrostSubtract(const Context& context, const CardEffectDef& effect)
{
	if (context.enemyMgr) {
		for (auto& e : context.enemyMgr->GetEnemies()) {
			if (e.IsAlive()) {
				e.SubtractBC(EffectValueInt_(effect));
			}
		}
	}
}

void CardEffectExecutor::ApplyFrostAmplify(const Context& context, const CardEffectDef& effect)
{
	if (context.enemyMgr) {
		for (auto& e : context.enemyMgr->GetEnemies()) {
			if (e.IsAlive()) {
				if (e.GetBC() == Enemy::BadCondition::kFrost) {
					e.AmplifyBC(EffectValueFloat_(effect));
				}
			}
		}
	}
}

void CardEffectExecutor::ApplyChangeNumber(const CardEffectDef& effect)
{
	(void)effect;
}

void CardEffectExecutor::ApplyChangeSuit(const CardEffectDef& effect)
{
	(void)effect;
}
