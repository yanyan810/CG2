#pragma once

#include <functional>

struct CardEffectDef;
class BattleDeckZone;
class CardDatabase;
class EnemyManager;
class Player;

class CardEffectExecutor {
public:
	struct Context {
		Player* player = nullptr;
		EnemyManager* enemyMgr = nullptr;
		const BattleDeckZone* deckZone = nullptr;
		const CardDatabase* cardDb = nullptr;
		std::function<void(int)> drawCards;
		int* nextTurnAtkUp = nullptr;
	};

	static void ApplyDraw(const Context& context, const CardEffectDef& effect);
	static void ApplyBlock(const Context& context, const CardEffectDef& effect);
	static void ApplyPowerBoost(const Context& context, const CardEffectDef& effect);
	static void ApplyNextTurnAtkUp(const Context& context, const CardEffectDef& effect);
	static void ApplyHeal(const Context& context, const CardEffectDef& effect);
	static void ApplyHealByBlock(const Context& context, const CardEffectDef& effect);
	static void ApplyHealByLowCostInHand(const Context& context, const CardEffectDef& effect);
	static void ApplyVampireBuff(const Context& context, const CardEffectDef& effect);
	static void ApplySelfDamage(const Context& context, const CardEffectDef& effect);
	static void ApplyPoisonAmplify(const Context& context, const CardEffectDef& effect);
	static void ApplyPoisonDamage(const Context& context, const CardEffectDef& effect);
	static void ApplyPoisonAll(const Context& context, const CardEffectDef& effect);
	static void ApplyPoisonDraw(const Context& context, const CardEffectDef& effect);
	static void ApplyPoisonRemove(const Context& context);
	static void ApplyFrostAll(const Context& context, const CardEffectDef& effect);
	static void ApplyFrostBlock(const Context& context, const CardEffectDef& effect);
	static void ApplyFrostDamage(const Context& context, const CardEffectDef& effect);
	static void ApplyFrostDraw(const Context& context, int targetIndex);
	static void ApplyFrostSubtract(const Context& context, const CardEffectDef& effect);
	static void ApplyFrostAmplify(const Context& context, const CardEffectDef& effect);
	static void ApplyChangeNumber(const CardEffectDef& effect);
	static void ApplyChangeSuit(const CardEffectDef& effect);
};
