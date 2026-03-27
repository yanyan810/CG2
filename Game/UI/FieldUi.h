#pragma once
#include <memory>
#include <string>
#include "UiLayout.h"
#include "TextSprite.h"
#include "Sprite.h"
#include "CardDef.h"
#include "BattleController.h"

#include <unordered_map>
#include <unordered_set>


class GameApp;


class FieldUi {
public:
	enum class DescMode {
		None,
		PokerChoice,
		CardDesc,
		Operation
	};

	void Initialize(GameApp& app);
	void Update(GameApp& app, const BattleController& battle);
	//  void Draw(GameApp& app);
	void Draw(GameApp& app, const BattleController& battle);


#ifdef USE_IMGUI
	void DrawImGui();
#endif

	bool LoadPokerEffectChoiceLayout(const std::string& path);
	bool SavePokerEffectChoiceLayout(const std::string& path) const;

	bool LoadFieldUiLayout(const std::string& path);
	bool SaveFieldUiLayout(const std::string& path) const;

	const PokerEffectChoiceLayout& GetPokerEffectChoiceLayout() const { return pokerEffectLayout_; }

    const FieldUiLayout& GetFieldUiLayout() const { return layout_; }

private:
	void ApplyPokerOptionImageLayout_(const BattleController& battle);
	void ApplyFieldUiLayout_();
	void SetTextScale_(TextSprite* text, float s);

private:

	//==================================
	// UI文字と背景スプライト。頻繁に更新されるものはポインタを保持、カード説明テキストはキャッシュから都度取得。
	//==================================
	std::unique_ptr<TextSprite> cardDescText_;

	std::unique_ptr<TextSprite> deckCountText_;
	std::unique_ptr<TextSprite> discardCountText_;
	std::unique_ptr<TextSprite> handCountText_;
	std::unique_ptr<TextSprite> fieldCountText_;
	std::unique_ptr<TextSprite> pokerPreviewText_;
	std::unique_ptr<TextSprite> turnText_;
	std::unique_ptr<TextSprite> costText_;

	std::unique_ptr<TextSprite> clickChoiceText_;
	std::unique_ptr<TextSprite> endTurnButtonText_;

	std::unique_ptr<Sprite> pokerTitleImage_;
	std::array<std::unique_ptr<Sprite>, 5> pokerOptionImageSprites_;
	std::unique_ptr<Sprite> pokerInfoButtonImage_;
	std::unique_ptr<Sprite> pokerPreviewTitleImage_;

	//ui用の背景スプライト
    std::unique_ptr<Sprite>     turnTextBg_;
    std::unique_ptr<Sprite>     costTextBg_;
    std::unique_ptr<Sprite> cardDescBg_;
    std::unique_ptr<Sprite> deckCountBg_;
    std::unique_ptr<Sprite> discardCountBg_;
    std::unique_ptr<Sprite> handCountBg_;
    std::unique_ptr<Sprite> fieldCountBg_;
    std::unique_ptr<Sprite> pokerOptionBgs_[5];
    std::unique_ptr<Sprite> modalOverlayBg_;
    std::unique_ptr<Sprite> pokerPreviewBg_;
    std::unique_ptr<Sprite> pokerActivateDescBg_;
    std::unique_ptr<Sprite> pokerEffectDescBg_;
	std::unique_ptr<Sprite> clickChoiceBg_;
    std::unique_ptr<Sprite> endTurnButtonBg_;

    std::unordered_map<int, std::unique_ptr<TextSprite>> cardDescSpriteCache_;
    TextSprite* activeCardDescText_ = nullptr;

	//キャッシュ用メンバ
	std::wstring lastPokerPreviewText_;
	bool lastPokerPreviewVisible_ = false;

	bool showPokerOptions_ = false;
	int pokerHoverIndex_ = -1;
	int pokerOptionCount_ = 0;
	bool showDescBg_ = false;

	//ターン終了用変数
	bool showEndTurnButton_ = false;
	bool endTurnHovered_ = false;

	DescMode lastDescMode_ = DescMode::None;
	int lastPreviewDefId_ = -1;
	std::wstring lastDescText_;

	PokerEffectChoiceLayout pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();
	std::string pokerEffectLayoutPath_ = "resources/ui/poker_effect_choice_ui.json";

	FieldUiLayout layout_{};
	std::string layoutPath_ = "resources/ui/field_ui_layout.json";

	// ポーカーUI文字キャッシュ
	//BattleController::PokerHandRank cachedPokerBonusRank_ = BattleController::PokerHandRank::None;

private:
	static std::wstring Utf8ToWString_(const std::string& s);

	TextSprite* GetOrCreateCardDescSprite_(GameApp& app, const CardDef& def);


	Vector2 debugPosition_ = {};

};