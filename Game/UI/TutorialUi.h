#pragma once
#include <memory>
#include <string>
#include "Sprite.h"
#include "TextSprite.h"
#include "UiLayout.h"

class GameApp;
class TutorialManager;
class BattleController;
class FieldUi;

class TutorialUi {
public:
    void Initialize(GameApp& app);

    void Update(GameApp& app,
        const TutorialManager& tutorial,
        const BattleController& battle,
        const FieldUi& fieldUi);

    void Draw(GameApp& app,
        const TutorialManager& tutorial,
        const BattleController& battle);

#ifdef USE_IMGUI
    void DrawImGui(TutorialManager& tutorial);
#endif

    bool LoadLayout(const std::string& path);
    bool SaveLayout(const std::string& path) const;

private:
    static constexpr int kMaxFocusFrames = 5;

    std::unique_ptr<Sprite> bg_;
    std::unique_ptr<TextSprite> text_;
    std::unique_ptr<Sprite> darkOverlay_;

    std::array<std::unique_ptr<Sprite>, kMaxFocusFrames> focusFrames_;

    float textAlpha_ = 0.0f;
    float focusBlink_ = 0.0f;
    std::wstring prevText_;

    TutorialUiLayout layout_ = MakeDefaultTutorialUiLayout();
    std::string layoutPath_ = "resources/ui/tutorial_ui_layout.json";

private:
    std::vector<UiRect> ResolveFocusRects_(
        const TutorialManager& tutorial,
        const BattleController& battle,
        const FieldUi& fieldUi) const;
};