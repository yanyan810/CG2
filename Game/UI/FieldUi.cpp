#include "FieldUi.h"
#include "GameApp.h"
#include "TextSprite.h"
#include "Sprite.h"
#include "WinApp.h"
#include "CardDatabase.h"

#include <fstream>
#include <nlohmann/json.hpp>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

using json = nlohmann::json;

static std::wstring Utf8ToWStringLocal(const std::string& s)
{
	if (s.empty()) return L"";
	int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring out(size - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), size);
	return out;
}

std::wstring FieldUi::Utf8ToWString_(const std::string& s)
{
	return Utf8ToWStringLocal(s);
}

void FieldUi::BuildStaticPokerUiTexts_()
{
	if (pokerTitleText_) {
		pokerTitleText_->SetText(L"特殊効果を発動しますか？");
	}

	if (pokerOptionTexts_[0]) pokerOptionTexts_[0]->SetText(L"発動する");
	if (pokerOptionTexts_[1]) pokerOptionTexts_[1]->SetText(L"発動しない");
	if (pokerOptionTexts_[2]) pokerOptionTexts_[2]->SetText(L"場を見る");
	if (pokerOptionTexts_[3]) pokerOptionTexts_[3]->SetText(L"");
	if (pokerOptionTexts_[4]) pokerOptionTexts_[4]->SetText(L"");
}

void FieldUi::UpdateDynamicPokerBonusTexts_(const BattleController& battle)
{
	BattleController::PokerBonus bonus = battle.GetCurrentPokerBonusForUi();

	pokerTitleText_->SetText(L"発動する効果を選んでください");
	pokerOptionTexts_[0]->SetText(L"戻る");
	pokerOptionTexts_[1]->SetText(L"次ターンATK UP +" + std::to_wstring(bonus.atkUp));
	pokerOptionTexts_[2]->SetText(std::to_wstring(bonus.drawCount) + L"枚ドロー");
	pokerOptionTexts_[3]->SetText(std::to_wstring(bonus.damage) + L"ダメージ");
	pokerOptionTexts_[4]->SetText(L"場を見る");
}

void FieldUi::SetTextScale_(TextSprite* text, float s)
{
	if (!text) return;
	text->SetSize({ s, s, 1.0f });
}

void FieldUi::ApplyFieldUiLayout_()
{

	if (cardDescBg_) {
		cardDescBg_->SetPosition({ layout_.cardDescBg.x, layout_.cardDescBg.y });
		cardDescBg_->SetScale({ layout_.cardDescBg.w, layout_.cardDescBg.h, 1.0f });
	}
	if (cardDescText_) {
		cardDescText_->SetPosition({ layout_.cardDescText.x, layout_.cardDescText.y });
		SetTextScale_(cardDescText_.get(), layout_.cardDescText.scale);
	}

	if (deckCountBg_) {
		deckCountBg_->SetPosition({ layout_.deckBg.x, layout_.deckBg.y });
		deckCountBg_->SetScale({ layout_.deckBg.w, layout_.deckBg.h, 1.0f });
	}
	if (deckCountText_) {
		deckCountText_->SetPosition({ layout_.deckText.x, layout_.deckText.y });
		SetTextScale_(deckCountText_.get(), layout_.deckText.scale);
	}

	if (discardCountBg_) {
		discardCountBg_->SetPosition({ layout_.discardBg.x, layout_.discardBg.y });
		discardCountBg_->SetScale({ layout_.discardBg.w, layout_.discardBg.h, 1.0f });
	}
	if (discardCountText_) {
		discardCountText_->SetPosition({ layout_.discardText.x, layout_.discardText.y });
		SetTextScale_(discardCountText_.get(), layout_.discardText.scale);
	}

	if (handCountBg_) {
		handCountBg_->SetPosition({ layout_.handBg.x, layout_.handBg.y });
		handCountBg_->SetScale({ layout_.handBg.w, layout_.handBg.h, 1.0f });
	}
	if (handCountText_) {
		handCountText_->SetPosition({ layout_.handText.x, layout_.handText.y });
		SetTextScale_(handCountText_.get(), layout_.handText.scale);
	}

	if (fieldCountBg_) {
		fieldCountBg_->SetPosition({ layout_.fieldBg.x, layout_.fieldBg.y });
		fieldCountBg_->SetScale({ layout_.fieldBg.w, layout_.fieldBg.h, 1.0f });
	}
	if (fieldCountText_) {
		fieldCountText_->SetPosition({ layout_.fieldText.x, layout_.fieldText.y });
		SetTextScale_(fieldCountText_.get(), layout_.fieldText.scale);
	}

	if (turnTextBg_) {
		turnTextBg_->SetPosition({ layout_.turnBg.x, layout_.turnBg.y });
		turnTextBg_->SetScale({ layout_.turnBg.w, layout_.turnBg.h, 1.0f });
	}
	if (turnText_) {
		turnText_->SetPosition({ layout_.turnText.x, layout_.turnText.y });
		SetTextScale_(turnText_.get(), layout_.turnText.scale);
	}

	if (costTextBg_) {
		costTextBg_->SetPosition({ layout_.costBg.x, layout_.costBg.y });
		costTextBg_->SetScale({ layout_.costBg.w, layout_.costBg.h, 1.0f });
		costTextBg_->SetColor({ 0.f,1.f,0.f,0.5f });
	}
	if (costText_) {
		costText_->SetPosition({ layout_.costText.x, layout_.costText.y });
		SetTextScale_(costText_.get(), layout_.costText.scale);
	}

	if (modalOverlayBg_) {
		modalOverlayBg_->SetPosition({ layout_.overlay.x, layout_.overlay.y });
		modalOverlayBg_->SetScale({ layout_.overlay.w, layout_.overlay.h, 1.0f });
	}

	if (endTurnButtonBg_) {
		endTurnButtonBg_->SetPosition({ layout_.endTurnBg.x, layout_.endTurnBg.y });
		endTurnButtonBg_->SetScale({ layout_.endTurnBg.w, layout_.endTurnBg.h, 1.0f });
	}

	if (endTurnButtonText_) {
		endTurnButtonText_->SetPosition({ layout_.endTurnText.x, layout_.endTurnText.y });
		SetTextScale_(endTurnButtonText_.get(), layout_.endTurnText.scale);
	}

	for (auto& [id, sprite] : cardDescSpriteCache_) {
		if (!sprite) continue;
		sprite->SetPosition({ layout_.cardDescText.x, layout_.cardDescText.y });
		SetTextScale_(sprite.get(), layout_.cardDescText.scale);
	}
}

TextSprite* FieldUi::GetOrCreateCardDescSprite_(GameApp& app, const CardDef& def)
{
	auto it = cardDescSpriteCache_.find(def.id);
	if (it != cardDescSpriteCache_.end()) {
		return it->second.get();
	}

	auto sprite = std::make_unique<TextSprite>();
	sprite->Initialize(app.SpriteCom(), app.Dx());
	sprite->SetText(Utf8ToWString_(def.desc));
	sprite->SetPosition({ layout_.cardDescText.x, layout_.cardDescText.y });
	SetTextScale_(sprite.get(), layout_.cardDescText.scale);

	TextSprite* raw = sprite.get();
	cardDescSpriteCache_[def.id] = std::move(sprite);
	return raw;
}

void FieldUi::Initialize(GameApp& app)
{
	cardDescText_ = std::make_unique<TextSprite>();
	cardDescText_->Initialize(app.SpriteCom(), app.Dx());

	cardDescBg_ = std::make_unique<Sprite>();
	cardDescBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	cardDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

	deckCountText_ = std::make_unique<TextSprite>();
	deckCountText_->Initialize(app.SpriteCom(), app.Dx());

	deckCountBg_ = std::make_unique<Sprite>();
	deckCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	deckCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

	discardCountText_ = std::make_unique<TextSprite>();
	discardCountText_->Initialize(app.SpriteCom(), app.Dx());

	discardCountBg_ = std::make_unique<Sprite>();
	discardCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	discardCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

	handCountText_ = std::make_unique<TextSprite>();
	handCountText_->Initialize(app.SpriteCom(), app.Dx());

	handCountBg_ = std::make_unique<Sprite>();
	handCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	handCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

	fieldCountText_ = std::make_unique<TextSprite>();
	fieldCountText_->Initialize(app.SpriteCom(), app.Dx());

	fieldCountBg_ = std::make_unique<Sprite>();
	fieldCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	fieldCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

	pokerTitleText_ = std::make_unique<TextSprite>();
	pokerTitleText_->Initialize(app.SpriteCom(), app.Dx());
	pokerTitleText_->SetSize({ 1.0f, 1.0f, 1.0f });

	for (int i = 0; i < 5; ++i) {
		pokerOptionBgs_[i] = std::make_unique<Sprite>();
		pokerOptionBgs_[i]->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
		pokerOptionBgs_[i]->SetColor({ 0.0f, 0.0f, 0.0f, 0.65f });
		pokerOptionBgs_[i]->SetScale({ 380.0f, 130.0f, 1.0f });

		pokerOptionTexts_[i] = std::make_unique<TextSprite>();
		pokerOptionTexts_[i]->Initialize(app.SpriteCom(), app.Dx());
		pokerOptionTexts_[i]->SetSize({ 1.0f, 1.0f, 1.0f });
	}

	turnText_ = std::make_unique<TextSprite>();
	turnText_->Initialize(app.SpriteCom(), app.Dx());

	turnTextBg_ = std::make_unique<Sprite>();
	turnTextBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	turnTextBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

	costText_ = std::make_unique<TextSprite>();
	costText_->Initialize(app.SpriteCom(), app.Dx());

	costTextBg_ = std::make_unique<Sprite>();
	costTextBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	costTextBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

	modalOverlayBg_ = std::make_unique<Sprite>();
	modalOverlayBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	modalOverlayBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.38f });

	pokerPreviewText_ = std::make_unique<TextSprite>();
	pokerPreviewText_->Initialize(app.SpriteCom(), app.Dx());
	pokerPreviewText_->SetSize({ 0.9f, 0.9f, 1.0f });
	pokerPreviewText_->SetPosition({ 160.0f, 250.0f });

	pokerInfoButtonText_ = std::make_unique<TextSprite>();
	pokerInfoButtonText_->Initialize(app.SpriteCom(), app.Dx());
	pokerInfoButtonText_->SetText(L"効果一覧");
	pokerInfoButtonText_->SetSize({ 0.9f, 0.9f, 1.0f });

	pokerPreviewTitleText_ = std::make_unique<TextSprite>();
	pokerPreviewTitleText_->Initialize(app.SpriteCom(), app.Dx());
	pokerPreviewTitleText_->SetText(L"発動する効果");
	pokerPreviewTitleText_->SetSize({ 1.0f, 1.0f, 1.0f });

	pokerPreviewBg_ = std::make_unique<Sprite>();
	pokerPreviewBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	pokerPreviewBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.82f });

	clickChoiceText_ = std::make_unique<TextSprite>();
	clickChoiceText_->Initialize(app.SpriteCom(), app.Dx());
	clickChoiceText_->SetText(L"");
	clickChoiceText_->SetSize({ 1.0f, 1.0f, 1.0f });
	clickChoiceText_->SetPosition({ 435.f,500.f });

	clickChoiceBg_ = std::make_unique<Sprite>();
	clickChoiceBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	clickChoiceBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.82f });
	clickChoiceBg_->SetScale({ 400.f,50.f,1.f });
	clickChoiceBg_->SetPosition({ 435.f,500.f });

	pokerActivateDescBg_ = std::make_unique<Sprite>();
	pokerActivateDescBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	pokerActivateDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.88f });

	pokerEffectDescBg_ = std::make_unique<Sprite>();
	pokerEffectDescBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	pokerEffectDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.88f });

	//ターン終了ボタンは常に同じ位置に表示する
	endTurnButtonText_ = std::make_unique<TextSprite>();
	endTurnButtonText_->Initialize(app.SpriteCom(), app.Dx());
	endTurnButtonText_->SetText(L"End\nTurn");

	//ターン終了用背景
	endTurnButtonBg_ = std::make_unique<Sprite>();
	endTurnButtonBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	endTurnButtonBg_->SetColor({ 0.1f, 0.3f, 0.95f, 0.95f });

	LoadPokerEffectChoiceLayout(pokerEffectLayoutPath_);
	LoadFieldUiLayout(layoutPath_);
	ApplyFieldUiLayout_();

	BuildStaticPokerUiTexts_();
	cachedPokerBonusRank_ = BattleController::PokerHandRank::None;
}

void FieldUi::Update(GameApp& app, const BattleController& battle)
{
	showDescBg_ = false;
	showPokerOptions_ = false;
	pokerHoverIndex_ = -1;
	pokerOptionCount_ = 0;

	DescMode newMode = DescMode::None;
	int newPreviewDefId = -1;
	std::wstring newText;

	if (battle.HasPokerChoiceUi()) {
		newMode = DescMode::PokerChoice;
		showDescBg_ = true;
		showPokerOptions_ = true;
		pokerHoverIndex_ = battle.GetPokerMouseChoiceIndex();

		if (battle.IsWaitingActivateChoice()) {
			pokerOptionCount_ = 3;

			BuildStaticPokerUiTexts_();

			pokerTitleText_->SetSize({ 1.15f, 1.15f, 1.0f });
			pokerTitleText_->SetPosition({
				pokerEffectLayout_.titleText.x,
				pokerEffectLayout_.titleText.y
				});

			pokerOptionTexts_[0]->SetPosition({
				pokerEffectLayout_.activateYesText.x,
				pokerEffectLayout_.activateYesText.y
				});
			pokerOptionTexts_[1]->SetPosition({
				pokerEffectLayout_.activateNoText.x,
				pokerEffectLayout_.activateNoText.y
				});
			pokerOptionTexts_[2]->SetPosition({
				pokerEffectLayout_.activateViewBoardText.x,
				pokerEffectLayout_.activateViewBoardText.y
				});


			cachedPokerBonusRank_ = BattleController::PokerHandRank::None;
		} else if (battle.IsWaitingEffectChoice()) {
			pokerOptionCount_ = 5;

			auto currentRank = battle.GetCurrentPokerRankForUi();
			UpdateDynamicPokerBonusTexts_(battle);
			cachedPokerBonusRank_ = currentRank;

			pokerTitleText_->SetSize({ 1.05f, 1.05f, 1.0f });
			pokerTitleText_->SetPosition({
				pokerEffectLayout_.titleText.x,
				pokerEffectLayout_.titleText.y
				});

			pokerOptionTexts_[0]->SetPosition({
				pokerEffectLayout_.backText.x,
				pokerEffectLayout_.backText.y
				});
			pokerOptionTexts_[0]->SetSize({ 1.15f, 1.15f, 1.0f });

			pokerOptionTexts_[1]->SetPosition({
				pokerEffectLayout_.effectTexts[0].x,
				pokerEffectLayout_.effectTexts[0].y
				});
			pokerOptionTexts_[2]->SetPosition({
				pokerEffectLayout_.effectTexts[1].x,
				pokerEffectLayout_.effectTexts[1].y
				});
			pokerOptionTexts_[3]->SetPosition({
				pokerEffectLayout_.effectTexts[2].x,
				pokerEffectLayout_.effectTexts[2].y
				});
			pokerOptionTexts_[4]->SetPosition({
	pokerEffectLayout_.effectViewBoardText.x,
	pokerEffectLayout_.effectViewBoardText.y
				});
		}

		for (int i = 0; i < 5; ++i) {
			if (i >= pokerOptionCount_) {
				if (pokerOptionTexts_[i]) {
					pokerOptionTexts_[i]->SetSize({ 1.0f, 1.0f, 1.0f });
				}
				continue;
			}

			if (i == pokerHoverIndex_) {
				if (battle.IsWaitingEffectChoice() && i == 0) {
					pokerOptionTexts_[i]->SetSize({ 1.20f, 1.20f, 1.0f });
				} else {
					pokerOptionTexts_[i]->SetSize({ 1.06f, 1.06f, 1.0f });
				}
			} else {
				if (battle.IsWaitingEffectChoice() && i == 0) {
					pokerOptionTexts_[i]->SetSize({ 1.15f, 1.15f, 1.0f });
				} else {
					pokerOptionTexts_[i]->SetSize({ 1.0f, 1.0f, 1.0f });
				}
			}
		}

		if (pokerInfoButtonText_) {
			pokerInfoButtonText_->SetPosition({
				pokerEffectLayout_.infoButtonText.x,
				pokerEffectLayout_.infoButtonText.y
				});
			pokerInfoButtonText_->SetSize({
				pokerEffectLayout_.infoButtonText.scale,
				pokerEffectLayout_.infoButtonText.scale,
				1.0f
				});
		}

		if (pokerPreviewTitleText_) {
			pokerPreviewTitleText_->SetPosition({
				pokerEffectLayout_.previewPanelTitle.x,
				pokerEffectLayout_.previewPanelTitle.y
				});
			pokerPreviewTitleText_->SetSize({
				pokerEffectLayout_.previewPanelTitle.scale,
				pokerEffectLayout_.previewPanelTitle.scale,
				1.0f
				});
		}



		const bool previewVisible = battle.IsPokerQuickPreviewVisible();

		if (previewVisible && pokerPreviewText_) {
			std::wstring previewText = battle.GetPokerQuickPreviewText();

			if (!lastPokerPreviewVisible_ || previewText != lastPokerPreviewText_) {
				pokerPreviewText_->SetText(previewText);
				lastPokerPreviewText_ = previewText;
			}

			pokerPreviewText_->SetPosition({
				pokerEffectLayout_.previewPanelText.x,
				pokerEffectLayout_.previewPanelText.y
				});
			pokerPreviewText_->SetSize({
				pokerEffectLayout_.previewPanelText.scale,
				pokerEffectLayout_.previewPanelText.scale,
				1.0f
				});
		} else {
			if (lastPokerPreviewVisible_) {
				lastPokerPreviewText_.clear();
				if (pokerPreviewText_) {
					pokerPreviewText_->SetText(L"");
				}
			}
		}

		lastPokerPreviewVisible_ = previewVisible;

	} else {
		if (battle.IsViewingBoardFromPokerUi()) {
			pokerHoverIndex_ = battle.GetPokerMouseChoiceIndex();


			if (pokerOptionTexts_[0]) {
				pokerOptionTexts_[0]->SetText(L"特殊効果選択に戻る");
				pokerOptionTexts_[0]->SetPosition({
					pokerEffectLayout_.backText.x - 40.0f,
					pokerEffectLayout_.backText.y
					});
				pokerOptionTexts_[0]->SetSize({ 1.0f, 1.0f, 1.0f });
			}

			activeCardDescText_ = nullptr;

			const CardDef* def = battle.GetPreviewCardDef();
			if (def) {
				newMode = DescMode::CardDesc;
				newPreviewDefId = def->id;
				showDescBg_ = true;

				cardDescBg_->SetPosition({ layout_.cardDescBg.x, layout_.cardDescBg.y });
				cardDescBg_->SetScale({ layout_.cardDescBg.w, layout_.cardDescBg.h, 1.0f });

				activeCardDescText_ = GetOrCreateCardDescSprite_(app, *def);
				if (activeCardDescText_) {
					activeCardDescText_->SetPosition({ layout_.cardDescText.x, layout_.cardDescText.y });
					SetTextScale_(activeCardDescText_, layout_.cardDescText.scale);
				}
			}
		} else if (battle.ShouldShowOperationUi()) {
			newMode = DescMode::Operation;
			showDescBg_ = true;
			newText = battle.GetOperationUiText();

			cardDescBg_->SetPosition({ 20.0f, 500.0f });
			cardDescBg_->SetScale({ 900.0f, 280.0f, 1.0f });

			cardDescText_->SetPosition({ 40.0f, 520.0f });
			cardDescText_->SetSize({ 1.0f, 1.0f, 1.0f });
		}
	}

	if (newMode == DescMode::CardDesc) {
		lastDescMode_ = newMode;
		lastPreviewDefId_ = newPreviewDefId;
		lastDescText_.clear();
	} else if (newMode == DescMode::None) {
		if (lastDescMode_ != DescMode::None) {
			cardDescText_->SetText(L"");
			lastDescMode_ = DescMode::None;
			lastPreviewDefId_ = -1;
			lastDescText_.clear();
		}
	} else {
		const bool modeChanged = (newMode != lastDescMode_);
		const bool defChanged = (newPreviewDefId != lastPreviewDefId_);
		const bool textChanged = (newText != lastDescText_);

		if (modeChanged || defChanged || textChanged) {
			cardDescText_->SetText(newText);
			lastDescMode_ = newMode;
			lastPreviewDefId_ = newPreviewDefId;
			lastDescText_ = newText;
		}
	}

	showEndTurnButton_ =
		battle.IsPlayerTurn() &&
		!battle.HasPokerChoiceUi() &&
		!battle.IsViewingBoardFromPokerUi() &&
		!battle.IsPlayerTargeting();

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

	deckCountText_->SetText(L"山札:" + std::to_wstring(battle.GetDeckCount()));
	discardCountText_->SetText(L"墓地:" + std::to_wstring(battle.GetDiscardCount()));
	handCountText_->SetText(L"手札:" + std::to_wstring(battle.GetHandCount()));
	fieldCountText_->SetText(battle.GetCurrentPokerHandUiText());

	if (turnText_) {
		turnText_->SetText(battle.GetTurnUiText());
	}
	if (costText_) {
		costText_->SetText(battle.GetEnergyText());
	}

	BattleController::CardInputState inputState = battle.GetNowCardInputState();


	switch (inputState) {
	case BattleController::CardInputState::Preview:
		clickChoiceText_->SetText(L"左クリック : カード決定 右クリック : キャンセル");
		clickChoiceBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.82f });
		clickChoiceBg_->SetScale({ 400.f,50.f,1.f });
		break;
	case BattleController::CardInputState::ChoosingEnemyTarget:
		clickChoiceText_->SetText(L"左クリック : 敵を選択   右クリック : キャンセル");
		clickChoiceBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.82f });
		clickChoiceBg_->SetScale({ 400.f,50.f,1.f });
		break;
	case BattleController::CardInputState::ChoosingFieldReplace:
		clickChoiceText_->SetText(L"左クリック : 場のカードを選択して、使ったカードと交換\n   右クリック : 使ったカードをそのまま墓地へ送る");
		clickChoiceBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.82f });
		clickChoiceBg_->SetScale({ 500.f,80.f,1.f });
		break;
	default:
		// それ以外の状態（何も表示しないとき）
		clickChoiceText_->SetText(L"");
		clickChoiceBg_->SetColor({ 1.f, 1.f, 1.f, 0.f });
		break;
	}

}

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

	// ==============================
	// ポーカーUI
	// ==============================
	if (showPokerOptions_) {

		if (modalOverlayBg_) {
			modalOverlayBg_->Update(view, proj);
			modalOverlayBg_->Draw();
		}

		// 2択UI
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
					pokerHoverIndex_ == i ?
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

		// 4択UI
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

			for (int i = 0; i < 3; ++i) {
				if (!pokerOptionBgs_[i + 1]) continue;

				pokerOptionBgs_[i + 1]->SetColor(
					pokerHoverIndex_ == (i + 1) ?
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
					pokerHoverIndex_ == 4 ?
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

		if (pokerTitleText_) {
			pokerTitleText_->Update(view, proj);
			pokerTitleText_->Draw();
		}

		for (int i = 0; i < pokerOptionCount_; ++i) {
			if (pokerOptionTexts_[i]) {
				pokerOptionTexts_[i]->Update(view, proj);
				pokerOptionTexts_[i]->Draw();
			}
		}

		if (pokerInfoButtonText_) {
			if (pokerPreviewBg_) {
				// 右上のボタン背景として使わず、
				// ここでは別の既存bgがないので pokerOptionBgs_[0] は使わない方が安全
			}
		}

		if (showPokerOptions_) {
			// 右上ボタン背景
			if (cardDescBg_) {
				cardDescBg_->SetPosition({
					pokerEffectLayout_.infoButtonRect.x,
					pokerEffectLayout_.infoButtonRect.y
					});
				cardDescBg_->SetScale({
					pokerEffectLayout_.infoButtonRect.w,
					pokerEffectLayout_.infoButtonRect.h,
					1.0f
					});
				cardDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.80f });
				cardDescBg_->Update(view, proj);
				cardDescBg_->Draw();
			}

			if (pokerInfoButtonText_) {
				pokerInfoButtonText_->Update(view, proj);
				pokerInfoButtonText_->Draw();
			}

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

				if (pokerPreviewTitleText_) {
					pokerPreviewTitleText_->Update(view, proj);
					pokerPreviewTitleText_->Draw();
				}

				if (pokerPreviewText_) {
					pokerPreviewText_->Update(view, proj);
					pokerPreviewText_->Draw();
				}
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

		if (pokerOptionTexts_[0]) {
			pokerOptionTexts_[0]->Update(view, proj);
			pokerOptionTexts_[0]->Draw();
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

	if (activeCardDescText_) {
		activeCardDescText_->Update(view, proj);
		activeCardDescText_->Draw();
	} else if (cardDescText_) {
		cardDescText_->Update(view, proj);
		cardDescText_->Draw();
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
	if (fieldCountText_) {
		fieldCountText_->Update(view, proj);
		fieldCountText_->Draw();
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

#ifdef USE_IMGUI
void FieldUi::DrawImGui()
{
	if (ImGui::TreeNode("PokerEffectChoiceLayout")) {
		bool changed = false;

		changed |= ImGui::DragFloat2("Title", &pokerEffectLayout_.titleText.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("Activate Choice");
		changed |= ImGui::DragFloat4("Activate Title Bg", &pokerEffectLayout_.activateTitleBg.x, 1.0f);
		changed |= ImGui::DragFloat4("Yes Rect", &pokerEffectLayout_.activateYesRect.x, 1.0f);
		changed |= ImGui::DragFloat2("Yes Text", &pokerEffectLayout_.activateYesText.x, 1.0f);
		changed |= ImGui::DragFloat4("No Rect", &pokerEffectLayout_.activateNoRect.x, 1.0f);
		changed |= ImGui::DragFloat2("No Text", &pokerEffectLayout_.activateNoText.x, 1.0f);
		changed |= ImGui::DragFloat4("ViewBoard Rect", &pokerEffectLayout_.activateViewBoardRect.x, 1.0f);
		changed |= ImGui::DragFloat2("ViewBoard Text", &pokerEffectLayout_.activateViewBoardText.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("Effect Choice");
		changed |= ImGui::DragFloat4("Effect Title Bg", &pokerEffectLayout_.effectTitleBg.x, 1.0f);

		changed |= ImGui::DragFloat4("Back Rect", &pokerEffectLayout_.backRect.x, 1.0f);
		changed |= ImGui::DragFloat2("Back Text", &pokerEffectLayout_.backText.x, 1.0f);

		changed |= ImGui::DragFloat4("Effect1 Rect", &pokerEffectLayout_.effectRects[0].x, 1.0f);
		changed |= ImGui::DragFloat2("Effect1 Text", &pokerEffectLayout_.effectTexts[0].x, 1.0f);

		changed |= ImGui::DragFloat4("Effect2 Rect", &pokerEffectLayout_.effectRects[1].x, 1.0f);
		changed |= ImGui::DragFloat2("Effect2 Text", &pokerEffectLayout_.effectTexts[1].x, 1.0f);

		changed |= ImGui::DragFloat4("Effect3 Rect", &pokerEffectLayout_.effectRects[2].x, 1.0f);
		changed |= ImGui::DragFloat2("Effect3 Text", &pokerEffectLayout_.effectTexts[2].x, 1.0f);

		changed |= ImGui::DragFloat4("Effect ViewBoard Rect", &pokerEffectLayout_.effectViewBoardRect.x, 1.0f);
		changed |= ImGui::DragFloat2("Effect ViewBoard Text", &pokerEffectLayout_.effectViewBoardText.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("InfoButton");
		changed |= ImGui::DragFloat4("InfoButton Rect", &pokerEffectLayout_.infoButtonRect.x, 1.0f);
		changed |= ImGui::DragFloat3("InfoButton Text", &pokerEffectLayout_.infoButtonText.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("PreviewPanel");
		changed |= ImGui::DragFloat4("PreviewPanel Bg", &pokerEffectLayout_.previewPanelBg.x, 1.0f);
		changed |= ImGui::DragFloat3("PreviewPanel Title", &pokerEffectLayout_.previewPanelTitle.x, 1.0f);
		changed |= ImGui::DragFloat3("PreviewPanel Text", &pokerEffectLayout_.previewPanelText.x, 1.0f);

		if (changed) {
			// PokerUI は Update / Draw が毎フレーム layout を見るのでこれで即反映
		}

		if (ImGui::Button("Save PokerEffectChoiceLayout")) {
			SavePokerEffectChoiceLayout(pokerEffectLayoutPath_);
		}
		ImGui::SameLine();
		if (ImGui::Button("Load PokerEffectChoiceLayout")) {
			LoadPokerEffectChoiceLayout(pokerEffectLayoutPath_);
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("FieldUiLayout")) {
		bool changed = false;

		changed |= ImGui::DragFloat4("cardDescBg", &layout_.cardDescBg.x, 1.0f);
		changed |= ImGui::DragFloat3("cardDescText", &layout_.cardDescText.x, 1.0f);

		changed |= ImGui::DragFloat4("deckBg", &layout_.deckBg.x, 1.0f);
		changed |= ImGui::DragFloat3("deckText", &layout_.deckText.x, 1.0f);

		changed |= ImGui::DragFloat4("discardBg", &layout_.discardBg.x, 1.0f);
		changed |= ImGui::DragFloat3("discardText", &layout_.discardText.x, 1.0f);

		changed |= ImGui::DragFloat4("handBg", &layout_.handBg.x, 1.0f);
		changed |= ImGui::DragFloat3("handText", &layout_.handText.x, 1.0f);

		changed |= ImGui::DragFloat4("fieldBg", &layout_.fieldBg.x, 1.0f);
		changed |= ImGui::DragFloat3("fieldText", &layout_.fieldText.x, 1.0f);

		changed |= ImGui::DragFloat4("turnBg", &layout_.turnBg.x, 1.0f);
		changed |= ImGui::DragFloat3("turnText", &layout_.turnText.x, 1.0f);

		changed |= ImGui::DragFloat4("costBg", &layout_.costBg.x, 1.0f);
		changed |= ImGui::DragFloat3("costText", &layout_.costText.x, 1.0f);

		changed |= ImGui::DragFloat4("endTurnBg", &layout_.endTurnBg.x, 1.0f);
		changed |= ImGui::DragFloat3("endTurnText", &layout_.endTurnText.x, 1.0f);

		changed |= ImGui::DragFloat4("overlay", &layout_.overlay.x, 1.0f);

		// Drag中に即反映
		if (changed) {
			ApplyFieldUiLayout_();
		}

		if (ImGui::Button("Save FieldUiLayout")) {
			SaveFieldUiLayout(layoutPath_);
		}
		ImGui::SameLine();
		if (ImGui::Button("Load FieldUiLayout")) {
			LoadFieldUiLayout(layoutPath_);
			ApplyFieldUiLayout_();
		}

		ImGui::TreePop();
	}


}
#endif

bool FieldUi::SavePokerEffectChoiceLayout(const std::string& path) const
{
	json j;

	j["title"]["x"] = pokerEffectLayout_.titleText.x;
	j["title"]["y"] = pokerEffectLayout_.titleText.y;

	j["backButton"]["rect"]["x"] = pokerEffectLayout_.backRect.x;
	j["backButton"]["rect"]["y"] = pokerEffectLayout_.backRect.y;
	j["backButton"]["rect"]["w"] = pokerEffectLayout_.backRect.w;
	j["backButton"]["rect"]["h"] = pokerEffectLayout_.backRect.h;
	j["backButton"]["text"]["x"] = pokerEffectLayout_.backText.x;
	j["backButton"]["text"]["y"] = pokerEffectLayout_.backText.y;

	for (int i = 0; i < 3; ++i) {
		std::string key = "effect" + std::to_string(i + 1);

		j[key]["rect"]["x"] = pokerEffectLayout_.effectRects[i].x;
		j[key]["rect"]["y"] = pokerEffectLayout_.effectRects[i].y;
		j[key]["rect"]["w"] = pokerEffectLayout_.effectRects[i].w;
		j[key]["rect"]["h"] = pokerEffectLayout_.effectRects[i].h;

		j[key]["text"]["x"] = pokerEffectLayout_.effectTexts[i].x;
		j[key]["text"]["y"] = pokerEffectLayout_.effectTexts[i].y;
	}

	j["infoButton"]["rect"]["x"] = pokerEffectLayout_.infoButtonRect.x;
	j["infoButton"]["rect"]["y"] = pokerEffectLayout_.infoButtonRect.y;
	j["infoButton"]["rect"]["w"] = pokerEffectLayout_.infoButtonRect.w;
	j["infoButton"]["rect"]["h"] = pokerEffectLayout_.infoButtonRect.h;
	j["infoButton"]["text"]["x"] = pokerEffectLayout_.infoButtonText.x;
	j["infoButton"]["text"]["y"] = pokerEffectLayout_.infoButtonText.y;
	j["infoButton"]["text"]["scale"] = pokerEffectLayout_.infoButtonText.scale;

	j["previewPanelBg"]["x"] = pokerEffectLayout_.previewPanelBg.x;
	j["previewPanelBg"]["y"] = pokerEffectLayout_.previewPanelBg.y;
	j["previewPanelBg"]["w"] = pokerEffectLayout_.previewPanelBg.w;
	j["previewPanelBg"]["h"] = pokerEffectLayout_.previewPanelBg.h;

	j["previewPanelTitle"]["x"] = pokerEffectLayout_.previewPanelTitle.x;
	j["previewPanelTitle"]["y"] = pokerEffectLayout_.previewPanelTitle.y;
	j["previewPanelTitle"]["scale"] = pokerEffectLayout_.previewPanelTitle.scale;

	j["previewPanelText"]["x"] = pokerEffectLayout_.previewPanelText.x;
	j["previewPanelText"]["y"] = pokerEffectLayout_.previewPanelText.y;
	j["previewPanelText"]["scale"] = pokerEffectLayout_.previewPanelText.scale;

	j["activateTitleBg"]["x"] = pokerEffectLayout_.activateTitleBg.x;
	j["activateTitleBg"]["y"] = pokerEffectLayout_.activateTitleBg.y;
	j["activateTitleBg"]["w"] = pokerEffectLayout_.activateTitleBg.w;
	j["activateTitleBg"]["h"] = pokerEffectLayout_.activateTitleBg.h;

	j["activateYes"]["rect"]["x"] = pokerEffectLayout_.activateYesRect.x;
	j["activateYes"]["rect"]["y"] = pokerEffectLayout_.activateYesRect.y;
	j["activateYes"]["rect"]["w"] = pokerEffectLayout_.activateYesRect.w;
	j["activateYes"]["rect"]["h"] = pokerEffectLayout_.activateYesRect.h;
	j["activateYes"]["text"]["x"] = pokerEffectLayout_.activateYesText.x;
	j["activateYes"]["text"]["y"] = pokerEffectLayout_.activateYesText.y;

	j["activateNo"]["rect"]["x"] = pokerEffectLayout_.activateNoRect.x;
	j["activateNo"]["rect"]["y"] = pokerEffectLayout_.activateNoRect.y;
	j["activateNo"]["rect"]["w"] = pokerEffectLayout_.activateNoRect.w;
	j["activateNo"]["rect"]["h"] = pokerEffectLayout_.activateNoRect.h;
	j["activateNo"]["text"]["x"] = pokerEffectLayout_.activateNoText.x;
	j["activateNo"]["text"]["y"] = pokerEffectLayout_.activateNoText.y;

	j["activateViewBoard"]["rect"]["x"] = pokerEffectLayout_.activateViewBoardRect.x;
	j["activateViewBoard"]["rect"]["y"] = pokerEffectLayout_.activateViewBoardRect.y;
	j["activateViewBoard"]["rect"]["w"] = pokerEffectLayout_.activateViewBoardRect.w;
	j["activateViewBoard"]["rect"]["h"] = pokerEffectLayout_.activateViewBoardRect.h;
	j["activateViewBoard"]["text"]["x"] = pokerEffectLayout_.activateViewBoardText.x;
	j["activateViewBoard"]["text"]["y"] = pokerEffectLayout_.activateViewBoardText.y;

	j["effectTitleBg"]["x"] = pokerEffectLayout_.effectTitleBg.x;
	j["effectTitleBg"]["y"] = pokerEffectLayout_.effectTitleBg.y;
	j["effectTitleBg"]["w"] = pokerEffectLayout_.effectTitleBg.w;
	j["effectTitleBg"]["h"] = pokerEffectLayout_.effectTitleBg.h;

	j["effectViewBoard"]["rect"]["x"] = pokerEffectLayout_.effectViewBoardRect.x;
	j["effectViewBoard"]["rect"]["y"] = pokerEffectLayout_.effectViewBoardRect.y;
	j["effectViewBoard"]["rect"]["w"] = pokerEffectLayout_.effectViewBoardRect.w;
	j["effectViewBoard"]["rect"]["h"] = pokerEffectLayout_.effectViewBoardRect.h;
	j["effectViewBoard"]["text"]["x"] = pokerEffectLayout_.effectViewBoardText.x;
	j["effectViewBoard"]["text"]["y"] = pokerEffectLayout_.effectViewBoardText.y;

	std::ofstream ofs(path);
	if (!ofs.is_open()) {
		return false;
	}

	ofs << j.dump(4);
	return true;
}

bool FieldUi::LoadPokerEffectChoiceLayout(const std::string& path)
{
	std::ifstream ifs(path);
	if (!ifs.is_open()) {
		pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();
		return false;
	}

	json j;
	try {
		ifs >> j;
	}
	catch (...) {
		pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();
		return false;
	}

	pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();

	pokerEffectLayout_.titleText.x = j.value("title", json::object()).value("x", pokerEffectLayout_.titleText.x);
	pokerEffectLayout_.titleText.y = j.value("title", json::object()).value("y", pokerEffectLayout_.titleText.y);

	if (j.contains("backButton")) {
		auto& b = j["backButton"];
		if (b.contains("rect")) {
			pokerEffectLayout_.backRect.x = b["rect"].value("x", pokerEffectLayout_.backRect.x);
			pokerEffectLayout_.backRect.y = b["rect"].value("y", pokerEffectLayout_.backRect.y);
			pokerEffectLayout_.backRect.w = b["rect"].value("w", pokerEffectLayout_.backRect.w);
			pokerEffectLayout_.backRect.h = b["rect"].value("h", pokerEffectLayout_.backRect.h);
		}
		if (b.contains("text")) {
			pokerEffectLayout_.backText.x = b["text"].value("x", pokerEffectLayout_.backText.x);
			pokerEffectLayout_.backText.y = b["text"].value("y", pokerEffectLayout_.backText.y);
		}
	}

	for (int i = 0; i < 3; ++i) {
		std::string key = "effect" + std::to_string(i + 1);
		if (!j.contains(key)) {
			continue;
		}

		auto& e = j[key];
		if (e.contains("rect")) {
			pokerEffectLayout_.effectRects[i].x = e["rect"].value("x", pokerEffectLayout_.effectRects[i].x);
			pokerEffectLayout_.effectRects[i].y = e["rect"].value("y", pokerEffectLayout_.effectRects[i].y);
			pokerEffectLayout_.effectRects[i].w = e["rect"].value("w", pokerEffectLayout_.effectRects[i].w);
			pokerEffectLayout_.effectRects[i].h = e["rect"].value("h", pokerEffectLayout_.effectRects[i].h);
		}
		if (e.contains("text")) {
			pokerEffectLayout_.effectTexts[i].x = e["text"].value("x", pokerEffectLayout_.effectTexts[i].x);
			pokerEffectLayout_.effectTexts[i].y = e["text"].value("y", pokerEffectLayout_.effectTexts[i].y);
		}
	}

	if (j.contains("infoButton")) {
		auto& ib = j["infoButton"];
		if (ib.contains("rect")) {
			pokerEffectLayout_.infoButtonRect.x = ib["rect"].value("x", pokerEffectLayout_.infoButtonRect.x);
			pokerEffectLayout_.infoButtonRect.y = ib["rect"].value("y", pokerEffectLayout_.infoButtonRect.y);
			pokerEffectLayout_.infoButtonRect.w = ib["rect"].value("w", pokerEffectLayout_.infoButtonRect.w);
			pokerEffectLayout_.infoButtonRect.h = ib["rect"].value("h", pokerEffectLayout_.infoButtonRect.h);
		}
		if (ib.contains("text")) {
			pokerEffectLayout_.infoButtonText.x = ib["text"].value("x", pokerEffectLayout_.infoButtonText.x);
			pokerEffectLayout_.infoButtonText.y = ib["text"].value("y", pokerEffectLayout_.infoButtonText.y);
			pokerEffectLayout_.infoButtonText.scale = ib["text"].value("scale", pokerEffectLayout_.infoButtonText.scale);
		}
	}

	if (j.contains("previewPanelBg")) {
		pokerEffectLayout_.previewPanelBg.x = j["previewPanelBg"].value("x", pokerEffectLayout_.previewPanelBg.x);
		pokerEffectLayout_.previewPanelBg.y = j["previewPanelBg"].value("y", pokerEffectLayout_.previewPanelBg.y);
		pokerEffectLayout_.previewPanelBg.w = j["previewPanelBg"].value("w", pokerEffectLayout_.previewPanelBg.w);
		pokerEffectLayout_.previewPanelBg.h = j["previewPanelBg"].value("h", pokerEffectLayout_.previewPanelBg.h);
	}

	if (j.contains("previewPanelTitle")) {
		pokerEffectLayout_.previewPanelTitle.x = j["previewPanelTitle"].value("x", pokerEffectLayout_.previewPanelTitle.x);
		pokerEffectLayout_.previewPanelTitle.y = j["previewPanelTitle"].value("y", pokerEffectLayout_.previewPanelTitle.y);
		pokerEffectLayout_.previewPanelTitle.scale = j["previewPanelTitle"].value("scale", pokerEffectLayout_.previewPanelTitle.scale);
	}

	if (j.contains("previewPanelText")) {
		pokerEffectLayout_.previewPanelText.x = j["previewPanelText"].value("x", pokerEffectLayout_.previewPanelText.x);
		pokerEffectLayout_.previewPanelText.y = j["previewPanelText"].value("y", pokerEffectLayout_.previewPanelText.y);
		pokerEffectLayout_.previewPanelText.scale = j["previewPanelText"].value("scale", pokerEffectLayout_.previewPanelText.scale);
	}

	if (j.contains("activateTitleBg")) {
		pokerEffectLayout_.activateTitleBg.x = j["activateTitleBg"].value("x", pokerEffectLayout_.activateTitleBg.x);
		pokerEffectLayout_.activateTitleBg.y = j["activateTitleBg"].value("y", pokerEffectLayout_.activateTitleBg.y);
		pokerEffectLayout_.activateTitleBg.w = j["activateTitleBg"].value("w", pokerEffectLayout_.activateTitleBg.w);
		pokerEffectLayout_.activateTitleBg.h = j["activateTitleBg"].value("h", pokerEffectLayout_.activateTitleBg.h);
	}

	if (j.contains("activateYes")) {
		auto& a = j["activateYes"];
		if (a.contains("rect")) {
			pokerEffectLayout_.activateYesRect.x = a["rect"].value("x", pokerEffectLayout_.activateYesRect.x);
			pokerEffectLayout_.activateYesRect.y = a["rect"].value("y", pokerEffectLayout_.activateYesRect.y);
			pokerEffectLayout_.activateYesRect.w = a["rect"].value("w", pokerEffectLayout_.activateYesRect.w);
			pokerEffectLayout_.activateYesRect.h = a["rect"].value("h", pokerEffectLayout_.activateYesRect.h);
		}
		if (a.contains("text")) {
			pokerEffectLayout_.activateYesText.x = a["text"].value("x", pokerEffectLayout_.activateYesText.x);
			pokerEffectLayout_.activateYesText.y = a["text"].value("y", pokerEffectLayout_.activateYesText.y);
		}
	}

	if (j.contains("activateNo")) {
		auto& a = j["activateNo"];
		if (a.contains("rect")) {
			pokerEffectLayout_.activateNoRect.x = a["rect"].value("x", pokerEffectLayout_.activateNoRect.x);
			pokerEffectLayout_.activateNoRect.y = a["rect"].value("y", pokerEffectLayout_.activateNoRect.y);
			pokerEffectLayout_.activateNoRect.w = a["rect"].value("w", pokerEffectLayout_.activateNoRect.w);
			pokerEffectLayout_.activateNoRect.h = a["rect"].value("h", pokerEffectLayout_.activateNoRect.h);
		}
		if (a.contains("text")) {
			pokerEffectLayout_.activateNoText.x = a["text"].value("x", pokerEffectLayout_.activateNoText.x);
			pokerEffectLayout_.activateNoText.y = a["text"].value("y", pokerEffectLayout_.activateNoText.y);
		}
	}

	if (j.contains("activateViewBoard")) {
		auto& a = j["activateViewBoard"];
		if (a.contains("rect")) {
			pokerEffectLayout_.activateViewBoardRect.x = a["rect"].value("x", pokerEffectLayout_.activateViewBoardRect.x);
			pokerEffectLayout_.activateViewBoardRect.y = a["rect"].value("y", pokerEffectLayout_.activateViewBoardRect.y);
			pokerEffectLayout_.activateViewBoardRect.w = a["rect"].value("w", pokerEffectLayout_.activateViewBoardRect.w);
			pokerEffectLayout_.activateViewBoardRect.h = a["rect"].value("h", pokerEffectLayout_.activateViewBoardRect.h);
		}
		if (a.contains("text")) {
			pokerEffectLayout_.activateViewBoardText.x = a["text"].value("x", pokerEffectLayout_.activateViewBoardText.x);
			pokerEffectLayout_.activateViewBoardText.y = a["text"].value("y", pokerEffectLayout_.activateViewBoardText.y);
		}
	}

	if (j.contains("effectTitleBg")) {
		pokerEffectLayout_.effectTitleBg.x = j["effectTitleBg"].value("x", pokerEffectLayout_.effectTitleBg.x);
		pokerEffectLayout_.effectTitleBg.y = j["effectTitleBg"].value("y", pokerEffectLayout_.effectTitleBg.y);
		pokerEffectLayout_.effectTitleBg.w = j["effectTitleBg"].value("w", pokerEffectLayout_.effectTitleBg.w);
		pokerEffectLayout_.effectTitleBg.h = j["effectTitleBg"].value("h", pokerEffectLayout_.effectTitleBg.h);
	}

	if (j.contains("effectViewBoard")) {
		auto& e = j["effectViewBoard"];
		if (e.contains("rect")) {
			pokerEffectLayout_.effectViewBoardRect.x = e["rect"].value("x", pokerEffectLayout_.effectViewBoardRect.x);
			pokerEffectLayout_.effectViewBoardRect.y = e["rect"].value("y", pokerEffectLayout_.effectViewBoardRect.y);
			pokerEffectLayout_.effectViewBoardRect.w = e["rect"].value("w", pokerEffectLayout_.effectViewBoardRect.w);
			pokerEffectLayout_.effectViewBoardRect.h = e["rect"].value("h", pokerEffectLayout_.effectViewBoardRect.h);
		}
		if (e.contains("text")) {
			pokerEffectLayout_.effectViewBoardText.x = e["text"].value("x", pokerEffectLayout_.effectViewBoardText.x);
			pokerEffectLayout_.effectViewBoardText.y = e["text"].value("y", pokerEffectLayout_.effectViewBoardText.y);
		}
	}

	return true;
}

bool FieldUi::LoadFieldUiLayout(const std::string& path)
{
	std::ifstream f(path);
	if (!f.is_open()) {
		layout_.cardDescBg = { 20.0f, 600.0f, 900.0f, 120.0f };
		layout_.cardDescText = { 40.0f, 620.0f, 1.0f };

		layout_.deckBg = { 20.0f, 310.0f, 150.0f, 60.0f };
		layout_.deckText = { 40.0f, 320.0f, 0.9f };

		layout_.discardBg = { 1100.0f, 350.0f, 150.0f, 60.0f };
		layout_.discardText = { 1120.0f, 350.0f, 0.9f };

		layout_.handBg = { 1000.0f, 640.0f, 250.0f, 60.0f };
		layout_.handText = { 1020.0f, 640.0f, 0.9f };

		layout_.fieldBg = { 540.0f, 250.0f, 250.0f, 60.0f };
		layout_.fieldText = { 600.0f, 250.0f, 0.9f };

		layout_.turnBg = { 490.0f, 25.0f, 250.0f, 60.0f };
		layout_.turnText = { 500.0f, 20.0f, 1.0f };

		layout_.costBg = { 75.0f, 405.0f, 170.0f, 55.0f };
		layout_.costText = { 90.0f, 400.0f, 1.0f };

		layout_.overlay = { 0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight) };
		return false;
	}

	json j;
	try {
		f >> j;
	}
	catch (...) {
		return false;
	}

	auto readRect = [&](const char* key, UiRect& out) {
		if (!j.contains(key)) return;
		auto& v = j[key];
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.w = v.value("w", out.w);
		out.h = v.value("h", out.h);
		};

	auto readText = [&](const char* key, UiText& out) {
		if (!j.contains(key)) return;
		auto& v = j[key];
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.scale = v.value("scale", out.scale);
		};

	layout_.cardDescBg = { 20.0f, 600.0f, 900.0f, 120.0f };
	layout_.cardDescText = { 40.0f, 620.0f, 1.0f };
	layout_.deckBg = { 20.0f, 310.0f, 150.0f, 60.0f };
	layout_.deckText = { 40.0f, 320.0f, 0.9f };
	layout_.discardBg = { 1100.0f, 350.0f, 150.0f, 60.0f };
	layout_.discardText = { 1120.0f, 350.0f, 0.9f };
	layout_.handBg = { 1000.0f, 640.0f, 250.0f, 60.0f };
	layout_.handText = { 1020.0f, 640.0f, 0.9f };
	layout_.fieldBg = { 540.0f, 250.0f, 250.0f, 60.0f };
	layout_.fieldText = { 600.0f, 250.0f, 0.9f };
	layout_.turnBg = { 490.0f, 25.0f, 250.0f, 60.0f };
	layout_.turnText = { 500.0f, 20.0f, 1.0f };
	layout_.costBg = { 75.0f, 405.0f, 170.0f, 55.0f };
	layout_.costText = { 90.0f, 400.0f, 1.0f };
	layout_.overlay = { 0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight) };

	readRect("cardDescBg", layout_.cardDescBg);
	readText("cardDescText", layout_.cardDescText);

	readRect("deckBg", layout_.deckBg);
	readText("deckText", layout_.deckText);

	readRect("discardBg", layout_.discardBg);
	readText("discardText", layout_.discardText);

	readRect("handBg", layout_.handBg);
	readText("handText", layout_.handText);

	readRect("fieldBg", layout_.fieldBg);
	readText("fieldText", layout_.fieldText);

	readRect("turnBg", layout_.turnBg);
	readText("turnText", layout_.turnText);

	readRect("costBg", layout_.costBg);
	readText("costText", layout_.costText);

	readRect("endTurnBg", layout_.endTurnBg);
	readText("endTurnText", layout_.endTurnText);

	readRect("overlay", layout_.overlay);

	return true;
}

bool FieldUi::SaveFieldUiLayout(const std::string& path) const
{
	json j;

	auto writeRect = [&](const char* key, const UiRect& r) {
		j[key]["x"] = r.x;
		j[key]["y"] = r.y;
		j[key]["w"] = r.w;
		j[key]["h"] = r.h;
		};

	auto writeText = [&](const char* key, const UiText& t) {
		j[key]["x"] = t.x;
		j[key]["y"] = t.y;
		j[key]["scale"] = t.scale;
		};

	writeRect("cardDescBg", layout_.cardDescBg);
	writeText("cardDescText", layout_.cardDescText);

	writeRect("deckBg", layout_.deckBg);
	writeText("deckText", layout_.deckText);

	writeRect("discardBg", layout_.discardBg);
	writeText("discardText", layout_.discardText);

	writeRect("handBg", layout_.handBg);
	writeText("handText", layout_.handText);

	writeRect("fieldBg", layout_.fieldBg);
	writeText("fieldText", layout_.fieldText);

	writeRect("turnBg", layout_.turnBg);
	writeText("turnText", layout_.turnText);

	writeRect("costBg", layout_.costBg);
	writeText("costText", layout_.costText);

	writeRect("endTurnBg", layout_.endTurnBg);
	writeText("endTurnText", layout_.endTurnText);

	writeRect("overlay", layout_.overlay);

	std::ofstream ofs(path);
	if (!ofs.is_open()) {
		return false;
	}

	ofs << j.dump(4);
	return true;
}

void FieldUi::UpdateDebugPokerEffectPreview(int hoverIndex)
{
	showDescBg_ = true;
	showPokerOptions_ = true;
	pokerHoverIndex_ = hoverIndex;
	pokerOptionCount_ = 4;

	pokerTitleText_->SetText(L"発動する効果を選んでください");
	pokerTitleText_->SetSize({ 1.05f, 1.05f, 1.0f });
	pokerTitleText_->SetPosition({
		pokerEffectLayout_.titleText.x,
		pokerEffectLayout_.titleText.y
		});

	pokerOptionTexts_[0]->SetText(L"戻る");
	pokerOptionTexts_[0]->SetPosition({
		pokerEffectLayout_.backText.x,
		pokerEffectLayout_.backText.y
		});
	pokerOptionTexts_[0]->SetSize(
		hoverIndex == 0 ?
		Vector3{ 1.20f, 1.20f, 1.0f } :
		Vector3{ 1.15f, 1.15f, 1.0f }
	);

	pokerOptionTexts_[1]->SetText(L"次ターンATK UP");
	pokerOptionTexts_[1]->SetPosition({
		pokerEffectLayout_.effectTexts[0].x,
		pokerEffectLayout_.effectTexts[0].y
		});
	pokerOptionTexts_[1]->SetSize(
		hoverIndex == 1 ?
		Vector3{ 1.06f, 1.06f, 1.0f } :
		Vector3{ 1.0f, 1.0f, 1.0f }
	);

	pokerOptionTexts_[2]->SetText(L"ドロー");
	pokerOptionTexts_[2]->SetPosition({
		pokerEffectLayout_.effectTexts[1].x,
		pokerEffectLayout_.effectTexts[1].y
		});
	pokerOptionTexts_[2]->SetSize(
		hoverIndex == 2 ?
		Vector3{ 1.06f, 1.06f, 1.0f } :
		Vector3{ 1.0f, 1.0f, 1.0f }
	);

	pokerOptionTexts_[3]->SetText(L"ダメージ");
	pokerOptionTexts_[3]->SetPosition({
		pokerEffectLayout_.effectTexts[2].x,
		pokerEffectLayout_.effectTexts[2].y
		});
	pokerOptionTexts_[3]->SetSize(
		hoverIndex == 3 ?
		Vector3{ 1.06f, 1.06f, 1.06f } :
		Vector3{ 1.0f, 1.0f, 1.0f }
	);
}