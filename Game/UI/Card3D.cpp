#include "Card3D.h"
#include "Camera.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"

void Card3D::Initialize(Object3dCommon* objCom, DirectXCommon* dx, Camera* cam, const CardDef& def)
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
}

void Card3D::Draw()
{
    if (!frame_ || !art_) return;
    frame_->Draw();
    art_->DrawWithOverrideSrv(artSrv_);
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