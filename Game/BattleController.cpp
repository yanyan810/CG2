#include "BattleController.h"
#include "GameApp.h"
#include "WinApp.h"
#include <Windows.h>
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include <array>
#include <algorithm>
#include <set>
#include <random>

#include"Player.h"
#include"Enemy.h"

#include "FieldUi.h"

//===============================
//役
//===============================

const char* BattleController::GetPokerHandName_(PokerHandRank rank) const
{
	switch (rank) {
	case PokerHandRank::None:                 return "None";
	case PokerHandRank::OnePair:              return "One Pair";
	case PokerHandRank::TwoPair:              return "Two Pair";
	case PokerHandRank::ThreeOfAKind:         return "Three of a Kind";
	case PokerHandRank::Straight:             return "Straight";
	case PokerHandRank::Flush:                return "Flush";
	case PokerHandRank::FullHouse:            return "Full House";
	case PokerHandRank::FourOfAKind:          return "Four of a Kind";
	case PokerHandRank::StraightFlush:        return "Straight Flush";
	case PokerHandRank::RoyalStraightFlush:   return "Royal Straight Flush";
	default:                                  return "?";
	}
}

BattleController::PokerHandResult BattleController::EvaluatePokerHand_() const
{
	PokerHandResult result{};

	if (field_.size() != 5) {
		result.rank = PokerHandRank::None;
		result.power = 0;
		return result;
	}

	std::array<int, 14> countNumber{}; // 1~13 を使う
	std::array<int, 4> countSuit{};    // Spade/Heart/Diamond/Club

	std::vector<int> numbers;
	numbers.reserve(5);

	for (const auto& c : field_) {
		if (c.number >= 1 && c.number <= 13) {
			countNumber[c.number]++;
			numbers.push_back(c.number);
		}

		int suitIndex = static_cast<int>(c.suit);
		if (suitIndex >= 0 && suitIndex < 4) {
			countSuit[suitIndex]++;
		}
	}

	std::sort(numbers.begin(), numbers.end());

	// ---- Flush 判定 ----
	bool isFlush = false;
	for (int s : countSuit) {
		if (s == 5) {
			isFlush = true;
			break;
		}
	}

	// ---- Straight 判定 ----
	bool isStraight = false;

	// 通常の連番
	{
		bool straight = true;
		for (int i = 0; i < 4; ++i) {
			if (numbers[i] + 1 != numbers[i + 1]) {
				straight = false;
				break;
			}
		}
		if (straight) {
			isStraight = true;
		}
	}

	// A,10,J,Q,K を認める
	if (!isStraight) {
		std::vector<int> royal = numbers;
		std::sort(royal.begin(), royal.end());
		if (royal.size() == 5 &&
			royal[0] == 1 &&
			royal[1] == 10 &&
			royal[2] == 11 &&
			royal[3] == 12 &&
			royal[4] == 13) {
			isStraight = true;
		}
	}

	// A,2,3,4,5 を認める
	if (!isStraight) {
		std::vector<int> lowA = numbers;
		std::sort(lowA.begin(), lowA.end());
		if (lowA.size() == 5 &&
			lowA[0] == 1 &&
			lowA[1] == 2 &&
			lowA[2] == 3 &&
			lowA[3] == 4 &&
			lowA[4] == 5) {
			isStraight = true;
		}
	}

	// ---- 同数枚数を数える ----
	int pairCount = 0;
	bool hasThree = false;
	bool hasFour = false;

	for (int n = 1; n <= 13; ++n) {
		if (countNumber[n] == 2) pairCount++;
		if (countNumber[n] == 3) hasThree = true;
		if (countNumber[n] == 4) hasFour = true;
	}

	// ---- 役判定（強い順）----
	bool isRoyal = (numbers[0] == 1 &&
		numbers[1] == 10 &&
		numbers[2] == 11 &&
		numbers[3] == 12 &&
		numbers[4] == 13);

	if (isStraight && isFlush && isRoyal) {
		result.rank = PokerHandRank::RoyalStraightFlush;
		result.power = 100;
	} else if (isStraight && isFlush) {
		result.rank = PokerHandRank::StraightFlush;
		result.power = 80;
	} else if (hasFour) {
		result.rank = PokerHandRank::FourOfAKind;
		result.power = 70;
	} else if (hasThree && pairCount == 1) {
		result.rank = PokerHandRank::FullHouse;
		result.power = 60;
	} else if (isFlush) {
		result.rank = PokerHandRank::Flush;
		result.power = 50;
	} else if (isStraight) {
		result.rank = PokerHandRank::Straight;
		result.power = 40;
	} else if (hasThree) {
		result.rank = PokerHandRank::ThreeOfAKind;
		result.power = 30;
	} else if (pairCount == 2) {
		result.rank = PokerHandRank::TwoPair;
		result.power = 20;
	} else if (pairCount == 1) {
		result.rank = PokerHandRank::OnePair;
		result.power = 10;
	} else {
		result.rank = PokerHandRank::None;
		result.power = 0;
	}

	return result;
}

void BattleController::RebuildDiscardView_()
{
	discardView_.reset();

	if (discard_.empty()) {
		return;
	}

	const CardInstance& top = discard_.back();
	const CardDef* def = db_.Find(top.defId);
	if (!def) {
		return;
	}

	discardView_ = std::make_unique<Card3D>();
	discardView_->Initialize(objCom_, dx_, cam_, *def, top);

	// 位置は右下寄りのイメージ。あとで調整
	Vector3 pos{ 16.0f, -8.0f, 6.0f };
	Vector3 rot{ 0.0f, 0.0f, 0.0f };
	Vector3 scl{ 1.0f, 1.0f, 1.0f };

	discardView_->SetTransform(pos, rot, scl);
}

void BattleController::ConsumeFieldCards_()
{
	for (auto& view : fieldViews_) {
		if (view) {
			handView_.AddDiscardingCard(std::move(view));
		}
	}
	fieldViews_.clear();

	for (auto& c : field_) {
		discard_.push_back(c);
	}
	field_.clear();
	RebuildDiscardView_();
	fieldLayoutDirty_ = true;
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

	const char* SuitToString(CardSuit suit)
	{
		switch (suit) {
		case CardSuit::Spade:   return "Spade";
		case CardSuit::Heart:   return "Heart";
		case CardSuit::Diamond: return "Diamond";
		case CardSuit::Club:    return "Club";
		default:                return "?";
		}
	}

	bool PointInRect(int mx, int my, float x, float y, float w, float h)
	{
		return mx >= x && mx <= x + w &&
			my >= y && my <= y + h;
	}

	std::wstring Utf8ToWString(const std::string& s) {
		if (s.empty()) {
			return L"";
		}

		int sizeNeeded = MultiByteToWideChar(
			CP_UTF8, 0, s.c_str(), -1, nullptr, 0
		);
		if (sizeNeeded <= 0) {
			return L"";
		}

		std::wstring result(sizeNeeded - 1, L'\0');
		MultiByteToWideChar(
			CP_UTF8, 0, s.c_str(), -1, result.data(), sizeNeeded
		);
		return result;
	}

}

void BattleController::UpdateCostViewTransform_(float dt)
{
	const int count = (int)costDigitModels_.size();
	if (count <= 0) return;

	const float gap = 1.2f;
	const float startX = -gap * 0.5f * (count - 1);

	const float baseX = -14.5f;  // 左へ
	const float baseY = -6.8f;   // 少し上
	const float baseZ = 6.0f;    // まずはそのまま

	for (int i = 0; i < count; ++i) {
		Vector3 pos{ baseX + startX + gap * i, baseY, baseZ };
		Vector3 rot{ 0.0f, 0.0f, 0.0f };
		Vector3 scl{ 0.8f, 0.8f, 0.8f };

		costDigitModels_[i]->SetRotate(rot);
		costDigitModels_[i]->SetTranslate(pos);
		costDigitModels_[i]->SetScale(scl);

		costDigitModels_[i]->Update(dt);
	}
}

void BattleController::ShuffleDeck_()
{
	static std::random_device rd;
	static std::mt19937 mt(rd());
	std::shuffle(deck_.begin(), deck_.end(), mt);
}

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
		// ストレートは5枚全部使う
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
	// 共通で使うモデルを先読み
	ModelManager::GetInstance()->LoadModel("cards/models/frame.obj");
	ModelManager::GetInstance()->LoadModel("cards/models/art_plane.obj");

	// スート
	ModelManager::GetInstance()->LoadModel("cards/models/spade.obj");
	ModelManager::GetInstance()->LoadModel("cards/models/heart.obj");
	ModelManager::GetInstance()->LoadModel("cards/models/daiya.obj");
	ModelManager::GetInstance()->LoadModel("cards/models/clover.obj");

	// コスト数字
	for (int i = 0; i <= 9; ++i) {
		ModelManager::GetInstance()->LoadModel("cards/models/" + std::to_string(i) + ".obj");
	}
	ModelManager::GetInstance()->LoadModel("cards/models/slash.obj");

	// cards.json に載っているカードの画像とモデルを全部先読み
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
		if (!db_.LoadFromJson("resources/cards/cards.json")) {
			db_.BuildSample();
		}
		cardDbLoaded_ = true;
	}

	if (!assetsPreloaded_) {
		PreloadCardAssets_();

		// デッキ定義もここで読んでおくと、ゲーム開始時がさらに軽くなる
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

	// まだ先読みされていなければここで保険として実行
	Preload(app);

	// -----------------------------
	// HPゲージ作成
	// -----------------------------
	playerHpBg_ = std::make_unique<Sprite>();
	playerHpBg_->Initialize(spriteCom_, dx_, "resources/ui/white.png");
	playerHpBg_->SetColor({ 0.2f, 0.2f, 0.2f, 1.0f });
	playerHpBg_->SetScale({ 250.0f, 18.0f, 1.0f });
	playerHpBg_->SetPosition({ 80.0f, 40.0f });

	playerHpFg_ = std::make_unique<Sprite>();
	playerHpFg_->Initialize(spriteCom_, dx_, "resources/ui/white.png");
	playerHpFg_->SetColor({ 0.2f, 0.8f, 0.2f, 1.0f });
	playerHpFg_->SetScale({ 250.0f, 18.0f, 1.0f });
	playerHpFg_->SetPosition({ 80.0f, 40.0f });

	playerHpPredict_ = std::make_unique<Sprite>();
	playerHpPredict_->Initialize(spriteCom_, dx_, "resources/ui/white.png");
	playerHpPredict_->SetColor({ 1.0f, 0.0f, 0.0f, 0.5f });

	playerBlockPredict_ = std::make_unique<Sprite>();
	playerBlockPredict_->Initialize(spriteCom_, dx_, "resources/ui/white.png");
	playerBlockPredict_->SetColor({ 0.0f, 0.0f, 1.0f, 1.f });

	enemyHpBgs_.clear();
	enemyHpFgs_.clear();
	enemyIntentIcons_.clear();

	for (int i = 0; i < 3; ++i) {
		auto bg = std::make_unique<Sprite>();
		bg->Initialize(spriteCom_, dx_, "resources/ui/white.png");
		bg->SetColor({ 0.2f, 0.2f, 0.2f, 1.0f });
		bg->SetScale({ 0.0f, 0.0f, 1.0f });
		bg->SetPosition({ 0.0f, 0.0f });
		enemyHpBgs_.push_back(std::move(bg));

		auto fg = std::make_unique<Sprite>();
		fg->Initialize(spriteCom_, dx_, "resources/ui/white.png");
		fg->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f });
		fg->SetScale({ 0.0f, 0.0f, 1.0f });
		fg->SetPosition({ 0.0f, 0.0f });
		enemyHpFgs_.push_back(std::move(fg));

		auto icon = std::make_unique<Sprite>();
		icon->Initialize(spriteCom_, dx_, "resources/ui/white.png");
		icon->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
		icon->SetScale({ 0.0f, 0.0f, 1.0f });
		icon->SetPosition({ 0.0f, 0.0f });
		enemyIntentIcons_.push_back(std::move(icon));
	}

	Matrix4x4 viewMat = Matrix4x4::MakeIdentity4x4();
	float width = (float)WinApp::kClientWidth;
	float height = (float)WinApp::kClientHeight;
	Matrix4x4 projMat = Matrix4x4::MakeOrthographicMatrix(width, height);

	if (playerHpBg_) playerHpBg_->Update(viewMat, projMat);
	if (playerHpFg_) playerHpFg_->Update(viewMat, projMat);
	if (playerHpPredict_)playerHpPredict_->Update(viewMat, projMat);
	if (playerBlockPredict_)playerBlockPredict_->Update(viewMat, projMat);

	for (auto& bg : enemyHpBgs_) { if (bg) bg->Update(viewMat, projMat); }
	for (auto& fg : enemyHpFgs_) { if (fg) fg->Update(viewMat, projMat); }
	for (auto& icon : enemyIntentIcons_) { if (icon) icon->Update(viewMat, projMat); }

	// ここはコピーだけ
	deck_ = prebuiltDeck_;
	ShuffleDeck_();

	if (useTutorialOpeningHand_) {
		// 最初に引く5枚を deck の末尾に積むため、
		// 先に同じカードがあれば軽く取り除く
		for (const auto& fixedCard : tutorialOpeningHand_) {
			auto it = std::find_if(deck_.begin(), deck_.end(),
				[&](const CardInstance& c) {
					return c.defId == fixedCard.defId;
				});
			if (it != deck_.end()) {
				deck_.erase(it);
			}
		}

		// DrawOne_ は back() を引くので、逆順で積む
		for (auto it = tutorialOpeningHand_.rbegin(); it != tutorialOpeningHand_.rend(); ++it) {
			deck_.push_back(*it);
		}
	}

	hand_.clear();
	discard_.clear();
	field_.clear();
	fieldViews_.clear();
	damagePopups_.clear();

	hasPendingCard_ = false;
	pendingCard_ = {};
	currentEnemyIndex_ = 0;
	nextTurnAtkUp_ = 0;
	currentTurnAtkUp_ = 0;

	energy_ = energyMax_;

	handView_.Initialize(objCom_, dx_, cam_, &db_);
	handView_.Clear();

	StartPlayerTurn_();
	RebuildDiscardView_();
	RebuildCostView_(deltaTime_);

	// ハイライト用Filter
	highlightFilter_ = std::make_unique<Sprite>();
	highlightFilter_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	highlightFilter_->SetPosition({ 0.0f, 0.0f });
	highlightFilter_->SetScale({ 1280.0f, 1280.0f, 1.0f });
	highlightFilter_->SetColor({ 0.0f, 0.0f, 0.0f, 0.8f });
}

bool BattleController::DrawOne_()
{
	if (deck_.empty()) {
		if (discard_.empty()) return false;
		deck_ = discard_;
		discard_.clear();
		ShuffleDeck_();
		RebuildDiscardView_();
	}

	if (deck_.empty()) return false;

	CardInstance card = deck_.back();
	deck_.pop_back();
	hand_.push_back(card);

	// ここを追加
	handView_.AddCard(card);

	return true;
}

void BattleController::DrawUntilFive_()
{
	while ((int)hand_.size() < 5) {
		if (!DrawOne_()) break;
	}
	//handView_.Rebuild(hand_);
}

void BattleController::DrawCards_(int count)
{
	for (int i = 0; i < count; ++i) {
		if (!DrawOne_()) {
			break;
		}
	}
	//	handView_.Rebuild(hand_);
}

void BattleController::ApplyEffectsList_(const std::vector<CardEffectDef>& effects, int targetIndex, bool applyAttackBuff)
{
	for (const auto& effect : effects) {
		if (effect.type == "Draw") {
			DrawCards_(effect.value);

		} else if (effect.type == "Damage") {
			if (enemyMgr_) {
				if (targetIndex >= 0 && targetIndex < enemyMgr_->GetEnemies().size()) {
					auto& e = enemyMgr_->GetEnemies()[targetIndex];
					if (e.IsAlive()) {
						int totalDamage = applyAttackBuff ? CalcFinalAttackDamage_(effect.value) : std::max(0, effect.value);
						ApplyDamageToEnemy_(e, totalDamage);
						if (totalDamage > 0) SpawnDamagePopup(e.GetPos(), totalDamage, false);
					}
				} else
				{
					for (auto& e : enemyMgr_->GetEnemies()) {
						if (!e.IsAlive()) continue;
						int totalDamage = applyAttackBuff ? CalcFinalAttackDamage_(effect.value) : std::max(0, effect.value);
						ApplyDamageToEnemy_(e, totalDamage);
						if (totalDamage > 0) SpawnDamagePopup(e.GetPos(), totalDamage, false);
						break;
					}
				}

			/*	if (applyAttackBuff) {
					nextTurnAtkUp_ = 0;
				}*/
			}

		} else if (effect.type == "DamageCrescent") {
			if (enemyMgr_) {
				int baseVal = effect.value;
				if (playerTurnCount_ % 2 != 0) {
					baseVal += 3;
				}

				int finalDamage = applyAttackBuff ? CalcFinalAttackDamage_(baseVal) : std::max(0, baseVal);

				if (targetIndex >= 0 && targetIndex < enemyMgr_->GetEnemies().size()) {
					auto& e = enemyMgr_->GetEnemies()[targetIndex];
					if (e.IsAlive()) {
						ApplyDamageToEnemy_(e, finalDamage);
						if (finalDamage > 0) SpawnDamagePopup(e.GetPos(), finalDamage, false);
					}
				} else {
					for (auto& e : enemyMgr_->GetEnemies()) {
						if (!e.IsAlive()) continue;
						ApplyDamageToEnemy_(e, finalDamage);
						if (finalDamage > 0) SpawnDamagePopup(e.GetPos(), finalDamage, false);
						break;
					}
				}

			/*	if (applyAttackBuff) {
					nextTurnAtkUp_ = 0;
				}*/
			}

		} else if (effect.type == "DamageByBlock") {
			if (enemyMgr_) {
				int baseVal = (player_ ? player_->GetBlock() : 0) * effect.value;
				int finalDamage = applyAttackBuff ? CalcFinalAttackDamage_(baseVal) : std::max(0, baseVal);

				if (targetIndex >= 0 && targetIndex < enemyMgr_->GetEnemies().size()) {
					auto& e = enemyMgr_->GetEnemies()[targetIndex];
					if (e.IsAlive()) {
						ApplyDamageToEnemy_(e, finalDamage);
						if (finalDamage > 0) SpawnDamagePopup(e.GetPos(), finalDamage, false);
					}
				} else {
					for (auto& e : enemyMgr_->GetEnemies()) {
						if (!e.IsAlive()) continue;
						ApplyDamageToEnemy_(e, finalDamage);
						if (finalDamage > 0) SpawnDamagePopup(e.GetPos(), finalDamage, false);
						break;
					}
				}

			/*	if (applyAttackBuff) {
					nextTurnAtkUp_ = 0;
				}*/
			}

		} else if (effect.type == "Block") {
			if (player_) {
				player_->AddBlock(effect.value);
			}

		} else if (effect.type == "DamageAll") {
			if (enemyMgr_ && player_) {
				int totalDamage = applyAttackBuff ? CalcFinalAttackDamage_(effect.value) : std::max(0, effect.value);

				player_->PlayAttackAnim(player_->GetPos());

				int hitCount = 0;
				for (auto& e : enemyMgr_->GetEnemies()) {
					if (e.IsAlive()) {
						e.TriggerHitFlash(0.2f);
						e.PlayDamageAnim();
						e.Damage(totalDamage);
						if (totalDamage > 0) {
							SpawnDamagePopup(e.GetPos(), totalDamage, false);
						}
						hitCount++;
					}
				}

				if (hitCount > 0 && player_->GetVampireHeal() > 0) {
					player_->Heal(player_->GetVampireHeal() * hitCount);
				}

				//if (applyAttackBuff) {
				//	nextTurnAtkUp_ = 0;
				//}
			}

		} else if (effect.type == "PowerBoost") {
			if (player_) {
				player_->PowerBoost(effect.value);
			}
		} else if (effect.type == "NextTurnAtkUp") {
			nextTurnAtkUp_ += effect.value;

		} else if (effect.type == "Heal") {
			if (player_) {
				player_->Heal(effect.value);
			}

		} else if (effect.type == "HealByBlock") {
			if (player_) {
				int healAmount = player_->GetBlock() * effect.value; // ブロック数 × 倍率
				player_->Heal(healAmount);
			}
		} else if (effect.type == "HealByLowCostInHand") {
			if (player_) {
				int count = 0;
				// 今の手札を1枚ずつ確認する
				for (const auto& cardInst : hand_) {
					const CardDef* cDef = db_.Find(cardInst.defId);
					if (cDef && cDef->cost == 1) {
						count++; // 1コストのカードだったらカウントを増やす
					}
				}
				int healAmount = count * effect.value;
				if (healAmount > 0) {
					player_->Heal(healAmount);
				}
			}
		} else if (effect.type == "VampireBuff") {
			if (player_) {
				player_->AddVampireHeal(effect.value);
			}
		} else if (effect.type == "SelfDamage") {
			if (player_) {
				player_->TriggerHitFlash(0.2f);
				player_->PlayDamageAnim();
				player_->Damage(effect.value);
			}

		} else if (effect.type == "ChangeNumber") {
			// 後で対象指定が必要

		} else if (effect.type == "ChangeSuit") {
			// 後で対象指定が必要
		}
	}
}

void BattleController::ApplyCardEffects_(const CardDef& def, int targetIndex)
{
	ApplyEffectsList_(def.effects, targetIndex, true);
}

BattleController::PokerHandRank BattleController::ParsePokerRankString_(const std::string& s) const
{
	if (s == "OnePair") return PokerHandRank::OnePair;
	if (s == "TwoPair") return PokerHandRank::TwoPair;
	if (s == "ThreeOfAKind") return PokerHandRank::ThreeOfAKind;
	if (s == "Straight") return PokerHandRank::Straight;
	if (s == "Flush") return PokerHandRank::Flush;
	if (s == "FullHouse") return PokerHandRank::FullHouse;
	if (s == "FourOfAKind") return PokerHandRank::FourOfAKind;
	if (s == "StraightFlush") return PokerHandRank::StraightFlush;
	if (s == "RoyalStraightFlush") return PokerHandRank::RoyalStraightFlush;
	return PokerHandRank::None;
}

bool BattleController::IsRankAtLeast_(PokerHandRank a, PokerHandRank b) const
{
	return static_cast<int>(a) >= static_cast<int>(b);
}

bool BattleController::IsRankInFamily_(PokerHandRank rank, const std::string& family) const
{
	if (family == "StraightFamily") {
		return rank == PokerHandRank::Straight ||
			rank == PokerHandRank::StraightFlush ||
			rank == PokerHandRank::RoyalStraightFlush;
	}

	if (family == "FlushFamily") {
		return rank == PokerHandRank::Flush ||
			rank == PokerHandRank::StraightFlush ||
			rank == PokerHandRank::RoyalStraightFlush;
	}

	if (family == "PairFamily") {
		return rank == PokerHandRank::OnePair ||
			rank == PokerHandRank::TwoPair ||
			rank == PokerHandRank::ThreeOfAKind ||
			rank == PokerHandRank::FullHouse ||
			rank == PokerHandRank::FourOfAKind;
	}

	return false;
}

bool BattleController::DoesSubEffectConditionMatch_(const CardSubEffectDef& sub, PokerHandRank rank) const
{
	switch (sub.condition.type) {
	case SubEffectConditionType::ExactRank:
		return rank == ParsePokerRankString_(sub.condition.rank);

	case SubEffectConditionType::AtLeastRank:
		return IsRankAtLeast_(rank, ParsePokerRankString_(sub.condition.rank));

	case SubEffectConditionType::RankFamily:
		return IsRankInFamily_(rank, sub.condition.family);

	default:
		return false;
	}
}


void BattleController::StartPlayerTurn_()
{
	currentTurnAtkUp_ = nextTurnAtkUp_;
	nextTurnAtkUp_ = 0;

	if (player_) {
		player_->ResetBlock();
		player_->ResetVampireHeal();
		player_->ResetPowerBoost();
	}
	playerTurnCount_++;

	energy_ = energyMax_;
	DrawUntilFive_();

	if (field_.size() == 5) {
		currentPoker_ = EvaluatePokerHand_();

		if (currentPoker_.rank != PokerHandRank::None) {
			TriggerSubEffectsForField_(
				SubEffectTrigger::OnTurnStartWithPoker,
				currentPoker_.rank
			);

			lastPokerTutorialResult_ = PokerTutorialResult::None;
			pokerChoiceState_ = PokerChoiceState::WaitingActivateChoice;
			pokerChoiceJustOpened_ = true;
		}
	}
	if (enemyMgr_) {
		for (auto& e : enemyMgr_->GetEnemies()) {
			if (e.IsAlive()) {
				e.GetBossAI().DecideNextAction();
			}
		}
	}
}

BattleController::PokerBonus BattleController::GetPokerBonus_(PokerHandRank rank) const
{
	PokerBonus b{};

	switch (rank) {
	case PokerHandRank::OnePair:
		b.atkUp = 10;
		b.drawCount = 2;
		b.damage = 15;
		break;

	case PokerHandRank::TwoPair:
		b.atkUp = 15;
		b.drawCount = 3;
		b.damage = 25;
		break;

	case PokerHandRank::ThreeOfAKind:
		b.atkUp = 20;
		b.drawCount = 3;
		b.damage = 35;
		break;

	case PokerHandRank::Straight:
		b.atkUp = 25;
		b.drawCount = 4;
		b.damage = 45;
		break;

	case PokerHandRank::Flush:
		b.atkUp = 30;
		b.drawCount = 4;
		b.damage = 55;
		break;

	case PokerHandRank::FullHouse:
		b.atkUp = 35;
		b.drawCount = 5;
		b.damage = 70;
		break;

	case PokerHandRank::FourOfAKind:
		b.atkUp = 40;
		b.drawCount = 5;
		b.damage = 85;
		break;

	case PokerHandRank::StraightFlush:
		b.atkUp = 50;
		b.drawCount = 6;
		b.damage = 110;
		break;

	case PokerHandRank::RoyalStraightFlush:
		b.atkUp = 70;
		b.drawCount = 7;
		b.damage = 150;
		break;

	default:
		break;
	}

	return b;
}

void BattleController::SetPokerQuickPreviewVisible(bool visible)
{
	pokerQuickPreviewVisible_ = visible;
}

std::wstring BattleController::GetSubEffectTriggerText_(SubEffectTrigger trigger) const
{
	switch (trigger) {
	case SubEffectTrigger::OnTurnStartWithPoker:
		return L"ターン開始時";

	case SubEffectTrigger::OnPokerSkillActivated:
		return L"特殊効果発動時";

	case SubEffectTrigger::OnPlayToField:
		return L"場に出した時";

	default:
		return L"";
	}
}

std::wstring BattleController::GetSubEffectConditionText_(const CardSubEffectDef& sub) const
{
	auto rankToText = [](PokerHandRank rank) -> std::wstring {
		switch (rank) {
		case PokerHandRank::OnePair:            return L"ワンペア";
		case PokerHandRank::TwoPair:            return L"ツーペア";
		case PokerHandRank::ThreeOfAKind:       return L"スリーカード";
		case PokerHandRank::Straight:           return L"ストレート";
		case PokerHandRank::Flush:              return L"フラッシュ";
		case PokerHandRank::FullHouse:          return L"フルハウス";
		case PokerHandRank::FourOfAKind:        return L"フォーカード";
		case PokerHandRank::StraightFlush:      return L"ストレートフラッシュ";
		case PokerHandRank::RoyalStraightFlush: return L"ロイヤルストレートフラッシュ";
		default:                                return L"";
		}
		};

	switch (sub.condition.type) {
	case SubEffectConditionType::ExactRank:
	{
		PokerHandRank rank = ParsePokerRankString_(sub.condition.rank);
		if (rank != PokerHandRank::None) {
			return rankToText(rank) + L"の場合";
		}
		break;
	}

	case SubEffectConditionType::AtLeastRank:
	{
		PokerHandRank rank = ParsePokerRankString_(sub.condition.rank);
		if (rank != PokerHandRank::None) {
			return rankToText(rank) + L"以上の場合";
		}
		break;
	}

	case SubEffectConditionType::RankFamily:
		if (sub.condition.family == "StraightFamily") return L"ストレート系の場合";
		if (sub.condition.family == "FlushFamily")    return L"フラッシュ系の場合";
		if (sub.condition.family == "PairFamily")     return L"ペア系の場合";
		return L"役条件あり";

	default:
		break;
	}

	return L"";
}

std::wstring BattleController::GetEffectValueText_(const CardEffectDef& effect) const
{
	if (!effect.valueText.empty()) {
		return Utf8ToWString(effect.valueText) + L": " + std::to_wstring(effect.value);
	}

	if (effect.type == "Draw") {
		return L"ドロー: " + std::to_wstring(effect.value);
	}
	if (effect.type == "Damage") {
		return L"ダメージ: " + std::to_wstring(effect.value);
	}
	if (effect.type == "DamageAll") {
		return L"全体ダメージ: " + std::to_wstring(effect.value);
	}
	if (effect.type == "Heal") {
		return L"回復: " + std::to_wstring(effect.value);
	}
	if (effect.type == "Block") {
		return L"ブロック: " + std::to_wstring(effect.value);
	}
	if (effect.type == "PowerBoost") {
		return L"パワー: " + std::to_wstring(effect.value);
	}
	if (effect.type == "EnergyCharge") {
		return L"コスト回復: " + std::to_wstring(effect.value);
	}
	if (effect.type == "NextTurnAtkUp") {
		return L"次ターンATK UP: " + std::to_wstring(effect.value);
	}
	if (effect.type == "SelfDamage") {
		return L"自傷: " + std::to_wstring(effect.value);
	}

	return Utf8ToWString(effect.type) + L": " + std::to_wstring(effect.value);
}

std::wstring BattleController::GetBaseEffectSummaryText_(const CardDef& def) const
{
	if (!def.effects.empty()) {
		std::wstring text;

		for (size_t i = 0; i < def.effects.size(); ++i) {
			if (i > 0) {
				text += L"\n";
			}
			text += GetEffectValueText_(def.effects[i]);
		}

		return text;
	}

	if (!def.desc.empty()) {
		return Utf8ToWString(def.desc);
	}

	return L"なし";
}

std::wstring BattleController::GetPreviewCardDetailText() const
{
	const CardDef* def = GetPreviewCardDef();
	if (!def) {
		return L"";
	}

	std::wstring text;
	text += L"基本効果:\n\n";
	text += GetBaseEffectSummaryText_(*def);

	if (!def->subEffects.empty()) {
		const CardSubEffectDef& sub = def->subEffects[0];

		text += L"\n\n------------------------\n\n";

		const std::wstring triggerText = GetSubEffectTriggerText_(sub.trigger);
		if (!triggerText.empty()) {
			text += triggerText + L"\n\n";
		}

		const std::wstring condText = GetSubEffectConditionText_(sub);
		if (!condText.empty()) {
			text += condText + L"\n\n";
		}

		if (!sub.effects.empty()) {
			for (size_t i = 0; i < sub.effects.size(); ++i) {
				if (i > 0) {
					text += L"\n";
				}
				text += GetEffectValueText_(sub.effects[i]);
			}
		} else {
			text += L"効果なし";
		}
	}

	return text;
}

std::vector<std::wstring> BattleController::CollectSubEffectPreviewLines_(
	SubEffectTrigger trigger,
	PokerHandRank rank
) const
{
	std::vector<std::wstring> lines;
	std::set<std::wstring> uniqueLines;

	for (const auto& card : field_) {
		const CardDef* def = db_.Find(card.defId);
		if (!def) continue;

		for (const auto& sub : def->subEffects) {
			if (sub.trigger != trigger) continue;
			if (!DoesSubEffectConditionMatch_(sub, rank)) continue;

			for (const auto& effect : sub.effects) {
				std::wstring line = L"・";

				if (!def->name.empty()) {
					int size = MultiByteToWideChar(CP_UTF8, 0, def->name.c_str(), -1, nullptr, 0);
					std::wstring cardName(size - 1, L'\0');
					MultiByteToWideChar(CP_UTF8, 0, def->name.c_str(), -1, cardName.data(), size);
					line += cardName + L" : ";
				}

				if (effect.type == "Draw") {
					line += L"カードを" + std::to_wstring(effect.value) + L"枚引く";
				} else if (effect.type == "Damage") {
					line += L"敵単体に" + std::to_wstring(effect.value) + L"ダメージ";
				} else if (effect.type == "DamageAll") {
					line += L"敵全体に" + std::to_wstring(effect.value) + L"ダメージ";
				} else if (effect.type == "Heal") {
					line += L"体力を" + std::to_wstring(effect.value) + L"回復";
				} else if (effect.type == "Block") {
					line += L"ブロックを" + std::to_wstring(effect.value) + L"獲得";
				} else if (effect.type == "PowerBoost") {
					line += L"パワーを" + std::to_wstring(effect.value) + L"獲得";
				} else if (effect.type == "EnergyCharge") {
					line += L"コストを" + std::to_wstring(effect.value) + L"回復";
				} else {
					line += Utf8ToWString(effect.type) + L" : " + std::to_wstring(effect.value);
				}

				if (uniqueLines.insert(line).second) {
					lines.push_back(line);
				}
			}
		}
	}

	return lines;
}

std::wstring BattleController::GetPokerEffectPreviewText() const
{
	std::wstring text;

	PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);

	text += L"選択効果:\n";
	text += L"・このあと1つ選びます\n";
	text += L"  1. 次ターンATK UP +" + std::to_wstring(bonus.atkUp) + L"\n";
	text += L"  2. " + std::to_wstring(bonus.drawCount) + L"枚ドロー\n";
	text += L"  3. 敵単体に" + std::to_wstring(bonus.damage) + L"ダメージ\n";
	text += L"\n";

	auto turnStartLines = CollectSubEffectPreviewLines_(
		SubEffectTrigger::OnTurnStartWithPoker,
		currentPoker_.rank
	);

	text += L"ターン開始時:\n";
	if (turnStartLines.empty()) {
		text += L"・なし\n";
	} else {
		for (const auto& line : turnStartLines) {
			text += line + L"\n";
		}
	}
	text += L"\n";

	auto pokerActivatedLines = CollectSubEffectPreviewLines_(
		SubEffectTrigger::OnPokerSkillActivated,
		currentPoker_.rank
	);

	text += L"特殊効果発動時:\n";
	if (pokerActivatedLines.empty()) {
		text += L"・なし\n";
	} else {
		for (const auto& line : pokerActivatedLines) {
			text += line + L"\n";
		}
	}

	return text;
}

void BattleController::TriggerSubEffectsForField_(SubEffectTrigger trigger, PokerHandRank rank)
{
	if (field_.size() != 5) return;
	if (rank == PokerHandRank::None) return;

	for (const auto& card : field_) {
		const CardDef* def = db_.Find(card.defId);
		if (!def) continue;

		for (const auto& sub : def->subEffects) {
			if (sub.trigger != trigger) continue;
			if (!DoesSubEffectConditionMatch_(sub, rank)) continue;

			ApplyEffectsList_(sub.effects, -1, false);
		}
	}
}

void BattleController::TriggerSubEffectsForCard_(
	const CardInstance& card,
	SubEffectTrigger trigger,
	PokerHandRank rank)
{
	const CardDef* def = db_.Find(card.defId);
	if (!def) return;
	if (rank == PokerHandRank::None) return;

	for (const auto& sub : def->subEffects) {
		if (sub.trigger != trigger) continue;
		if (!DoesSubEffectConditionMatch_(sub, rank)) continue;

		ApplyEffectsList_(sub.effects, -1, false);
	}
}

void BattleController::RebuildFieldView_()
{
	const int n = (int)field_.size();
	if (n <= 0) {
		fieldViews_.clear();
		return;
	}

	while (fieldViews_.size() < field_.size()) {
		int i = (int)fieldViews_.size();
		const CardDef* def = db_.Find(field_[i].defId);

		auto card = std::make_unique<Card3D>();
		if (def) card->Initialize(objCom_, dx_, cam_, *def, field_[i]);
		card->SetIsHand(false);
		card->SetTransform({ 0.0f, -10.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.15f, 1.15f, 1.15f }); // 見えない場所から
		fieldViews_.push_back(std::move(card));
	}

	std::array<bool, 5> highlightMask = GetPokerHighlightMask_();

	const float y = -5.0f;
	const float z = 5.0f;
	const float gap = 5.0f;
	const float startX = -gap * 0.5f * (n - 1);

	for (int i = 0; i < n; ++i) {
		if (!fieldViews_[i]) continue;

		Vector3 pos{ startX + gap * i, y, z };
		Vector3 rot{ 0.0f, 0.0f, 0.0f };
		Vector3 scl{ 1.15f, 1.15f, 1.15f };

		fieldViews_[i]->SetTargetTransform(pos, rot, scl, false);

		if (i < 5 && highlightMask[i]) {
			fieldViews_[i]->SetFrameColor({ 1.0f, 0.85f, 0.2f, 1.0f });
		} else {
			fieldViews_[i]->ResetFrameColor();
		}
	}

	// 1. 今の役を評価
	PokerHandResult result = EvaluatePokerHand_();

	// 2. 役の強さに応じてキラキラの強さを決める
	float intensity = 0.0f;
	if (result.rank == PokerHandRank::None) {
		intensity = 10.0f;
	} else if (result.rank <= PokerHandRank::TwoPair) {
		intensity = 10.3f;  // 弱い役：うっすら
	} else if (result.rank <= PokerHandRank::FullHouse) {
		intensity = 10.8f;  // 中堅の役：はっきり
	} else {
		intensity = 10.0f;  // 強い役：まばゆい！
	}

	// 3. 役に関係しているカードだけをハイライト（既存のマスクを利用）
	std::array<bool, 5> mask = GetPokerHighlightMask_();

	for (int i = 0; i < (int)fieldViews_.size(); ++i) {
		if (i < 5 && mask[i]) {
			fieldViews_[i]->SetGlitter(intensity);

			// ★応用：強い役の時は枠の色も豪華にする
			if (result.rank >= PokerHandRank::Straight) {
				fieldViews_[i]->SetFrameColor({ 1.0f, 0.9f, 0.2f, 1.0f }); // 金色
			}
		} else {
			fieldViews_[i]->SetGlitter(0.0f);
			fieldViews_[i]->ResetFrameColor();
		}
	}

	fieldLayoutDirty_ = true;
}

void BattleController::UpdateFieldCardTransform_(int index, bool hovered, float dt)
{
	if (index < 0 || index >= (int)fieldViews_.size()) {
		return;
	}

	const int fieldCount = (int)fieldViews_.size();
	if (fieldCount <= 0) {
		return;
	}

	const float y = -5.0f;
	const float z = 5.0f;
	const float gap = 5.0f;
	const float startX = -gap * 0.5f * (fieldCount - 1);

	Vector3 pos{ startX + gap * index, y, z };
	Vector3 rot{ 0.0f, 0.0f, 0.0f };
	Vector3 scl{ 1.15f, 1.15f, 1.15f };

	if (hovered) {

		pos.y += 0.18f;
		pos.z -= 0.08f;
		scl = { 1.18f, 1.18f, 1.18f };

	}

	fieldViews_[index]->SetTargetTransform(pos, rot, scl, false);
	fieldViews_[index]->Update(dt);

	Vector3 curPos = fieldViews_[index]->GetWorldPos();
	float distSq = (curPos.x - pos.x) * (curPos.x - pos.x) +
		(curPos.y - pos.y) * (curPos.y - pos.y) +
		(curPos.z - pos.z) * (curPos.z - pos.z);
	if (distSq > 0.00001f) {
		fieldLayoutDirty_ = true;
	}
}

void BattleController::RefreshAllFieldCardTransforms_(float dt)
{
	for (int i = 0; i < (int)fieldViews_.size(); ++i) {
		const bool hovered =
			(cardState_ == CardInputState::ChoosingFieldReplace && i == fieldReplaceHoverIndex_) ||
			(pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi && i == fieldReplaceHoverIndex_);

		UpdateFieldCardTransform_(i, hovered, dt);
	}
}

int BattleController::PickFieldIndexByMouse_(int mouseX, int mouseY) const
{
	const Matrix4x4& vp = cam_->GetViewProjectionMatrix();
	const float sw = (float)WinApp::kClientWidth;
	const float sh = (float)WinApp::kClientHeight;

	int best = -1;
	float bestD2 = 80.0f * 80.0f;

	for (int i = 0; i < (int)fieldViews_.size(); ++i) {
		Vector3 w = fieldViews_[i]->GetWorldPos();

		Vector4 clip{};
		clip.x = w.x * vp.m[0][0] + w.y * vp.m[1][0] + w.z * vp.m[2][0] + 1.0f * vp.m[3][0];
		clip.y = w.x * vp.m[0][1] + w.y * vp.m[1][1] + w.z * vp.m[2][1] + 1.0f * vp.m[3][1];
		clip.z = w.x * vp.m[0][2] + w.y * vp.m[1][2] + w.z * vp.m[2][2] + 1.0f * vp.m[3][2];
		clip.w = w.x * vp.m[0][3] + w.y * vp.m[1][3] + w.z * vp.m[2][3] + 1.0f * vp.m[3][3];

		if (clip.w <= 0.0f) {
			continue;
		}

		const float ndcX = clip.x / clip.w;
		const float ndcY = clip.y / clip.w;

		const float sx = (ndcX * 0.5f + 0.5f) * sw;
		const float sy = (-ndcY * 0.5f + 0.5f) * sh;

		const float dx = sx - (float)mouseX;
		const float dy = sy - (float)mouseY;
		const float d2 = dx * dx + dy * dy;

		if (d2 < bestD2) {
			bestD2 = d2;
			best = i;
		}
	}

	return best;
}

void BattleController::Update(GameApp& app, FieldUi& fieldUi, float dt)
{

	Input* input = app.GetInput();
	if (!input) {
		return;
	}

	bool yTrig = input->IsKeyTrigger(DIK_Y);
	bool nTrig = input->IsKeyTrigger(DIK_N);
	/*bool key1Trig = input->IsKeyTrigger(DIK_1);
	bool key2Trig = input->IsKeyTrigger(DIK_2);
	bool key3Trig = input->IsKeyTrigger(DIK_3);*/

	POINT mouse = input->GetMousePosition();

	bool lNow = input->IsMousePressed(0);
	bool lTrig = input->IsMouseTrigger(0);
	bool lRel = input->IsMouseReleased(0);

	bool rTrig = input->IsMouseTrigger(1);

	pokerMouseChoice_ = PokerMouseChoice::None;

	// -----------------------------
	// ポーカー発動する/しない 選択
	// -----------------------------
	if (pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice)
	{
		HandlePokerActivateChoice_(fieldUi, mouse, lTrig, yTrig, nTrig);
		return;
	}

	// -----------------------------
	// ポーカー効果選択
	// -----------------------------
	if (pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice)
	{
		HandlePokerEffectChoice_(fieldUi, mouse, lTrig, nTrig);
		return;
	}

	if (pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi)
	{
		HandlePokerViewBoard_(fieldUi, mouse, lTrig, dt);
		return;
	}

	bool enterTrig = input->IsKeyTrigger(DIK_RETURN);
	operationUiVisible_ = input->IsKeyPressed(DIK_TAB);

	bool endTurnButtonClicked = false;
	endTurnButtonHovered_ = false;

	if (turn_ == TurnState::Player &&
		cardState_ == CardInputState::Idle &&
		pokerChoiceState_ == PokerChoiceState::None) {

		const auto& ui = fieldUi.GetFieldUiLayout();

		endTurnButtonHovered_ = PointInRect(
			mouse.x, mouse.y,
			ui.endTurnBg.x, ui.endTurnBg.y,
			ui.endTurnBg.w, ui.endTurnBg.h
		);

		if (endTurnButtonHovered_ && lTrig) {
			endTurnButtonClicked = true;
		}
	}

	if (turn_ == TurnState::Player) {

		if (cardState_ != CardInputState::ChoosingFieldReplace) {
			fieldReplaceHoverIndex_ = -1;
			prevFieldReplaceHoverIndex_ = -1;
		}

		if (cardState_ == CardInputState::ChoosingFieldReplace ||
			cardState_ == CardInputState::Preview) {
			handView_.SetHoverIndex(-1);
		} else {
			int hover = handView_.PickIndexByMouse(
				mouse.x, mouse.y,
				cam_->GetViewProjectionMatrix(),
				(float)WinApp::kClientWidth, (float)WinApp::kClientHeight
			);
			handView_.SetHoverIndex(hover);
		}
	} else {
		handView_.SetHoverIndex(-1);
	}

	if (turn_ == TurnState::Player) {

		if (cardState_ != CardInputState::ChoosingFieldReplace) {
			fieldReplaceHoverIndex_ = -1;
			prevFieldReplaceHoverIndex_ = -1;
		}

		if ((/*enterTrig ||*/ endTurnButtonClicked) && cardState_ == CardInputState::Idle) {

			OutputDebugStringA(("Before EndTurn hand=" + std::to_string(hand_.size()) +
				" deck=" + std::to_string(deck_.size()) +
				" discard=" + std::to_string(discard_.size()) +
				" field=" + std::to_string(field_.size()) + "\n").c_str());

			turn_ = TurnState::Enemy;
			enemyTurnCount_++;
			hasPendingCard_ = false;
			pendingCard_ = {};
			enemyWait_ = 1.0f;
			handView_.SetHoverIndex(-1);
			handView_.SetDrag(-1, 0, 0, false);
			handView_.SetPreviewIndex(-1);
			selectedIndex_ = -1;
			cardState_ = CardInputState::Idle;
			fieldReplaceHoverIndex_ = -1;
			prevFieldReplaceHoverIndex_ = -1;
		} else {
			if (cardState_ != CardInputState::Preview &&
				cardState_ != CardInputState::ChoosingFieldReplace) {
				int hover = handView_.PickIndexByMouse(
					mouse.x, mouse.y,
					cam_->GetViewProjectionMatrix(),
					(float)WinApp::kClientWidth, (float)WinApp::kClientHeight
				);
				handView_.SetHoverIndex(hover);
			} else {
				handView_.SetHoverIndex(-1);
			}

			switch (cardState_) {
			case CardInputState::Idle:
				handView_.SetDrag(-1, 0, 0, false);
				handView_.SetPreviewIndex(-1);

				if (lTrig) {
					int idx = handView_.PickIndexByMouse(
						mouse.x, mouse.y,
						cam_->GetViewProjectionMatrix(),
						(float)WinApp::kClientWidth, (float)WinApp::kClientHeight
					);
					if (idx >= 0) {
						selectedIndex_ = idx;
						dragStartMouse_ = mouse;
						dragDx_ = dragDy_ = 0.0f;
						cardState_ = CardInputState::Dragging;
					}
				}
				break;

			case CardInputState::Dragging:
			{
				dragDx_ = float(mouse.x - dragStartMouse_.x);
				dragDy_ = float(mouse.y - dragStartMouse_.y);

				handView_.SetDrag(selectedIndex_, dragDx_, dragDy_, true);

				const float threshold = 80.0f;

				if (lRel) {
					handView_.SetDrag(-1, 0, 0, false);

					if (dragDy_ <= -threshold) {
						cardState_ = CardInputState::Preview;
						handView_.SetPreviewIndex(selectedIndex_);
					} else {
						cardState_ = CardInputState::Idle;
						selectedIndex_ = -1;
						handView_.SetPreviewIndex(-1);
					}
				}
			}
			break;

			case CardInputState::Preview:
				handView_.SetPreviewIndex(selectedIndex_);

				if (lTrig) {
					int idx = selectedIndex_;
					if (idx >= 0 && idx < (int)hand_.size()) {
						CardInstance inst = hand_[idx];
						const CardDef* def = db_.Find(inst.defId);

						if (def && def->cost <= energy_) {

							bool needsTarget = false;
							int dmgVal = 0;
							int hitCount = 0; // 攻撃回数（バフを乗せる回数）

							for (const auto& effect : def->effects) {
								// 通常ダメージ（OverClock等で複数ある場合は加算していく）
								if (effect.type == "Damage") {
									needsTarget = true;
									dmgVal += effect.value;
									hitCount++;
								}
								// クレセントムーン（奇数ターンなら+3ダメージ）
								else if (effect.type == "DamageCrescent") {
									needsTarget = true;
									int val = effect.value;
									if (playerTurnCount_ % 2 != 0) {
										val += 3; // 奇数ターンなら追加ダメージ
									}
									dmgVal += val;
									hitCount++;
								}
								// シールドバッシュ
								else if (effect.type == "DamageByBlock") {
									needsTarget = true;
									dmgVal += (player_ ? player_->GetBlock() : 0) * effect.value;
									hitCount++;
								}
							}

							// もし攻撃カードなら、発動せずに「敵を選ぶモード」へ移行！
							if (needsTarget) {
								int buff = currentTurnAtkUp_ + (player_ ? player_->GetBoostedPower() : 0);
								pendingDamage_ = dmgVal + (buff * hitCount);

								isPokerDamageTargeting_ = false;                 // 手札カード由来
								pendingCardHandIndex_ = idx;                     // 使った手札位置を覚える
								handView_.SetFocusIndex(idx);
								cardState_ = CardInputState::ChoosingEnemyTarget; // 敵選択へ

								selectedIndex_ = -1;
								handView_.SetPreviewIndex(-1);
								return;
							}
							energy_ -= def->cost;
							auto usedCardView = handView_.ExtractCardAt(idx);
							hand_.erase(hand_.begin() + idx);
							//handView_.Rebuild(hand_);



							ApplyCardEffects_(*def);

							if ((int)field_.size() < 5) {
								field_.push_back(inst);
								if (usedCardView) {
									usedCardView->SetIsHand(false);
									fieldViews_.push_back(std::move(usedCardView));
								}
								RebuildFieldView_();
								if ((int)field_.size() == 5) {
									PokerHandResult poker = EvaluatePokerHand_();
									TriggerSubEffectsForCard_(
										inst,
										SubEffectTrigger::OnPlayToField,
										poker.rank
									);
								}

								cardState_ = CardInputState::Idle;
								hasPendingCard_ = false;
								pendingCard_ = {};
							} else {
								pendingCard_ = inst;
								hasPendingCard_ = true;
								pendingCardView_ = std::move(usedCardView);
								cardState_ = CardInputState::ChoosingFieldReplace;
							}
						} else {
							cardState_ = CardInputState::Idle;
						}
					} else {
						cardState_ = CardInputState::Idle;
					}

					selectedIndex_ = -1;
					handView_.SetPreviewIndex(-1);
				}

				if (rTrig) {
					cardState_ = CardInputState::Idle;
					selectedIndex_ = -1;
					handView_.SetPreviewIndex(-1);
				}
				break;

			case CardInputState::ChoosingFieldReplace:
			{
				int newHover = PickFieldIndexByMouse_(mouse.x, mouse.y);

				if (newHover != fieldReplaceHoverIndex_) {
					fieldReplaceHoverIndex_ = newHover;
					fieldLayoutDirty_ = true;
				}

				if (lTrig) {
					int replaceIndex = fieldReplaceHoverIndex_;
					if (replaceIndex >= 0 && replaceIndex < (int)field_.size() && hasPendingCard_) {
						if (fieldViews_[replaceIndex]) {
							handView_.AddDiscardingCard(std::move(fieldViews_[replaceIndex]));
						}
						discard_.push_back(field_[replaceIndex]);
						field_[replaceIndex] = pendingCard_;
						if (pendingCardView_) {
							pendingCardView_->SetIsHand(false);
							fieldViews_[replaceIndex] = std::move(pendingCardView_);
						}
						RebuildFieldView_();
						RebuildDiscardView_();

						PokerHandResult poker = EvaluatePokerHand_();
						TriggerSubEffectsForCard_(
							pendingCard_,
							SubEffectTrigger::OnPlayToField,
							poker.rank
						);

						hasPendingCard_ = false;
						pendingCard_ = {};
						fieldReplaceHoverIndex_ = -1;
						prevFieldReplaceHoverIndex_ = -1;
						cardState_ = CardInputState::Idle;
						fieldLayoutDirty_ = true;
					}
				}

				if (rTrig) {
					if (hasPendingCard_) {
						discard_.push_back(pendingCard_);
					}
					if (pendingCardView_) {
						handView_.AddDiscardingCard(std::move(pendingCardView_));
					}
					hasPendingCard_ = false;
					pendingCard_ = {};

					fieldReplaceHoverIndex_ = -1;
					prevFieldReplaceHoverIndex_ = -1;
					cardState_ = CardInputState::Idle;
					RebuildDiscardView_();
					fieldLayoutDirty_ = true;
				}

				handView_.SetHoverIndex(-1);
				handView_.SetDrag(-1, 0, 0, false);
				handView_.SetPreviewIndex(-1);
			}
			break;
			case CardInputState::ChoosingEnemyTarget:
			{
				// マウスの位置にいる敵を探す
				int hoverIndex = enemyMgr_->PickEnemyByMouse(
					mouse.x, mouse.y,
					cam_->GetViewProjectionMatrix(),
					(float)WinApp::kClientWidth, (float)WinApp::kClientHeight
				);

				// マウスが重なっている敵を黄色く光らせる
				if (hoverIndex >= 0) {
					enemyMgr_->GetEnemies()[hoverIndex].SetHighlight(true);
				}

				// 左クリック：決定して攻撃！
				if (lTrig) {
					if (hoverIndex >= 0) {
						Enemy& targetEnemy = enemyMgr_->GetEnemies()[hoverIndex];

						//nextTurnAtkUp_ = 0;
						if (isPokerDamageTargeting_) {
							ApplyDamageToEnemy_(targetEnemy, pendingDamage_);
							// 選んだ敵に向かって突進＆ダメージ！
							if (pendingDamage_ > 0) {
								SpawnDamagePopup(targetEnemy.GetPos(), pendingDamage_, false);
							}


							// ポーカー役での攻撃だった場合
							ConsumeFieldCards_();
							cardState_ = CardInputState::Idle;
							turn_ = TurnState::Enemy;
							enemyTurnCount_++;
							enemyWait_ = 1.0f;
						} else {
							// 手札のカードでの攻撃だった場合
							int idx = pendingCardHandIndex_;
							CardInstance inst = hand_[idx];
							const CardDef* def = db_.Find(inst.defId);

							// コストを消費して手札から消す
							energy_ -= def->cost;
							auto usedCardView = handView_.ExtractCardAt(idx);
							hand_.erase(hand_.begin() + idx);

							handView_.Rebuild(hand_);

							// ダメージ以外の効果（ドローなど）を発動
							ApplyCardEffects_(*def, hoverIndex);

							handView_.SetFocusIndex(-1);

							// 場に出す処理
							if ((int)field_.size() < 5) {
								field_.push_back(inst);
								if (usedCardView) {
									usedCardView->SetIsHand(false);
									fieldViews_.push_back(std::move(usedCardView));
								}
								RebuildFieldView_();
								if ((int)field_.size() == 5) {
									PokerHandResult poker = EvaluatePokerHand_();
									TriggerSubEffectsForCard_(inst, SubEffectTrigger::OnPlayToField, poker.rank);
								}
								cardState_ = CardInputState::Idle;
								hasPendingCard_ = false;
								pendingCard_ = {};
							} else {
								pendingCard_ = inst;
								hasPendingCard_ = true;
								pendingCardView_ = std::move(usedCardView);
								cardState_ = CardInputState::ChoosingFieldReplace;
							}
						}
					}
				}

				// 右クリック：キャンセルして元に戻る
				if (rTrig) {
					handView_.SetFocusIndex(-1);
					cardState_ = CardInputState::Idle;
					if (isPokerDamageTargeting_) {
						pokerChoiceState_ = PokerChoiceState::WaitingEffectChoice; // ポーカー選択へ戻る
					}
				}
			}
			break;
			}
		}

	} else {
		// --------------------------------------------------
		// エネミーターンの処理
		// --------------------------------------------------
		handView_.SetHoverIndex(-1);
		handView_.SetDrag(-1, 0, 0, false);
		handView_.SetPreviewIndex(-1);
		cardState_ = CardInputState::Idle;
		selectedIndex_ = -1;
		enemyMgr_&& player_;
		hasPendingCard_ = false;
		pendingCard_ = {};

		enemyWait_ -= dt;

		if (enemyWait_ <= 0.0f) {

			if (enemyMgr_ && player_) {
				auto& enemies = enemyMgr_->GetEnemies();
				while (currentEnemyIndex_ < enemies.size() && !enemies[currentEnemyIndex_].IsAlive()) {
					currentEnemyIndex_++;
				}

				// まだ行動していない敵がいる場合
				if (currentEnemyIndex_ < enemies.size()) {

					// 今回行動する敵を1体だけ取得
					Enemy& e = enemies[currentEnemyIndex_];
					EnemyAction action = e.GetBossAI().GetNextAction();

					if (action.type == "Attack") {
						e.PlayAttackAnim(player_->GetPos());
						player_->TriggerHitFlash(0.2f);
						player_->PlayDamageAnim();
						player_->Damage(action.value);
						if (action.value > 0) {
							SpawnDamagePopup(player_->GetPos(), action.value, true); // ★追加
						}
					} else if (action.type == "Heal") {
						e.Heal(action.value);
					} else if (action.type == "Block") {
						// 防御処理
					}

					enemyWait_ = 1.0f;

					// 次の敵の番へ進めておく
					currentEnemyIndex_++;

				} else {
					// 全ての敵の行動が終わった場合
					currentEnemyIndex_ = 0; // 次のターンに向けてリセットしておく

					// プレイヤーターンへ移行
					turn_ = TurnState::Player;
					StartPlayerTurn_();
				}
			}
		}
	}

	handView_.Update(dt);
	if (fieldLayoutDirty_ || cardState_ == CardInputState::ChoosingFieldReplace || pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi) {
		RefreshAllFieldCardTransforms_(dt);
		fieldLayoutDirty_ = false;
	}
	for (auto& cardView : fieldViews_) {
		if (cardView) {
			cardView->Update(dt);
		}
	}
	if (discardView_) {
		discardView_->Update(dt);
	}
	// HPゲージの長さと位置を毎フレーム更新する
	// プレイヤーのHPゲージ計算
	if (player_ && playerHpFg_) {
		float hpRatio = (float)player_->GetHP() / (float)player_->GetMaxHP();
		if (hpRatio < 0.0f) hpRatio = 0.0f;

		//playerHpFg_->SetScale({ 250.0f * hpRatio, 18.0f, 1.0f });
		playerHpFg_->SetPosition({ 80.0f, 40.0f });
	}

	// ボスのHPゲージ計算
	if (enemyMgr_) {
		auto& enemies = enemyMgr_->GetEnemies();
		for (size_t i = 0; i < enemyHpFgs_.size(); ++i) {
			if (i < enemies.size() && enemies[i].IsAlive()) {
				float hpRatio = (float)enemies[i].GetHP() / (float)enemies[i].GetMaxHP();
				if (hpRatio < 0.0f) hpRatio = 0.0f;

				float gaugeWidth = 200.0f;
				float posX = 1000.0f; // 右上に配置
				float posY = 40.0f + (i * 30.0f); // 30pxずつ下にずらす

				enemyHpBgs_[i]->SetScale({ gaugeWidth, 15.0f, 1.0f });
				enemyHpBgs_[i]->SetPosition({ posX, posY });

				enemyHpFgs_[i]->SetScale({ gaugeWidth * hpRatio, 15.0f, 1.0f });
				enemyHpFgs_[i]->SetPosition({ posX, posY });
				EnemyAction nextAct = enemies[i].GetBossAI().GetNextAction();

				// 行動タイプによって色を変える！
				if (nextAct.type == "Attack") {
					enemyIntentIcons_[i]->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f }); // 赤（攻撃）
				} else if (nextAct.type == "Heal") {
					enemyIntentIcons_[i]->SetColor({ 0.2f, 1.0f, 0.2f, 1.0f }); // 緑（回復）
				} else {
					enemyIntentIcons_[i]->SetColor({ 0.8f, 0.8f, 0.8f, 1.0f }); // 白・グレー（その他）
				}

				// HPゲージの少し左に配置する
				enemyIntentIcons_[i]->SetScale({ 20.0f, 20.0f, 1.0f });
				// HPゲージの原点にもよりますが、左に30pxほどずらします
				enemyIntentIcons_[i]->SetPosition({ posX - 30.0f, posY });
			} else {
				// 敵がいない、または死んでいる場合はゲージを見えなくする
				enemyHpBgs_[i]->SetScale({ 0.0f, 0.0f, 1.0f });
				enemyHpFgs_[i]->SetScale({ 0.0f, 0.0f, 1.0f });
				enemyIntentIcons_[i]->SetScale({ 0.0f, 0.0f, 1.0f });
			}
		}
	}
	// 2D UI用の行列を作成する（Mathクラスを使用）
	// View行列（カメラは原点でまっすぐ前を向く = 単位行列）
	Matrix4x4 viewMat = Matrix4x4::MakeIdentity4x4();

	// Projection行列（画面サイズに合わせた正射影行列）
	float width = (float)WinApp::kClientWidth;
	float height = (float)WinApp::kClientHeight;
	Matrix4x4 projMat = Matrix4x4::MakeOrthographicMatrix(width, height);

	//if (energy_ != prevEnergy_ || energyMax_ != prevEnergyMax_) {
	//	RebuildCostView_(dt);
	//} else {
	//	UpdateCostViewTransform_(dt);
	//}

	//if (cardState_ == CardInputState::ChoosingFieldReplace) {
	//	RefreshAllFieldCardTransforms_(dt);
	//}

	for (auto it = damagePopups_.begin(); it != damagePopups_.end(); ) {
		it->timer -= 1.0f;        // 1フレームごとにタイマーを1減らす
		it->pos.y += 0.05f;       // 1フレームごとに少し上に昇る

		if (it->timer <= 0.0f) {
			it = damagePopups_.erase(it); // 寿命が来たら消す
		} else {
			float gap = 0.8f;

			float startX = it->pos.x - gap * 0.5f * (it->digitModels.size() - 1);

			for (size_t i = 0; i < it->digitModels.size(); ++i) {
				// キャラクターより少し手前(z - 1.0f)に配置
				Vector3 digitPos = { startX + gap * i, it->pos.y, it->pos.z - 1.0f };

				it->digitModels[i]->SetTranslate(digitPos);
				it->digitModels[i]->SetScale({ 0.8f, 0.8f, 0.8f });

				// （もし数字が横や後ろを向いてしまう場合はここで SetRotate で回す）
				// it->digitModels[i]->SetRotate({ 0.0f, 0.0f, 0.0f });

				it->digitModels[i]->Update(dt);
			}
			++it;
		}
	}

	// --------------------------------------------------
	// スプライトの更新
	// --------------------------------------------------
	if (playerHpBg_) playerHpBg_->Update(viewMat, projMat);
	if (playerHpFg_) playerHpFg_->Update(viewMat, projMat);
	UpdateHpGauges();
	if (playerHpPredict_)playerHpPredict_->Update(viewMat, projMat);
	if (playerBlockPredict_)playerBlockPredict_->Update(viewMat, projMat);
	for (auto& bg : enemyHpBgs_) { if (bg) bg->Update(viewMat, projMat); }
	for (auto& fg : enemyHpFgs_) { if (fg) fg->Update(viewMat, projMat); }
	for (auto& icon : enemyIntentIcons_) { if (icon) icon->Update(viewMat, projMat); }
	if (highlightFilter_)highlightFilter_->Update(viewMat, projMat);

}

void BattleController::HandlePokerActivateChoice_(FieldUi& fieldUi, POINT mouse, bool lTrig, bool yTrig, bool nTrig)
{
	if (pokerChoiceJustOpened_) {
		pokerChoiceJustOpened_ = false;
		return;
	}

	const auto& layout = fieldUi.GetPokerEffectChoiceLayout();

	if (PointInRect(mouse.x, mouse.y,
		layout.infoButtonRect.x, layout.infoButtonRect.y,
		layout.infoButtonRect.w, layout.infoButtonRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::None;
		if (lTrig) {
			pokerQuickPreviewVisible_ = !pokerQuickPreviewVisible_;
			return;
		}
	} else if (PointInRect(mouse.x, mouse.y,
		layout.activateYesRect.x, layout.activateYesRect.y,
		layout.activateYesRect.w, layout.activateYesRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::ActivateYes;
	} else if (!tutorialActivateOnly_ &&
		PointInRect(mouse.x, mouse.y,
			layout.activateNoRect.x, layout.activateNoRect.y,
			layout.activateNoRect.w, layout.activateNoRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::ActivateNo;
	} else if (!tutorialActivateOnly_ &&
		PointInRect(mouse.x, mouse.y,
			layout.activateViewBoardRect.x, layout.activateViewBoardRect.y,
			layout.activateViewBoardRect.w, layout.activateViewBoardRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::ActivateViewBoard;
	}

	if (yTrig || (lTrig && pokerMouseChoice_ == PokerMouseChoice::ActivateYes)) {
		pokerQuickPreviewVisible_ = false;
		pokerChoiceState_ = PokerChoiceState::WaitingEffectChoice;
		pokerChoiceJustOpened_ = true;
		return;
	}

	if (!tutorialActivateOnly_ &&
		(nTrig || (lTrig && pokerMouseChoice_ == PokerMouseChoice::ActivateNo))) {
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Skipped;
		pokerChoiceState_ = PokerChoiceState::None;
		return;
	}

	if (!tutorialActivateOnly_ &&
		lTrig && pokerMouseChoice_ == PokerMouseChoice::ActivateViewBoard) {
		pokerQuickPreviewVisible_ = false;
		pokerReturnState_ = PokerChoiceState::WaitingActivateChoice;
		pokerChoiceState_ = PokerChoiceState::ViewingBoardFromPokerUi;

		handView_.SetPreviewIndex(-1);
		handView_.SetDrag(-1, 0.0f, 0.0f, false);
		handView_.SetFocusIndex(-1);
		handView_.SetHoverIndex(-1);

		fieldReplaceHoverIndex_ = -1;
		fieldLayoutDirty_ = true;
		return;
	}

	// チュートリアル中は「発動する」を強制ハイライト
	if (tutorialActivateOnly_) {
		if (!(PointInRect(mouse.x, mouse.y,
			layout.infoButtonRect.x, layout.infoButtonRect.y,
			layout.infoButtonRect.w, layout.infoButtonRect.h))) {
			pokerMouseChoice_ = PokerMouseChoice::ActivateYes;
		}
	}
}

void BattleController::HandlePokerEffectChoice_(FieldUi& fieldUi, POINT mouse, bool lTrig, bool nTrig)
{
	if (pokerChoiceJustOpened_) {
		pokerChoiceJustOpened_ = false;
		return;
	}

	PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);
	const auto& layout = fieldUi.GetPokerEffectChoiceLayout();

	// 毎フレームいったんリセット
	pokerMouseChoice_ = PokerMouseChoice::None;

	// -----------------------------
	// チュートリアル中は「ダメージ」だけ光らせる
	// -----------------------------
	if (tutorialDamageOnly_) {
		// infoボタンだけは通常通り押せる
		if (PointInRect(mouse.x, mouse.y,
			layout.infoButtonRect.x, layout.infoButtonRect.y,
			layout.infoButtonRect.w, layout.infoButtonRect.h)) {
			if (lTrig) {
				pokerQuickPreviewVisible_ = !pokerQuickPreviewVisible_;
				return;
			}
		} else {
			// 常にダメージだけハイライト
			pokerMouseChoice_ = PokerMouseChoice::EffectDamage;
		}

		if (lTrig && pokerMouseChoice_ == PokerMouseChoice::EffectDamage) {
			TriggerSubEffectsForField_(SubEffectTrigger::OnPokerSkillActivated, currentPoker_.rank);
			pendingDamage_ = CalcFinalAttackDamage_(bonus.damage);
			isPokerDamageTargeting_ = true;
			pokerQuickPreviewVisible_ = false;
			lastPokerTutorialResult_ = PokerTutorialResult::Activated;
			cardState_ = CardInputState::ChoosingEnemyTarget;
			pokerChoiceState_ = PokerChoiceState::None;
			return;
		}

		return;
	}

	// -----------------------------
	// 通常時
	// -----------------------------
	if (PointInRect(mouse.x, mouse.y,
		layout.infoButtonRect.x, layout.infoButtonRect.y,
		layout.infoButtonRect.w, layout.infoButtonRect.h)) {
		if (lTrig) {
			pokerQuickPreviewVisible_ = !pokerQuickPreviewVisible_;
			return;
		}
	} else if (PointInRect(mouse.x, mouse.y,
		layout.backRect.x, layout.backRect.y,
		layout.backRect.w, layout.backRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::EffectBack;
	} else if (PointInRect(mouse.x, mouse.y,
		layout.effectRects[0].x, layout.effectRects[0].y,
		layout.effectRects[0].w, layout.effectRects[0].h)) {
		pokerMouseChoice_ = PokerMouseChoice::EffectAtkUp;
	} else if (PointInRect(mouse.x, mouse.y,
		layout.effectRects[1].x, layout.effectRects[1].y,
		layout.effectRects[1].w, layout.effectRects[1].h)) {
		pokerMouseChoice_ = PokerMouseChoice::EffectDamage;
	} else if (PointInRect(mouse.x, mouse.y,
		layout.effectRects[2].x, layout.effectRects[2].y,
		layout.effectRects[2].w, layout.effectRects[2].h)) {
		pokerMouseChoice_ = PokerMouseChoice::EffectDraw;
	} else if (PointInRect(mouse.x, mouse.y,
		layout.effectViewBoardRect.x, layout.effectViewBoardRect.y,
		layout.effectViewBoardRect.w, layout.effectViewBoardRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::EffectViewBoard;
	}

	if (lTrig && pokerMouseChoice_ == PokerMouseChoice::EffectAtkUp) {
		nextTurnAtkUp_ += bonus.atkUp;
		TriggerSubEffectsForField_(SubEffectTrigger::OnPokerSkillActivated, currentPoker_.rank);
		ConsumeFieldCards_();
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Activated;
		pokerChoiceState_ = PokerChoiceState::None;
		turn_ = TurnState::Enemy;
		enemyWait_ = 1.0f;
		return;
	}

	if (lTrig && pokerMouseChoice_ == PokerMouseChoice::EffectDraw) {
		DrawCards_(bonus.drawCount);
		TriggerSubEffectsForField_(SubEffectTrigger::OnPokerSkillActivated, currentPoker_.rank);
		ConsumeFieldCards_();
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Activated;
		pokerChoiceState_ = PokerChoiceState::None;
		turn_ = TurnState::Enemy;
		enemyWait_ = 1.0f;
		return;
	}

	if (lTrig && pokerMouseChoice_ == PokerMouseChoice::EffectDamage) {
		TriggerSubEffectsForField_(SubEffectTrigger::OnPokerSkillActivated, currentPoker_.rank);
		pendingDamage_ = CalcFinalAttackDamage_(bonus.damage);
		isPokerDamageTargeting_ = true;
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Activated;
		cardState_ = CardInputState::ChoosingEnemyTarget;
		pokerChoiceState_ = PokerChoiceState::None;
		return;
	}

	if (nTrig || (lTrig && pokerMouseChoice_ == PokerMouseChoice::EffectBack)) {
		pokerQuickPreviewVisible_ = false;
		pokerChoiceState_ = PokerChoiceState::WaitingActivateChoice;
		return;
	}

	if (lTrig && pokerMouseChoice_ == PokerMouseChoice::EffectViewBoard) {
		pokerQuickPreviewVisible_ = false;
		pokerReturnState_ = PokerChoiceState::WaitingEffectChoice;
		pokerChoiceState_ = PokerChoiceState::ViewingBoardFromPokerUi;

		handView_.SetPreviewIndex(-1);
		handView_.SetDrag(-1, 0.0f, 0.0f, false);
		handView_.SetFocusIndex(-1);
		handView_.SetHoverIndex(-1);

		fieldReplaceHoverIndex_ = -1;
		fieldLayoutDirty_ = true;
		return;
	}
}

void BattleController::HandlePokerViewBoard_(FieldUi& fieldUi, POINT mouse, bool lTrig, float dt)
{
	handView_.SetPreviewIndex(-1);
	handView_.SetDrag(-1, 0.0f, 0.0f, false);
	handView_.SetFocusIndex(-1);

	const auto& layout = fieldUi.GetPokerEffectChoiceLayout();

	if (PointInRect(mouse.x, mouse.y,
		layout.backRect.x, layout.backRect.y,
		layout.backRect.w, layout.backRect.h)) {
		pokerMouseChoice_ = PokerMouseChoice::ReturnFromBoard;
	} else {
		pokerMouseChoice_ = PokerMouseChoice::None;
	}

	int hover = handView_.PickIndexByMouse(
		mouse.x, mouse.y,
		cam_->GetViewProjectionMatrix(),
		(float)WinApp::kClientWidth, (float)WinApp::kClientHeight
	);
	handView_.SetHoverIndex(hover);

	if (hover < 0) {
		int newHover = PickFieldIndexByMouse_(mouse.x, mouse.y);
		if (newHover != fieldReplaceHoverIndex_) {
			fieldReplaceHoverIndex_ = newHover;
			fieldLayoutDirty_ = true;
		}
	} else {
		if (fieldReplaceHoverIndex_ != -1) {
			fieldReplaceHoverIndex_ = -1;
			fieldLayoutDirty_ = true;
		}
	}

	handView_.Update(dt);
	RefreshAllFieldCardTransforms_(dt);
	fieldLayoutDirty_ = false;

	if (lTrig && pokerMouseChoice_ == PokerMouseChoice::ReturnFromBoard) {
		handView_.SetHoverIndex(-1);
		fieldReplaceHoverIndex_ = -1;
		fieldLayoutDirty_ = true;

		pokerChoiceState_ = pokerReturnState_;
		pokerReturnState_ = PokerChoiceState::None;
		pokerChoiceJustOpened_ = true;
		return;
	}
}


void BattleController::Draw3D(GameApp& app)
{

	// 墓地
	if (discardView_) {
		discardView_->Draw();
	}

	// 手札
	handView_.Draw();

	// ダメージポップアップ
	for (auto& popup : damagePopups_) {
		for (auto& obj : popup.digitModels) {
			if (obj) obj->Draw();
		}
	}

	// 場と交換時フィルター
	if (cardState_ == CardInputState::ChoosingFieldReplace) {
		highlightFilter_->Draw();
	}

	// 場のカード
	for (auto& c : fieldViews_) {
		c->Draw();
	}

	if (cardState_ == CardInputState::ChoosingEnemyTarget) {
		highlightFilter_->Draw();
	}

}

void BattleController::Draw2D(GameApp& app)
{


	if (playerHpBg_) playerHpBg_->Draw();
	if (playerHpPredict_)playerHpPredict_->Draw();
	if (playerBlockPredict_ && player_->GetBlock() > 0)playerBlockPredict_->Draw();
	if (playerHpFg_) playerHpFg_->Draw();

	for (auto& bg : enemyHpBgs_) {
		if (bg) bg->Draw();
	}
	for (auto& fg : enemyHpFgs_) {
		if (fg) fg->Draw();
	}
	for (auto& icon : enemyIntentIcons_) {
		if (icon) icon->Draw();
	}

}

#ifdef USE_IMGUI
#include <imgui.h>
void BattleController::DrawImGui()
{
	Card3D::DrawAdjustImGui();

	ImGui::Text("turn: %s", turn_ == TurnState::Player ? "Player" : "Enemy");
	ImGui::Text("PlayerTurnCount : %d", playerTurnCount_);
	ImGui::Text("EnemyTurnCount : %d", enemyTurnCount_);
	ImGui::Text("energy: %d / %d", energy_, energyMax_);
	ImGui::Text("hand: %d  discard: %d", (int)hand_.size(), (int)discard_.size());
	ImGui::Text("field: %d", (int)field_.size());

	handView_.DrawImGui();

	const char* stateName = "";
	switch (cardState_) {
	case CardInputState::Idle: stateName = "Idle"; break;
	case CardInputState::Dragging: stateName = "Dragging"; break;
	case CardInputState::Preview: stateName = "Preview"; break;
	case CardInputState::ChoosingFieldReplace: stateName = "ChoosingFieldReplace"; break;
	}
	ImGui::Text("cardState: %s", stateName);

	if (hasPendingCard_) {
		ImGui::Text("pending: defId=%d number=%d suit=%s",
			pendingCard_.defId,
			pendingCard_.number,
			SuitToString(pendingCard_.suit));
	} else {
		ImGui::Text("pending: none");
	}

	ImGui::Separator();
	ImGui::Text("Hand Cards");
	for (int i = 0; i < (int)hand_.size(); ++i) {
		ImGui::Text("hand[%d] defId=%d number=%d suit=%s",
			i,
			hand_[i].defId,
			hand_[i].number,
			SuitToString(hand_[i].suit));
	}

	ImGui::Separator();
	ImGui::Text("Field Cards");
	for (int i = 0; i < (int)field_.size(); ++i) {
		ImGui::Text("field[%d] defId=%d number=%d suit=%s",
			i,
			field_[i].defId,
			field_[i].number,
			SuitToString(field_[i].suit));
	}

	ImGui::Separator();
	PokerHandResult poker = EvaluatePokerHand_();
	ImGui::Text("Poker Hand: %s", GetPokerHandName_(poker.rank));
	ImGui::Text("Poker Power: %d", poker.power);

	if (pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice)
	{
		ImGui::Separator();
		ImGui::Text("Poker Skill Available!");
		ImGui::Text("Hand : %s", GetPokerHandName_(currentPoker_.rank));
		ImGui::Text("Press Y = Activate");
		ImGui::Text("Press N = Skip");
	}

	if (pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice)
	{
		PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);

		ImGui::Separator();
		ImGui::Text("Choose Poker Effect");
		ImGui::Text("Hand : %s", GetPokerHandName_(currentPoker_.rank));
		ImGui::Text("1 : Next Turn ATK UP (+%d)", bonus.atkUp);
		ImGui::Text("2 : Draw %d", bonus.drawCount);
		ImGui::Text("3 : Damage %d", bonus.damage);
		ImGui::Text("N : Back");
	}
	ImGui::Separator();

	if (player_) {
		ImGui::Text("Player Hp: %d", player_->GetHP());
		ImGui::Text("Player Hp: %d (Block: %d)", player_->GetHP(), player_->GetBlock());
		ImGui::Text("Player Power: %d (CurrentTurnAtkUp: %d / NextTurnAtkUp: %d)",
			player_->GetBoostedPower(),
			currentTurnAtkUp_,
			nextTurnAtkUp_);
	} else {
		ImGui::Text("Player: null");
		ImGui::Text("Player Power: preview mode (CurrentTurnAtkUp: %d / NextTurnAtkUp: %d)",
			currentTurnAtkUp_,
			nextTurnAtkUp_);
	}

	if (enemyMgr_ && !enemyMgr_->GetEnemies().empty()) {
		ImGui::Text("Enemy Hp: %d", enemyMgr_->GetEnemies()[0].GetHP());
	} else {
		ImGui::Text("Enemy: null");
	}

	ImGui::Separator();
	ImGui::Text("Attack Debug");

	ImGui::Checkbox("Use Debug Preview Buff", &useDebugPreviewBuff_);

	if (player_) {
		ImGui::Text("Runtime Player Connected");

		int previewPower = player_->GetBoostedPower();
		if (ImGui::DragInt("Player PowerBoost", &previewPower, 1.0f, -999, 999)) {
			player_->ResetPowerBoost();
			if (previewPower > 0) {
				player_->PowerBoost(previewPower);
			}
		}

		ImGui::DragInt("CurrentTurnAtkUp", &currentTurnAtkUp_, 1.0f, -999, 999);
		ImGui::DragInt("NextTurnAtkUp", &nextTurnAtkUp_, 1.0f, -999, 999);
	} else {
		ImGui::Text("Preview Only (No Player Connected)");
		ImGui::DragInt("Debug PowerBoost", &debugPreviewPowerBoost_, 1.0f, -999, 999);
		ImGui::DragInt("Debug CurrentTurnAtkUp", &debugPreviewCurrentTurnAtkUp_, 1.0f, -999, 999);
		ImGui::DragInt("Debug NextTurnAtkUp", &debugPreviewNextTurnAtkUp_, 1.0f, -999, 999);
	}

}
#endif

int BattleController::CalcFinalAttackDamage_(int baseDamage) const
{
	int total = baseDamage;

	if (player_) {
		total += player_->GetBoostedPower();
	} else if (useDebugPreviewBuff_) {
		total += debugPreviewPowerBoost_;
	}

	if (player_) {
		total += currentTurnAtkUp_;
	} else if (useDebugPreviewBuff_) {
		total += debugPreviewCurrentTurnAtkUp_;
	} else {
		total += currentTurnAtkUp_;
	}

	if (total < 0) {
		total = 0;
	}

	return total;
}

int BattleController::GetDisplayEffectValue(const CardEffectDef& effect, bool applyAttackBuff) const
{
	if (!applyAttackBuff) {
		if (effect.type == "DamageCrescent") {
			int value = effect.value;
			if (playerTurnCount_ % 2 != 0) {
				value += 3;
			}
			return value;
		}
		if (effect.type == "DamageByBlock") {
			return (player_ ? player_->GetBlock() : 0) * effect.value;
		}
		return effect.value;
	}

	if (effect.type == "Damage") {
		return CalcFinalAttackDamage_(effect.value);
	}

	if (effect.type == "DamageAll") {
		return CalcFinalAttackDamage_(effect.value);
	}

	if (effect.type == "DamageCrescent") {
		int value = effect.value;
		if (playerTurnCount_ % 2 != 0) {
			value += 3;
		}
		return CalcFinalAttackDamage_(value);
	}

	if (effect.type == "DamageByBlock") {
		int value = (player_ ? player_->GetBlock() : 0) * effect.value;
		return CalcFinalAttackDamage_(value);
	}

	return effect.value;
}

void BattleController::ApplyDamageToEnemy_(Enemy& enemy, int damage)
{
	if (!player_) {
		return;
	}

	player_->PlayAttackAnim(enemy.GetPos());
	enemy.TriggerHitFlash(0.2f);
	enemy.PlayDamageAnim();
	enemy.Damage(damage);

	if (player_->GetVampireHeal() > 0) {
		player_->Heal(player_->GetVampireHeal());
	}
}

void BattleController::SetPlayer(Player* player) {
	player_ = player;
}

void BattleController::SetEnemyManager(EnemyManager* enemyMgr) {
	enemyMgr_ = enemyMgr;
}

void BattleController::SpawnDamagePopup(const Vector3& pos, int damage, bool isPlayer)
{
	DamagePopup p;
	p.damage = damage;
	p.pos = pos;
	p.pos.y += 2.0f; // キャラクターの頭上からスタート
	p.timer = 60.0f; // 60フレーム表示させる

	// 数字を文字列（"15"など）にして、1文字ずつ処理する
	std::string dmgStr = std::to_string(damage);
	for (char c : dmgStr) {
		if (c >= '0' && c <= '9') {
			// コストと同じようにモデルのパスを作る（例："cards/models/5.obj"）
			std::string path = "cards/models/";
			path += c;
			path += ".obj";

			auto obj = std::make_unique<Object3d>();

			obj->Initialize(objCom_, dx_);
			obj->SetModel(path);

			p.digitModels.push_back(std::move(obj));
		}
	}

	damagePopups_.push_back(std::move(p));
}

const CardDef* BattleController::GetPreviewCardDef() const
{
	if (cardState_ == CardInputState::Preview) {
		if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(hand_.size())) {
			return db_.Find(hand_[selectedIndex_].defId);
		}
	}

	if (cardState_ == CardInputState::ChoosingFieldReplace) {
		if (fieldReplaceHoverIndex_ >= 0 &&
			fieldReplaceHoverIndex_ < static_cast<int>(field_.size())) {
			return db_.Find(field_[fieldReplaceHoverIndex_].defId);
		}
	}

	if (pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi) {
		int handHover = handView_.GetHoverIndex();
		if (handHover >= 0 && handHover < static_cast<int>(hand_.size())) {
			return db_.Find(hand_[handHover].defId);
		}

		if (fieldReplaceHoverIndex_ >= 0 &&
			fieldReplaceHoverIndex_ < static_cast<int>(field_.size())) {
			return db_.Find(field_[fieldReplaceHoverIndex_].defId);
		}
	}

	return nullptr;
}

bool BattleController::HasPokerChoiceUi() const
{
	return pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice ||
		pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice;
}

std::wstring BattleController::GetPokerChoiceUiText() const
{
	if (pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice) {
		std::wstring text = L"";
		text += L"ポーカー効果が発動可能です\n";
		text += L"役: ";
		text += std::wstring(GetPokerHandName_(currentPoker_.rank),
			GetPokerHandName_(currentPoker_.rank) + std::strlen(GetPokerHandName_(currentPoker_.rank)));
		text += L"\n";
		text += L"左クリック : 発動する\n";
		text += L"左クリック : 発動しない\n";
		text += L"左クリック : 場を見る\n";
		return text;
	}

	if (pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice) {
		PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);

		std::wstring text = L"";
		text += L"発動する効果を選んでください\n";
		text += L"左クリック : 戻る\n";
		text += L"左クリック : 次ターンATK UP (+" + std::to_wstring(bonus.atkUp) + L")\n";
		text += L"左クリック : " + std::to_wstring(bonus.drawCount) + L"枚ドロー\n";
		text += L"左クリック : " + std::to_wstring(bonus.damage) + L"ダメージ\n";
		text += L"左クリック : 場を見る\n";
		return text;
	}

	if (pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi) {
		std::wstring text;
		text += L"場確認中\n";
		text += L"カードにマウスを乗せて確認できます\n";
		text += L"左クリック : 特殊効果選択に戻る\n";
		return text;
	}

	return L"";
}

int BattleController::GetPokerMouseChoiceIndex() const
{
	switch (pokerMouseChoice_) {
	case PokerMouseChoice::ActivateYes:       return 0;
	case PokerMouseChoice::ActivateNo:        return 1;
	case PokerMouseChoice::ActivateViewBoard: return 2;

	case PokerMouseChoice::EffectBack:        return 0;
	case PokerMouseChoice::EffectAtkUp:       return 1;
	case PokerMouseChoice::EffectDamage:      return 2;
	case PokerMouseChoice::EffectDraw:        return 3;
	case PokerMouseChoice::EffectViewBoard:   return 4;

	case PokerMouseChoice::ReturnFromBoard:   return 0;

	default:                                  return -1;
	}
}

bool BattleController::IsWaitingActivateChoice() const
{
	return pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice;
}

bool BattleController::IsWaitingEffectChoice() const
{
	return pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice;
}

bool BattleController::IsViewingBoardFromPokerUi() const
{
	return pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi;
}

bool BattleController::ShouldShowOperationUi() const
{
	return operationUiVisible_;
}

std::wstring BattleController::GetOperationUiText() const
{
	if (cardState_ == CardInputState::ChoosingFieldReplace) {
		std::wstring text;
		text += L"カード交換\n";
		text += L"左クリック : 選択中の場カードと入れ替える\n";
		text += L"右クリック : 入れ替えず墓地へ送る\n";
		return text;
	}

	if (cardState_ == CardInputState::Preview) {
		std::wstring text;
		text += L"カード選択中\n";
		text += L"左クリック : 使用する\n";
		text += L"右クリック : キャンセル\n";
		text += L"Tab : 操作説明を表示\n";
		return text;
	}

	std::wstring text;
	text += L"基本操作\n";
	text += L"左クリック＋上ドラッグ : カードをプレビュー\n";
	text += L"プレビュー中に左クリック : カードを使用\n";
	text += L"プレビュー中に右クリック : キャンセル\n";
	text += L"Enter : ターン終了\n";
	text += L"Tab : 操作説明を表示\n";
	return text;
}

std::wstring BattleController::GetZoneCountUiText() const
{
	std::wstring text;
	text += L"山札 : " + std::to_wstring(deck_.size()) + L"\n";
	text += L"手札 : " + std::to_wstring(hand_.size()) + L"\n";
	text += L"墓地 : " + std::to_wstring(discard_.size()) + L"\n";
	text += L"場   : " + std::to_wstring(field_.size()) + L"\n";
	return text;
}

std::wstring BattleController::GetCurrentPokerHandUiText() const
{
	PokerHandResult poker = EvaluatePokerHand_();

	if (field_.size() < 5) {
		return L"役:       判定中";
	}

	if (poker.rank == PokerHandRank::None) {
		return L"役:       なし";
	}

	switch (poker.rank) {
	case PokerHandRank::OnePair: return L"役: ワンペア";
	case PokerHandRank::TwoPair: return L"役: ツーペア";
	case PokerHandRank::ThreeOfAKind: return L"役: スリーカード";
	case PokerHandRank::Straight: return L"役: ストレート";
	case PokerHandRank::Flush: return L"役: フラッシュ";
	case PokerHandRank::FullHouse: return L"役: フルハウス";
	case PokerHandRank::FourOfAKind: return L"役: フォーカード";
	case PokerHandRank::StraightFlush: return L"役: ストレートフラッシュ";
	case PokerHandRank::RoyalStraightFlush: return L"役: ロイヤルストレートフラッシュ";
	default: return L"役: ?";
	}
}

std::wstring BattleController::GetTurnUiText() const
{
	std::wstring text;

	switch (turn_) {
	case TurnState::Player: return L"あなたのターン : " + std::to_wstring(playerTurnCount_);
	case TurnState::Enemy: return L"あいてのターン : " + std::to_wstring(enemyTurnCount_);
	}

	return text;
}

std::wstring BattleController::GetEnergyText() const {

	std::wstring text;

	text += std::to_wstring(energy_) + L" / " + std::to_wstring(energyMax_);

	return text;

}

std::vector<std::wstring> BattleController::GetEnemyHpTexts() const {
	std::vector<std::wstring> hpTexts;
	auto& enemies = enemyMgr_->GetEnemies();

	for (const auto& enemy : enemies) {
		if (enemy.IsAlive()) {
			// "100 / 100" という形式の文字列を作成
			std::wstring text = std::to_wstring(enemy.GetHP()) + L" / " + std::to_wstring(enemy.GetMaxHP());
			hpTexts.push_back(text);
		}
	}
	return hpTexts;
}


BattleController::PokerBonus BattleController::GetCurrentPokerBonusForUi() const
{
	return GetPokerBonus_(currentPoker_.rank);
}


std::wstring BattleController::GetPlayerHpTexts() const {
	std::wstring text;

	text = std::to_wstring(player_->GetHP()) + L" / " + std::to_wstring(player_->GetMaxHP());

	return text;
}

bool BattleController::IsAllEnemiesDead() const {
	auto& enemies = enemyMgr_->GetEnemies();
	for (auto& e : enemies) {
		if (e.IsAlive()) return false; // 一人でも生きていたらfalse
	}
	return true; // 全員死んでいたらtrue
}

std::wstring BattleController::GetPlayerPowerBoostText()const {
	std::wstring text;

	text = std::to_wstring(player_->GetBoostedPower());

	return text;
}

std::wstring BattleController::GetPlayerBlockText()const {
	std::wstring text;

	text = std::to_wstring(player_->GetBlock());

	return text;
}

int BattleController::CalcTotalIncomingDamage() const {
	int total = 0;
	if (!enemyMgr_) return 0;

	for (auto& enemy : enemyMgr_->GetEnemies()) {
		if (enemy.IsAlive()) {
			// 敵が次のターンに行う攻撃力を取得（シールド等があればここで減算処理）
			total += enemy.GetIncomingDamage() - player_->GetBlock();
		}
	}
	return total;
}

void BattleController::UpdateHpGauges() {
	float maxHP = (float)player_->GetMaxHP();
	float currentHP = (float)player_->GetHP();
	int incomingDamage = CalcTotalIncomingDamage();

	// 現在のHPバーの長さ
	float currentRatio = currentHP / maxHP;


	// 予測ダメージ後のHPバー（ここがミソ）
	// 現在のHPからダメージを引いた残量を計算（0以下にならないよう clamp）
	float predictedHP = std::max(0.0f, currentHP - incomingDamage);
	if (predictedHP >= currentHP) {
		predictedHP = currentHP;
	}
	float predictedRatio = predictedHP / maxHP;

	int currentBlock = player_->GetBlock();
	float predictedRatioBlock = float(currentBlock) / maxHP;

	// 赤いバー（playerHpPredict_）は「現在のHPバーと同じ位置」に置きつつ、
	// 長さは「現在のHP」のままにする。
	// 緑のバー（playerHpFg_）を「予測後のHP」の長さに縮めることで、
	// 「元あった場所が赤く残る」という表現になります。

	if (turn_ == TurnState::Player) {
		playerHpFg_->SetScale({ 250.0f * predictedRatio, 18.0f, 1.0f });
	}
	playerHpPredict_->SetScale({ 250.0f * currentRatio, 18.0f, 1.0f });
	playerHpPredict_->SetPosition(playerHpFg_->GetPosition());

	float offset = 5.f;

	playerBlockPredict_->SetScale({ 250.0f * predictedRatioBlock + offset,18.0f + (offset * 2.f), 1.0f });
	playerBlockPredict_->SetPosition({ 80.f - offset,40.f - offset });

}

//=====================
//チュートリアル用
//=====================
void BattleController::SetTutorialOpeningHand(const std::vector<CardInstance>& cards)
{
	tutorialOpeningHand_ = cards;
	useTutorialOpeningHand_ = !cards.empty();
}

void BattleController::SetTutorialPokerRestriction(bool activateOnly, bool damageOnly) {
	tutorialActivateOnly_ = activateOnly;
	tutorialDamageOnly_ = damageOnly;
}