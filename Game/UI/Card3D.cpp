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

void Card3D::Initialize(Object3dCommon* objCom, DirectXCommon* dx, Camera* cam, const CardDef& def, const CardInstance& inst)
{
    // 枠
    frame_ = std::make_unique<Object3d>();
    frame_->Initialize(objCom, dx);
    frame_->SetModel(def.frameModel.c_str());
    frame_->SetCamera(cam);
    frame_->SetEnableLighting(1);

    // 絵板
    art_ = std::make_unique<Object3d>();
    art_->Initialize(objCom, dx);
    art_->SetModel(def.artModel.c_str());
    art_->SetCamera(cam);
    art_->SetEnableLighting(0);

    // 絵テクスチャ
    TextureManager::GetInstance()->LoadTexture(def.artTex);
    artSrv_ = TextureManager::GetInstance()->GetSrvHandleGPU(def.artTex);

    // コストオブジェクト
    costObj_ = std::make_unique<Object3d>();
    costObj_->Initialize(objCom, dx);

    // コスト(1〜5)に応じて .obj ファイルを読み込む
    std::string costModelPath = "cards/models/" + std::to_string(def.cost) + ".obj";
    costObj_->SetModel(costModelPath.c_str());
    costObj_->SetCamera(cam);
    costObj_->SetEnableLighting(1);

    // マークオブジェクト
    suitObj_ = std::make_unique<Object3d>();
    suitObj_->Initialize(objCom, dx);

    // マークに応じて読み込む .obj ファイルを切り替える
    std::string suitModelPath = "";
    switch (inst.suit) {
    case CardSuit::Spade:   suitModelPath = "cards/models/spade.obj"; break;
    case CardSuit::Heart:   suitModelPath = "cards/models/heart.obj"; break;
    case CardSuit::Diamond: suitModelPath = "cards/models/daiya.obj"; break;
    case CardSuit::Club:    suitModelPath = "cards/models/clover.obj"; break;
    }
    suitObj_->SetModel(suitModelPath.c_str());
    suitObj_->SetCamera(cam);
    suitObj_->SetEnableLighting(1);


}

void Card3D::SetTransform(const Vector3& pos, const Vector3& rot, const Vector3& scale)
{
    pos_ = pos;
    rot_ = rot;
    scale_ = scale;
}

void Card3D::Update(float dt)
{
    if (!frame_ || !art_) return;

    Vector3 fixRot = rot_;
    fixRot.x += modelFixRot_.x;
    fixRot.y += modelFixRot_.y;
    fixRot.z += modelFixRot_.z;

    frame_->SetTranslate(pos_);
    frame_->SetRotate(fixRot);
    frame_->SetScale(scale_);
    frame_->Update(dt);

    Vector3 artPos = pos_;
    artPos.z += 0.01f;
    art_->SetTranslate(artPos);
    art_->SetRotate(fixRot);
    art_->SetScale(scale_);
    art_->Update(dt);

    int mode = isHand_ ? 1 : 0;

    // コストの配置
    if (costObj_) {
        Vector3 costRot = fixRot;

        Vector3 localOffset = { g_costX[mode], g_costY[mode], g_costZ[mode] };
        Vector3 rotatedOffset = CalcLocalOffset(localOffset, scale_, fixRot);

        Vector3 costPos = {
            pos_.x + rotatedOffset.x,
            pos_.y + rotatedOffset.y,
            pos_.z + rotatedOffset.z
        };

        costObj_->SetTranslate(costPos);
        costObj_->SetRotate(costRot);
        costObj_->SetScale({
            scale_.x * g_costScaleX[mode],
            scale_.y * g_costScaleY[mode],
            scale_.z * g_costScaleZ[mode]
            });
        costObj_->Update(dt);
    }

    // マークの配置
    if (suitObj_) {
        Vector3 suitRot = fixRot;

        Vector3 localOffset = { g_suitX[mode], g_suitY[mode], g_suitZ[mode] };
        Vector3 rotatedOffset = CalcLocalOffset(localOffset, scale_, fixRot);

        Vector3 suitPos = {
            pos_.x + rotatedOffset.x,
            pos_.y + rotatedOffset.y,
            pos_.z + rotatedOffset.z
        };

        suitObj_->SetTranslate(suitPos);
        suitObj_->SetRotate(suitRot);
        suitObj_->SetScale({
            scale_.x * g_suitScaleX[mode],
            scale_.y * g_suitScaleY[mode],
            scale_.z * g_suitScaleZ[mode]
            });
        suitObj_->Update(dt);
    }
}

void Card3D::Draw()
{
    if (!frame_ || !art_) return;
    frame_->Draw();
    art_->DrawWithOverrideSrv(artSrv_);

    // コストとマークを描画
    if (costObj_) costObj_->Draw();
    if (suitObj_) suitObj_->Draw();
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

    ImGui::End();
}
#endif