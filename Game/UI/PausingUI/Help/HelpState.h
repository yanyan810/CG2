#pragma once
#include "../IPauseState.h"
#include "../../../Logic/Button.h"
#include "TextSprite.h"
#include "UiLayout.h"
#include <vector>
#include <memory>
#include <string>

class GameApp;

struct HelpItem {
    std::wstring title;
    std::wstring text;
    std::string imagePath;
    Vector2 imagePos{ 640.0f, 360.0f }; // 画面中央をデフォルト
    Vector2 imageScale{ 1.0f, 1.0f };
};

class HelpState : public IPauseState {
public:
    HelpState() = default;
    ~HelpState() override = default;

    void Initialize(GameApp& app) override;
    void Update(PausingUI* context, GameApp& app, Input* input) override;
    void Draw(GameApp& app) override;
    void DrawImGui() override;

private:
    void SaveLayout_() const;
    void SaveHelpItems_() const;
    void LoadLayout_();
    void ApplyLayout_();

    HelpUiLayout layout_{};
    std::string layoutPath_ = "resources/ui/help_ui_layout.json";

    std::vector<HelpItem> helpItems_;
    int selectedIndex_ = 0;
    std::vector<std::string> availableImages_;
    // Scrollbar UI elements
    std::unique_ptr<Sprite> scrollBarBg_; // background track
    std::unique_ptr<Sprite> scrollBarHandle_; // draggable knob

    // Dragging state for scrollbar
    bool isScrolling_ = false;          // true while mouse button held on track
    float scrollStartY_ = 0.0f;          // mouse Y at drag start
    float scrollStartPos_ = 0.0f;        // scrollY_ at drag start

    float scrollY_ = 0.0f;
    float maxScrollY_ = 0.0f;

    std::unique_ptr<Button> backButton_;
    std::vector<std::unique_ptr<Button>> itemButtons_;
    std::vector<std::unique_ptr<TextSprite>> itemTexts_;

    std::unique_ptr<Sprite> photoBg_;
    std::unique_ptr<Sprite> textBg_;
    std::unique_ptr<TextSprite> descText_;
};
