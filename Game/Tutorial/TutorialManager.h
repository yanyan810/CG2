#pragma once
#include <string>
class BattleController;

class TutorialManager {
public:
    enum class TutorialStep {
        Intro,
        HoverHand,
        PlayCard,
        ExplainEnergy,
        FillField,
        EndPlayerTurn,
        WaitEnemyTurn,
        ExplainPokerReady,
        ChoosePokerEffect,

        SkipPokerContinueTurn,
        SkipPokerEndTurn,
        SkipPokerWaitEnemyTurn,
        ViewingBoardFromPoker,

        EndAfterPoker,
        Finished
    };

    enum class FocusType {
        None,
        HandArea,
        FieldArea,
        EnergyArea,
        EndTurnButtonArea,
        PokerActivateChoiceArea,
        PokerEffectChoiceArea,
        PokerBackButtonArea,
        PokerViewBoardButtonArea,
        EnemyTurnArea
    };

public:
    void Initialize();
    void Reset();
    void Update(BattleController& battle);

    void NextStep();

    bool IsActive() const { return isActive_; }
    bool IsFinished() const { return step_ == TutorialStep::Finished; }

    const std::wstring& GetMessage() const { return message_; }
    TutorialStep GetStep() const { return step_; }
    FocusType GetFocusType() const;

private:
    void UpdateMessage_();
    void Advance_();

private:
    TutorialStep step_ = TutorialStep::Intro;
    bool isActive_ = true;
    std::wstring message_;

    bool sawEnemyTurn_ = false;
    bool skippedPokerOnce_ = false;
};