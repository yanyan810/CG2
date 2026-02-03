#include "TitleScene.h"
#include <Windows.h>

#include "GameApp.h"
#include "Particle.h"
#include "ParticleCommon.h"
#include "Camera.h"

struct SRT {
    Vector3 pos{ 0,0,0 };
    Vector3 rot{ 0,0,0 };   // degで管理が楽（必要ならSet時にradへ）
    Vector3 scale{ 1,1,1 };
};




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

    // ★スカイドームは基本「ライト無視」
    skyDome_->SetEnableLighting(0);              // ← あなたの仕様の「無照明モード」に合わせて
    skyDome_->SetMaterialColor({ 1,1,1,1 });       // 念のため
    skyDome_->SetShininess(1.0f);                // 影響しないけど保険
    skyDome_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

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

    // ===== Video Plane =====
    videoPlane_ = std::make_unique<Object3d>();
    videoPlane_->Initialize(app.ObjCom(), app.Dx());
    videoPlane_->SetModel("video/plane.obj");

    // 表示しやすい設定（ライト無視）
    videoPlane_->SetEnableLighting(0);
    videoPlane_->SetMaterialColor({ 1,1,1,1 });
    videoPlane_->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);

    // 位置調整（カメラの前に置く）
    videoPlane_->SetTranslate({ 0.0f, 3.0f, 3.1f });  // 例：Zはカメラ向きに合わせて調整
    videoPlane_->SetScale({ 9.5f, 5.3f, 2.0f });
    videoPlane_->SetRotate({ 0.0f, 0.0f, 0.0f });

    // ===== Video Player =====
    video_ = std::make_unique<VideoPlayerMF>();

    // ※ ここはあなたの実装の関数名に合わせて
    video_->Open("resources/video/battle.mp4", true); // loop = true
    video_->CreateDxResources(app.Dx()->GetDevice(), app.Srv()); // SRV確保 + texture作成
	video_->SetVolume(1.5f); // 音量セット（0.0f〜1.0f）
    enableVideo_ = true;

    bool ok = video_->ReadNextFrame();  // ★最初の1枚
    if (!ok) {
        OutputDebugStringA("[TitleScene] First ReadNextFrame failed\n");
    }


    // Sky
    srtSky_.pos = { 0,0,0 };
    srtSky_.rot = { 0,0,0 };
    srtSky_.scale = { 1,1,1 };

    // VideoPlane（あなたの初期値と合わせる）
    srtVideo_.pos = { 0.0f, 1.0f, 3.0f };
    srtVideo_.rot = { 0.0f, 0.0f, 0.0f };
    srtVideo_.scale = { 2.0f, 2.0f, 2.0f };

    // BG / Press（2D）
    srtBG_.pos = { 0,0,0 };
    srtBG_.scale = { 1,1,1 };
    srtPress_.pos = { 0,0,0 };
    srtPress_.scale = { 1,1,1 };


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
    video_->Close();
    video_.reset();

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

    switch (state_) {
    case State::Idle:
        if (spaceTrig) {
            state_ = State::ExitClose; // ★すぐ遷移しない
        }
        break;

    case State::ExitClose:
        circle_ -= 1.8f * dt;          // ★閉じる
        if (circle_ <= 0.0f) {
            circle_ = 0.0f;
            RequestChangeScene_(kNextScene_);
        }
        break;
    }


    skyDome_->Update(dt);
    if (enableVideo_ && video_) {

        videoPlane_->Update(dt);

        // ★音は足りない時だけ読む（軽い）
        video_->PumpAudio();

        // ★映像は毎フレーム1回（または2回）読む
        video_->ReadNextVideoFrame();
    }




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
     "SkyDome",
     "VideoPlane",
     "Particle",
     "BG(Sprite)",
     "PressStart(Sprite)",
    };
    ImGui::Combo("Target", &editTarget_, targetLabels, IM_ARRAYSIZE(targetLabels));


    // ターゲットに応じて参照先を切り替え
    SRT* cur = nullptr;
    switch ((EditTarget)editTarget_) {
    case EditTarget::SkyDome:     cur = &srtSky_; break;
    case EditTarget::VideoPlane:  cur = &srtVideo_; break;
    case EditTarget::Particle:    cur = &srtParticle_; break;
    case EditTarget::BG:          cur = &srtBG_; break;
    case EditTarget::PressStart:  cur = &srtPress_; break;
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

    ImGui::Begin("Video");
    ImGui::Checkbox("Enable Video", &enableVideo_);
    ImGui::End();

    ImGui::Begin("Video");

    if (video_) {
        int n = video_->GetAudioTrackCount();
        ImGui::Text("Audio tracks: %d", n);

        static int track = 0;
        if (n > 0) {
            if (track >= n) track = n - 1;

            if (ImGui::BeginCombo("Audio Track", ("Track" + std::to_string(track + 1)).c_str())) {
                for (int i = 0; i < n; ++i) {
                    std::string label = "Track" + std::to_string(i + 1);
                    bool selected = (i == track);
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        track = i;
                        video_->SetAudioTrack(i);
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
    }

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


  
    // ===== カメラ反映 =====
    if (camera_) {
        camera_->SetTranslate(imguiCamPos_);
        camera_->SetRotate(imguiCamRot_);
        camera_->Update();
    }

    auto ApplyObject3dSRT = [](Object3d* o, const SRT& s) {
        if (!o) return;
        o->SetTranslate(s.pos);
        o->SetRotate(s.rot);   // ※Object3dがdeg前提ならOK。radなら変換が必要。
        o->SetScale(s.scale);
        };

    auto ApplySpriteSRT = [](Sprite* sp, const SRT& s) {
        if (!sp) return;
        // Spriteが2D(Vector2)なら適宜合わせる
        sp->SetPosition({ s.pos.x, s.pos.y });
        sp->SetScale({ s.scale.x, s.scale.y, 1.0f }); // あなたのSpriteがVector3ならそのまま
        // Spriteに回転があるなら sp->SetRotation(s.rot.z) など
        };

    // 3D
    ApplyObject3dSRT(skyDome_.get(), srtSky_);
    ApplyObject3dSRT(videoPlane_.get(), srtVideo_);
    if (particle_) {
        particle_->SetTranslate(srtParticle_.pos);
        particle_->SetRotate(srtParticle_.rot);
        particle_->SetScale(srtParticle_.scale); // Particleに無ければ外す
    }

    // 2D
    ApplySpriteSRT(bg_.get(), srtBG_);
    ApplySpriteSRT(pressStart_.get(), srtPress_);


}

void TitleScene::Draw(GameApp& app) {

    app.ObjCom()->SetGraphicsPipelineState();

	skyDome_->Draw();

    // ===== Video Plane =====
    if (enableVideo_ && videoPlane_ && video_) {
        auto* cmd = app.Dx()->GetCommandList();
        char msg[256]{};
        sprintf_s(msg, "[TitleScene] video srv gpu ptr=0x%llX\n", (unsigned long long)video_->SrvGpu().ptr);
        OutputDebugStringA(msg);


        // GPUへ転送（Copy + PSR）
        video_->UploadToGpu(cmd);

        // SRV を取得して override 描画
        D3D12_GPU_DESCRIPTOR_HANDLE vh = video_->SrvGpu();
        videoPlane_->DrawWithOverrideSrv(vh);

        // 次のCopyに備える運用なら
        video_->EndFrame(cmd);
    }

    // ★PSO/RS をここでセット（Particle::Draw は rootにSRV/CBV積むだけ）
    app.ParticleCom()->SetGraphicsPipelineState();

    // ★PreDraw/SrvPreDraw/PostDraw は GameApp がやるのでここでは絶対呼ばない
  //  particle_->Draw();

    // ---- 2D ----
    app.SpriteCom()->SetGraphicsPipelineState();

    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0, 100);

   if (bg_) {
        bg_->Update(view, proj);
        bg_->Draw();
    }

    if (pressStart_) {
        pressStart_->Update(view, proj);
        pressStart_->Draw();
    }

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
