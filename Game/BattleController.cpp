#include "BattleController.h"
#include "GameApp.h"
#include "WinApp.h"
#include <Windows.h>
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include <array>
#include <algorithm>

#include <random>

#include"Player.h"
#include"Enemy.h"

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
	Vector3 pos{ 14.0f, -9.0f, 6.0f };
	Vector3 rot{ 0.0f, 0.0f, 0.0f };
	Vector3 scl{ 1.0f, 1.0f, 1.0f };

	discardView_->SetTransform(pos, rot, scl);
}

void BattleController::ConsumeFieldCards_()
{
	for (auto& c : field_) {
		discard_.push_back(c);
	}
	field_.clear();
	fieldViews_.clear();
	RebuildDiscardView_();
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

void BattleController::Initialize(GameApp& app, Camera* camera)
{
	cam_ = camera;
	objCom_ = app.ObjCom();
	dx_ = app.Dx();

	spriteCom_ = app.SpriteCom();

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
	playerHpFg_->SetScale({ 250.0f, 18.0f, 1.0f });   // ← 最初から正しい値
	playerHpFg_->SetPosition({ 80.0f, 40.0f });       // ← 最初から正しい値

	// 敵の最大数（3体）分のゲージを生成する
	enemyHpBgs_.clear();
	enemyHpFgs_.clear();
	for (int i = 0; i < 3; ++i) {
		auto bg = std::make_unique<Sprite>();
		bg->Initialize(spriteCom_, dx_, "resources/ui/white.png");
		bg->SetColor({ 0.2f, 0.2f, 0.2f, 1.0f });           // 暗いグレー
		enemyHpBgs_.push_back(std::move(bg));

		auto fg = std::make_unique<Sprite>();
		fg->Initialize(spriteCom_, dx_, "resources/ui/white.png");
		fg->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f });           // 赤色
		enemyHpFgs_.push_back(std::move(fg));

		// 予告アイコンの生成
		auto icon = std::make_unique<Sprite>();
		icon->Initialize(spriteCom_, dx_,"resources/ui/white.png");
		enemyIntentIcons_.push_back(std::move(icon));
	}


	// -----------------------------
	// ここで一度だけ即時反映
	// -----------------------------
	Matrix4x4 viewMat = Matrix4x4::MakeIdentity4x4();
	float width = (float)WinApp::kClientWidth;
	float height = (float)WinApp::kClientHeight;
	Matrix4x4 projMat = Matrix4x4::MakeOrthographicMatrix(width, height);

	if (playerHpBg_) playerHpBg_->Update(viewMat, projMat);
	if (playerHpFg_) playerHpFg_->Update(viewMat, projMat);

	if (!db_.LoadFromJson("resources/cards/cards.json")) {
		db_.BuildSample();
	}

	DeckDef deckDef{};
	std::string err;

	if (DeckLoader::LoadFromJson("resources/cards/deck/deck.json", deckDef) &&
		DeckLoader::ValidateDeck(deckDef, db_, err)) {

		deck_.clear();
		for (const auto& e : deckDef.cards) {
			for (int i = 0; i < e.count; ++i) {
				deck_.push_back(MakeCardInstance(e.id));
			}
		}
	} else {
		deck_.clear();
		for (int i = 0; i < 4; ++i) {

			deck_.push_back(MakeCardInstance(9));
			deck_.push_back(MakeCardInstance(8));
			deck_.push_back(MakeCardInstance(7));
			deck_.push_back(MakeCardInstance(6));
			deck_.push_back(MakeCardInstance(5));
			deck_.push_back(MakeCardInstance(4));
			deck_.push_back(MakeCardInstance(3));
			deck_.push_back(MakeCardInstance(2));
			deck_.push_back(MakeCardInstance(1));
			deck_.push_back(MakeCardInstance(20));
			deck_.push_back(MakeCardInstance(19));
			deck_.push_back(MakeCardInstance(18));
			deck_.push_back(MakeCardInstance(17));
			deck_.push_back(MakeCardInstance(16));
			deck_.push_back(MakeCardInstance(15));
			deck_.push_back(MakeCardInstance(14));
			deck_.push_back(MakeCardInstance(13));
			deck_.push_back(MakeCardInstance(12));
			deck_.push_back(MakeCardInstance(11));
			deck_.push_back(MakeCardInstance(10));
		}
	}

	ShuffleDeck_();

	hand_.clear();
	discard_.clear();
	field_.clear();
	fieldViews_.clear();

	hasPendingCard_ = false;
	pendingCard_ = {};

	energy_ = energyMax_;
	if (enemy_) {
		enemy_->GetBossAI().LoadPattern("resources/cards/Boos.json");
	}
	handView_.Initialize(objCom_, dx_, cam_, &db_);
	handView_.Rebuild(hand_);

	StartPlayerTurn_();
	OutputDebugStringA(("After StartPlayerTurn hand=" + std::to_string(hand_.size()) +
		" deck=" + std::to_string(deck_.size()) +
		" discard=" + std::to_string(discard_.size()) + "\n").c_str());

	RebuildDiscardView_();

	RebuildCostView_(deltaTime_);

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
	return true;
}

void BattleController::DrawUntilFive_()
{
	while ((int)hand_.size() < 5) {
		if (!DrawOne_()) break;
	}
	handView_.Rebuild(hand_);
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

void BattleController::ApplyEffectsList_(const std::vector<CardEffectDef>& effects)
{
	for (const auto& effect : effects) {
		if (effect.type == "Draw") {
			DrawCards_(effect.value);

		} else if (effect.type == "Damage") {
			if (player_ && enemy_) {
				// プレイヤーが敵の位置に向かって突進！
				player_->PlayAttackAnim(enemy_->GetPos());

				// 敵が赤く光ってのけぞる！
				enemy_->TriggerHitFlash(0.2f);
				enemy_->PlayDamageAnim();
			}
			if (enemy_) {
				enemy_->Damage(effect.value);
			} else {
				enemyHp_ -= effect.value;
				if (enemyHp_ < 0) {
					enemyHp_ = 0;
				}
			}

		} else if (effect.type == "Block") {
			// 例: playerBlock_ += effect.value;

		} else if (effect.type == "NextTurnAtkUp") {
			nextTurnAtkUp_ += effect.value;

		} else if (effect.type == "Heal") {
			if (player_) {
				player_->Heal(effect.value);
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

void BattleController::ApplyCardEffects_(const CardDef& def)
{
	ApplyEffectsList_(def.effects);
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

			pokerChoiceState_ = PokerChoiceState::WaitingActivateChoice;
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

			ApplyEffectsList_(sub.effects);
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

		ApplyEffectsList_(sub.effects);
	}
}

void BattleController::RebuildFieldView_()
{
	fieldViews_.clear();
	fieldViews_.reserve(field_.size());

	const int n = (int)field_.size();
	if (n <= 0) {
		return;
	}

	const float y = -5.0f;
	const float z = 5.0f;
	const float gap = 5.0f;
	const float startX = -gap * 0.5f * (n - 1);

	for (int i = 0; i < n; ++i) {
		const CardDef* def = db_.Find(field_[i].defId);
		if (!def) {
			continue;
		}

		auto card = std::make_unique<Card3D>();
		card->Initialize(objCom_, dx_, cam_, *def, field_[i]);

		Vector3 pos{ startX + gap * i, y, z };
		Vector3 rot{ 0.0f, 0.0f, 0.0f };
		Vector3 scl{ 1.15f, 1.15f, 1.15f };

		card->SetTransform(pos, rot, scl);
		fieldViews_.push_back(std::move(card));
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

void BattleController::Update(GameApp& app, float dt)
{

	bool yNow = (GetAsyncKeyState('Y') & 0x8000) != 0;
	bool nNow = (GetAsyncKeyState('N') & 0x8000) != 0;
	bool key1Now = (GetAsyncKeyState('1') & 0x8000) != 0;
	bool key2Now = (GetAsyncKeyState('2') & 0x8000) != 0;
	bool key3Now = (GetAsyncKeyState('3') & 0x8000) != 0;

	bool yTrig = yNow && !prevY_;
	bool nTrig = nNow && !prevN_;
	bool key1Trig = key1Now && !prev1_;
	bool key2Trig = key2Now && !prev2_;
	bool key3Trig = key3Now && !prev3_;

	prevY_ = yNow;
	prevN_ = nNow;
	prev1_ = key1Now;
	prev2_ = key2Now;
	prev3_ = key3Now;

	if (pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice)
	{
		if (yTrig) {
			TriggerSubEffectsForField_(
				SubEffectTrigger::OnPokerSkillActivated,
				currentPoker_.rank
			);
			pokerChoiceState_ = PokerChoiceState::WaitingEffectChoice;
		}

		if (nTrig) {
			pokerChoiceState_ = PokerChoiceState::None;
		}

		return;
	}

	if (pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice)
	{
		PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);

		if (key1Trig) {
			nextTurnAtkUp_ += bonus.atkUp;
			ConsumeFieldCards_();
			pokerChoiceState_ = PokerChoiceState::None;
			turn_ = TurnState::Enemy;
			enemyTurnCount_++;
			enemyWait_ = 1.0f;
			return;
		}

		if (key2Trig) {
			DrawCards_(bonus.drawCount);
			ConsumeFieldCards_();
			pokerChoiceState_ = PokerChoiceState::None;
			turn_ = TurnState::Enemy;
			enemyTurnCount_++;
			enemyWait_ = 1.0f;
			return;
		}

		if (key3Trig) {
			if (player_ && enemy_) {
				player_->PlayAttackAnim(enemy_->GetPos());
				enemy_->TriggerHitFlash(0.2f);
				enemy_->PlayDamageAnim();
			}
			enemyHp_ -= bonus.damage;
			if (enemyHp_ < 0) {
				enemyHp_ = 0;
			}

			ConsumeFieldCards_();
			pokerChoiceState_ = PokerChoiceState::None;
			turn_ = TurnState::Enemy;
			enemyTurnCount_++;
			enemyWait_ = 1.0f;
			return;
		}

		if (nTrig) {
			pokerChoiceState_ = PokerChoiceState::WaitingActivateChoice;
			return;
		}

		return;
	}

	bool enterNow = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
	bool enterTrig = enterNow && !prevEnter_;

	bool tabNow = (GetAsyncKeyState(VK_TAB) & 0x8000) != 0;
	operationUiVisible_ = tabNow;

	prevEnter_ = enterNow;

	POINT mouse{};
	GetCursorPos(&mouse);
	ScreenToClient(app.Win()->GetHwnd(), &mouse);

	if (turn_ == TurnState::Player) {

		if (cardState_ != CardInputState::ChoosingFieldReplace) {
			fieldReplaceHoverIndex_ = -1;
		}

		int hover = handView_.PickIndexByMouse(
			mouse.x, mouse.y,
			cam_->GetViewProjectionMatrix(),
			(float)WinApp::kClientWidth, (float)WinApp::kClientHeight
		);
		handView_.SetHoverIndex(hover);
	} else {
		handView_.SetHoverIndex(-1);
	}

	bool lNow = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	bool lTrig = lNow && !prevL_;
	bool lRel = !lNow && prevL_;
	prevL_ = lNow;

	bool rNow = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
	bool rTrig = rNow && !prevR_;
	prevR_ = rNow;

	if (turn_ == TurnState::Player) {

		if (cardState_ != CardInputState::ChoosingFieldReplace) {
			fieldReplaceHoverIndex_ = -1;
		}

		if (enterTrig && cardState_ == CardInputState::Idle) {

			OutputDebugStringA(("Before EndTurn hand=" + std::to_string(hand_.size()) +
				" deck=" + std::to_string(deck_.size()) +
				" discard=" + std::to_string(discard_.size()) +
				" field=" + std::to_string(field_.size()) + "\n").c_str());

			turn_ = TurnState::Enemy;
			hasPendingCard_ = false;
			pendingCard_ = {};
			enemyWait_ = 1.0f;
			handView_.SetHoverIndex(-1);
			handView_.SetDrag(-1, 0, 0, false);
			handView_.SetPreviewIndex(-1);
			selectedIndex_ = -1;
			cardState_ = CardInputState::Idle;
		} else {
			if (cardState_ != CardInputState::Preview) {
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
							energy_ -= def->cost;

							hand_.erase(hand_.begin() + idx);
							handView_.Rebuild(hand_);

							ApplyCardEffects_(*def);

							if ((int)field_.size() < 5) {
								field_.push_back(inst);
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
				fieldReplaceHoverIndex_ = PickFieldIndexByMouse_(mouse.x, mouse.y);

				if (lTrig) {
					int replaceIndex = fieldReplaceHoverIndex_;
					if (replaceIndex >= 0 && replaceIndex < (int)field_.size() && hasPendingCard_) {
						discard_.push_back(field_[replaceIndex]);
						field_[replaceIndex] = pendingCard_;
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
						cardState_ = CardInputState::Idle;
					}
				}

				if (rTrig) {
					if (hasPendingCard_) {
						discard_.push_back(pendingCard_);
					}
					hasPendingCard_ = false;
					pendingCard_ = {};
					fieldReplaceHoverIndex_ = -1;
					cardState_ = CardInputState::Idle;
					RebuildDiscardView_();
				}

				handView_.SetHoverIndex(-1);
				handView_.SetDrag(-1, 0, 0, false);
				handView_.SetPreviewIndex(-1);
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

		hasPendingCard_ = false;
		pendingCard_ = {};

		enemyWait_ -= dt;

		// ★変更：待機時間が0になったら、敵が行動を実行してからプレイヤーのターンへ！
		if (enemyWait_ <= 0.0f) {

			// AIからランダムな行動を取得して実行
			if (enemy_ && player_) {
				EnemyAction action = enemy_->GetBossAI().GetRandomAction();


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
					} else if (action.type == "Heal") {
						e.Heal(action.value);
					} else if (action.type == "Block") {
						// 防御処理
					}

					enemyWait_ = 1.0f;


					player_->Damage(action.value);

				} else if (action.type == "Heal") {
					// ボスの回復！
					enemy_->Heal(action.value);

				} else if (action.type == "Block") {
					// ボスの防御（必要に応じて処理を追加）
				}
			}

			// プレイヤーターンへ移行
			turn_ = TurnState::Player;
			StartPlayerTurn_();

			OutputDebugStringA(("After StartPlayerTurn hand=" + std::to_string(hand_.size()) +
				" deck=" + std::to_string(deck_.size()) +
				" discard=" + std::to_string(discard_.size()) + "\n").c_str());

		}
	}

	handView_.Update(dt);

	//for (auto& c : fieldViews_) {
	//	c->Update(dt);
	//}

	const int fieldCount = (int)fieldViews_.size();
	if (fieldCount > 0) {
		const float y = -5.0f;
		const float z = 5.0f;
		const float gap = 5.0f;
		const float startX = -gap * 0.5f * (fieldCount - 1);

		for (int i = 0; i < fieldCount; ++i) {
			Vector3 pos{ startX + gap * i, y, z };
			Vector3 rot{ 0.0f, 0.0f, 0.0f };
			Vector3 scl{ 1.15f, 1.15f, 1.15f };

			if (cardState_ == CardInputState::ChoosingFieldReplace &&
				i == fieldReplaceHoverIndex_) {
				pos.y += 0.35f;
				pos.z -= 0.25f;
				scl = { 1.28f, 1.28f, 1.28f };
			}

			fieldViews_[i]->SetTransform(pos, rot, scl);
			fieldViews_[i]->Update(dt);
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

		playerHpFg_->SetScale({ 250.0f * hpRatio, 18.0f, 1.0f });
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

	if (energy_ != prevEnergy_ || energyMax_ != prevEnergyMax_) {
		RebuildCostView_(dt);
	} else {
		UpdateCostViewTransform_(dt);
	}

	// --------------------------------------------------
	// スプライトの更新
	// --------------------------------------------------
	if (playerHpBg_) playerHpBg_->Update(viewMat, projMat);
	if (playerHpFg_) playerHpFg_->Update(viewMat, projMat);
	for (auto& bg : enemyHpBgs_) { if (bg) bg->Update(viewMat, projMat); }
	for (auto& fg : enemyHpFgs_) { if (fg) fg->Update(viewMat, projMat); }
	for (auto& icon : enemyIntentIcons_) { if (icon) icon->Update(viewMat, projMat); }


}

void BattleController::Draw3D(GameApp& app)
{
	for (auto& c : fieldViews_) {
		c->Draw();
	}

	if (discardView_) {
		discardView_->Draw();
	}

	handView_.Draw();

	for (auto& obj : costDigitModels_) {
		obj->Draw();
	}

	if (playerHpBg_) playerHpBg_->Draw();
	if (playerHpFg_) playerHpFg_->Draw();


	for (auto& bg : enemyHpBgs_) { if (bg) bg->Draw(); }
	for (auto& fg : enemyHpFgs_) { if (fg) fg->Draw(); }
	for (auto& icon : enemyIntentIcons_) { if (icon) icon->Draw(); }

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

	ImGui::Text("Player Hp: %d", player_->GetHP());
	ImGui::Text("Enemy  Hp: %d", enemy_->GetHP());

}
#endif

void BattleController::SetPlayer(Player* player) {
	player_ = player;
}

void BattleController::SetEnemy(Enemy* enemy) {
	enemy_ = enemy;
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

	return nullptr;
}

bool BattleController::HasPokerChoiceUi() const
{
	return pokerChoiceState_ != PokerChoiceState::None;
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
		text += L"Y : 発動する\n";
		text += L"N : スキップ\n";
		return text;
	}

	if (pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice) {
		PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);

		std::wstring text = L"";
		text += L"発動する効果を選んでください\n";
		text += L"1 : 次ターンATK UP (+" + std::to_wstring(bonus.atkUp) + L")\n";
		text += L"2 : " + std::to_wstring(bonus.drawCount) + L"枚ドロー\n";
		text += L"3 : " + std::to_wstring(bonus.damage) + L"ダメージ\n";
		text += L"N : 戻る\n";
		return text;
	}

	return L"";
}

bool BattleController::ShouldShowOperationUi() const
{
	if (HasPokerChoiceUi()) {
		return false;
	}

	if (cardState_ == CardInputState::ChoosingFieldReplace) {
		return true;
	}

	if (!operationUiVisible_) {
		return false;
	}

	if (GetPreviewCardDef() != nullptr) {
		return false;
	}

	return true;
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
		return L"役: 判定中";
	}

	if (poker.rank == PokerHandRank::None) {
		return L"役: なし";
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

	

	return text;
}