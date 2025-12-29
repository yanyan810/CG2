#include "GameApp.h"
#include "SceneManager.h"
#include "GameScene.h"  
#include "TitleScene.h"

#include "WinApp.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "ParticleCommon.h"
#include "ImGuiManagaer.h"

#include <Windows.h>

GameApp::GameApp() = default;
GameApp::~GameApp() = default;

int GameApp::Run() {
    if (!Initialize_()) {
        Finalize_();
        return -1;
    }

    // ループ
    while (!quit_) {
        if (win_->ProcessMessage()) break;

        const float dt = 1.0f / 60.0f;

#ifdef ImGUI_DEBUG



        // ★ ImGui フレーム開始（ここで1回だけ）
        imgui_->Begin();
#endif // DEBUG

        // Update
        sceneMgr_->Update(*this, dt);

        // Draw（★PreDraw/SrvPreDraw/PostDraw はここで1回だけ）
        dx_->PreDraw();
        srv_->PreDraw();

      
        sceneMgr_->Draw(*this);

#ifdef ImGUI_DEBUG
        imgui_->End(dx_->GetCommandList());

#endif // DEBUG

        dx_->PostDraw();
    }

    Finalize_();
    return 0;
}



bool GameApp::Initialize_() {

    OutputDebugStringA("[GameApp] Initialize BEGIN\n");

    win_ = std::make_unique<WinApp>();
    win_->Initialize();

    dx_ = std::make_unique<DirectXCommon>();
    dx_->Initialize(win_.get());

    srv_ = std::make_unique<SrvManager>();
    srv_->Initialize(dx_.get());

    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->Initialize(dx_.get());

    TextureManager::GetInstance()->Initialize(dx_.get(), srv_.get());

    ModelManager::GetInstance()->Initialize(dx_.get());

    objCommon_ = std::make_unique<Object3dCommon>();
    objCommon_->Initialize(dx_.get());

    particleCommon_ = std::make_unique<ParticleCommon>();
    particleCommon_->Initialize(dx_.get());

    imgui_ = std::make_unique<ImGuiManagaer>();
    imgui_->Initialize(win_.get(), dx_.get(), srv_.get());

    sceneMgr_ = std::make_unique<SceneManager>();

    // ★ここで登録（1回だけ）
    sceneMgr_->Register("Title", [] { return std::make_unique<TitleScene>(); });
    sceneMgr_->Register("Game", [] { return std::make_unique<GameScene>();  });

    // ★最初は Title
    sceneMgr_->Change(*this, "Title");


    OutputDebugStringA("[GameApp] Initialize END\n");

    return true;



}

void GameApp::Finalize_() {
    // Scene 終了（必要ならここで current_->OnExit 呼んでもOK）

    if (imgui_) imgui_->Shutdown();

    TextureManager::GetInstance()->Finalize();
    ModelManager::GetInstance()->Finalize();

    if (win_) win_->Finalize();

    if (dx_) dx_->ReportLiveObjects();

    sceneMgr_.reset();
    imgui_.reset();
    particleCommon_.reset();
    objCommon_.reset();
    spriteCommon_.reset();
    srv_.reset();
    dx_.reset();
    win_.reset();
}

void GameApp::Update(float dt) {
    OutputDebugStringA("[GameApp] Update\n");
    sceneMgr_->Update(*this, dt); // ここがあるかが重要
}

void GameApp::Draw() {
    OutputDebugStringA("[GameApp] Draw\n");

    dx_->PreDraw();
    srv_->PreDraw();

    sceneMgr_->Draw(*this); // ここがあるかが重要

    dx_->PostDraw();

}

