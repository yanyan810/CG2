#pragma once
#include <memory>
#include <d3d12.h>

#include "Object3d.h"
#include "Object3dCommon.h"
#include "TextureManager.h"
#include "CardDef.h"
#include "CardInstance.h"
#include "ScopedTimer.h"

class Camera;
class DirectXCommon;

class Card3D {
public:

    void Initialize(
        Object3dCommon* objCom,
        DirectXCommon* dx,
        Camera* cam,
        const CardDef& def,
        const CardInstance& inst);

    void SetTransform(const Vector3& pos, const Vector3& rot, const Vector3& scale);
    void Update(float dt);
    void Draw();
    void DrawFrameOnly();

    Vector3 GetWorldPos() const { return pos_; }
    Vector3 GetWorldPointFromLocal(const Vector3& localOffset) const;
    void SetIsHand(bool isHand) { isHand_ = isHand; }
    void SetCamera(Camera* camera);

    void Setup(
        Object3dCommon* objCom,
        DirectXCommon* dx,
        Camera* cam);

    void SetCardData(const CardDef& def, const CardInstance& inst);
    bool IsSameCardData(const CardDef& def, const CardInstance& inst) const;


    //特定のカードの枠の変更
    void SetFrameColor(const Vector4& color);
    void ResetFrameColor();

#ifdef USE_IMGUI
    void DrawImGui(const char* label = "Card3D");
#endif

    void SetModelFixRot(const Vector3& r) { modelFixRot_ = r; }
    Vector3 GetModelFixRot() const { return modelFixRot_; }
    static void DrawAdjustImGui();

    void SetTargetTransform(const Vector3& pos, const Vector3& rot, const Vector3& scale, bool instant = false);

    void SetIsPreview(bool flag) { isPreview_ = flag; }

    void SetShowCost(bool show) { showCost_ = show; }

    void SetShowZeroCount(bool show) { showZeroCount_ = show; }

    void SetCount(int count);

    void SetGlitter(float intensity);

private:

    std::unique_ptr<Object3d> frame_;
    std::unique_ptr<Object3d> art_;
    std::unique_ptr<Object3d> costObj_;
    std::unique_ptr<Object3d> suitObj_;
    std::unique_ptr<Object3d> numberObjTens_;
    std::unique_ptr<Object3d> numberObjOnes_;
    std::unique_ptr<Object3d> countObj_;

    D3D12_GPU_DESCRIPTOR_HANDLE artSrv_{};

    Vector3 pos_{ 0.0f, 0.0f, 0.0f };
    Vector3 rot_{ 0.0f, 0.0f, 0.0f };
    Vector3 scale_{ 1.0f, 1.0f, 1.0f };

    Vector3 targetPos_{ 0.0f, 0.0f, 0.0f };
    Vector3 targetRot_{ 0.0f, 0.0f, 0.0f };
    Vector3 targetScale_{ 1.0f, 1.0f, 1.0f };

    Vector3 modelFixRot_{ 0.0f, 0.0f, 0.0f };
    Vector4 frameColor_{ 1.0f, 1.0f, 1.0f, 1.0f };

    bool isHand_ = false;
    bool transformDirty_ = true;
    bool frameColorDirty_ = true;
    bool hasSubmittedOnce_ = false;

	// 10の位の数字を描画するオブジェクトが必要かどうか
    bool hasTensDigit_ = false;

    bool isPreview_ = false;
    bool showCost_ = true;
    bool showZeroCount_ = true;

    int count_ = 0;
    int currentDefId_ = -1;
    int currentNumber_ = -1;
    CardSuit currentSuit_ = CardSuit::Spade;

    float glitterIntensity_ = 0.0f;
    float glitterTimer_ = 0.0f;
};
