#include "GameApp.h"
#include "SceneManager.h"
#include "GameScene.h"   // 今回作るやつ

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

    // 最初のシーン
    sceneMgr_->Register("Game", [] { return std::make_unique<GameScene>(); });
    sceneMgr_->Change(*this, "Game");

    // ループ
    while (!quit_) {
        if (win_->ProcessMessage()) break;

        // dt（簡易：固定でもOK）
        const float dt = 1.0f / 60.0f;

        sceneMgr_->Update(*this, dt);
        sceneMgr_->Draw(*this);
    }

    Finalize_();
    return 0;
}

bool GameApp::Initialize_() {
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
