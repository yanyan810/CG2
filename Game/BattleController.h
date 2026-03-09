#pragma once
#include <vector>
#include "CardDatabase.h"
#include "HandView3D.h"
#include "CardInstance.h"
#include "DeckDef.h"
#include "DeckLoader.h"
#include "Card3D.h"
#include <memory>

class GameApp;
class Camera;
class Object3dCommon;
class DirectXCommon;

class Player;
class Enemy;

class BattleController {
public:

    enum class CardInputState {
        Idle,
        Dragging,
        Preview,
        ChoosingFieldReplace,
    };

    enum class PokerChoiceState
    {
        None,
        WaitingActivateChoice, // 発動する/しない
        WaitingEffectChoice    // 発動すると決めた後、どの効果か選ぶ
    };

    struct PokerBonus {
        int atkUp = 0;
        int drawCount = 0;
        int damage = 0;
    };

    void Initialize(GameApp& app, Camera* camera);
    void Update(GameApp& app, float dt);
    void Draw(GameApp& app);

#ifdef USE_IMGUI
    void DrawImGui();
#endif

    //役
    enum class PokerHandRank {
        None = 0,
        OnePair,
        TwoPair,
        ThreeOfAKind,
        Straight,
        Flush,
        FullHouse,
        FourOfAKind,
        StraightFlush,
        RoyalStraightFlush,
    };

    struct PokerHandResult {
        PokerHandRank rank = PokerHandRank::None;
        int power = 0;
    };

    void SetPlayer(Player* player);
    void SetEnemy(Enemy* enemy);

private:
    enum class TurnState { Player, Enemy };
    TurnState turn_ = TurnState::Player;

    Camera* cam_ = nullptr;

    CardDatabase db_;
    HandView3D handView_;

    Object3dCommon* objCom_ = nullptr;
    DirectXCommon* dx_ = nullptr;

    std::vector<CardInstance> deck_;
    std::vector<CardInstance> hand_;
    std::vector<CardInstance> discard_;
    std::vector<CardInstance> field_;
    std::vector<std::unique_ptr<Card3D>> fieldViews_;

    int energyMax_ = 50;
    int energy_ = 50;

 
    float enemyWait_ = 0.0f;

    CardInputState cardState_ = CardInputState::Idle;

    int selectedIndex_ = -1;
    CardInstance pendingCard_;
    bool hasPendingCard_ = false;

    POINT dragStartMouse_{};
    float dragDx_ = 0.0f;
    float dragDy_ = 0.0f;

    PokerChoiceState pokerChoiceState_ = PokerChoiceState::None;
    PokerHandResult currentPoker_;

    //キー用
    bool prevY_ = false;
    bool prevN_ = false;
    bool prev1_ = false;
    bool prev2_ = false;
    bool prev3_ = false;
    bool prevEnter_ = false;
    bool prevL_ = false;
    bool prevR_ = false;

    int nextTurnAtkUp_ = 0;
    int enemyHp_ = 300;

    Player* player_ = nullptr;
    Enemy* enemy_ = nullptr;

private:
    void StartPlayerTurn_();
    void DrawUntilFive_();
    bool DrawOne_();
    void RebuildFieldView_();
    int PickFieldIndexByMouse_(int mouseX, int mouseY) const;
    void DrawCards_(int count);
    void ApplyCardEffects_(const CardDef& def);
    PokerHandResult EvaluatePokerHand_() const;
    const char* GetPokerHandName_(PokerHandRank rank) const;

    PokerBonus GetPokerBonus_(PokerHandRank rank) const;
    void ConsumeFieldCards_();

    PokerHandRank ParsePokerRankString_(const std::string& s) const;
    bool IsRankAtLeast_(PokerHandRank a, PokerHandRank b) const;
    bool IsRankInFamily_(PokerHandRank rank, const std::string& family) const;
    bool DoesSubEffectConditionMatch_(const CardSubEffectDef& sub, PokerHandRank rank) const;

    void ApplyEffectsList_(const std::vector<CardEffectDef>& effects);
    void TriggerSubEffectsForField_(SubEffectTrigger trigger, PokerHandRank rank);
    void TriggerSubEffectsForCard_(const CardInstance& card, SubEffectTrigger trigger, PokerHandRank rank);

};