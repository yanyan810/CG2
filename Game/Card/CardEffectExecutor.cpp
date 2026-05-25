#include "CardEffectExecutor.h"

#include "Audio/BattleSfxPlayer.h"
#include "Battle/BattleDeckZone.h"
#include "CardDatabase.h"
#include "CardDef.h"
#include "Enemy.h"
#include "Player.h"

void CardEffectExecutor::ApplyDraw(const Context& context, const CardEffectDef& effect)
{
	if (context.drawCards) {
		context.drawCards(effect.value);
	}
}

void CardEffectExecutor::ApplyBlock(const Context& context, const CardEffectDef& effect)
{
	if (context.player && effect.value > 0) {
		const int beforeBlock = context.player->GetBlock();
		context.player->AddBlock(effect.value);
		BattleSfxPlayer::PlayBlockGainSEIfIncreased(beforeBlock, context.player->GetBlock());
	}
}

void CardEffectExecutor::ApplyPowerBoost(const Context& context, const CardEffectDef& effect)
{
	if (context.player && effect.value > 0) {
		const int beforePower = context.player->GetBoostedPower();
		context.player->PowerBoost(effect.value);
		if (context.player->GetBoostedPower() > beforePower) {
			BattleSfxPlayer::PlayPowerChargeSE();
		}
	}
}

void CardEffectExecutor::ApplyNextTurnAtkUp(const Context& context, const CardEffectDef& effect)
{
	if (effect.value > 0) {
		BattleSfxPlayer::PlayPowerChargeSE();
	}
	if (context.nextTurnAtkUp) {
		*context.nextTurnAtkUp += effect.value;
	}
}

void CardEffectExecutor::ApplyHeal(const Context& context, const CardEffectDef& effect)
{
	if (context.player) {
		int beforeHp = context.player->GetHP();
		context.player->Heal(effect.value);
		BattleSfxPlayer::PlayHealSEIfHpIncreased(context.player, beforeHp);
	}
}

void CardEffectExecutor::ApplyHealByBlock(const Context& context, const CardEffectDef& effect)
{
	if (context.player) {
		int healAmount = context.player->GetBlock() * effect.value;
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
		int healAmount = count * effect.value;
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
		context.player->AddVampireHeal(effect.value);
	}
}

void CardEffectExecutor::ApplySelfDamage(const Context& context, const CardEffectDef& effect)
{
	if (context.player) {
		context.player->TriggerHitFlash(0.2f);
		context.player->PlayDamageAnim();
		context.player->Damage(effect.value);
	}
}

void CardEffectExecutor::ApplyPoisonAmplify(const Context& context, const CardEffectDef& effect)
{
	if (context.enemyMgr) {
		for (auto& e : context.enemyMgr->GetEnemies()) {
			if (e.IsAlive()) {
				if (e.GetBC() == Enemy::BadCondition::kPoison) {
					e.AmplifyBC(effect.value);
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
				e.DamageBC(effect.value);
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
				e.AddBC(effect.value);
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
					e.AddBC(effect.value);
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
				e.AddBC(effect.value);
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

	int blockAmount = count * effect.value;
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
				e.DamageBC(effect.value);
			}
		}
	}
}

void CardEffectExecutor::ApplyFrostDraw(const Context& context, int targetIndex)
{
	if (context.enemyMgr) {
		if (targetIndex >= 0 && targetIndex < context.enemyMgr->GetEnemies().size()) {
			auto& e = context.enemyMgr->GetEnemies()[targetIndex];
			if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kFrost) {
				if (context.drawCards) {
					context.drawCards(e.GetBCPoint() / 2);
				}
			}
		} else {
			for (auto& e : context.enemyMgr->GetEnemies()) {
				if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kFrost) {
					if (context.drawCards) {
						context.drawCards(e.GetBCPoint() / 2);
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
				e.SubtractBC(effect.value);
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
					e.AmplifyBC(effect.value);
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
