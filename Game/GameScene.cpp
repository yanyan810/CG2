#include "GameScene.h"
#include "GameApp.h"

#include "Camera.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "Particle.h"
#include "ParticleCommon.h"
#include "TextureManager.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "WinApp.h"
#include "Matrix4x4.h"

#include <d3d12.h>

GameScene::~GameScene() = default;

void GameScene::OnEnter(GameApp& app) {
    // テクスチャやモデルのロード（必要なものをここで）
    TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

    camera_ = std::make_unique<Camera>();
    camera_->SetRotate({ 0,0,0 });
    camera_->SetTranslate({ 0,3,-10 });

    // ObjCommon は GameApp が持つ。カメラ設定だけここで。
    app.ObjCom()->SetDefaultCamera(camera_.get());

    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/uvChecker.png");
    sprite_->AdjustTextureSize();

    objA_ = std::make_unique<Object3d>();
    objA_->Initialize(app.ObjCom(), app.Dx());
    objA_->SetModel("plane.obj");
    objA_->SetTranslate({ 0,0,20 });
    objA_->SetCamera(camera_.get());

    objB_ = std::make_unique<Object3d>();
    objB_->Initialize(app.ObjCom(), app.Dx());
    objB_->SetModel("fence/fence.obj");
    objB_->SetTranslate({ 2,0,20 });
    objB_->SetCamera(camera_.get());

    particle_ = std::make_unique<Particle>();
    particle_->Initialize(app.ParticleCom(), app.Dx(), app.Srv());
    particle_->SetModel("plane.obj");
    particle_->SetTranslate({ -2, 0, 0 });
    particle_->SetCamera(camera_.get());

    debugTitleParticle_ = std::make_unique<Particle>();
    debugTitleParticle_->Initialize(app.ParticleCom(), app.Dx(), app.Srv());
    debugTitleParticle_->SetModel("plane.obj");
    debugTitleParticle_->SetCamera(camera_.get()); // ← GameSceneで普段使ってるカメラ


}

void GameScene::OnExit(GameApp& /*app*/) {
    particle_.reset();
    objB_.reset();
    objA_.reset();
    sprite_.reset();
    camera_.reset();
    debugTitleParticle_.reset();
}

void GameScene::Update(GameApp& app, float /*dt*/) {
    // ESC で終了（Input クラス持ってるなら差し替え）
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        app.RequestQuit();
        return;
    }

    camera_->Update();

    // 物体回転サンプル
    if (objA_) {
        auto r = objA_->GetRotate();
        r.y += 0.02f;
        objA_->SetRotate(r);
        objA_->Update();
    }
    if (objB_) {
        auto r = objB_->GetRotate();
        r.y -= 0.01f;
        objB_->SetRotate(r);
        objB_->Update();
    }

    if (particle_) {
        particle_->SpawnParticle();
        particle_->Update();
    }

    debugTitleParticle_->SpawnParticle();
    debugTitleParticle_->Update();


}

void GameScene::Draw(GameApp& app) {
    auto* dx = app.Dx();
    auto* srv = app.Srv();
    auto* cmd = dx->GetCommandList();


    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 3D
    app.ObjCom()->SetGraphicsPipelineState();
    if (objA_) objA_->Draw();
    if (objB_) objB_->Draw();

    // Particle
    app.ParticleCom()->SetGraphicsPipelineState();
    // particle_ が自分で Draw を持ってるならここで
    // particle_->Draw();

    debugTitleParticle_->Draw();


    // 2D sprite
    app.SpriteCom()->SetGraphicsPipelineState();
    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0, 100);

    if (sprite_) {
        sprite_->Update(view, proj);
        sprite_->Draw();
    }

}
