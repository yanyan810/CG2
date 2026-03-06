#include "BattleController.h"
#include "GameApp.h"
#include "WinApp.h"
#include <Windows.h>
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include <array>
#include <algorithm>

#include <random>

#include"Player.h"
#include"Enemy.h"

//===============================
//役
//===============================

const char* BattleController::GetPokerHandName_(PokerHandRank rank) const
{
    switch (rank) {
    case PokerHandRank::None:                 return "None";
    case PokerHandRank::OnePair:              return "One Pair";
    case PokerHandRank::TwoPair:              return "Two Pair";
    case PokerHandRank::ThreeOfAKind:         return "Three of a Kind";
    case PokerHandRank::Straight:             return "Straight";
    case PokerHandRank::Flush:                return "Flush";
    case PokerHandRank::FullHouse:            return "Full House";
    case PokerHandRank::FourOfAKind:          return "Four of a Kind";
    case PokerHandRank::StraightFlush:        return "Straight Flush";
    case PokerHandRank::RoyalStraightFlush:   return "Royal Straight Flush";
    default:                                  return "?";
    }
}

BattleController::PokerHandResult BattleController::EvaluatePokerHand_() const
{
    PokerHandResult result{};

    if (field_.size() != 5) {
        result.rank = PokerHandRank::None;
        result.power = 0;
        return result;
    }

    std::array<int, 14> countNumber{}; // 1~13 を使う
    std::array<int, 4> countSuit{};    // Spade/Heart/Diamond/Club

    std::vector<int> numbers;
    numbers.reserve(5);

    for (const auto& c : field_) {
        if (c.number >= 1 && c.number <= 13) {
            countNumber[c.number]++;
            numbers.push_back(c.number);
        }

        int suitIndex = static_cast<int>(c.suit);
        if (suitIndex >= 0 && suitIndex < 4) {
            countSuit[suitIndex]++;
        }
    }

    std::sort(numbers.begin(), numbers.end());

    // ---- Flush 判定 ----
    bool isFlush = false;
    for (int s : countSuit) {
        if (s == 5) {
            isFlush = true;
            break;
        }
    }

    // ---- Straight 判定 ----
    bool isStraight = false;

    // 通常の連番
    {
        bool straight = true;
        for (int i = 0; i < 4; ++i) {
            if (numbers[i] + 1 != numbers[i + 1]) {
                straight = false;
                break;
            }
        }
        if (straight) {
            isStraight = true;
        }
    }

    // A,10,J,Q,K を認める
    if (!isStraight) {
        std::vector<int> royal = numbers;
        std::sort(royal.begin(), royal.end());
        if (royal.size() == 5 &&
            royal[0] == 1 &&
            royal[1] == 10 &&
            royal[2] == 11 &&
            royal[3] == 12 &&
            royal[4] == 13) {
            isStraight = true;
        }
    }

    // A,2,3,4,5 を認める
    if (!isStraight) {
        std::vector<int> lowA = numbers;
        std::sort(lowA.begin(), lowA.end());
        if (lowA.size() == 5 &&
            lowA[0] == 1 &&
            lowA[1] == 2 &&
            lowA[2] == 3 &&
            lowA[3] == 4 &&
            lowA[4] == 5) {
            isStraight = true;
        }
    }

    // ---- 同数枚数を数える ----
    int pairCount = 0;
    bool hasThree = false;
    bool hasFour = false;

    for (int n = 1; n <= 13; ++n) {
        if (countNumber[n] == 2) pairCount++;
        if (countNumber[n] == 3) hasThree = true;
        if (countNumber[n] == 4) hasFour = true;
    }

    // ---- 役判定（強い順）----
    bool isRoyal = (numbers[0] == 1 &&
        numbers[1] == 10 &&
        numbers[2] == 11 &&
        numbers[3] == 12 &&
        numbers[4] == 13);

    if (isStraight && isFlush && isRoyal) {
        result.rank = PokerHandRank::RoyalStraightFlush;
        result.power = 100;
    } else if (isStraight && isFlush) {
        result.rank = PokerHandRank::StraightFlush;
        result.power = 80;
    } else if (hasFour) {
        result.rank = PokerHandRank::FourOfAKind;
        result.power = 70;
    } else if (hasThree && pairCount == 1) {
        result.rank = PokerHandRank::FullHouse;
        result.power = 60;
    } else if (isFlush) {
        result.rank = PokerHandRank::Flush;
        result.power = 50;
    } else if (isStraight) {
        result.rank = PokerHandRank::Straight;
        result.power = 40;
    } else if (hasThree) {
        result.rank = PokerHandRank::ThreeOfAKind;
        result.power = 30;
    } else if (pairCount == 2) {
        result.rank = PokerHandRank::TwoPair;
        result.power = 20;
    } else if (pairCount == 1) {
        result.rank = PokerHandRank::OnePair;
        result.power = 10;
    } else {
        result.rank = PokerHandRank::None;
        result.power = 0;
    }

    return result;
}


namespace {
    int RandomRangeInt(int minValue, int maxValue)
    {
        static std::random_device rd;
        static std::mt19937 mt(rd());
        std::uniform_int_distribution<int> dist(minValue, maxValue);
        return dist(mt);
    }

    CardSuit RandomSuit()
    {
        int v = RandomRangeInt(0, 3);
        return static_cast<CardSuit>(v);
    }

    CardInstance MakeCardInstance(int defId)
    {
        CardInstance c{};
        c.defId = defId;
        c.number = RandomRangeInt(1, 13);
        c.suit = RandomSuit();
        return c;
    }

    const char* SuitToString(CardSuit suit)
    {
        switch (suit) {
        case CardSuit::Spade:   return "Spade";
        case CardSuit::Heart:   return "Heart";
        case CardSuit::Diamond: return "Diamond";
        case CardSuit::Club:    return "Club";
        default:                return "?";
        }
    }
}

void BattleController::Initialize(GameApp& app, Camera* camera)
{
    cam_ = camera;
    objCom_ = app.ObjCom();
    dx_ = app.Dx();

    if (!db_.LoadFromJson("resources/cards/cards.json")) {
        db_.BuildSample();
    }

    deck_.clear();
    for (int id : { 1, 2, 3, 4, 5, 1, 2, 3, 4, 5 }) {
        deck_.push_back(MakeCardInstance(id));
    }

    hand_.clear();
    discard_.clear();
    field_.clear();
    fieldViews_.clear();

    hasPendingCard_ = false;
    pendingCard_ = {};

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

    CardInstance card = deck_.back();
    deck_.pop_back();
    hand_.push_back(card);
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
            enemy_->Damage(effect.value);
        } else if (effect.type == "Block") {
            // 後で実装
        } else if (effect.type == "Heal") {
            player_->Heal(effect.value);
        }
        else if (effect.type == "SelfDamage") {
            player_->Damage(effect.value);
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

    const float y = -5.0f;
    const float z = 5.0f;
    const float gap = 5.0f;
    const float startX = -gap * 0.5f * (n - 1);

    for (int i = 0; i < n; ++i) {
        const CardDef* def = db_.Find(field_[i].defId);
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
    float bestD2 = 80.0f * 80.0f;

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
    bool enterNow = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
    bool enterTrig = enterNow && !prevEnter_;
    prevEnter_ = enterNow;

    POINT mouse{};
    GetCursorPos(&mouse);
    ScreenToClient(app.Win()->GetHwnd(), &mouse);

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

    bool lNow = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool lTrig = lNow && !prevL_;
    bool lRel = !lNow && prevL_;
    prevL_ = lNow;

    bool rNow = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    bool rTrig = rNow && !prevR_;
    prevR_ = rNow;

    if (turn_ == TurnState::Player) {

        if (enterTrig && cardState_ == CardInputState::Idle) {
            turn_ = TurnState::Enemy;
            hasPendingCard_ = false;
            pendingCard_ = {};
            enemyWait_ = 1.0f;
            handView_.SetHoverIndex(-1);
            handView_.SetDrag(-1, 0, 0, false);
            handView_.SetPreviewIndex(-1);
            selectedIndex_ = -1;
            cardState_ = CardInputState::Idle;
        } else {
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
                        CardInstance inst = hand_[idx];
                        const CardDef* def = db_.Find(inst.defId);

                        if (def && def->cost <= energy_) {
                            energy_ -= def->cost;

                            hand_.erase(hand_.begin() + idx);
                            handView_.Rebuild(hand_);

                            ApplyCardEffects_(*def);

                            if ((int)field_.size() < 5) {
                                field_.push_back(inst);
                                RebuildFieldView_();

                                cardState_ = CardInputState::Idle;
                                hasPendingCard_ = false;
                                pendingCard_ = {};
                            } else {
                                pendingCard_ = inst;
                                hasPendingCard_ = true;
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
                if (lTrig) {
                    int replaceIndex = PickFieldIndexByMouse_(mouse.x, mouse.y);
                    if (replaceIndex >= 0 && replaceIndex < (int)field_.size() && hasPendingCard_) {
                        discard_.push_back(field_[replaceIndex]);
                        field_[replaceIndex] = pendingCard_;
                        RebuildFieldView_();

                        hasPendingCard_ = false;
                        pendingCard_ = {};
                        cardState_ = CardInputState::Idle;
                    }
                }

                if (rTrig) {
                    if (hasPendingCard_) {
                        discard_.push_back(pendingCard_);
                    }
                    hasPendingCard_ = false;
                    pendingCard_ = {};
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
        handView_.SetHoverIndex(-1);
        handView_.SetDrag(-1, 0, 0, false);
        handView_.SetPreviewIndex(-1);
        cardState_ = CardInputState::Idle;
        selectedIndex_ = -1;

        hasPendingCard_ = false;
        pendingCard_ = {};

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

    if (hasPendingCard_) {
        ImGui::Text("pending: defId=%d number=%d suit=%s",
            pendingCard_.defId,
            pendingCard_.number,
            SuitToString(pendingCard_.suit));
    } else {
        ImGui::Text("pending: none");
    }

    ImGui::Separator();
    ImGui::Text("Hand Cards");
    for (int i = 0; i < (int)hand_.size(); ++i) {
        ImGui::Text("hand[%d] defId=%d number=%d suit=%s",
            i,
            hand_[i].defId,
            hand_[i].number,
            SuitToString(hand_[i].suit));
    }

    ImGui::Separator();
    ImGui::Text("Field Cards");
    for (int i = 0; i < (int)field_.size(); ++i) {
        ImGui::Text("field[%d] defId=%d number=%d suit=%s",
            i,
            field_[i].defId,
            field_[i].number,
            SuitToString(field_[i].suit));
    }

    ImGui::Separator();
    PokerHandResult poker = EvaluatePokerHand_();
    ImGui::Text("Poker Hand: %s", GetPokerHandName_(poker.rank));
    ImGui::Text("Poker Power: %d", poker.power);

    ImGui::Separator();

    ImGui::Text("Player Hp: %d", player_->GetHP());
    ImGui::Text("Enemy  Hp: %d", enemy_->GetHP());

}
#endif

void BattleController::SetPlayer(Player* player) {
    player_ = player;
}

void BattleController::SetEnemy(Enemy* enemy) {
    enemy_ = enemy;
}