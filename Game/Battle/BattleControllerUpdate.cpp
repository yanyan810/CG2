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
void BattleController::Update(GameApp& app, FieldUi& fieldUi, float dt)
{
	if (actionDirector_.IsPlaying()) {
		Camera* actionCamera = GetActionCamera();
		if (actionCamera) {
			app.ObjCom()->SetDefaultCamera(actionCamera);
			if (player_) {
				player_->SetCamera(actionCamera);
			}
			if (enemyMgr_) {
				enemyMgr_->UpdateCamera(actionCamera);
			}
		}

		const bool sequenceFinished = actionDirector_.Update(dt, app.GetInput());
		const bool isFinalActionSequence =
			cardState_ == CardInputState::ExecutingSequence &&
			actionSequenceIndex_ >= actionSequenceQueue_.size();
		if (isFinalActionSequence &&
			!actionSequenceDamageApplied_ &&
			actionDirector_.HasReachedImpact()) {
			Enemy& targetEnemy = actionSequenceTarget_
				? *actionSequenceTarget_
				: enemyMgr_->GetEnemies()[currentEnemyIndex_];
			ExecutePendingAttack_(targetEnemy);
			actionSequenceDamageApplied_ = true;
		}

		if (sequenceFinished) {
			app.ObjCom()->SetDefaultCamera(cam_);
			if (player_) {
				player_->SetCamera(cam_);
			}
			if (enemyMgr_) {
				enemyMgr_->UpdateCamera(cam_);
			}
			if (cardState_ == CardInputState::ExecutingSequence) {
				if (StartNextActionSequence_()) {
					return;
				}
				Enemy& targetEnemy = actionSequenceTarget_
					? *actionSequenceTarget_
					: enemyMgr_->GetEnemies()[currentEnemyIndex_];
				if (!actionSequenceDamageApplied_) {
					ExecutePendingAttack_(targetEnemy);
				}
			} else if (actionSequenceDamageApplied_) {
				actionSequenceQueue_.clear();
				actionSequenceIndex_ = 0;
				actionSequenceCardDef_ = nullptr;
				actionSequenceDamageApplied_ = false;
			}
		}
		// Skip logic but update visuals
		UpdateVisuals_(dt);
		return;
	}

	UpdateLogic_(app, fieldUi, dt);

	UpdateVisuals_(dt);
	EmitFieldCardGlitter_(dt);
	EmitHandCardGlitter_(dt);
}

void BattleController::UpdateClearTransitionVisuals(float dt)
{
	UpdateVisuals_(dt);
}

void BattleController::PrepareForClearTransition()
{
	cardState_ = CardInputState::Idle;
	pokerChoiceState_ = PokerChoiceState::None;
	pokerReturnState_ = PokerChoiceState::None;
	pokerQuickPreviewVisible_ = false;
	hasPendingCard_ = false;
	pendingCard_ = {};
	pendingCardView_.reset();
	selectedIndex_ = -1;
	fieldReplaceHoverIndex_ = -1;
	prevFieldReplaceHoverIndex_ = -1;
	isPokerDamageTargeting_ = false;
	tutorialLockPokerTargetingCancel_ = false;
	pendingDamage_ = 0;
	handView_.SetHoverIndex(-1);
	handView_.SetPreviewIndex(-1);
	handView_.SetDrag(-1, 0.0f, 0.0f, false);
	handView_.SetFocusIndex(-1);
	if (enemyMgr_) {
		for (auto& enemy : enemyMgr_->GetEnemies()) {
			enemy.SetHighlight(false);
		}
	}
}

Camera* BattleController::GetActionCamera() const
{
	if (!actionDirector_.IsPlaying()) {
		return nullptr;
	}
	if (!actionDirector_.GetProfile().enableCameraWork) {
		return nullptr;
	}
	return actionDirector_.GetCinematicCamera();
}

Enemy* BattleController::GetActionTarget() const
{
	if (actionDirector_.IsPlaying()) {
		return actionDirector_.GetTarget();
	}
	return actionSequenceTarget_;
}

void BattleController::UpdateLogic_(GameApp& app, FieldUi& fieldUi, float dt)
{
	Input* input = app.GetInput();
	if (!input) {
		return;
	}

	bool yTrig = input->IsKeyTrigger(DIK_Y);
	bool nTrig = input->IsKeyTrigger(DIK_N);

	POINT mouse = input->GetMousePosition();

	bool lNow = input->IsMousePressed(0);
	bool lTrig = input->IsMouseTrigger(0);
	bool lRel = input->IsMouseReleased(0);

	bool rTrig = input->IsMouseTrigger(1);
	// ---------------------------------
	// ---------------------------------
	if (tutorialInputLocked_) {
		yTrig = false;
		nTrig = false;
		lTrig = false;
		lRel = false;
		rTrig = false;
	}

	const BattleCardInputController::CardInputSnapshot cardInput{ mouse.x, mouse.y, lTrig, lRel, rTrig, lNow };

	pokerMouseChoice_ = PokerMouseChoice::None;

	// -----------------------------
	// -----------------------------
	if (pokerChoiceState_ == PokerChoiceState::WaitingActivateChoice)
	{
		HandlePokerActivateChoice_(fieldUi, mouse, lTrig, yTrig, nTrig);
		return;
	}

	// -----------------------------
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
	if (tutorialInputLocked_) {
		enterTrig = false;
	}

	operationUiVisible_ = !tutorialInputLocked_ && input->IsKeyPressed(DIK_TAB);

	bool endTurnButtonClicked = false;
	endTurnButtonHovered_ = false;

	if (turn_ == TurnState::Player &&
		cardState_ == CardInputState::Idle &&
		pokerChoiceState_ == PokerChoiceState::None &&
		!tutorialEndTurnLocked_) {

		endTurnButtonHovered_ = false;

		if (propManager_) {
			Matrix4x4 vp = cam_->GetViewProjectionMatrix();
			for (const auto& prop : propManager_->GetProps()) {
				if (prop.name == "Button" || prop.name == "EndTurnButton") {
					Vector4 clip = MulRowVec4Mat4({ prop.pos.x, prop.pos.y, prop.pos.z, 1.0f }, vp);
					if (clip.w > 0.0f) {
						float sx = (clip.x / clip.w + 1.0f) * 0.5f * WinApp::kClientWidth;
						float sy = (1.0f - clip.y / clip.w) * 0.5f * WinApp::kClientHeight;

						constexpr float kEndTurnButtonHitRadiusBase = 35.0f;
						float radius = kEndTurnButtonHitRadiusBase * prop.scale.x;
						float dx = mouse.x - sx;
						float dy = mouse.y - sy;
						if (dx * dx + dy * dy <= radius * radius) {
							endTurnButtonHovered_ = true;
							break;
						}
					}
				}
			}
		}

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
			int hover = BattleCardInputController::PickHandIndexByMouse(handView_, cam_->GetViewProjectionMatrix(), mouse.x, mouse.y, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight);
			if (!IsTutorialForcedCardAllowed_(hover)) {
				hover = -1;
			}
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

			OutputDebugStringA(("Before EndTurn hand=" + std::to_string(deckZone_.GetHandCount()) +
				" deck=" + std::to_string(deckZone_.GetDeckCount()) +
				" discard=" + std::to_string(deckZone_.GetDiscardCount()) +
				" field=" + std::to_string(field_.size()) + "\n").c_str());

			turn_ = TurnState::Enemy;
			player_->SetPoisonDrawActive(false);
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
			int hover = BattleCardInputController::PickHandIndexByMouse(handView_, cam_->GetViewProjectionMatrix(), mouse.x, mouse.y, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight);
			if (!IsTutorialForcedCardAllowed_(hover)) {
				hover = -1;
			}
			handView_.SetHoverIndex(hover);
			} else {
				handView_.SetHoverIndex(-1);
			}

			switch (cardState_) {
			case CardInputState::Idle:
				handView_.SetDrag(-1, 0, 0, false);
				handView_.SetPreviewIndex(-1);

				if (lTrig) {
					int idx = BattleCardInputController::PickHandIndexByMouse(handView_, cam_->GetViewProjectionMatrix(), mouse.x, mouse.y, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight);
					if (!IsTutorialForcedCardAllowed_(idx)) {
						idx = -1;
					}
					const auto handDecision = BattleCardInputController::ResolveIdle(cardInput, idx);
					if (handDecision.action == BattleCardInputController::HandInputAction::StartDrag) {
						selectedIndex_ = handDecision.handIndex;
						dragStartMouse_ = mouse;
						dragDx_ = dragDy_ = 0.0f;
						cardState_ = CardInputState::Dragging;
					}
				}
				break;

			case CardInputState::Dragging:
			{
				const float threshold = 80.0f;
				const auto handDecision = BattleCardInputController::ResolveDragging(cardInput, selectedIndex_, dragStartMouse_.x, dragStartMouse_.y, threshold);

				dragDx_ = handDecision.dragDx;
				dragDy_ = handDecision.dragDy;

				handView_.SetDrag(selectedIndex_, dragDx_, dragDy_, true);

				if (handDecision.action == BattleCardInputController::HandInputAction::OpenPreview ||
					handDecision.action == BattleCardInputController::HandInputAction::ReturnToIdle) {
					handView_.SetDrag(-1, 0, 0, false);

					if (handDecision.action == BattleCardInputController::HandInputAction::OpenPreview) {
						BattleSfxPlayer::PlaySE("SE_CardFlick");
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

			{
				handView_.SetPreviewIndex(selectedIndex_);

				if (!IsTutorialForcedCardAllowed_(selectedIndex_)) {
					cardState_ = CardInputState::Idle;
					selectedIndex_ = -1;
					handView_.SetPreviewIndex(-1);
					break;
				}

				if (lTrig) {
					int idx = selectedIndex_;
					if (idx >= 0 && idx < static_cast<int>(deckZone_.GetHandCount())) {
						CardInstance inst = deckZone_.GetHand()[idx];
						const CardDef* def = db_.Find(inst.defId);

						if (def && def->cost <= energy_) {

							const auto targetRequirement =
								BattleCardInputController::AnalyzeTargetRequirement(
									*def,
									player_ ? player_->GetBlock() : 0,
									currentTurnAtkUp_ + (player_ ? player_->GetBoostedPower() : 0),
									playerTurnCount_);

							if (targetRequirement.needsTarget) {
								pendingDamage_ = targetRequirement.pendingDamage;

								isPokerDamageTargeting_ = false;
								pendingCardHandIndex_ = idx;
								handView_.SetFocusIndex(idx);
								cardState_ = CardInputState::ChoosingEnemyTarget;

								selectedIndex_ = -1;
								handView_.SetPreviewIndex(-1);
								return;
							}

							pendingCardHandIndex_ = idx;
							Enemy* sequenceTarget = nullptr;
							if (enemyMgr_) {
								for (auto& enemy : enemyMgr_->GetEnemies()) {
									if (enemy.IsAlive()) {
										sequenceTarget = &enemy;
										break;
									}
								}
							}

							if (sequenceTarget && BeginCardActionSequence_(app, *def, inst, *sequenceTarget)) {
								handView_.SetFocusIndex(idx);
								cardState_ = CardInputState::ExecutingSequence;

								selectedIndex_ = -1;
								handView_.SetPreviewIndex(-1);
								return;
							}

							energy_ -= def->cost;
							BattleSfxPlayer::PlaySE("SE_CardPlay");
							BattleSfxPlayer::PlayAttackSEForCard(*def);
							auto usedCardView = handView_.ExtractCardAt(idx);
							deckZone_.RemoveHandAt(static_cast<std::size_t>(idx));
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
							OnPlayerCardUsed_();
						} else {
							cardState_ = CardInputState::Idle;
						}
					} else {
						cardState_ = CardInputState::Idle;
					}

					selectedIndex_ = -1;
					handView_.SetPreviewIndex(-1);
				}
				const auto previewDecision = BattleCardInputController::ResolvePreview(cardInput, selectedIndex_);
				if (previewDecision.action == BattleCardInputController::HandInputAction::CancelPreview) {
					cardState_ = CardInputState::Idle;
					selectedIndex_ = -1;
					handView_.SetPreviewIndex(-1);
				}
				break;
			}

			case CardInputState::ChoosingFieldReplace:
			{
				if (pendingCardView_) {

					Vector3 previewPos = { -10.f, 2.0f, 3.0 };
					pendingCardView_->SetTransform(previewPos, { 0.0f, 0.0f, 0.0f }, { 1.f, 1.f, 1.f });
					pendingCardView_->Update(dt);
				}

				int newHover = PickFieldIndexByMouse_(mouse.x, mouse.y);
				const auto fieldReplaceDecision = BattleCardInputController::ResolveFieldReplaceInput(cardInput, newHover);

				if (fieldReplaceDecision.hoverIndex != fieldReplaceHoverIndex_) {
					fieldReplaceHoverIndex_ = fieldReplaceDecision.hoverIndex;
					fieldLayoutDirty_ = true;
				}

				if (fieldReplaceDecision.replaceRequested) {
					int replaceIndex = fieldReplaceHoverIndex_;
					if (replaceIndex >= 0 && replaceIndex < (int)field_.size() && hasPendingCard_) {
						BattleSfxPlayer::PlaySE("SE_CardFlick");
						if (fieldViews_[replaceIndex]) {
							handView_.AddDiscardingCard(std::move(fieldViews_[replaceIndex]));
						}
						deckZone_.AddToDiscard(field_[replaceIndex]);
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

				if (fieldReplaceDecision.cancelRequested) {
					if (hasPendingCard_) {
						BattleSfxPlayer::PlaySE("SE_CardFlick");
						deckZone_.AddToDiscard(pendingCard_);
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
			case CardInputState::ExecutingSequence:
			{
				// Handled in BattleController::Update
			}
			break;
			case CardInputState::ChoosingEnemyTarget:
			{
				int hoverIndex = cardTargetingController_.PickHoveredEnemy(
					enemyMgr_,
					cam_,
					mouse.x,
					mouse.y,
					static_cast<float>(WinApp::kClientWidth),
					static_cast<float>(WinApp::kClientHeight));

				cardTargetingController_.ClearHighlights(enemyMgr_);
				cardTargetingController_.ApplyHoverHighlight(enemyMgr_, hoverIndex);
                const auto targetDecision = BattleCardInputController::ResolveEnemyTargetInput(
                    hoverIndex,
                    lTrig,
                    rTrig,
                    isPokerDamageTargeting_ && tutorialLockPokerTargetingCancel_);

				if (targetDecision.confirmRequested) {
					if (cardTargetingController_.IsValidTarget(enemyMgr_, targetDecision.targetIndex)) {
						Enemy& targetEnemy = enemyMgr_->GetEnemies()[targetDecision.targetIndex];
						currentEnemyIndex_ = targetDecision.targetIndex;

						if (isPokerDamageTargeting_) {
							actionSequenceTarget_ = &targetEnemy;
							ExecutePendingAttack_(targetEnemy);
							return;
						}

						const CardInstance& inst = deckZone_.GetHand()[pendingCardHandIndex_];
						const CardDef* def = db_.Find(inst.defId);
						if (def && BeginCardActionSequence_(app, *def, inst, targetEnemy)) {
							cardState_ = CardInputState::ExecutingSequence;
						} else {
							actionSequenceTarget_ = &targetEnemy;
							ExecutePendingAttack_(targetEnemy);
						}
					}
				}

				if (targetDecision.cancelRequested) {
					if (targetDecision.action == BattleCardInputController::TargetAction::LockedCancel) {
						handView_.SetFocusIndex(-1);
						handView_.SetHoverIndex(-1);
						handView_.SetPreviewIndex(-1);
						return;
					}

					handView_.SetFocusIndex(-1);
					cardState_ = CardInputState::Idle;

					if (isPokerDamageTargeting_) {
						pokerChoiceState_ = PokerChoiceState::WaitingEffectChoice;
					}
				}
			}
			break;
			}
		}

	} else {
		// --------------------------------------------------
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
				while (currentEnemyIndex_ < enemies.size() &&
					(!enemies[currentEnemyIndex_].IsAlive() ||
						enemyActionCountSystem_.ShouldSkipEnemyTurn(currentEnemyIndex_))) {
					currentEnemyIndex_++;
				}

				if (currentEnemyIndex_ < enemies.size()) {

					Enemy& e = enemies[currentEnemyIndex_];
					EnemyAction action = e.GetBossAI().GetNextAction();

					ExecuteEnemyAction_(e, action);
					enemyActionCountSystem_.MarkActedByCount(currentEnemyIndex_);

					enemyWait_ = 1.0f;

					currentEnemyIndex_++;

				} else {

					if (enemyMgr_) {
						for (auto& enemy : enemyMgr_->GetEnemies()) {
							if (enemy.IsAlive()) {
								enemy.TurnEndApplyBC();

								// SpawnDamagePopup(enemy.GetPos(), effect.value, false); 
							}
						}
					}

					currentEnemyIndex_ = 0;

					turn_ = TurnState::Player;
					StartPlayerTurn_();
				}
			}
		}
	}

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
	if (player_) {
		playerLastHp_ = player_->GetHP();
	}

}

void BattleController::UpdateVisuals_(float dt)
{
	sPokerGlowRainbowTime += dt;

	const bool actionOrEnemyAttack =
		actionDirector_.IsPlaying() ||
		cardState_ == CardInputState::ExecutingSequence ||
		turn_ == TurnState::Enemy;

	if (actionOrEnemyAttack) {
		UpdatePoisonIdleEffects_(dt);
		UpdateFrostIdleEffects_(dt);
		UpdateFieldFrameEffects_();
		for (auto& cardView : fieldViews_) {
			if (cardView) {
				cardView->Update(dt);
			}
		}
		handView_.Update(dt);
		if (discardView_) {
			discardView_->Update(dt);
		}

		damagePopupUi_.Update(dt);
		UpdateEnemyStatusLayout_();
		return;
	}

	UpdatePoisonIdleEffects_(dt);
	UpdateFrostIdleEffects_(dt);
	UpdateFieldFrameEffects_();

	for (auto& cardView : fieldViews_) {
		if (cardView) {
			cardView->Update(dt);
		}
	}

	if (fieldLayoutDirty_ ||
		cardState_ == CardInputState::ChoosingFieldReplace ||
		pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi)
	{
		RefreshAllFieldCardTransforms_(dt);
	}
	UpdateFieldFrameEffects_();

	UpdateHandPokerPreviewEffects_();
	handView_.Update(dt);

	if (discardView_) discardView_->Update(dt);

	damagePopupUi_.Update(dt);

	UpdateHpGauges();

	Matrix4x4 viewMat = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 projMat = Matrix4x4::MakeOrthographicMatrix((float)WinApp::kClientWidth, (float)WinApp::kClientHeight);

	if (enemyMgr_) {
		auto& enemies = enemyMgr_->GetEnemies();
		enemyStatusUi_.UpdateLayout(enemies, enemyActionCountSystem_.GetCounts(), enemyActionCountSystem_.GetActedFlags(), viewMat, projMat);
	}
	if (highlightFilter_)highlightFilter_->Update(viewMat, projMat);

	if (propManager_) {
		propManager_->Update(dt);

		for (auto& prop : propManager_->GetPropsMutable()) {
			if (prop.name == "Button" || prop.name == "EndTurnButton") {
				if (endTurnButtonHovered_) {
					prop.object->SetIntensity(prop.lightIntensity * 0.3f);
				} else {
					prop.object->SetIntensity(prop.lightIntensity);
				}
				prop.object->Update(dt);
			}
		}
	}
}

void BattleController::UpdateEnemyStatusLayout_()
{
	if (!enemyMgr_) {
		return;
	}

	Matrix4x4 viewMat = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 projMat = Matrix4x4::MakeOrthographicMatrix((float)WinApp::kClientWidth, (float)WinApp::kClientHeight);
	auto& enemies = enemyMgr_->GetEnemies();
	enemyStatusUi_.UpdateLayout(enemies, enemyActionCountSystem_.GetCounts(), enemyActionCountSystem_.GetActedFlags(), viewMat, projMat);
}

