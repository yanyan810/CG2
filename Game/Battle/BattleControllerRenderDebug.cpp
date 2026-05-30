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
void BattleController::Draw3D(GameApp& app)
{
	DrawDamagePopups3D(app);
	DrawCardArea3D(app);
	DrawField3D(app);
	DrawBattleOverlay3D(app);
}

void BattleController::DrawDamagePopups3D(GameApp& app)
{
	(void)app;
	BattleRenderView::DrawDamagePopups3D(actionDirector_, damagePopupUi_);
}

void BattleController::DrawPlayerBattleStatusUI(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj)
{
	(void)app;
	BattleRenderView::DrawPlayerBattleStatusUI(
		playerStatusUi_,
		GetPlayerHpTexts(),
		GetPlayerBlockText(),
		GetPlayerPowerBoostText(),
		view,
		proj);
}

void BattleController::DrawEnemyBattleStatusHpTexts(const Matrix4x4& view, const Matrix4x4& proj)
{
	BattleRenderView::DrawEnemyBattleStatusHpTexts(enemyStatusUi_, view, proj);
}

void BattleController::DrawEnemyBattleStatusBcTexts(const Matrix4x4& view, const Matrix4x4& proj)
{
	BattleRenderView::DrawEnemyBattleStatusBcTexts(enemyStatusUi_, view, proj);
}

void BattleController::DrawCardArea3D(GameApp& app)
{
	(void)app;
	BattleRenderView::CardAreaContext context{};
	context.discardView = discardView_.get();
	context.handView = &handView_;
	context.damagePopupUi = &damagePopupUi_;
	context.choosingFieldReplace = cardState_ == CardInputState::ChoosingFieldReplace;
	context.highlightFilter = highlightFilter_.get();
	context.pendingCardView = pendingCardView_.get();
	context.fieldViews = &fieldViews_;
	BattleRenderView::DrawCardArea3D(context);
}

void BattleController::DrawField3D(GameApp& app)
{
	(void)app;
	BattleRenderView::DrawField3D(propManager_.get());
}

void BattleController::DrawBattleOverlay3D(GameApp& app)
{
	BloomParam targetParam = MakeEnemyTargetBloomParam_(app.ObjectPost()->GetParam(), sPokerGlowRainbowTime);
	BattleRenderView::BattleOverlayContext context{};
	context.app = &app;
	context.highlightFilter = highlightFilter_.get();
	context.enemyMgr = enemyMgr_;
	context.choosingEnemyTarget = cardState_ == CardInputState::ChoosingEnemyTarget;
	context.enemyTargetBloomEnabled = sEnemyTargetBloomEnabled;
	context.enemyTargetBloomParam = &targetParam;
	BattleRenderView::DrawBattleOverlay3D(context);
}

void BattleController::DrawPostEffect3D(GameApp& app)
{
	BattleRenderView::DrawPostEffect3D(handView_, app);
}

void BattleController::DrawFieldFrameBloom(GameApp& app)
{
	BattleRenderView::FieldFrameBloomContext context{};
	context.app = &app;
	context.fieldViews = &fieldViews_;
	context.handView = &handView_;
	context.fieldReplacePreviewActive = &fieldReplacePreviewActive_;
	context.handPreviewRanks = &handPreviewRanks_;
	context.pokerHighlightMask = GetPokerHighlightMask_();
	context.currentPokerRank = currentPoker_.rank;
	context.inReplacePreview = cardState_ == CardInputState::ChoosingFieldReplace && hasPendingCard_;
	context.enabled = sFieldFrameBloomEnabled;
	context.threshold = sFieldFrameBloomThreshold;
	context.intensity = sFieldFrameBloomIntensity;
	context.handIntensity = sHandFrameBloomIntensity;
	context.minPulse = sFieldFrameBloomMinPulse;
	context.chromAb = sFieldFrameBloomChromAb;
	context.time = sPokerGlowRainbowTime;
	BattleRenderView::DrawFieldFrameBloom(context);
}
void BattleController::DrawPreviewCard3D(GameApp& app) {
	app.ObjCom()->SetGraphicsPipelineState();
	if (cardState_ == CardInputState::Preview && pendingCardView_) {
		pendingCardView_->Draw();
	}
	handView_.DrawPreviewCard();
}

void BattleController::Draw2D(GameApp& app)
{
	BattleRenderView::Draw2DContext context{};
	context.app = &app;
	context.actionDirector = &actionDirector_;
	context.playerStatusUi = &playerStatusUi_;
	context.enemyStatusUi = &enemyStatusUi_;
	context.player = player_;
	context.enemyMgr = enemyMgr_;
	context.enemyActedFlags = &enemyActionCountSystem_.GetActedFlags();
	context.incomingDamage = std::max(0, CalcTotalIncomingDamage());
	context.time = sPokerGlowRainbowTime;
	context.hpDamageBlinkSpeed = sHpDamageBlinkSpeed;
	context.hpGaugeBloomEnabled = sHpGaugeBloomEnabled;
	context.hpGaugeBloomIntensity = sHpGaugeBloomIntensity;
	context.hpDamageBloomIntensity = sHpDamageBloomIntensity;
	context.enemyIntentBloomEnabled = sEnemyIntentBloomEnabled;
	context.enemyIntentBloomIntensity = sEnemyIntentBloomIntensity;
	context.enemyIntentBloomMinPulse = sEnemyIntentBloomMinPulse;
	BattleRenderView::Draw2D(context);
}

void BattleController::DrawHpGaugeBloom_(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj)
{
	BattleRenderView::Draw2DContext context{};
	context.app = &app;
	context.playerStatusUi = &playerStatusUi_;
	context.enemyStatusUi = &enemyStatusUi_;
	context.player = player_;
	context.enemyMgr = enemyMgr_;
	context.incomingDamage = std::max(0, CalcTotalIncomingDamage());
	context.time = sPokerGlowRainbowTime;
	context.hpDamageBlinkSpeed = sHpDamageBlinkSpeed;
	context.hpGaugeBloomEnabled = sHpGaugeBloomEnabled;
	context.hpGaugeBloomIntensity = sHpGaugeBloomIntensity;
	context.hpDamageBloomIntensity = sHpDamageBloomIntensity;
	BattleRenderView::DrawHpGaugeBloom(context, view, proj);
}
#ifdef USE_IMGUI
void BattleController::DrawPlayerHudImGuiControls()
{
	BattleDebugImGui::DrawPlayerHudControls(playerStatusUi_);
}

void BattleController::DrawImGui()
{
	const char* cardStateName = "";
	switch (cardState_) {
	case CardInputState::Idle: cardStateName = "Idle"; break;
	case CardInputState::Dragging: cardStateName = "Dragging"; break;
	case CardInputState::Preview: cardStateName = "Preview"; break;
	case CardInputState::ChoosingFieldReplace: cardStateName = "ChoosingFieldReplace"; break;
	}

	const PokerBonus bonus = GetPokerBonus_(currentPoker_.rank);

	BattleDebugImGui::Context context{};
	context.playerStatusUi = &playerStatusUi_;
	context.turnName = turn_ == TurnState::Player ? "Player" : "Enemy";
	context.playerTurnCount = playerTurnCount_;
	context.enemyTurnCount = enemyTurnCount_;
	context.energy = energy_;
	context.energyMax = energyMax_;
	context.deckZone = &deckZone_;
	context.field = &field_;
	context.handView = &handView_;
	context.cardStateName = cardStateName;
	context.hasPendingCard = hasPendingCard_;
	context.pendingCard = pendingCard_;
	context.evaluatedPoker = EvaluatePokerHand_();
	context.currentPoker = currentPoker_;
	context.waitingActivateChoice = pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice;
	context.waitingEffectChoice = pokerChoiceState_ == PokerChoiceState::WaitingEffectChoice;
	context.currentPokerBonus = { bonus.atkUp, bonus.drawCount, bonus.damage };
	context.player = player_;
	context.enemyMgr = enemyMgr_;
	context.propManager = propManager_.get();
	context.useDebugPreviewBuff = &useDebugPreviewBuff_;
	context.currentTurnAtkUp = &currentTurnAtkUp_;
	context.nextTurnAtkUp = &nextTurnAtkUp_;
	context.debugPreviewPowerBoost = &debugPreviewPowerBoost_;
	context.debugPreviewCurrentTurnAtkUp = &debugPreviewCurrentTurnAtkUp_;
	context.debugPreviewNextTurnAtkUp = &debugPreviewNextTurnAtkUp_;
	context.actionDirector = &actionDirector_;
	context.camera = cam_;

	context.fieldCardGlitter.localOffset = &sFieldCardGlitterLocalOffset;
	context.fieldCardGlitter.spreadX = &sFieldCardGlitterSpreadX;
	context.fieldCardGlitter.spreadY = &sFieldCardGlitterSpreadY;
	context.fieldCardGlitter.emitInterval = &sFieldCardGlitterEmitInterval;
	context.fieldCardGlitter.normalCount = &sFieldCardGlitterNormalCount;
	context.fieldCardGlitter.highlightCount = &sFieldCardGlitterHighlightCount;
	context.fieldCardGlitter.handPreviewEnabled = &sHandPokerPreviewEnabled;
	context.fieldCardGlitter.handEmitInterval = &sHandCardGlitterEmitInterval;
	context.fieldCardGlitter.handCount = &sHandCardGlitterCount;

	context.fieldFrameBloom.enabled = &sFieldFrameBloomEnabled;
	context.fieldFrameBloom.threshold = &sFieldFrameBloomThreshold;
	context.fieldFrameBloom.intensity = &sFieldFrameBloomIntensity;
	context.fieldFrameBloom.handIntensity = &sHandFrameBloomIntensity;
	context.fieldFrameBloom.minPulse = &sFieldFrameBloomMinPulse;
	context.fieldFrameBloom.chromAb = &sFieldFrameBloomChromAb;

	context.enemyBloom.intentEnabled = &sEnemyIntentBloomEnabled;
	context.enemyBloom.intentIntensity = &sEnemyIntentBloomIntensity;
	context.enemyBloom.intentMinPulse = &sEnemyIntentBloomMinPulse;
	context.enemyBloom.targetEnabled = &sEnemyTargetBloomEnabled;
	context.enemyBloom.targetIntensity = &sEnemyTargetBloomIntensity;
	context.enemyBloom.targetChromAb = &sEnemyTargetBloomChromAb;

	context.hpGaugeBloom.enabled = &sHpGaugeBloomEnabled;
	context.hpGaugeBloom.intensity = &sHpGaugeBloomIntensity;
	context.hpGaugeBloom.minPulse = &sHpGaugeBloomMinPulse;
	context.hpGaugeBloom.damageBlinkSpeed = &sHpDamageBlinkSpeed;
	context.hpGaugeBloom.damageIntensity = &sHpDamageBloomIntensity;
	context.frostAction.threshold = &sFrostBurstThreshold;
	context.frostAction.burstMultiplier = &sFrostBurstMultiplier;

	BattleDebugImGui::Draw(context);

	if (ImGui::CollapsingHeader("Status Effect Particles")) {
		ImGui::DragFloat("Apply Height", &sStatusEffectApplyHeight, 0.01f, -5.0f, 8.0f);
		ImGui::DragFloat("Idle Height", &sStatusEffectIdleHeight, 0.01f, -5.0f, 8.0f);
		ImGui::DragFloat("Camera Forward Offset", &sStatusEffectCameraForwardOffset, 0.01f, 0.0f, 5.0f);
		if (ImGui::Button("Reset Status Effect Particles")) {
			sStatusEffectApplyHeight = 1.35f;
			sStatusEffectIdleHeight = 1.28f;
			sStatusEffectCameraForwardOffset = 0.65f;
		}
	}
}
#endif

