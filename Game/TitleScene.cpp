#include "TitleScene.h"
#include <Windows.h>

#include "GameApp.h"
#include "Particle.h"
#include "ParticleCommon.h"
#include "Camera.h"

void TitleScene::OnEnter(GameApp& app) {
    // カメラ（Particle は内部カメラでも動くけど、外部カメラを渡すと安定）
    camera_ = std::make_unique<Camera>();
    camera_->SetRotate({ 0, 0, 0 });
    camera_->SetTranslate({ 0, 3, -20 });

    // Particle
    particle_ = std::make_unique<Particle>();
    particle_->Initialize(app.ParticleCom(), app.Dx(), app.Srv());

    // ★これが無いと「model_ が null」で Draw しても何も出ません
    particle_->SetModel("plane.obj"); // 既にある板ポリ等に合わせて

    // カメラを渡せるなら渡す（Particle.cpp は camera_ があればそれを使う仕様）
    particle_->SetCamera(camera_.get());
    Vector3 rotate = { 0.0f,0.0f,0.0f };

    particle_->SetRotate(rotate);
    // 見えやすいように（任意）
    particle_->SetBlendMode(ParticleCommon::BlendMode::kBlendModeAdd);
    particle_->SetMaterialColor({ 1, 1, 1, 1 });
}

void TitleScene::OnExit(GameApp&) {
    particle_.reset();
    camera_.reset();
}

void TitleScene::Update(GameApp& app, float dt) {
    // SPACEでGameへ
    bool now = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    bool trigger = now && !prevSpace_;
    prevSpace_ = now;
    if (trigger) {
        RequestChangeScene_("Game");
    }



#ifdef USE_IMGUI

    // ===== ImGui =====
    ImGui::Begin("Camera Debug");

    ImGui::DragFloat3("Position", &imguiCamPos_.x, 0.1f);
    ImGui::DragFloat3("Rotation", &imguiCamRot_.x, 0.01f);

    ImGui::End();
    
#endif // DEBUG

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
    if (!particle_) return;

    // ★PSO/RS をここでセット（Particle::Draw は rootにSRV/CBV積むだけ）
    app.ParticleCom()->SetGraphicsPipelineState();

    // ★PreDraw/SrvPreDraw/PostDraw は GameApp がやるのでここでは絶対呼ばない
    particle_->Draw();
}
