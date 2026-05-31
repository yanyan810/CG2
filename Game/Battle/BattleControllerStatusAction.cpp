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
void BattleController::SetPlayer(Player* player) {
	player_ = player;
}

void BattleController::SetEnemyManager(EnemyManager* enemyMgr) {
	enemyMgr_ = enemyMgr;
}

void BattleController::SpawnDamagePopup(const Vector3& pos, int damage, bool isPlayer)
{
	damagePopupUi_.SpawnDamage(pos, damage, isPlayer);
}

Vector3 BattleController::CalcStatusEffectEmitPos_(const Enemy& enemy, float heightOffset) const
{
	Vector3 emitPos = enemy.GetPos() + Vector3{ 0.0f, heightOffset, 0.0f };
	if (!cam_ || sStatusEffectCameraForwardOffset <= 0.0f) {
		return emitPos;
	}

	const Matrix4x4& cameraWorld = cam_->GetWorldMatrix();
	const Vector3 cameraPos{
		cameraWorld.m[3][0],
		cameraWorld.m[3][1],
		cameraWorld.m[3][2],
	};
	const Vector3 towardCamera = Matrix4x4::Normalize(cameraPos - emitPos);
	return emitPos + towardCamera * sStatusEffectCameraForwardOffset;
}

void BattleController::EmitPoisonAppliedEffect_(Enemy& enemy, int poisonPoint)
{
	if (!battleParticleManager_) {
		return;
	}

	if (player_) {
		player_->PlayStatusCastAnim();
	}

	const int point = std::max(1, poisonPoint);
	const uint32_t emitCount = static_cast<uint32_t>(std::clamp(8 + point * 2, 10, 36));
	Vector3 emitPos = CalcStatusEffectEmitPos_(enemy, sStatusEffectApplyHeight);
	battleParticleManager_->Emit("player_poison", emitPos, emitCount);
}

void BattleController::PlayPoisonDamageFeedback_(Enemy& enemy, int actualDamage)
{
	enemy.TriggerHitFlash(0.22f);
	enemy.PlayDamageAnim();
	if (actualDamage > 0) {
		SpawnDamagePopup(enemy.GetPos(), actualDamage, false);
	}
	BattleSfxPlayer::PlaySE("SE_PoisonDamage");
}

void BattleController::UpdatePoisonIdleEffects_(float dt)
{
	if (!battleParticleManager_ || !enemyMgr_) {
		return;
	}

	auto& enemies = enemyMgr_->GetEnemies();
	if (poisonIdleEffectTimers_.size() < enemies.size()) {
		poisonIdleEffectTimers_.resize(enemies.size(), 0.0f);
	} else if (poisonIdleEffectTimers_.size() > enemies.size()) {
		poisonIdleEffectTimers_.resize(enemies.size());
	}

	for (size_t i = 0; i < enemies.size(); ++i) {
		Enemy& enemy = enemies[i];
		if (!enemy.IsAlive() || enemy.GetBC() != Enemy::BadCondition::kPoison || enemy.GetBCPoint() <= 0) {
			poisonIdleEffectTimers_[i] = 0.0f;
			continue;
		}

		const int point = enemy.GetBCPoint();
		const float interval = std::clamp(1.15f - static_cast<float>(point) * 0.045f, 0.22f, 1.15f);
		poisonIdleEffectTimers_[i] -= dt;
		if (poisonIdleEffectTimers_[i] > 0.0f) {
			continue;
		}

		const uint32_t emitCount = static_cast<uint32_t>(std::clamp(1 + point / 4, 2, 10));
		Vector3 emitPos = CalcStatusEffectEmitPos_(enemy, sStatusEffectIdleHeight);
		battleParticleManager_->Emit("player_poison", emitPos, emitCount);
		poisonIdleEffectTimers_[i] = interval;
	}
}

void BattleController::EmitFrostAppliedEffect_(Enemy& enemy, int frostPoint, bool playCastAnim)
{
	if (!battleParticleManager_) {
		return;
	}

	if (playCastAnim && player_) {
		player_->PlayStatusCastAnim();
	}

	const int point = std::max(1, frostPoint);
	const uint32_t emitCount = static_cast<uint32_t>(std::clamp(6 + point * 2, 8, 34));
	Vector3 emitPos = CalcStatusEffectEmitPos_(enemy, sStatusEffectApplyHeight);
	battleParticleManager_->Emit("player_froze", emitPos, emitCount);
}

void BattleController::UpdateFrostIdleEffects_(float dt)
{
	if (!battleParticleManager_ || !enemyMgr_) {
		return;
	}

	auto& enemies = enemyMgr_->GetEnemies();
	if (frostIdleEffectTimers_.size() < enemies.size()) {
		frostIdleEffectTimers_.resize(enemies.size(), 0.0f);
	} else if (frostIdleEffectTimers_.size() > enemies.size()) {
		frostIdleEffectTimers_.resize(enemies.size());
	}

	for (size_t i = 0; i < enemies.size(); ++i) {
		Enemy& enemy = enemies[i];
		if (!enemy.IsAlive() || enemy.GetBC() != Enemy::BadCondition::kFrost || enemy.GetBCPoint() <= 0) {
			frostIdleEffectTimers_[i] = 0.0f;
			continue;
		}

		const int point = enemy.GetBCPoint();
		const bool burstReady = point >= std::max(1, sFrostBurstThreshold);
		const float minInterval = burstReady ? 0.14f : 0.25f;
		const float interval = std::clamp(1.25f - static_cast<float>(point) * 0.04f, minInterval, 1.25f);
		frostIdleEffectTimers_[i] -= dt;
		if (frostIdleEffectTimers_[i] > 0.0f) {
			continue;
		}

		const int baseCount = burstReady ? 4 + point / 3 : 1 + point / 4;
		const uint32_t emitCount = static_cast<uint32_t>(std::clamp(baseCount, 2, burstReady ? 18 : 9));
		Vector3 emitPos = CalcStatusEffectEmitPos_(enemy, sStatusEffectIdleHeight);
		battleParticleManager_->Emit("player_froze", emitPos, emitCount);
		frostIdleEffectTimers_[i] = interval;
	}
}

const CardDef* BattleController::GetPreviewCardDef() const
{
	if (cardState_ == CardInputState::Dragging) {
		if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(deckZone_.GetHandCount())) {
			return db_.Find(deckZone_.GetHand()[selectedIndex_].defId);
		}
	}

	if (cardState_ == CardInputState::Preview) {
		if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(deckZone_.GetHandCount())) {
			return db_.Find(deckZone_.GetHand()[selectedIndex_].defId);
		}
	}

	if (cardState_ == CardInputState::Idle) {
		int handHover = handView_.GetHoverIndex();
		if (handHover >= 0 && handHover < static_cast<int>(deckZone_.GetHandCount())) {
			return db_.Find(deckZone_.GetHand()[handHover].defId);
		}
		if (fieldReplaceHoverIndex_ >= 0 &&
			fieldReplaceHoverIndex_ < static_cast<int>(field_.size())) {
			return db_.Find(field_[fieldReplaceHoverIndex_].defId);
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
		if (handHover >= 0 && handHover < static_cast<int>(deckZone_.GetHandCount())) {
			return db_.Find(deckZone_.GetHand()[handHover].defId);
		}

		if (fieldReplaceHoverIndex_ >= 0 &&
			fieldReplaceHoverIndex_ < static_cast<int>(field_.size())) {
			return db_.Find(field_[fieldReplaceHoverIndex_].defId);
		}
	}

	return nullptr;
}


bool BattleController::ShouldShowOperationUi() const
{
	return operationUiVisible_;
}

std::wstring BattleController::GetOperationUiText() const
{
	BattleInfoTextProvider::OperationState operationState = BattleInfoTextProvider::OperationState::Basic;
	if (cardState_ == CardInputState::ChoosingFieldReplace) {
		operationState = BattleInfoTextProvider::OperationState::ChoosingFieldReplace;
	} else if (cardState_ == CardInputState::Preview) {
		operationState = BattleInfoTextProvider::OperationState::Preview;
	}

	return BattleInfoTextProvider::BuildOperationText(operationState);
}

std::wstring BattleController::GetZoneCountUiText() const
{
	return BattleInfoTextProvider::BuildZoneCountText({
		static_cast<int>(deckZone_.GetDeckCount()),
		static_cast<int>(deckZone_.GetHandCount()),
		static_cast<int>(deckZone_.GetDiscardCount()),
		static_cast<int>(field_.size()) });
}

std::wstring BattleController::GetCurrentPokerHandUiText() const
{
	PokerHandResult poker = EvaluatePokerHand_();
	return BattleInfoTextProvider::BuildCurrentPokerHandText(poker.rank, field_.size());
}

std::wstring BattleController::GetTurnUiText() const
{
	return BattleInfoTextProvider::BuildTurnText({
		turn_ == TurnState::Player,
		playerTurnCount_,
		enemyTurnCount_ });
}

std::wstring BattleController::GetEnergyText() const
{
	return BattleInfoTextProvider::BuildEnergyText(energy_, energyMax_);
}

std::vector<std::wstring> BattleController::GetEnemyHpTexts() const
{
	return BattleInfoTextProvider::BuildEnemyHpTexts(enemyMgr_);
}

std::vector<std::wstring> BattleController::GetEnemyBCTexts() const
{
	return BattleInfoTextProvider::BuildEnemyBcTexts(enemyMgr_);
}

BattleController::PokerBonus BattleController::GetCurrentPokerBonusForUi() const
{
	return GetPokerBonus_(currentPoker_.rank);
}


std::wstring BattleController::GetPlayerHpTexts() const
{
	return BattleInfoTextProvider::BuildPlayerHpText(*player_);
}

bool BattleController::IsAllEnemiesDead() const {
	auto& enemies = enemyMgr_->GetEnemies();
	for (auto& e : enemies) {
		if (e.IsAlive()) return false;
	}
	return true;
}

std::wstring BattleController::GetPlayerPowerBoostText() const
{
	const int power = player_ ? player_->GetBoostedPower() : 0;
	return std::to_wstring(power + currentTurnAtkUp_);
}

std::wstring BattleController::GetPlayerBlockText() const
{
	return BattleInfoTextProvider::BuildPlayerBlockText(*player_);
}
int BattleController::CalcTotalIncomingDamage() const {
	int total = 0;
	if (!enemyMgr_ || !player_) return 0;

	auto& enemies = enemyMgr_->GetEnemies();
	for (size_t i = 0; i < enemies.size(); ++i) {
		const auto& enemy = enemies[i];
		if (!enemy.IsAlive() || enemyActionCountSystem_.IsActedByCount(i)) {
			continue;
		}
		total += enemy.GetIncomingDamage() - player_->GetBlock();
	}
	return total;
}

void BattleController::UpdateHpGauges() {}

//=====================
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

void BattleController::SetTutorialForcedEnemyTargetCardId(int cardDefId)
{
	const int newId = cardDefId > 0 ? cardDefId : -1;
	if (tutorialForcedEnemyTargetCardId_ == newId) {
		return;
	}

	tutorialForcedEnemyTargetCardId_ = newId;
	handPreviewSignature_ = 0;
	if (!IsTutorialForcedCardActive_()) {
		handView_.ClearCardEffects();
	}
}

std::vector<std::string> BattleController::CollectEffectTypes_(const CardDef& def) const
{
	std::vector<std::string> types;
	types.reserve(def.effects.size());

	for (const auto& effect : def.effects) {
		if (!effect.type.empty()) {
			types.push_back(effect.type);
		}
	}

	return types;
}

bool BattleController::BeginCardActionSequence_(GameApp& app, const CardDef& def, const CardInstance& card, Enemy& targetEnemy)
{
	actionSequenceQueue_.clear();
	actionSequenceIndex_ = 0;
	actionSequenceTarget_ = &targetEnemy;
	actionSequenceCardDef_ = &def;
	actionSequenceCard_ = card;
	actionSequenceDamageApplied_ = false;

	if (const ActionSequenceProfile* useProfile = app.PickCardUseSequenceProfile()) {
		actionSequenceQueue_.push_back(useProfile);
	}

	return StartNextActionSequence_();
}

bool BattleController::StartNextActionSequence_()
{
	if (!actionSequenceTarget_ || actionSequenceIndex_ >= actionSequenceQueue_.size()) {
		actionSequenceQueue_.clear();
		actionSequenceIndex_ = 0;
		actionSequenceCardDef_ = nullptr;
		actionSequenceDamageApplied_ = false;
		return false;
	}

	const ActionSequenceProfile* profile = actionSequenceQueue_[actionSequenceIndex_++];
	if (!profile) {
		return StartNextActionSequence_();
	}

	actionDirector_.SetProfile(*profile);
	actionSequenceDamageApplied_ = false;
	if (actionSequenceCardDef_) {
		actionDirector_.StartAction(player_, actionSequenceTarget_, *actionSequenceCardDef_, actionSequenceCard_);
	} else {
		actionDirector_.StartAction(player_, actionSequenceTarget_);
	}
	return true;
}

bool BattleController::ApplyFrostBeforeEnemyAction_(Enemy& enemy)
{
	if (!enemy.IsAlive() ||
		enemy.GetBC() != Enemy::BadCondition::kFrost ||
		enemy.GetBCPoint() <= 0) {
		return false;
	}

	const int frostPoint = enemy.GetBCPoint();
	const bool burst = frostPoint >= std::max(1, sFrostBurstThreshold);
	const int damage = burst
		? frostPoint * std::max(1, sFrostBurstMultiplier)
		: frostPoint;
	const int actualDamage = enemy.Damage(damage);

	enemy.TriggerFrostFlash(burst ? 0.42f : 0.22f);
	enemy.PlayDamageAnim();
	if (actualDamage > 0) {
		SpawnDamagePopup(enemy.GetPos(), actualDamage, false);
	}

	if (burst) {
		EmitFrostAppliedEffect_(enemy, frostPoint, false);
		enemy.RemoveBC();
		return true;
	}

	return !enemy.IsAlive();
}

void BattleController::ExecuteEnemyAction_(Enemy& enemy, const EnemyAction& action)
{
	if (ApplyFrostBeforeEnemyAction_(enemy)) {
		return;
	}

	auto findLowestHpAlly = [&]() -> Enemy* {
		if (!enemyMgr_) {
			return nullptr;
		}

		Enemy* target = nullptr;
		for (auto& ally : enemyMgr_->GetEnemies()) {
			if (!ally.IsAlive()) {
				continue;
			}
			if (!target || ally.GetHP() < target->GetHP()) {
				target = &ally;
			}
		}
		return target;
	};

	if (action.type == "Attack") {
		if (!player_) {
			return;
		}
		enemy.PlayAttackAnim(player_->GetPos());
		const int beforeHp = player_->GetHP();
		const int beforeBlock = player_->GetBlock();
		player_->Damage(action.value);
		const int actualDamage = std::max(0, beforeHp - player_->GetHP());
		const int afterBlock = player_->GetBlock();
		BattleSfxPlayer::PlayBlockReactionSE(beforeBlock, afterBlock, actualDamage, action.value);
	} else if (action.type == "Heal") {
		const int beforeHp = enemy.GetHP();
		enemy.Heal(action.value);
		if (enemy.GetHP() > beforeHp) {
			BattleSfxPlayer::PlaySE("SE_Heal");
		}
	} else if (action.type == "HealLowestAlly") {
		Enemy* target = findLowestHpAlly();
		if (!target) {
			return;
		}
		const int beforeHp = target->GetHP();
		target->Heal(action.value);
		if (target->GetHP() > beforeHp) {
			BattleSfxPlayer::PlaySE("SE_Heal");
		}
	} else if (action.type == "HealAll") {
		if (!enemyMgr_) {
			return;
		}
		bool healedAny = false;
		for (auto& ally : enemyMgr_->GetEnemies()) {
			if (!ally.IsAlive()) {
				continue;
			}
			const int beforeHp = ally.GetHP();
			ally.Heal(action.value);
			healedAny = healedAny || ally.GetHP() > beforeHp;
		}
		if (healedAny) {
			BattleSfxPlayer::PlaySE("SE_Heal");
		}
	} else if (action.type == "Block") {
		const int beforeBlock = enemy.GetBlock();
		enemy.AddBlock(action.value);
		BattleSfxPlayer::PlayBlockGainSEIfIncreased(beforeBlock, enemy.GetBlock());
	} else if (action.type == "BlockLowestAlly") {
		Enemy* target = findLowestHpAlly();
		if (!target) {
			return;
		}
		const int beforeBlock = target->GetBlock();
		target->AddBlock(action.value);
		BattleSfxPlayer::PlayBlockGainSEIfIncreased(beforeBlock, target->GetBlock());
	} else if (action.type == "BlockAll") {
		if (!enemyMgr_) {
			return;
		}
		bool blockedAny = false;
		for (auto& ally : enemyMgr_->GetEnemies()) {
			if (!ally.IsAlive()) {
				continue;
			}
			const int beforeBlock = ally.GetBlock();
			ally.AddBlock(action.value);
			blockedAny = blockedAny || ally.GetBlock() > beforeBlock;
		}
		if (blockedAny) {
			BattleSfxPlayer::PlaySE("SE_Block");
		}
	}
}

void BattleController::OnPlayerCardUsed_()
{
	if (!enemyMgr_) {
		return;
	}

	auto& enemies = enemyMgr_->GetEnemies();
	const auto triggeredEnemies = enemyActionCountSystem_.OnPlayerCardUsed(enemies);
	for (size_t index : triggeredEnemies) {
		if (index >= enemies.size() || !enemies[index].IsAlive()) {
			continue;
		}
		ExecuteEnemyAction_(enemies[index], enemies[index].GetBossAI().GetNextAction());
		enemyActionCountSystem_.MarkActedByCount(index);
	}
}

void BattleController::ExecutePendingAttack_(Enemy& targetEnemy)
{
	if (isPokerDamageTargeting_) {
		BattleSfxPlayer::PlaySE("SE_StrongAttack");
		const int actualDamage = ApplyDamageToEnemy_(targetEnemy, pendingDamage_);
		if (pendingDamage_ > 0) {
			SpawnDamagePopup(targetEnemy.GetPos(), actualDamage, false);
		}
		TriggerSubEffectsForField_(SubEffectTrigger::OnPokerSkillActivated, currentPoker_.rank);

		lastPokerTutorialResult_ = PokerTutorialResult::Activated;

		isPokerDamageTargeting_ = false;
		tutorialLockPokerTargetingCancel_ = false;
		pendingDamage_ = 0;

		ConsumeFieldCards_();
		cardState_ = CardInputState::Idle;
		turn_ = TurnState::Enemy;
		enemyTurnCount_++;
		enemyWait_ = 1.0f;
	} else {
		int idx = pendingCardHandIndex_;
		CardInstance inst = deckZone_.GetHand()[idx];
		const CardDef* def = db_.Find(inst.defId);

		energy_ -= def->cost;
		BattleSfxPlayer::PlaySE("SE_CardPlay");
		BattleSfxPlayer::PlayAttackSEForCard(*def);
		auto usedCardView = handView_.ExtractCardAt(idx);
		deckZone_.RemoveHandAt(static_cast<std::size_t>(idx));

		handView_.Rebuild(deckZone_.GetHand());

		ApplyCardEffects_(*def, currentEnemyIndex_);

		handView_.SetFocusIndex(-1);

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
		OnPlayerCardUsed_();
	}
}

