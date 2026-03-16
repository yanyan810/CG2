#pragma once
#include <memory>
#include <string>

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

    bool showDescBg_ = false;

    DescMode lastDescMode_ = DescMode::None;
    int lastPreviewDefId_ = -1;
    std::wstring lastDescText_;

private:
    static std::wstring Utf8ToWString_(const std::string& s);
};