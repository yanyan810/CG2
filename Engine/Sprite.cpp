#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include <cassert>


// シェーダの入力と合わせた簡易頂点
struct SpriteVertex {
    float px, py, pz, pw; // POSITION
    float u, v;           // TEXCOORD
    float nx, ny, nz;     // NORMAL（使わないが入力合わせで保持）
};

void Sprite::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dx, std::string textureFilePath) {
    spriteCommon_ = spriteCommon;
    dx_ = dx;

    //単位行列を書き込んでおく
	textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);

    // === 頂点/インデックス ===
    vertexResource_ = dx->CreateBufferResource(sizeof(SpriteVertex) * 4);
    indexResource_ = dx->CreateBufferResource(sizeof(uint32_t) * 6);

    size_ = { 128.0f, 128.0f };// デフォルトサイズ

    // 頂点データ（画面ピクセル座標のまま。WVPは main 側で正射影にする想定）
    SpriteVertex* v = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&v));
    // 0: 左下 / 1: 左上 / 2: 右下 / 3: 右上
    v[0] = { 0.0f, 300.0f, 0.0f, 1.0f,  0.0f,1.0f,  0,0,-1 };
    v[1] = { 0.0f,   0.0f, 0.0f, 1.0f,  0.0f,0.0f,  0,0,-1 };
    v[2] = { 640.f, 300.0f,0.0f, 1.0f,  1.0f,1.0f,  0,0,-1 };
    v[3] = { 640.f,   0.0f,0.0f, 1.0f,  1.0f,0.0f,  0,0,-1 };
    // UnmapしなくてOK（永続Mapでも可）

    uint32_t* idx = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&idx));
    idx[0] = 0; idx[1] = 1; idx[2] = 2;
    idx[3] = 1; idx[4] = 3; idx[5] = 2;

    // ビュー
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(SpriteVertex) * 4;
    vertexBufferView_.StrideInBytes = sizeof(SpriteVertex);

    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    // === マテリアル（※重複を削除して1回だけ作る）
    materialResource_ = dx->CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = color_;
    materialData_->enableLighting = false;
    materialData_->uvTransform = Matrix4x4::MakeIdentity4x4();

    // === 変換（※重複を削除して1回だけ作る）
    transformResource_ = dx->CreateBufferResource(sizeof(TransformationMatrix));
    transformResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformData_));
    transformData_->WVP = Matrix4x4::MakeIdentity4x4();
    transformData_->World = Matrix4x4::MakeIdentity4x4();

}

// 座標-反映処理：position_ → transform.translate
void Sprite::Update(const Matrix4x4& view, const Matrix4x4& proj) {
    Matrix4x4 S = Matrix4x4::Scale(scale_);
    Matrix4x4 R = Matrix4x4::RotateXYZ(rotate_.x, rotate_.y, rotate_.z);
    Matrix4x4 T = Matrix4x4::Translation({ position_.x, position_.y, 0.0f });

    Matrix4x4 world = Matrix4x4::Multiply(Matrix4x4::Multiply(S, R), T);
    Matrix4x4 vp = Matrix4x4::Multiply(view, proj);
    Matrix4x4 wvp = Matrix4x4::Multiply(world, vp);

    transformData_->World = world;
    transformData_->WVP = wvp;


}


void Sprite::Draw() {
    assert(spriteCommon_ && "Sprite not initialized (spriteCommon_ is null)");
    assert(dx_ && "Sprite not initialized (dx_ is null)");
    assert(srv_.ptr != 0 && "Call SetTexture() before Draw()");

    if (srv_.ptr == 0 && srvSlot_ != UINT32_MAX) {
        srv_ = dx_->GetSRVGPUDescriptorHandle(static_cast<int>(srvSlot_));
    }
    assert(srv_.ptr != 0 && "SetTexture() か SetTextureSlot() でテクスチャを設定して下さい");


    auto* cmd = dx_->GetCommandList();

    // 呼び出し側不要：ここでPSO/RootSigをセット
    spriteCommon_->SetGraphicsPipelineState();

    cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);
    cmd->IASetIndexBuffer(&indexBufferView_);

    // Root 0=Material, 1=Transform, 2=Texture SRV（プロジェクトの順に合わせて）
    cmd->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    cmd->SetGraphicsRootConstantBufferView(1, transformResource_->GetGPUVirtualAddress());
    cmd->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex));

    cmd->DrawIndexedInstanced(6, 1, 0, 0, 0);
}