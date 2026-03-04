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

void HandView3D::Rebuild(const std::vector<int>& handDefIds)
{
    handDefIds_ = handDefIds;
    cards_.clear();
    cards_.reserve(handDefIds.size());

    for (int id : handDefIds) {
        const CardDef* def = db_->Find(id);
        if (!def) continue;

        auto c = std::make_unique<Card3D>();
        c->Initialize(objCom_, dx_, cam_, *def);
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

        basePos_[i] = { t * 8.0f, -3.0f, 6.0f + std::abs(t) * 1.0f };
        baseRot_[i] = { 0.0f, t * 0.35f, 0.0f };
        baseScl_[i] = { 1.2f, 1.2f, 1.2f };

        // ここでは基準だけ適用
        cards_[i]->SetTransform(basePos_[i], baseRot_[i], baseScl_[i]);
    }
}

void HandView3D::Update(float dt)
{
    // 好みで調整
    const float hoverLift = 0.6f;     // どれだけ上げるか（ワールド単位）
    const float follow = 18.0f;       // 追従速度（大きいほどキビキビ）
    const float k = 1.0f - std::exp(-follow * dt); // dt依存で安定する補間係数

    int n = (int)cards_.size();
    for (int i = 0; i < n; ++i) {

        float target = (i == hoverIndex_) ? hoverLift : 0.0f;
        liftY_[i] += (target - liftY_[i]) * k; // ふわっと

        Vector3 pos = basePos_[i];
        pos.y += liftY_[i];

        cards_[i]->SetTransform(pos, baseRot_[i], baseScl_[i]);
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