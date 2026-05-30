#include "BattleController.h"
#include "Battle/BattleControllerShared.h"
#include "Battle/BattleFieldViewController.h"
#include "Battle/BattleDebugImGui.h"
#include "Battle/BattleRenderView.h"
#include "Battle/BattleInfoTextProvider.h"
#include "Battle/BattleCardInputController.h"
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

#include "Player.h"
#include "Enemy.h"

#include "Card/CardEffectTextBuilder.h"
#include "Card/CardEffectExecutor.h"
#include "FieldUi.h"
#include "Audio/BattleSfxPlayer.h"
#include "Camera.h"
#include "ModelParticleManager.h"
#include "Poker/PokerChoiceQuery.h"
#include "Poker/PokerChoiceController.h"
#include "Poker/PokerChoiceTextBuilder.h"

using namespace BattleControllerDetail;
const char* BattleController::GetPokerHandName_(PokerHandRank rank) const
{
	return PokerHandEvaluator::GetHandName(rank);
}

BattleController::PokerHandResult BattleController::EvaluatePokerHand_() const
{
	return PokerHandEvaluator::Evaluate(field_);
}

BattleController::PokerHandResult BattleController::EvaluatePokerHandForCards_(const std::vector<CardInstance>& cards) const
{
	return PokerHandEvaluator::Evaluate(cards);
}

void BattleController::RebuildDiscardView_()
{
	discardView_.reset();

	const auto& discard = deckZone_.GetDiscard();
	if (discard.empty()) {
		return;
	}

	const CardInstance& top = discard.back();
	const CardDef* def = db_.Find(top.defId);
	if (!def) {
		return;
	}

	discardView_ = std::make_unique<Card3D>();
	discardView_->Initialize(objCom_, dx_, cam_, *def, top);

	Vector3 pos{ 16.0f, -8.0f, 6.0f };
	Vector3 rot{ 0.0f, 0.0f, 1.5707963f };
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

	deckZone_.AddManyToDiscard(field_);
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

	const float baseX = -14.5f;
	const float baseY = -6.8f;
	const float baseZ = 6.0f;

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
	deckZone_.ShuffleDeck();
}


bool BattleController::DrawOne_()
{
	CardInstance card{};
	bool reshuffledDiscard = false;
	if (!deckZone_.DrawOne(card, reshuffledDiscard)) {
		return false;
	}

	if (reshuffledDiscard) {
		RebuildDiscardView_();
	}

	handView_.AddCard(card);

	return true;
}

void BattleController::DrawTurnStartCards_()
{
	const int drawCount = (playerTurnCount_ == 1) ? 5 : 3;
	for (int i = 0; i < drawCount; ++i) {
		if (!DrawOne_()) {
			break;
		}
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


