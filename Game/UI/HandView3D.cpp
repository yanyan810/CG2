#include "HandView3D.h"
#include "CardDatabase.h"
#include "GameApp.h"
#include "WinApp.h"
#include <algorithm>
#include <cmath>

// ここはあなたの WorldToScreen 実装に置き換えてOK
static bool WorldToScreen_RowVector(const Vector3& w, const Matrix4x4& viewProj,
    float sw, float sh, Vector2& out)
{
    Vector4 clip = MulRowVec4Mat4({ w.x, w.y, w.z, 1.0f }, viewProj);
    if (clip.w <= 0.0001f) return false;

    float ndcX = clip.x / clip.w;
    float ndcY = clip.y / clip.w;

    out.x = (ndcX * 0.5f + 0.5f) * sw;
    out.y = (-ndcY * 0.5f + 0.5f) * sh;
    return true;
}

namespace {
float Clamp01_(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

BloomParam MakeCardDissolveParam_(const BloomParam& baseParam, float dissolveAmount)
{
    BloomParam param = baseParam;
    param.threshold = 0.0f;
    param.dissolveAmount = dissolveAmount;
    param.dissolveEdgeWidth = 0.05f;
    param.dissolveEdgeIntensity = 3.0f;
    param.dissolveNoiseScale = 34.0f;
    param.dissolveEdgeColor = { 0.10f, 0.95f, 1.0f, 1.0f };
    param.intensity = 1.0f;
    param.chromAbAmount = 0.004f;
    param.distortionAmount = 0.0015f;
    param.noiseIntensity = 0.0f;
    param.scanlineIntensity = 0.0f;
    param.glitchAmount = 0.0f;
    param.vignetteIntensity = 0.0f;
    param.vignetteScale = 0.0f;
    param.isGrayscale = 0.0f;
    param.isInverted = 0.0f;
    return param;
}
}

void HandView3D::Initialize(Object3dCommon* objCom, DirectXCommon* dx, Camera* cam, CardDatabase* db)
{
    objCom_ = objCom;
    dx_ = dx;
    cam_ = cam;
    db_ = db;

    const int kPoolSize = 12;
    for (int i = 0; i < kPoolSize; ++i) {
        auto c = std::make_unique<Card3D>();
        c->Setup(objCom_, dx_, cam_);
        c->SetIsHand(true);
        c->SetTransform(
            { -25.0f, -15.0f, 10.0f }, // 山札の座標
            { 0.0f, 3.14159f, 0.0f },  // 裏向き
            { 0.0f, 0.0f, 0.0f }       // 見えないサイズ
        );
        cardPool_.push_back(std::move(c));
    }
}

void HandView3D::SetHoverIndex(int idx)
{
    if (hoverIndex_ != idx) {
        prevHoverIndex_ = hoverIndex_;
        hoverIndex_ = idx;
    }
    layoutDirty_ = true;
}

void HandView3D::Rebuild(const std::vector<CardInstance>& hand)
{
    handCards_ = hand;
    cards_.clear();
    cards_.reserve(hand.size());

    for (const auto& inst : hand) {
        const CardDef* def = db_->Find(inst.defId);
        if (!def) continue;

        auto c = std::make_unique<Card3D>();
        c->Initialize(objCom_, dx_, cam_, *def, inst);
        c->SetIsHand(true);
        cards_.push_back(std::move(c));
    }

    basePos_.assign(cards_.size(), {});
    baseRot_.assign(cards_.size(), {});
    baseScl_.assign(cards_.size(), { 1,1,1 });
    liftY_.assign(cards_.size(), 0.0f);

    hoverIndex_ = -1;
    previewIndex_ = -1;
    dragIndex_ = -1;
    dragDxPx_ = 0.0f;
    dragDyPx_ = 0.0f;
    dragActive_ = false;

    LayoutFan_();

    // 初回描画前に「HandView側の通常更新」を1回通す
    Update(1.0f / 60.0f);
}
void HandView3D::LayoutFan_()
{
    int n = (int)cards_.size();
    if (n <= 0) return;

    // 横方向の間隔
    const float xStep = 2.2f;

    // 奥へ重なる量
    const float zStep = 0.22f;

    // 少しだけ右肩上がりにしたいなら使う
    const float yStep = 0.02f;

    // 枚数が多いときは少し縮小
    float handScale = 1.0f;
    if (n >= 7) handScale = 0.95f;
    if (n >= 9) handScale = 0.90f;
    if (n >= 11) handScale = 0.85f;

    // 中央寄せ
    float startX = -((n - 1) * xStep) * 0.5f;

    // 手札全体の基準位置
    const float baseY = -13.0f;
    const float baseZ = 6.8f;

    for (int i = 0; i < n; ++i) {
        basePos_[i] = {
            startX + i * xStep,
            baseY + i * yStep,
            baseZ + i * zStep   // indexが後ろほど奥へ
        };

        // 傾けない
        baseRot_[i] = { 0.0f, 0.0f, 0.0f };

        baseScl_[i] = { handScale, handScale, handScale };

        cards_[i]->SetTransform(basePos_[i], baseRot_[i], baseScl_[i]);
    }

    layoutDirty_ = true;

}

void HandView3D::Update(float dt)
{
    if (!layoutDirty_ && discardingCards_.empty()) {
        return;
    }

    const float hoverLift = 0.6f;
    const float hoverFrontZ = 0.35f;
    const float follow = 18.0f;
    const float k = 1.0f - std::exp(-follow * dt);

    const float pxToWorldX = 0.03f;
    const float pxToWorldY = -0.03f;

    int n = (int)cards_.size();
    bool stillAnimating = false;

    for (int i = 0; i < n; ++i) {
        float targetLift = (i == hoverIndex_) ? hoverLift : 0.0f;
        liftY_[i] += (targetLift - liftY_[i]) * k;

        if (std::fabs(liftY_[i] - targetLift) < 0.001f) {
            liftY_[i] = targetLift;
        } else {
            stillAnimating = true;
        }

        Vector3 pos = basePos_[i];
        Vector3 rot = baseRot_[i];
        Vector3 scl = baseScl_[i];

        pos.y += liftY_[i];

        if (i == hoverIndex_ && !dragActive_ && previewIndex_ < 0) {
            pos.z -= hoverFrontZ;
            rot = { 0.0f, 0.0f, 0.0f };
            scl = {
                baseScl_[i].x * 1.02f,
                baseScl_[i].y * 1.02f,
                baseScl_[i].z * 1.02f
            };
        }

        if (dragActive_ && i == dragIndex_) {
            rot = { 0.0f, 0.0f, 0.0f };
            pos.x += dragDxPx_ * pxToWorldX;
            pos.y += dragDyPx_ * pxToWorldY;
            pos.y += 0.3f;
            pos.z = 4.0f;
            stillAnimating = true;
        }

        if (previewIndex_ >= 0 && i == previewIndex_) {
            pos = { 0.0f, 0.0f, 3.0f };
            rot = { 0.0f, 0.0f, 0.0f };
            scl = { 2.0f, 2.0f, 2.0f };
        }

        if (focusIndex_ >= 0 && i == focusIndex_) {
            pos = { -10.f, 2.0f, 3.0f };   // BattleControllerからセットされた座標
            rot = { 0.0f, 0.0f, 0.0f }; // プレイヤーに見えやすいよう正面に向ける
            scl = { 1.f, 1.f, 1.f }; // 少し大きくする
            stillAnimating = true;      // 座標を固定するために更新を継続させる
        }

        bool isDrag = (dragActive_ && i == dragIndex_);
        cards_[i]->SetTargetTransform(pos, rot, scl, isDrag);
        cards_[i]->Update(dt);

        Vector3 curPos = cards_[i]->GetWorldPos();
        float distSq = (curPos.x - pos.x) * (curPos.x - pos.x) +
            (curPos.y - pos.y) * (curPos.y - pos.y) +
            (curPos.z - pos.z) * (curPos.z - pos.z);
        if (distSq > 0.00001f) {
            stillAnimating = true;
        }

        
    }

    for (auto& entry : discardingCards_) {
        entry.timer += dt;
        if (entry.card) {
            entry.card->Update(dt);
        }
    }
    discardingCards_.erase(
        std::remove_if(discardingCards_.begin(), discardingCards_.end(),
            [this](DiscardingCard& entry) {
                if (entry.timer >= entry.duration) {
                    cardPool_.push_back(std::move(entry.card));
                    return true;
                }
                return false;
            }),
        discardingCards_.end()
    );
    if (!discardingCards_.empty()) {
        stillAnimating = true;
    }

    layoutDirty_ = stillAnimating;
}

void HandView3D::Draw()
{
    for (int i = 0; i < (int)cards_.size(); ++i) {
        if (i != previewIndex_ && i != focusIndex_) {
            cards_[i]->Draw();
        }
    }
}

void HandView3D::DrawDiscardingCardsObjectPost(GameApp& app)
{
    if (discardingCards_.empty()) {
        return;
    }

    const BloomParam baseParam = app.ObjectPost()->GetParam();
    for (auto& entry : discardingCards_) {
        if (!entry.card) {
            continue;
        }

        float progress = Clamp01_(entry.timer / entry.duration);
        BloomParam dissolveParam = MakeCardDissolveParam_(baseParam, progress);

        app.ObjectPost()->SetParam(dissolveParam);
        app.BeginObjectPostEffect();
        entry.card->Draw();
        app.EndObjectPostEffect();
        app.ObjCom()->SetGraphicsPipelineState();
    }
}

void HandView3D::DrawPreviewCard()
{
    if (previewIndex_ >= 0 && previewIndex_ < (int)cards_.size()) {
        cards_[previewIndex_]->Draw();
    }
    if (focusIndex_ >= 0 && focusIndex_ < (int)cards_.size()) {
        cards_[focusIndex_]->Draw();
    }
}

int HandView3D::PickIndexByMouse(int mouseX, int mouseY,
    const Matrix4x4& viewProj, float screenW, float screenH) const
{
    int best = -1;
    float bestD2 = 60.0f * 60.0f; // クリック半径(px)

    for (int i = 0; i < (int)cards_.size(); ++i) {
        Vector2 s{};
        if (!WorldToScreen_RowVector(cards_[i]->GetWorldPos(), viewProj, screenW, screenH, s)) {
            continue;
        }
        float dx = s.x - (float)mouseX;
        float dy = s.y - (float)mouseY;
        float d2 = dx * dx + dy * dy;

        if (d2 < bestD2) {
            bestD2 = d2;
            best = i;
        }
    }
    return best;
}

void HandView3D::SetDrag(int idx, float dxPx, float dyPx, bool active)
{
    if (dragIndex_ != idx || dragActive_ != active ||
        dragDxPx_ != dxPx || dragDyPx_ != dyPx) {
        layoutDirty_ = true;
    }

    dragIndex_ = idx;
    dragDxPx_ = dxPx;
    dragDyPx_ = dyPx;
    dragActive_ = active;
}

void HandView3D::Clear()
{
    handCards_.clear();
    cards_.clear();
    basePos_.clear();
    baseRot_.clear();
    baseScl_.clear();
    liftY_.clear();

    hoverIndex_ = -1;
    previewIndex_ = -1;
    dragIndex_ = -1;
    dragDxPx_ = 0.0f;
    dragDyPx_ = 0.0f;
    dragActive_ = false;
    prevHoverIndex_ = -1;
    layoutDirty_ = true;

}

#include "ScopedTimer.h"

void HandView3D::AddCard(const CardInstance& inst)
{
    ScopedTimer timer("HandView3D::AddCard");

    const CardDef* def = db_->Find(inst.defId);
    if (!def) {
        return;
    }

    handCards_.push_back(inst);

    std::unique_ptr<Card3D> c;
    if (!cardPool_.empty()) {
        {
            ScopedTimer t("  reuse card from pool");
            c = std::move(cardPool_.back());
            cardPool_.pop_back();
        }
        {
            ScopedTimer t("  Card3D::SetCardData");
            c->SetCardData(*def, inst);
        }
    } else {
        {
            ScopedTimer t("  create new Card3D");
            c = std::make_unique<Card3D>();
            c->Setup(objCom_, dx_, cam_);
            c->SetCardData(*def, inst);
            c->SetIsHand(true);
        }
    }

    c->SetIsHand(true);
    c->SetTransform({ -25.0f, -15.0f, 10.0f }, { 0.0f, 3.14159f, 0.0f }, { 0.0f, 0.0f, 0.0f });
    cards_.push_back(std::move(c));

    basePos_.push_back({});
    baseRot_.push_back({});
    baseScl_.push_back({ 1.0f, 1.0f, 1.0f });
    liftY_.push_back(0.0f);

    {
        ScopedTimer t("  LayoutFan_");
        LayoutFan_();
    }

    {
        ScopedTimer t("  Refresh all hand cards");
        Update(1.0f / 60.0f);
    }
}

void HandView3D::RemoveCardAt(int index)
{
    if (index < 0 || index >= static_cast<int>(cards_.size())) {
        return;
    }
    cardPool_.push_back(std::move(cards_[index]));

    handCards_.erase(handCards_.begin() + index);
    cards_.erase(cards_.begin() + index);
    basePos_.erase(basePos_.begin() + index);
    baseRot_.erase(baseRot_.begin() + index);
    baseScl_.erase(baseScl_.begin() + index);
    liftY_.erase(liftY_.begin() + index);

    if (hoverIndex_ == index) hoverIndex_ = -1;
    else if (hoverIndex_ > index) --hoverIndex_;

    if (previewIndex_ == index) previewIndex_ = -1;
    else if (previewIndex_ > index) --previewIndex_;

    if (dragIndex_ == index) {
        dragIndex_ = -1;
        dragActive_ = false;
        dragDxPx_ = 0.0f;
        dragDyPx_ = 0.0f;
    } else if (dragIndex_ > index) {
        --dragIndex_;
    }

    LayoutFan_();
    Update(1.0f / 60.0f);
}

std::unique_ptr<Card3D> HandView3D::ExtractCardAt(int index) {
    if (index < 0 || index >= static_cast<int>(cards_.size())) return nullptr;

    auto extractedCard = std::move(cards_[index]);

    handCards_.erase(handCards_.begin() + index);
    cards_.erase(cards_.begin() + index);
    basePos_.erase(basePos_.begin() + index);
    baseRot_.erase(baseRot_.begin() + index);
    baseScl_.erase(baseScl_.begin() + index);
    liftY_.erase(liftY_.begin() + index);

    if (hoverIndex_ == index) hoverIndex_ = -1;
    else if (hoverIndex_ > index) --hoverIndex_;
    if (previewIndex_ == index) previewIndex_ = -1;
    else if (previewIndex_ > index) --previewIndex_;
    if (dragIndex_ == index) { dragIndex_ = -1; dragActive_ = false; dragDxPx_ = 0; dragDyPx_ = 0; } else if (dragIndex_ > index) --dragIndex_;

    LayoutFan_();
    Update(1.0f / 60.0f);

    return extractedCard;
}

void HandView3D::AddDiscardingCard(std::unique_ptr<Card3D> card) {
    if (!card) return;
    const Vector3 pos = card->GetWorldPos();
    card->SetTargetTransform(pos, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, true);

    DiscardingCard entry{};
    entry.card = std::move(card);
    discardingCards_.push_back(std::move(entry));
    layoutDirty_ = true;
}

void HandView3D::RefreshLayout()
{
    LayoutFan_();
    Update(1.0f / 60.0f);
}

#ifdef USE_IMGUI
#include <imgui.h>
void HandView3D::DrawImGui()
{
    ImGui::Text("hoverIndex: %d", hoverIndex_);
    if (hoverIndex_ >= 0 && hoverIndex_ < (int)cards_.size()) {
        cards_[hoverIndex_]->DrawImGui("Hovered Card");
    } else {
        ImGui::Text("Hover a card to edit fix rot");
    }
}
#endif
