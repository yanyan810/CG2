#include "GameClearScene.h"
#include <Windows.h>
#include <algorithm>


#include "Camera.h"
#include "GameApp.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"

#include "Object3d.h"
#include "ModelManager.h"

static float Clamp01(float x) { return std::clamp(x, 0.0f, 1.0f); }

void GameClearScene::OnEnter(GameApp& app) {


    camera_ = std::make_unique<Camera>();
    camera_->SetRotate({ 0.35f, 0.0f, 0.0f });
    camera_->SetTranslate({ 0.0f, -10.0f, -0.0f });

    // 3D共通にこのカメラを使わせる

    app.ObjCom()->SetDefaultCamera(camera_.get());

    state_ = State::EnterOpen;

    circle_ = 0.0f;
    softness_ = 0.6f;

    prevSpace_ = false;
    prevEnter_ = false;

    objScaleT_ = 0.0f;

    // ※ GameOverと同じ damage.obj を流用してもOK
    damageObj_ = std::make_unique<Object3d>();
    damageObj_->Initialize(app.ObjCom(), app.Dx());

    // 好きなモデルに差し替え可（例：clear.obj）
    // damageObj_->SetModel("ui/clear/clear.obj");
    damageObj_->SetModel("Player/clear/clear.obj");

    damageObj_->SetTranslate({ 0.0f, 10.0f, 0.0f });
    damageObj_->SetRotate({ 0.0f, 1.4f, 0.0f });
    damageObj_->SetScale({ 1.0f, 1.0f, 1.0f });

	skyDome_ = std::make_unique<Object3d>();
	skyDome_->Initialize(app.ObjCom(), app.Dx());
	skyDome_->SetModel("skydome/SkyDome.obj");

   

    // オブジェクトにも一応セット（Object3d が個別カメラ方式なら必要）
    damageObj_->SetCamera(camera_.get());

    // ===== 2D UI =====
    TextureManager::GetInstance()->LoadTexture("resources/ui/char/clear.png");
    TextureManager::GetInstance()->LoadTexture("resources/ui/char/goTitle.png");

    bg_ = std::make_unique<Sprite>();
    bg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/char/clear.png");
    bg_->SetAnchorPoint({ 0.0f, 0.0f });
    bg_->SetPosition({ 0.0f, 0.0f });
    bg_->SetScale({ 1.0f, 1.0f, 1.0f });

    goTitle_ = std::make_unique<Sprite>();
    goTitle_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/char/goTitle.png");
    goTitle_->SetAnchorPoint({ 0.5f, 0.5f });
    goTitle_->SetPosition({ 1280.0f * 0.5f, 720.0f * 0.82f }); // 下の真ん中
    goTitle_->SetScale({ 1.0f, 1.0f, 1.0f });


}

void GameClearScene::OnExit(GameApp&) {
    goTitle_.reset();
    bg_.reset();
    //clearObj_.reset();
    skyDome_.reset();
    camera_.reset();
}


void GameClearScene::Update(GameApp& app, float dt) {
    bool spaceNow = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    bool enterNow = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;

    bool spaceTrig = spaceNow && !prevSpace_;
    bool enterTrig = enterNow && !prevEnter_;

    prevSpace_ = spaceNow;
    prevEnter_ = enterNow;

    skyDome_->Update();

	rotateYAngle_ += 0.05f * dt; // ゆっくり回転

    skyDome_->SetRotate({ 0.0f ,rotateYAngle_,0.0f });

    switch (state_) {
    case State::EnterOpen:
        circle_ = Clamp01(circle_ + 1.35f * dt);
        objScaleT_ = Clamp01(objScaleT_ + 2.5f * dt);
        if (circle_ >= 1.0f) state_ = State::Idle;
        break;

    case State::Idle:
        // 決定だけ（Titleへ）
        if (spaceTrig || enterTrig) {
            state_ = State::ExitClose;
        }
        break;

    case State::ExitClose:
        circle_ = Clamp01(circle_ - 1.8f * dt);
        if (circle_ <= 0.0f) {
            RequestChangeScene_(kNextTitle_);
        }
        break;
    }

#ifdef USE_IMGUI
	// ===== ImGui =====
    ImGui::Begin("Clear");
    ImGui::DragFloat("clearPosZ", &clearPosZ_, 0.1f);
    ImGui::End();

#endif

}

void GameClearScene::Draw(GameApp& app) {
    app.ObjCom()->SetGraphicsPipelineState();

	skyDome_->Draw();

    if (damageObj_) {

        damageObj_->SetTranslate({ 0.0f, -2.0f, clearPosZ_ });

        // “クリアっぽくドン”：大きめに
        const float s = 0.002f + objScaleT_ * 1.2f;
        damageObj_->SetScale({ s, s, s });

        damageObj_->Update();
        damageObj_->Draw();
    }

    // ===== 2D =====
    app.SpriteCom()->SetGraphicsPipelineState();

    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0, 100);

    if (bg_) {
        bg_->Update(view, proj);
        bg_->Draw();
    }

    // 点滅（alphaをSpriteが持ってる前提。無いなら消してOK）
    if (goTitle_) {
        const float a = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(uiTime_ * 4.0f));
        goTitle_->SetColor({ 1,1,1,a });

        goTitle_->Update(view, proj);
        goTitle_->Draw();
    }


    // 円マスク（最後）
    app.SpriteCom()->DrawCircleMask(circle_, softness_);
}
