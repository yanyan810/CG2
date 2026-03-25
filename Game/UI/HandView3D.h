#pragma once
#include <vector>
#include <memory>
#include "Card3D.h"
#include "MathStruct.h"
#include "CardInstance.h"
#include "ScopedTimer.h"

class Camera;
class Object3dCommon;
class DirectXCommon;
class CardDatabase;

class HandView3D {
public:
    void Initialize(Object3dCommon* objCom, DirectXCommon* dx, Camera* cam, CardDatabase* db);

    void Rebuild(const std::vector<CardInstance>& hand);

    void Update(float dt);
    void Draw();

    void Clear();
    void AddCard(const CardInstance& inst);
    void RemoveCardAt(int index);
    void RefreshLayout();

    int PickIndexByMouse(int mouseX, int mouseY, const Matrix4x4& viewProj, float screenW, float screenH) const;

    void SetHoverIndex(int idx);
    void SetDrag(int idx, float dxPx, float dyPx, bool active);
    void SetPreviewIndex(int idx) { previewIndex_ = idx; layoutDirty_ = true; }

    void SetFocusIndex(int index) { focusIndex_ = index; }

    int GetHoverIndex() const { return hoverIndex_; }

#ifdef USE_IMGUI
    void DrawImGui();
#endif

private:
    void LayoutFan_();

    Object3dCommon* objCom_ = nullptr;
    DirectXCommon* dx_ = nullptr;
    Camera* cam_ = nullptr;
    CardDatabase* db_ = nullptr;

    std::vector<std::unique_ptr<Card3D>> cards_;
    std::vector<std::unique_ptr<Card3D>> cardPool_;
    std::vector<CardInstance> handCards_;

    std::vector<Vector3> basePos_;
    std::vector<Vector3> baseRot_;
    std::vector<Vector3> baseScl_;
    std::vector<float> liftY_;

    int hoverIndex_ = -1;
    int prevHoverIndex_ = -1;

    int dragIndex_ = -1;
    bool dragActive_ = false;
    float dragDxPx_ = 0.0f;
    float dragDyPx_ = 0.0f;

    int previewIndex_ = -1;

    bool layoutDirty_ = true;

    int focusIndex_ = -1;
};