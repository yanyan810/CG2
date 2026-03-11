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
        c->Initialize(objCom_, dx_, cam_, *def, inst);
        c->SetIsHand(true);
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

    float spread = 1.2f;
    if (n >= 7) spread = 1.5f;
    if (n >= 9) spread = 1.8f;

    float start = -spread * 0.5f;
    float step = (n <= 1) ? 0.0f : (spread / (n - 1));

    float handScale = 1.2f;
    if (n >= 7) handScale = 1.05f;
    if (n >= 9) handScale = 0.95f;

    for (int i = 0; i < n; ++i) {
        float t = start + step * i;

        basePos_[i] = { t * 8.0f, -13.0f, 6.0f + std::abs(t) * 1.0f - i * 0.02f };
        baseRot_[i] = { 0.0f, t * 0.35f, 0.0f };
        baseScl_[i] = { handScale, handScale, handScale };

        cards_[i]->SetTransform(basePos_[i], baseRot_[i], baseScl_[i]);
    }
}

void HandView3D::Update(float dt)
{
    const float hoverLift = 0.6f;
    const float hoverFrontZ = 0.8f;   // hover時にどれだけ手前へ出すか
    const float follow = 18.0f;
    const float k = 1.0f - std::exp(-follow * dt);

    const float pxToWorldX = 0.03f;
    const float pxToWorldY = -0.03f;

    int n = (int)cards_.size();
    for (int i = 0; i < n; ++i) {

        float targetLift = (i == hoverIndex_) ? hoverLift : 0.0f;
        liftY_[i] += (targetLift - liftY_[i]) * k;

        Vector3 pos = basePos_[i];
        Vector3 rot = baseRot_[i];
        Vector3 scl = baseScl_[i];

        pos.y += liftY_[i];

        // --- hover中：少し手前へ ---
        if (i == hoverIndex_ && !dragActive_ && previewIndex_ < 0) {
            pos.z -= hoverFrontZ;   // あなたの座標系では小さいほど手前
            rot = { 0.0f, 0.0f, 0.0f };

            scl = {
    baseScl_[i].x * 1.05f,
    baseScl_[i].y * 1.05f,
    baseScl_[i].z * 1.05f
            };

        }

        // --- ドラッグ中 ---
        if (dragActive_ && i == dragIndex_) {
            rot = { 0.0f, 0.0f, 0.0f };
            pos.x += dragDxPx_ * pxToWorldX;
            pos.y += dragDyPx_ * pxToWorldY;
            pos.y += 0.3f;
            pos.z = 4.0f;           // 最前面寄り
        }

        // --- プレビュー中 ---
        if (previewIndex_ >= 0 && i == previewIndex_) {
            Vector3 frontPos{ 0.0f, 0.0f, 3.0f };
            Vector3 frontRot{ 0.0f, 0.0f, 0.0f };
            Vector3 frontScl{ 2.0f, 2.0f, 2.0f };

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