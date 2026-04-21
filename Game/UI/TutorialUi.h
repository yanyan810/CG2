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
        TutorialManager& tutorial,
        const BattleController& battle);

    void DrawDimOverlay(GameApp& app);

    void DrawDebugCardGuide(GameApp& app);

#ifdef USE_IMGUI
    void DrawImGui(TutorialManager& tutorial);
#endif

    bool LoadLayout(const std::string& path);
    bool SaveLayout(const std::string& path) const;

private:

    //カードのコスなどの説明用レイアウト

    struct GuideCircleLayout {
        UiVec2 center{ 0.0f, 0.0f };
        UiVec2 size{ 96.0f, 96.0f };
        Vector4 color{ 1.0f, 1.0f, 0.2f, 0.30f };
    };

    struct GuideBoxLayout {
        UiRect bg{ 0.0f, 0.0f, 260.0f, 96.0f };
        UiVec2 textPos{ 0.0f, 0.0f };
        float textScale = 1.0f;
        Vector4 color{ 1.0f, 1.0f, 1.0f, 0.95f };
    };

    struct GuideArrowLayout {
        UiRect rect{ 0.0f, 0.0f, 120.0f, 32.0f };
        bool flipX = false;
    };

    struct CardGuideLayout {
        bool enable = true;

        GuideCircleLayout costCircle;
        GuideCircleLayout suitCircle;
        GuideCircleLayout numberCircle;

        GuideBoxLayout costBox;
        GuideBoxLayout suitBox;
        GuideBoxLayout numberBox;

        GuideArrowLayout costArrow;
        GuideArrowLayout suitArrow;
        GuideArrowLayout numberArrow;
    };


    static constexpr int kMaxFocusFrames = 5;

    std::unique_ptr<Sprite> bg_;
    std::unique_ptr<TextSprite> text_;
    std::unique_ptr<Sprite> darkOverlay_;

    std::unique_ptr<Sprite> dimOverlay_;

    std::array<std::unique_ptr<Sprite>, kMaxFocusFrames> focusFrames_;

    float textAlpha_ = 0.0f;
    float focusBlink_ = 0.0f;
    std::wstring prevText_;

    TutorialUiLayout layout_ = MakeDefaultTutorialUiLayout();
    std::string layoutPath_ = "resources/ui/tutorial_ui_layout.json";

    CardGuideLayout cardGuideLayout_{};

    std::array<std::unique_ptr<Sprite>, 3> guideCircles_;
    std::array<std::unique_ptr<Sprite>, 3> guideArrows_;

    std::array<std::unique_ptr<Sprite>, 3> guideBoxBgs_;
    std::array<std::unique_ptr<TextSprite>, 3> guideBoxTexts_;


private:
    std::vector<UiRect> ResolveFocusRects_(
        const TutorialManager& tutorial,
        const BattleController& battle,
        const FieldUi& fieldUi) const;

    //拡大したカードの説明用関数
    void DrawCardGuide_(
        const TutorialManager& tutorial,
        const Matrix4x4& view,
        const Matrix4x4& proj);

    void DrawGuideCircle_(
        int index,
        const GuideCircleLayout& layout,
        const Matrix4x4& view,
        const Matrix4x4& proj);

    void DrawGuideArrow_(
        int index,
        const GuideArrowLayout& layout,
        const Matrix4x4& view,
        const Matrix4x4& proj);

    void DrawGuideBox_(
        int index,
        const GuideBoxLayout& layout,
        const std::wstring& text,
        const Matrix4x4& view,
        const Matrix4x4& proj);

};