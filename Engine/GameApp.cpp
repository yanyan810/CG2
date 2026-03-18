#include "GameApp.h"
#include "SceneManager.h"
#include "GameScene.h"  
#include "TitleScene.h"
#include "TestScene.h"
#include "GameOverScene.h"
#include "GameClearScene.h"

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

    while (!quit_) {
        if (win_->ProcessMessage()) break;

        const float dt = 1.0f / 60.0f;

#ifdef USE_IMGUI
        imgui_->Begin();
#endif

        if (input_) input_->Update();

        sceneMgr_->Update(*this, dt);

        bloom_->Update();

        dx_->PreDraw();
        srv_->PreDraw();

        bloom_->PreDraw();

        Draw3D();
        Draw2D();

        bloom_->PostDraw();

#ifdef USE_IMGUI
        DrawImGui();
        imgui_->End(dx_->GetCommandList());
#endif

        dx_->PostDraw();
    }

    Finalize_();
    return 0;
}

bool GameApp::Initialize_() {
    OutputDebugStringA("[GameApp] Initialize START\n");

    win_ = std::make_unique<WinApp>();
    win_->Initialize();

    dx_ = std::make_unique<DirectXCommon>();
    dx_->Initialize(win_.get());

    srv_ = std::make_unique<SrvManager>();
    srv_->Initialize(dx_.get());

    rtv_ = std::make_unique<RtvManager>();
    rtv_->Initialize(dx_.get());

    bloom_ = std::make_unique<Bloom>();
    bloom_->Initialize(dx_.get(), srv_.get(), rtv_.get());

    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->Initialize(dx_.get());

    TextureManager::GetInstance()->Initialize(dx_.get(), srv_.get());
    ModelManager::GetInstance()->Initialize(dx_.get());

    objCommon_ = std::make_unique<Object3dCommon>();
    objCommon_->Initialize(dx_.get());

    objCommon_->SetSrvManager(srv_.get());
  

    particleCommon_ = std::make_unique<ParticleCommon>();
    particleCommon_->Initialize(dx_.get());

#ifdef USE_IMGUI
    imgui_ = std::make_unique<ImGuiManagaer>();
    imgui_->Initialize(win_.get(), dx_.get(), srv_.get());
#endif

    // GameApp::Initialize など
    skinCom_ = std::make_unique<SkinningCommon>();
    skinCom_->Initialize(dx_.get());
    objCommon_->SetSkinningCommon(skinCom_.get());

    // ★ Input は Scene を動かす前に作る（最重要）
    input_ = std::make_unique<Input>();
    input_->Initialize(win_.get());
    input_->Update(); // 初回


    WarmupAssets_();

    // SceneManager
    sceneMgr_ = std::make_unique<SceneManager>();
    sceneMgr_->Register("Title", [] { return std::make_unique<TitleScene>(); });
    sceneMgr_->Register("Game", [] { return std::make_unique<GameScene>();  });
    sceneMgr_->Register("Test", [] { return std::make_unique<TestScene>();  }); 
    sceneMgr_->Register("GameOver", [] { return std::make_unique<GameOverScene>();  }); 
	sceneMgr_->Register("GameClear", [] { return std::make_unique<GameClearScene>();  });

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

    input_->Update();


    sceneMgr_->Update(*this, dt); // ここがあるかが重要
}

//void GameApp::Draw() {
//    OutputDebugStringA("[GameApp] Draw\n");
//
//    dx_->PreDraw();
//    srv_->PreDraw();
//
//    sceneMgr_->Draw(*this); // ここがあるかが重要
//
//    dx_->PostDraw();
//
//}

void GameApp::Draw3D() {
    sceneMgr_->Draw3D(*this);
}

void GameApp::Draw2D() {
    sceneMgr_->Draw2D(*this);
}

void GameApp::DrawImGui() {
    sceneMgr_->DrawImGui(*this);
}

void GameApp::Draw() {
    Draw3D();
    Draw2D();
#ifdef USE_IMGUI
    DrawImGui();
#endif
}

void GameApp::WarmupAssets_() {
    OutputDebugStringA("[Warmup] START\n");

    // もしテクスチャも初回で刺さるならここで
    TextureManager::GetInstance()->LoadTexture("resources/shadow/shadow.png");

    // モデル（ModelManager がキャッシュする前提）
    ModelManager::GetInstance()->LoadModel("human/walk.gltf");
    ModelManager::GetInstance()->LoadModel("human/sneakWalk.gltf");
    //ModelManager::GetInstance()->LoadModel("gltf/walk.glb");
    ModelManager::GetInstance()->LoadModel("Player/player.gltf");
    ModelManager::GetInstance()->LoadModel("Player/sword.obj");
    ModelManager::GetInstance()->LoadModel("enemy/boss/boss.gltf");
    ModelManager::GetInstance()->LoadModel("cards/models/1.obj");
    ModelManager::GetInstance()->LoadModel("cards/models/2.obj");
    ModelManager::GetInstance()->LoadModel("cards/models/3.obj");
    ModelManager::GetInstance()->LoadModel("cards/models/4.obj");
    ModelManager::GetInstance()->LoadModel("cards/models/5.obj");
    ModelManager::GetInstance()->LoadModel("cards/models/art_plane.obj");
    ModelManager::GetInstance()->LoadModel("cards/models/frame.obj");
    
    OutputDebugStringA("[Warmup] END\n");
}
