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
#include "BattleController.h"
#include "BattleAnimeEditerScene.h"

#include "WinApp.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "ParticleCommon.h"
#include "ImGuiManagaer.h"
#include "ModelParticleManager.h"
#include "ParticleManager.h"
#include "ParticleEditor.h"

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <random>

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

#ifdef USE_IMGUI
		ParticleEditor::GetInstance()->Update();
#endif

		sceneMgr_->Update(*this, dt);

		bloom_->Update();
		objectPostEffect_->Update();

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

	objectPostEffect_ = std::make_unique<ObjectPostEffect>();
	objectPostEffect_->Initialize(dx_.get(), srv_.get(), rtv_.get());

	spriteCommon_ = std::make_unique<SpriteCommon>();
	spriteCommon_->Initialize(dx_.get());

	TextureManager::GetInstance()->Initialize(dx_.get(), srv_.get());
	ModelManager::GetInstance()->Initialize(dx_.get());

	objCommon_ = std::make_unique<Object3dCommon>();
	objCommon_->Initialize(dx_.get());

	objCommon_->SetSrvManager(srv_.get());


	particleCommon_ = std::make_unique<ParticleCommon>();
	particleCommon_->Initialize(dx_.get());

	ParticleManager::GetInstance()->Initialize(dx_.get(), srv_.get(), particleCommon_.get());

#ifdef USE_IMGUI
	imgui_ = std::make_unique<ImGuiManagaer>();
	imgui_->Initialize(win_.get(), dx_.get(), srv_.get());
	ParticleEditor::GetInstance()->Initialize();
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
	LoadActionSequenceProfiles_();

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
	sceneMgr_->Register("BattleAnimeEditer", [] { return std::make_unique<BattleAnimeEditerScene>(); });

	// デフォルトデッキ
	for (int i = 0; i < 4; i++) {
		deckInstances_.push_back(MakeCardInstance(21));
		deckInstances_.push_back(MakeCardInstance(22));
		deckInstances_.push_back(MakeCardInstance(23));
		deckInstances_.push_back(MakeCardInstance(24));
		deckInstances_.push_back(MakeCardInstance(25));
		deckInstances_.push_back(MakeCardInstance(26));
		deckInstances_.push_back(MakeCardInstance(27));
		deckInstances_.push_back(MakeCardInstance(28));
		deckInstances_.push_back(MakeCardInstance(29));
		deckInstances_.push_back(MakeCardInstance(30));
	}

	cardDB_ = std::make_unique<CardDatabase>();
	cardDB_->LoadFromJson("resources/cards/cards.json");

	// 事前にカードなどの全アセットを読み込んでおく（画面遷移時のカクつき防止）
	BattleController dummyBattle;
	dummyBattle.Preload(*this);

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

#ifdef USE_IMGUI
	ParticleEditor::GetInstance()->Update();
#endif

	sceneMgr_->Update(*this, dt); // ここがあるかが重要

	Camera* cam = objCommon_->GetDefaultCamera();
	if (cam) {
		ParticleManager::GetInstance()->Update(dt, *cam);
	}
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
	auto* cmd = dx_->GetCommandList();
	
	// すべての3D描画の前にパーティクルのコンピュート処理を実行
	ParticleManager::GetInstance()->UpdateCompute(cmd);

	sceneMgr_->Draw3D(*this);

	// 3D描画の最後にパーティクルを描画（最前面に出るように）
	ParticleManager::GetInstance()->Draw(cmd);
}

void GameApp::Draw2D() {
	sceneMgr_->Draw2D(*this);
}

void GameApp::DrawImGui() {
	sceneMgr_->DrawImGui(*this);
#ifdef USE_IMGUI
	ParticleEditor::GetInstance()->DrawImGui();
#endif
}

void GameApp::Draw() {
	Draw3D();
	Draw2D();
#ifdef USE_IMGUI
	DrawImGui();
#endif
}

void GameApp::BeginObjectPostEffect() {
	objectPostEffect_->BeginCapture();
}

void GameApp::EndObjectPostEffect() {
	objectPostEffect_->EndCapture();
}

void GameApp::EndObjectPostEffectToBloomScene() {
	objectPostEffect_->EndCaptureToRenderTarget(
		bloom_->GetSceneRTVHandle(),
		WinApp::kClientWidth,
		WinApp::kClientHeight
	);
}

void GameApp::DrawSpriteObjectPost(Sprite* sprite, const Matrix4x4& view, const Matrix4x4& proj, const BloomParam& param)
{
	if (!sprite) {
		return;
	}

	objectPostEffect_->SetParam(param);
	sprite->Update(view, proj);
	BeginObjectPostEffect();
	sprite->Draw();
	EndObjectPostEffect();
	spriteCommon_->SetGraphicsPipelineState();
}

void GameApp::DrawModelParticlesObjectPost(ModelParticleManager* particles, const BloomParam& param)
{
	if (!particles) {
		return;
	}

	objectPostEffect_->SetParam(param);
	BeginObjectPostEffect();
	particles->Draw();
	EndObjectPostEffect();
	objCommon_->SetGraphicsPipelineState();
}

void GameApp::DrawModelParticlesObjectPostToBloomScene(ModelParticleManager* particles, const BloomParam& param, int clipHeight)
{
	if (!particles) {
		return;
	}

	objectPostEffect_->SetParam(param);
	BeginObjectPostEffect();
	if (clipHeight > 0) {
		dx_->SetScissorRect(0, 0, WinApp::kClientWidth, clipHeight);
	}
	particles->Draw();
	objectPostEffect_->EndCaptureToRenderTarget(
		bloom_->GetSceneRTVHandle(),
		WinApp::kClientWidth,
		WinApp::kClientHeight,
		clipHeight
	);
	dx_->SetRenderTarget(bloom_->GetSceneRTVHandle());
	dx_->SetViewport(WinApp::kClientWidth, WinApp::kClientHeight);
	if (clipHeight > 0) {
		dx_->SetScissorRect(0, 0, WinApp::kClientWidth, clipHeight);
	}
	objCommon_->SetGraphicsPipelineState();
}

void GameApp::SetRadialBlur(float strength)
{
	if (bloom_) {
		bloom_->SetRadialBlur(strength);
	}
}

void GameApp::ResetRadialBlur()
{
	if (bloom_) {
		bloom_->ResetRadialBlur();
	}
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

	TextureManager::GetInstance()->LoadTexture("resources/ui/text/resonance_title.png");
	TextureManager::GetInstance()->LoadTexture("resources/ui/text/clickStart.png");

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
	TextureManager::GetInstance()->LoadTexture("resources/ui/stage_select/button_deckEdit.png");
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

	// パーティクルの全エフェクト自動ロード
	ParticleManager::GetInstance()->LoadAllEffects();

	OutputDebugStringA("[Warmup] END\n");
}

void GameApp::LoadActionSequenceProfiles_() {
	actionSequenceProfiles_.clear();
	cardUseSequenceNames_.clear();
	effectSequenceNames_.clear();
	cardSequenceNames_.clear();

	const std::filesystem::path sequenceDir = "resources/sequences";
	if (!std::filesystem::exists(sequenceDir)) {
		OutputDebugStringA("[ActionSequence] resources/sequences not found\n");
		return;
	}

	for (const auto& entry : std::filesystem::directory_iterator(sequenceDir)) {
		if (!entry.is_regular_file() || entry.path().extension() != ".json") {
			continue;
		}
		if (entry.path().filename() == "card_sequence_map.json") {
			continue;
		}
		if (entry.path().stem().string().ends_with("_camera")) {
			continue;
		}

		std::ifstream file(entry.path());
		if (!file.is_open()) {
			continue;
		}

		try {
			nlohmann::json j;
			file >> j;

			ActionSequenceProfile profile;
			profile.FromJson(j);

			const std::string key = entry.path().stem().string();
			actionSequenceProfiles_[key] = profile;

			OutputDebugStringA(("[ActionSequence] loaded: " + key + "\n").c_str());
		} catch (...) {
			OutputDebugStringA(("[ActionSequence] failed: " + entry.path().string() + "\n").c_str());
		}
	}

	const std::filesystem::path mapPath = sequenceDir / "card_sequence_map.json";
	if (!std::filesystem::exists(mapPath)) {
		return;
	}

	std::ifstream mapFile(mapPath);
	if (!mapFile.is_open()) {
		return;
	}

	try {
		nlohmann::json j;
		mapFile >> j;

		if (j.contains("cardUse") && j["cardUse"].is_array()) {
			for (const auto& name : j["cardUse"]) {
				cardUseSequenceNames_.push_back(name.get<std::string>());
			}
		}

		if (j.contains("effects") && j["effects"].is_object()) {
			for (const auto& [effectType, names] : j["effects"].items()) {
				if (!names.is_array()) {
					continue;
				}
				auto& list = effectSequenceNames_[effectType];
				for (const auto& name : names) {
					list.push_back(name.get<std::string>());
				}
			}
		}

		if (j.contains("cards") && j["cards"].is_object()) {
			for (const auto& [cardIdText, names] : j["cards"].items()) {
				if (!names.is_array()) {
					continue;
				}
				auto& list = cardSequenceNames_[std::stoi(cardIdText)];
				for (const auto& name : names) {
					list.push_back(name.get<std::string>());
				}
			}
		}

		OutputDebugStringA("[ActionSequence] loaded card_sequence_map\n");
	} catch (...) {
		OutputDebugStringA("[ActionSequence] failed: card_sequence_map.json\n");
	}
}

const ActionSequenceProfile* GameApp::FindActionSequenceProfile(const std::string& name) const {
	auto it = actionSequenceProfiles_.find(name);
	if (it == actionSequenceProfiles_.end()) {
		return nullptr;
	}

	return &it->second;
}

const ActionSequenceProfile* GameApp::PickSequenceFromNames_(const std::vector<std::string>& names) const {
	std::vector<const ActionSequenceProfile*> available;
	available.reserve(names.size());

	for (const std::string& name : names) {
		if (const ActionSequenceProfile* profile = FindActionSequenceProfile(name)) {
			available.push_back(profile);
		}
	}

	if (available.empty()) {
		return nullptr;
	}

	static std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<size_t> dist(0, available.size() - 1);
	return available[dist(rng)];
}

const ActionSequenceProfile* GameApp::PickCardUseSequenceProfile() const {
	return PickSequenceFromNames_(cardUseSequenceNames_);
}

const ActionSequenceProfile* GameApp::PickCardEffectSequenceProfile(
	int cardId,
	const std::vector<std::string>& effectTypes) const {
	auto cardIt = cardSequenceNames_.find(cardId);
	if (cardIt != cardSequenceNames_.end()) {
		if (const ActionSequenceProfile* profile = PickSequenceFromNames_(cardIt->second)) {
			return profile;
		}
	}

	for (const std::string& effectType : effectTypes) {
		auto effectIt = effectSequenceNames_.find(effectType);
		if (effectIt != effectSequenceNames_.end()) {
			if (const ActionSequenceProfile* profile = PickSequenceFromNames_(effectIt->second)) {
				return profile;
			}
		}
	}

	return nullptr;
}

void GameApp::SetDeckInstancesFromId(const std::vector<int>& ids) {
	deckInstances_.clear();
	for (const auto& id : ids) {
		deckInstances_.push_back(MakeCardInstance(id));
	}
}

void GameApp::SetSelectedStage(int stageId, const std::string& configPath)
{
	selectedStageId_ = stageId;
	selectedStageConfigPath_ = configPath;
}
