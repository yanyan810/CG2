#pragma once
#include <string>
#include <unordered_map>

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

    bool LoadMessages(const std::string& path);

    //文字用
    void SetMessageForCurrentStep(const std::wstring& text);

    void SetStepMessage(TutorialStep step, const std::wstring& text);
    std::wstring GetStepMessage(TutorialStep step) const;

    bool ReloadMessages();

private:
    void UpdateMessage_();
    void Advance_();

    static const char* StepToKey_(TutorialStep step);
    std::wstring GetMessageFromTable_(TutorialStep step) const;
    static std::wstring Utf8ToWString_(const std::string& s);

private:
    TutorialStep step_ = TutorialStep::Intro;
    bool isActive_ = true;
    std::wstring message_;

    bool sawEnemyTurn_ = false;
    bool skippedPokerOnce_ = false;

    std::unordered_map<std::string, std::wstring> messageTable_;
    std::string messagePath_ = "resources/ui/tutorial_messages.json";

    std::unordered_map<int, std::wstring> overrideMessages_;
};