#include "Card3D.h"
#include "Camera.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include <cmath>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// --------------------------------------------------
// 変数を配列に変更（[0]:場, [1]:手札）
// --------------------------------------------------

// コスト用の調整変数
static float g_costX[2] = { 0.05f, 0.05f };
static float g_costY[2] = { 0.0f, 0.0f };
static float g_costZ[2] = { -0.1f, -0.1f };
static float g_costScaleX[2] = { 1.0f, 1.0f };
static float g_costScaleY[2] = { 1.0f, 1.0f };
static float g_costScaleZ[2] = { 1.0f, 1.0f };
static float g_costRotX_deg[2] = { -90.0f, -90.0f };
static float g_costRotY_deg[2] = { 0.0f, 0.0f };
static float g_costRotZ_deg[2] = { 270.0f, 270.0f };

// マーク用の調整変数
static float g_suitX[2] = { 0.21f, 0.21f };
static float g_suitY[2] = { 0.25f, 0.25f };
static float g_suitZ[2] = { -0.1f, -0.1f };
static float g_suitScaleX[2] = { 0.9f, 0.9f };
static float g_suitScaleY[2] = { 0.9f, 0.9f };
static float g_suitScaleZ[2] = { 0.9f, 0.9f };
static float g_suitRotX_deg[2] = { -90.0f, -90.0f };
static float g_suitRotY_deg[2] = { 0.0f, 0.0f };
static float g_suitRotZ_deg[2] = { 270.0f, 270.0f };

// 番号用の調整変数
static float g_numberX[2] = { 2.260f, 2.260f };
static float g_numberY[2] = { -2.0f, -2.0f };
static float g_numberZ[2] = { -0.1f, -0.1f };
static float g_numberScaleX[2] = { 0.9f, 0.9f };
static float g_numberScaleY[2] = { 0.9f, 0.9f };
static float g_numberScaleZ[2] = { 0.75f, 0.75f };
static float g_numberRotX_deg[2] = { -90.0f, -90.0f };
static float g_numberRotY_deg[2] = { 0.0f, 0.0f };
static float g_numberRotZ_deg[2] = { 270.0f, 270.0f };

// カードの向きと大きさに合わせて、マークのズレ（オフセット）を計算する関数
static Vector3 CalcLocalOffset(const Vector3& offset, const Vector3& cardScale, const Vector3& cardRot)
{
    Vector3 scaledOffset = {
        offset.x * cardScale.x,
        offset.y * cardScale.y,
        offset.z * cardScale.z
    };

    Matrix4x4 rotM = Matrix4x4::RotateXYZ(cardRot.x, cardRot.y, cardRot.z);

    Vector4 v = MulRowVec4Mat4(
        { scaledOffset.x, scaledOffset.y, scaledOffset.z, 0.0f },
        rotM
    );

    return { v.x, v.y, v.z };
}

void Card3D::Initialize(
    Object3dCommon* objCom,
    DirectXCommon* dx,
    Camera* cam,
    const CardDef& def,
    const CardInstance& inst)
{
    Setup(objCom, dx, cam);
    SetCardData(def, inst);
}

void Card3D::Setup(
    Object3dCommon* objCom,
    DirectXCommon* dx,
    Camera* cam)
{
    if (!frame_) {
        frame_ = std::make_unique<Object3d>();
        frame_->Initialize(objCom, dx);
        frame_->SetCamera(cam);
        frame_->SetEnableLighting(0);
    }

    if (!art_) {
        art_ = std::make_unique<Object3d>();
        art_->Initialize(objCom, dx);
        art_->SetCamera(cam);
        art_->SetEnableLighting(0);
    }

    if (!costObj_) {
        costObj_ = std::make_unique<Object3d>();
        costObj_->Initialize(objCom, dx);
        costObj_->SetCamera(cam);
        costObj_->SetEnableLighting(0);
    }

    if (!suitObj_) {
        suitObj_ = std::make_unique<Object3d>();
        suitObj_->Initialize(objCom, dx);
        suitObj_->SetCamera(cam);
        suitObj_->SetEnableLighting(0);
    }

    if (!numberObjTens_) {
        numberObjTens_ = std::make_unique<Object3d>();
        numberObjTens_->Initialize(objCom, dx);
        numberObjTens_->SetCamera(cam);
        numberObjTens_->SetEnableLighting(0);
    }

    if (!numberObjOnes_) {
        numberObjOnes_ = std::make_unique<Object3d>();
        numberObjOnes_->Initialize(objCom, dx);
        numberObjOnes_->SetCamera(cam);
        numberObjOnes_->SetEnableLighting(0);
    }

}

void Card3D::SetCardData(const CardDef& def, const CardInstance& inst)
{
    ScopedTimer timer("Card3D::SetCardData");

    if (!frame_ || !art_ || !costObj_ || !suitObj_ || !numberObjTens_ || !numberObjOnes_) {
        return;
    }

    {
        ScopedTimer t("  frame SetModel");
        frame_->SetModel(def.frameModel.c_str());
    }

    {
        ScopedTimer t("  art SetModel");
        art_->SetModel(def.artModel.c_str());
    }

    {
        ScopedTimer t("  GetSrvHandleGPU");
        artSrv_ = TextureManager::GetInstance()->GetSrvHandleGPU(def.artTex);
    }

    std::string costModelPath = "cards/models/" + std::to_string(def.cost) + ".obj";
    {
        ScopedTimer t("  cost SetModel");
        costObj_->SetModel(costModelPath.c_str());
    }

    int tens = inst.number / 10;
    int ones = inst.number % 10;

    hasTensDigit_ = (inst.number >= 10);

    // 一の位は常に表示
    {
        std::string onesPath = "cards/models/" + std::to_string(ones) + ".obj";
        ScopedTimer t("  ones SetModel");
        numberObjOnes_->SetModel(onesPath.c_str());
    }

    // 十の位があるときだけ表示
    if (hasTensDigit_) {
        std::string tensPath = "cards/models/" + std::to_string(tens) + ".obj";
        ScopedTimer t("  tens SetModel");
        numberObjTens_->SetModel(tensPath.c_str());
    }

    std::string suitModelPath;
    switch (inst.suit) {
    case CardSuit::Spade:   suitModelPath = "cards/models/spade.obj"; break;
    case CardSuit::Heart:   suitModelPath = "cards/models/heart.obj"; break;
    case CardSuit::Diamond: suitModelPath = "cards/models/daiya.obj"; break;
    case CardSuit::Club:    suitModelPath = "cards/models/clover.obj"; break;
    default: break;
    }

    if (!suitModelPath.empty()) {
        ScopedTimer t("  suit SetModel");
        suitObj_->SetModel(suitModelPath.c_str());
    }

    frameColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    transformDirty_ = true;
    frameColorDirty_ = true;
    hasSubmittedOnce_ = false;
}

void Card3D::SetTransform(const Vector3& pos, const Vector3& rot, const Vector3& scale)
{
    pos_ = pos;
    rot_ = rot;
    scale_ = scale;

    // まずは必ず更新扱いにする
    transformDirty_ = true;
}

void Card3D::Update(float dt)
{
    if (!frame_ || !art_) return;

    if (hasSubmittedOnce_ && !transformDirty_ && !frameColorDirty_) {
        return;
    }

    Vector3 fixRot = rot_;
    fixRot.x += modelFixRot_.x;
    fixRot.y += modelFixRot_.y;
    fixRot.z += modelFixRot_.z;

    frame_->SetTranslate(pos_);
    frame_->SetRotate(fixRot);
    frame_->SetScale(scale_);
    if (frameColorDirty_) {
        frame_->SetMaterialColor(frameColor_);
    }
    frame_->Update(dt);

    Vector3 artPos = pos_;
    artPos.z += 0.01f;
    art_->SetTranslate(artPos);
    art_->SetRotate(fixRot);
    art_->SetScale(scale_);
    art_->Update(dt);

    int mode = isHand_ ? 1 : 0;

    if (costObj_) {
        Vector3 localOffset = { g_costX[mode], g_costY[mode], g_costZ[mode] };
        Vector3 rotatedOffset = CalcLocalOffset(localOffset, scale_, fixRot);

        Vector3 costPos = {
            pos_.x + rotatedOffset.x,
            pos_.y + rotatedOffset.y,
            pos_.z + rotatedOffset.z
        };

        costObj_->SetTranslate(costPos);
        costObj_->SetRotate(fixRot);
        costObj_->SetScale({
            scale_.x * g_costScaleX[mode],
            scale_.y * g_costScaleY[mode],
            scale_.z * g_costScaleZ[mode]
            });
        costObj_->Update(dt);
    }

    if (suitObj_) {
        Vector3 localOffset = { g_suitX[mode], g_suitY[mode], g_suitZ[mode] };
        Vector3 rotatedOffset = CalcLocalOffset(localOffset, scale_, fixRot);

        Vector3 suitPos = {
            pos_.x + rotatedOffset.x,
            pos_.y + rotatedOffset.y,
            pos_.z + rotatedOffset.z
        };

        suitObj_->SetTranslate(suitPos);
        suitObj_->SetRotate(fixRot);
        suitObj_->SetScale({
            scale_.x * g_suitScaleX[mode],
            scale_.y * g_suitScaleY[mode],
            scale_.z * g_suitScaleZ[mode]
            });
        suitObj_->Update(dt);
    }

    // 一の位
    if (numberObjOnes_) {
        float digitSpacing = 0.18f;
        float baseX = g_numberX[mode];
        if (hasTensDigit_) {
            baseX += digitSpacing * 0.5f;
        }

        Vector3 localOffset = { baseX, g_numberY[mode], g_numberZ[mode] };
        Vector3 rotatedOffset = CalcLocalOffset(localOffset, scale_, fixRot);

        Vector3 onesPos = {
            pos_.x + rotatedOffset.x,
            pos_.y + rotatedOffset.y,
            pos_.z + rotatedOffset.z
        };

        numberObjOnes_->SetTranslate(onesPos);
        numberObjOnes_->SetRotate(fixRot);
        numberObjOnes_->SetScale({
            scale_.x * g_numberScaleX[mode],
            scale_.y * g_numberScaleY[mode],
            scale_.z * g_numberScaleZ[mode]
            });
        numberObjOnes_->Update(dt);
    }

    // 十の位
    if (hasTensDigit_ && numberObjTens_) {
        float digitSpacing = 0.7f;
        float baseX = g_numberX[mode] - digitSpacing * 0.5f;

        Vector3 localOffset = { baseX, g_numberY[mode], g_numberZ[mode] };
        Vector3 rotatedOffset = CalcLocalOffset(localOffset, scale_, fixRot);

        Vector3 tensPos = {
            pos_.x + rotatedOffset.x,
            pos_.y + rotatedOffset.y,
            pos_.z + rotatedOffset.z
        };

        numberObjTens_->SetTranslate(tensPos);
        numberObjTens_->SetRotate(fixRot);
        numberObjTens_->SetScale({
            scale_.x * g_numberScaleX[mode],
            scale_.y * g_numberScaleY[mode],
            scale_.z * g_numberScaleZ[mode]
            });
        numberObjTens_->Update(dt);
    }

    transformDirty_ = false;
    frameColorDirty_ = false;
    hasSubmittedOnce_ = true;

}

void Card3D::Draw()
{
    if (!frame_ || !art_) return;
    frame_->Draw();
    art_->DrawWithOverrideSrv(artSrv_);

    // コストとマークと数字を描画
    if (costObj_) costObj_->Draw();
    if (suitObj_) suitObj_->Draw();
    if (hasTensDigit_ && numberObjTens_) numberObjTens_->Draw();
    if (numberObjOnes_) numberObjOnes_->Draw();

}
void Card3D::SetFrameColor(const Vector4& color)
{
    if (frameColor_.x == color.x && frameColor_.y == color.y &&
        frameColor_.z == color.z && frameColor_.w == color.w) {
        return;
    }

    frameColor_ = color;
    frameColorDirty_ = true;
}


void Card3D::ResetFrameColor()
{
    SetFrameColor({ 1.0f, 1.0f, 1.0f, 1.0f });
}

#ifdef USE_IMGUI
#include <imgui.h>
void Card3D::DrawImGui(const char* label)
{
    if (ImGui::TreeNode(label)) {
        ImGui::SliderFloat3("Model Fix Rot", &modelFixRot_.x, -3.14159f, 3.14159f);
        ImGui::TreePop();
    }

}
#endif
#ifdef USE_IMGUI
void Card3D::DrawAdjustImGui()
{
    ImGui::Begin("Icon Adjuster");

    // ★場と手札、どちらの設定をいじるか選択するボタン
    static int editMode = 0; // 0:場, 1:手札
    ImGui::RadioButton("Field (場のカード)", &editMode, 0); ImGui::SameLine();
    ImGui::RadioButton("Hand (手札のカード)", &editMode, 1);
    ImGui::Separator();

    // 選択されている方(editMode)の配列をいじる
    ImGui::Text("Cost Icon");
    ImGui::DragFloat("Cost X (Left/Right)", &g_costX[editMode], 0.01f);
    ImGui::DragFloat("Cost Y (Up/Down)", &g_costY[editMode], 0.01f);
    ImGui::DragFloat("Cost Z (Depth)", &g_costZ[editMode], 0.01f);
    ImGui::DragFloat("Cost Rot X", &g_costRotX_deg[editMode], 1.0f);
    ImGui::DragFloat("Cost Rot Y", &g_costRotY_deg[editMode], 1.0f);
    ImGui::DragFloat("Cost Rot Z", &g_costRotZ_deg[editMode], 1.0f);
    ImGui::DragFloat("Cost Scale X", &g_costScaleX[editMode], 0.01f);
    ImGui::DragFloat("Cost Scale Y", &g_costScaleY[editMode], 0.01f);
    ImGui::DragFloat("Cost Scale Z", &g_costScaleZ[editMode], 0.01f);

    ImGui::Separator();

    ImGui::Text("Suit Icon");
    ImGui::DragFloat("Suit X (Left/Right)", &g_suitX[editMode], 0.01f);
    ImGui::DragFloat("Suit Y (Up/Down)", &g_suitY[editMode], 0.01f);
    ImGui::DragFloat("Suit Z (Depth)", &g_suitZ[editMode], 0.01f);
    ImGui::DragFloat("Suit Rot X", &g_suitRotX_deg[editMode], 1.0f);
    ImGui::DragFloat("Suit Rot Y", &g_suitRotY_deg[editMode], 1.0f);
    ImGui::DragFloat("Suit Rot Z", &g_suitRotZ_deg[editMode], 1.0f);
    ImGui::DragFloat("Suit Scale X", &g_suitScaleX[editMode], 0.01f);
    ImGui::DragFloat("Suit Scale Y", &g_suitScaleY[editMode], 0.01f);
    ImGui::DragFloat("Suit Scale Z", &g_suitScaleZ[editMode], 0.01f);

    ImGui::Separator();

    ImGui::Text("Number Icon");
    ImGui::DragFloat("Number X (Left/Right)", &g_numberX[editMode], 0.01f);
    ImGui::DragFloat("Number Y (Up/Down)", &g_numberY[editMode], 0.01f);
    ImGui::DragFloat("Number Z (Depth)", &g_numberZ[editMode], 0.01f);
    ImGui::DragFloat("Number Rot X", &g_numberRotX_deg[editMode], 1.0f);
    ImGui::DragFloat("Number Rot Y", &g_numberRotY_deg[editMode], 1.0f);
    ImGui::DragFloat("Number Rot Z", &g_numberRotZ_deg[editMode], 1.0f);
    ImGui::DragFloat("Number Scale X", &g_numberScaleX[editMode], 0.01f);
    ImGui::DragFloat("Number Scale Y", &g_numberScaleY[editMode], 0.01f);
    ImGui::DragFloat("Number Scale Z", &g_numberScaleZ[editMode], 0.01f);

    ImGui::End();

 

}
#endif