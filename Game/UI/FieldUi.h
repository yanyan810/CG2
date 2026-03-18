#pragma once
#include <memory>
#include <string>
#include "UiLayout.h"

class GameApp;
class BattleController;
class TextSprite;
class Sprite;

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

    const PokerEffectChoiceLayout& GetPokerEffectChoiceLayout() const { return pokerEffectLayout_; }

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

    std::unique_ptr<TextSprite> pokerTitleText_;
    std::unique_ptr<TextSprite> pokerOptionTexts_[4];
    std::unique_ptr<Sprite> pokerOptionBgs_[4];

    bool showPokerOptions_ = false;
    int pokerHoverIndex_ = -1;
    int pokerOptionCount_ = 0;
    bool showDescBg_ = false;

    DescMode lastDescMode_ = DescMode::None;
    int lastPreviewDefId_ = -1;
    std::wstring lastDescText_;

    PokerEffectChoiceLayout pokerEffectLayout_ = MakeDefaultPokerEffectChoiceLayout();
    std::string pokerEffectLayoutPath_ = "resources/ui/poker_effect_choice_ui.json";

private:
    static std::wstring Utf8ToWString_(const std::string& s);
};