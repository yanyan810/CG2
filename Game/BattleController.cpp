#include "BattleController.h"
#include "GameApp.h"
#include "WinApp.h"
#include <Windows.h>

void BattleController::Initialize(GameApp& app, Camera* camera)
{
    cam_ = camera;

    db_.BuildSample();

    // 仮デッキ（今はサンプル）
    deck_ = { 1,2,1,2,1, 1,2,1,2,1 };

    hand_.clear();
    discard_.clear();

    energy_ = energyMax_;

    handView_.Initialize(app.ObjCom(), app.Dx(), cam_, &db_);
    handView_.Rebuild(hand_);

    StartPlayerTurn_();
}

bool BattleController::DrawOne_()
{
    if (deck_.empty()) {
        if (discard_.empty()) return false;
        deck_ = discard_;
        discard_.clear();
    }
    if (deck_.empty()) return false;

    int id = deck_.back();
    deck_.pop_back();
    hand_.push_back(id);
    return true;
}

void BattleController::DrawUntilFive_()
{
    while ((int)hand_.size() < 5) {
        if (!DrawOne_()) break;
    }
    handView_.Rebuild(hand_);
}

void BattleController::StartPlayerTurn_()
{
    energy_ = energyMax_;
    DrawUntilFive_();
}

void BattleController::Update(GameApp& app, float dt)
{
    // Enterトリガー（ターン終了）
    bool enterNow = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
    bool enterTrig = enterNow && !prevEnter_;
    prevEnter_ = enterNow;

    // マウス座標（毎フレーム）
    POINT mouse{};
    GetCursorPos(&mouse);
    ScreenToClient(app.Win()->GetHwnd(), &mouse);

    // hover（自分ターンのみ）
    if (turn_ == TurnState::Player) {
        int hover = handView_.PickIndexByMouse(
            mouse.x, mouse.y,
            cam_->GetViewProjectionMatrix(),
            (float)WinApp::kClientWidth, (float)WinApp::kClientHeight
        );
        handView_.SetHoverIndex(hover);
    } else {
        handView_.SetHoverIndex(-1);
    }

    // クリック（自分ターンのみ）
    bool lNow = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool lTrig = lNow && !prevL_;
    bool lRel = !lNow && prevL_;
    prevL_ = lNow;

    bool rNow = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    bool rTrig = rNow && !prevR_;
    prevR_ = rNow;

    // ターン遷移：Enterで自分ターン終了→敵ターン(待つだけ)→自分ターン開始（5枚補充）
    if (turn_ == TurnState::Player) {

        // hover更新（Preview中はhover無しでもOK）
        if (cardState_ != CardInputState::Preview) {
            int hover = handView_.PickIndexByMouse(
                mouse.x, mouse.y,
                cam_->GetViewProjectionMatrix(),
                (float)WinApp::kClientWidth, (float)WinApp::kClientHeight
            );
            handView_.SetHoverIndex(hover);
        } else {
            handView_.SetHoverIndex(-1);
        }

        switch (cardState_) {
        case CardInputState::Idle:
            handView_.SetDrag(-1, 0, 0, false);
            handView_.SetPreviewIndex(-1);

            if (lTrig) {
                int idx = handView_.PickIndexByMouse(
                    mouse.x, mouse.y,
                    cam_->GetViewProjectionMatrix(),
                    (float)WinApp::kClientWidth, (float)WinApp::kClientHeight
                );
                if (idx >= 0) {
                    selectedIndex_ = idx;
                    dragStartMouse_ = mouse;
                    dragDx_ = dragDy_ = 0.0f;
                    cardState_ = CardInputState::Dragging;
                }
            }
            break;

        case CardInputState::Dragging:
        {
            dragDx_ = float(mouse.x - dragStartMouse_.x);
            dragDy_ = float(mouse.y - dragStartMouse_.y);

            const float threshold = 80.0f;

            if (dragDy_ <= -threshold) {
                cardState_ = CardInputState::Preview;
                handView_.SetDrag(-1, 0, 0, false);
                handView_.SetPreviewIndex(selectedIndex_);
            }

            if (lRel && cardState_ != CardInputState::Preview) {
                cardState_ = CardInputState::Idle;
                selectedIndex_ = -1;
                handView_.SetDrag(-1, 0, 0, false);
            }
        }
        break;

        case CardInputState::Preview:
            handView_.SetPreviewIndex(selectedIndex_);

            // 右クリック：発動
            if (rTrig) {
                int idx = selectedIndex_;
                if (idx >= 0 && idx < (int)hand_.size()) {
                    int defId = hand_[idx];
                    const CardDef* def = db_.Find(defId);
                    if (def && def->cost <= energy_) {
                        energy_ -= def->cost;
                        discard_.push_back(defId);
                        hand_.erase(hand_.begin() + idx);
                        handView_.Rebuild(hand_);
                    }
                }
                // リセット
                cardState_ = CardInputState::Idle;
                selectedIndex_ = -1;
                handView_.SetPreviewIndex(-1);
            }

            // 左クリック：キャンセル
            if (lTrig) {
                cardState_ = CardInputState::Idle;
                selectedIndex_ = -1;
                handView_.SetPreviewIndex(-1);
            }
            break;
        }

    } else {
        // Enemyターン：入力無効化
        handView_.SetHoverIndex(-1);
        handView_.SetDrag(-1, 0, 0, false);
        handView_.SetPreviewIndex(-1);
        cardState_ = CardInputState::Idle;
        selectedIndex_ = -1;
    }

    handView_.Update(dt);
}

void BattleController::Draw(GameApp& app)
{
    // ここは「3D PSO」側で描きたいので、Scene側でObjCom()->SetGraphicsPipelineState()後に呼ぶのが安全
    handView_.Draw();
}