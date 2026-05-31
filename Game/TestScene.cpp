#include "TestScene.h"
#include "GameApp.h"
#include "Input.h"
#include "Camera.h"
#include "Player.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "ParticleManager.h"

#include <d3d12.h>
#include <cassert>
#include <cmath>

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
    player_->SetSpawnPos({ -12.0f, 0.0f, 5.0f }); // 好みで調整

    // EnemyManager
    enemyMgr_.Initialize(app.ObjCom(), app.Dx(), camera_.get());

    // ★真ん中に敵を1体（テスト用）
    const Vector3 enemyPos{ 0.0f, 0.0f, 5.0f };
    enemyMgr_.Spawn(EnemyType::Boss, enemyPos);

    actionDirector_.Initialize(app.SpriteCom(), app.Dx(), app.ObjCom());
    if (const ActionSequenceProfile* profile = app.FindActionSequenceProfile("test_attack")) {
        actionDirector_.SetProfile(*profile);
        actionDirector_.SetProfilePath("resources/sequences/test_attack.json");

        player_->SetSpawnPos(profile->playerPos);
        if (!enemyMgr_.GetEnemies().empty()) {
            enemyMgr_.GetEnemies().front().SetPosition(profile->enemyPos);
        }
    }

	TextureManager::GetInstance()->LoadTexture("resources/ui/text1.png");

    playTxst_ = std::make_unique<Sprite>();
    playTxst_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text1.png");
    playTxst_->AdjustTextureSize();
    playTxst_->SetScale({ 1.0f, 1.0f ,1.0f });
	playTxst_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    light_.dirIntensity = 1.6f;
    light_.pointIntensity = 2.5f;
    light_.spotIntensity = 0.0f;

    //player_->SetLighting(light_);
    enemyMgr_.SetLighting(light_);


    // どこかで（OnEnterの中）
    //auto* mgr = ModelManager::GetInstance();
    //mgr->LoadModel("ground/ground.obj");   // resources/ground/ground.obj を想定

    ground_ = std::make_unique<Object3d>();
    ground_->Initialize(app.ObjCom(), app.Dx());
    ground_->SetCamera(camera_.get());
    ground_->SetModel("ground/ground.obj");

    

    // 位置・大きさは好みで調整
    ground_->SetTranslate({ 0.0f, -5.0f, 0.0f });
    ground_->SetScale({ 1.0f, 1.0f, 1.0f });
    ground_->SetRotate({ 0.0f, 0.0f, 0.0f });
    ground_->SetEnableLighting(2);     // 2はハーフランバート
    ground_->SetIntensity(2.0f);
    ground_->SetLightColor(light_.dirColor);
    ground_->SetEnableLighting(0);
    // Groundは pointだけ使う
    groundLight_ = light_;              // とりあえず既存をコピーしてもOK
    groundLight_.dirIntensity = 0.0f;   // ★Directional 無効
    groundLight_.spotIntensity = 0.0f;  // ★Spot 無効

    groundLight_.pointIntensity = 16.0f;
    groundLight_.pointPos = { 0.0f, -42.0f, -1.0f };
    groundLight_.pointRadius = 200.0f;
    groundLight_.pointDecay = 1.0f;
    groundLight_.pointColor = { 1.0f, 1.0f, 1.0f }; // Vector3想定

    // Spot ON
    groundLight_.spotIntensity = 20.0f;
    groundLight_.spotPos = { 0.0f, 15.0f, 15.0f };

    // ★direction は「どこを向くか」
    // とりあえず地面の中央へ向ける（後で毎フレ更新してもOK）
    Vector3 target = { 0.0f, 0.0f, 15.0f };
    Vector3 d = { target.x - groundLight_.spotPos.x, target.y - groundLight_.spotPos.y, target.z - groundLight_.spotPos.z };
    {
        float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        if (len > 1e-6f) { d.x /= len; d.y /= len; d.z /= len; }
    }
    groundLight_.spotDir = d;

    groundLight_.spotDistance = 80.0f;
    groundLight_.spotDecay = 1.0f;

    // 角度（degree管理してる前提）
    groundLight_.spotAngleDeg = 25.0f;
    groundLight_.spotFalloffStartDeg = 15.0f;
    groundLight_.spotColor = { 1.0f, 1.0f, 1.0f };

    // --- Spot マーカー ---
    spotMarker_ = std::make_unique<Object3d>();
    spotMarker_->Initialize(app.ObjCom(), app.Dx());
    spotMarker_->SetCamera(camera_.get());
    spotMarker_->SetModel("cube/cube.obj");
    spotMarker_->SetEnableLighting(0);
    spotMarker_->SetMaterialColor({ 0, 1, 1, 1 }); // シアン
    spotMarker_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

    // PointLightマーカー
    pointMarker_ = std::make_unique<Object3d>();
    pointMarker_->Initialize(app.ObjCom(), app.Dx());
    pointMarker_->SetCamera(camera_.get());
    pointMarker_->SetModel("cube/cube.obj");

    // 見た目
    pointMarker_->SetEnableLighting(0);                 // ★ライトの影響を受けない
    pointMarker_->SetMaterialColor({ 1, 1, 0, 1 });     // 黄色とか（好みで）
    pointMarker_->SetShininess(1.0f);
    pointMarker_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

    skyDome_ = std::make_unique<Object3d>();
    skyDome_->Initialize(app.ObjCom(), app.Dx());
    skyDome_->SetModel("skydome/SkyDome.obj");

    // ★スカイドームは基本「ライト無視」
    skyDome_->SetEnableLighting(0);              // ← あなたの仕様の「無照明モード」に合わせて
    skyDome_->SetMaterialColor({ 1,1,1,1 });       // 念のため
    skyDome_->SetShininess(1.0f);                // 影響しないけど保険
    skyDome_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

    normalCube_ = std::make_unique<Object3d>();
    normalCube_->Initialize(app.ObjCom(), app.Dx());
    normalCube_->SetCamera(camera_.get());
    normalCube_->SetModel("cube/cube.obj");
    normalCube_->SetTranslate({ -8.0f, 0.0f, 10.0f });
    normalCube_->SetScale({ 2.0f, 2.0f, 2.0f });
    normalCube_->SetEnableLighting(0);
    normalCube_->SetMaterialColor({ 0.2f, 0.9f, 1.0f, 1.0f });
    normalCube_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

    postCube_ = std::make_unique<Object3d>();
    postCube_->Initialize(app.ObjCom(), app.Dx());
    postCube_->SetCamera(camera_.get());
    postCube_->SetModel("cube/cube.obj");
    postCube_->SetTranslate({ 0.0f, 0.0f, 10.0f });
    postCube_->SetScale({ 2.0f, 2.0f, 2.0f });
    postCube_->SetEnableLighting(0);
    postCube_->SetMaterialColor({ 1.0f, 0.25f, 0.75f, 1.0f });
    postCube_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

    postTeapot_ = std::make_unique<Object3d>();
    postTeapot_->Initialize(app.ObjCom(), app.Dx());
    postTeapot_->SetCamera(camera_.get());
    postTeapot_->SetModel("teapot.obj");
    postTeapot_->SetTranslate({ 8.0f, 0.0f, 10.0f });
    postTeapot_->SetScale({ 0.8f, 0.8f, 0.8f });
    postTeapot_->SetEnableLighting(0);
    postTeapot_->SetMaterialColor({ 1.0f, 0.85f, 0.2f, 1.0f });
    postTeapot_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

    auto& postParam = app.ObjectPost()->GetParam();
    postParam.threshold = 0.0f;
    postParam.intensity = 1.35f;
    postParam.chromAbAmount = 0.02f;
    postParam.distortionAmount = 0.004f;
    postParam.noiseIntensity = 0.02f;
    postParam.glitchAmount = 0.0f;
    postParam.dissolveAmount = -1.0f;

    pausingUI_ = std::make_unique<PausingUI>();
    pausingUI_->Initialize(app);
}

void TestScene::OnExit(GameApp& /*app*/) {
    spotMarker_.reset();
    pointMarker_.reset();
    skyDome_.reset();
    postTeapot_.reset();
    postCube_.reset();
    normalCube_.reset();
    ground_.reset();
    playTxst_.reset();

    player_.reset();
    camera_.reset();

    // EnemyManager に Clear() があるなら呼ぶ
    // enemyMgr_.Clear();
}
void TestScene::Update(GameApp& app, float dt) {
    if (!input_) return;

    if (pausingUI_) {
        pausingUI_->Update(app, input_);
        if (pausingUI_->GetIsSceneChangeRequested()) {
            RequestChangeScene_("Title");
            return;
        }
        if (pausingUI_->GetIsPaused()) {
            return;
        }
    }

    camera_->Update();

    ground_->Update(dt);

    skyDome_->Update(dt);
    demoTimer_ += dt;

    if (normalCube_) {
        normalCube_->SetRotate({ 0.0f, demoTimer_ * 0.8f, 0.0f });
        normalCube_->Update(dt);
    }


    if (postCube_) {
        postCube_->SetRotate({ demoTimer_ * 0.5f, demoTimer_ * 1.2f, 0.0f });
        postCube_->Update(dt);
    }
    if (postTeapot_) {
        postTeapot_->SetRotate({ 0.0f, -demoTimer_ * 0.7f, 0.0f });
        postTeapot_->Update(dt);
    }

    auto& postParam = app.ObjectPost()->GetParam();
    //postParam.intensity = 1.35f + std::sinf(demoTimer_ * 2.0f) * 0.25f;
    //postParam.chromAbAmount = 0.018f;
    //postParam.distortionAmount = 0.003f + std::sinf(demoTimer_ * 1.5f) * 0.0015f;
    //postParam.noiseIntensity = 0.02f;

    if (player_) {
        player_->Update(dt);
    }
    
    enemyMgr_.Update(dt);

    actionDirector_.Update(dt, input_);

    if (actionDirector_.IsPlaying() && actionDirector_.GetProfile().enableCameraWork) {
        // 再生中はシネマティックカメラの座標・回転をメインカメラにコピーする
        Camera* cinematicCam = actionDirector_.GetCinematicCamera();
        camera_->SetTranslate(cinematicCam->GetTranslate());
        camera_->SetRotate(cinematicCam->GetRotate());
        camera_->SetFovY(cinematicCam->GetFovY());
    }

    // ===============================
    // ★ クランプ到達チェック
    // ===============================
    const float zNear = -10.0f;
    const float zFar = 20.0f;
    const float xMaxNear = 15.0f;
    const float xMaxFar = 20.0f;

    //float z = player_->GetZ();
    //float t = (z - zNear) / (zFar - zNear);
   /* t = std::clamp(t, 0.0f, 1.0f);
    float xMax = xMaxNear + (xMaxFar - xMaxNear) * t;*/

    //float x = player_->GetX();

    // ★ 右端に到達したら GameScene へ

   /* if (!reachedEdge_ && x >= xMax - 0.01f) {
        reachedEdge_ = true;
        RequestChangeScene_("Game");
    }*/

#ifdef USE_IMGUI

    // ===== ImGui =====
    ImGui::Begin("Camera Debug");

    ImGui::End();

#endif // DEBUG

    //player_->SetLighting(light_);
    enemyMgr_.SetLighting(light_);
    //// ground は SpotLightのみ反映
    //if (ground_) {
    //    ground_->SetEnableLighting(2);

    //    // ★Directional OFF
    //    ground_->SetIntensity(0.0f);

    //    // ★Point OFF
    //    ground_->SetPointLightIntensity(0.0f);

    //    // ★Spot ON
    //    const float cosOuter = std::cosf(groundLight_.spotAngleDeg * (std::numbers::pi_v<float> / 180.0f));
    //    const float cosInner = std::cosf(groundLight_.spotFalloffStartDeg * (std::numbers::pi_v<float> / 180.0f));
    //    ground_->SetShininess(128.0f);
    //    ground_->SetSpotLightPos(groundLight_.spotPos);
    //    ground_->SetSpotLightDirection(groundLight_.spotDir); // 正規化されてる前提
    //    ground_->SetSpotLightIntensity(groundLight_.spotIntensity);
    //    ground_->SetSpotLightDistance(groundLight_.spotDistance);
    //    ground_->SetSpotLightDecay(groundLight_.spotDecay);
    //    ground_->SetSpotLightCosAngle(cosOuter);
    //    ground_->SetSpotLightCosFalloffStart(cosInner);
    //    ground_->SetSpotLightColor({ groundLight_.spotColor.x, groundLight_.spotColor.y, groundLight_.spotColor.z, 1.0f });
    //}

    //// Spot マーカー追従
    //if (spotMarker_) {
    //    spotMarker_->SetTranslate(groundLight_.spotPos);
    //    spotMarker_->SetScale({ spotMarkerScale_, spotMarkerScale_, spotMarkerScale_ });
    //    spotMarker_->Update(dt);
    //}



#ifdef USE_IMGUI
    ImGui::Begin("Ground PointLight");

    ImGui::Checkbox("Point Only (ground)", &groundPointOnly_);

    ImGui::DragFloat3("Point Pos", &groundLight_.pointPos.x, 0.1f);
    ImGui::DragFloat("Point Intensity", &groundLight_.pointIntensity, 0.05f, 0.0f, 50.0f);
    ImGui::DragFloat("Point Radius", &groundLight_.pointRadius, 0.1f, 0.1f, 200.0f);
    ImGui::DragFloat("Point Decay", &groundLight_.pointDecay, 0.01f, 0.0f, 10.0f);

    ImGui::ColorEdit3("Point Color", &groundLight_.pointColor.x);

    if (ImGui::Button("Reset")) {
        groundLight_.dirIntensity = 0.0f;
        groundLight_.spotIntensity = 0.0f;
        groundLight_.pointIntensity = 2.5f;
        groundLight_.pointPos = { 0.0f, 5.0f, 15.0f };
        groundLight_.pointRadius = 15.0f;
        groundLight_.pointDecay = 1.0f;
        groundLight_.pointColor = { 1.0f, 1.0f, 1.0f };
    }

    ImGui::Checkbox("Draw Point Marker", &drawPointMarker_);
    ImGui::DragFloat("Marker Scale", &pointMarkerScale_, 0.01f, 0.01f, 5.0f);


    ImGui::End();
#endif

#ifdef USE_IMGUI
    ImGui::Begin("Ground SpotLight");

    ImGui::Checkbox("Spot Only (ground)", &groundSpotOnly_);

    ImGui::DragFloat3("Spot Pos", &groundLight_.spotPos.x, 0.1f);
    ImGui::DragFloat3("Spot Dir", &groundLight_.spotDir.x, 0.01f, -1.0f, 1.0f);

    // ★Dirは正規化しないと壊れやすいので、ボタンで正規化も入れる
    if (ImGui::Button("Normalize Dir")) {
        Vector3 d = groundLight_.spotDir;
        float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        if (len > 1e-6f) { groundLight_.spotDir = { d.x / len, d.y / len, d.z / len }; }
    }

    ImGui::DragFloat("Spot Intensity", &groundLight_.spotIntensity, 0.05f, 0.0f, 200.0f);
    ImGui::DragFloat("Spot Distance", &groundLight_.spotDistance, 0.1f, 0.1f, 500.0f);
    ImGui::DragFloat("Spot Decay", &groundLight_.spotDecay, 0.01f, 0.0f, 10.0f);

    ImGui::DragFloat("Spot Angle (deg)", &groundLight_.spotAngleDeg, 0.1f, 1.0f, 89.0f);
    ImGui::DragFloat("Falloff Start (deg)", &groundLight_.spotFalloffStartDeg, 0.1f, 0.0f, 89.0f);

    if (groundLight_.spotFalloffStartDeg > groundLight_.spotAngleDeg - 0.1f) {
        groundLight_.spotFalloffStartDeg = groundLight_.spotAngleDeg - 0.1f;
    }

    ImGui::ColorEdit3("Spot Color", &groundLight_.spotColor.x);

    if (ImGui::Button("Reset Spot")) {
        groundLight_.dirIntensity = 0.0f;
        groundLight_.pointIntensity = 0.0f;

        groundLight_.spotIntensity = 20.0f;
        groundLight_.spotPos = { 0.0f, 15.0f, 15.0f };
        groundLight_.spotDir = { 0.0f, -1.0f, 0.0f };
        groundLight_.spotDistance = 80.0f;
        groundLight_.spotDecay = 1.0f;
        groundLight_.spotAngleDeg = 25.0f;
        groundLight_.spotFalloffStartDeg = 15.0f;
        groundLight_.spotColor = { 1.0f,1.0f,1.0f };
    }

    ImGui::Checkbox("Draw Spot Marker", &drawSpotMarker_);
    ImGui::DragFloat("Marker Scale", &spotMarkerScale_, 0.01f, 0.01f, 5.0f);

    ImGui::End();
#endif

#ifdef USE_IMGUI
    ImGui::Begin("Object Post Demo");
    ImGui::Checkbox("Enable Object Post", &enableObjectPostDemo_);
    ImGui::DragFloat("Bloom Intensity", &postParam.intensity, 0.01f, 0.0f);
    ImGui::DragFloat("Chromatic Aberration", &postParam.chromAbAmount, 0.001f, 0.0f);
    ImGui::DragFloat("Wave Distortion", &postParam.distortionAmount, 0.001f, 0.0f);
    ImGui::DragFloat("Noise", &postParam.noiseIntensity, 0.001f, 0.0f);
    ImGui::End();
#endif


}

void TestScene::Draw3D(GameApp& app) {
    auto* cmd = app.Dx()->GetCommandList();
    
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    app.ObjCom()->SetGraphicsPipelineState();

//    if (ground_) ground_->Draw();
    if (skyDome_) skyDome_->Draw();
    if (normalCube_) normalCube_->Draw();
    if (drawPointMarker_ && pointMarker_) pointMarker_->Draw();
    if (drawSpotMarker_ && spotMarker_) spotMarker_->Draw();
    if (player_) player_->Draw();

    enemyMgr_.Draw();

    if (enableObjectPostDemo_) {
        app.BeginObjectPostEffect();
        if (postCube_) postCube_->Draw();
        if (postTeapot_) postTeapot_->Draw();
        app.EndObjectPostEffect();
    } else {
        if (postCube_) postCube_->Draw();
        if (postTeapot_) postTeapot_->Draw();
    }
}

void TestScene::Draw2D(GameApp& app) {
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

    if (pausingUI_) {
        pausingUI_->Draw(app);
    }
}

void TestScene::DrawImGui(GameApp& app) {
#ifdef USE_IMGUI
    Enemy* targetEnemy = nullptr;
    if (!enemyMgr_.GetEnemies().empty()) {
        targetEnemy = &enemyMgr_.GetEnemies().front();
    }
    actionDirector_.DrawImGuiEditor(camera_.get(), player_.get(), targetEnemy);

    if (pausingUI_) {
        pausingUI_->DrawImGui();
    }
#endif
}
