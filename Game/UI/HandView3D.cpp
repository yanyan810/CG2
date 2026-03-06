#include "HandView3D.h"
#include "CardDatabase.h"
#include "WinApp.h"

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

void HandView3D::Initialize(Object3dCommon* objCom, DirectXCommon* dx, Camera* cam, CardDatabase* db)
{
    objCom_ = objCom;
    dx_ = dx;
    cam_ = cam;
    db_ = db;
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
        c->Initialize(objCom_, dx_, cam_, *def);
        // あとで inst.number / inst.suit を見た目に反映できる
        cards_.push_back(std::move(c));
    }

    // ★基準Transform/持ち上げ配列をカード数に合わせる
    basePos_.assign(cards_.size(), {});
    baseRot_.assign(cards_.size(), {});
    baseScl_.assign(cards_.size(), { 1,1,1 });
    liftY_.assign(cards_.size(), 0.0f);

    LayoutFan_();
}

void HandView3D::LayoutFan_()
{
    int n = (int)cards_.size();
    if (n <= 0) return;

    float start = -0.6f;
    float step = (n <= 1) ? 0.0f : (1.2f / (n - 1));

    for (int i = 0; i < n; ++i) {
        float t = start + step * i; // -0.6..0.6

        basePos_[i] = { t * 8.0f, -13.0f, 6.0f + std::abs(t) * 1.0f };
        baseRot_[i] = { 0.0f, t * 0.35f, 0.0f };
        baseScl_[i] = { 1.2f, 1.2f, 1.2f };

        // ここでは基準だけ適用
        cards_[i]->SetTransform(basePos_[i], baseRot_[i], baseScl_[i]);
    }
}

void HandView3D::Update(float dt)
{
    const float hoverLift = 0.6f;
    const float follow = 18.0f;
    const float k = 1.0f - std::exp(-follow * dt);

    // px -> world のざっくり換算（好みで調整）
    const float pxToWorldX = 0.01f;
    const float pxToWorldY = -0.01f; // 画面Y下向きなので符号逆が自然なことが多い

    int n = (int)cards_.size();
    for (int i = 0; i < n; ++i) {

        // --- 基本のhover持ち上げ ---
        float targetLift = (i == hoverIndex_) ? hoverLift : 0.0f;
        liftY_[i] += (targetLift - liftY_[i]) * k;

        Vector3 pos = basePos_[i];
        Vector3 rot = baseRot_[i];
        Vector3 scl = baseScl_[i];

        pos.y += liftY_[i];

        // --- ドラッグ中：カードをスライド ---
        if (dragActive_ && i == dragIndex_) {
            pos.x += dragDxPx_ * pxToWorldX;
            pos.y += dragDyPx_ * pxToWorldY;
            // ちょい持ち上げ強めでも良い
            pos.y += 0.3f;
        }

        // --- プレビュー中：目の前に出す ---
        if (previewIndex_ >= 0 && i == previewIndex_) {
            // 目の前（数値は好みで）
            Vector3 frontPos{ 0.0f, 0.0f, 3.0f };
            Vector3 frontRot{ 0.0f, 0.0f, 0.0f };
            Vector3 frontScl{ 2.0f, 2.0f, 2.0f };

            // ふわっと遷移したいなら補間してもOK（いまは即反映）
            pos = frontPos;
            rot = frontRot;
            scl = frontScl;
        }

        cards_[i]->SetTransform(pos, rot, scl);
        cards_[i]->Update(dt);
    }
}

void HandView3D::Draw()
{
    for (auto& c : cards_) c->Draw();
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
    dragIndex_ = idx;
    dragDxPx_ = dxPx;
    dragDyPx_ = dyPx;
    dragActive_ = active;
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