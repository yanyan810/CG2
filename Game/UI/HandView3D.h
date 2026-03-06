#pragma once
#include <vector>
#include <memory>
#include "Card3D.h"
#include "MathStruct.h"

class Camera;
class Object3dCommon;
class DirectXCommon;
class CardDatabase;

class HandView3D {
public:
    void Initialize(Object3dCommon* objCom, DirectXCommon* dx, Camera* cam, CardDatabase* db);

    void Rebuild(const std::vector<int>& handDefIds);

    void Update(float dt);
    void Draw();

    int PickIndexByMouse(int mouseX, int mouseY, const Matrix4x4& viewProj, float screenW, float screenH) const;

    // 外から hover をセット
    void SetHoverIndex(int idx) { hoverIndex_ = idx; }

    // ドラッグ中の情報
    void SetDrag(int idx, float dxPx, float dyPx, bool active);

    // プレビュー（目の前に出すカード）
    void SetPreviewIndex(int idx) { previewIndex_ = idx; }

#ifdef USE_IMGUI
    void DrawImGui();
#endif

private:
    void LayoutFan_(); // 扇状に並べる

    Object3dCommon* objCom_ = nullptr;
    DirectXCommon* dx_ = nullptr;
    Camera* cam_ = nullptr;
    CardDatabase* db_ = nullptr;

    std::vector<int> handDefIds_;
    std::vector<std::unique_ptr<Card3D>> cards_;

    // ★追加：レイアウトの基準（hoverで動かす前の値）
    std::vector<Vector3> basePos_;
    std::vector<Vector3> baseRot_;
    std::vector<Vector3> baseScl_;

    // ★追加：ふわっと上げる用（カードごとの現在の持ち上げ量）
    std::vector<float> liftY_;

    int hoverIndex_ = -1;

    int dragIndex_ = -1;
    bool dragActive_ = false;
    float dragDxPx_ = 0.0f;
    float dragDyPx_ = 0.0f;

    int previewIndex_ = -1;

};