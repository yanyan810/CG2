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
    particle_->SetModel(assimpPlanePaths_[assimpPlaneIndex_]);
    assimpPlaneIndexPrev_ = assimpPlaneIndex_;

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

    testObj_->SetTranslate({0.0f,5.0f,0.0f});
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

    // 手元にあるモデル名にしてOK（例：sphere.obj / cube.obj / Suzanne.obj 等）
    terrainObj_->SetModel("terrain/terrain.obj");  // ★あなたのresourcesにあるやつに合わせて変えてOK

    terrainObj_->SetTranslate({ 0.0f,5.0f,0.0f });
    terrainObj_->SetScale(testScale_);
    // 回転も使うなら
    terrainObj_->SetRotate(testRot_);

    // 鏡面反射が分かりやすいように：白っぽく、ライティングON、shininess高め
    terrainObj_->SetMaterialColor({ 1,1,1,1 });
    terrainObj_->SetEnableLighting(lightingMode_);
    terrainObj_->SetShininess(shininess_);
    terrainObj_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);


    // TitleScene.cpp（OnEnter内の nodeObj_ 部分だけ置き換え）

    nodeObj_ = std::make_unique<Object3d>();
    nodeObj_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());

    // ★初期モデル（今は 05）
    nodeObj_->SetModel(nodeModelPaths_[nodeModelIndex_]);
    nodeObj_->PlayAnimation("", true);

    nodeObj_->SetDebugDrawBones(false);
    nodeObj_->SetBoneMarkerModel("cube/cube.obj");

    nodeObj_->SetTranslate({ 0, 0.0f, 0 });
    nodeObj_->SetScale(testScale_);
    nodeObj_->SetRotate(testRot_);

    nodeObj_->SetMaterialColor({ 1,1,1,1 });
    nodeObj_->SetShininess(shininess_);
    nodeObj_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

    if (nodeObj_->HasAnimation()) {
        OutputDebugStringA("has animation\n");
    }

    nodeMiscObj_ = std::make_unique<Object3d>();
    nodeMiscObj_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());

    // 初期モデル
    nodeMiscObj_->SetModel(nodeMiscModelPaths_[nodeMiscModelIndex_]);
    nodeMiscObj_->PlayAnimation("", true);

    nodeMiscObj_->SetDebugDrawBones(false);
    nodeMiscObj_->SetBoneMarkerModel("cube/cube.obj");

    // 位置が被ると見づらいので少しずらす（任意）
    nodeMiscObj_->SetTranslate({ 3.0f, 0.0f, 0.0f });
    nodeMiscObj_->SetScale(testScale_);
    nodeMiscObj_->SetRotate(testRot_);

    nodeMiscObj_->SetMaterialColor({ 1,1,1,1 });
    nodeMiscObj_->SetShininess(shininess_);
    nodeMiscObj_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

    if (nodeMiscObj_->HasAnimation()) {
        OutputDebugStringA("nodeMisc: has animation\n");
    }


    skinObj_ = std::make_unique<Object3d>();
    skinObj_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());

    // 初期モデル
    skinObj_->SetModel(skinModelPaths_[skinModelIndex_]);
    skinObj_->PlayAnimation("", true);

    skinObj_->SetDebugDrawBones(false);
    skinObj_->SetBoneMarkerModel("cube/cube.obj");

    // 位置は被らないようにずらす（任意）
    skinObj_->SetTranslate({ -3.0f, 0.0f, 0.0f });
    skinObj_->SetScale(testScale_);
    skinObj_->SetRotate(testRot_);

    skinObj_->SetMaterialColor({ 1,1,1,1 });
    skinObj_->SetShininess(shininess_);
    skinObj_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

    if (skinObj_->HasAnimation()) {
        OutputDebugStringA("skin: has animation\n");
    }

    // ----------------------------
// Mesh_Primitives
// ----------------------------
    meshPrimObj_ = std::make_unique<Object3d>();
    meshPrimObj_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());
    meshPrimObj_->SetModel(meshPrimPaths_[meshPrimIndex_]);
    meshPrimObj_->PlayAnimation("", true);
    meshPrimObj_->SetTranslate({ -6.0f, 0.0f, 0.0f });
    meshPrimObj_->SetScale(testScale_);
    meshPrimObj_->SetRotate(testRot_);
    meshPrimObj_->SetMaterialColor({ 1,1,1,1 });
    meshPrimObj_->SetShininess(shininess_);
    meshPrimObj_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

    // ----------------------------
    // Material_AlphaBlend
    // ----------------------------
    alphaBlendObj_ = std::make_unique<Object3d>();
    alphaBlendObj_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());
    alphaBlendObj_->SetModel(alphaBlendPaths_[alphaBlendIndex_]);
    alphaBlendObj_->PlayAnimation("", true);
    alphaBlendObj_->SetTranslate({ 6.0f, 0.0f, 0.0f });
    alphaBlendObj_->SetScale(testScale_);
    alphaBlendObj_->SetRotate(testRot_);
    alphaBlendObj_->SetMaterialColor({ 1,1,1,1 });
    alphaBlendObj_->SetShininess(shininess_);
    // ★透明テスト用：通常αブレンド（あなたのenum名に合わせて変更）
    alphaBlendObj_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNormal);

    // ----------------------------
    // Texture_Sampler
    // ----------------------------
    texSamplerObj_ = std::make_unique<Object3d>();
    texSamplerObj_->Initialize(app.ObjCom(), app.Dx(), app.Srv(), app.SkinCom());
    texSamplerObj_->SetModel(texSamplerPaths_[texSamplerIndex_]);
    texSamplerObj_->PlayAnimation("", true);
    texSamplerObj_->SetTranslate({ 0.0f, 0.0f, 6.0f });
    texSamplerObj_->SetScale(testScale_);
    texSamplerObj_->SetRotate(testRot_);
    texSamplerObj_->SetMaterialColor({ 1,1,1,1 });
    texSamplerObj_->SetShininess(shininess_);
    texSamplerObj_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

 
    srtTest_.pos = { 0.0f, 5.0f, 0.0f };
    srtTerrain_.pos = { 0.0f, 0.0f, 0.0f };

    srtNode_.pos = { 0.0f, 0.0f, 0.0f };
    srtNodeMisc_.pos = { 3.0f, 0.0f, 0.0f };
    srtSkin_.pos = { -3.0f, 0.0f, 0.0f };

    srtMeshPrim_.pos = { -6.0f, 0.0f, 0.0f };
    srtAlphaBlend_.pos = { 6.0f, 0.0f, 0.0f };
    srtTexSampler_.pos = { 0.0f, 0.0f, 6.0f };

    // scale/rot も必要なら個別に


}

void TitleScene::OnExit(GameApp&) {
    skyDome_.reset();
    particle_.reset();
    camera_.reset();
	nodeObj_.reset();
    nodeMiscObj_.reset();
    skinObj_.reset();
    meshPrimObj_.reset();
    alphaBlendObj_.reset();
    texSamplerObj_.reset();

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


    skyDome_->Update(dt);

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

    ImGui::Begin("Object SRT (Per-Object)");

    static const char* targetLabels[] = {
        "TestObj",
        "Terrain",
        "Node",
        "NodeMisc",
        "Skin",
        "MeshPrim",
        "AlphaBlend",
        "TexSampler"
    };
    ImGui::Combo("Target", &editTarget_, targetLabels, IM_ARRAYSIZE(targetLabels));

    // ターゲットに応じて参照先を切り替え
    SRT* cur = nullptr;
    switch (static_cast<EditTarget>(editTarget_)) {
    case EditTarget::TestObj:     cur = &srtTest_; break;
    case EditTarget::Terrain:     cur = &srtTerrain_; break;
    case EditTarget::Node:        cur = &srtNode_; break;
    case EditTarget::NodeMisc:    cur = &srtNodeMisc_; break;
    case EditTarget::Skin:        cur = &srtSkin_; break;
    case EditTarget::MeshPrim:    cur = &srtMeshPrim_; break;
    case EditTarget::AlphaBlend:  cur = &srtAlphaBlend_; break;
    case EditTarget::TexSampler:  cur = &srtTexSampler_; break;
    default: break;
    }

    if (cur) {
        ImGui::DragFloat3("T", &cur->pos.x, 0.1f);
        ImGui::DragFloat3("R", &cur->rot.x, 0.01f);
        ImGui::DragFloat3("S", &cur->scale.x, 0.1f, 0.001f, 100.0f);
    }

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

    if (particle_) {
        particle_->DebugImGui(); // ★Particle側のウィンドウを出す
    }


    DrawImGui_ModelSwitchersOneWindow();



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
        testObj_->SetTranslate(srtTest_.pos);
        testObj_->SetRotate(srtTest_.rot);
        testObj_->SetScale(srtTest_.scale);

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


        testObj_->Update(dt);
    }


    if (terrainObj_) {
        // ★SRT反映
        terrainObj_->SetTranslate(srtTerrain_.pos);
        terrainObj_->SetRotate(srtTerrain_.rot);
        terrainObj_->SetScale(srtTerrain_.scale);

        // 既存
        terrainObj_->SetEnableLighting(lighting);
        terrainObj_->SetShininess(shininess_);
        terrainObj_->SetDirection(lightDir_);
        terrainObj_->SetIntensity(lightIntensity_);
        terrainObj_->SetLightColor(lightColor_);

        // PointLight 反映
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


        terrainObj_->Update(dt);
    }

    if (nodeObj_) {

        // ★モデル切替：indexが変わった瞬間に読み直す
        if (nodeObj_ && nodeModelIndex_ != nodeModelIndexPrev_) {
            nodeModelIndexPrev_ = nodeModelIndex_;

            nodeObj_->SetModel(nodeModelPaths_[nodeModelIndex_]);
            nodeObj_->PlayAnimation("", true);

            // もしモデル変更で内部状態がリセットされるなら、必要に応じて再設定
            nodeObj_->SetDebugDrawBones(false);
            nodeObj_->SetBoneMarkerModel("cube/cube.obj");
        }


        // ★SRT反映
        nodeObj_->SetTranslate(srtNode_.pos);
        nodeObj_->SetRotate(srtNode_.rot);
        nodeObj_->SetScale(srtNode_.scale);

        // 既存
        nodeObj_->SetEnableLighting(lighting);
        nodeObj_->SetShininess(shininess_);
        nodeObj_->SetDirection(lightDir_);
        nodeObj_->SetIntensity(lightIntensity_);
        nodeObj_->SetLightColor(lightColor_);

        nodeObj_->SetPointLightPos(pointPos_);
        nodeObj_->SetPointLightIntensity(pointIntensity_);
        nodeObj_->SetPointLightColor(pointColor_);

        nodeObj_->SetPointLightRadius(pointRadius_);
        nodeObj_->SetPointLightDecay(pointDecay_);

        // SpotLight 反映
        nodeObj_->SetSpotLightPos(spotPos_);
        nodeObj_->SetSpotLightDirection(spotDir_);
        nodeObj_->SetSpotLightIntensity(spotIntensity_);
        nodeObj_->SetSpotLightDistance(spotDistance_);
        nodeObj_->SetSpotLightDecay(spotDecay_);
        nodeObj_->SetSpotLightCosAngle(cosOuter);
        nodeObj_->SetSpotLightCosFalloffStart(cosInner);
        nodeObj_->SetSpotLightColor({ spotColor_.x, spotColor_.y, spotColor_.z, 1.0f });


        nodeObj_->Update(dt);


    }

    if (nodeMiscObj_) {

        // ★モデル切替
        if (nodeMiscModelIndex_ != nodeMiscModelIndexPrev_) {
            nodeMiscModelIndexPrev_ = nodeMiscModelIndex_;

            nodeMiscObj_->SetModel(nodeMiscModelPaths_[nodeMiscModelIndex_]);
            nodeMiscObj_->PlayAnimation("", true);

            nodeMiscObj_->SetDebugDrawBones(false);
            nodeMiscObj_->SetBoneMarkerModel("cube/cube.obj");
        }

        // ★SRT反映（位置だけ少しずらす例）
        nodeMiscObj_->SetTranslate(srtNodeMisc_.pos);
        nodeMiscObj_->SetRotate(srtNodeMisc_.rot);
        nodeMiscObj_->SetScale(srtNodeMisc_.scale);

        // ライト
        nodeMiscObj_->SetEnableLighting(lighting);
        nodeMiscObj_->SetShininess(shininess_);
        nodeMiscObj_->SetDirection(lightDir_);
        nodeMiscObj_->SetIntensity(lightIntensity_);
        nodeMiscObj_->SetLightColor(lightColor_);

        nodeMiscObj_->SetPointLightPos(pointPos_);
        nodeMiscObj_->SetPointLightIntensity(pointIntensity_);
        nodeMiscObj_->SetPointLightColor(pointColor_);
        nodeMiscObj_->SetPointLightRadius(pointRadius_);
        nodeMiscObj_->SetPointLightDecay(pointDecay_);

        nodeMiscObj_->SetSpotLightPos(spotPos_);
        nodeMiscObj_->SetSpotLightDirection(spotDir_);
        nodeMiscObj_->SetSpotLightIntensity(spotIntensity_);
        nodeMiscObj_->SetSpotLightDistance(spotDistance_);
        nodeMiscObj_->SetSpotLightDecay(spotDecay_);
        nodeMiscObj_->SetSpotLightCosAngle(cosOuter);
        nodeMiscObj_->SetSpotLightCosFalloffStart(cosInner);
        nodeMiscObj_->SetSpotLightColor({ spotColor_.x, spotColor_.y, spotColor_.z, 1.0f });

        nodeMiscObj_->Update(dt);
    }

    if (skinObj_) {

        // ★モデル切替
        if (skinModelIndex_ != skinModelIndexPrev_) {
            skinModelIndexPrev_ = skinModelIndex_;

            skinObj_->SetModel(skinModelPaths_[skinModelIndex_]);
            skinObj_->PlayAnimation("", true);

            skinObj_->SetDebugDrawBones(false);
            skinObj_->SetBoneMarkerModel("cube/cube.obj");
        }

        // ★SRT（位置だけさらにずらす例）
        skinObj_->SetTranslate(srtSkin_.pos);
        skinObj_->SetRotate(srtSkin_.rot);
        skinObj_->SetScale(srtSkin_.scale);

        // ライト（nodeMisc と同じ）
        skinObj_->SetEnableLighting(lighting);
        skinObj_->SetShininess(shininess_);
        skinObj_->SetDirection(lightDir_);
        skinObj_->SetIntensity(lightIntensity_);
        skinObj_->SetLightColor(lightColor_);

        skinObj_->SetPointLightPos(pointPos_);
        skinObj_->SetPointLightIntensity(pointIntensity_);
        skinObj_->SetPointLightColor(pointColor_);
        skinObj_->SetPointLightRadius(pointRadius_);
        skinObj_->SetPointLightDecay(pointDecay_);

        skinObj_->SetSpotLightPos(spotPos_);
        skinObj_->SetSpotLightDirection(spotDir_);
        skinObj_->SetSpotLightIntensity(spotIntensity_);
        skinObj_->SetSpotLightDistance(spotDistance_);
        skinObj_->SetSpotLightDecay(spotDecay_);
        skinObj_->SetSpotLightCosAngle(cosOuter);
        skinObj_->SetSpotLightCosFalloffStart(cosInner);
        skinObj_->SetSpotLightColor({ spotColor_.x, spotColor_.y, spotColor_.z, 1.0f });

        skinObj_->Update(dt);
    }

    if (meshPrimObj_) {

        if (meshPrimIndex_ != meshPrimIndexPrev_) {
            meshPrimIndexPrev_ = meshPrimIndex_;
            meshPrimObj_->SetModel(meshPrimPaths_[meshPrimIndex_]);
            meshPrimObj_->PlayAnimation("", true);
        }

        meshPrimObj_->SetTranslate({ testPos_.x - 6.0f, testPos_.y, testPos_.z });
        meshPrimObj_->SetRotate(testRot_);
        meshPrimObj_->SetScale(testScale_);

        meshPrimObj_->SetEnableLighting(lighting);
        meshPrimObj_->SetShininess(shininess_);
        meshPrimObj_->SetDirection(lightDir_);
        meshPrimObj_->SetIntensity(lightIntensity_);
        meshPrimObj_->SetLightColor(lightColor_);

        meshPrimObj_->SetPointLightPos(pointPos_);
        meshPrimObj_->SetPointLightIntensity(pointIntensity_);
        meshPrimObj_->SetPointLightColor(pointColor_);
        meshPrimObj_->SetPointLightRadius(pointRadius_);
        meshPrimObj_->SetPointLightDecay(pointDecay_);

        meshPrimObj_->SetSpotLightPos(spotPos_);
        meshPrimObj_->SetSpotLightDirection(spotDir_);
        meshPrimObj_->SetSpotLightIntensity(spotIntensity_);
        meshPrimObj_->SetSpotLightDistance(spotDistance_);
        meshPrimObj_->SetSpotLightDecay(spotDecay_);
        meshPrimObj_->SetSpotLightCosAngle(cosOuter);
        meshPrimObj_->SetSpotLightCosFalloffStart(cosInner);
        meshPrimObj_->SetSpotLightColor({ spotColor_.x, spotColor_.y, spotColor_.z, 1.0f });

        meshPrimObj_->Update(dt);
    }

    if (alphaBlendObj_) {

        if (alphaBlendIndex_ != alphaBlendIndexPrev_) {
            alphaBlendIndexPrev_ = alphaBlendIndex_;
            alphaBlendObj_->SetModel(alphaBlendPaths_[alphaBlendIndex_]);
            alphaBlendObj_->PlayAnimation("", true);
            // ★透明確認：モデル切替してもブレンドを維持したいならここでもセット
            alphaBlendObj_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNormal);
        }

        alphaBlendObj_->SetTranslate({ testPos_.x + 6.0f, testPos_.y, testPos_.z });
        alphaBlendObj_->SetRotate(testRot_);
        alphaBlendObj_->SetScale(testScale_);

        alphaBlendObj_->SetEnableLighting(lighting);
        alphaBlendObj_->SetShininess(shininess_);
        alphaBlendObj_->SetDirection(lightDir_);
        alphaBlendObj_->SetIntensity(lightIntensity_);
        alphaBlendObj_->SetLightColor(lightColor_);

        alphaBlendObj_->SetPointLightPos(pointPos_);
        alphaBlendObj_->SetPointLightIntensity(pointIntensity_);
        alphaBlendObj_->SetPointLightColor(pointColor_);
        alphaBlendObj_->SetPointLightRadius(pointRadius_);
        alphaBlendObj_->SetPointLightDecay(pointDecay_);

        alphaBlendObj_->SetSpotLightPos(spotPos_);
        alphaBlendObj_->SetSpotLightDirection(spotDir_);
        alphaBlendObj_->SetSpotLightIntensity(spotIntensity_);
        alphaBlendObj_->SetSpotLightDistance(spotDistance_);
        alphaBlendObj_->SetSpotLightDecay(spotDecay_);
        alphaBlendObj_->SetSpotLightCosAngle(cosOuter);
        alphaBlendObj_->SetSpotLightCosFalloffStart(cosInner);
        alphaBlendObj_->SetSpotLightColor({ spotColor_.x, spotColor_.y, spotColor_.z, 1.0f });

        alphaBlendObj_->Update(dt);
    }

    if (texSamplerObj_) {

        if (texSamplerIndex_ != texSamplerIndexPrev_) {
            texSamplerIndexPrev_ = texSamplerIndex_;
            texSamplerObj_->SetModel(texSamplerPaths_[texSamplerIndex_]);
            texSamplerObj_->PlayAnimation("", true);
        }

        texSamplerObj_->SetTranslate({ testPos_.x, testPos_.y, testPos_.z + 6.0f });
        texSamplerObj_->SetRotate(testRot_);
        texSamplerObj_->SetScale(testScale_);

        texSamplerObj_->SetEnableLighting(lighting);
        texSamplerObj_->SetShininess(shininess_);
        texSamplerObj_->SetDirection(lightDir_);
        texSamplerObj_->SetIntensity(lightIntensity_);
        texSamplerObj_->SetLightColor(lightColor_);

        texSamplerObj_->SetPointLightPos(pointPos_);
        texSamplerObj_->SetPointLightIntensity(pointIntensity_);
        texSamplerObj_->SetPointLightColor(pointColor_);
        texSamplerObj_->SetPointLightRadius(pointRadius_);
        texSamplerObj_->SetPointLightDecay(pointDecay_);

        texSamplerObj_->SetSpotLightPos(spotPos_);
        texSamplerObj_->SetSpotLightDirection(spotDir_);
        texSamplerObj_->SetSpotLightIntensity(spotIntensity_);
        texSamplerObj_->SetSpotLightDistance(spotDistance_);
        texSamplerObj_->SetSpotLightDecay(spotDecay_);
        texSamplerObj_->SetSpotLightCosAngle(cosOuter);
        texSamplerObj_->SetSpotLightCosFalloffStart(cosInner);
        texSamplerObj_->SetSpotLightColor({ spotColor_.x, spotColor_.y, spotColor_.z, 1.0f });

        texSamplerObj_->Update(dt);
    }

    if (particle_ && assimpPlaneIndex_ != assimpPlaneIndexPrev_) {
        assimpPlaneIndexPrev_ = assimpPlaneIndex_;

        particle_->SetModel(assimpPlanePaths_[assimpPlaneIndex_]);

        // 見えなくなった時の保険（任意）
        // particle_->Reset(); みたいな関数があるなら呼ぶ
    }

    // ===== カメラ反映 =====
    if (camera_) {
        camera_->SetTranslate(imguiCamPos_);
        camera_->SetRotate(imguiCamRot_);
        camera_->Update();
    }

    // ★重要：Spawn → Update（Spawnしないと instanceCount_ が0のまま）
    if (particle_) {
       // particle_->SpawnParticle(); // emitter.frequency=0.5なので0.5秒ごとに出る
       particle_->Update();
    }
}

void TitleScene::Draw(GameApp& app) {

    app.ObjCom()->SetGraphicsPipelineState();

	//skyDome_->Draw();

    if (testObj_) testObj_->Draw();
    if (terrainObj_) terrainObj_->Draw();
	if (nodeObj_) nodeObj_->Draw();
    if (nodeMiscObj_) nodeMiscObj_->Draw();
    if (skinObj_) skinObj_->Draw();
    if (meshPrimObj_)   meshPrimObj_->Draw();
    if (texSamplerObj_) texSamplerObj_->Draw();
    if (alphaBlendObj_) alphaBlendObj_->Draw();

    // ★PSO/RS をここでセット（Particle::Draw は rootにSRV/CBV積むだけ）
    app.ParticleCom()->SetGraphicsPipelineState();

    // ★PreDraw/SrvPreDraw/PostDraw は GameApp がやるのでここでは絶対呼ばない
    particle_->Draw();

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


}

void TitleScene::DrawImGui_ModelSwitchBlock(const char* header,
    const char* comboLabel,
    int& index,
    const char* const* paths,
    int count,
    const char* const* labels,
    int labelCount)
{
    // 折りたたみ（好きなら TreeNode でもOK）
    if (ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen)) {

        if (labels && labelCount == count) {
            ImGui::Combo(comboLabel, &index, labels, labelCount);
        }
        else {
            ImGui::Combo(comboLabel, &index, paths, count);
        }

        ImGui::Text("Path: %s", paths[index]);
        ImGui::Separator();
    }
}

void TitleScene::DrawImGui_ModelSwitchersOneWindow()
{
    ImGui::Begin("Model Switchers");  // ★ウィンドウはこれ1個だけ

    // Node
    {
        static const char* labelsNode[] = { "00","01","02","03","04","05" };
        DrawImGui_ModelSwitchBlock(
            "Animation_Node",
            "Index##Node",
            nodeModelIndex_,
            nodeModelPaths_.data(),
            (int)nodeModelPaths_.size(),
            labelsNode,
            IM_ARRAYSIZE(labelsNode)
        );
    }

    // NodeMisc
    {
        static const char* labelsMisc[] = { "00","01","02","03","04","05","06","07","08" };
        DrawImGui_ModelSwitchBlock(
            "Animation_NodeMisc",
            "Index##NodeMisc",
            nodeMiscModelIndex_,
            nodeMiscModelPaths_.data(),
            (int)nodeMiscModelPaths_.size(),
            labelsMisc,
            IM_ARRAYSIZE(labelsMisc)
        );
    }

    // Skin
    {
        static const char* labelsSkin[] = { "00","01","02","03","04","05","06","07","08","09","10","11" };
        DrawImGui_ModelSwitchBlock(
            "Animation_Skin",
            "Index##Skin",
            skinModelIndex_,
            skinModelPaths_.data(),
            (int)skinModelPaths_.size(),
            labelsSkin,
            IM_ARRAYSIZE(labelsSkin)
        );
    }

    // Mesh_Primitives
    {
        static const char* labelsMeshPrim[] = { "00" };
        DrawImGui_ModelSwitchBlock(
            "Mesh_Primitives",
            "Index##MeshPrim",
            meshPrimIndex_,
            meshPrimPaths_.data(),
            (int)meshPrimPaths_.size(),
            labelsMeshPrim,
            IM_ARRAYSIZE(labelsMeshPrim)
        );
    }

    // Material_AlphaBlend
    {
        static const char* labelsAlpha[] = { "00","01","02","03","04","05","06" };
        DrawImGui_ModelSwitchBlock(
            "Material_AlphaBlend",
            "Index##Alpha",
            alphaBlendIndex_,
            alphaBlendPaths_.data(),
            (int)alphaBlendPaths_.size(),
            labelsAlpha,
            IM_ARRAYSIZE(labelsAlpha)
        );
    }

    // Texture_Sampler
    {
        static const char* labelsSampler[] = { "00","01","02","03","04","05","06","07","08","09","10","11","12","13" };
        DrawImGui_ModelSwitchBlock(
            "Texture_Sampler",
            "Index##Sampler",
            texSamplerIndex_,
            texSamplerPaths_.data(),
            (int)texSamplerPaths_.size(),
            labelsSampler,
            IM_ARRAYSIZE(labelsSampler)
        );
    }

    // Assimp Plane
    {
        static const char* labelsPlane[] = { "plane.obj", "plane.gltf" };
        DrawImGui_ModelSwitchBlock(
            "Assimp Plane",
            "Plane##Assimp",
            assimpPlaneIndex_,
            assimpPlanePaths_.data(),
            (int)assimpPlanePaths_.size(),
            labelsPlane,
            IM_ARRAYSIZE(labelsPlane)
        );
    }

    ImGui::End();
}
