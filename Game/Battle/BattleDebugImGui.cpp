#include "Battle/BattleDebugImGui.h"

#include "Battle/BattleDeckZone.h"
#include "Card3D.h"
#include "Enemy.h"
#include "HandView3D.h"
#include "Object3d.h"
#include "Player.h"
#include "PropManager.h"
#include "UI/BattleActionDirector.h"
#include "UI/PlayerBattleStatusUI.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include <string>

namespace {
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

void BattleDebugImGui::DrawPlayerHudControls(PlayerBattleStatusUI& playerStatusUi)
{
#ifdef USE_IMGUI
	playerStatusUi.DrawImGuiControls();
#else
	(void)playerStatusUi;
#endif
}

void BattleDebugImGui::Draw(const Context& context)
{
#ifndef USE_IMGUI
	(void)context;
#else
	Card3D::DrawAdjustImGui();

	ImGui::Text("turn: %s", context.turnName);
	ImGui::Text("PlayerTurnCount : %d", context.playerTurnCount);
	ImGui::Text("EnemyTurnCount : %d", context.enemyTurnCount);
	ImGui::Text("energy: %d / %d", context.energy, context.energyMax);
	ImGui::Text("hand: %d  discard: %d",
		context.deckZone ? static_cast<int>(context.deckZone->GetHandCount()) : 0,
		context.deckZone ? static_cast<int>(context.deckZone->GetDiscardCount()) : 0);
	ImGui::Text("field: %d", context.field ? static_cast<int>(context.field->size()) : 0);

	if (ImGui::CollapsingHeader("Field Card Glitter")) {
		ImGui::DragFloat3("Emitter Local Offset", &context.fieldCardGlitter.localOffset->x, 0.01f);
		ImGui::DragFloat("Emitter Spread X", context.fieldCardGlitter.spreadX, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Emitter Spread Y", context.fieldCardGlitter.spreadY, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Emit Interval", context.fieldCardGlitter.emitInterval, 0.01f, 0.01f, 2.0f);
		ImGui::SliderInt("Normal Count", context.fieldCardGlitter.normalCount, 0, 30);
		ImGui::SliderInt("Highlight Count", context.fieldCardGlitter.highlightCount, 0, 60);
		ImGui::Checkbox("Hand Poker Preview", context.fieldCardGlitter.handPreviewEnabled);
		ImGui::DragFloat("Hand Emit Interval", context.fieldCardGlitter.handEmitInterval, 0.01f, 0.01f, 2.0f);
		ImGui::SliderInt("Hand Count", context.fieldCardGlitter.handCount, 0, 60);
		if (ImGui::Button("Reset Field Card Glitter")) {
			*context.fieldCardGlitter.localOffset = { 0.0f, 2.2f, 0.12f };
			*context.fieldCardGlitter.spreadX = 1.05f;
			*context.fieldCardGlitter.spreadY = 0.08f;
			*context.fieldCardGlitter.emitInterval = 0.12f;
			*context.fieldCardGlitter.normalCount = 2;
			*context.fieldCardGlitter.highlightCount = 5;
			*context.fieldCardGlitter.handPreviewEnabled = true;
			*context.fieldCardGlitter.handEmitInterval = 0.12f;
			*context.fieldCardGlitter.handCount = 3;
		}
	}

	if (ImGui::CollapsingHeader("Field Frame Bloom")) {
		ImGui::Checkbox("Enable Frame Bloom", context.fieldFrameBloom.enabled);
		ImGui::DragFloat("Frame Bloom Threshold", context.fieldFrameBloom.threshold, 0.01f, 0.0f, 3.0f);
		ImGui::DragFloat("Frame Bloom Intensity", context.fieldFrameBloom.intensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Hand Bloom Intensity", context.fieldFrameBloom.handIntensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Frame Bloom Min Pulse", context.fieldFrameBloom.minPulse, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Frame Bloom ChromAb", context.fieldFrameBloom.chromAb, 0.0005f, 0.0f, 0.05f);
		if (ImGui::Button("Reset Field Frame Bloom")) {
			*context.fieldFrameBloom.enabled = true;
			*context.fieldFrameBloom.threshold = 0.35f;
			*context.fieldFrameBloom.intensity = 1.45f;
			*context.fieldFrameBloom.handIntensity = 1.15f;
			*context.fieldFrameBloom.minPulse = 0.15f;
			*context.fieldFrameBloom.chromAb = 0.0015f;
		}
	}

	if (ImGui::CollapsingHeader("Enemy Bloom")) {
		ImGui::Checkbox("Intent Bloom", context.enemyBloom.intentEnabled);
		ImGui::DragFloat("Intent Intensity", context.enemyBloom.intentIntensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Intent Min Pulse", context.enemyBloom.intentMinPulse, 0.01f, 0.0f, 1.0f);
		ImGui::Checkbox("Target Bloom", context.enemyBloom.targetEnabled);
		ImGui::DragFloat("Target Intensity", context.enemyBloom.targetIntensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Target ChromAb", context.enemyBloom.targetChromAb, 0.0005f, 0.0f, 0.05f);
		if (ImGui::Button("Reset Enemy Bloom")) {
			*context.enemyBloom.intentEnabled = true;
			*context.enemyBloom.intentIntensity = 1.8f;
			*context.enemyBloom.intentMinPulse = 0.45f;
			*context.enemyBloom.targetEnabled = true;
			*context.enemyBloom.targetIntensity = 2.1f;
			*context.enemyBloom.targetChromAb = 0.002f;
		}
	}

	if (ImGui::CollapsingHeader("HP Gauge Bloom")) {
		ImGui::Checkbox("HP Bloom", context.hpGaugeBloom.enabled);
		ImGui::DragFloat("HP Bloom Intensity", context.hpGaugeBloom.intensity, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("HP Bloom Min Pulse", context.hpGaugeBloom.minPulse, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Damage Blink Speed", context.hpGaugeBloom.damageBlinkSpeed, 0.1f, 0.0f, 20.0f);
		ImGui::DragFloat("Damage Bloom Intensity", context.hpGaugeBloom.damageIntensity, 0.01f, 0.0f, 5.0f);
		if (ImGui::Button("Reset HP Gauge Bloom")) {
			*context.hpGaugeBloom.enabled = true;
			*context.hpGaugeBloom.intensity = 0.55f;
			*context.hpGaugeBloom.minPulse = 0.65f;
			*context.hpGaugeBloom.damageBlinkSpeed = 6.0f;
			*context.hpGaugeBloom.damageIntensity = 1.05f;
		}
	}

	if (ImGui::CollapsingHeader("Character Scale")) {
		if (context.player && context.player->GetObject3d()) {
			Vector3 pScale = context.player->GetObject3d()->GetScale();
			if (ImGui::DragFloat3("Player Scale", &pScale.x, 0.01f)) {
				context.player->GetObject3d()->SetScale(pScale);
			}
		}
		if (context.enemyMgr) {
			int idx = 0;
			for (auto& enemy : context.enemyMgr->GetEnemies()) {
				if (!enemy.IsAlive() || !enemy.GetObject3d()) {
					idx++;
					continue;
				}
				ImGui::PushID(idx);
				Vector3 eScale = enemy.GetObject3d()->GetScale();
				if (ImGui::DragFloat3(("Enemy " + std::to_string(idx) + " Scale").c_str(), &eScale.x, 0.01f)) {
					enemy.GetObject3d()->SetScale(eScale);
				}
				ImGui::PopID();
				idx++;
			}
		}
	}

	if (context.handView) {
		context.handView->DrawImGui();
	}

	ImGui::Text("cardState: %s", context.cardStateName);

	if (context.hasPendingCard) {
		ImGui::Text("pending: defId=%d number=%d suit=%s",
			context.pendingCard.defId,
			context.pendingCard.number,
			SuitToString(context.pendingCard.suit));
	} else {
		ImGui::Text("pending: none");
	}

	ImGui::Separator();
	ImGui::Text("Hand Cards");
	if (context.deckZone) {
		const auto& hand = context.deckZone->GetHand();
		for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
			ImGui::Text("hand[%d] defId=%d number=%d suit=%s",
				i,
				hand[i].defId,
				hand[i].number,
				SuitToString(hand[i].suit));
		}
	}

	ImGui::Separator();
	ImGui::Text("Field Cards");
	if (context.field) {
		for (int i = 0; i < static_cast<int>(context.field->size()); ++i) {
			const CardInstance& card = (*context.field)[i];
			ImGui::Text("field[%d] defId=%d number=%d suit=%s",
				i,
				card.defId,
				card.number,
				SuitToString(card.suit));
		}
	}

	ImGui::Separator();
	ImGui::Text("Poker Hand: %s", PokerHandEvaluator::GetHandName(context.evaluatedPoker.rank));
	ImGui::Text("Poker Power: %d", context.evaluatedPoker.power);

	if (context.waitingActivateChoice)
	{
		ImGui::Separator();
		ImGui::Text("Poker Skill Available!");
		ImGui::Text("Hand : %s", PokerHandEvaluator::GetHandName(context.currentPoker.rank));
		ImGui::Text("Press Y = Activate");
		ImGui::Text("Press N = Skip");
	}

	if (context.waitingEffectChoice)
	{
		ImGui::Separator();
		ImGui::Text("Choose Poker Effect");
		ImGui::Text("Hand : %s", PokerHandEvaluator::GetHandName(context.currentPoker.rank));
		ImGui::Text("1 : Next Turn ATK UP (+%d)", context.currentPokerBonus.atkUp);
		ImGui::Text("2 : Draw %d", context.currentPokerBonus.drawCount);
		ImGui::Text("3 : Damage %d", context.currentPokerBonus.damage);
		ImGui::Text("N : Back");
	}
	ImGui::Separator();

	if (context.player) {
		ImGui::Text("Player Hp: %d", context.player->GetHP());
		ImGui::Text("Player Hp: %d (Block: %d)", context.player->GetHP(), context.player->GetBlock());
		ImGui::Text("Player Power: %d (CurrentTurnAtkUp: %d / NextTurnAtkUp: %d)",
			context.player->GetBoostedPower(),
			*context.currentTurnAtkUp,
			*context.nextTurnAtkUp);
	} else {
		ImGui::Text("Player: null");
		ImGui::Text("Player Power: preview mode (CurrentTurnAtkUp: %d / NextTurnAtkUp: %d)",
			*context.currentTurnAtkUp,
			*context.nextTurnAtkUp);
	}

	if (context.enemyMgr && !context.enemyMgr->GetEnemies().empty()) {
		ImGui::Text("Enemy Hp: %d", context.enemyMgr->GetEnemies()[0].GetHP());
	} else {
		ImGui::Text("Enemy: null");
	}

	ImGui::Separator();
	if (context.propManager) {
		context.propManager->DrawImGui();
	}

	ImGui::Separator();
	ImGui::Text("Attack Debug");

	ImGui::Checkbox("Use Debug Preview Buff", context.useDebugPreviewBuff);

	if (context.player) {
		ImGui::Text("Runtime Player Connected");

		int previewPower = context.player->GetBoostedPower();
		if (ImGui::DragInt("Player PowerBoost", &previewPower, 1.0f, -999, 999)) {
			context.player->ResetPowerBoost();
			if (previewPower > 0) {
				context.player->PowerBoost(previewPower);
			}
		}

		ImGui::DragInt("CurrentTurnAtkUp", context.currentTurnAtkUp, 1.0f, -999, 999);
		ImGui::DragInt("NextTurnAtkUp", context.nextTurnAtkUp, 1.0f, -999, 999);
	} else {
		ImGui::Text("Preview Only (No Player Connected)");
		ImGui::DragInt("Debug PowerBoost", context.debugPreviewPowerBoost, 1.0f, -999, 999);
		ImGui::DragInt("Debug CurrentTurnAtkUp", context.debugPreviewCurrentTurnAtkUp, 1.0f, -999, 999);
		ImGui::DragInt("Debug NextTurnAtkUp", context.debugPreviewNextTurnAtkUp, 1.0f, -999, 999);
	}

	if (context.actionDirector) {
		context.actionDirector->DrawImGuiEditor(context.camera);
	}
#endif
}