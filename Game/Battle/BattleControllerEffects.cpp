#include "BattleController.h"
#include "Battle/BattleControllerShared.h"
#include "Battle/BattleFieldViewController.h"
#include "Battle/BattleDebugImGui.h"
#include "Battle/BattleRenderView.h"
#include "Battle/BattleInfoTextProvider.h"
#include "Battle/BattleCardInputController.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include "GameApp.h"
#include "WinApp.h"
#include <Windows.h>
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "MathStruct.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <set>
#include <random>
#include <filesystem>
#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include "Player.h"
#include "Enemy.h"

#include "Card/CardEffectTextBuilder.h"
#include "Card/CardEffectExecutor.h"
#include "FieldUi.h"
#include "Audio/BattleSfxPlayer.h"
#include "Camera.h"
#include "ModelParticleManager.h"
#include "Poker/PokerChoiceQuery.h"
#include "Poker/PokerChoiceController.h"
#include "Poker/PokerChoiceTextBuilder.h"

using namespace BattleControllerDetail;
Enemy* BattleController::ResolveTargetEnemy_(int targetIndex) const
{
	if (!enemyMgr_) {
		return nullptr;
	}

	auto& enemies = enemyMgr_->GetEnemies();
	if (targetIndex >= 0 && targetIndex < static_cast<int>(enemies.size())) {
		return &enemies[static_cast<std::size_t>(targetIndex)];
	}

	for (auto& enemy : enemies) {
		if (enemy.IsAlive()) {
			return &enemy;
		}
	}

	return nullptr;
}

void BattleController::ApplyFrostBiteToEnemyIfActive_(Enemy& enemy)
{
	if (player_ && player_->GetFrostBiteActive()) {
		enemy.SetBC(Enemy::BadCondition::kFrost);
		enemy.AddBC(1);
		EmitFrostAppliedEffect_(enemy, 1, false);
	}
}

void BattleController::ApplyDamageEffect_(const CardEffectDef& effect, int targetIndex, bool applyAttackBuff)
{
	if (enemyMgr_) {
		const bool hasExplicitTarget =
			targetIndex >= 0 && targetIndex < static_cast<int>(enemyMgr_->GetEnemies().size());
		Enemy* targetEnemy = ResolveTargetEnemy_(targetIndex);
		if (!targetEnemy || !targetEnemy->IsAlive()) return;

		int totalDamage = applyAttackBuff ? CalcFinalAttackDamage_(EffectValueInt_(effect)) : EffectValueInt_(effect);

		if (hasExplicitTarget && targetEnemy->GetBC() == Enemy::BadCondition::kFrost) {
			totalDamage += targetEnemy->GetBCPoint();
		}

		if (hasExplicitTarget) {
			ApplyFrostBiteToEnemyIfActive_(*targetEnemy);
		}

		const int actualDamage = ApplyDamageToEnemy_(*targetEnemy, totalDamage);
		if (!hasExplicitTarget) {
			ApplyFrostBiteToEnemyIfActive_(*targetEnemy);
		}
		if (totalDamage > 0) SpawnDamagePopup(targetEnemy->GetPos(), actualDamage, false);

	}
}

void BattleController::ApplyDamageCrescentEffect_(const CardEffectDef& effect, int targetIndex, bool applyAttackBuff)
{
	if (enemyMgr_) {
		int baseVal = EffectValueInt_(effect);
		if (playerTurnCount_ % 2 != 0) {
			baseVal += 3;
		}

		int finalDamage = applyAttackBuff ? CalcFinalAttackDamage_(baseVal) : std::max(0, baseVal);

		Enemy* targetEnemy = ResolveTargetEnemy_(targetIndex);
		if (targetEnemy && targetEnemy->IsAlive()) {
			const int actualDamage = ApplyDamageToEnemy_(*targetEnemy, finalDamage);
			if (finalDamage > 0) SpawnDamagePopup(targetEnemy->GetPos(), actualDamage, false);
			ApplyFrostBiteToEnemyIfActive_(*targetEnemy);
		}

	}
}

void BattleController::ApplyDamageByBlockEffect_(const CardEffectDef& effect, int targetIndex, bool applyAttackBuff)
{
	if (enemyMgr_) {
		int baseVal = ScaleEffectAmount_(player_ ? player_->GetBlock() : 0, effect);
		int finalDamage = applyAttackBuff ? CalcFinalAttackDamage_(baseVal) : baseVal;
		Enemy* targetEnemy = ResolveTargetEnemy_(targetIndex);
		if (targetEnemy && targetEnemy->IsAlive()) {
			const int actualDamage = ApplyDamageToEnemy_(*targetEnemy, finalDamage);
			if (finalDamage > 0) SpawnDamagePopup(targetEnemy->GetPos(), actualDamage, false);
			ApplyFrostBiteToEnemyIfActive_(*targetEnemy);
		}

	}
}

void BattleController::ApplyDamageAllEffect_(const CardEffectDef& effect, bool applyAttackBuff)
{
	if (enemyMgr_ && player_) {
		int totalDamage = applyAttackBuff ? CalcFinalAttackDamage_(EffectValueInt_(effect)) : EffectValueInt_(effect);

		player_->PlayAttackAnimWithEffect(player_->GetPos(), -1);

		int hitCount = 0;
		for (auto& e : enemyMgr_->GetEnemies()) {
			if (e.IsAlive()) {
				e.TriggerHitFlash(0.2f);
				e.PlayDamageAnim();
				const int beforeBlock = e.GetBlock();
				const int actualDamage = e.Damage(totalDamage);
				BattleSfxPlayer::PlayBlockReactionSE(beforeBlock, e.GetBlock(), actualDamage, totalDamage);
				if (totalDamage > 0) {
					SpawnDamagePopup(e.GetPos(), actualDamage, false);
				}
				ApplyFrostBiteToEnemyIfActive_(e);
				hitCount++;
			}
		}

		if (hitCount > 0 && player_->GetVampireHeal() > 0) {
			int beforeHp = player_->GetHP();
			player_->Heal(player_->GetVampireHeal() * hitCount);
			BattleSfxPlayer::PlayHealSEIfHpIncreased(player_, beforeHp);
		}

	}
}

void BattleController::ApplyDrawEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.drawCards = [this](int count) {
		DrawCards_(count);
	};
	CardEffectExecutor::ApplyDraw(context, effect);
}

void BattleController::ApplyBlockEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.player = player_;
	CardEffectExecutor::ApplyBlock(context, effect);
}

void BattleController::ApplyPowerBoostEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.player = player_;
	CardEffectExecutor::ApplyPowerBoost(context, effect);
}

void BattleController::ApplyNextTurnAtkUpEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.nextTurnAtkUp = &nextTurnAtkUp_;
	CardEffectExecutor::ApplyNextTurnAtkUp(context, effect);
}

void BattleController::ApplyHealEffect_(const CardEffectDef& effect)
{
	if (!player_) {
		return;
	}
	int beforeHp = player_->GetHP();
	player_->Heal(EffectValueInt_(effect));
	BattleSfxPlayer::PlayHealSEIfHpIncreased(player_, beforeHp);
}

void BattleController::ApplyHealByBlockEffect_(const CardEffectDef& effect)
{
	if (!player_) {
		return;
	}
	int healAmount = ScaleEffectAmount_(player_->GetBlock(), effect);
	int beforeHp = player_->GetHP();
	player_->Heal(healAmount);
	BattleSfxPlayer::PlayHealSEIfHpIncreased(player_, beforeHp);
}

void BattleController::ApplyHealByLowCostInHandEffect_(const CardEffectDef& effect)
{
	if (!player_) {
		return;
	}

	int count = 0;
	for (const auto& cardInst : deckZone_.GetHand()) {
		const CardDef* cDef = db_.Find(cardInst.defId);
		if (cDef && cDef->cost == 1) {
			count++;
		}
	}

	const int healAmount = ScaleEffectAmount_(count, effect);
	if (healAmount > 0) {
		int beforeHp = player_->GetHP();
		player_->Heal(healAmount);
		BattleSfxPlayer::PlayHealSEIfHpIncreased(player_, beforeHp);
	}
}

void BattleController::ApplyVampireBuffEffect_(const CardEffectDef& effect)
{
	if (player_) {
		player_->AddVampireHeal(EffectValueInt_(effect));
	}
}

void BattleController::ApplySelfDamageEffect_(const CardEffectDef& effect)
{
	if (player_) {
		player_->TriggerHitFlash(0.2f);
		player_->PlayDamageAnim();
		player_->Damage(EffectValueInt_(effect));
	}
}

void BattleController::ApplyPoisonEffect_(const CardEffectDef& effect, int targetIndex)
{
	if (enemyMgr_) {
		if (targetIndex >= 0 && targetIndex < enemyMgr_->GetEnemies().size()) {
			auto& e = enemyMgr_->GetEnemies()[targetIndex];
			if (e.IsAlive()) {
				e.SetBC(Enemy::BadCondition::kPoison);
				const int value = EffectValueInt_(effect);
				e.AddBC(value);
				EmitPoisonAppliedEffect_(e, value);
			}
		} else {
			for (auto& e : enemyMgr_->GetEnemies()) {
				if (e.IsAlive()) {
					e.SetBC(Enemy::BadCondition::kPoison);
					const int value = EffectValueInt_(effect);
					e.AddBC(value);
					EmitPoisonAppliedEffect_(e, value);
					break;
				}
			}
		}
	}
	if (player_->GetPoisonDrawActive()) {
		DrawCards_(1);
	}
}

void BattleController::ApplyPoisonAllEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.player = player_;
	context.enemyMgr = enemyMgr_;
	context.drawCards = [this](int count) {
		DrawCards_(count);
	};
	CardEffectExecutor::ApplyPoisonAll(context, effect);
	if (enemyMgr_) {
		for (auto& e : enemyMgr_->GetEnemies()) {
			if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kPoison) {
				EmitPoisonAppliedEffect_(e, EffectValueInt_(effect));
			}
		}
	}
}

void BattleController::ApplyPoisonAmplifyEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyPoisonAmplify(context, effect);
	if (enemyMgr_) {
		for (auto& e : enemyMgr_->GetEnemies()) {
			if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kPoison) {
				EmitPoisonAppliedEffect_(e, e.GetBCPoint());
			}
		}
	}
}

void BattleController::ApplyPoisonDamageEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyPoisonDamage(context, effect);
	if (enemyMgr_) {
		for (auto& e : enemyMgr_->GetEnemies()) {
			if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kPoison) {
				EmitPoisonAppliedEffect_(e, EffectValueInt_(effect));
			}
		}
	}
}

void BattleController::ApplyPoisonDrawEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.player = player_;
	context.enemyMgr = enemyMgr_;
	context.drawCards = [this](int count) {
		DrawCards_(count);
	};
	CardEffectExecutor::ApplyPoisonDraw(context, effect);
	if (enemyMgr_) {
		for (auto& e : enemyMgr_->GetEnemies()) {
			if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kPoison) {
				EmitPoisonAppliedEffect_(e, EffectValueInt_(effect));
			}
		}
	}
}

void BattleController::ApplyPoisonRemoveEffect_()
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyPoisonRemove(context);
}

void BattleController::ApplyPoisonHealEffect_(const CardEffectDef& effect)
{
	int poisonTotal = 0;

	if (enemyMgr_) {
		for (auto& e : enemyMgr_->GetEnemies()) {
			if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kPoison) {
				e.SetBC(Enemy::BadCondition::kPoison);
				poisonTotal += e.GetBCPoint();
			}
		}
	}

	const int healAmount = ScaleEffectAmount_(poisonTotal, effect);
	int beforeHp = player_->GetHP();
	player_->Heal(healAmount);
	BattleSfxPlayer::PlayHealSEIfHpIncreased(player_, beforeHp);
}

void BattleController::ApplyFrostEffect_(const CardEffectDef& effect, int targetIndex)
{
	if (enemyMgr_) {
		if (targetIndex >= 0 && targetIndex < enemyMgr_->GetEnemies().size()) {
			auto& e = enemyMgr_->GetEnemies()[targetIndex];
			if (e.IsAlive()) {
				e.SetBC(Enemy::BadCondition::kFrost);
				const int value = EffectValueInt_(effect);
				e.AddBC(value);
				EmitFrostAppliedEffect_(e, value);
			}
		} else {
			for (auto& e : enemyMgr_->GetEnemies()) {
				if (e.IsAlive()) {
					e.SetBC(Enemy::BadCondition::kFrost);
					const int value = EffectValueInt_(effect);
					e.AddBC(value);
					EmitFrostAppliedEffect_(e, value);
					break;
				}
			}
		}
	}
}

void BattleController::ApplyFrostAllEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyFrostAll(context, effect);
	if (enemyMgr_) {
		for (auto& e : enemyMgr_->GetEnemies()) {
			if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kFrost) {
				EmitFrostAppliedEffect_(e, EffectValueInt_(effect));
			}
		}
	}
}

void BattleController::ApplyFrostBlockEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.player = player_;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyFrostBlock(context, effect);
}

void BattleController::ApplyFrostDamageEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyFrostDamage(context, effect);
	if (enemyMgr_) {
		for (auto& e : enemyMgr_->GetEnemies()) {
			if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kFrost) {
				EmitFrostAppliedEffect_(e, e.GetBCPoint());
			}
		}
	}
}

void BattleController::ApplyFrostSubtractEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyFrostSubtract(context, effect);
}

void BattleController::ApplyFrostBiteEffect_(const CardEffectDef& effect)
{
	bool isActivated = player_->GetFrostBiteActive();

	if (!isActivated) {
		player_->SetFrostBiteActive(true);
	} else {
		if (enemyMgr_) {
			for (auto& e : enemyMgr_->GetEnemies()) {
				if (e.IsAlive()) {
					e.SetBC(Enemy::BadCondition::kFrost);
					const int value = EffectValueInt_(effect);
					e.AddBC(value);
					EmitFrostAppliedEffect_(e, value);
				}
			}
		}
	}
}

void BattleController::ApplyFrostAmplifyEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	CardEffectExecutor::ApplyFrostAmplify(context, effect);
	if (enemyMgr_) {
		for (auto& e : enemyMgr_->GetEnemies()) {
			if (e.IsAlive() && e.GetBC() == Enemy::BadCondition::kFrost) {
				EmitFrostAppliedEffect_(e, e.GetBCPoint());
			}
		}
	}
}

void BattleController::ApplyChangeNumberEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::ApplyChangeNumber(effect);
}

void BattleController::ApplyChangeSuitEffect_(const CardEffectDef& effect)
{
	CardEffectExecutor::ApplyChangeSuit(effect);
}

void BattleController::ApplyFrostDrawEffect_(const CardEffectDef& effect, int targetIndex)
{
	CardEffectExecutor::Context context;
	context.enemyMgr = enemyMgr_;
	context.drawCards = [this](int count) {
		DrawCards_(count);
	};
	CardEffectExecutor::ApplyFrostDraw(context, effect, targetIndex);
}

void BattleController::ApplyEffectsList_(const std::vector<CardEffectDef>& effects, int targetIndex, bool applyAttackBuff)
{
	for (const auto& effect : effects) {
		if (effect.type == "Draw") {
			ApplyDrawEffect_(effect);

		} else if (effect.type == "Damage") {
			ApplyDamageEffect_(effect, targetIndex, applyAttackBuff);

		} else if (effect.type == "DamageCrescent") {
			ApplyDamageCrescentEffect_(effect, targetIndex, applyAttackBuff);

		} else if (effect.type == "DamageByBlock") {
			ApplyDamageByBlockEffect_(effect, targetIndex, applyAttackBuff);

		} else if (effect.type == "Block") {
			ApplyBlockEffect_(effect);

		} else if (effect.type == "DamageAll") {
			ApplyDamageAllEffect_(effect, applyAttackBuff);

		} else if (effect.type == "PowerBoost") {
			ApplyPowerBoostEffect_(effect);
		} else if (effect.type == "NextTurnAtkUp") {
			ApplyNextTurnAtkUpEffect_(effect);

		} else if (effect.type == "Heal") {
			ApplyHealEffect_(effect);

		} else if (effect.type == "HealByBlock") {
			ApplyHealByBlockEffect_(effect);
		} else if (effect.type == "HealByLowCostInHand") {
			ApplyHealByLowCostInHandEffect_(effect);
		} else if (effect.type == "VampireBuff") {
			ApplyVampireBuffEffect_(effect);
		} else if (effect.type == "SelfDamage") {
			ApplySelfDamageEffect_(effect);

		} else if (effect.type == "Poison") {
			ApplyPoisonEffect_(effect, targetIndex);
		} else if (effect.type == "PoisonAll") {
			ApplyPoisonAllEffect_(effect);
		} else if (effect.type == "PoisonAmplify") {
			ApplyPoisonAmplifyEffect_(effect);
		} else if (effect.type == "PoisonDamage") {
			ApplyPoisonDamageEffect_(effect);
		} else if (effect.type == "PoisonDraw") {
			ApplyPoisonDrawEffect_(effect);

		} else if (effect.type == "PoisonRemove") {
			ApplyPoisonRemoveEffect_();

		} else if (effect.type == "PoisonHeal") {
			ApplyPoisonHealEffect_(effect);

		} else if (effect.type == "Frost") {
			ApplyFrostEffect_(effect, targetIndex);

		} else if (effect.type == "FrostAll") {
			ApplyFrostAllEffect_(effect);

		} else if (effect.type == "FrostBlock") {
			ApplyFrostBlockEffect_(effect);

		} else if (effect.type == "FrostDamage") {
			ApplyFrostDamageEffect_(effect);

		} else if (effect.type == "FrostSubtract") {
			ApplyFrostSubtractEffect_(effect);

		} else if (effect.type == "FrostBite") {
			ApplyFrostBiteEffect_(effect);

		} else if (effect.type == "FrostAmplify") {
			ApplyFrostAmplifyEffect_(effect);

		} else if (effect.type == "FrostDraw") {
			ApplyFrostDrawEffect_(effect, targetIndex);

		} else if (effect.type == "ChangeNumber") {
			ApplyChangeNumberEffect_(effect);

		} else if (effect.type == "ChangeSuit") {
			ApplyChangeSuitEffect_(effect);
		}
	}
}

void BattleController::ApplyCardEffects_(const CardDef& def, int targetIndex)
{
	ApplyEffectsList_(def.effects, targetIndex, true);
}

BattleController::PokerHandRank BattleController::ParsePokerRankString_(const std::string& s) const
{
	if (s == "OnePair") return PokerHandRank::OnePair;
	if (s == "TwoPair") return PokerHandRank::TwoPair;
	if (s == "ThreeOfAKind") return PokerHandRank::ThreeOfAKind;
	if (s == "Straight") return PokerHandRank::Straight;
	if (s == "Flush") return PokerHandRank::Flush;
	if (s == "FullHouse") return PokerHandRank::FullHouse;
	if (s == "FourOfAKind") return PokerHandRank::FourOfAKind;
	if (s == "StraightFlush") return PokerHandRank::StraightFlush;
	if (s == "RoyalStraightFlush") return PokerHandRank::RoyalStraightFlush;
	return PokerHandRank::None;
}

int BattleController::CalcFinalAttackDamage_(int baseDamage) const
{
	int total = baseDamage;

	if (player_) {
		total += player_->GetBoostedPower();
	} else if (useDebugPreviewBuff_) {
		total += debugPreviewPowerBoost_;
	}

	if (player_) {
		total += currentTurnAtkUp_;
	} else if (useDebugPreviewBuff_) {
		total += debugPreviewCurrentTurnAtkUp_;
	} else {
		total += currentTurnAtkUp_;
	}

	if (total < 0) {
		total = 0;
	}

	return total;
}

int BattleController::GetDisplayEffectValue(const CardEffectDef& effect, bool applyAttackBuff) const
{
	if (!applyAttackBuff) {
		if (effect.type == "DamageCrescent") {
			int value = EffectValueInt_(effect);
			if (playerTurnCount_ % 2 != 0) {
				value += 3;
			}
			return value;
		}
		if (effect.type == "DamageByBlock") {
			return ScaleEffectAmount_(player_ ? player_->GetBlock() : 0, effect);
		}
		return EffectValueInt_(effect);
	}

	if (effect.type == "Damage") {
		return CalcFinalAttackDamage_(EffectValueInt_(effect));
	}

	if (effect.type == "DamageAll") {
		return CalcFinalAttackDamage_(EffectValueInt_(effect));
	}

	if (effect.type == "DamageCrescent") {
		int value = EffectValueInt_(effect);
		if (playerTurnCount_ % 2 != 0) {
			value += 3;
		}
		return CalcFinalAttackDamage_(value);
	}

	if (effect.type == "DamageByBlock") {
		int value = ScaleEffectAmount_(player_ ? player_->GetBlock() : 0, effect);
		return CalcFinalAttackDamage_(value);
	}

	return EffectValueInt_(effect);
}

int BattleController::ApplyDamageToEnemy_(Enemy& enemy, int damage)
{
	if (!player_) {
		return 0;
	}

	player_->PlayAttackAnimWithEffect(enemy.GetPos(), -1);
	enemy.TriggerHitFlash(0.2f);
	enemy.PlayDamageAnim();
	const int beforeBlock = enemy.GetBlock();
	const int actualDamage = enemy.Damage(damage);
	BattleSfxPlayer::PlayBlockReactionSE(beforeBlock, enemy.GetBlock(), actualDamage, damage);

	if (player_->GetVampireHeal() > 0) {
		int beforeHp = player_->GetHP();
		player_->Heal(player_->GetVampireHeal());
		BattleSfxPlayer::PlayHealSEIfHpIncreased(player_, beforeHp);
	}

	return actualDamage;
}

