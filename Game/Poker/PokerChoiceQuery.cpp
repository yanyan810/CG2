#include "PokerChoiceQuery.h"

bool PokerChoiceQuery::HasChoiceUi(BattleController::PokerChoiceState state)
{
	return IsWaitingActivate(state) || IsWaitingEffect(state);
}

bool PokerChoiceQuery::IsWaitingActivate(BattleController::PokerChoiceState state)
{
	return state == BattleController::PokerChoiceState::WaitingActivateChoice;
}

bool PokerChoiceQuery::IsWaitingEffect(BattleController::PokerChoiceState state)
{
	return state == BattleController::PokerChoiceState::WaitingEffectChoice;
}

bool PokerChoiceQuery::IsViewingBoard(BattleController::PokerChoiceState state)
{
	return state == BattleController::PokerChoiceState::ViewingBoardFromPokerUi;
}

int PokerChoiceQuery::GetMouseChoiceIndex(BattleController::PokerMouseChoice choice)
{
	switch (choice) {
	case BattleController::PokerMouseChoice::ActivateYes:       return 0;
	case BattleController::PokerMouseChoice::ActivateNo:        return 1;
	case BattleController::PokerMouseChoice::ActivateViewBoard: return 2;

	case BattleController::PokerMouseChoice::EffectBack:        return 0;
	case BattleController::PokerMouseChoice::EffectAtkUp:       return 1;
	case BattleController::PokerMouseChoice::EffectDamage:      return 2;
	case BattleController::PokerMouseChoice::EffectDraw:        return 3;
	case BattleController::PokerMouseChoice::EffectViewBoard:   return 4;

	case BattleController::PokerMouseChoice::ReturnFromBoard:   return 0;

	default:                                                    return -1;
	}
}

PokerChoiceTextBuilder::ChoiceState PokerChoiceQuery::ToTextBuilderState(BattleController::PokerChoiceState state)
{
	switch (state) {
	case BattleController::PokerChoiceState::WaitingActivateChoice:
		return PokerChoiceTextBuilder::ChoiceState::WaitingActivateChoice;
	case BattleController::PokerChoiceState::WaitingEffectChoice:
		return PokerChoiceTextBuilder::ChoiceState::WaitingEffectChoice;
	case BattleController::PokerChoiceState::ViewingBoardFromPokerUi:
		return PokerChoiceTextBuilder::ChoiceState::ViewingBoard;
	case BattleController::PokerChoiceState::None:
	default:
		return PokerChoiceTextBuilder::ChoiceState::None;
	}
}
