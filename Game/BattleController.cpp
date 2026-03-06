#include "BattleController.h"
#include "GameApp.h"
#include "WinApp.h"
#include <Windows.h>
#include "Object3dCommon.h"
#include "DirectXCommon.h"

void BattleController::Initialize(GameApp& app, Camera* camera)
{
    cam_ = camera;
    objCom_ = app.ObjCom();
    dx_ = app.Dx();

    if (!db_.LoadFromJson("resources/cards/cards.json")) {
        db_.BuildSample();
    }

    deck_ = { 1,2,3,1,2,3,1,2,3,1 };

    hand_.clear();
    discard_.clear();
    field_.clear();
    fieldViews_.clear();

    energy_ = energyMax_;

    handView_.Initialize(objCom_, dx_, cam_, &db_);
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

void BattleController::DrawCards_(int count)
{
    for (int i = 0; i < count; ++i) {
        if (!DrawOne_()) {
            break;
        }
    }
    handView_.Rebuild(hand_);
}

void BattleController::ApplyCardEffects_(const CardDef& def)
{
    for (const auto& effect : def.effects) {
        if (effect.type == "Draw") {
            DrawCards_(effect.value);
        } else if (effect.type == "Damage") {
            // まだ敵処理がないので後で実装
        } else if (effect.type == "Block") {
            // まだブロック処理がないので後で実装
        }
    }
}

void BattleController::StartPlayerTurn_()
{
    energy_ = energyMax_;
    DrawUntilFive_();
}

void BattleController::RebuildFieldView_()
{
    fieldViews_.clear();
    fieldViews_.reserve(field_.size());

    const int n = (int)field_.size();
    if (n <= 0) {
        return;
    }

    const float y = 0.8f;
    const float z = 5.0f;
    const float gap = 5.0f;
    const float startX = -gap * 0.5f * (n - 1);

    for (int i = 0; i < n; ++i) {
        const CardDef* def = db_.Find(field_[i]);
        if (!def) {
            continue;
        }

        auto card = std::make_unique<Card3D>();
        card->Initialize(objCom_, dx_, cam_, *def);

        Vector3 pos{ startX + gap * i, y, z };
        Vector3 rot{ 0.0f, 0.0f, 0.0f };
        Vector3 scl{ 1.15f, 1.15f, 1.15f };

        card->SetTransform(pos, rot, scl);
        fieldViews_.push_back(std::move(card));
    }
}

int BattleController::PickFieldIndexByMouse_(int mouseX, int mouseY) const
{
    const Matrix4x4& vp = cam_->GetViewProjectionMatrix();
    const float sw = (float)WinApp::kClientWidth;
    const float sh = (float)WinApp::kClientHeight;

    int best = -1;
    float bestD2 = 80.0f * 80.0f; // 判定半径

    for (int i = 0; i < (int)fieldViews_.size(); ++i) {
        Vector3 w = fieldViews_[i]->GetWorldPos();

        Vector4 clip{};
        clip.x = w.x * vp.m[0][0] + w.y * vp.m[1][0] + w.z * vp.m[2][0] + 1.0f * vp.m[3][0];
        clip.y = w.x * vp.m[0][1] + w.y * vp.m[1][1] + w.z * vp.m[2][1] + 1.0f * vp.m[3][1];
        clip.z = w.x * vp.m[0][2] + w.y * vp.m[1][2] + w.z * vp.m[2][2] + 1.0f * vp.m[3][2];
        clip.w = w.x * vp.m[0][3] + w.y * vp.m[1][3] + w.z * vp.m[2][3] + 1.0f * vp.m[3][3];

        if (clip.w <= 0.0f) {
            continue;
        }

        const float ndcX = clip.x / clip.w;
        const float ndcY = clip.y / clip.w;

        const float sx = (ndcX * 0.5f + 0.5f) * sw;
        const float sy = (-ndcY * 0.5f + 0.5f) * sh;

        const float dx = sx - (float)mouseX;
        const float dy = sy - (float)mouseY;
        const float d2 = dx * dx + dy * dy;

        if (d2 < bestD2) {
            bestD2 = d2;
            best = i;
        }
    }

    return best;
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

        // Enterでターン終了
        if (enterTrig && cardState_ == CardInputState::Idle) {
            turn_ = TurnState::Enemy;
            pendingCardId_ = -1;
            enemyWait_ = 1.0f; // 1秒待つ
            handView_.SetHoverIndex(-1);
            handView_.SetDrag(-1, 0, 0, false);
            handView_.SetPreviewIndex(-1);
            selectedIndex_ = -1;
            cardState_ = CardInputState::Idle;
        } else {
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

                handView_.SetDrag(selectedIndex_, dragDx_, dragDy_, true);

                const float threshold = 80.0f;

                if (lRel) {
                    handView_.SetDrag(-1, 0, 0, false);

                    if (dragDy_ <= -threshold) {
                        cardState_ = CardInputState::Preview;
                        handView_.SetPreviewIndex(selectedIndex_);
                    } else {
                        cardState_ = CardInputState::Idle;
                        selectedIndex_ = -1;
                        handView_.SetPreviewIndex(-1);
                    }
                }
            }
            break;

            case CardInputState::Preview:
                handView_.SetPreviewIndex(selectedIndex_);

                if (rTrig) {
                    int idx = selectedIndex_;
                    if (idx >= 0 && idx < (int)hand_.size()) {
                        int defId = hand_[idx];
                        const CardDef* def = db_.Find(defId);

                        if (def && def->cost <= energy_) {
                            energy_ -= def->cost;

                            int cardId = hand_[idx];
                            hand_.erase(hand_.begin() + idx);
                            handView_.Rebuild(hand_);

                            ApplyCardEffects_(*def);

                            if ((int)field_.size() < 5) {
                                field_.push_back(cardId);
                                RebuildFieldView_();

                                cardState_ = CardInputState::Idle;
                                pendingCardId_ = -1;
                            } else {
                                // 場が満杯なら入れ替え待ちへ
                                pendingCardId_ = cardId;
                                cardState_ = CardInputState::ChoosingFieldReplace;
                            }
                        } else {
                            cardState_ = CardInputState::Idle;
                        }
                    } else {
                        cardState_ = CardInputState::Idle;
                    }

                    selectedIndex_ = -1;
                    handView_.SetPreviewIndex(-1);
                }

                if (lTrig) {
                    cardState_ = CardInputState::Idle;
                    selectedIndex_ = -1;
                    handView_.SetPreviewIndex(-1);
                }
                break;

            case CardInputState::ChoosingFieldReplace:
            {
                // 左クリックで入れ替え先を選ぶ
                if (lTrig) {
                    int replaceIndex = PickFieldIndexByMouse_(mouse.x, mouse.y);
                    if (replaceIndex >= 0 && replaceIndex < (int)field_.size() && pendingCardId_ >= 0) {
                        // 置き換えられる場カードは墓地へ
                        discard_.push_back(field_[replaceIndex]);

                        // 新しいカードをその位置へ
                        field_[replaceIndex] = pendingCardId_;
                        RebuildFieldView_();

                        pendingCardId_ = -1;
                        cardState_ = CardInputState::Idle;
                    }
                }

                // 右クリックでキャンセル → 使用カードは墓地へ
                if (rTrig) {
                    if (pendingCardId_ >= 0) {
                        discard_.push_back(pendingCardId_);
                    }
                    pendingCardId_ = -1;
                    cardState_ = CardInputState::Idle;
                }

                handView_.SetHoverIndex(-1);
                handView_.SetDrag(-1, 0, 0, false);
                handView_.SetPreviewIndex(-1);
            }
            break;


            }
        }

    } else {
        // Enemyターン：少し待ってからプレイヤーターンへ戻す
        handView_.SetHoverIndex(-1);
        handView_.SetDrag(-1, 0, 0, false);
        handView_.SetPreviewIndex(-1);
        cardState_ = CardInputState::Idle;
        selectedIndex_ = -1;

        pendingCardId_ = -1;

        enemyWait_ -= dt;
        if (enemyWait_ <= 0.0f) {
            turn_ = TurnState::Player;
            StartPlayerTurn_();
        }
    }

    handView_.Update(dt);

    for (auto& c : fieldViews_) {
        c->Update(dt);
    }

}

void BattleController::Draw(GameApp& app)
{

    for (auto& c : fieldViews_) {
        c->Draw();
    }

    // ここは「3D PSO」側で描きたいので、Scene側でObjCom()->SetGraphicsPipelineState()後に呼ぶのが安全
    handView_.Draw();


}

#ifdef USE_IMGUI
#include <imgui.h>
void BattleController::DrawImGui()
{
    ImGui::Text("turn: %s", turn_ == TurnState::Player ? "Player" : "Enemy");
    ImGui::Text("energy: %d / %d", energy_, energyMax_);
    ImGui::Text("hand: %d  discard: %d", (int)hand_.size(), (int)discard_.size());
    ImGui::Text("field: %d", (int)field_.size());
   handView_.DrawImGui();

   const char* stateName = "";
   switch (cardState_) {
   case CardInputState::Idle: stateName = "Idle"; break;
   case CardInputState::Dragging: stateName = "Dragging"; break;
   case CardInputState::Preview: stateName = "Preview"; break;
   case CardInputState::ChoosingFieldReplace: stateName = "ChoosingFieldReplace"; break;
   }
   ImGui::Text("cardState: %s", stateName);
   ImGui::Text("pendingCardId: %d", pendingCardId_);

   ImGui::Text("field: %d", (int)field_.size());
   for (int i = 0; i < (int)field_.size(); ++i) {
       ImGui::Text("field[%d] = %d", i, field_[i]);
   }



}
#endif