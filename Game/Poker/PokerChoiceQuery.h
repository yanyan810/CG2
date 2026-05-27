#pragma once

#include "BattleController.h"
#include "PokerChoiceTextBuilder.h"

class PokerChoiceQuery {
public:
	static bool HasChoiceUi(BattleController::PokerChoiceState state);
	static bool IsWaitingActivate(BattleController::PokerChoiceState state);
	static bool IsWaitingEffect(BattleController::PokerChoiceState state);
	static bool IsViewingBoard(BattleController::PokerChoiceState state);
	static int GetMouseChoiceIndex(BattleController::PokerMouseChoice choice);
	static PokerChoiceTextBuilder::ChoiceState ToTextBuilderState(BattleController::PokerChoiceState state);
};
