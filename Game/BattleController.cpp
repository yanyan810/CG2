#include "BattleController.h"
#include "Battle/BattleFieldViewController.h"
#include "Battle/BattleDebugImGui.h"
#include "Battle/BattleRenderView.h"
#include "Battle/BattleInfoTextProvider.h"
#include "Battle/BattleCardInputController.h"
#include "Battle/BattleControllerShared.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include "GameApp.h"
#include "WinApp.h"
#include <Windows.h>
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "MathStruct.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <set>
#include <random>
#include <filesystem>
#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include"Player.h"
#include"Enemy.h"

#include "Card/CardEffectTextBuilder.h"
#include "Card/CardEffectExecutor.h"
#include "FieldUi.h"
#include "Audio/BattleSfxPlayer.h"
#include "Camera.h"
#include "ModelParticleManager.h"
#include "Poker/PokerChoiceQuery.h"
#include "Poker/PokerChoiceController.h"
#include "Poker/PokerChoiceTextBuilder.h"


//===============================
//===============================

void BattleController::RebuildCostView_(float dt)
{
	costDigitModels_.clear();

	std::string text = std::to_string(energy_) + "/" + std::to_string(energyMax_);

	for (char ch : text) {
		std::string path;

		if (ch >= '0' && ch <= '9') {
			path = "cards/models/";
			path += ch;
			path += ".obj";
		} else if (ch == '/') {
			path = "cards/models/slash.obj";
		} else {
			continue;
		}

		auto obj = std::make_unique<Object3d>();
		obj->Initialize(objCom_, dx_);

		obj->SetModel(path);
		obj->SetCamera(cam_);

		costDigitModels_.push_back(std::move(obj));
	}

	prevEnergy_ = energy_;
	prevEnergyMax_ = energyMax_;

	UpdateCostViewTransform_(dt);
}

std::array<bool, 5> BattleController::GetPokerHighlightMask_() const
{
	std::array<bool, 5> mask{};
	mask.fill(false);

	if (field_.size() != 5) {
		return mask;
	}

	PokerHandResult result = EvaluatePokerHand_();
	if (result.rank == PokerHandRank::None) {
		return mask;
	}

	std::array<int, 14> countNumber{};
	std::array<int, 4> countSuit{};

	for (const auto& c : field_) {
		if (c.number >= 1 && c.number <= 13) {
			countNumber[c.number]++;
		}
		int suitIndex = static_cast<int>(c.suit);
		if (suitIndex >= 0 && suitIndex < 4) {
			countSuit[suitIndex]++;
		}
	}

	auto markNumber = [&](int number) {
		for (int i = 0; i < 5; ++i) {
			if (field_[i].number == number) {
				mask[i] = true;
			}
		}
		};

	auto markSuit = [&](CardSuit suit) {
		for (int i = 0; i < 5; ++i) {
			if (field_[i].suit == suit) {
				mask[i] = true;
			}
		}
		};

	switch (result.rank) {
	case PokerHandRank::OnePair:
		for (int n = 1; n <= 13; ++n) {
			if (countNumber[n] == 2) {
				markNumber(n);
				break;
			}
		}
		break;

	case PokerHandRank::TwoPair:
		for (int n = 1; n <= 13; ++n) {
			if (countNumber[n] == 2) {
				markNumber(n);
			}
		}
		break;

	case PokerHandRank::ThreeOfAKind:
		for (int n = 1; n <= 13; ++n) {
			if (countNumber[n] == 3) {
				markNumber(n);
				break;
			}
		}
		break;

	case PokerHandRank::FourOfAKind:
		for (int n = 1; n <= 13; ++n) {
			if (countNumber[n] == 4) {
				markNumber(n);
				break;
			}
		}
		break;

	case PokerHandRank::FullHouse:
		for (int n = 1; n <= 13; ++n) {
			if (countNumber[n] == 3 || countNumber[n] == 2) {
				markNumber(n);
			}
		}
		break;

	case PokerHandRank::Flush:
	case PokerHandRank::StraightFlush:
	case PokerHandRank::RoyalStraightFlush:
		for (int s = 0; s < 4; ++s) {
			if (countSuit[s] == 5) {
				markSuit(static_cast<CardSuit>(s));
				break;
			}
		}
		break;

	case PokerHandRank::Straight:
		for (int i = 0; i < 5; ++i) {
			mask[i] = true;
		}
		break;

	default:
		break;
	}

	return mask;
}

const CardDef* BattleController::FindCardDef(int id) const
{
	return db_.Find(id);
}

void BattleController::PreloadCardAssets_()
{
	ModelManager::GetInstance()->LoadModel("cards/models/frame.obj");
	ModelManager::GetInstance()->LoadModel("cards/models/art_plane.obj");

	ModelManager::GetInstance()->LoadModel("cards/models/spade.obj");
	ModelManager::GetInstance()->LoadModel("cards/models/heart.obj");
	ModelManager::GetInstance()->LoadModel("cards/models/daiya.obj");
	ModelManager::GetInstance()->LoadModel("cards/models/clover.obj");

	for (int i = 0; i <= 9; ++i) {
		ModelManager::GetInstance()->LoadModel("cards/models/" + std::to_string(i) + ".obj");
	}
	ModelManager::GetInstance()->LoadModel("cards/models/slash.obj");

	for (int id = 1; id <= 100; ++id) {
		const CardDef* def = db_.Find(id);
		if (!def) {
			continue;
		}

		ModelManager::GetInstance()->LoadModel(def->frameModel);
		ModelManager::GetInstance()->LoadModel(def->artModel);
		TextureManager::GetInstance()->LoadTexture(def->artTex);
	}
}

void BattleController::Preload(GameApp& app)
{
	objCom_ = app.ObjCom();
	dx_ = app.Dx();
	spriteCom_ = app.SpriteCom();

	if (!cardDbLoaded_) {
		db_ = *app.GetCardDB();
		cardDbLoaded_ = true;
	}

	if (!assetsPreloaded_) {
		PreloadCardAssets_();

		DeckDef deckDef{};
		std::string err;

		prebuiltDeck_.clear();

		/*if (DeckLoader::LoadFromJson("resources/cards/deck/deck.json", deckDef) &&
			DeckLoader::ValidateDeck(deckDef, db_, err)) {

			for (const auto& e : deckDef.cards) {
				for (int i = 0; i < e.count; ++i) {
					prebuiltDeck_.push_back(MakeCardInstance(e.id));
				}
			}
		} else {
			for (int i = 0; i < 4; ++i) {
				prebuiltDeck_.push_back(MakeCardInstance(9));
				prebuiltDeck_.push_back(MakeCardInstance(8));
				prebuiltDeck_.push_back(MakeCardInstance(7));
				prebuiltDeck_.push_back(MakeCardInstance(6));
				prebuiltDeck_.push_back(MakeCardInstance(5));
				prebuiltDeck_.push_back(MakeCardInstance(4));
				prebuiltDeck_.push_back(MakeCardInstance(3));
				prebuiltDeck_.push_back(MakeCardInstance(2));
				prebuiltDeck_.push_back(MakeCardInstance(1));
				prebuiltDeck_.push_back(MakeCardInstance(20));
				prebuiltDeck_.push_back(MakeCardInstance(19));
				prebuiltDeck_.push_back(MakeCardInstance(18));
				prebuiltDeck_.push_back(MakeCardInstance(17));
				prebuiltDeck_.push_back(MakeCardInstance(16));
				prebuiltDeck_.push_back(MakeCardInstance(15));
				prebuiltDeck_.push_back(MakeCardInstance(14));
				prebuiltDeck_.push_back(MakeCardInstance(13));
				prebuiltDeck_.push_back(MakeCardInstance(12));
				prebuiltDeck_.push_back(MakeCardInstance(11));
				prebuiltDeck_.push_back(MakeCardInstance(10));
			}
		}*/

		for (CardInstance instance : app.GetDeckInstances()) {
			prebuiltDeck_.push_back(instance);
		}

		assetsPreloaded_ = true;
	}
}

void BattleController::Initialize(GameApp& app, Camera* camera)
{
	cam_ = camera;
	objCom_ = app.ObjCom();
	dx_ = app.Dx();
	spriteCom_ = app.SpriteCom();

	Preload(app);
	LoadFieldCardLayout(fieldCardLayoutPath_);

	actionDirector_.Initialize(spriteCom_, dx_, objCom_);

	damagePopupUi_.Initialize(objCom_, dx_, cam_);
	playerStatusUi_.Initialize(app);
	enemyStatusUi_.Initialize(app, 3);
	playerLastHp_ = player_ ? player_->GetHP() : -1;

	deckZone_.SetDeck(prebuiltDeck_);
	ShuffleDeck_();

	if (useTutorialOpeningHand_) {
		for (const auto& fixedCard : tutorialOpeningHand_) {
			deckZone_.RemoveFirstFromDeckByDefId(fixedCard.defId);
		}

		for (auto it = tutorialOpeningHand_.rbegin(); it != tutorialOpeningHand_.rend(); ++it) {
			deckZone_.PushDeckBack(*it);
		}
	}

	deckZone_.ClearHand();
	deckZone_.ClearDiscard();
	field_.clear();
	fieldViews_.clear();
	damagePopupUi_.Clear();

	hasPendingCard_ = false;
	pendingCard_ = {};
	currentEnemyIndex_ = 0;
	nextTurnAtkUp_ = 0;
	currentTurnAtkUp_ = 0;
	playerTurnCount_ = 0;
	enemyTurnCount_ = 0;

	energy_ = energyMax_;

	handView_.Initialize(objCom_, dx_, cam_, &db_);
	handView_.Clear();

	StartPlayerTurn_();
	RebuildDiscardView_();
	RebuildCostView_(deltaTime_);

	highlightFilter_ = std::make_unique<Sprite>();
	highlightFilter_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	highlightFilter_->SetPosition({ 0.0f, 0.0f });
	highlightFilter_->SetScale({ 1280.0f, 1280.0f, 1.0f });
	highlightFilter_->SetColor({ 0.0f, 0.0f, 0.0f, 0.8f });

	propManager_ = std::make_unique<PropManager>();
	propManager_->Initialize(objCom_, dx_, cam_);
	propManager_->LoadFromJson("resources/configs/sceneProps.json");

	tutorialLockPokerTargetingCancel_ = false;

}
