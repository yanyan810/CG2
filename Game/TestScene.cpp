#include "TestScene.h"
#include "GameApp.h"
#include "Input.h"
#include "Camera.h"
#include "Player.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "SrvManager.h"

#include <d3d12.h>
#include <cassert>

void TestScene::OnEnter(GameApp& app) {
  //  TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

    input_ = app.GetInput();
    assert(input_);

    camera_ = std::make_unique<Camera>();

    // GameSceneと同じカメラでOK
    camera_->SetTranslate({ 0.0f, 20.0f, -50.0f });
    camera_->SetRotate({ 0.35f, 0.0f, 0.0f });

    app.ObjCom()->SetDefaultCamera(camera_.get());

    // Player
    player_ = std::make_unique<Player>();
    player_->Initialize(app.ObjCom(), app.Dx(), camera_.get());
    player_->SetSpawnPos({ -12.0f, 0.0f, 15.0f }); // 好みで調整

    // EnemyManager
    enemyMgr_.Initialize(app.ObjCom(), app.Dx(), camera_.get());

    // ★真ん中に「動かない敵」を1体
    const Vector3 enemyPos{ 0.0f, 0.0f, 15.0f };
    enemyMgr_.Spawn(EnemyType::Melee, enemyPos);

    // ★凍結（GetEnemies() は PlayerCombo でも使ってるので存在してる前提）
    auto& enemies = enemyMgr_.GetEnemies();
    if (!enemies.empty()) {
        enemies.back().SetInvincible(true); // 死なない
        enemies.back().SetAIDisabled(true); // AI止める（でも吹き飛ぶ）
    }

	TextureManager::GetInstance()->LoadTexture("resources/ui/text1.png");

    playTxst_ = std::make_unique<Sprite>();
    playTxst_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text1.png");
    playTxst_->AdjustTextureSize();
    playTxst_->SetScale({ 1.0f, 1.0f ,1.0f });

    // どこかで（OnEnterの中）
    auto* mgr = ModelManager::GetInstance();
    mgr->LoadModel("ground/ground.obj");   // resources/ground/ground.obj を想定

    ground_ = std::make_unique<Object3d>();
    ground_->Initialize(app.ObjCom(), app.Dx());
    ground_->SetCamera(camera_.get());
    ground_->SetModel("ground/ground.obj");

    // 位置・大きさは好みで調整
    ground_->SetTranslate({ 0.0f, -5.0f, 0.0f });
    ground_->SetScale({ 1.0f, 1.0f, 1.0f });
    ground_->SetRotate({ 0.0f, 0.0f, 0.0f });
    ground_->Update();

}

void TestScene::OnExit(GameApp& /*app*/) {
    player_.reset();
    camera_.reset();
    enemyMgr_.Clear();
}

void TestScene::Update(GameApp& app, float dt) {
    if (!input_) return;

    camera_->Update();

    if (player_) {
        player_->Update(dt, *input_, enemyMgr_);
    }

    Vector2 playerPos2D = player_->GetPos2D();
    float playerZ = player_->GetZ();

    enemyMgr_.Update(dt, playerPos2D, playerZ, *player_);

    // ===============================
    // ★ クランプ到達チェック
    // ===============================
    const float zNear = -10.0f;
    const float zFar = 20.0f;
    const float xMaxNear = 15.0f;
    const float xMaxFar = 20.0f;

    float z = player_->GetZ();
    float t = (z - zNear) / (zFar - zNear);
    t = std::clamp(t, 0.0f, 1.0f);
    float xMax = xMaxNear + (xMaxFar - xMaxNear) * t;

    float x = player_->GetX();

    // ★ 右端に到達したら GameScene へ

    if (!reachedEdge_ && x >= xMax - 0.01f) {
        reachedEdge_ = true;
        RequestChangeScene_("Game");
    }

#ifdef USE_IMGUI

    // ===== ImGui =====
    ImGui::Begin("Camera Debug");

    ImGui::End();

#endif // DEBUG

}


void TestScene::Draw(GameApp& app) {
    auto* cmd = app.Dx()->GetCommandList();
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ===== 3D =====
    app.ObjCom()->SetGraphicsPipelineState();

    if (ground_) ground_->Draw();

    if (player_) player_->Draw();
    enemyMgr_.Draw();

    // ===== 2D (Sprite) =====
    app.SpriteCom()->SetGraphicsPipelineState();

    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0,
        float(WinApp::kClientWidth),
        float(WinApp::kClientHeight),
        0, 100
    );

    if (playTxst_) {
        playTxst_->Update(view, proj);
        playTxst_->Draw();
    }
}
