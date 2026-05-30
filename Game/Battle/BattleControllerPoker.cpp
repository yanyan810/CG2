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
		if (sPlayerBlockCarryOverEnabled) {
			player_->DecayBlock(sPlayerBlockTurnDecayRate);
		} else {
			player_->ResetBlock();
		}
		player_->ResetVampireHeal();
		player_->ResetPowerBoost();
		player_->SetPowerBoostEffectBonus(currentTurnAtkUp_);
	}
	playerTurnCount_++;

	energy_ = energyMax_;
	DrawTurnStartCards_();

	if (!field_.empty()) {
		RebuildFieldView_();
	}

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
			BattleSfxPlayer::PlaySE("SE_Pop");
		}
	}
	if (enemyMgr_) {
		auto& enemies = enemyMgr_->GetEnemies();
		for (auto& enemy : enemies) {
			if (enemy.IsAlive()) {
				enemy.GetBossAI().DecideNextAction();
			}
		}
		enemyActionCountSystem_.StartPlayerTurn(enemies);
	} else {
		enemyActionCountSystem_.Clear();
	}
}

BattleController::PokerBonus BattleController::GetPokerBonus_(PokerHandRank rank) const
{
	PokerBonus b{};

	switch (rank) {
	case PokerHandRank::OnePair:
		b.atkUp = 3;
		b.drawCount = 2;
		b.damage = 55;
		break;

	case PokerHandRank::TwoPair:
		b.atkUp = 5;
		b.drawCount = 3;
		b.damage = 65;
		break;

	case PokerHandRank::ThreeOfAKind:
		b.atkUp = 10;
		b.drawCount = 3;
		b.damage = 80;
		break;

	case PokerHandRank::Straight:
		b.atkUp = 15;
		b.drawCount = 4;
		b.damage = 95;
		break;

	case PokerHandRank::Flush:
		b.atkUp = 20;
		b.drawCount = 4;
		b.damage = 105;
		break;

	case PokerHandRank::FullHouse:
		b.atkUp = 25;
		b.drawCount = 5;
		b.damage = 120;
		break;

	case PokerHandRank::FourOfAKind:
		b.atkUp = 30;
		b.drawCount = 5;
		b.damage = 135;
		break;

	case PokerHandRank::StraightFlush:
		b.atkUp = 40;
		b.drawCount = 6;
		b.damage = 160;
		break;

	case PokerHandRank::RoyalStraightFlush:
		b.atkUp = 60;
		b.drawCount = 7;
		b.damage = 200;
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
	return CardEffectTextBuilder::GetSubEffectTriggerText(trigger);
}

std::wstring BattleController::GetSubEffectConditionText_(const CardSubEffectDef& sub) const
{
	return CardEffectTextBuilder::GetSubEffectConditionText(sub);
}

std::wstring BattleController::GetEffectValueText_(const CardEffectDef& effect) const
{
	if (!effect.valueText.empty()) {
		return Utf8ToWString(effect.valueText) + L": " + FormatEffectValue_(effect);
	}

	if (effect.type == "Draw") {
		return L"ドロー: " + FormatEffectValue_(effect);
	}
	if (effect.type == "Damage") {
		return L"ダメージ: " + FormatEffectValue_(effect);
	}
	if (effect.type == "DamageAll") {
		return L"全体ダメージ: " + FormatEffectValue_(effect);
	}
	if (effect.type == "Heal") {
		return L"回復: " + FormatEffectValue_(effect);
	}
	if (effect.type == "Block") {
		return L"ブロック: " + FormatEffectValue_(effect);
	}
	if (effect.type == "PowerBoost") {
		return L"パワー: " + FormatEffectValue_(effect);
	}
	if (effect.type == "EnergyCharge") {
		return L"コスト回復: " + FormatEffectValue_(effect);
	}
	if (effect.type == "NextTurnAtkUp") {
		return L"次ターンATK UP: " + FormatEffectValue_(effect);
	}
	if (effect.type == "SelfDamage") {
		return L"自傷: " + FormatEffectValue_(effect);
	}

	return Utf8ToWString(effect.type) + L": " + FormatEffectValue_(effect);
}

std::wstring BattleController::GetBaseEffectSummaryText_(const CardDef& def) const
{
	return CardEffectTextBuilder::GetBaseEffectSummaryText(def);
}

std::wstring BattleController::GetPreviewCardDetailText() const
{
	return BattleInfoTextProvider::BuildPreviewCardDetailText(GetPreviewCardDef());
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
					line += L"カードを" + FormatEffectValue_(effect) + L"枚引く";
				} else if (effect.type == "Damage") {
					line += L"敵単体に" + FormatEffectValue_(effect) + L"ダメージ";
				} else if (effect.type == "DamageAll") {
					line += L"敵全体に" + FormatEffectValue_(effect) + L"ダメージ";
				} else if (effect.type == "Heal") {
					line += L"体力を" + FormatEffectValue_(effect) + L"回復";
				} else if (effect.type == "Block") {
					line += L"ブロックを" + FormatEffectValue_(effect) + L"獲得";
				} else if (effect.type == "PowerBoost") {
					line += L"パワーを" + FormatEffectValue_(effect) + L"獲得";
				} else if (effect.type == "EnergyCharge") {
					line += L"コストを" + FormatEffectValue_(effect) + L"回復";
				} else {
					line += Utf8ToWString(effect.type) + L" : " + FormatEffectValue_(effect);
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
	const PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);

	auto turnStartLines = CollectSubEffectPreviewLines_(
		SubEffectTrigger::OnTurnStartWithPoker,
		currentPoker_.rank
	);

	auto pokerActivatedLines = CollectSubEffectPreviewLines_(
		SubEffectTrigger::OnPokerSkillActivated,
		currentPoker_.rank
	);

	return BattleInfoTextProvider::BuildPokerEffectPreviewText(
		{ bonus.atkUp, bonus.drawCount, bonus.damage },
		turnStartLines,
		pokerActivatedLines);
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


void BattleController::HandlePokerActivateChoice_(FieldUi& fieldUi, POINT mouse, bool lTrig, bool yTrig, bool nTrig)
{
	if (pokerChoiceJustOpened_) {
		pokerChoiceJustOpened_ = false;
		return;
	}

	const auto& layout = fieldUi.GetPokerEffectChoiceLayout();
	const auto decision = PokerChoiceController::ResolveActivateChoice(
		layout,
		mouse.x,
		mouse.y,
		lTrig,
		yTrig,
		nTrig,
		tutorialActivateOnly_);
	const auto& hover = decision.hover;

	pokerQuickPreviewVisible_ = hover.infoHovered;

	if (hover.infoHovered) {
		pokerMouseChoice_ = PokerMouseChoice::None;
	} else if (hover.choice != PokerChoiceController::Choice::None) {
		pokerMouseChoice_ = ToPokerMouseChoice_(hover.choice);
	}

	switch (decision.action) {
	case PokerChoiceController::ActivateAction::ToggleInfo:
		return;

	case PokerChoiceController::ActivateAction::Activate:
		pokerQuickPreviewVisible_ = false;
		pokerChoiceState_ = PokerChoiceState::WaitingEffectChoice;
		pokerChoiceJustOpened_ = true;
		return;

	case PokerChoiceController::ActivateAction::Skip:
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Skipped;
		pokerChoiceState_ = PokerChoiceState::None;
		return;

	case PokerChoiceController::ActivateAction::ViewBoard:
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

	case PokerChoiceController::ActivateAction::None:
	default:
		break;
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

	pokerMouseChoice_ = PokerMouseChoice::None;
	const auto decision = PokerChoiceController::ResolveEffectChoice(
		layout,
		mouse.x,
		mouse.y,
		lTrig,
		nTrig,
		tutorialDamageOnly_);
	const auto& hover = decision.hover;

	pokerQuickPreviewVisible_ = hover.infoHovered;

	if (hover.infoHovered) {
		pokerMouseChoice_ = PokerMouseChoice::None;
	} else {
		pokerMouseChoice_ = ToPokerMouseChoice_(hover.choice);
	}

	switch (decision.action) {
	case PokerChoiceController::EffectAction::ToggleInfo:
		return;

	case PokerChoiceController::EffectAction::AtkUp:
		nextTurnAtkUp_ += bonus.atkUp;
		TriggerSubEffectsForField_(SubEffectTrigger::OnPokerSkillActivated, currentPoker_.rank);
		ConsumeFieldCards_();
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Activated;
		pokerChoiceState_ = PokerChoiceState::None;
		turn_ = TurnState::Enemy;
		enemyWait_ = 1.0f;
		return;

	case PokerChoiceController::EffectAction::Draw:
		DrawCards_(bonus.drawCount);
		TriggerSubEffectsForField_(SubEffectTrigger::OnPokerSkillActivated, currentPoker_.rank);
		ConsumeFieldCards_();
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::Activated;
		pokerChoiceState_ = PokerChoiceState::None;
		turn_ = TurnState::Enemy;
		enemyWait_ = 1.0f;
		return;

	case PokerChoiceController::EffectAction::Damage:
		pendingDamage_ = CalcFinalAttackDamage_(bonus.damage);
		isPokerDamageTargeting_ = true;
		if (tutorialDamageOnly_) {
			tutorialLockPokerTargetingCancel_ = true;
		}
		pokerQuickPreviewVisible_ = false;
		lastPokerTutorialResult_ = PokerTutorialResult::None;

		cardState_ = CardInputState::ChoosingEnemyTarget;
		pokerChoiceState_ = PokerChoiceState::None;
		return;

	case PokerChoiceController::EffectAction::Back:
		pokerQuickPreviewVisible_ = false;
		pokerChoiceState_ = PokerChoiceState::WaitingActivateChoice;
		return;

	case PokerChoiceController::EffectAction::ViewBoard:
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

	case PokerChoiceController::EffectAction::None:
	default:
		break;
	}

	if (tutorialDamageOnly_) {
		return;
	}
}

void BattleController::HandlePokerViewBoard_(FieldUi& fieldUi, POINT mouse, bool lTrig, float dt)
{
	handView_.SetPreviewIndex(-1);
	handView_.SetDrag(-1, 0.0f, 0.0f, false);
	handView_.SetFocusIndex(-1);

	const auto& layout = fieldUi.GetPokerEffectChoiceLayout();
	pokerMouseChoice_ = ToPokerMouseChoice_(
		PokerChoiceController::ResolveViewBoardHover(layout, mouse.x, mouse.y));

	int hover = BattleCardInputController::PickHandIndexByMouse(handView_, cam_->GetViewProjectionMatrix(), mouse.x, mouse.y, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight);
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


bool BattleController::HasPokerChoiceUi() const
{
	return PokerChoiceQuery::HasChoiceUi(pokerChoiceState_);
}

std::wstring BattleController::GetPokerChoiceUiText() const
{
	const PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);
	BattleInfoTextProvider::PokerChoiceState choiceState = BattleInfoTextProvider::PokerChoiceState::None;
	switch (pokerChoiceState_) {
	case PokerChoiceState::WaitingActivateChoice:
		choiceState = BattleInfoTextProvider::PokerChoiceState::WaitingActivateChoice;
		break;
	case PokerChoiceState::WaitingEffectChoice:
		choiceState = BattleInfoTextProvider::PokerChoiceState::WaitingEffectChoice;
		break;
	case PokerChoiceState::ViewingBoardFromPokerUi:
		choiceState = BattleInfoTextProvider::PokerChoiceState::ViewingBoard;
		break;
	case PokerChoiceState::None:
	default:
		break;
	}
	return BattleInfoTextProvider::BuildPokerChoiceUiText(
		choiceState,
		currentPoker_.rank,
		{ bonus.atkUp, bonus.drawCount, bonus.damage });
}

int BattleController::GetPokerMouseChoiceIndex() const
{
	return PokerChoiceQuery::GetMouseChoiceIndex(pokerMouseChoice_);
}

bool BattleController::IsWaitingActivateChoice() const
{
	return PokerChoiceQuery::IsWaitingActivate(pokerChoiceState_);
}

bool BattleController::IsWaitingEffectChoice() const
{
	return PokerChoiceQuery::IsWaitingEffect(pokerChoiceState_);
}

bool BattleController::IsViewingBoardFromPokerUi() const
{
	return PokerChoiceQuery::IsViewingBoard(pokerChoiceState_);
}

