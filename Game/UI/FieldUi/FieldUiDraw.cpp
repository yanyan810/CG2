#include "../FieldUi.h"
#include "GameApp.h"
#include "TextSprite.h"
#include "Sprite.h"
#include "WinApp.h"
#include "CardDatabase.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

using json = nlohmann::json;
void FieldUi::Draw(GameApp& app, const BattleController& battle)
{
	app.SpriteCom()->SetGraphicsPipelineState();

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
		0, 0,
		float(WinApp::kClientWidth),
		float(WinApp::kClientHeight),
		0, 100
	);

	const BattleController::CardInputState inputState = battle.GetNowCardInputState();
	const bool isBattleCardPreview =
		(inputState == BattleController::CardInputState::Preview ||
		 inputState == BattleController::CardInputState::Dragging ||
		 inputState == BattleController::CardInputState::Idle) &&
		!battle.HasPokerChoiceUi() &&
		!battle.IsViewingBoardFromPokerUi() &&
		battle.GetPreviewCardDef() != nullptr;

	const int drawPokerHoverIndex =
		(forcedPokerHoverIndex_ >= 0) ? forcedPokerHoverIndex_ : pokerHoverIndex_;

	// ==============================
	// ポーカーUI
	// ==============================
	if (showPokerOptions_) {

		if (modalOverlayBg_) {
			modalOverlayBg_->Update(view, proj);
			modalOverlayBg_->Draw();
		}

		// 選択肢まわりの背景だけ残す
		if (pokerOptionCount_ == 3) {
			if (pokerActivateDescBg_) {
				pokerActivateDescBg_->SetPosition({
					pokerEffectLayout_.activateTitleBg.x,
					pokerEffectLayout_.activateTitleBg.y
					});
				pokerActivateDescBg_->SetScale({
					pokerEffectLayout_.activateTitleBg.w,
					pokerEffectLayout_.activateTitleBg.h,
					1.0f
					});
				pokerActivateDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.88f });
				pokerActivateDescBg_->Update(view, proj);
				pokerActivateDescBg_->Draw();
			}

			for (int i = 0; i < 3; ++i) {
				if (!pokerOptionBgs_[i]) continue;

				pokerOptionBgs_[i]->SetColor(
					drawPokerHoverIndex == i ?
					Vector4{ 0.15f, 0.15f, 0.15f, 0.95f } :
					Vector4{ 0.0f, 0.0f, 0.0f, 0.78f }
				);

				if (i == 0) {
					pokerOptionBgs_[i]->SetPosition({
						pokerEffectLayout_.activateYesRect.x,
						pokerEffectLayout_.activateYesRect.y
						});
					pokerOptionBgs_[i]->SetScale({
						pokerEffectLayout_.activateYesRect.w,
						pokerEffectLayout_.activateYesRect.h,
						1.0f
						});
				} else if (i == 1) {
					pokerOptionBgs_[i]->SetPosition({
						pokerEffectLayout_.activateNoRect.x,
						pokerEffectLayout_.activateNoRect.y
						});
					pokerOptionBgs_[i]->SetScale({
						pokerEffectLayout_.activateNoRect.w,
						pokerEffectLayout_.activateNoRect.h,
						1.0f
						});
				} else {
					pokerOptionBgs_[i]->SetPosition({
						pokerEffectLayout_.activateViewBoardRect.x,
						pokerEffectLayout_.activateViewBoardRect.y
						});
					pokerOptionBgs_[i]->SetScale({
						pokerEffectLayout_.activateViewBoardRect.w,
						pokerEffectLayout_.activateViewBoardRect.h,
						1.0f
						});
				}

				pokerOptionBgs_[i]->Update(view, proj);
				pokerOptionBgs_[i]->Draw();
			}
		}

		if (pokerOptionCount_ == 5) {
			if (pokerEffectDescBg_) {
				pokerEffectDescBg_->SetPosition({
					pokerEffectLayout_.effectTitleBg.x,
					pokerEffectLayout_.effectTitleBg.y
					});
				pokerEffectDescBg_->SetScale({
					pokerEffectLayout_.effectTitleBg.w,
					pokerEffectLayout_.effectTitleBg.h,
					1.0f
					});
				pokerEffectDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.88f });
				pokerEffectDescBg_->Update(view, proj);
				pokerEffectDescBg_->Draw();
			}

			if (pokerOptionBgs_[0]) {
				pokerOptionBgs_[0]->SetColor(
					drawPokerHoverIndex == 0 ?
					Vector4{ 0.18f, 0.18f, 0.18f, 0.96f } :
					Vector4{ 0.0f, 0.0f, 0.0f, 0.78f }
				);
				pokerOptionBgs_[0]->SetPosition({
					pokerEffectLayout_.backRect.x,
					pokerEffectLayout_.backRect.y
					});
				pokerOptionBgs_[0]->SetScale({
					pokerEffectLayout_.backRect.w,
					pokerEffectLayout_.backRect.h,
					1.0f
					});
				pokerOptionBgs_[0]->Update(view, proj);
				pokerOptionBgs_[0]->Draw();
			}

			for (int i = 0; i < 3; ++i) {
				if (!pokerOptionBgs_[i + 1]) continue;
				pokerOptionBgs_[i + 1]->SetColor(
					drawPokerHoverIndex == (i + 1) ?
					Vector4{ 0.18f, 0.18f, 0.18f, 0.96f } :
					Vector4{ 0.0f, 0.0f, 0.0f, 0.78f }
				);
				pokerOptionBgs_[i + 1]->SetPosition({
					pokerEffectLayout_.effectRects[i].x,
					pokerEffectLayout_.effectRects[i].y
					});
				pokerOptionBgs_[i + 1]->SetScale({
					pokerEffectLayout_.effectRects[i].w,
					pokerEffectLayout_.effectRects[i].h,
					1.0f
					});
				pokerOptionBgs_[i + 1]->Update(view, proj);
				pokerOptionBgs_[i + 1]->Draw();
			}

			if (pokerOptionBgs_[4]) {
				pokerOptionBgs_[4]->SetColor(
					drawPokerHoverIndex == 4 ?
					Vector4{ 0.18f, 0.18f, 0.18f, 0.96f } :
					Vector4{ 0.0f, 0.0f, 0.0f, 0.78f }
				);
				pokerOptionBgs_[4]->SetPosition({
					pokerEffectLayout_.effectViewBoardRect.x,
					pokerEffectLayout_.effectViewBoardRect.y
					});
				pokerOptionBgs_[4]->SetScale({
					pokerEffectLayout_.effectViewBoardRect.w,
					pokerEffectLayout_.effectViewBoardRect.h,
					1.0f
					});
				pokerOptionBgs_[4]->Update(view, proj);
				pokerOptionBgs_[4]->Draw();
			}
		}

		// タイトル
		if (pokerTitleText_) {
			pokerTitleText_->Update(view, proj);
			pokerTitleText_->Draw();
		}

		if (pokerInfoButtonBg_) {
			pokerInfoButtonBg_->SetPosition({
				pokerEffectLayout_.infoButtonRect.x,
				pokerEffectLayout_.infoButtonRect.y
				});
			pokerInfoButtonBg_->SetScale({
				pokerEffectLayout_.infoButtonRect.w,
				pokerEffectLayout_.infoButtonRect.h,
				1.0f
				});
			pokerInfoButtonBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.78f });
			pokerInfoButtonBg_->Update(view, proj);
			pokerInfoButtonBg_->Draw();
		}

		// 右上の「特殊効果一覧」
		if (pokerInfoButtonText_) {
			pokerInfoButtonText_->Update(view, proj);
			pokerInfoButtonText_->Draw();
		}

		if (pokerInfoButtonText_) {
			pokerInfoButtonText_->Update(view, proj);
			pokerInfoButtonText_->Draw();
		}

		// 各ボタン上の文字
		for (int i = 0; i < pokerOptionCount_; ++i) {
			if (pokerOptionTexts_[i]) {
				pokerOptionTexts_[i]->Update(view, proj);
				pokerOptionTexts_[i]->Draw();
			}
		}

		// プレビューパネル
		if (battle.IsPokerQuickPreviewVisible()) {
			if (pokerPreviewBg_) {
				pokerPreviewBg_->SetPosition({
					pokerEffectLayout_.previewPanelBg.x,
					pokerEffectLayout_.previewPanelBg.y
					});
				pokerPreviewBg_->SetScale({
					pokerEffectLayout_.previewPanelBg.w,
					pokerEffectLayout_.previewPanelBg.h,
					1.0f
					});
				pokerPreviewBg_->Update(view, proj);
				pokerPreviewBg_->Draw();
			}

			if (pokerPreviewText_) {
				pokerPreviewText_->Update(view, proj);
				pokerPreviewText_->Draw();
			}
		}

		return;
	}

	if (battle.IsViewingBoardFromPokerUi()) {
		pokerHoverIndex_ = battle.GetPokerMouseChoiceIndex();
		if (modalOverlayBg_) {
			modalOverlayBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.18f });
			modalOverlayBg_->Update(view, proj);
			modalOverlayBg_->Draw();
		}

		if (pokerOptionBgs_[0]) {
			pokerOptionBgs_[0]->SetColor(
				pokerHoverIndex_ == 0 ?
				Vector4{ 0.18f, 0.18f, 0.18f, 0.96f } :
				Vector4{ 0.0f, 0.0f, 0.0f, 0.78f }
			);
			pokerOptionBgs_[0]->SetPosition({
				pokerEffectLayout_.backRect.x,
				pokerEffectLayout_.backRect.y
				});
			pokerOptionBgs_[0]->SetScale({
				pokerEffectLayout_.backRect.w,
				pokerEffectLayout_.backRect.h,
				1.0f
				});
			pokerOptionBgs_[0]->Update(view, proj);
			pokerOptionBgs_[0]->Draw();
		}

		// 「戻る」テキストの描画
		if (pokerOptionTexts_[0]) {
			pokerOptionTexts_[0]->Update(view, proj);
			pokerOptionTexts_[0]->Draw();
		}
	}

	if (debugShowPokerPreview_ && !showPokerOptions_) {
		if (pokerPreviewBg_) {
			pokerPreviewBg_->SetPosition({
				pokerEffectLayout_.previewPanelBg.x,
				pokerEffectLayout_.previewPanelBg.y
				});
			pokerPreviewBg_->SetScale({
				pokerEffectLayout_.previewPanelBg.w,
				pokerEffectLayout_.previewPanelBg.h,
				1.0f
				});
			pokerPreviewBg_->Update(view, proj);
			pokerPreviewBg_->Draw();
		}

	
	}

	// ==============================
	// 通常UI
	// ==============================

	if (showEndTurnButton_) {
		if (endTurnButtonBg_) {
			endTurnButtonBg_->Update(view, proj);
			endTurnButtonBg_->Draw();
		}
		if (endTurnButtonText_) {
			endTurnButtonText_->Update(view, proj);
			endTurnButtonText_->Draw();
		}
	}

	if (showDescBg_ && cardDescBg_) {
		cardDescBg_->Update(view, proj);
		cardDescBg_->Draw();
	}

	if (deckCountBg_) {
		deckCountBg_->Update(view, proj);
		deckCountBg_->Draw();
	}
	if (discardCountBg_) {
		discardCountBg_->Update(view, proj);
		discardCountBg_->Draw();
	}
	if (handCountBg_) {
		handCountBg_->Update(view, proj);
		handCountBg_->Draw();
	}
	if (fieldCountBg_) {
		fieldCountBg_->Update(view, proj);
		fieldCountBg_->Draw();
	}

	if (turnTextBg_) {
		turnTextBg_->Update(view, proj);
		turnTextBg_->Draw();
	}
	if (costTextBg_) {
		costTextBg_->Update(view, proj);
		costTextBg_->Draw();
	}

	if (clickChoiceBg_) {
		clickChoiceBg_->Update(view, proj);
		clickChoiceBg_->Draw();
	}

	if (useImageCardDesc_ &&
		(battle.IsViewingBoardFromPokerUi() || isBattleCardPreview || debugImageCardDescVisible_)) {
		
	} else if (activeCardDescText_) {
		activeCardDescText_->Update(view, proj);
		activeCardDescText_->Draw();
	} else if (cardDescText_) {
		cardDescText_->Update(view, proj);
		cardDescText_->Draw();
	}

	if (deckLabelImage_) {
		deckLabelImage_->Update(view, proj);
		deckLabelImage_->Draw();
	}
	if (discardLabelImage_) {
		discardLabelImage_->Update(view, proj);
		discardLabelImage_->Draw();
	}
	if (handLabelImage_) {
		handLabelImage_->Update(view, proj);
		handLabelImage_->Draw();
	}

	if (fieldCountText_) {
		fieldCountText_->Update(view, proj);
		fieldCountText_->Draw();
	}

	if (deckLabelText_) {
		deckLabelText_->Update(view, proj);
		deckLabelText_->Draw();
	}
	if (discardLabelText_) {
		discardLabelText_->Update(view, proj);
		discardLabelText_->Draw();
	}
	if (handLabelText_) {
		handLabelText_->Update(view, proj);
		handLabelText_->Draw();
	}

	if (deckCountText_) {
		deckCountText_->Update(view, proj);
		deckCountText_->Draw();
	}
	if (discardCountText_) {
		discardCountText_->Update(view, proj);
		discardCountText_->Draw();
	}
	if (handCountText_) {
		handCountText_->Update(view, proj);
		handCountText_->Draw();
	}

	if (turnText_) {
		turnText_->Update(view, proj);
		turnText_->Draw();
	}
	if (costText_) {
		costText_->Update(view, proj);
		costText_->Draw();
	}

	if (clickChoiceText_) {
		clickChoiceText_->Update(view, proj);
		clickChoiceText_->Draw();
	}
}


