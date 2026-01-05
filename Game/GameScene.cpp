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

    input_ = app.GetInput();
    assert(input_); // ここでnullなら初期化順が悪い

    camera_ = std::make_unique<Camera>();

    // 斜め上・少し後ろ
    camera_->SetTranslate({
        0.0f,   // X
        20.0f,   // Y（高さ）
       -50.0f   // Z（後ろ）
        });

    // 少し下向き（ラジアン）
    camera_->SetRotate({
        0.35f,  // X回転（見下ろし）
        0.0f,   // Y
        0.0f
        });


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

    //player
    player_ = std::make_unique<Player>();
    player_->Initialize(app.ObjCom(), app.Dx(), camera_.get());

    //enemy
    enemyMgr_.Initialize(app.ObjCom(), app.Dx(), camera_.get());

    // テストスポーン
    enemyMgr_.Spawn(EnemyType::Melee, { 5.0f, 0.0f ,0.0f});
    enemyMgr_.Spawn(EnemyType::Shooter, { 9.0f, 0.0f ,0.0f});
    // bossはステージ5で


}

void GameScene::OnExit(GameApp& /*app*/) {
    player_.reset(); // ★追加
    particle_.reset();
    objB_.reset();
    objA_.reset();
    sprite_.reset();
    camera_.reset();
    debugTitleParticle_.reset();
}


void GameScene::Update(GameApp& app, float dt) {
    // ESC で終了（Input クラス持ってるなら差し替え）
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        app.RequestQuit();
        return;
    }

    if (!input_) return; // 念のため

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

    if (player_) {
        player_->Update(dt, *input_, enemyMgr_);
    }

    // enemyMgr_ に渡す playerPos を Player から取る
    Vector2 playerPos2D{ 0.0f, 0.0f };
    if (player_) {
        playerPos2D = player_->GetPos2D();
    }

    float playerZ = 15.0f;
    if (player_) {
        playerZ = player_->GetZ(); // 追加したgetter
    }
    enemyMgr_.Update(dt, playerPos2D, playerZ);
}

void GameScene::Draw(GameApp& app) {
    auto* dx = app.Dx();
    auto* srv = app.Srv();
    auto* cmd = dx->GetCommandList();


    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 3D
    app.ObjCom()->SetGraphicsPipelineState();
    //if (objA_) objA_->Draw();
    //if (objB_) objB_->Draw();

    //player
    if (player_) player_->Draw();

    // ★攻撃判定 可視化（当たり判定が有効なフレームだけ描く）
    if (player_ && debugHitboxObj_) {
        const PlayerCombo* combo = player_->GetCombo();
        if (combo) {
            AABB2 hb{};
            if (combo->GetDebugHitBox(hb)) {
                // AABB2 は center(x,y) と half(hx,hy)
                const float w = hb.hx * 2.0f;
                const float h = hb.hy * 2.0f;

                // plane.obj が XZ 平面（地面）向きなら、XYにするために90度回す
                debugHitboxObj_->SetRotate({ 1.570796f, 0.0f, 0.0f });

                // サイズ
                debugHitboxObj_->SetScale({ w, h, 1.0f });

                // Zはプレイヤーと同じ位置に（少しだけ前に出すとZ-fighting避け）
                float z = player_->GetZ() + 0.05f;
                debugHitboxObj_->SetTranslate({ hb.x, hb.y, z });

                debugHitboxObj_->Update();
                debugHitboxObj_->Draw();
            }
        }
    }

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
      //  sprite_->Draw();
    }

    // GameScene.cpp Draw の 3D 描画のところ
    enemyMgr_.Draw();

}
