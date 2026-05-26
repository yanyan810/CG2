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
void FieldUi::Update(GameApp& app, const BattleController& battle)
{
	showDescBg_ = false;
	showPokerOptions_ = false;
	pokerHoverIndex_ = -1;
	pokerOptionCount_ = 0;

	DescMode newMode = DescMode::None;
	int newPreviewDefId = -1;
	std::wstring newText;

	const BattleController::CardInputState inputState = battle.GetNowCardInputState();

	const bool isBattleCardPreview =
		(inputState == BattleController::CardInputState::Preview ||
		 inputState == BattleController::CardInputState::Dragging ||
		 inputState == BattleController::CardInputState::Idle) &&
		!battle.HasPokerChoiceUi() &&
		!battle.IsViewingBoardFromPokerUi() &&
		battle.GetPreviewCardDef() != nullptr;

	if (battle.HasPokerChoiceUi()) {
		newMode = DescMode::PokerChoice;
		showDescBg_ = false;
		showPokerOptions_ = true;
		pokerHoverIndex_ = battle.GetPokerMouseChoiceIndex();

		// 旧まとめテキストは使わない
		newText.clear();
		if (cardDescText_) {
			cardDescText_->SetText(L"");
		}

		//// いったん全部空にしておく
		//if (pokerTitleText_) {
		//	pokerTitleText_->SetText(L"");
		//}
		//if (pokerInfoButtonText_) {
		//	pokerInfoButtonText_->SetText(L"");
		//}
		//for (auto& t : pokerOptionTexts_) {
		//	if (t) {
		//		t->SetText(L"");
		//	}
		//}

		// -----------------------------
		// 発動する / しない / 場を見る
		// -----------------------------
		if (battle.IsWaitingActivateChoice()) {
			pokerOptionCount_ = 3;

			if (pokerTitleText_) {
				pokerTitleText_->SetText(L"特殊効果を発動しますか？");
				pokerTitleText_->SetPosition({
					pokerEffectLayout_.activateTitleText.x,
					pokerEffectLayout_.activateTitleText.y
					});
				SetTextScale_(pokerTitleText_.get(), pokerEffectLayout_.activateTitleText.scale);
			}

			if (pokerInfoButtonText_) {
				pokerInfoButtonText_->SetText(L"特殊効果一覧");
				pokerInfoButtonText_->SetPosition({
					pokerEffectLayout_.infoButtonText.x,
					pokerEffectLayout_.infoButtonText.y
					});
				SetTextScale_(pokerInfoButtonText_.get(), pokerEffectLayout_.infoButtonText.scale);
			}

			if (pokerOptionTexts_[0]) {
				pokerOptionTexts_[0]->SetText(L"発動する");
				pokerOptionTexts_[0]->SetPosition({
					pokerEffectLayout_.activateYesText.x,
					pokerEffectLayout_.activateYesText.y
					});
				SetTextScale_(pokerOptionTexts_[0].get(), pokerEffectLayout_.activateYesText.scale);
			}

			if (pokerOptionTexts_[1]) {
				pokerOptionTexts_[1]->SetText(L"発動しない");
				pokerOptionTexts_[1]->SetPosition({
					pokerEffectLayout_.activateNoText.x,
					pokerEffectLayout_.activateNoText.y
					});
				SetTextScale_(pokerOptionTexts_[1].get(), pokerEffectLayout_.activateNoText.scale);
			}

			if (pokerOptionTexts_[2]) {
				pokerOptionTexts_[2]->SetText(L"場を見る");
				pokerOptionTexts_[2]->SetPosition({
					pokerEffectLayout_.activateViewBoardText.x,
					pokerEffectLayout_.activateViewBoardText.y
					});
				SetTextScale_(pokerOptionTexts_[2].get(), pokerEffectLayout_.activateViewBoardText.scale);
			}
		}
		// -----------------------------
		// 戻る / 3つの効果 / 場を見る
		// -----------------------------
		else if (battle.IsWaitingEffectChoice()) {
			pokerOptionCount_ = 5;

			const BattleController::PokerBonus bonus = battle.GetCurrentPokerBonusForUi();

			if (pokerTitleText_) {
				pokerTitleText_->SetText(L"効果を選んでください");
				pokerTitleText_->SetPosition({
					pokerEffectLayout_.effectTitleText.x,
					pokerEffectLayout_.effectTitleText.y
					});
				SetTextScale_(pokerTitleText_.get(), pokerEffectLayout_.effectTitleText.scale);
			}

			if (pokerInfoButtonText_) {
				pokerInfoButtonText_->SetText(L"特殊効果一覧");
				pokerInfoButtonText_->SetPosition({
					pokerEffectLayout_.infoButtonText.x,
					pokerEffectLayout_.infoButtonText.y
					});
				SetTextScale_(pokerInfoButtonText_.get(), pokerEffectLayout_.infoButtonText.scale);
			}

			if (pokerOptionTexts_[0]) {
				pokerOptionTexts_[0]->SetText(L"戻る");
				pokerOptionTexts_[0]->SetPosition({
					pokerEffectLayout_.backText.x,
					pokerEffectLayout_.backText.y
					});
				SetTextScale_(pokerOptionTexts_[0].get(), pokerEffectLayout_.backText.scale);
			}

			// ここは表示順を見本に合わせる
			// 左  : ATK UP
			// 中央: ダメージ
			// 右  : ドロー
			if (pokerOptionTexts_[1]) {
				pokerOptionTexts_[1]->SetText(L"次ターンATK UP +" + std::to_wstring(bonus.atkUp));
				pokerOptionTexts_[1]->SetPosition({
					pokerEffectLayout_.effectTexts[0].x,
					pokerEffectLayout_.effectTexts[0].y
					});
				SetTextScale_(pokerOptionTexts_[1].get(), pokerEffectLayout_.effectTexts[0].scale);
			}

			if (pokerOptionTexts_[2]) {
				pokerOptionTexts_[2]->SetText(L"敵単体に" + std::to_wstring(bonus.damage) + L"ダメージ");
				pokerOptionTexts_[2]->SetPosition({
					pokerEffectLayout_.effectTexts[1].x,
					pokerEffectLayout_.effectTexts[1].y
					});
				SetTextScale_(pokerOptionTexts_[2].get(), pokerEffectLayout_.effectTexts[1].scale);
			}

			if (pokerOptionTexts_[3]) {
				pokerOptionTexts_[3]->SetText(std::to_wstring(bonus.drawCount) + L"枚ドロー");
				pokerOptionTexts_[3]->SetPosition({
					pokerEffectLayout_.effectTexts[2].x,
					pokerEffectLayout_.effectTexts[2].y
					});
				SetTextScale_(pokerOptionTexts_[3].get(), pokerEffectLayout_.effectTexts[2].scale);
			}

			if (pokerOptionTexts_[4]) {
				pokerOptionTexts_[4]->SetText(L"場を見る");
				pokerOptionTexts_[4]->SetPosition({
					pokerEffectLayout_.effectViewBoardText.x,
					pokerEffectLayout_.effectViewBoardText.y
					});
				SetTextScale_(pokerOptionTexts_[4].get(), pokerEffectLayout_.effectViewBoardText.scale);
			}
		}

		// -----------------------------
		// 右の効果一覧パネル
		// -----------------------------
		const bool previewVisible = battle.IsPokerQuickPreviewVisible();
		if (previewVisible) {
			if (pokerPreviewText_) {
				pokerPreviewText_->SetText(battle.GetPokerEffectPreviewText());
				pokerPreviewText_->SetPosition({
					pokerEffectLayout_.previewPanelText.x,
					pokerEffectLayout_.previewPanelText.y
					});
				SetTextScale_(pokerPreviewText_.get(), pokerEffectLayout_.previewPanelText.scale);
			}
		} else {
			if (pokerPreviewText_) {
				pokerPreviewText_->SetText(L"");
			}
		}

		lastPokerPreviewVisible_ = previewVisible;

	

		for (auto& option : pokerEffectValueDigits_) {
			for (auto& d : option) {
				if (d) {
					d->SetColor({ 1.f, 1.f, 1.f, 0.f });
				}
			}
		}
	} else {
		if (battle.IsViewingBoardFromPokerUi()) {
			pokerHoverIndex_ = battle.GetPokerMouseChoiceIndex();

			// 「戻る」テキストの設定
			if (pokerOptionTexts_[0]) {
				pokerOptionTexts_[0]->SetText(L"戻る");
				pokerOptionTexts_[0]->SetPosition({
					pokerEffectLayout_.backText.x,
					pokerEffectLayout_.backText.y
				});
				SetTextScale_(pokerOptionTexts_[0].get(), pokerEffectLayout_.backText.scale);
			}

			activeCardDescText_ = nullptr;

			const CardDef* def = battle.GetPreviewCardDef();
			if (def) {
				newMode = DescMode::CardDesc;
				newPreviewDefId = def->id;
				showDescBg_ = true;

				cardDescBg_->SetPosition({ layout_.cardDescBg.x, layout_.cardDescBg.y });
				cardDescBg_->SetScale({ layout_.cardDescBg.w, layout_.cardDescBg.h, 1.0f });

				if (useImageCardDesc_) {
					newText.clear();
				} else {
					newText = battle.GetPreviewCardDetailText();
					if (cardDescText_) {
						cardDescText_->SetPosition({ layout_.cardDescText.x, layout_.cardDescText.y });
						SetTextScale_(cardDescText_.get(), layout_.cardDescText.scale);
					}
				}
			} else {
				
			}


		} else if (isBattleCardPreview) {
			activeCardDescText_ = nullptr;

			const CardDef* def = battle.GetPreviewCardDef();
			if (def) {
				newMode = DescMode::CardDesc;
				newPreviewDefId = def->id;
				showDescBg_ = true;

				if (cardDescBg_) {
					cardDescBg_->SetPosition({ layout_.cardDescBg.x, layout_.cardDescBg.y });
					cardDescBg_->SetScale({ layout_.cardDescBg.w, layout_.cardDescBg.h, 1.0f });
				}

				if (useImageCardDesc_) {
					
					newText.clear();
				} else {
					newText = battle.GetPreviewCardDetailText();
					if (cardDescText_) {
						cardDescText_->SetPosition({ layout_.cardDescText.x, layout_.cardDescText.y });
						SetTextScale_(cardDescText_.get(), layout_.cardDescText.scale);
					}
				}
			} else {
			
			}
		} else if (battle.ShouldShowOperationUi()) {
			newMode = DescMode::Operation;
			showDescBg_ = true;
			newText = battle.GetOperationUiText();

			cardDescBg_->SetPosition({ 20.0f, 500.0f });
			cardDescBg_->SetScale({ 900.0f, 280.0f, 1.0f });

			cardDescText_->SetPosition({ 40.0f, 520.0f });
			cardDescText_->SetSize({ 1.0f, 1.0f, 1.0f });

		} else {
			
		}
	}

	if (newMode == DescMode::CardDesc) {
		const bool modeChanged = (newMode != lastDescMode_);
		const bool defChanged = (newPreviewDefId != lastPreviewDefId_);
		const bool textChanged = (newText != lastDescText_);

		if (modeChanged || defChanged || textChanged) {
			if (cardDescText_) {
				cardDescText_->SetText(newText);
			}
			lastDescMode_ = newMode;
			lastPreviewDefId_ = newPreviewDefId;
			lastDescText_ = newText;
		}
	} else if (newMode == DescMode::None) {
		if (lastDescMode_ != DescMode::None) {
			if (cardDescText_) {
				cardDescText_->SetText(L"");
			}
			lastDescMode_ = DescMode::None;
			lastPreviewDefId_ = -1;
			lastDescText_.clear();
		}
	} else {
		const bool modeChanged = (newMode != lastDescMode_);
		const bool defChanged = (newPreviewDefId != lastPreviewDefId_);
		const bool textChanged = (newText != lastDescText_);

		if (modeChanged || defChanged || textChanged) {
			if (cardDescText_) {
				cardDescText_->SetText(newText);
			}
			lastDescMode_ = newMode;
			lastPreviewDefId_ = newPreviewDefId;
			lastDescText_ = newText;
		}
	}

	// 2DのEndTurnボタンは隠す（3Dのプロップを使用するため）
	showEndTurnButton_ = false;

	endTurnHovered_ = battle.IsEndTurnButtonHovered();

	if (endTurnButtonBg_) {
		if (endTurnHovered_) {
			endTurnButtonBg_->SetColor({ 0.2f, 0.45f, 1.0f, 1.0f });
			endTurnButtonBg_->SetScale({
				layout_.endTurnBg.w * 1.05f,
				layout_.endTurnBg.h * 1.05f,
				1.0f
				});
		} else {
			endTurnButtonBg_->SetColor({ 0.1f, 0.3f, 0.95f, 0.95f });
			endTurnButtonBg_->SetScale({
				layout_.endTurnBg.w,
				layout_.endTurnBg.h,
				1.0f
				});
		}
	}

	fieldCountText_->SetText(battle.GetCurrentPokerHandUiText());

	//数字用レイアウト
	UpdateNumberSprites_(deckCountDigits_, battle.GetDeckCount(),
		numberLayout_.deckCount.x,
		numberLayout_.deckCount.y,
		numberLayout_.deckCount.scale,
		numberLayout_.deckCount.spacing);

	UpdateNumberSprites_(discardCountDigits_, battle.GetDiscardCount(),
		numberLayout_.discardCount.x,
		numberLayout_.discardCount.y,
		numberLayout_.discardCount.scale,
		numberLayout_.discardCount.spacing);

	UpdateNumberSprites_(handCountDigits_, battle.GetHandCount(),
		numberLayout_.handCount.x,
		numberLayout_.handCount.y,
		numberLayout_.handCount.scale,
		numberLayout_.handCount.spacing);
	//UpdatePokerEffectValueSprites_(battle);

	if (turnText_) {
		turnText_->SetText(battle.GetTurnUiText());
	}
	if (costText_) {
		costText_->SetText(battle.GetEnergyText());
		costText_->SetAlpha(0.0f);
	}
	UpdateCostMeter_(battle);

	if (deckCountText_) {
		deckCountText_->SetText(std::to_wstring(battle.GetDeckCount()));
	}
	if (discardCountText_) {
		discardCountText_->SetText(std::to_wstring(battle.GetDiscardCount()));
	}
	if (handCountText_) {
		handCountText_->SetText(std::to_wstring(battle.GetHandCount()));
	}

	switch (inputState) {
	case BattleController::CardInputState::Preview:
		clickChoiceText_->SetText(L"左クリック : カード決定 右クリック : キャンセル");
		clickChoiceBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.82f });
		clickChoiceBg_->SetScale({ 400.f,50.f,1.f });
		break;
		if (battle.IsTutorialPokerTargetCancelLocked()) {
			clickChoiceText_->SetText(L"左クリック : 敵を選択");
		} else {
			clickChoiceText_->SetText(L"左クリック : 敵を選択   右クリック : キャンセル");
		}

		clickChoiceBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.82f });
		clickChoiceBg_->SetScale({ 400.f,50.f,1.f });
		break;
	case BattleController::CardInputState::ChoosingFieldReplace:
		clickChoiceText_->SetText(L"左クリック : 場のカードを選択して、使ったカードと交換\n   右クリック : 使ったカードをそのまま墓地へ送る");
		clickChoiceBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.82f });
		clickChoiceBg_->SetScale({ 500.f,80.f,1.f });
		break;
	default:
		clickChoiceText_->SetText(L"");
		clickChoiceBg_->SetColor({ 1.f, 1.f, 1.f, 0.f });
		break;
	}

	if (debugCardDescVisible_) {
		showDescBg_ = true;
		if (cardDescBg_) {
			cardDescBg_->SetPosition({ layout_.cardDescBg.x, layout_.cardDescBg.y });
			cardDescBg_->SetScale({ layout_.cardDescBg.w, layout_.cardDescBg.h, 1.0f });
		}
		if (cardDescText_) {
			cardDescText_->SetPosition({ layout_.cardDescText.x, layout_.cardDescText.y });
			SetTextScale_(cardDescText_.get(), layout_.cardDescText.scale);
			cardDescText_->SetText(debugCardDescText_);
		}
	}

	if (debugImageCardDescVisible_ && debugImageCardDescCard_) {
		showDescBg_ = true;

		if (cardDescBg_) {
			cardDescBg_->SetPosition({ layout_.cardDescBg.x, layout_.cardDescBg.y });
			cardDescBg_->SetScale({ layout_.cardDescBg.w, layout_.cardDescBg.h, 1.0f });
		}

	
	}
}


