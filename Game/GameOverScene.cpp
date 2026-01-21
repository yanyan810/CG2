#include "GameOverScene.h"
#include <Windows.h>
#include <algorithm>

#include "GameApp.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"

#include "Object3d.h"
#include "ModelManager.h"   // LoadModel がある想定

#include "Sprite.h"
#include "TextureManager.h"
#include "WinApp.h"

static float Clamp01(float x) { return std::clamp(x, 0.0f, 1.0f); }

void GameOverScene::OnEnter(GameApp& app) {

   app.ObjCom()->SetDefaultCamera(camera_.get());

   // 画像ロード（1回でOKなら別の場所でもOK）
   TextureManager::GetInstance()->LoadTexture("resources/ui/char/gameOver.png");
   TextureManager::GetInstance()->LoadTexture("resources/ui/char/retry.png");
   TextureManager::GetInstance()->LoadTexture("resources/ui/char/goTitle.png");


    state_ = State::EnterOpen;
    select_ = Select::Retry;
    decided_ = Select::Retry;

    circle_ = 0.0f;
    softness_ = 0.6f;

    damageScale_ = 0.0f;
    damageAlpha_ = 0.0f;

    prevSpace_ = false;
    prevEnter_ = false;

    // damage.obj 表示（Object3dで出す）
    damageObj_ = std::make_unique<Object3d>();
    damageObj_->Initialize(app.ObjCom(), app.Dx());

    // ★ここはあなたのプロジェクト内パスに合わせる
    // /mnt/data/damage.obj はこのチャット環境専用なのでPC側では使えません
    //auto* model = ModelManager::GetInstance()->LoadModel("resources/models/damage.obj");
    damageObj_->SetModel("Player/damage/damage.obj");

    // ざっくり中央（必要なら調整）
    damageObj_->SetTranslate( { 0.0f, 0.0f, 0.0f });
    damageObj_->SetRotate({ 120.0f, 180.0f, 90.0f });
    damageObj_->SetScale( { 1.0f, 1.0f, 1.0f });


    skyDome_ = std::make_unique<Object3d>();
    skyDome_->Initialize(app.ObjCom(), app.Dx());
    skyDome_->SetModel("skydome/SkyDome.obj");

    bg_ = std::make_unique<Sprite>();
    bg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/char/gameOver.png");
    bg_->SetAnchorPoint({ 0,0 });
    bg_->SetPosition({ 0,0 });
    bg_->SetScale({ 1,1,1 }); // 1280x720ならそのまま

    retrySp_ = std::make_unique<Sprite>();
    retrySp_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/char/retry.png");
    retrySp_->SetAnchorPoint({ 0.5f, 0.5f });
    retrySp_->SetPosition({ 1280.0f * 0.5f - 180.0f, 720.0f * 0.72f });
    retrySp_->SetScale({ 2,2,1 }); // 128x128想定
	retrySp_->SetColor({ 0,0,0,1 });


    titleSp_ = std::make_unique<Sprite>();
    titleSp_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/char/goTitle.png");
    titleSp_->SetAnchorPoint({ 0.5f, 0.5f });
    titleSp_->SetPosition({ 1280.0f * 0.5f + 180.0f, 720.0f * 0.72f });
    titleSp_->SetScale({ 2,2,1 });
    titleSp_->SetColor({ 0,0,0,1 });

}

void GameOverScene::OnExit(GameApp& app) {
    damageObj_.reset();
    skyDome_.reset();
}

void GameOverScene::Update(GameApp& app, float dt) {
    // ここも TitleScene と同じく GetAsyncKeyState でOK（Inputを使ってもOK）
    bool spaceNow = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    bool enterNow = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;

    bool spaceTrigger = spaceNow && !prevSpace_;
    
    skyDome_->Update(dt);
    bool enterTrigger = enterNow && !prevEnter_;

    prevSpace_ = spaceNow;
    prevEnter_ = enterNow;

    switch (state_) {
    case State::EnterOpen:
    {
        // 円を開く：0→1
        circle_ = Clamp01(circle_ + 1.35f * dt);

        // damageの出現（0→1へ近づける）
        damageAlpha_ = Clamp01(damageAlpha_ + 2.5f * dt);
        damageScale_ = Clamp01(damageScale_ + 1.0f * dt);

        if (circle_ >= 1.0f) {
            state_ = State::Idle;
        }
    } break;

    case State::Idle:
    {
        const Input* input = app.GetInput();

        if (input->IsKeyPressed(DIK_LEFT) || input->IsKeyPressed(DIK_A)) {
            select_ = Select::Retry;
        }
        if (input->IsKeyPressed(DIK_RIGHT) || input->IsKeyPressed(DIK_D)) {
            select_ = Select::Title;
        }

        // 決定（SPACE or ENTER）
        if (spaceTrigger || enterTrigger) {
            decided_ = select_;
            state_ = State::ExitClose;
        }
    } break;

    case State::ExitClose:
    {
        // 円を閉じる：1→0
        circle_ = Clamp01(circle_ - 1.8f * dt);

        if (circle_ <= 0.0f) {
            // 真っ黒になった瞬間にシーン遷移
            if (decided_ == Select::Retry) {
                RequestChangeScene_(kNextRetry_);
            } else {
                RequestChangeScene_(kNextTitle_);
            }
        }
    } break;
    }

    // damage.obj（3D）を描く
    if (damageObj_) {
        // ★ 顔を大きく見せたいので倍率を強める
        const float base = 0.005f;                // ← 基本サイズ（ここが超重要）
        const float punch = damageScale_ * 1.5f; // ← ドン！と来る分
        const float s = base + punch;

        damageObj_->SetScale({ s, s, s });

        // ※ Object3d に色を渡せるなら alpha を使う（無ければ無視でOK）
        // damageObj_->SetColor({1,1,1,damageAlpha_});

        damageObj_->Update(dt);

       
    }

}

void GameOverScene::Draw(GameApp& app) {

    app.ObjCom()->SetGraphicsPipelineState();

    skyDome_->Draw();

    // damage.obj（3D）を描く
    if (damageObj_) {
     
        // ★Object3dCommon の PSO/RS セットが必要ならここで呼ぶ
        // 例：app.ObjCom()->SetGraphicsPipelineState();
        damageObj_->Draw();
    }

    // 2D
    app.SpriteCom()->SetGraphicsPipelineState();

    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0, 100);

    if (bg_) { bg_->Update(view, proj); bg_->Draw(); }

    // 選択中の強調（例：選択中だけ少し大きく）
    if (retrySp_) retrySp_->SetScale(select_ == Select::Retry ? Vector3{ 1.8f,1.8f,1 } : Vector3{ 1,1,1 });
    if (titleSp_) titleSp_->SetScale(select_ == Select::Title ? Vector3{ 1.8f,1.8f,1 } : Vector3{ 1,1,1 });

    if (retrySp_) { retrySp_->Update(view, proj); retrySp_->Draw(); }
    if (titleSp_) { titleSp_->Update(view, proj); titleSp_->Draw(); }

    // 最後に円マスク（必ず最後）
    app.SpriteCom()->DrawCircleMask(circle_, softness_);
}
