#pragma once
#include <vector>
#include "CardDatabase.h"
#include "HandView3D.h"

class GameApp;
class Camera;

class BattleController {
public:

    enum class CardInputState {
        Idle,
        Dragging,
        Preview,
    };

    void Initialize(GameApp& app, Camera* camera);
    void Update(GameApp& app, float dt);
    void Draw(GameApp& app);

private:
    enum class TurnState { Player, Enemy };
    TurnState turn_ = TurnState::Player;

    Camera* cam_ = nullptr;

    CardDatabase db_;
    HandView3D handView_;

    std::vector<int> deck_;
    std::vector<int> hand_;
    std::vector<int> discard_;

    int energyMax_ = 3;
    int energy_ = 3;

    bool prevEnter_ = false;
    bool prevL_ = false;
    bool prevR_ = false;
    float enemyWait_ = 0.0f;

    CardInputState cardState_ = CardInputState::Idle;

    int selectedIndex_ = -1;      // 掴んでるカード
    POINT dragStartMouse_{};
    float dragDx_ = 0.0f;
    float dragDy_ = 0.0f;

private:
    void StartPlayerTurn_();
    void DrawUntilFive_();
    bool DrawOne_();
};