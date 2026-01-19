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
    testObj_->SetModel("enemy/shooter/bullet/bullet.obj");  // ★あなたのresourcesにあるやつに合わせて変えてOK

    testObj_->SetTranslate(testPos_);
    testObj_->SetScale(testScale_);
    // 回転も使うなら
    testObj_->SetRotate(testRot_);

    // 鏡面反射が分かりやすいように：白っぽく、ライティングON、shininess高め
    testObj_->SetMaterialColor({ 1,1,1,1 });
    testObj_->SetEnableLighting(lightingMode_);
    testObj_->SetShininess(shininess_);
	testObj_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);


    terrainObj_ = std::make_unique<Object3d>();
    terrainObj_->Initialize(app.ObjCom(), app.Dx());

    terrainObj_->SetModel("terrain/terrain.obj");  // ★あなたのresourcesにあるやつに合わせて変えてOK

    terrainObj_->SetTranslate(testPos_);
    terrainObj_->SetScale(testScale_);
    // 回転も使うなら
    terrainObj_->SetRotate(testRot_);

    // 鏡面反射が分かりやすいように：白っぽく、ライティングON、shininess高め
    terrainObj_->SetMaterialColor({ 1,1,1,1 });
    terrainObj_->SetEnableLighting(lightingMode_);
    terrainObj_->SetShininess(shininess_);
    terrainObj_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

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
    ImGui::RadioButton("SpecOnly(Phong)", &lightingMode_, 3); ImGui::SameLine();
    ImGui::RadioButton("SpecOnly(Blinn)", &lightingMode_, 4);

    ImGui::Separator();
    ImGui::Text("Light");

    ImGui::DragFloat3("Dir", &lightDir_.x, 0.01f, -1.0f, 1.0f);
    ImGui::SliderFloat("Intensity", &lightIntensity_, 0.0f, 5.0f);
    ImGui::ColorEdit3("Color", &lightColor_.x); // RGBだけ


    ImGui::End();

	ImGui::Begin("Test Object SRT");

    // 位置
    ImGui::DragFloat3("T", &testPos_.x, 0.1f);

    // 回転：ラジアン想定なら 0.01f 刻みが扱いやすい
    ImGui::DragFloat3("R", &testRot_.x, 0.01f);

    // スケール
    ImGui::DragFloat3("S", &testScale_.x, 0.1f, 0.001f, 100.0f);

    ImGui::Separator();
    ImGui::Text("PointLight");

    ImGui::DragFloat3("Point Pos", &pointPos_.x, 0.1f);
    ImGui::SliderFloat("Point Intensity", &pointIntensity_, 0.0f, 10.0f);
    ImGui::DragFloat(
        "Point Radius",
        &pointRadius_,
        0.1f,        // 1ドラッグの変化量
        0,        // 最小
        100.0f,      // 最大（まずは100で十分）
        "%.2f"
    );
    ImGui::DragFloat(
        "Point Decay",
        &pointDecay_,
        0.05f,
        0.0f,
        8.0f,
        "%.2f"
    );

    ImGui::ColorEdit3("Point Color", &pointColor_.x);

    ImGui::Separator();
    ImGui::RadioButton("View: All", &lightViewMode_, 0); ImGui::SameLine();
    ImGui::RadioButton("View: DirOnly", &lightViewMode_, 1); ImGui::SameLine();
    ImGui::RadioButton("View: PointOnly", &lightViewMode_, 2);

    ImGui::Separator();
    ImGui::Text("SpotLight");

    ImGui::DragFloat3("Spot Pos", &spotPos_.x, 0.1f);
    ImGui::DragFloat3("Spot Dir", &spotDir_.x, 0.01f, -1.0f, 1.0f);

    ImGui::SliderFloat("Spot Intensity", &spotIntensity_, 0.0f, 20.0f);
    ImGui::DragFloat("Spot Distance", &spotDistance_, 0.1f, 0.0f, 200.0f, "%.2f");
    ImGui::DragFloat("Spot Decay", &spotDecay_, 0.05f, 0.0f, 8.0f, "%.2f");

    ImGui::ColorEdit3("Spot Color", &spotColor_.x);

    ImGui::SliderFloat("Spot Angle (deg)", &spotAngleDeg_, 1.0f, 89.0f);
    ImGui::SliderFloat("Falloff Start (deg)", &spotFalloffStartDeg_, 0.5f, 89.0f);


    ImGui::End();

#endif
    spotCos = std::cosf(spotAngleDeg_ * (std::numbers::pi_v<float> / 180.0f));

    // ---- Spot angle deg -> cos ----
//    const float kPi = 3.14159265f;

    // 外側は spotAngleDeg_
// 内側は spotFalloffStartDeg_（必ず外側より小さく）
    spotFalloffStartDeg_ = std::min(spotFalloffStartDeg_, spotAngleDeg_ - 0.1f);

    float cosOuter = std::cosf(spotAngleDeg_ * (std::numbers::pi_v<float> / 180.0f));
    float cosInner = std::cosf(spotFalloffStartDeg_ * (std::numbers::pi_v<float> / 180.0f));

    // cosは「角度が小さいほど大きい」ので、内側のcosは外側より大きいのが正しい
    // cosInner > cosOuter になってる状態が正常



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

    int lighting = lightingMode_;
    if (lightViewMode_ == 1) lighting = 11; // DirOnly
    if (lightViewMode_ == 2) lighting = 12; // PointOnly


    if (testObj_) {
        // ★SRT反映
        testObj_->SetTranslate(testPos_);
        testObj_->SetRotate(testRot_);
        testObj_->SetScale(testScale_);

        // 既存
        testObj_->SetEnableLighting(lighting);
        testObj_->SetShininess(shininess_);
        testObj_->SetDirection(lightDir_);
        testObj_->SetIntensity(lightIntensity_);
        testObj_->SetLightColor(lightColor_);

        // PointLight 反映
        testObj_->SetPointLightPos(pointPos_);
        testObj_->SetPointLightIntensity(pointIntensity_);
        testObj_->SetPointLightColor(pointColor_);

		testObj_->SetPointLightRadius(pointRadius_);
		testObj_->SetPointLightDecay(pointDecay_);

        // SpotLight 反映
        testObj_->SetSpotLightPos(spotPos_);
        testObj_->SetSpotLightDirection(spotDir_);
        testObj_->SetSpotLightIntensity(spotIntensity_);
        testObj_->SetSpotLightDistance(spotDistance_);
        testObj_->SetSpotLightDecay(spotDecay_);
        testObj_->SetSpotLightCosAngle(cosOuter);
        testObj_->SetSpotLightCosFalloffStart(cosInner);
        testObj_->SetSpotLightColor({ spotColor_.x, spotColor_.y, spotColor_.z, 1.0f });


        testObj_->Update();
    }

    if (terrainObj_) {

        // ★SRT反映
        terrainObj_->SetTranslate(testPos_);
        terrainObj_->SetRotate(testRot_);
        terrainObj_->SetScale(testScale_);

        // 既存
        terrainObj_->SetEnableLighting(lighting);
        terrainObj_->SetShininess(shininess_);
        terrainObj_->SetDirection(lightDir_);
        terrainObj_->SetIntensity(lightIntensity_);
        terrainObj_->SetLightColor(lightColor_);

        terrainObj_->SetPointLightPos(pointPos_);
        terrainObj_->SetPointLightIntensity(pointIntensity_);
        terrainObj_->SetPointLightColor(pointColor_);

        terrainObj_->SetPointLightRadius(pointRadius_);
        terrainObj_->SetPointLightDecay(pointDecay_);

        // SpotLight 反映
        terrainObj_->SetSpotLightPos(spotPos_);
        terrainObj_->SetSpotLightDirection(spotDir_);
        terrainObj_->SetSpotLightIntensity(spotIntensity_);
        terrainObj_->SetSpotLightDistance(spotDistance_);
        terrainObj_->SetSpotLightDecay(spotDecay_);
        terrainObj_->SetSpotLightCosAngle(cosOuter);
        terrainObj_->SetSpotLightCosFalloffStart(cosInner);
        terrainObj_->SetSpotLightColor({ spotColor_.x, spotColor_.y, spotColor_.z, 1.0f });


        terrainObj_->Update();


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
	if (terrainObj_) terrainObj_->Draw();

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
