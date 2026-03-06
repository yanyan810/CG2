#pragma once
#include <vector>
#include "CardDatabase.h"
#include "HandView3D.h"

#include "Card3D.h"
#include <memory>

class GameApp;
class Camera;
class Object3dCommon;
class DirectXCommon;

class BattleController {
public:

    enum class CardInputState {
        Idle,
        Dragging,
        Preview,
        ChoosingFieldReplace,
    };

    void Initialize(GameApp& app, Camera* camera);
    void Update(GameApp& app, float dt);
    void Draw(GameApp& app);

#ifdef USE_IMGUI
    void DrawImGui();
#endif

private:
    enum class TurnState { Player, Enemy };
    TurnState turn_ = TurnState::Player;

    Camera* cam_ = nullptr;

    CardDatabase db_;
    HandView3D handView_;

    Object3dCommon* objCom_ = nullptr;
    DirectXCommon* dx_ = nullptr;

	std::vector<int> deck_; //デッキ（カードIDのリスト）
    std::vector<int> hand_; //手札
	std::vector<int> discard_; //捨て札
    std::vector<int> field_;   // 場のカード（最大5）
    std::vector<std::unique_ptr<Card3D>> fieldViews_;

    int energyMax_ = 50;
    int energy_ = 50;

    bool prevEnter_ = false;
    bool prevL_ = false;
    bool prevR_ = false;
    float enemyWait_ = 0.0f;

    CardInputState cardState_ = CardInputState::Idle;

    int selectedIndex_ = -1;      // 掴んでるカード
    int pendingCardId_ = -1;   // 入れ替え待ちの使用カード
    POINT dragStartMouse_{};
    float dragDx_ = 0.0f;
    float dragDy_ = 0.0f;

private:
    void StartPlayerTurn_();
    void DrawUntilFive_();
    bool DrawOne_();
    void RebuildFieldView_();
    int PickFieldIndexByMouse_(int mouseX, int mouseY) const;
    void DrawCards_(int count);
    void ApplyCardEffects_(const CardDef& def);

};