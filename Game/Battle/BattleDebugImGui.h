#pragma once

#include "Card/CardInstance.h"
#include "Poker/PokerHandEvaluator.h"

#include <vector>

class BattleActionDirector;
class BattleDeckZone;
class Camera;
class EnemyManager;
class HandView3D;
class Player;
class PlayerBattleStatusUI;
class PropManager;
struct Vector3;

class BattleDebugImGui {
public:
	struct PokerBonusView {
		int atkUp = 0;
		int drawCount = 0;
		int damage = 0;
	};

	struct FieldCardGlitterControls {
		Vector3* localOffset = nullptr;
		float* spreadX = nullptr;
		float* spreadY = nullptr;
		float* emitInterval = nullptr;
		int* normalCount = nullptr;
		int* highlightCount = nullptr;
		bool* handPreviewEnabled = nullptr;
		float* handEmitInterval = nullptr;
		int* handCount = nullptr;
	};

	struct FieldFrameBloomControls {
		bool* enabled = nullptr;
		float* threshold = nullptr;
		float* intensity = nullptr;
		float* handIntensity = nullptr;
		float* minPulse = nullptr;
		float* chromAb = nullptr;
	};

	struct EnemyBloomControls {
		bool* intentEnabled = nullptr;
		float* intentIntensity = nullptr;
		float* intentMinPulse = nullptr;
		bool* targetEnabled = nullptr;
		float* targetIntensity = nullptr;
		float* targetChromAb = nullptr;
	};

	struct HpGaugeBloomControls {
		bool* enabled = nullptr;
		float* intensity = nullptr;
		float* minPulse = nullptr;
		float* damageBlinkSpeed = nullptr;
		float* damageIntensity = nullptr;
	};

	struct FrostActionControls {
		int* threshold = nullptr;
		int* burstMultiplier = nullptr;
	};

	struct Context {
		PlayerBattleStatusUI* playerStatusUi = nullptr;
		const char* turnName = "";
		int playerTurnCount = 0;
		int enemyTurnCount = 0;
		int energy = 0;
		int energyMax = 0;
		const BattleDeckZone* deckZone = nullptr;
		const std::vector<CardInstance>* field = nullptr;
		HandView3D* handView = nullptr;
		const char* cardStateName = "";
		bool hasPendingCard = false;
		CardInstance pendingCard{};
		PokerHandResult evaluatedPoker{};
		PokerHandResult currentPoker{};
		bool waitingActivateChoice = false;
		bool waitingEffectChoice = false;
		PokerBonusView currentPokerBonus{};
		Player* player = nullptr;
		EnemyManager* enemyMgr = nullptr;
		PropManager* propManager = nullptr;
		bool* useDebugPreviewBuff = nullptr;
		int* currentTurnAtkUp = nullptr;
		int* nextTurnAtkUp = nullptr;
		int* debugPreviewPowerBoost = nullptr;
		int* debugPreviewCurrentTurnAtkUp = nullptr;
		int* debugPreviewNextTurnAtkUp = nullptr;
		BattleActionDirector* actionDirector = nullptr;
		Camera* camera = nullptr;
		FieldCardGlitterControls fieldCardGlitter{};
		FieldFrameBloomControls fieldFrameBloom{};
		EnemyBloomControls enemyBloom{};
		HpGaugeBloomControls hpGaugeBloom{};
		FrostActionControls frostAction{};
	};

	static void DrawPlayerHudControls(PlayerBattleStatusUI& playerStatusUi);
	static void Draw(const Context& context);
};
