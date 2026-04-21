#pragma once
#include <memory>
#include <string>
#include "UiLayout.h"
#include "TextSprite.h"
#include "Sprite.h"
#include "CardDef.h"
#include "BattleController.h"

#include <vector>
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

	static constexpr int kMaxUiDigits = 3;

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
	bool LoadCardShowUiLayout(const std::string& path);
	bool SaveCardShowUiLayout(const std::string& path) const;

	const PokerEffectChoiceLayout& GetPokerEffectChoiceLayout() const { return pokerEffectLayout_; }

	const FieldUiLayout& GetFieldUiLayout() const { return layout_; }

	void UpdateNumberSprites_(std::array<std::unique_ptr<Sprite>, kMaxUiDigits>& digits,
		int value, float x, float y, float scale, float spacing);

	void UpdatePokerEffectValueSprites_(const BattleController& battle);

	//数字用レイアウトのゲッター
	const UiNumberLayout& GetUiNumberLayout() const { return numberLayout_; }

	void SetForcedPokerHoverIndex(int index) { forcedPokerHoverIndex_ = index; }

public:

	//=======================
	//デバッグ用
	//=======================

	struct DebugPokerPreviewData {
		bool enabled = false;

		BattleController::PokerHandRank rank = BattleController::PokerHandRank::ThreeOfAKind;

		int atkUp = 5;
		int draw = 2;
		int damage = 15;

		std::vector<std::wstring> turnStartLines;
		std::vector<std::wstring> activatedLines;
	};

	enum class PokerPreviewEffectKind {
		None,
		SingleDamage,
		AllDamage,
		Draw,
		Block,
		Heal
	};

	PokerPreviewEffectKind ClassifyPreviewEffectKind_(const std::wstring& line) const;

	const UiPokerPreviewLineAnchor& GetPreviewEffectAnchor_(
		PokerPreviewEffectKind kind,
		const UiPokerPreviewEffectAnchors& anchors,
		int laneIndex) const;

	void SetDebugCardDescVisible(bool visible) { debugCardDescVisible_ = visible; }
	void SetDebugCardDescText(const std::wstring& text) { debugCardDescText_ = text; }
	void SetDebugImageCardDescVisible(bool visible) { debugImageCardDescVisible_ = visible; }
	void SetDebugImageCardDescCard(const CardDef* def) { debugImageCardDescCard_ = def; }

	void SetEditCardId(int cardId) { editCardId_ = (cardId > 0) ? cardId : 1; }
	int GetEditCardId() const { return editCardId_; }//Id対応

	void SetDebugPokerPreviewVisible(bool v);

	void SetDebugPokerPreviewData(const DebugPokerPreviewData& data);
	void ClearDebugPokerPreviewData();

private:

	void ApplyFieldUiLayout_();
	void SetTextScale_(TextSprite* text, float s);

	bool LoadUiNumberLayout(const std::string& path);
	bool SaveUiNumberLayout(const std::string& path) const;

	//カード説明のカスタムレイアウト関連
	const UiCardDescCustomLayout& GetCardDescCustomLayout_(int cardId) const;
	UiCardDescCustomLayout& GetOrCreateCardDescCustomLayout_(int cardId);
	bool IsCustomDescCardId_(int cardId) const;
	const UiCustomDescImageLayout& GetCustomDescImageLayout_(int cardId) const;
	UiCustomDescImageLayout& GetOrCreateCustomDescImageLayout_(int cardId);

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

	std::unique_ptr<Sprite> deckLabelImage_; //デッキ
	std::unique_ptr<Sprite> discardLabelImage_; //墓地
	std::unique_ptr<Sprite> handLabelImage_; //手札

	std::unique_ptr<TextSprite> pokerTitleText_;
	std::unique_ptr<TextSprite> pokerInfoButtonText_;
	std::array<std::unique_ptr<TextSprite>, 5> pokerOptionTexts_;

	std::unique_ptr<TextSprite> deckLabelText_;
	std::unique_ptr<TextSprite> discardLabelText_;
	std::unique_ptr<TextSprite> handLabelText_;

	bool useImageCardDesc_ = false;

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
	std::unique_ptr<Sprite> pokerInfoButtonBg_;
	std::unique_ptr<Sprite> endTurnButtonBg_;

	std::unordered_map<int, std::unique_ptr<TextSprite>> cardDescSpriteCache_;
	TextSprite* activeCardDescText_ = nullptr;

	//キャッシュ用メンバ
	std::wstring lastPokerPreviewText_;
	bool lastPokerPreviewVisible_ = false;

	bool showPokerOptions_ = false;
	int pokerHoverIndex_ = -1;
	int forcedPokerHoverIndex_ = -1;
	int pokerOptionCount_ = 0;
	bool showDescBg_ = false;

	//ターン終了用変数
	bool showEndTurnButton_ = false;
	bool endTurnHovered_ = false;

	DescMode lastDescMode_ = DescMode::None;
	int lastPreviewDefId_ = -1;
	std::wstring lastDescText_;

	//レゾナンス用
	PokerEffectChoiceLayout pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();
	std::string pokerEffectLayoutPath_ = "resources/ui/poker_effect_choice_ui.json";

	//フィールド用
	FieldUiLayout layout_{};
	std::string layoutPath_ = "resources/ui/field_ui_layout.json";
	std::string cardShowLayoutPath_ = "resources/ui/card_show_ui.json";

	//数字用
	UiNumberLayout numberLayout_{};
	std::string numberLayoutPath_ = "resources/ui/num_layout.json";


	//数字用スプライト
	std::array<std::unique_ptr<Sprite>, kMaxUiDigits> deckCountDigits_;
	std::array<std::unique_ptr<Sprite>, kMaxUiDigits> discardCountDigits_;
	std::array<std::unique_ptr<Sprite>, kMaxUiDigits> handCountDigits_;

	// 特殊効果3択の数値表示用
	std::array<std::array<std::unique_ptr<Sprite>, kMaxUiDigits>, 3> pokerEffectValueDigits_;

	//========================
	// デバッグ用
	//========================


	bool debugCardDescVisible_ = false;
	std::wstring debugCardDescText_;
	bool debugImageCardDescVisible_ = false;
	const CardDef* debugImageCardDescCard_ = nullptr;

	bool debugShowPokerPreview_ = false;

	//カード説明のカスタムレイアウト。カードIDごとに個別のレイアウトを指定できる。UIレイアウトのJSONにはない、カード固有のレイアウト調整に使う。
	std::unordered_map<int, UiCardDescCustomLayout> perCardDescCustomLayouts_;
	int editCardId_ = 1; // ImGuiで今編集するカードID
	std::unordered_map<int, UiCustomDescImageLayout> perCardCustomDescImageLayouts_;


	DebugPokerPreviewData debugPokerPreviewData_;

private:
	static std::wstring Utf8ToWString_(const std::string& s);

	TextSprite* GetOrCreateCardDescSprite_(GameApp& app, const CardDef& def);

	GameApp* app_ = nullptr;

	Vector2 debugPosition_ = {};

public:


	void SetTutorialInputLocked(bool locked) { tutorialInputLocked_ = locked; }
	bool IsTutorialInputLocked() const { return tutorialInputLocked_; }


private:

	bool tutorialInputLocked_ = false;

private:

	struct UiCountTextLayout {
		float offsetX = 80.0f;   // ラベルからの横距離
		float offsetY = 0.0f;    // 微調整
		float scale = 0.5f;
	};

	UiCountTextLayout deckCountTextLayout_ = { 103.0f, 0.0f, 1.3f };
	UiCountTextLayout discardCountTextLayout_ = { 83.0f, 0.0f, 1.3f };
	UiCountTextLayout handCountTextLayout_ = { 89.0f, 0.0f, 1.3f };


public:

};