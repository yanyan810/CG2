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

FieldUi::PokerPreviewEffectKind FieldUi::ClassifyPreviewEffectKind_(const std::wstring& line) const
{
	if (line.find(L"敵単体に") != std::wstring::npos &&
		line.find(L"ダメージ") != std::wstring::npos) {
		return PokerPreviewEffectKind::SingleDamage;
	}

	if (line.find(L"敵全体に") != std::wstring::npos &&
		line.find(L"ダメージ") != std::wstring::npos) {
		return PokerPreviewEffectKind::AllDamage;
	}

	if (line.find(L"枚引く") != std::wstring::npos) {
		return PokerPreviewEffectKind::Draw;
	}

	if (line.find(L"ブロック") != std::wstring::npos) {
		return PokerPreviewEffectKind::Block;
	}

	if (line.find(L"回復") != std::wstring::npos) {
		return PokerPreviewEffectKind::Heal;
	}

	return PokerPreviewEffectKind::None;
}

const UiPokerPreviewLineAnchor& FieldUi::GetPreviewEffectAnchor_(
    PokerPreviewEffectKind kind,
    const UiPokerPreviewEffectAnchors& anchors,
    int laneIndex) const
{
    laneIndex = (std::max)(0, (std::min)(laneIndex, 4));

    switch (kind) {
    case PokerPreviewEffectKind::SingleDamage:
        return anchors.singleDamage.lanes[laneIndex];
    case PokerPreviewEffectKind::AllDamage:
        return anchors.allDamage.lanes[laneIndex];
    case PokerPreviewEffectKind::Draw:
        return anchors.draw.lanes[laneIndex];
    case PokerPreviewEffectKind::Block:
        return anchors.block.lanes[laneIndex];
    case PokerPreviewEffectKind::Heal:
        return anchors.heal.lanes[laneIndex];
    default:
        return anchors.none.lanes[laneIndex];
    }
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

	// デッキ
	if (deckCountText_ && deckLabelText_) {
		deckCountText_->SetPosition({
			layout_.deckLabelImage.x + deckCountTextLayout_.offsetX,
			layout_.deckLabelImage.y + deckCountTextLayout_.offsetY
			});
		SetTextScale_(deckCountText_.get(), deckCountTextLayout_.scale);
	}

	if (discardCountBg_) {
		discardCountBg_->SetPosition({ layout_.discardBg.x, layout_.discardBg.y });
		discardCountBg_->SetScale({ layout_.discardBg.w, layout_.discardBg.h, 1.0f });
	}

	// 墓地
	if (discardCountText_ && discardLabelText_) {
		discardCountText_->SetPosition({
			layout_.discardLabelImage.x + discardCountTextLayout_.offsetX,
			layout_.discardLabelImage.y + discardCountTextLayout_.offsetY
			});
		SetTextScale_(discardCountText_.get(), discardCountTextLayout_.scale);
	}

	if (handCountBg_) {
		handCountBg_->SetPosition({ layout_.handBg.x, layout_.handBg.y });
		handCountBg_->SetScale({ layout_.handBg.w, layout_.handBg.h, 1.0f });
	}
	
	// 手札
	if (handCountText_ && handLabelText_) {
		handCountText_->SetPosition({
			layout_.handLabelImage.x + handCountTextLayout_.offsetX,
			layout_.handLabelImage.y + handCountTextLayout_.offsetY
			});
		SetTextScale_(handCountText_.get(), handCountTextLayout_.scale);
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

	if (deckLabelImage_) {
		deckLabelImage_->SetPosition({ layout_.deckLabelImage.x, layout_.deckLabelImage.y });
		deckLabelImage_->SetScale({ layout_.deckLabelImage.scale, layout_.deckLabelImage.scale, 1.0f });
	}

	if (discardLabelImage_) {
		discardLabelImage_->SetPosition({ layout_.discardLabelImage.x, layout_.discardLabelImage.y });
		discardLabelImage_->SetScale({ layout_.discardLabelImage.scale, layout_.discardLabelImage.scale, 1.0f });
	}

	if (handLabelImage_) {
		handLabelImage_->SetPosition({ layout_.handLabelImage.x, layout_.handLabelImage.y });
		handLabelImage_->SetScale({ layout_.handLabelImage.scale, layout_.handLabelImage.scale, 1.0f });
	}

	if (deckLabelText_) {
		deckLabelText_->SetPosition({ layout_.deckLabelImage.x, layout_.deckLabelImage.y });
		SetTextScale_(deckLabelText_.get(), layout_.deckLabelImage.scale);
	}

	if (discardLabelText_) {
		discardLabelText_->SetPosition({ layout_.discardLabelImage.x, layout_.discardLabelImage.y });
		SetTextScale_(discardLabelText_.get(), layout_.discardLabelImage.scale);
	}

	if (handLabelText_) {
		handLabelText_->SetPosition({ layout_.handLabelImage.x, layout_.handLabelImage.y });
		SetTextScale_(handLabelText_.get(), layout_.handLabelImage.scale);
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

void FieldUi::UpdateNumberSprites_(
	std::array<std::unique_ptr<Sprite>, kMaxUiDigits>& digits,
	int value, float x, float y, float scale, float spacing)
{
	std::string text = std::to_string(std::max(0, value));

	for (auto& s : digits) {
		if (s) {
			s->SetColor({ 1.f, 1.f, 1.f, 0.f });
		}
	}

	const int count = static_cast<int>(text.size());
	for (int i = 0; i < count && i < kMaxUiDigits; ++i) {
		char c = text[i];
		std::string path = "resources/ui/num/";
		path += c;
		path += ".png";

		auto& spr = digits[i];
		if (!spr) { continue; }

		spr->SetTextureFilePath(path);
		spr->SetPosition({ x + spacing * i, y });
		spr->SetScale({ scale, scale, 1.0f });
		spr->SetColor({ 1.f, 1.f, 1.f, 1.f });
	}
}

void FieldUi::UpdatePokerEffectValueSprites_(const BattleController& battle)
{
	if (!battle.IsWaitingEffectChoice()) {
		for (auto& option : pokerEffectValueDigits_) {
			for (auto& d : option) {
				if (d) d->SetColor({ 1.f,1.f,1.f,0.f });
			}
		}
		return;
	}

	const BattleController::PokerBonus bonus = battle.GetCurrentPokerBonusForUi();

	// 表示順に合わせる
	// effect[0] = atkUp
	// effect[1] = damage
	// effect[2] = draw
	int effect1Value = bonus.atkUp;
	int effect2Value = bonus.damage;
	int effect3Value = bonus.drawCount;

	UpdateNumberSprites_(pokerEffectValueDigits_[0], effect1Value,
		pokerEffectLayout_.effectImages[0].x + numberLayout_.effectValue[0].offsetX,
		pokerEffectLayout_.effectImages[0].y + numberLayout_.effectValue[0].offsetY,
		numberLayout_.effectValue[0].scale,
		numberLayout_.effectValue[0].spacing);

	UpdateNumberSprites_(pokerEffectValueDigits_[1], effect2Value,
		pokerEffectLayout_.effectImages[1].x + numberLayout_.effectValue[1].offsetX,
		pokerEffectLayout_.effectImages[1].y + numberLayout_.effectValue[1].offsetY,
		numberLayout_.effectValue[1].scale,
		numberLayout_.effectValue[1].spacing);

	UpdateNumberSprites_(pokerEffectValueDigits_[2], effect3Value,
		pokerEffectLayout_.effectImages[2].x + numberLayout_.effectValue[2].offsetX,
		pokerEffectLayout_.effectImages[2].y + numberLayout_.effectValue[2].offsetY,
		numberLayout_.effectValue[2].scale,
		numberLayout_.effectValue[2].spacing);
}



const UiCardDescCustomLayout& FieldUi::GetCardDescCustomLayout_(int cardId) const
{
	auto it = perCardDescCustomLayouts_.find(cardId);
	if (it != perCardDescCustomLayouts_.end()) {
		return it->second;
	}
	return layout_.cardDescCustom;
}

UiCardDescCustomLayout& FieldUi::GetOrCreateCardDescCustomLayout_(int cardId)
{
	auto it = perCardDescCustomLayouts_.find(cardId);
	if (it != perCardDescCustomLayouts_.end()) {
		return it->second;
	}

	perCardDescCustomLayouts_[cardId] = layout_.cardDescCustom;
	return perCardDescCustomLayouts_[cardId];
}

bool FieldUi::IsCustomDescCardId_(int cardId) const
{
	return cardId == 6 || cardId == 17 || cardId == 18 || cardId == 19;
}

const UiCustomDescImageLayout& FieldUi::GetCustomDescImageLayout_(int cardId) const
{
	auto it = perCardCustomDescImageLayouts_.find(cardId);
	if (it != perCardCustomDescImageLayouts_.end()) {
		return it->second;
	}

	static UiCustomDescImageLayout defaultLayout{};
	defaultLayout.x = layout_.cardDescBg.x;
	defaultLayout.y = layout_.cardDescBg.y;
	defaultLayout.scaleX = 1.0f;
	defaultLayout.scaleY = 1.0f;
	return defaultLayout;
}

UiCustomDescImageLayout& FieldUi::GetOrCreateCustomDescImageLayout_(int cardId)
{
	auto it = perCardCustomDescImageLayouts_.find(cardId);
	if (it != perCardCustomDescImageLayouts_.end()) {
		return it->second;
	}

	UiCustomDescImageLayout layout{};
	layout.x = layout_.cardDescBg.x;
	layout.y = layout_.cardDescBg.y;
	layout.scaleX = 1.0f;
	layout.scaleY = 1.0f;

	perCardCustomDescImageLayouts_[cardId] = layout;
	return perCardCustomDescImageLayouts_[cardId];
}

void FieldUi::SetDebugPokerPreviewVisible(bool v)
{
	debugShowPokerPreview_ = v;
}

void FieldUi::SetDebugPokerPreviewData(const DebugPokerPreviewData& data)
{
	debugPokerPreviewData_ = data;
}

void FieldUi::ClearDebugPokerPreviewData()
{
	debugPokerPreviewData_ = DebugPokerPreviewData{};
}

void FieldUi::Initialize(GameApp& app)
{
	app_ = &app;

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

	deckLabelText_ = std::make_unique<TextSprite>();
	deckLabelText_->Initialize(app.SpriteCom(), app.Dx());
	deckLabelText_->SetText(L"デッキ :");

	discardLabelText_ = std::make_unique<TextSprite>();
	discardLabelText_->Initialize(app.SpriteCom(), app.Dx());
	discardLabelText_->SetText(L"墓地 :");

	handLabelText_ = std::make_unique<TextSprite>();
	handLabelText_->Initialize(app.SpriteCom(), app.Dx());
	handLabelText_->SetText(L"手札 :");


	for (int i = 0; i < 5; ++i) {
		pokerOptionBgs_[i] = std::make_unique<Sprite>();
		pokerOptionBgs_[i]->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
		pokerOptionBgs_[i]->SetColor({ 0.0f, 0.0f, 0.0f, 0.65f });
		pokerOptionBgs_[i]->SetScale({ 380.0f, 130.0f, 1.0f });


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

	

	pokerTitleText_ = std::make_unique<TextSprite>();
	pokerTitleText_->Initialize(app.SpriteCom(), app.Dx());

	pokerInfoButtonText_ = std::make_unique<TextSprite>();
	pokerInfoButtonText_->Initialize(app.SpriteCom(), app.Dx());

	for (int i = 0; i < 5; ++i) {
		pokerOptionTexts_[i] = std::make_unique<TextSprite>();
		pokerOptionTexts_[i]->Initialize(app.SpriteCom(), app.Dx());
	}

	//==================
	// レイアウトの読み込みと適用
	//==================
	LoadPokerEffectChoiceLayout(pokerEffectLayoutPath_);
	LoadFieldUiLayout(layoutPath_);
	LoadCardShowUiLayout(cardShowLayoutPath_);
	LoadUiNumberLayout(numberLayoutPath_);
	ApplyFieldUiLayout_();

	//	cachedPokerBonusRank_ = BattleController::PokerHandRank::None;
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

	const BattleController::CardInputState inputState = battle.GetNowCardInputState();

	const bool isBattleCardPreview =
		(inputState == BattleController::CardInputState::Preview) &&
		!battle.HasPokerChoiceUi() &&
		!battle.IsViewingBoardFromPokerUi();

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
	}

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
		(inputState == BattleController::CardInputState::Preview) &&
		!battle.HasPokerChoiceUi() &&
		!battle.IsViewingBoardFromPokerUi();

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

#ifdef USE_IMGUI
void FieldUi::DrawImGui()
{
	if (ImGui::TreeNode("PokerEffectChoiceLayout")) {
		bool changed = false;

		changed |= ImGui::DragFloat2("Title Image", &pokerEffectLayout_.titleImage.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("Activate Choice");
		changed |= ImGui::DragFloat4("Activate Title Bg", &pokerEffectLayout_.activateTitleBg.x, 1.0f);
		changed |= ImGui::DragFloat4("Yes Rect", &pokerEffectLayout_.activateYesRect.x, 1.0f);
		changed |= ImGui::DragFloat2("Yes Image", &pokerEffectLayout_.activateYesImage.x, 1.0f);
		changed |= ImGui::DragFloat4("No Rect", &pokerEffectLayout_.activateNoRect.x, 1.0f);
		changed |= ImGui::DragFloat2("No Image", &pokerEffectLayout_.activateNoImage.x, 1.0f);
		changed |= ImGui::DragFloat4("ViewBoard Rect", &pokerEffectLayout_.activateViewBoardRect.x, 1.0f);
		changed |= ImGui::DragFloat2("ViewBoard Image", &pokerEffectLayout_.activateViewBoardImage.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("Effect Choice");
		changed |= ImGui::DragFloat4("Effect Title Bg", &pokerEffectLayout_.effectTitleBg.x, 1.0f);

		changed |= ImGui::DragFloat4("Back Rect", &pokerEffectLayout_.backRect.x, 1.0f);
		changed |= ImGui::DragFloat2("Back Image", &pokerEffectLayout_.backImage.x, 1.0f);

		changed |= ImGui::DragFloat4("Effect1 Rect", &pokerEffectLayout_.effectRects[0].x, 1.0f);
		changed |= ImGui::DragFloat2("Effect1 Image", &pokerEffectLayout_.effectImages[0].x, 1.0f);

		changed |= ImGui::DragFloat4("Effect2 Rect", &pokerEffectLayout_.effectRects[1].x, 1.0f);
		changed |= ImGui::DragFloat2("Effect2 Image", &pokerEffectLayout_.effectImages[1].x, 1.0f);

		changed |= ImGui::DragFloat4("Effect3 Rect", &pokerEffectLayout_.effectRects[2].x, 1.0f);
		changed |= ImGui::DragFloat2("Effect3 Image", &pokerEffectLayout_.effectImages[2].x, 1.0f);

		changed |= ImGui::DragFloat4("Effect ViewBoard Rect", &pokerEffectLayout_.effectViewBoardRect.x, 1.0f);
		changed |= ImGui::DragFloat2("Effect ViewBoard Image", &pokerEffectLayout_.effectViewBoardImage.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("InfoButton");
		changed |= ImGui::DragFloat4("InfoButton Rect", &pokerEffectLayout_.infoButtonRect.x, 1.0f);
		changed |= ImGui::DragFloat3("InfoButton Image", &pokerEffectLayout_.infoButtonImage.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("PreviewPanel");
		changed |= ImGui::DragFloat4("PreviewPanel Bg", &pokerEffectLayout_.previewPanelBg.x, 1.0f);
		changed |= ImGui::DragFloat3("PreviewPanel Title Image", &pokerEffectLayout_.previewPanelTitleImage.x, 1.0f);
		changed |= ImGui::DragFloat3("PreviewPanel Text", &pokerEffectLayout_.previewPanelText.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("PreviewPanel Images");

		changed |= ImGui::DragFloat2("Preview Rank Pos", &pokerEffectLayout_.previewImages.rank.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Rank Scale", &pokerEffectLayout_.previewImages.rank.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat2("Preview ATK Label Pos", &pokerEffectLayout_.previewImages.atkLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview ATK Label Scale", &pokerEffectLayout_.previewImages.atkLabel.scale, 0.01f, 0.1f, 5.0f);
		changed |= ImGui::DragFloat2("Preview ATK Value Pos", &pokerEffectLayout_.previewImages.atkValue.x, 1.0f);
		changed |= ImGui::DragFloat("Preview ATK Value Scale", &pokerEffectLayout_.previewImages.atkValue.scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Preview ATK Value Spacing", &pokerEffectLayout_.previewImages.atkValue.spacing, 1.0f, 0.0f, 200.0f);

		changed |= ImGui::DragFloat2("Preview Draw Label Pos", &pokerEffectLayout_.previewImages.drawLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Draw Label Scale", &pokerEffectLayout_.previewImages.drawLabel.scale, 0.01f, 0.1f, 5.0f);
		changed |= ImGui::DragFloat2("Preview Draw Value Pos", &pokerEffectLayout_.previewImages.drawValue.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Draw Value Scale", &pokerEffectLayout_.previewImages.drawValue.scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Preview Draw Value Spacing", &pokerEffectLayout_.previewImages.drawValue.spacing, 1.0f, 0.0f, 200.0f);

		changed |= ImGui::DragFloat2("Preview Damage Label Pos", &pokerEffectLayout_.previewImages.damageLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Damage Label Scale", &pokerEffectLayout_.previewImages.damageLabel.scale, 0.01f, 0.1f, 5.0f);
		changed |= ImGui::DragFloat2("Preview Damage Value Pos", &pokerEffectLayout_.previewImages.damageValue.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Damage Value Scale", &pokerEffectLayout_.previewImages.damageValue.scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Preview Damage Value Spacing", &pokerEffectLayout_.previewImages.damageValue.spacing, 1.0f, 0.0f, 200.0f);

		changed |= ImGui::DragFloat2("Preview TurnStart Pos", &pokerEffectLayout_.previewImages.turnStartLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview TurnStart Scale", &pokerEffectLayout_.previewImages.turnStartLabel.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat2("Preview TurnStart None Pos", &pokerEffectLayout_.previewImages.turnStartNoneLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview TurnStart None Scale", &pokerEffectLayout_.previewImages.turnStartNoneLabel.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat2("Preview Activated Pos", &pokerEffectLayout_.previewImages.activatedLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Activated Scale", &pokerEffectLayout_.previewImages.activatedLabel.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat2("Preview Activated None Pos", &pokerEffectLayout_.previewImages.activatedNoneLabel.x, 1.0f);
		changed |= ImGui::DragFloat("Preview Activated None Scale", &pokerEffectLayout_.previewImages.activatedNoneLabel.scale, 0.01f, 0.1f, 5.0f);

		auto DrawLinesEditor = [&](const char* title, UiPokerPreviewLinesLayout& lines, const char* suffix) {
			if (ImGui::TreeNode(title)) {
				for (int i = 0; i < 5; ++i) {
					std::string label = "Lane" + std::to_string(i + 1) + "##" + suffix;
					changed |= ImGui::DragFloat2(label.c_str(), &lines.lanes[i].x, 1.0f);
				}
				ImGui::TreePop();
			}
			};

		auto DrawEffectAnchorsEditor = [&](const char* title, UiPokerPreviewEffectAnchors& anchors, const char* suffix) {
			if (ImGui::TreeNode(title)) {
				DrawLinesEditor("SingleDamage", anchors.singleDamage, (std::string(suffix) + "_sd").c_str());
				DrawLinesEditor("AllDamage", anchors.allDamage, (std::string(suffix) + "_ad").c_str());
				DrawLinesEditor("Draw", anchors.draw, (std::string(suffix) + "_dr").c_str());
				DrawLinesEditor("Block", anchors.block, (std::string(suffix) + "_bl").c_str());
				DrawLinesEditor("Heal", anchors.heal, (std::string(suffix) + "_he").c_str());
				DrawLinesEditor("None", anchors.none, (std::string(suffix) + "_no").c_str());
				ImGui::TreePop();
			}
			};

		ImGui::Separator();
		ImGui::Text("Preview Effect Anchors");

		DrawEffectAnchorsEditor(
			"TurnStart Effect Anchors",
			pokerEffectLayout_.previewImages.turnStartEffectAnchors,
			"ts_anchor");

		DrawEffectAnchorsEditor(
			"Activated Effect Anchors",
			pokerEffectLayout_.previewImages.activatedEffectAnchors,
			"ac_anchor");

		auto DrawPatternEditor = [&](const char* name, UiPokerPreviewPatternLayout& pat, const char* suffix) {
			if (ImGui::TreeNode(name)) {
				changed |= ImGui::DragFloat(("LabelScale##" + std::string(suffix)).c_str(), &pat.labelScale, 0.01f);
				changed |= ImGui::DragFloat2(("PrefixOffset##" + std::string(suffix)).c_str(), &pat.prefixOffsetX, 1.0f);
				changed |= ImGui::DragFloat2(("NumberOffset##" + std::string(suffix)).c_str(), &pat.numberOffsetX, 1.0f);
				changed |= ImGui::DragFloat(("NumberScale##" + std::string(suffix)).c_str(), &pat.numberScale, 0.01f);
				changed |= ImGui::DragFloat(("NumberSpacing##" + std::string(suffix)).c_str(), &pat.numberSpacing, 1.0f);
				changed |= ImGui::DragFloat2(("SuffixOffset##" + std::string(suffix)).c_str(), &pat.suffixOffsetX, 1.0f);
				ImGui::TreePop();
			}
			};

		ImGui::Separator();
		ImGui::Text("TurnStart Patterns");
		DrawPatternEditor("SingleDamage TS", pokerEffectLayout_.previewImages.turnStartPatterns.singleDamage, "ts_sd");
		DrawPatternEditor("AllDamage TS", pokerEffectLayout_.previewImages.turnStartPatterns.allDamage, "ts_ad");
		DrawPatternEditor("Draw TS", pokerEffectLayout_.previewImages.turnStartPatterns.draw, "ts_dr");
		DrawPatternEditor("Block TS", pokerEffectLayout_.previewImages.turnStartPatterns.block, "ts_bl");
		DrawPatternEditor("Heal TS", pokerEffectLayout_.previewImages.turnStartPatterns.heal, "ts_he");

		ImGui::Separator();
		ImGui::Text("Activated Patterns");
		DrawPatternEditor("SingleDamage AC", pokerEffectLayout_.previewImages.activatedPatterns.singleDamage, "ac_sd");
		DrawPatternEditor("AllDamage AC", pokerEffectLayout_.previewImages.activatedPatterns.allDamage, "ac_ad");
		DrawPatternEditor("Draw AC", pokerEffectLayout_.previewImages.activatedPatterns.draw, "ac_dr");
		DrawPatternEditor("Block AC", pokerEffectLayout_.previewImages.activatedPatterns.block, "ac_bl");
		DrawPatternEditor("Heal AC", pokerEffectLayout_.previewImages.activatedPatterns.heal, "ac_he");

		if (ImGui::Button("Save PokerEffectChoiceLayout")) {
			SavePokerEffectChoiceLayout(pokerEffectLayoutPath_);
		}
		ImGui::SameLine();
		if (ImGui::Button("Load PokerEffectChoiceLayout")) {
			LoadPokerEffectChoiceLayout(pokerEffectLayoutPath_);
		}


		ImGui::Separator();
		ImGui::Text("Activate Choice Text");

		changed |= ImGui::DragFloat3("ActivateTitleText", &pokerEffectLayout_.activateTitleText.x, 1.0f);
		changed |= ImGui::DragFloat3("ActivateYesText", &pokerEffectLayout_.activateYesText.x, 1.0f);
		changed |= ImGui::DragFloat3("ActivateNoText", &pokerEffectLayout_.activateNoText.x, 1.0f);
		changed |= ImGui::DragFloat3("ActivateViewBoardText", &pokerEffectLayout_.activateViewBoardText.x, 1.0f);
		changed |= ImGui::DragFloat3("InfoButtonText", &pokerEffectLayout_.infoButtonText.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("Effect Choice Text");

		changed |= ImGui::DragFloat3("EffectTitleText", &pokerEffectLayout_.effectTitleText.x, 1.0f);
		changed |= ImGui::DragFloat3("BackText", &pokerEffectLayout_.backText.x, 1.0f);
		changed |= ImGui::DragFloat3("EffectText0", &pokerEffectLayout_.effectTexts[0].x, 1.0f);
		changed |= ImGui::DragFloat3("EffectText1", &pokerEffectLayout_.effectTexts[1].x, 1.0f);
		changed |= ImGui::DragFloat3("EffectText2", &pokerEffectLayout_.effectTexts[2].x, 1.0f);
		changed |= ImGui::DragFloat3("EffectViewBoardText", &pokerEffectLayout_.effectViewBoardText.x, 1.0f);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("FieldUiLayout")) {
		bool changed = false;

		changed |= ImGui::DragFloat4("deckBg", &layout_.deckBg.x, 1.0f);
		changed |= ImGui::DragFloat3("deckText", &layout_.deckText.x, 1.0f);
		changed |= ImGui::DragFloat2("deckLabelPos", &layout_.deckLabelImage.x, 1.0f);
		changed |= ImGui::DragFloat("deckLabelScale", &layout_.deckLabelImage.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat4("discardBg", &layout_.discardBg.x, 1.0f);
		changed |= ImGui::DragFloat3("discardText", &layout_.discardText.x, 1.0f);
		changed |= ImGui::DragFloat2("discardLabelPos", &layout_.discardLabelImage.x, 1.0f);
		changed |= ImGui::DragFloat("discardLabelScale", &layout_.discardLabelImage.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat4("handBg", &layout_.handBg.x, 1.0f);
		changed |= ImGui::DragFloat3("handText", &layout_.handText.x, 1.0f);
		changed |= ImGui::DragFloat2("handLabelPos", &layout_.handLabelImage.x, 1.0f);
		changed |= ImGui::DragFloat("handLabelScale", &layout_.handLabelImage.scale, 0.01f, 0.1f, 5.0f);

		changed |= ImGui::DragFloat4("fieldBg", &layout_.fieldBg.x, 1.0f);
		changed |= ImGui::DragFloat3("fieldText", &layout_.fieldText.x, 1.0f);

		changed |= ImGui::DragFloat4("turnBg", &layout_.turnBg.x, 1.0f);
		changed |= ImGui::DragFloat3("turnText", &layout_.turnText.x, 1.0f);

		changed |= ImGui::DragFloat4("costBg", &layout_.costBg.x, 1.0f);
		changed |= ImGui::DragFloat3("costText", &layout_.costText.x, 1.0f);

		changed |= ImGui::DragFloat4("endTurnBg", &layout_.endTurnBg.x, 1.0f);
		changed |= ImGui::DragFloat3("endTurnText", &layout_.endTurnText.x, 1.0f);

		changed |= ImGui::DragFloat4("overlay", &layout_.overlay.x, 1.0f);

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

		ImGui::Separator();
		ImGui::Text("=== Count Text Layout ===");

		bool countChanged = false;

		// デッキ
		ImGui::Text("Deck");
		countChanged |= ImGui::DragFloat2("Deck Offset", &deckCountTextLayout_.offsetX, 1.0f);
		countChanged |= ImGui::DragFloat("Deck Scale", &deckCountTextLayout_.scale, 0.01f, 0.1f, 5.0f);

		// 墓地
		ImGui::Text("Discard");
		countChanged |= ImGui::DragFloat2("Discard Offset", &discardCountTextLayout_.offsetX, 1.0f);
		countChanged |= ImGui::DragFloat("Discard Scale", &discardCountTextLayout_.scale, 0.01f, 0.1f, 5.0f);

		// 手札
		ImGui::Text("Hand");
		countChanged |= ImGui::DragFloat2("Hand Offset", &handCountTextLayout_.offsetX, 1.0f);
		countChanged |= ImGui::DragFloat("Hand Scale", &handCountTextLayout_.scale, 0.01f, 0.1f, 5.0f);

		if (countChanged) {
			ApplyFieldUiLayout_();
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("CardShowUiLayout")) {
		bool changed = false;

		changed |= ImGui::DragFloat4("cardDescBg", &layout_.cardDescBg.x, 1.0f);
		changed |= ImGui::DragFloat2("cardDescText Pos", &layout_.cardDescText.x, 1.0f);
		changed |= ImGui::DragFloat("cardDescText Scale", &layout_.cardDescText.scale, 0.1f);

		ImGui::Separator();
		ImGui::Text("CardDescCustomLayout");

		static bool editDefaultLayout = false;
		ImGui::Checkbox("Edit Default Layout", &editDefaultLayout);

		ImGui::BeginDisabled();
		ImGui::DragInt("Edit Card ID", &editCardId_, 1.0f, 1, 999);
		ImGui::EndDisabled();
		ImGui::Text("※ Title Debug の Debug Card ID と連動");

		UiCardDescCustomLayout* editLayoutPtr = nullptr;

		if (editDefaultLayout) {
			editLayoutPtr = &layout_.cardDescCustom;
			ImGui::Text("Editing : Default Layout");
		} else {
			editLayoutPtr = &GetOrCreateCardDescCustomLayout_(editCardId_);
			ImGui::Text("Editing : Card ID = %d", editCardId_);

			bool hasPerCard = perCardDescCustomLayouts_.find(editCardId_) != perCardDescCustomLayouts_.end();
			ImGui::Text("Per Card Exists : %s", hasPerCard ? "Yes" : "No");

			if (hasPerCard) {
				if (ImGui::Button("Remove Per Card Layout")) {
					perCardDescCustomLayouts_.erase(editCardId_);
					editLayoutPtr = &GetOrCreateCardDescCustomLayout_(editCardId_);
					changed = true;
				}
			} else {
				ImGui::BeginDisabled();
				ImGui::Button("Remove Per Card Layout");
				ImGui::EndDisabled();
			}
		}

		UiCardDescCustomLayout& editLayout = *editLayoutPtr;

		ImGui::Separator();

		ImGui::Text("Custom Desc Image Layout");

		if (!editDefaultLayout && IsCustomDescCardId_(editCardId_)) {
			auto& customImage = GetOrCreateCustomDescImageLayout_(editCardId_);

			changed |= ImGui::DragFloat2("CustomDescImage Pos", &customImage.x, 1.0f);
			changed |= ImGui::DragFloat2("CustomDescImage Scale", &customImage.scaleX, 0.01f, 0.01f, 10.0f);

			ImGui::Text("Custom Desc Image Target Card ID = %d", editCardId_);
		} else {
			ImGui::BeginDisabled();
			static float dummyPos[2] = { 0.0f, 0.0f };
			static float dummyScale[2] = { 1.0f, 1.0f };
			ImGui::DragFloat2("CustomDescImage Pos", dummyPos, 1.0f);
			ImGui::DragFloat2("CustomDescImage Scale", dummyScale, 0.01f, 0.01f, 10.0f);
			ImGui::EndDisabled();

			if (editDefaultLayout) {
				ImGui::Text("※ Default Layout編集中は専用画像レイアウトは編集できません");
			} else {
				ImGui::Text("※ このCard IDは専用画像カードではありません");
			}
		}

		ImGui::Separator();

		ImGui::Text("titleBasicEffect");
		changed |= ImGui::DragFloat2("titleBasicEffect Pos", &editLayout.titleBasicEffect.x, 1.0f);
		changed |= ImGui::DragFloat("titleBasicEffect Scale", &editLayout.titleBasicEffect.scale, 0.01f, 0.1f, 10.0f);

		ImGui::Text("separatorCustom");
		changed |= ImGui::DragFloat2("separatorCustom Pos", &editLayout.separator.x, 1.0f);
		changed |= ImGui::DragFloat("separatorCustom Scale", &editLayout.separator.scale, 0.01f, 0.1f, 10.0f);

		for (int i = 0; i < 3; ++i) {
			std::string label = "BaseRow" + std::to_string(i);
			if (ImGui::TreeNode(label.c_str())) {
				ImGui::Text("target");
				changed |= ImGui::DragFloat2(("target Pos##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].target.x, 1.0f);
				changed |= ImGui::DragFloat(("target Scale##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].target.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("particle");
				changed |= ImGui::DragFloat2(("particle Pos##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].particle.x, 1.0f);
				changed |= ImGui::DragFloat(("particle Scale##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].particle.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("effectType");
				changed |= ImGui::DragFloat2(("effectType Pos##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].effectType.x, 1.0f);
				changed |= ImGui::DragFloat(("effectType Scale##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].effectType.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("value");
				changed |= ImGui::DragFloat2(("value Pos##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].value.x, 1.0f);
				changed |= ImGui::DragFloat(("value Scale##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].value.scale, 0.01f, 0.01f, 10.0f);
				changed |= ImGui::DragFloat(("value Spacing##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].value.spacing, 0.1f, 0.0f, 200.0f);

				ImGui::Text("special1");
				changed |= ImGui::DragFloat2(("special1 Pos##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].special1.x, 1.0f);
				changed |= ImGui::DragFloat(("special1 Scale##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].special1.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("special2");
				changed |= ImGui::DragFloat2(("special2 Pos##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].special2.x, 1.0f);
				changed |= ImGui::DragFloat(("special2 Scale##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].special2.scale, 0.01f, 0.1f, 10.0f);

				changed |= ImGui::DragFloat(("specialAdvance##base" + std::to_string(i)).c_str(),
					&editLayout.baseRows[i].specialAdvance, 1.0f, 0.0f, 1000.0f);

				ImGui::TreePop();
			}
		}

		for (int i = 0; i < 3; ++i) {
			std::string label = "SubBlock" + std::to_string(i);
			if (ImGui::TreeNode(label.c_str())) {
				ImGui::Text("trigger");
				changed |= ImGui::DragFloat2(("trigger Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].trigger.x, 1.0f);
				changed |= ImGui::DragFloat(("trigger Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].trigger.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("rank");
				changed |= ImGui::DragFloat2(("rank Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].rank.x, 1.0f);
				changed |= ImGui::DragFloat(("rank Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].rank.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("suffix");
				changed |= ImGui::DragFloat2(("suffix Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].suffix.x, 1.0f);
				changed |= ImGui::DragFloat(("suffix Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].suffix.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("target");
				changed |= ImGui::DragFloat2(("target Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].target.x, 1.0f);
				changed |= ImGui::DragFloat(("target Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].target.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("particle");
				changed |= ImGui::DragFloat2(("particle Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].particle.x, 1.0f);
				changed |= ImGui::DragFloat(("particle Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].particle.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("effectType");
				changed |= ImGui::DragFloat2(("effectType Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].effectType.x, 1.0f);
				changed |= ImGui::DragFloat(("effectType Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].effectType.scale, 0.01f, 0.1f, 10.0f);

				ImGui::Text("value");
				changed |= ImGui::DragFloat2(("value Pos##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].value.x, 1.0f);
				changed |= ImGui::DragFloat(("value Scale##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].value.scale, 0.01f, 0.01f, 10.0f);
				changed |= ImGui::DragFloat(("value Spacing##sub" + std::to_string(i)).c_str(),
					&editLayout.subBlocks[i].value.spacing, 0.1f, 0.0f, 200.0f);

				ImGui::TreePop();
			}
		}

		if (changed) {
			ApplyFieldUiLayout_();
		}

		if (ImGui::Button("Save CardShowUiLayout")) {
			SaveCardShowUiLayout(cardShowLayoutPath_);
		}
		ImGui::SameLine();
		if (ImGui::Button("Load CardShowUiLayout")) {
			LoadCardShowUiLayout(cardShowLayoutPath_);
			ApplyFieldUiLayout_();
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("UiNumberLayout")) {
		bool changed = false;

		ImGui::Text("Deck Count");
		changed |= ImGui::DragFloat2("Deck Pos", &numberLayout_.deckCount.x, 1.0f);
		changed |= ImGui::DragFloat("Deck Scale", &numberLayout_.deckCount.scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Deck Spacing", &numberLayout_.deckCount.spacing, 1.0f, 0.0f, 200.0f);

		ImGui::Separator();
		ImGui::Text("Discard Count");
		changed |= ImGui::DragFloat2("Discard Pos", &numberLayout_.discardCount.x, 1.0f);
		changed |= ImGui::DragFloat("Discard Scale", &numberLayout_.discardCount.scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Discard Spacing", &numberLayout_.discardCount.spacing, 1.0f, 0.0f, 200.0f);

		ImGui::Separator();
		ImGui::Text("Hand Count");
		changed |= ImGui::DragFloat2("Hand Pos", &numberLayout_.handCount.x, 1.0f);
		changed |= ImGui::DragFloat("Hand Scale", &numberLayout_.handCount.scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Hand Spacing", &numberLayout_.handCount.spacing, 1.0f, 0.0f, 200.0f);

		ImGui::Separator();
		ImGui::Text("Effect1 Value");
		changed |= ImGui::DragFloat2("Effect1 Offset", &numberLayout_.effectValue[0].offsetX, 1.0f);
		changed |= ImGui::DragFloat("Effect1 Scale", &numberLayout_.effectValue[0].scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Effect1 Spacing", &numberLayout_.effectValue[0].spacing, 1.0f, 0.0f, 200.0f);

		ImGui::Separator();
		ImGui::Text("Effect2 Value");
		changed |= ImGui::DragFloat2("Effect2 Offset", &numberLayout_.effectValue[1].offsetX, 1.0f);
		changed |= ImGui::DragFloat("Effect2 Scale", &numberLayout_.effectValue[1].scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Effect2 Spacing", &numberLayout_.effectValue[1].spacing, 1.0f, 0.0f, 200.0f);

		ImGui::Separator();
		ImGui::Text("Effect3 Value");
		changed |= ImGui::DragFloat2("Effect3 Offset", &numberLayout_.effectValue[2].offsetX, 1.0f);
		changed |= ImGui::DragFloat("Effect3 Scale", &numberLayout_.effectValue[2].scale, 0.01f, 0.05f, 5.0f);
		changed |= ImGui::DragFloat("Effect3 Spacing", &numberLayout_.effectValue[2].spacing, 1.0f, 0.0f, 200.0f);

		if (ImGui::Button("Save UiNumberLayout")) {
			SaveUiNumberLayout(numberLayoutPath_);
		}
		ImGui::SameLine();
		if (ImGui::Button("Load UiNumberLayout")) {
			LoadUiNumberLayout(numberLayoutPath_);
		}

		ImGui::TreePop();
	}
}

#endif

bool FieldUi::SavePokerEffectChoiceLayout(const std::string& path) const
{
	json j;

	auto writePattern = [&](json& dst, const UiPokerPreviewPatternLayout& pat) {
		dst["labelScale"] = pat.labelScale;
		dst["prefixOffsetX"] = pat.prefixOffsetX;
		dst["prefixOffsetY"] = pat.prefixOffsetY;
		dst["numberOffsetX"] = pat.numberOffsetX;
		dst["numberOffsetY"] = pat.numberOffsetY;
		dst["numberScale"] = pat.numberScale;
		dst["numberSpacing"] = pat.numberSpacing;
		dst["suffixOffsetX"] = pat.suffixOffsetX;
		dst["suffixOffsetY"] = pat.suffixOffsetY;
		dst["leadingLabelOffsetX"] = pat.leadingLabelOffsetX;
		dst["leadingLabelOffsetY"] = pat.leadingLabelOffsetY;
		dst["leadingAdvanceX"] = pat.leadingAdvanceX;
		};

	j["title"]["x"] = pokerEffectLayout_.titleImage.x;
	j["title"]["y"] = pokerEffectLayout_.titleImage.y;

	j["backButton"]["rect"]["x"] = pokerEffectLayout_.backRect.x;
	j["backButton"]["rect"]["y"] = pokerEffectLayout_.backRect.y;
	j["backButton"]["rect"]["w"] = pokerEffectLayout_.backRect.w;
	j["backButton"]["rect"]["h"] = pokerEffectLayout_.backRect.h;
	j["backButton"]["image"]["x"] = pokerEffectLayout_.backImage.x;
	j["backButton"]["image"]["y"] = pokerEffectLayout_.backImage.y;

	for (int i = 0; i < 3; ++i) {
		std::string key = "effect" + std::to_string(i + 1);

		j[key]["rect"]["x"] = pokerEffectLayout_.effectRects[i].x;
		j[key]["rect"]["y"] = pokerEffectLayout_.effectRects[i].y;
		j[key]["rect"]["w"] = pokerEffectLayout_.effectRects[i].w;
		j[key]["rect"]["h"] = pokerEffectLayout_.effectRects[i].h;

		j[key]["image"]["x"] = pokerEffectLayout_.effectImages[i].x;
		j[key]["image"]["y"] = pokerEffectLayout_.effectImages[i].y;
	}

	j["infoButton"]["rect"]["x"] = pokerEffectLayout_.infoButtonRect.x;
	j["infoButton"]["rect"]["y"] = pokerEffectLayout_.infoButtonRect.y;
	j["infoButton"]["rect"]["w"] = pokerEffectLayout_.infoButtonRect.w;
	j["infoButton"]["rect"]["h"] = pokerEffectLayout_.infoButtonRect.h;
	j["infoButton"]["image"]["x"] = pokerEffectLayout_.infoButtonImage.x;
	j["infoButton"]["image"]["y"] = pokerEffectLayout_.infoButtonImage.y;
	j["infoButton"]["image"]["scale"] = pokerEffectLayout_.infoButtonImage.scale;

	j["previewPanelBg"]["x"] = pokerEffectLayout_.
		previewPanelBg.x;
	j["previewPanelBg"]["y"] = pokerEffectLayout_.previewPanelBg.y;
	j["previewPanelBg"]["w"] = pokerEffectLayout_.previewPanelBg.w;
	j["previewPanelBg"]["h"] = pokerEffectLayout_.previewPanelBg.h;

	j["previewPanelTitle"]["x"] = pokerEffectLayout_.previewPanelTitleImage.x;
	j["previewPanelTitle"]["y"] = pokerEffectLayout_.previewPanelTitleImage.y;
	j["previewPanelTitle"]["scale"] = pokerEffectLayout_.previewPanelTitleImage.scale;

	j["previewPanelText"]["x"] = pokerEffectLayout_.previewPanelText.x;
	j["previewPanelText"]["y"] = pokerEffectLayout_.previewPanelText.y;
	j["previewPanelText"]["scale"] = pokerEffectLayout_.previewPanelText.scale;

	j["previewImages"]["rank"]["x"] = pokerEffectLayout_.previewImages.rank.x;
	j["previewImages"]["rank"]["y"] = pokerEffectLayout_.previewImages.rank.y;
	j["previewImages"]["rank"]["scale"] = pokerEffectLayout_.previewImages.rank.scale;

	j["previewImages"]["atkLabel"]["x"] = pokerEffectLayout_.previewImages.atkLabel.x;
	j["previewImages"]["atkLabel"]["y"] = pokerEffectLayout_.previewImages.atkLabel.y;
	j["previewImages"]["atkLabel"]["scale"] = pokerEffectLayout_.previewImages.atkLabel.scale;

	j["previewImages"]["atkValue"]["x"] = pokerEffectLayout_.previewImages.atkValue.x;
	j["previewImages"]["atkValue"]["y"] = pokerEffectLayout_.previewImages.atkValue.y;
	j["previewImages"]["atkValue"]["scale"] = pokerEffectLayout_.previewImages.atkValue.scale;
	j["previewImages"]["atkValue"]["spacing"] = pokerEffectLayout_.previewImages.atkValue.spacing;

	j["previewImages"]["drawLabel"]["x"] = pokerEffectLayout_.previewImages.drawLabel.x;
	j["previewImages"]["drawLabel"]["y"] = pokerEffectLayout_.previewImages.drawLabel.y;
	j["previewImages"]["drawLabel"]["scale"] = pokerEffectLayout_.previewImages.drawLabel.scale;

	j["previewImages"]["drawValue"]["x"] = pokerEffectLayout_.previewImages.drawValue.x;
	j["previewImages"]["drawValue"]["y"] = pokerEffectLayout_.previewImages.drawValue.y;
	j["previewImages"]["drawValue"]["scale"] = pokerEffectLayout_.previewImages.drawValue.scale;
	j["previewImages"]["drawValue"]["spacing"] = pokerEffectLayout_.previewImages.drawValue.spacing;

	j["previewImages"]["damageLabel"]["x"] = pokerEffectLayout_.previewImages.damageLabel.x;
	j["previewImages"]["damageLabel"]["y"] = pokerEffectLayout_.previewImages.damageLabel.y;
	j["previewImages"]["damageLabel"]["scale"] = pokerEffectLayout_.previewImages.damageLabel.scale;

	j["previewImages"]["damageValue"]["x"] = pokerEffectLayout_.previewImages.damageValue.x;
	j["previewImages"]["damageValue"]["y"] = pokerEffectLayout_.previewImages.damageValue.y;
	j["previewImages"]["damageValue"]["scale"] = pokerEffectLayout_.previewImages.damageValue.scale;
	j["previewImages"]["damageValue"]["spacing"] = pokerEffectLayout_.previewImages.damageValue.spacing;

	j["previewImages"]["turnStartLabel"]["x"] = pokerEffectLayout_.previewImages.turnStartLabel.x;
	j["previewImages"]["turnStartLabel"]["y"] = pokerEffectLayout_.previewImages.turnStartLabel.y;
	j["previewImages"]["turnStartLabel"]["scale"] = pokerEffectLayout_.previewImages.turnStartLabel.scale;

	j["previewImages"]["turnStartNoneLabel"]["x"] = pokerEffectLayout_.previewImages.turnStartNoneLabel.x;
	j["previewImages"]["turnStartNoneLabel"]["y"] = pokerEffectLayout_.previewImages.turnStartNoneLabel.y;
	j["previewImages"]["turnStartNoneLabel"]["scale"] = pokerEffectLayout_.previewImages.turnStartNoneLabel.scale;

	j["previewImages"]["activatedNoneLabel"]["x"] = pokerEffectLayout_.previewImages.activatedNoneLabel.x;
	j["previewImages"]["activatedNoneLabel"]["y"] = pokerEffectLayout_.previewImages.activatedNoneLabel.y;
	j["previewImages"]["activatedNoneLabel"]["scale"] = pokerEffectLayout_.previewImages.activatedNoneLabel.scale;

	

	writePattern(j["previewImages"]["turnStartPatterns"]["singleDamage"],
		pokerEffectLayout_.previewImages.turnStartPatterns.singleDamage);
	writePattern(j["previewImages"]["turnStartPatterns"]["allDamage"],
		pokerEffectLayout_.previewImages.turnStartPatterns.allDamage);
	writePattern(j["previewImages"]["turnStartPatterns"]["draw"],
		pokerEffectLayout_.previewImages.turnStartPatterns.draw);
	writePattern(j["previewImages"]["turnStartPatterns"]["block"],
		pokerEffectLayout_.previewImages.turnStartPatterns.block);
	writePattern(j["previewImages"]["turnStartPatterns"]["heal"],
		pokerEffectLayout_.previewImages.turnStartPatterns.heal);

	writePattern(j["previewImages"]["activatedPatterns"]["singleDamage"],
		pokerEffectLayout_.previewImages.activatedPatterns.singleDamage);
	writePattern(j["previewImages"]["activatedPatterns"]["allDamage"],
		pokerEffectLayout_.previewImages.activatedPatterns.allDamage);
	writePattern(j["previewImages"]["activatedPatterns"]["draw"],
		pokerEffectLayout_.previewImages.activatedPatterns.draw);
	writePattern(j["previewImages"]["activatedPatterns"]["block"],
		pokerEffectLayout_.previewImages.activatedPatterns.block);
	writePattern(j["previewImages"]["activatedPatterns"]["heal"],
		pokerEffectLayout_.previewImages.activatedPatterns.heal);

	auto writeLines = [&](json& dst, const UiPokerPreviewLinesLayout& lines) {
		for (int i = 0; i < 5; ++i) {
			dst[i]["x"] = lines.lanes[i].x;
			dst[i]["y"] = lines.lanes[i].y;
		}
		};

	auto writeEffectAnchors = [&](json& dst, const UiPokerPreviewEffectAnchors& a) {
		writeLines(dst["singleDamage"], a.singleDamage);
		writeLines(dst["allDamage"], a.allDamage);
		writeLines(dst["draw"], a.draw);
		writeLines(dst["block"], a.block);
		writeLines(dst["heal"], a.heal);
		writeLines(dst["none"], a.none);
		};

	writeEffectAnchors(
		j["previewImages"]["turnStartEffectAnchors"],
		pokerEffectLayout_.previewImages.turnStartEffectAnchors);

	writeEffectAnchors(
		j["previewImages"]["activatedEffectAnchors"],
		pokerEffectLayout_.previewImages.activatedEffectAnchors);

	j["previewImages"]["activatedLabel"]["x"] = pokerEffectLayout_.previewImages.activatedLabel.x;
	j["previewImages"]["activatedLabel"]["y"] = pokerEffectLayout_.previewImages.activatedLabel.y;
	j["previewImages"]["activatedLabel"]["scale"] = pokerEffectLayout_.previewImages.activatedLabel.scale;

	j["activateTitleBg"]["x"] = pokerEffectLayout_.activateTitleBg.x;
	j["activateTitleBg"]["y"] = pokerEffectLayout_.activateTitleBg.y;
	j["activateTitleBg"]["w"] = pokerEffectLayout_.activateTitleBg.w;
	j["activateTitleBg"]["h"] = pokerEffectLayout_.activateTitleBg.h;

	j["activateYes"]["rect"]["x"] = pokerEffectLayout_.activateYesRect.x;
	j["activateYes"]["rect"]["y"] = pokerEffectLayout_.activateYesRect.y;
	j["activateYes"]["rect"]["w"] = pokerEffectLayout_.activateYesRect.w;
	j["activateYes"]["rect"]["h"] = pokerEffectLayout_.activateYesRect.h;
	j["activateYes"]["image"]["x"] = pokerEffectLayout_.activateYesImage.x;
	j["activateYes"]["image"]["y"] = pokerEffectLayout_.activateYesImage.y;

	j["activateNo"]["rect"]["x"] = pokerEffectLayout_.activateNoRect.x;
	j["activateNo"]["rect"]["y"] = pokerEffectLayout_.activateNoRect.y;
	j["activateNo"]["rect"]["w"] = pokerEffectLayout_.activateNoRect.w;
	j["activateNo"]["rect"]["h"] = pokerEffectLayout_.activateNoRect.h;
	j["activateNo"]["image"]["x"] = pokerEffectLayout_.activateNoImage.x;
	j["activateNo"]["image"]["y"] = pokerEffectLayout_.activateNoImage.y;

	j["activateViewBoard"]["rect"]["x"] = pokerEffectLayout_.activateViewBoardRect.x;
	j["activateViewBoard"]["rect"]["y"] = pokerEffectLayout_.activateViewBoardRect.y;
	j["activateViewBoard"]["rect"]["w"] = pokerEffectLayout_.activateViewBoardRect.w;
	j["activateViewBoard"]["rect"]["h"] = pokerEffectLayout_.activateViewBoardRect.h;
	j["activateViewBoard"]["image"]["x"] = pokerEffectLayout_.activateViewBoardImage.x;
	j["activateViewBoard"]["image"]["y"] = pokerEffectLayout_.activateViewBoardImage.y;

	j["effectTitleBg"]["x"] = pokerEffectLayout_.effectTitleBg.x;
	j["effectTitleBg"]["y"] = pokerEffectLayout_.effectTitleBg.y;
	j["effectTitleBg"]["w"] = pokerEffectLayout_.effectTitleBg.w;
	j["effectTitleBg"]["h"] = pokerEffectLayout_.effectTitleBg.h;

	j["effectViewBoard"]["rect"]["x"] = pokerEffectLayout_.effectViewBoardRect.x;
	j["effectViewBoard"]["rect"]["y"] = pokerEffectLayout_.effectViewBoardRect.y;
	j["effectViewBoard"]["rect"]["w"] = pokerEffectLayout_.effectViewBoardRect.w;
	j["effectViewBoard"]["rect"]["h"] = pokerEffectLayout_.effectViewBoardRect.h;
	j["effectViewBoard"]["image"]["x"] = pokerEffectLayout_.effectViewBoardImage.x;
	j["effectViewBoard"]["image"]["y"] = pokerEffectLayout_.effectViewBoardImage.y;

	auto writeText = [&](json& dst, const UiText& t) {
		dst["x"] = t.x;
		dst["y"] = t.y;
		dst["scale"] = t.scale;
		};

	writeText(j["activateTitleText"], pokerEffectLayout_.activateTitleText);
	writeText(j["activateYesText"], pokerEffectLayout_.activateYesText);
	writeText(j["activateNoText"], pokerEffectLayout_.activateNoText);
	writeText(j["activateViewBoardText"], pokerEffectLayout_.activateViewBoardText);
	writeText(j["infoButtonText"], pokerEffectLayout_.infoButtonText);

	writeText(j["effectTitleText"], pokerEffectLayout_.effectTitleText);
	writeText(j["backText"], pokerEffectLayout_.backText);
	writeText(j["effectTexts"][0], pokerEffectLayout_.effectTexts[0]);
	writeText(j["effectTexts"][1], pokerEffectLayout_.effectTexts[1]);
	writeText(j["effectTexts"][2], pokerEffectLayout_.effectTexts[2]);
	writeText(j["effectViewBoardText"], pokerEffectLayout_.effectViewBoardText);

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
	} catch (...) {
		pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();
		return false;
	}

	pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();

	auto readPattern = [&](const json& src, UiPokerPreviewPatternLayout& pat) {
		pat.labelScale = src.value("labelScale", pat.labelScale);
		pat.prefixOffsetX = src.value("prefixOffsetX", pat.prefixOffsetX);
		pat.prefixOffsetY = src.value("prefixOffsetY", pat.prefixOffsetY);
		pat.numberOffsetX = src.value("numberOffsetX", pat.numberOffsetX);
		pat.numberOffsetY = src.value("numberOffsetY", pat.numberOffsetY);
		pat.numberScale = src.value("numberScale", pat.numberScale);
		pat.numberSpacing = src.value("numberSpacing", pat.numberSpacing);
		pat.suffixOffsetX = src.value("suffixOffsetX", pat.suffixOffsetX);
		pat.suffixOffsetY = src.value("suffixOffsetY", pat.suffixOffsetY);
		
		pat.leadingLabelOffsetX = src.value("leadingLabelOffsetX", pat.leadingLabelOffsetX);
		pat.leadingLabelOffsetY = src.value("leadingLabelOffsetY", pat.leadingLabelOffsetY);
		pat.leadingAdvanceX = src.value("leadingAdvanceX", pat.leadingAdvanceX);
		
		};

	auto readLines = [&](const json& src, UiPokerPreviewLinesLayout& lines) {
		if (!src.is_array()) return;

		for (int i = 0; i < static_cast<int>(src.size()) && i < 5; ++i) {
			lines.lanes[i].x = src[i].value("x", lines.lanes[i].x);
			lines.lanes[i].y = src[i].value("y", lines.lanes[i].y);
		}
		};

	auto readEffectAnchors = [&](const json& src, UiPokerPreviewEffectAnchors& a) {
		if (src.contains("singleDamage")) readLines(src["singleDamage"], a.singleDamage);
		if (src.contains("allDamage"))    readLines(src["allDamage"], a.allDamage);
		if (src.contains("draw"))         readLines(src["draw"], a.draw);
		if (src.contains("block"))        readLines(src["block"], a.block);
		if (src.contains("heal"))         readLines(src["heal"], a.heal);
		if (src.contains("none"))         readLines(src["none"], a.none);
		};

	// title
	pokerEffectLayout_.titleImage.x = j.value("title", json::object()).value("x", pokerEffectLayout_.titleImage.x);
	pokerEffectLayout_.titleImage.y = j.value("title", json::object()).value("y", pokerEffectLayout_.titleImage.y);

	// backButton
	if (j.contains("backButton")) {
		auto& b = j["backButton"];
		if (b.contains("rect")) {
			pokerEffectLayout_.backRect.x = b["rect"].value("x", pokerEffectLayout_.backRect.x);
			pokerEffectLayout_.backRect.y = b["rect"].value("y", pokerEffectLayout_.backRect.y);
			pokerEffectLayout_.backRect.w = b["rect"].value("w", pokerEffectLayout_.backRect.w);
			pokerEffectLayout_.backRect.h = b["rect"].value("h", pokerEffectLayout_.backRect.h);
		}
		if (b.contains("image")) {
			pokerEffectLayout_.backImage.x = b["image"].value("x", pokerEffectLayout_.backImage.x);
			pokerEffectLayout_.backImage.y = b["image"].value("y", pokerEffectLayout_.backImage.y);
		}
	}

	// effect1 ~ effect3
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
		if (e.contains("image")) {
			pokerEffectLayout_.effectImages[i].x = e["image"].value("x", pokerEffectLayout_.effectImages[i].x);
			pokerEffectLayout_.effectImages[i].y = e["image"].value("y", pokerEffectLayout_.effectImages[i].y);
		}
	}

	// infoButton
	if (j.contains("infoButton")) {
		auto& ib = j["infoButton"];
		if (ib.contains("rect")) {
			pokerEffectLayout_.infoButtonRect.x = ib["rect"].value("x", pokerEffectLayout_.infoButtonRect.x);
			pokerEffectLayout_.infoButtonRect.y = ib["rect"].value("y", pokerEffectLayout_.infoButtonRect.y);
			pokerEffectLayout_.infoButtonRect.w = ib["rect"].value("w", pokerEffectLayout_.infoButtonRect.w);
			pokerEffectLayout_.infoButtonRect.h = ib["rect"].value("h", pokerEffectLayout_.infoButtonRect.h);
		}
		if (ib.contains("image")) {
			pokerEffectLayout_.infoButtonImage.x = ib["image"].value("x", pokerEffectLayout_.infoButtonImage.x);
			pokerEffectLayout_.infoButtonImage.y = ib["image"].value("y", pokerEffectLayout_.infoButtonImage.y);
			pokerEffectLayout_.infoButtonImage.scale = ib["image"].value("scale", pokerEffectLayout_.infoButtonImage.scale);
		}
	}

	// previewPanelBg
	if (j.contains("previewPanelBg")) {
		pokerEffectLayout_.previewPanelBg.x = j["previewPanelBg"].value("x", pokerEffectLayout_.previewPanelBg.x);
		pokerEffectLayout_.previewPanelBg.y = j["previewPanelBg"].value("y", pokerEffectLayout_.previewPanelBg.y);
		pokerEffectLayout_.previewPanelBg.w = j["previewPanelBg"].value("w", pokerEffectLayout_.previewPanelBg.w);
		pokerEffectLayout_.previewPanelBg.h = j["previewPanelBg"].value("h", pokerEffectLayout_.previewPanelBg.h);
	}

	// previewPanelTitle
	if (j.contains("previewPanelTitle")) {
		pokerEffectLayout_.previewPanelTitleImage.x = j["previewPanelTitle"].value("x", pokerEffectLayout_.previewPanelTitleImage.x);
		pokerEffectLayout_.previewPanelTitleImage.y = j["previewPanelTitle"].value("y", pokerEffectLayout_.previewPanelTitleImage.y);
		pokerEffectLayout_.previewPanelTitleImage.scale = j["previewPanelTitle"].value("scale", pokerEffectLayout_.previewPanelTitleImage.scale);
	}

	// previewPanelText
	if (j.contains("previewPanelText")) {
		pokerEffectLayout_.previewPanelText.x = j["previewPanelText"].value("x", pokerEffectLayout_.previewPanelText.x);
		pokerEffectLayout_.previewPanelText.y = j["previewPanelText"].value("y", pokerEffectLayout_.previewPanelText.y);
		pokerEffectLayout_.previewPanelText.scale = j["previewPanelText"].value("scale", pokerEffectLayout_.previewPanelText.scale);
	}

	if (j.contains("previewImages")) {
		auto& p = j["previewImages"];

		if (p.contains("rank")) {
			pokerEffectLayout_.previewImages.rank.x = p["rank"].value("x", pokerEffectLayout_.previewImages.rank.x);
			pokerEffectLayout_.previewImages.rank.y = p["rank"].value("y", pokerEffectLayout_.previewImages.rank.y);
			pokerEffectLayout_.previewImages.rank.scale = p["rank"].value("scale", pokerEffectLayout_.previewImages.rank.scale);
		}

		if (p.contains("atkLabel")) {
			pokerEffectLayout_.previewImages.atkLabel.x = p["atkLabel"].value("x", pokerEffectLayout_.previewImages.atkLabel.x);
			pokerEffectLayout_.previewImages.atkLabel.y = p["atkLabel"].value("y", pokerEffectLayout_.previewImages.atkLabel.y);
			pokerEffectLayout_.previewImages.atkLabel.scale = p["atkLabel"].value("scale", pokerEffectLayout_.previewImages.atkLabel.scale);
		}
		if (p.contains("atkValue")) {
			pokerEffectLayout_.previewImages.atkValue.x = p["atkValue"].value("x", pokerEffectLayout_.previewImages.atkValue.x);
			pokerEffectLayout_.previewImages.atkValue.y = p["atkValue"].value("y", pokerEffectLayout_.previewImages.atkValue.y);
			pokerEffectLayout_.previewImages.atkValue.scale = p["atkValue"].value("scale", pokerEffectLayout_.previewImages.atkValue.scale);
			pokerEffectLayout_.previewImages.atkValue.spacing = p["atkValue"].value("spacing", pokerEffectLayout_.previewImages.atkValue.spacing);
		}

		if (p.contains("drawLabel")) {
			pokerEffectLayout_.previewImages.drawLabel.x = p["drawLabel"].value("x", pokerEffectLayout_.previewImages.drawLabel.x);
			pokerEffectLayout_.previewImages.drawLabel.y = p["drawLabel"].value("y", pokerEffectLayout_.previewImages.drawLabel.y);
			pokerEffectLayout_.previewImages.drawLabel.scale = p["drawLabel"].value("scale", pokerEffectLayout_.previewImages.drawLabel.scale);
		}
		if (p.contains("drawValue")) {
			pokerEffectLayout_.previewImages.drawValue.x = p["drawValue"].value("x", pokerEffectLayout_.previewImages.drawValue.x);
			pokerEffectLayout_.previewImages.drawValue.y = p["drawValue"].value("y", pokerEffectLayout_.previewImages.drawValue.y);
			pokerEffectLayout_.previewImages.drawValue.scale = p["drawValue"].value("scale", pokerEffectLayout_.previewImages.drawValue.scale);
			pokerEffectLayout_.previewImages.drawValue.spacing = p["drawValue"].value("spacing", pokerEffectLayout_.previewImages.drawValue.spacing);
		}

		if (p.contains("damageLabel")) {
			pokerEffectLayout_.previewImages.damageLabel.x = p["damageLabel"].value("x", pokerEffectLayout_.previewImages.damageLabel.x);
			pokerEffectLayout_.previewImages.damageLabel.y = p["damageLabel"].value("y", pokerEffectLayout_.previewImages.damageLabel.y);
			pokerEffectLayout_.previewImages.damageLabel.scale = p["damageLabel"].value("scale", pokerEffectLayout_.previewImages.damageLabel.scale);
		}
		if (p.contains("damageValue")) {
			pokerEffectLayout_.previewImages.damageValue.x = p["damageValue"].value("x", pokerEffectLayout_.previewImages.damageValue.x);
			pokerEffectLayout_.previewImages.damageValue.y = p["damageValue"].value("y", pokerEffectLayout_.previewImages.damageValue.y);
			pokerEffectLayout_.previewImages.damageValue.scale = p["damageValue"].value("scale", pokerEffectLayout_.previewImages.damageValue.scale);
			pokerEffectLayout_.previewImages.damageValue.spacing = p["damageValue"].value("spacing", pokerEffectLayout_.previewImages.damageValue.spacing);
		}

		if (p.contains("turnStartLabel")) {
			pokerEffectLayout_.previewImages.turnStartLabel.x = p["turnStartLabel"].value("x", pokerEffectLayout_.previewImages.turnStartLabel.x);
			pokerEffectLayout_.previewImages.turnStartLabel.y = p["turnStartLabel"].value("y", pokerEffectLayout_.previewImages.turnStartLabel.y);
			pokerEffectLayout_.previewImages.turnStartLabel.scale = p["turnStartLabel"].value("scale", pokerEffectLayout_.previewImages.turnStartLabel.scale);
		}

		if (p.contains("turnStartNoneLabel")) {
			pokerEffectLayout_.previewImages.turnStartNoneLabel.x = p["turnStartNoneLabel"].value("x", pokerEffectLayout_.previewImages.turnStartNoneLabel.x);
			pokerEffectLayout_.previewImages.turnStartNoneLabel.y = p["turnStartNoneLabel"].value("y", pokerEffectLayout_.previewImages.turnStartNoneLabel.y);
			pokerEffectLayout_.previewImages.turnStartNoneLabel.scale = p["turnStartNoneLabel"].value("scale", pokerEffectLayout_.previewImages.turnStartNoneLabel.scale);
		}

		if (p.contains("activatedLabel")) {
			pokerEffectLayout_.previewImages.activatedLabel.x = p["activatedLabel"].value("x", pokerEffectLayout_.previewImages.activatedLabel.x);
			pokerEffectLayout_.previewImages.activatedLabel.y = p["activatedLabel"].value("y", pokerEffectLayout_.previewImages.activatedLabel.y);
			pokerEffectLayout_.previewImages.activatedLabel.scale = p["activatedLabel"].value("scale", pokerEffectLayout_.previewImages.activatedLabel.scale);
		}

		if (p.contains("activatedNoneLabel")) {
			pokerEffectLayout_.previewImages.activatedNoneLabel.x = p["activatedNoneLabel"].value("x", pokerEffectLayout_.previewImages.activatedNoneLabel.x);
			pokerEffectLayout_.previewImages.activatedNoneLabel.y = p["activatedNoneLabel"].value("y", pokerEffectLayout_.previewImages.activatedNoneLabel.y);
			pokerEffectLayout_.previewImages.activatedNoneLabel.scale = p["activatedNoneLabel"].value("scale", pokerEffectLayout_.previewImages.activatedNoneLabel.scale);
		}

		if (p.contains("turnStartEffectAnchors")) {
			readEffectAnchors(
				p["turnStartEffectAnchors"],
				pokerEffectLayout_.previewImages.turnStartEffectAnchors);
		}

		if (p.contains("activatedEffectAnchors")) {
			readEffectAnchors(
				p["activatedEffectAnchors"],
				pokerEffectLayout_.previewImages.activatedEffectAnchors);
		}

		if (p.contains("turnStartPatterns")) {
			auto& pp = p["turnStartPatterns"];

			if (pp.contains("singleDamage")) {
				readPattern(pp["singleDamage"],
					pokerEffectLayout_.previewImages.turnStartPatterns.singleDamage);
			}
			if (pp.contains("allDamage")) {
				readPattern(pp["allDamage"],
					pokerEffectLayout_.previewImages.turnStartPatterns.allDamage);
			}
			if (pp.contains("draw")) {
				readPattern(pp["draw"],
					pokerEffectLayout_.previewImages.turnStartPatterns.draw);
			}
			if (pp.contains("block")) {
				readPattern(pp["block"],
					pokerEffectLayout_.previewImages.turnStartPatterns.block);
			}
			if (pp.contains("heal")) {
				readPattern(pp["heal"],
					pokerEffectLayout_.previewImages.turnStartPatterns.heal);
			}
		}

		if (p.contains("activatedPatterns")) {
			auto& pp = p["activatedPatterns"];

			if (pp.contains("singleDamage")) {
				readPattern(pp["singleDamage"],
					pokerEffectLayout_.previewImages.activatedPatterns.singleDamage);
			}
			if (pp.contains("allDamage")) {
				readPattern(pp["allDamage"],
					pokerEffectLayout_.previewImages.activatedPatterns.allDamage);
			}
			if (pp.contains("draw")) {
				readPattern(pp["draw"],
					pokerEffectLayout_.previewImages.activatedPatterns.draw);
			}
			if (pp.contains("block")) {
				readPattern(pp["block"],
					pokerEffectLayout_.previewImages.activatedPatterns.block);
			}
			if (pp.contains("heal")) {
				readPattern(pp["heal"],
					pokerEffectLayout_.previewImages.activatedPatterns.heal);
			}
		}

	}



	// activateTitleBg
	if (j.contains("activateTitleBg")) {
		pokerEffectLayout_.activateTitleBg.x = j["activateTitleBg"].value("x", pokerEffectLayout_.activateTitleBg.x);
		pokerEffectLayout_.activateTitleBg.y = j["activateTitleBg"].value("y", pokerEffectLayout_.activateTitleBg.y);
		pokerEffectLayout_.activateTitleBg.w = j["activateTitleBg"].value("w", pokerEffectLayout_.activateTitleBg.w);
		pokerEffectLayout_.activateTitleBg.h = j["activateTitleBg"].value("h", pokerEffectLayout_.activateTitleBg.h);
	}

	// activateYes
	if (j.contains("activateYes")) {
		auto& a = j["activateYes"];
		if (a.contains("rect")) {
			pokerEffectLayout_.activateYesRect.x = a["rect"].value("x", pokerEffectLayout_.activateYesRect.x);
			pokerEffectLayout_.activateYesRect.y = a["rect"].value("y", pokerEffectLayout_.activateYesRect.y);
			pokerEffectLayout_.activateYesRect.w = a["rect"].value("w", pokerEffectLayout_.activateYesRect.w);
			pokerEffectLayout_.activateYesRect.h = a["rect"].value("h", pokerEffectLayout_.activateYesRect.h);
		}
		if (a.contains("image")) {
			pokerEffectLayout_.activateYesImage.x = a["image"].value("x", pokerEffectLayout_.activateYesImage.x);
			pokerEffectLayout_.activateYesImage.y = a["image"].value("y", pokerEffectLayout_.activateYesImage.y);
		}
	}

	// activateNo
	if (j.contains("activateNo")) {
		auto& a = j["activateNo"];
		if (a.contains("rect")) {
			pokerEffectLayout_.activateNoRect.x = a["rect"].value("x", pokerEffectLayout_.activateNoRect.x);
			pokerEffectLayout_.activateNoRect.y = a["rect"].value("y", pokerEffectLayout_.activateNoRect.y);
			pokerEffectLayout_.activateNoRect.w = a["rect"].value("w", pokerEffectLayout_.activateNoRect.w);
			pokerEffectLayout_.activateNoRect.h = a["rect"].value("h", pokerEffectLayout_.activateNoRect.h);
		}
		if (a.contains("image")) {
			pokerEffectLayout_.activateNoImage.x = a["image"].value("x", pokerEffectLayout_.activateNoImage.x);
			pokerEffectLayout_.activateNoImage.y = a["image"].value("y", pokerEffectLayout_.activateNoImage.y);
		}
	}

	// activateViewBoard
	if (j.contains("activateViewBoard")) {
		auto& a = j["activateViewBoard"];
		if (a.contains("rect")) {
			pokerEffectLayout_.activateViewBoardRect.x = a["rect"].value("x", pokerEffectLayout_.activateViewBoardRect.x);
			pokerEffectLayout_.activateViewBoardRect.y = a["rect"].value("y", pokerEffectLayout_.activateViewBoardRect.y);
			pokerEffectLayout_.activateViewBoardRect.w = a["rect"].value("w", pokerEffectLayout_.activateViewBoardRect.w);
			pokerEffectLayout_.activateViewBoardRect.h = a["rect"].value("h", pokerEffectLayout_.activateViewBoardRect.h);
		}
		if (a.contains("image")) {
			pokerEffectLayout_.activateViewBoardImage.x = a["image"].value("x", pokerEffectLayout_.activateViewBoardImage.x);
			pokerEffectLayout_.activateViewBoardImage.y = a["image"].value("y", pokerEffectLayout_.activateViewBoardImage.y);
		}
	}

	// effectTitleBg
	if (j.contains("effectTitleBg")) {
		pokerEffectLayout_.effectTitleBg.x = j["effectTitleBg"].value("x", pokerEffectLayout_.effectTitleBg.x);
		pokerEffectLayout_.effectTitleBg.y = j["effectTitleBg"].value("y", pokerEffectLayout_.effectTitleBg.y);
		pokerEffectLayout_.effectTitleBg.w = j["effectTitleBg"].value("w", pokerEffectLayout_.effectTitleBg.w);
		pokerEffectLayout_.effectTitleBg.h = j["effectTitleBg"].value("h", pokerEffectLayout_.effectTitleBg.h);
	}

	// effectViewBoard
	if (j.contains("effectViewBoard")) {
		auto& e = j["effectViewBoard"];
		if (e.contains("rect")) {
			pokerEffectLayout_.effectViewBoardRect.x = e["rect"].value("x", pokerEffectLayout_.effectViewBoardRect.x);
			pokerEffectLayout_.effectViewBoardRect.y = e["rect"].value("y", pokerEffectLayout_.effectViewBoardRect.y);
			pokerEffectLayout_.effectViewBoardRect.w = e["rect"].value("w", pokerEffectLayout_.effectViewBoardRect.w);
			pokerEffectLayout_.effectViewBoardRect.h = e["rect"].value("h", pokerEffectLayout_.effectViewBoardRect.h);
		}
		if (e.contains("image")) {
			pokerEffectLayout_.effectViewBoardImage.x = e["image"].value("x", pokerEffectLayout_.effectViewBoardImage.x);
			pokerEffectLayout_.effectViewBoardImage.y = e["image"].value("y", pokerEffectLayout_.effectViewBoardImage.y);
		}
	}

	auto readText = [&](const json& src, UiText& t) {
		t.x = src.value("x", t.x);
		t.y = src.value("y", t.y);
		t.scale = src.value("scale", t.scale);
		};

	if (j.contains("activateTitleText")) {
		readText(j["activateTitleText"], pokerEffectLayout_.activateTitleText);
	}
	if (j.contains("activateYesText")) {
		readText(j["activateYesText"], pokerEffectLayout_.activateYesText);
	}
	if (j.contains("activateNoText")) {
		readText(j["activateNoText"], pokerEffectLayout_.activateNoText);
	}
	if (j.contains("activateViewBoardText")) {
		readText(j["activateViewBoardText"], pokerEffectLayout_.activateViewBoardText);
	}
	if (j.contains("infoButtonText")) {
		readText(j["infoButtonText"], pokerEffectLayout_.infoButtonText);
	}

	if (j.contains("effectTitleText")) {
		readText(j["effectTitleText"], pokerEffectLayout_.effectTitleText);
	}
	if (j.contains("backText")) {
		readText(j["backText"], pokerEffectLayout_.backText);
	}
	if (j.contains("effectTexts") && j["effectTexts"].is_array()) {
		for (int i = 0; i < 3 && i < static_cast<int>(j["effectTexts"].size()); ++i) {
			readText(j["effectTexts"][i], pokerEffectLayout_.effectTexts[i]);
		}
	}
	if (j.contains("effectViewBoardText")) {
		readText(j["effectViewBoardText"], pokerEffectLayout_.effectViewBoardText);
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

		layout_.deckLabelImage = { 40.0f, 320.0f, 1.0f };
		layout_.discardLabelImage = { 1120.0f, 350.0f, 1.0f };
		layout_.handLabelImage = { 1020.0f, 640.0f, 1.0f };

		layout_.overlay = { 0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight) };
		
		layout_.cardDescImage.baseEffectOffsetY = 0.0f;

		layout_.cardDescImage.baseEffectTypeOffsetX = 0.0f;
		layout_.cardDescImage.baseEffectTypeOffsetY = 0.0f;

		layout_.cardDescImage.baseColonOffsetX = 170.0f;
		layout_.cardDescImage.baseColonOffsetY = 0.0f;

		layout_.cardDescImage.baseValueOffsetX = 210.0f;
		layout_.cardDescImage.baseValueOffsetY = 0.0f;
		layout_.cardDescImage.baseValueScale = 0.35f;
		layout_.cardDescImage.baseValueSpacing = 28.0f;

		layout_.cardDescImage.separatorOffsetX = 0.0f;
		layout_.cardDescImage.separatorOffsetY = 0.0f;
		layout_.cardDescImage.separatorWidth = 320.0f;
		layout_.cardDescImage.separatorHeight = 2.0f;

		layout_.cardDescImage.triggerOffsetX = 0.0f;
		layout_.cardDescImage.triggerOffsetY = 112.0f;

		layout_.cardDescImage.rankOffsetX = 0.0f;
		layout_.cardDescImage.rankOffsetY = 0.0f;

		layout_.cardDescImage.suffixOffsetX = 170.0f;
		layout_.cardDescImage.suffixOffsetY = 0.0f;

		layout_.cardDescImage.subEffectTypeOffsetX = 0.0f;
		layout_.cardDescImage.subEffectTypeOffsetY = 0.0f;

		layout_.cardDescImage.subColonOffsetX = 170.0f;
		layout_.cardDescImage.subColonOffsetY = 0.0f;

		layout_.cardDescImage.subValueOffsetX = 210.0f;
		layout_.cardDescImage.subValueOffsetY = 0.0f;
		layout_.cardDescImage.subValueScale = 0.35f;
		layout_.cardDescImage.subValueSpacing = 28.0f;
		
		// =========================
        // cardDescCustom default
        // =========================
 		layout_.cardDescCustom.titleBasicEffect = { 0.0f, 0.0f, 1.0f };
		layout_.cardDescCustom.separator = { 0.0f, 130.0f, 1.0f };

		layout_.cardDescCustom.baseRows[0].target = { -150.0f, 42.0f, 0.75f };
		layout_.cardDescCustom.baseRows[0].particle = { -10.0f, 42.0f, 0.75f };
		layout_.cardDescCustom.baseRows[0].effectType = { 90.0f, 42.0f, 0.75f };
		layout_.cardDescCustom.baseRows[0].value = { 40.0f, 72.0f, 0.28f, 28.0f };

		layout_.cardDescCustom.baseRows[1].target = { -150.0f, 112.0f, 0.75f };
		layout_.cardDescCustom.baseRows[1].particle = { -10.0f, 112.0f, 0.75f };
		layout_.cardDescCustom.baseRows[1].effectType = { 90.0f, 112.0f, 0.75f };
		layout_.cardDescCustom.baseRows[1].value = { 40.0f, 142.0f, 0.28f, 28.0f };

		layout_.cardDescCustom.baseRows[2].target = { -150.0f, 182.0f, 0.75f };
		layout_.cardDescCustom.baseRows[2].particle = { -10.0f, 182.0f, 0.75f };
		layout_.cardDescCustom.baseRows[2].effectType = { 90.0f, 182.0f, 0.75f };
		layout_.cardDescCustom.baseRows[2].value = { 40.0f, 212.0f, 0.28f, 28.0f };
		
		layout_.cardDescCustom.baseRows[0].special1 = { 90.0f, 42.0f, 0.75f };
		layout_.cardDescCustom.baseRows[0].special2 = { 260.0f, 42.0f, 0.75f };
		layout_.cardDescCustom.baseRows[0].specialAdvance = 250.0f;

		layout_.cardDescCustom.baseRows[1].special1 = { 90.0f, 112.0f, 0.75f };
		layout_.cardDescCustom.baseRows[1].special2 = { 260.0f, 112.0f, 0.75f };
		layout_.cardDescCustom.baseRows[1].specialAdvance = 250.0f;

		layout_.cardDescCustom.baseRows[2].special1 = { 90.0f, 182.0f, 0.75f };
		layout_.cardDescCustom.baseRows[2].special2 = { 260.0f, 182.0f, 0.75f };
		layout_.cardDescCustom.baseRows[2].specialAdvance = 250.0f;

		layout_.cardDescCustom.subBlocks[0].trigger = { -140.0f, 250.0f, 0.74f };
		layout_.cardDescCustom.subBlocks[0].rank = { -141.0f, 295.0f, 0.73f };
		layout_.cardDescCustom.subBlocks[0].suffix = { 34.0f, 293.0f, 0.67f };
		layout_.cardDescCustom.subBlocks[0].target = { -120.0f, 331.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[0].particle = { 15.0f, 331.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[0].effectType = { 110.0f, 331.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[0].value = { 40.0f, 363.0f, 0.23f, 28.0f };

		layout_.cardDescCustom.subBlocks[1].trigger = { -140.0f, 410.0f, 0.74f };
		layout_.cardDescCustom.subBlocks[1].rank = { -141.0f, 455.0f, 0.73f };
		layout_.cardDescCustom.subBlocks[1].suffix = { 34.0f, 453.0f, 0.67f };
		layout_.cardDescCustom.subBlocks[1].target = { -120.0f, 491.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[1].particle = { 15.0f, 491.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[1].effectType = { 110.0f, 491.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[1].value = { 40.0f, 523.0f, 0.23f, 28.0f };

		layout_.cardDescCustom.subBlocks[2].trigger = { -140.0f, 570.0f, 0.74f };
		layout_.cardDescCustom.subBlocks[2].rank = { -141.0f, 615.0f, 0.73f };
		layout_.cardDescCustom.subBlocks[2].suffix = { 34.0f, 613.0f, 0.67f };
		layout_.cardDescCustom.subBlocks[2].target = { -120.0f, 651.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[2].particle = { 15.0f, 651.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[2].effectType = { 110.0f, 651.0f, 0.75f };
		layout_.cardDescCustom.subBlocks[2].value = { 40.0f, 683.0f, 0.23f, 28.0f };

		return false;
	}

	json j;
	try {
		f >> j;
	} catch (...) {
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

	auto readCardDescImage = [&](const char* key, UiCardDescImageLayout& out) {
		if (!j.contains(key)) return;
		auto& v = j[key];

		out.baseEffectOffsetY = v.value("baseEffectOffsetY", out.baseEffectOffsetY);

		out.baseEffectTypeOffsetX = v.value("baseEffectTypeOffsetX", out.baseEffectTypeOffsetX);
		out.baseEffectTypeOffsetY = v.value("baseEffectTypeOffsetY", out.baseEffectTypeOffsetY);

		out.baseColonOffsetX = v.value("baseColonOffsetX", out.baseColonOffsetX);
		out.baseColonOffsetY = v.value("baseColonOffsetY", out.baseColonOffsetY);

		out.baseValueOffsetX = v.value("baseValueOffsetX", out.baseValueOffsetX);
		out.baseValueOffsetY = v.value("baseValueOffsetY", out.baseValueOffsetY);
		out.baseValueScale = v.value("baseValueScale", out.baseValueScale);
		out.baseValueSpacing = v.value("baseValueSpacing", out.baseValueSpacing);

		out.separatorOffsetX = v.value("separatorOffsetX", out.separatorOffsetX);
		out.separatorOffsetY = v.value("separatorOffsetY", out.separatorOffsetY);
		out.separatorWidth = v.value("separatorWidth", out.separatorWidth);
		out.separatorHeight = v.value("separatorHeight", out.separatorHeight);

		out.triggerOffsetX = v.value("triggerOffsetX", out.triggerOffsetX);
		out.triggerOffsetY = v.value("triggerOffsetY", out.triggerOffsetY);

		out.rankOffsetX = v.value("rankOffsetX", out.rankOffsetX);
		out.rankOffsetY = v.value("rankOffsetY", out.rankOffsetY);

		out.suffixOffsetX = v.value("suffixOffsetX", out.suffixOffsetX);
		out.suffixOffsetY = v.value("suffixOffsetY", out.suffixOffsetY);

		out.subEffectTypeOffsetX = v.value("subEffectTypeOffsetX", out.subEffectTypeOffsetX);
		out.subEffectTypeOffsetY = v.value("subEffectTypeOffsetY", out.subEffectTypeOffsetY);

		out.subColonOffsetX = v.value("subColonOffsetX", out.subColonOffsetX);
		out.subColonOffsetY = v.value("subColonOffsetY", out.subColonOffsetY);

		out.subValueOffsetX = v.value("subValueOffsetX", out.subValueOffsetX);
		out.subValueOffsetY = v.value("subValueOffsetY", out.subValueOffsetY);
		out.subValueScale = v.value("subValueScale", out.subValueScale);
		out.subValueSpacing = v.value("subValueSpacing", out.subValueSpacing);

		out.baseEffectOffsetX = v.value("baseEffectOffsetX", out.baseEffectOffsetX);
		out.baseEffectOffsetY = v.value("baseEffectOffsetY", out.baseEffectOffsetY);
		out.baseEffectScale = v.value("baseEffectScale", out.baseEffectScale);

		out.baseEffectTypeScale = v.value("baseEffectTypeScale", out.baseEffectTypeScale);
		out.baseColonScale = v.value("baseColonScale", out.baseColonScale);

		out.triggerScale = v.value("triggerScale", out.triggerScale);
		out.rankScale = v.value("rankScale", out.rankScale);
		out.suffixScale = v.value("suffixScale", out.suffixScale);

		out.subEffectTypeScale = v.value("subEffectTypeScale", out.subEffectTypeScale);
		out.subColonScale = v.value("subColonScale", out.subColonScale);

		};

	auto readText = [&](const char* key, UiText& out) {
		if (!j.contains(key)) return;
		auto& v = j[key];
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.scale = v.value("scale", out.scale);
		};

	auto readImageItem = [&](const json& v, UiImageItem& out) {
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.scale = v.value("scale", out.scale);
		};

	auto readNumberItem = [&](const json& v, UiNumberItem& out) {
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.scale = v.value("scale", out.scale);
		out.spacing = v.value("spacing", out.spacing);
		};

	auto readCardDescCustom = [&](const char* key, UiCardDescCustomLayout& out) {
		if (!j.contains(key)) return;
		auto& v = j[key];

		if (v.contains("titleBasicEffect")) readImageItem(v["titleBasicEffect"], out.titleBasicEffect);
		if (v.contains("separator"))       readImageItem(v["separator"], out.separator);

		if (v.contains("baseRows") && v["baseRows"].is_array()) {
			for (int i = 0; i < static_cast<int>(v["baseRows"].size()) && i < 3; ++i) {
				auto& row = v["baseRows"][i];
				if (row.contains("target"))    readImageItem(row["target"], out.baseRows[i].target);
				if (row.contains("particle"))  readImageItem(row["particle"], out.baseRows[i].particle);
				if (row.contains("effectType"))readImageItem(row["effectType"], out.baseRows[i].effectType);
				if (row.contains("value"))     readNumberItem(row["value"], out.baseRows[i].value);
				if (row.contains("special1")) readImageItem(row["special1"], out.baseRows[i].special1);
				if (row.contains("special2")) readImageItem(row["special2"], out.baseRows[i].special2);
				out.baseRows[i].specialAdvance = row.value("specialAdvance", out.baseRows[i].specialAdvance);
			}
		}

		if (v.contains("subBlocks") && v["subBlocks"].is_array()) {
			for (int i = 0; i < static_cast<int>(v["subBlocks"].size()) && i < 3; ++i) {
				auto& block = v["subBlocks"][i];
				if (block.contains("trigger"))    readImageItem(block["trigger"], out.subBlocks[i].trigger);
				if (block.contains("rank"))       readImageItem(block["rank"], out.subBlocks[i].rank);
				if (block.contains("suffix"))     readImageItem(block["suffix"], out.subBlocks[i].suffix);
				if (block.contains("target"))    readImageItem(block["target"], out.subBlocks[i].target);
				if (block.contains("particle"))  readImageItem(block["particle"], out.subBlocks[i].particle);
				if (block.contains("effectType"))readImageItem(block["effectType"], out.subBlocks[i].effectType);
				if (block.contains("value"))     readNumberItem(block["value"], out.subBlocks[i].value);
			}
		}
		};

	layout_.cardDescBg = { 20.0f, 600.0f, 900.0f, 120.0f };
	layout_.cardDescText = { 40.0f, 620.0f, 1.0f };
	layout_.deckBg = { 20.0f, 310.0f, 150.0f, 60.0f };
	layout_.deckText = { 40.0f, 320.0f, 0.9f };
	layout_.discardBg = { 1100.0f, 350.0f, 150.0f, 60.0f };
	layout_.discardText = { 1120.0f, 350.0f, 0.9f };
	layout_.handBg = { 1000.0f, 640.0f, 250.0f, 60.0f };
	layout_.handText = { 1020.0f, 640.0f, 0.9f };
	layout_.deckLabelImage = { 40.0f, 320.0f, 1.0f };
	layout_.discardLabelImage = { 1120.0f, 350.0f, 1.0f };
	layout_.handLabelImage = { 1020.0f, 640.0f, 1.0f };
	layout_.fieldBg = { 540.0f, 250.0f, 250.0f, 60.0f };
	layout_.fieldText = { 600.0f, 250.0f, 0.9f };
	layout_.turnBg = { 490.0f, 25.0f, 250.0f, 60.0f };
	layout_.turnText = { 500.0f, 20.0f, 1.0f };
	layout_.costBg = { 75.0f, 405.0f, 170.0f, 55.0f };
	layout_.costText = { 90.0f, 400.0f, 1.0f };
	layout_.overlay = { 0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight) };
	layout_.cardDescImage.baseEffectOffsetY = 0.0f;

	layout_.cardDescImage.baseEffectTypeOffsetX = 0.0f;
	layout_.cardDescImage.baseEffectTypeOffsetY = 0.0f;

	layout_.cardDescImage.baseColonOffsetX = 170.0f;
	layout_.cardDescImage.baseColonOffsetY = 0.0f;

	layout_.cardDescImage.baseValueOffsetX = 210.0f;
	layout_.cardDescImage.baseValueOffsetY = 0.0f;
	layout_.cardDescImage.baseValueScale = 0.35f;
	layout_.cardDescImage.baseValueSpacing = 28.0f;

	layout_.cardDescImage.separatorOffsetY = 0.0f;
	layout_.cardDescImage.separatorWidth = 320.0f;
	layout_.cardDescImage.separatorHeight = 2.0f;

	layout_.cardDescImage.triggerOffsetX = 0.0f;
	layout_.cardDescImage.triggerOffsetY = 0.0f;

	layout_.cardDescImage.rankOffsetX = 0.0f;
	layout_.cardDescImage.rankOffsetY = 0.0f;

	layout_.cardDescImage.suffixOffsetX = 170.0f;
	layout_.cardDescImage.suffixOffsetY = 0.0f;

	layout_.cardDescImage.subEffectTypeOffsetX = 0.0f;
	layout_.cardDescImage.subEffectTypeOffsetY = 0.0f;

	layout_.cardDescImage.subColonOffsetX = 170.0f;
	layout_.cardDescImage.subColonOffsetY = 0.0f;

	layout_.cardDescImage.subValueOffsetX = 210.0f;
	layout_.cardDescImage.subValueOffsetY = 0.0f;
	layout_.cardDescImage.subValueScale = 0.35f;
	layout_.cardDescImage.subValueSpacing = 28.0f;

	// =========================
    // cardDescCustom default
    // ========================= 
	layout_.cardDescCustom.titleBasicEffect = { 0.0f, 0.0f, 1.0f };
	layout_.cardDescCustom.separator = { 0.0f, 130.0f, 1.0f };

	layout_.cardDescCustom.baseRows[0].target = { -150.0f, 42.0f, 0.75f };
	layout_.cardDescCustom.baseRows[0].particle = { -10.0f, 42.0f, 0.75f };
	layout_.cardDescCustom.baseRows[0].effectType = { 90.0f, 42.0f, 0.75f };
	layout_.cardDescCustom.baseRows[0].value = { 40.0f, 72.0f, 0.28f, 28.0f };
	layout_.cardDescCustom.baseRows[0].special1 = { 90.0f, 42.0f, 0.75f };
	layout_.cardDescCustom.baseRows[0].special2 = { 260.0f, 42.0f, 0.75f };
	layout_.cardDescCustom.baseRows[0].specialAdvance = 250.0f;

	layout_.cardDescCustom.baseRows[1].target = { -150.0f, 112.0f, 0.75f };
	layout_.cardDescCustom.baseRows[1].particle = { -10.0f, 112.0f, 0.75f };
	layout_.cardDescCustom.baseRows[1].effectType = { 90.0f, 112.0f, 0.75f };
	layout_.cardDescCustom.baseRows[1].value = { 40.0f, 142.0f, 0.28f, 28.0f };
	layout_.cardDescCustom.baseRows[1].special1 = { 90.0f, 112.0f, 0.75f };
	layout_.cardDescCustom.baseRows[1].special2 = { 260.0f, 112.0f, 0.75f };
	layout_.cardDescCustom.baseRows[1].specialAdvance = 250.0f;

	layout_.cardDescCustom.baseRows[2].target = { -150.0f, 182.0f, 0.75f };
	layout_.cardDescCustom.baseRows[2].particle = { -10.0f, 182.0f, 0.75f };
	layout_.cardDescCustom.baseRows[2].effectType = { 90.0f, 182.0f, 0.75f };
	layout_.cardDescCustom.baseRows[2].value = { 40.0f, 212.0f, 0.28f, 28.0f };
	layout_.cardDescCustom.baseRows[2].special1 = { 90.0f, 182.0f, 0.75f };
	layout_.cardDescCustom.baseRows[2].special2 = { 260.0f, 182.0f, 0.75f };
	layout_.cardDescCustom.baseRows[2].specialAdvance = 250.0f;

	layout_.cardDescCustom.subBlocks[0].trigger = { -140.0f, 250.0f, 0.74f };
	layout_.cardDescCustom.subBlocks[0].rank = { -141.0f, 295.0f, 0.73f };
	layout_.cardDescCustom.subBlocks[0].suffix = { 34.0f, 293.0f, 0.67f };
	layout_.cardDescCustom.subBlocks[0].target = { -120.0f, 331.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[0].particle = { 15.0f, 331.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[0].effectType = { 110.0f, 331.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[0].value = { 40.0f, 363.0f, 0.23f, 28.0f };

	layout_.cardDescCustom.subBlocks[1].trigger = { -140.0f, 410.0f, 0.74f };
	layout_.cardDescCustom.subBlocks[1].rank = { -141.0f, 455.0f, 0.73f };
	layout_.cardDescCustom.subBlocks[1].suffix = { 34.0f, 453.0f, 0.67f };
	layout_.cardDescCustom.subBlocks[1].target = { -120.0f, 491.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[1].particle = { 15.0f, 491.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[1].effectType = { 110.0f, 491.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[1].value = { 40.0f, 523.0f, 0.23f, 28.0f };

	layout_.cardDescCustom.subBlocks[2].trigger = { -140.0f, 570.0f, 0.74f };
	layout_.cardDescCustom.subBlocks[2].rank = { -141.0f, 615.0f, 0.73f };
	layout_.cardDescCustom.subBlocks[2].suffix = { 34.0f, 613.0f, 0.67f };
	layout_.cardDescCustom.subBlocks[2].target = { -120.0f, 651.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[2].particle = { 15.0f, 651.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[2].effectType = { 110.0f, 651.0f, 0.75f };
	layout_.cardDescCustom.subBlocks[2].value = { 40.0f, 683.0f, 0.23f, 28.0f };

	readRect("cardDescBg", layout_.cardDescBg);
	readText("cardDescText", layout_.cardDescText);

	readRect("deckBg", layout_.deckBg);
	readText("deckText", layout_.deckText);
	readText("deckLabelImage", layout_.deckLabelImage);

	readRect("discardBg", layout_.discardBg);
	readText("discardText", layout_.discardText);
	readText("discardLabelImage", layout_.discardLabelImage);

	readRect("handBg", layout_.handBg);
	readText("handText", layout_.handText);
	readText("handLabelImage", layout_.handLabelImage);

	readRect("fieldBg", layout_.fieldBg);
	readText("fieldText", layout_.fieldText);

	readRect("turnBg", layout_.turnBg);
	readText("turnText", layout_.turnText);

	readRect("costBg", layout_.costBg);
	readText("costText", layout_.costText);

	readRect("endTurnBg", layout_.endTurnBg);
	readText("endTurnText", layout_.endTurnText);

	readRect("overlay", layout_.overlay);

	readCardDescImage("cardDescImage", layout_.cardDescImage);

	readCardDescCustom("cardDescCustom", layout_.cardDescCustom);

	return true;
}
bool FieldUi::LoadCardShowUiLayout(const std::string& path)
{
	std::ifstream f(path);
	if (!f.is_open()) {
		return false;
	}

	json j;
	try {
		f >> j;
	} catch (...) {
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

	auto readImageItem = [&](const json& v, UiImageItem& out) {
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.scale = v.value("scale", out.scale);
		};

	auto readNumberItem = [&](const json& v, UiNumberItem& out) {
		out.x = v.value("x", out.x);
		out.y = v.value("y", out.y);
		out.scale = v.value("scale", out.scale);
		out.spacing = v.value("spacing", out.spacing);
		};

	auto readCardDescCustomFromJson = [&](const json& v, UiCardDescCustomLayout& out) {
		if (v.contains("titleBasicEffect")) readImageItem(v["titleBasicEffect"], out.titleBasicEffect);
		if (v.contains("separator")) readImageItem(v["separator"], out.separator);

		if (v.contains("baseRows") && v["baseRows"].is_array()) {
			for (int i = 0; i < static_cast<int>(v["baseRows"].size()) && i < 3; ++i) {
				auto& row = v["baseRows"][i];
				if (row.contains("target")) readImageItem(row["target"], out.baseRows[i].target);
				if (row.contains("particle")) readImageItem(row["particle"], out.baseRows[i].particle);
				if (row.contains("effectType")) readImageItem(row["effectType"], out.baseRows[i].effectType);
				if (row.contains("value")) readNumberItem(row["value"], out.baseRows[i].value);
				if (row.contains("special1")) readImageItem(row["special1"], out.baseRows[i].special1);
				if (row.contains("special2")) readImageItem(row["special2"], out.baseRows[i].special2);
				out.baseRows[i].specialAdvance = row.value("specialAdvance", out.baseRows[i].specialAdvance);
			}
		}

		if (v.contains("subBlocks") && v["subBlocks"].is_array()) {
			for (int i = 0; i < static_cast<int>(v["subBlocks"].size()) && i < 3; ++i) {
				auto& block = v["subBlocks"][i];
				if (block.contains("trigger")) readImageItem(block["trigger"], out.subBlocks[i].trigger);
				if (block.contains("rank")) readImageItem(block["rank"], out.subBlocks[i].rank);
				if (block.contains("suffix")) readImageItem(block["suffix"], out.subBlocks[i].suffix);
				if (block.contains("target")) readImageItem(block["target"], out.subBlocks[i].target);
				if (block.contains("particle")) readImageItem(block["particle"], out.subBlocks[i].particle);
				if (block.contains("effectType")) readImageItem(block["effectType"], out.subBlocks[i].effectType);
				if (block.contains("value")) readNumberItem(block["value"], out.subBlocks[i].value);
			}
		}
		};

	readRect("cardDescBg", layout_.cardDescBg);
	readText("cardDescText", layout_.cardDescText);

	if (j.contains("defaultCardDescCustom")) {
		readCardDescCustomFromJson(j["defaultCardDescCustom"], layout_.cardDescCustom);
	} else if (j.contains("cardDescCustom")) {
		// 旧形式互換
		readCardDescCustomFromJson(j["cardDescCustom"], layout_.cardDescCustom);
	}

	perCardDescCustomLayouts_.clear();
	if (j.contains("perCard") && j["perCard"].is_object()) {
		for (auto it = j["perCard"].begin(); it != j["perCard"].end(); ++it) {
			int cardId = std::stoi(it.key());
			UiCardDescCustomLayout custom = layout_.cardDescCustom;
			readCardDescCustomFromJson(it.value(), custom);
			perCardDescCustomLayouts_[cardId] = custom;
		}
	}

	perCardCustomDescImageLayouts_.clear();
	if (j.contains("perCardCustomDescImage") && j["perCardCustomDescImage"].is_object()) {
		for (auto it = j["perCardCustomDescImage"].begin(); it != j["perCardCustomDescImage"].end(); ++it) {
			int cardId = std::stoi(it.key());

			UiCustomDescImageLayout layout{};
			layout.x = it.value().value("x", layout_.cardDescBg.x);
			layout.y = it.value().value("y", layout_.cardDescBg.y);
			layout.scaleX = it.value().value("scaleX", 1.0f);
			layout.scaleY = it.value().value("scaleY", 1.0f);

			perCardCustomDescImageLayouts_[cardId] = layout;
		}
	}

	return true;
}

bool FieldUi::SaveCardShowUiLayout(const std::string& path) const
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

	auto writeImageItem = [&](json& dst, const UiImageItem& v) {
		dst["x"] = v.x;
		dst["y"] = v.y;
		dst["scale"] = v.scale;
		};

	auto writeNumberItem = [&](json& dst, const UiNumberItem& v) {
		dst["x"] = v.x;
		dst["y"] = v.y;
		dst["scale"] = v.scale;
		dst["spacing"] = v.spacing;
		};

	auto writeCardDescCustomToJson = [&](json& dst, const UiCardDescCustomLayout& v) {
		writeImageItem(dst["titleBasicEffect"], v.titleBasicEffect);
		writeImageItem(dst["separator"], v.separator);

		for (int i = 0; i < 3; ++i) {
			writeImageItem(dst["baseRows"][i]["target"], v.baseRows[i].target);
			writeImageItem(dst["baseRows"][i]["particle"], v.baseRows[i].particle);
			writeImageItem(dst["baseRows"][i]["effectType"], v.baseRows[i].effectType);
			writeNumberItem(dst["baseRows"][i]["value"], v.baseRows[i].value);
			writeImageItem(dst["baseRows"][i]["special1"], v.baseRows[i].special1);
			writeImageItem(dst["baseRows"][i]["special2"], v.baseRows[i].special2);
			dst["baseRows"][i]["specialAdvance"] = v.baseRows[i].specialAdvance;
		}

		for (int i = 0; i < 3; ++i) {
			writeImageItem(dst["subBlocks"][i]["trigger"], v.subBlocks[i].trigger);
			writeImageItem(dst["subBlocks"][i]["rank"], v.subBlocks[i].rank);
			writeImageItem(dst["subBlocks"][i]["suffix"], v.subBlocks[i].suffix);
			writeImageItem(dst["subBlocks"][i]["target"], v.subBlocks[i].target);
			writeImageItem(dst["subBlocks"][i]["particle"], v.subBlocks[i].particle);
			writeImageItem(dst["subBlocks"][i]["effectType"], v.subBlocks[i].effectType);
			writeNumberItem(dst["subBlocks"][i]["value"], v.subBlocks[i].value);
		}
		};

	writeRect("cardDescBg", layout_.cardDescBg);
	writeText("cardDescText", layout_.cardDescText);

	writeCardDescCustomToJson(j["defaultCardDescCustom"], layout_.cardDescCustom);

	for (const auto& [cardId, layout] : perCardDescCustomLayouts_) {
		writeCardDescCustomToJson(j["perCard"][std::to_string(cardId)], layout);
	}

	for (const auto& [cardId, layout] : perCardCustomDescImageLayouts_) {
		j["perCardCustomDescImage"][std::to_string(cardId)]["x"] = layout.x;
		j["perCardCustomDescImage"][std::to_string(cardId)]["y"] = layout.y;
		j["perCardCustomDescImage"][std::to_string(cardId)]["scaleX"] = layout.scaleX;
		j["perCardCustomDescImage"][std::to_string(cardId)]["scaleY"] = layout.scaleY;
	}

	std::ofstream ofs(path);
	if (!ofs.is_open()) return false;
	ofs << j.dump(4);
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
	writeText("deckLabelImage", layout_.deckLabelImage);

	writeRect("discardBg", layout_.discardBg);
	writeText("discardText", layout_.discardText);
	writeText("discardLabelImage", layout_.discardLabelImage);

	writeRect("handBg", layout_.handBg);
	writeText("handText", layout_.handText);
	writeText("handLabelImage", layout_.handLabelImage);

	writeRect("fieldBg", layout_.fieldBg);
	writeText("fieldText", layout_.fieldText);

	writeRect("turnBg", layout_.turnBg);
	writeText("turnText", layout_.turnText);

	writeRect("costBg", layout_.costBg);
	writeText("costText", layout_.costText);

	writeRect("endTurnBg", layout_.endTurnBg);
	writeText("endTurnText", layout_.endTurnText);

	
	auto writeCardDescImage = [&](const char* key, const UiCardDescImageLayout& v) {
		j[key]["baseEffectOffsetY"] = v.baseEffectOffsetY;

		j[key]["baseEffectTypeOffsetX"] = v.baseEffectTypeOffsetX;
		j[key]["baseEffectTypeOffsetY"] = v.baseEffectTypeOffsetY;

		j[key]["baseColonOffsetX"] = v.baseColonOffsetX;
		j[key]["baseColonOffsetY"] = v.baseColonOffsetY;

		j[key]["baseValueOffsetX"] = v.baseValueOffsetX;
		j[key]["baseValueOffsetY"] = v.baseValueOffsetY;
		j[key]["baseValueScale"] = v.baseValueScale;
		j[key]["baseValueSpacing"] = v.baseValueSpacing;

		j[key]["separatorOffsetX"] = v.separatorOffsetX;
		j[key]["separatorOffsetY"] = v.separatorOffsetY;
		j[key]["separatorWidth"] = v.separatorWidth;
		j[key]["separatorHeight"] = v.separatorHeight;

		j[key]["triggerOffsetX"] = v.triggerOffsetX;
		j[key]["triggerOffsetY"] = v.triggerOffsetY;

		j[key]["rankOffsetX"] = v.rankOffsetX;
		j[key]["rankOffsetY"] = v.rankOffsetY;

		j[key]["suffixOffsetX"] = v.suffixOffsetX;
		j[key]["suffixOffsetY"] = v.suffixOffsetY;

		j[key]["subEffectTypeOffsetX"] = v.subEffectTypeOffsetX;
		j[key]["subEffectTypeOffsetY"] = v.subEffectTypeOffsetY;

		j[key]["subColonOffsetX"] = v.subColonOffsetX;
		j[key]["subColonOffsetY"] = v.subColonOffsetY;

		j[key]["subValueOffsetX"] = v.subValueOffsetX;
		j[key]["subValueOffsetY"] = v.subValueOffsetY;
		j[key]["subValueScale"] = v.subValueScale;
		j[key]["subValueSpacing"] = v.subValueSpacing;

		j[key]["baseEffectOffsetX"] = v.baseEffectOffsetX;
		j[key]["baseEffectOffsetY"] = v.baseEffectOffsetY;
		j[key]["baseEffectScale"] = v.baseEffectScale;

		j[key]["baseEffectTypeScale"] = v.baseEffectTypeScale;
		j[key]["baseColonScale"] = v.baseColonScale;

		j[key]["triggerScale"] = v.triggerScale;
		j[key]["rankScale"] = v.rankScale;
		j[key]["suffixScale"] = v.suffixScale;

		j[key]["subEffectTypeScale"] = v.subEffectTypeScale;
		j[key]["subColonScale"] = v.subColonScale;

		};

	auto writeImageItem = [&](json& dst, const UiImageItem& v) {
		dst["x"] = v.x;
		dst["y"] = v.y;
		dst["scale"] = v.scale;
		};

	auto writeNumberItem = [&](json& dst, const UiNumberItem& v) {
		dst["x"] = v.x;
		dst["y"] = v.y;
		dst["scale"] = v.scale;
		dst["spacing"] = v.spacing;
		};

	auto writeCardDescCustom = [&](const char* key, const UiCardDescCustomLayout& v) {
		writeImageItem(j[key]["titleBasicEffect"], v.titleBasicEffect);
		writeImageItem(j[key]["separator"], v.separator);

		for (int i = 0; i < 3; ++i) {
			writeImageItem(j[key]["baseRows"][i]["target"], v.baseRows[i].target);
			writeImageItem(j[key]["baseRows"][i]["particle"], v.baseRows[i].particle);
			writeImageItem(j[key]["baseRows"][i]["effectType"], v.baseRows[i].effectType);
			writeNumberItem(j[key]["baseRows"][i]["value"], v.baseRows[i].value);
			writeImageItem(j[key]["baseRows"][i]["special1"], v.baseRows[i].special1);
			writeImageItem(j[key]["baseRows"][i]["special2"], v.baseRows[i].special2);
			j[key]["baseRows"][i]["specialAdvance"] = v.baseRows[i].specialAdvance;
		}

		for (int i = 0; i < 3; ++i) {
			writeImageItem(j[key]["subBlocks"][i]["trigger"], v.subBlocks[i].trigger);
			writeImageItem(j[key]["subBlocks"][i]["rank"], v.subBlocks[i].rank);
			writeImageItem(j[key]["subBlocks"][i]["suffix"], v.subBlocks[i].suffix);
			writeImageItem(j[key]["subBlocks"][i]["target"], v.subBlocks[i].target);
			writeImageItem(j[key]["subBlocks"][i]["particle"], v.subBlocks[i].particle);
			writeImageItem(j[key]["subBlocks"][i]["effectType"], v.subBlocks[i].effectType);
			writeNumberItem(j[key]["subBlocks"][i]["value"], v.subBlocks[i].value);
		}
		};
	

	writeRect("overlay", layout_.overlay);

	writeCardDescImage("cardDescImage", layout_.cardDescImage);

	writeCardDescCustom("cardDescCustom", layout_.cardDescCustom);

	std::ofstream ofs(path);
	if (!ofs.is_open()) {
		return false;
	}

	ofs << j.dump(4);
	return true;
}


bool FieldUi::SaveUiNumberLayout(const std::string& path) const
{
	json j;

	auto writeNumber = [&](const char* key, const UiNumber& n) {
		j[key]["x"] = n.x;
		j[key]["y"] = n.y;
		j[key]["scale"] = n.scale;
		j[key]["spacing"] = n.spacing;
		};

	auto writeRelative = [&](const char* key, const UiNumberRelative& n) {
		j[key]["offsetX"] = n.offsetX;
		j[key]["offsetY"] = n.offsetY;
		j[key]["scale"] = n.scale;
		j[key]["spacing"] = n.spacing;
		};

	writeNumber("deckCount", numberLayout_.deckCount);
	writeNumber("discardCount", numberLayout_.discardCount);
	writeNumber("handCount", numberLayout_.handCount);

	writeRelative("effect1", numberLayout_.effectValue[0]);
	writeRelative("effect2", numberLayout_.effectValue[1]);
	writeRelative("effect3", numberLayout_.effectValue[2]);

	
	std::ofstream ofs(path);
	if (!ofs.is_open()) return false;
	ofs << j.dump(4);



	return true;
}

bool FieldUi::LoadUiNumberLayout(const std::string& path)
{
	numberLayout_.deckCount = { 120.0f, 340.0f, 0.45f, 34.0f };
	numberLayout_.discardCount = { 1200.0f, 370.0f, 0.45f, 34.0f };
	numberLayout_.handCount = { 1120.0f, 660.0f, 0.45f, 34.0f };

	numberLayout_.effectValue[0] = { 150.0f, 0.0f, 0.35f, 28.0f };
	numberLayout_.effectValue[1] = { 170.0f, 0.0f, 0.35f, 28.0f };
	numberLayout_.effectValue[2] = { 170.0f, 0.0f, 0.35f, 28.0f };

	std::ifstream ifs(path);
	if (!ifs.is_open()) return false;

	json j;
	try {
		ifs >> j;
	} catch (...) {
		return false;
	}

	auto readNumber = [&](const char* key, UiNumber& n) {
		if (!j.contains(key)) return;
		auto& v = j[key];
		n.x = v.value("x", n.x);
		n.y = v.value("y", n.y);
		n.scale = v.value("scale", n.scale);
		n.spacing = v.value("spacing", n.spacing);
		};

	auto readRelative = [&](const char* key, UiNumberRelative& n) {
		if (!j.contains(key)) return;
		auto& v = j[key];
		n.offsetX = v.value("offsetX", n.offsetX);
		n.offsetY = v.value("offsetY", n.offsetY);
		n.scale = v.value("scale", n.scale);
		n.spacing = v.value("spacing", n.spacing);
		};

	readNumber("deckCount", numberLayout_.deckCount);
	readNumber("discardCount", numberLayout_.discardCount);
	readNumber("handCount", numberLayout_.handCount);

	readRelative("effect1", numberLayout_.effectValue[0]);
	readRelative("effect2", numberLayout_.effectValue[1]);
	readRelative("effect3", numberLayout_.effectValue[2]);

	return true;
}