#include "Battle/BattleRenderView.h"

#include "Card3D.h"
#include "Enemy.h"
#include "GameApp.h"
#include "HandView3D.h"
#include "MathStruct.h"
#include "Object3dCommon.h"
#include "ObjectPostEffect.h"
#include "DirectXCommon.h"
#include "Player.h"
#include "Poker/PokerHandEvaluator.h"
#include "PropManager.h"
#include "Sprite.h"
#include "UI/BattleActionDirector.h"
#include "UI/DamagePopupUI.h"
#include "UI/EnemyBattleStatusUI.h"
#include "UI/PlayerBattleStatusUI.h"
#include "WinApp.h"

#include <algorithm>
#include <cmath>

void BattleRenderView::DrawDamagePopups3D(BattleActionDirector& actionDirector, DamagePopupUI& damagePopupUi)
{
	actionDirector.Draw3D();
	damagePopupUi.Draw3D();
}

void BattleRenderView::DrawCardArea3D(const CardAreaContext& context)
{
	if (context.discardView) {
		context.discardView->Draw();
	}

	context.handView->Draw();
	context.handView->DrawPreviewCard();

	context.damagePopupUi->Draw3D();

	if (context.choosingFieldReplace) {
		context.highlightFilter->Draw();
		context.pendingCardView->Draw();
	}

	for (auto& card : *context.fieldViews) {
		card->Draw();
	}
}

void BattleRenderView::DrawField3D(PropManager* propManager)
{
	if (propManager) {
		propManager->Draw3D();
	}
}

void BattleRenderView::DrawBattleOverlay3D(const BattleOverlayContext& context)
{
	if (!context.choosingEnemyTarget) {
		return;
	}

	context.highlightFilter->Draw();

	if (context.enemyMgr) {
		context.app->ObjCom()->SetGraphicsPipelineState();
		context.app->Dx()->ClearDepthBuffer();

		bool hasHighlightedEnemy = false;
		for (const auto& enemy : context.enemyMgr->GetEnemies()) {
			hasHighlightedEnemy = hasHighlightedEnemy || enemy.IsHighlighted();
		}

		if (context.enemyTargetBloomEnabled && hasHighlightedEnemy && context.enemyTargetBloomParam) {
			context.app->ObjectPost()->SetParam(*context.enemyTargetBloomParam);
			context.app->BeginObjectPostEffect();
			for (auto& enemy : context.enemyMgr->GetEnemies()) {
				if (enemy.IsHighlighted()) {
					enemy.Draw();
				}
			}
			context.app->EndObjectPostEffect();
			context.app->ObjCom()->SetGraphicsPipelineState();
		}

		for (auto& enemy : context.enemyMgr->GetEnemies()) {
			if (enemy.IsHighlighted()) {
				enemy.Draw();
			}
		}
	}
}

void BattleRenderView::DrawPostEffect3D(HandView3D& handView, GameApp& app)
{
	handView.DrawDiscardingCardsObjectPost(app);
}

void BattleRenderView::DrawFieldFrameBloom(const FieldFrameBloomContext& context)
{
	if (!context.enabled) {
		return;
	}

	bool hasBloomTarget = false;
	for (int i = 0; i < static_cast<int>(context.fieldViews->size()); ++i) {
		const bool replacePreview = i < static_cast<int>(context.fieldReplacePreviewActive->size()) &&
			(*context.fieldReplacePreviewActive)[i];
		if ((*context.fieldViews)[i] &&
			(replacePreview || (!context.inReplacePreview && i < 5 && context.pokerHighlightMask[i] && context.currentPokerRank != PokerHandRank::None))) {
			hasBloomTarget = true;
			break;
		}
	}
	for (PokerHandRank rank : *context.handPreviewRanks) {
		if (rank != PokerHandRank::None) {
			hasBloomTarget = true;
			break;
		}
	}
	if (!hasBloomTarget) {
		return;
	}

	BloomParam param = context.app->ObjectPost()->GetParam();
	param.threshold = context.threshold;
	const float pulse = context.minPulse + (1.0f - context.minPulse) * (0.5f + 0.5f * std::sin(context.time * 1.6f));
	param.intensity = std::max(context.intensity, context.handIntensity) * pulse;
	param.vignetteIntensity = 0.0f;
	param.vignetteScale = 0.0f;
	param.distortionAmount = 0.0f;
	param.chromAbAmount = context.chromAb;
	param.isGrayscale = 0.0f;
	param.isInverted = 0.0f;
	param.noiseIntensity = 0.0f;
	param.scanlineIntensity = 0.0f;
	param.curvature = 0.0f;
	param.borderSharp = 0.0f;
	param.glitchAmount = 0.0f;
	param.dissolveAmount = -1.0f;

	context.app->ObjectPost()->SetParam(param);
	context.app->BeginObjectPostEffect();
	for (int i = 0; i < static_cast<int>(context.fieldViews->size()); ++i) {
		const bool replacePreview = i < static_cast<int>(context.fieldReplacePreviewActive->size()) &&
			(*context.fieldReplacePreviewActive)[i];
		const bool currentHighlight = !context.inReplacePreview && i < 5 && context.pokerHighlightMask[i] &&
			context.currentPokerRank != PokerHandRank::None;
		if (!(*context.fieldViews)[i] || (!replacePreview && !currentHighlight)) {
			continue;
		}
		(*context.fieldViews)[i]->DrawFrameOnly();
	}
	const int handCount = std::min<int>(static_cast<int>(context.handPreviewRanks->size()), context.handView->GetCardCount());
	for (int i = 0; i < handCount; ++i) {
		if ((*context.handPreviewRanks)[i] == PokerHandRank::None) {
			continue;
		}
		Card3D* card = context.handView->GetCard(i);
		if (card) {
			card->DrawFrameOnly();
		}
	}
	context.app->EndObjectPostEffect();
	context.app->ObjCom()->SetGraphicsPipelineState();
}

void BattleRenderView::Draw2D(const Draw2DContext& context)
{
	if (context.actionDirector->IsPlaying()) {
		context.actionDirector->Draw2D();
		return;
	}

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
		0, 0,
		float(WinApp::kClientWidth),
		float(WinApp::kClientHeight),
		0, 100
	);
	DrawHpGaugeBloom(context, view, proj);
	if (context.player) {
		context.playerStatusUi->DrawHpGauge(
			view,
			proj,
			context.player->GetHP(),
			context.player->GetMaxHP(),
			context.player->GetBlock(),
			context.incomingDamage,
			context.time,
			context.hpDamageBlinkSpeed);
	}

	if (context.enemyMgr) {
		auto& enemies = context.enemyMgr->GetEnemies();
		EnemyBattleStatusUI::EnemyBloomSettings bloomSettings{};
		bloomSettings.intentEnabled = context.enemyIntentBloomEnabled;
		bloomSettings.intentIntensity = context.enemyIntentBloomIntensity;
		bloomSettings.intentMinPulse = context.enemyIntentBloomMinPulse;
		context.enemyStatusUi->DrawGaugeAndIntent2D(
			*context.app,
			enemies,
			*context.enemyActedFlags,
			context.time,
			view,
			proj,
			bloomSettings);
	}
	context.actionDirector->Draw2D();
}

void BattleRenderView::DrawHpGaugeBloom(const Draw2DContext& context, const Matrix4x4& view, const Matrix4x4& proj)
{
	if (!context.hpGaugeBloomEnabled) {
		return;
	}

	const float baseIntensity = context.hpGaugeBloomIntensity;
	if (context.player) {
		context.playerStatusUi->DrawHpGaugeBloom(
			*context.app,
			view,
			proj,
			context.player->GetHP(),
			context.player->GetMaxHP(),
			context.player->GetBlock(),
			context.incomingDamage,
			context.time,
			baseIntensity,
			context.hpDamageBloomIntensity,
			context.hpDamageBlinkSpeed);
	}

	if (context.enemyMgr) {
		auto& enemies = context.enemyMgr->GetEnemies();
		context.enemyStatusUi->DrawGaugeBloom(*context.app, enemies, view, proj, baseIntensity);
	}
}

void BattleRenderView::DrawPlayerBattleStatusUI(
	PlayerBattleStatusUI& playerStatusUi,
	const std::wstring& hpText,
	const std::wstring& blockText,
	const std::wstring& powerBoostText,
	const Matrix4x4& view,
	const Matrix4x4& proj)
{
	playerStatusUi.SetTexts(hpText, blockText, powerBoostText);
	playerStatusUi.DrawStatus2D(view, proj);
}

void BattleRenderView::DrawEnemyBattleStatusHpTexts(EnemyBattleStatusUI& enemyStatusUi, const Matrix4x4& view, const Matrix4x4& proj)
{
	enemyStatusUi.DrawHpTexts2D(view, proj);
}

void BattleRenderView::DrawEnemyBattleStatusBcTexts(EnemyBattleStatusUI& enemyStatusUi, const Matrix4x4& view, const Matrix4x4& proj)
{
	enemyStatusUi.DrawBcTexts2D(view, proj);
}
