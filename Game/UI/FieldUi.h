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


	//デバッグ用
	void SetDebugCardDescVisible(bool visible) { debugCardDescVisible_ = visible; }
	void SetDebugCardDescText(const std::wstring& text) { debugCardDescText_ = text; }
	void SetDebugImageCardDescVisible(bool visible) { debugImageCardDescVisible_ = visible; }
	void SetDebugImageCardDescCard(const CardDef* def) { debugImageCardDescCard_ = def; }

private:
	void ApplyPokerOptionImageLayout_(const BattleController& battle);
	void ApplyFieldUiLayout_();
	void SetTextScale_(TextSprite* text, float s);

	bool LoadUiNumberLayout(const std::string& path);
	bool SaveUiNumberLayout(const std::string& path) const;

	std::string GetTriggerImagePath_(SubEffectTrigger trigger) const;
	std::string GetRankImagePath_(BattleController::PokerHandRank rank) const;
	std::string GetConditionSuffixImagePath_(const CardSubEffectDef& sub) const;
	std::string GetEffectTypeImagePath_(const CardEffectDef& effect) const;
	std::string GetEffectTargetImagePath_(const CardEffectDef& effect) const;
	std::string GetEffectParticleImagePath_(const CardEffectDef& effect) const;

	//カード説明のカスタムレイアウト関連
	const UiCardDescCustomLayout& GetCardDescCustomLayout_(int cardId) const;
	UiCardDescCustomLayout& GetOrCreateCardDescCustomLayout_(int cardId);

	void UpdatePreviewCardImageDesc_(const BattleController& battle);
	void HidePreviewCardImageDesc_();
	void DrawPreviewCardImageDesc_(const Matrix4x4& view, const Matrix4x4& proj);

	//デバッグ用
	void UpdatePreviewCardImageDescFromDef_(const CardDef* def);

private:

	struct PreviewImageCommand {
		std::string texturePath;
		Vector2 position{};
		Vector3 scale{ 1.0f, 1.0f, 1.0f };
		Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
		std::unique_ptr<Sprite> sprite;
	};

	void AddPreviewImageCommand_(
		const std::string& texturePath,
		float x, float y,
		float sx = 1.0f, float sy = 1.0f,
		Vector4 color = { 1.0f,1.0f,1.0f,1.0f });

	void AddPreviewNumberCommands_(
		int value,
		float x, float y,
		float scale,
		float spacing);

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

	std::unique_ptr<Sprite> deckLabelImage_; //デッキ
	std::unique_ptr<Sprite> discardLabelImage_; //墓地
	std::unique_ptr<Sprite> handLabelImage_; //手札

	bool useImageCardDesc_ = true;

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

	std::vector<PreviewImageCommand> previewImageCommands_;
	//std::unique_ptr<Sprite> previewWorkSprite_;

	//デバッグ用
	bool debugCardDescVisible_ = false;
	std::wstring debugCardDescText_;
	bool debugImageCardDescVisible_ = false;
	const CardDef* debugImageCardDescCard_ = nullptr;

	//カード説明のカスタムレイアウト。カードIDごとに個別のレイアウトを指定できる。UIレイアウトのJSONにはない、カード固有のレイアウト調整に使う。
	std::unordered_map<int, UiCardDescCustomLayout> perCardDescCustomLayouts_;
	int editCardId_ = 1; // ImGuiで今編集するカードID



private:
	static std::wstring Utf8ToWString_(const std::string& s);

	TextSprite* GetOrCreateCardDescSprite_(GameApp& app, const CardDef& def);

	GameApp* app_ = nullptr;

	Vector2 debugPosition_ = {};

};