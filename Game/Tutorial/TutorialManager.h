#pragma once
#include <string>
#include <unordered_map>

class BattleController;

class TutorialManager {
public:

    enum class TutorialChapter {
        Full,
        FieldUi,
        Card,
        SpecialEffect,
        Practice
    };

    enum class TutorialStep {
        Intro,

        UiPlayerHp,
        UiEnemyIntentDamage,
        UiEnemyHp,
        UiEnemyNextAction,
        UiTurnText,
        UiHand,
        UiField,
        UiRoleText,
        UiEndTurn,
        UiDeckCount,
        UiPokerHandHelp,
        UiFinished,

        HoverHand,

        ExplainCardCost,
        ExplainCardSuit,
        ExplainCardNumber,
        ExplainCardAll,

        PlayCard,
        ChooseEnemyTarget,
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
        EnemyTurnArea,

        PlayerHpArea,
        EnemyHpArea,
        TurnTextArea,
        RoleTextArea,
        DeckCountArea,
        PokerHandHelpArea,

        PlayerIncomingDamageArea,
        EnemyNextActionArea
    };

public:
    void Initialize();
    void Reset();
    void StartChapter(TutorialChapter chapter);
    void Update(BattleController& battle);
    void NextStep();

    bool IsActive() const { return isActive_; }
    bool IsFinished() const { return step_ == TutorialStep::Finished; }

    const std::wstring& GetMessage() const { return message_; }
    TutorialStep GetStep() const { return step_; }
    FocusType GetFocusType() const;

    bool LoadMessages(const std::string& path);
    bool SaveMessages(const std::string& path = "") const;

    //文字用
    void SetMessageForCurrentStep(const std::wstring& text);

    void SetStepMessage(TutorialStep step, const std::wstring& text);
    std::wstring GetStepMessage(TutorialStep step) const;
    std::wstring GetEditableStepMessage(TutorialStep step) const;
    const std::string& GetMessagePath() const { return messagePath_; }
    static const char* GetStepKey(TutorialStep step) { return StepToKey_(step); }
    static std::wstring Utf8ToWString(const std::string& s);
    static std::string WStringToUtf8(const std::wstring& s);

    bool ReloadMessages();

    bool IsForceActivateOnly() const;
    bool IsForceDamageOnly() const;

	// UI説明ステップかどうか（UI説明ステップなら、UIの操作を制限する）
    bool IsUiExplanationStep() const;
    bool IsGameplayInputLocked() const;

private:
    void UpdateMessage_();
    void Advance_();
    void FinishIfPastChapterEnd_();
    bool IsPastChapterEnd_(TutorialStep step) const;
    TutorialStep GetChapterStartStep_(TutorialChapter chapter) const;
    TutorialStep GetChapterEndStep_(TutorialChapter chapter) const;

    static const char* StepToKey_(TutorialStep step);
    std::wstring GetMessageFromTable_(TutorialStep step) const;

private:
    TutorialStep step_ = TutorialStep::Intro;
    TutorialChapter chapter_ = TutorialChapter::Full;
    bool isActive_ = true;
    std::wstring message_;

    bool sawEnemyTurn_ = false;
    bool skippedPokerOnce_ = false;

    std::unordered_map<std::string, std::wstring> messageTable_;
    std::string messagePath_ = "resources/ui/tutorial_messages.json";

    std::unordered_map<int, std::wstring> overrideMessages_;
};
