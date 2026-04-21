#include "GameApp.h"
#include "SceneManager.h"
#include "GameScene.h"  
#include "TitleScene.h"
#include"DeckEditScene.h"
#include "TestScene.h"
#include "GameOverScene.h"
#include "GameClearScene.h"
#include "TutorialScene.h"
#include "StageSelectScene.h"


#include "WinApp.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "ParticleCommon.h"
#include "ImGuiManagaer.h"
#include "ModelParticleManager.h"

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

		sceneMgr_->DrawSkydome(*this);

		// ポストエフェクト描画
		bloom_->PreDraw();

		sceneMgr_->DrawPostEffect3D(*this);
		sceneMgr_->DrawPostEffect2D(*this);

		bloom_->PostDraw();

		// 普通の描画
		Draw3D();
		Draw2D();

#ifdef USE_IMGUI
		DrawImGui();
		imgui_->End(dx_->GetCommandList());
#endif

		dx_->PostDraw();
	}

	Finalize_();
	return 0;
}

namespace {

	int RandomRangeInt(int minValue, int maxValue)
	{
		static std::random_device rd;
		static std::mt19937 mt(rd());
		std::uniform_int_distribution<int> dist(minValue, maxValue);
		return dist(mt);
	}

	CardSuit RandomSuit()
	{
		int v = RandomRangeInt(0, 3);
		return static_cast<CardSuit>(v);
	}

	CardInstance MakeCardInstance(int defId)
	{
		CardInstance c{};
		c.defId = defId;
		c.number = RandomRangeInt(1, 13);
		c.suit = RandomSuit();
		return c;
	}
}

bool GameApp::Initialize_() {
	OutputDebugStringA("[GameApp] Initialize START\n");

	TextSprite::InitFontSystem();

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

	win_->SetInputPointer(input_.get());

	WarmupAssets_();

	ModelParticleManager::GetInstance()->Initialize(dx_.get(), srv_.get());

	Audio::GetInstance()->Initialize();
	AudioManager::GetInstance()->LoadAllConfigs("resources/configs/audioSettings.json");

	// SceneManager
	sceneMgr_ = std::make_unique<SceneManager>();
	sceneMgr_->Register("Title", [] { return std::make_unique<TitleScene>(); });
	sceneMgr_->Register("DeckEdit", [] { return std::make_unique<DeckEditScene>(); });
	sceneMgr_->Register("Game", [] { return std::make_unique<GameScene>();  });
	sceneMgr_->Register("Test", [] { return std::make_unique<TestScene>();  });
	sceneMgr_->Register("Tutorial", [] { return std::make_unique<TutorialScene>();  });
	sceneMgr_->Register("StageSelect", [] { return std::make_unique<StageSelectScene>(); });
	sceneMgr_->Register("GameOver", [] { return std::make_unique<GameOverScene>();  });
	sceneMgr_->Register("GameClear", [] { return std::make_unique<GameClearScene>();  });


	sceneMgr_->Change(*this, "Title");

	// デフォルトデッキ
	for (int i = 0; i < 2; i++) {
		deckInstances_.push_back(MakeCardInstance(9));
		deckInstances_.push_back(MakeCardInstance(8));
		deckInstances_.push_back(MakeCardInstance(7));
		deckInstances_.push_back(MakeCardInstance(6));
		deckInstances_.push_back(MakeCardInstance(5));
		deckInstances_.push_back(MakeCardInstance(4));
		deckInstances_.push_back(MakeCardInstance(3));
		deckInstances_.push_back(MakeCardInstance(2));
		deckInstances_.push_back(MakeCardInstance(1));
		deckInstances_.push_back(MakeCardInstance(20));
		deckInstances_.push_back(MakeCardInstance(19));
		deckInstances_.push_back(MakeCardInstance(18));
		deckInstances_.push_back(MakeCardInstance(17));
		deckInstances_.push_back(MakeCardInstance(16));
		deckInstances_.push_back(MakeCardInstance(15));
		deckInstances_.push_back(MakeCardInstance(14));
		deckInstances_.push_back(MakeCardInstance(13));
		deckInstances_.push_back(MakeCardInstance(12));
		deckInstances_.push_back(MakeCardInstance(11));
		deckInstances_.push_back(MakeCardInstance(10));
	}

	cardDB_ = std::make_unique<CardDatabase>();
	cardDB_->LoadFromJson("resources/cards/cards.json");


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
	if (dx_) dx_->Release();

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

	//テクスチャ初回読み込み
	TextureManager::GetInstance()->LoadTexture("resources/shadow/shadow.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/activatingEffect.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/activation.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/back.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/backChooseActive.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/chooseActive.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/doActivation.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/effectsList.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/noActivation.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/showField.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/damage.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/draw.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/attakUp.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/hand.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/deck.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/discard.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/num/0.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/num/1.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/num/2.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/num/3.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/num/4.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/num/5.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/num/6.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/num/7.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/num/8.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/num/9.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/white.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/enemySingle.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/enemyAll.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/self.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/ni.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/ha.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/cost.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/power.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/x1.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/blockCountBlue.png");

	TextureManager::GetInstance()->LoadTexture("resources/ui/text/basicEffect.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/startTurn.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/specialEffectsActivat.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/playerField.png");

	TextureManager::GetInstance()->LoadTexture("resources/ui/text/onePair.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/twoPair.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/threeCard.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/straightType.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/flashType.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/fullHouse.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/fourCard.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/straightFlash.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/RoyalStraightFlush.png");

	TextureManager::GetInstance()->LoadTexture("resources/ui/text/inTheCase.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/inTheAboveCases.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/colon.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/nasi.png");

	TextureManager::GetInstance()->LoadTexture("resources/ui/text/heal.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/block.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/nextTurnATKUP.png");

	TextureManager::GetInstance()->LoadTexture("resources/ui/card_desc/desc_6.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/card_desc/desc_17.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/card_desc/desc_18.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/card_desc/desc_19.png");

	TextureManager::GetInstance()->LoadTexture("resources/ui/PauseMenu.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/GiveUpCheck.png");


	TextureManager::GetInstance()->LoadTexture("resources/ui/stage_select/bg.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/stage_select/title_stage_select.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/stage_select/button_tutorial.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/stage_select/button_battle.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/stage_select/desc_bg.png");

	// モデル初回読み込み
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
	ModelManager::GetInstance()->LoadModel("triangleParticle.obj");



	OutputDebugStringA("[Warmup] END\n");
}

void GameApp::SetDeckInstancesFromId(const std::vector<int>& ids) {
	deckInstances_.clear();
	for (const auto& id : ids) {
		deckInstances_.push_back(MakeCardInstance(id));
	}
}