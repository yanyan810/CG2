#include "TitleScene.h"
#include <Windows.h>

#include "GameApp.h"
#include "Particle.h"
#include "ParticleCommon.h"
#include "Camera.h"

void TitleScene::OnEnter(GameApp& app) {

    state_ = State::Idle;
    circle_ = 1.0f;     // 最初は表示されている
    softness_ = 0.6f;
    prevSpace_ = false;

    TextureManager::GetInstance()->LoadTexture("resources/ui/char/title.png");


    // カメラ（Particle は内部カメラでも動くけど、外部カメラを渡すと安定）
    camera_ = std::make_unique<Camera>();
    camera_->SetRotate({ 0, 0, 0 });
    camera_->SetTranslate({ 0, 3, -20 });

    app.ObjCom()->SetDefaultCamera(camera_.get());


    // Particle
    particle_ = std::make_unique<Particle>();
    particle_->Initialize(app.ParticleCom(), app.Dx(), app.Srv());

    // ★これが無いと「model_ が null」で Draw しても何も出ません
   // particle_->SetModel("plane.obj"); // 既にある板ポリ等に合わせて

    // カメラを渡せるなら渡す（Particle.cpp は camera_ があればそれを使う仕様）
    particle_->SetCamera(camera_.get());
    Vector3 rotate = { 0.0f,0.0f,0.0f };

    particle_->SetRotate(rotate);
    // 見えやすいように（任意）
    particle_->SetBlendMode(ParticleCommon::BlendMode::kBlendModeAdd);
    particle_->SetMaterialColor({ 1, 1, 1, 1 });


    skyDome_ = std::make_unique<Object3d>();
    skyDome_->Initialize(app.ObjCom(), app.Dx());
    skyDome_->SetModel("skydome/SkyDome.obj");

    bg_ = std::make_unique<Sprite>();
    bg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/char/Title.png");
    bg_->SetAnchorPoint({ 0,0 });
    bg_->SetPosition({ 0,0 });
    bg_->SetScale({ 1,1,1 }); // 1280x720ならそのまま


    pressStart_ = std::make_unique<Sprite>();
    pressStart_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/char/pressSpace.png"); // 例
    pressStart_->SetAnchorPoint({ 0, 0 });
 //   pressStart_->SetPosition({ WinApp::kClientWidth * 0.5f, WinApp::kClientHeight * 0.80f });
    pressStart_->SetScale({ 1,1,1 }); // 128x128なら等倍でOK

    testObj_ = std::make_unique<Object3d>();
    testObj_->Initialize(app.ObjCom(), app.Dx());

    // 手元にあるモデル名にしてOK（例：sphere.obj / cube.obj / Suzanne.obj 等）
    testObj_->SetModel("cube/cube.obj");  // ★あなたのresourcesにあるやつに合わせて変えてOK

    testObj_->SetTranslate({ 0.0f, 1.0f, 0.0f });
    testObj_->SetScale({ 2.0f, 2.0f, 2.0f });

    // 鏡面反射が分かりやすいように：白っぽく、ライティングON、shininess高め
    testObj_->SetMaterialColor({ 1,1,1,1 });
    testObj_->SetEnableLighting(lightingMode_);
    testObj_->SetShininess(shininess_);



}

void TitleScene::OnExit(GameApp&) {
    skyDome_.reset();
    particle_.reset();
    camera_.reset();
}

void TitleScene::Update(GameApp& app, float dt) {

    // ESC で終了（Input クラス持ってるなら差し替え）
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        app.RequestQuit();
        return;
    }

    bool spaceNow = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    bool spaceTrig = spaceNow && !prevSpace_;
    prevSpace_ = spaceNow;

    //switch (state_) {
    //case State::Idle:
    //    if (spaceTrig) {
    //        state_ = State::ExitClose; // ★すぐ遷移しない
    //    }
    //    break;

    //case State::ExitClose:
    //    circle_ -= 1.8f * dt;          // ★閉じる
    //    if (circle_ <= 0.0f) {
    //        circle_ = 0.0f;
    //        RequestChangeScene_(kNextScene_);
    //    }
    //    break;
    //}


    skyDome_->Update();

#ifdef USE_IMGUI

    // ===== ImGui =====
    ImGui::Begin("Camera Debug");

    ImGui::DragFloat3("Position", &imguiCamPos_.x, 0.1f);
    ImGui::DragFloat3("Rotation", &imguiCamRot_.x, 0.01f);

    ImGui::End();
    
#endif // DEBUG

#ifdef USE_IMGUI
    ImGui::Begin("Phong Check");

    ImGui::Checkbox("Orbit Camera", &orbitCam_);
    ImGui::DragFloat("Orbit Speed", &orbitSpeed_, 0.01f, 0.0f, 5.0f);
    ImGui::DragFloat("Orbit Radius", &orbitRadius_, 0.1f, 1.0f, 50.0f);

    ImGui::SliderFloat("Shininess", &shininess_, 1.0f, 256.0f);

    // 1:Lambert 2:Half 3:SpecOnly
    ImGui::RadioButton("Lambert", &lightingMode_, 1); ImGui::SameLine();
    ImGui::RadioButton("HalfLambert", &lightingMode_, 2); ImGui::SameLine();
    ImGui::RadioButton("SpecOnly", &lightingMode_, 3);

    ImGui::End();
#endif

    if (orbitCam_ && camera_) {
        orbitT_ += orbitSpeed_ * dt;

        Vector3 target{ 0.0f, 1.0f, 0.0f }; // testObjを見る想定
        Vector3 eye{
            target.x + std::cosf(orbitT_) * orbitRadius_,
            target.y + 3.0f,
            target.z + std::sinf(orbitT_) * orbitRadius_
        };

        camera_->SetTranslate(eye);

        // 回転は「LookAt」関数が無いなら、いったん手動でYawだけでもOK
        // ここはあなたのCamera仕様次第なので、とりあえずImGui回転を残す運用でもOK
    }

    if (testObj_) {
        testObj_->SetEnableLighting(lightingMode_);
        testObj_->SetShininess(shininess_);
        testObj_->Update();
    }

    // ===== カメラ反映 =====
    if (camera_) {
        camera_->SetTranslate(imguiCamPos_);
        camera_->SetRotate(imguiCamRot_);
        camera_->Update();
    }

    // ★重要：Spawn → Update（Spawnしないと instanceCount_ が0のまま）
    if (particle_) {
        particle_->SpawnParticle(); // emitter.frequency=0.5なので0.5秒ごとに出る
       particle_->Update();
    }
}

void TitleScene::Draw(GameApp& app) {

    app.ObjCom()->SetGraphicsPipelineState();

	//skyDome_->Draw();

    if (testObj_) testObj_->Draw();

    // ---- 2D ----
    app.SpriteCom()->SetGraphicsPipelineState();

    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0, 100);

 /*   if (bg_) {
        bg_->Update(view, proj);
        bg_->Draw();
    }

    if (pressStart_) {
        pressStart_->Update(view, proj);
        pressStart_->Draw();
    }*/

    // ===== マスクは必ず最後 =====
    app.SpriteCom()->DrawCircleMask(circle_, softness_);

    if (!particle_) return;

    // ★PSO/RS をここでセット（Particle::Draw は rootにSRV/CBV積むだけ）
    app.ParticleCom()->SetGraphicsPipelineState();

    // ★PreDraw/SrvPreDraw/PostDraw は GameApp がやるのでここでは絶対呼ばない
    //particle_->Draw();
}
