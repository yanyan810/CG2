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
    void Draw(GameApp& app);

#ifdef USE_IMGUI
    void DrawImGui();
#endif

    bool LoadPokerEffectChoiceLayout(const std::string& path);
    bool SavePokerEffectChoiceLayout(const std::string& path) const;

    bool LoadFieldUiLayout(const std::string& path);
    bool SaveFieldUiLayout(const std::string& path) const;

    const PokerEffectChoiceLayout& GetPokerEffectChoiceLayout() const { return pokerEffectLayout_; }

    void UpdateDebugPokerEffectPreview(int hoverIndex = -1);

private:
    void ApplyFieldUiLayout_();
    void SetTextScale_(TextSprite* text, float s);

private:
    std::unique_ptr<TextSprite> cardDescText_;

    std::unique_ptr<TextSprite> deckCountText_;
    std::unique_ptr<TextSprite> discardCountText_;
    std::unique_ptr<TextSprite> handCountText_;
    std::unique_ptr<TextSprite> fieldCountText_;

    std::unique_ptr<Sprite> cardDescBg_;
    std::unique_ptr<Sprite> deckCountBg_;
    std::unique_ptr<Sprite> discardCountBg_;
    std::unique_ptr<Sprite> handCountBg_;
    std::unique_ptr<Sprite> fieldCountBg_;

    std::unique_ptr<TextSprite> turnText_;
    std::unique_ptr<Sprite>     turnTextBg_;

    std::unique_ptr<TextSprite> costText_;
    std::unique_ptr<Sprite>     costTextBg_;

    std::unique_ptr<TextSprite> pokerTitleText_;
    std::unique_ptr<TextSprite> pokerOptionTexts_[4];
    std::unique_ptr<Sprite> pokerOptionBgs_[4];

    std::unique_ptr<Sprite> modalOverlayBg_;

    std::unordered_map<int, std::unique_ptr<TextSprite>> cardDescSpriteCache_;
    TextSprite* activeCardDescText_ = nullptr;

    bool showPokerOptions_ = false;
    int pokerHoverIndex_ = -1;
    int pokerOptionCount_ = 0;
    bool showDescBg_ = false;

    DescMode lastDescMode_ = DescMode::None;
    int lastPreviewDefId_ = -1;
    std::wstring lastDescText_;

    PokerEffectChoiceLayout pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();
    std::string pokerEffectLayoutPath_ = "resources/ui/poker_effect_choice_ui.json";

    FieldUiLayout layout_{};
    std::string layoutPath_ = "resources/ui/field_ui_layout.json";

    // ポーカーUI文字キャッシュ
    BattleController::PokerHandRank cachedPokerBonusRank_ = BattleController::PokerHandRank::None;

    void BuildStaticPokerUiTexts_();
    void UpdateDynamicPokerBonusTexts_(const BattleController& battle);

private:
    static std::wstring Utf8ToWString_(const std::string& s);

    TextSprite* GetOrCreateCardDescSprite_(GameApp& app, const CardDef& def);

};