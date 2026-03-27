#pragma once
#include <string>

struct UiVec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct UiRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};


struct UiText {
    float x;
    float y;
    float scale;
};


struct PokerEffectChoiceLayout {
    UiVec2 titleText;

    // 発動確認UI
    UiRect activateTitleBg;
    UiRect activateYesRect;
    UiVec2 activateYesText;
    UiRect activateNoRect;
    UiVec2 activateNoText;
    UiRect activateViewBoardRect;
    UiVec2 activateViewBoardText;

    // 効果選択UI
    UiRect effectTitleBg;

    UiRect backRect;
    UiVec2 backText;

    UiRect effectRects[3];
    UiVec2 effectTexts[3];

    UiRect effectViewBoardRect;
    UiVec2 effectViewBoardText;

    // 共通
    UiRect infoButtonRect;
    UiText infoButtonText;

    UiRect previewPanelBg;
    UiText previewPanelTitle;
    UiText previewPanelText;
};

struct FieldUiLayout {

    UiRect cardDescBg;
    UiText cardDescText;

    UiRect deckBg;
    UiText deckText;

    UiRect discardBg;
    UiText discardText;

    UiRect handBg;
    UiText handText;

    UiRect fieldBg;
    UiText fieldText;

    UiRect turnBg;
    UiText turnText;

    UiRect costBg;
    UiText costText;

    UiRect endTurnBg;
    UiText endTurnText;

    UiRect overlay;
};

inline PokerEffectChoiceLayout MakeDefaultPokerEffectChoiceLayout()
{
    PokerEffectChoiceLayout l{};

    l.titleText = { 420.0f, 120.0f };

    l.effectTitleBg = { 360.0f, 110.0f, 560.0f, 90.0f };

    l.backRect = { 30.0f, 40.0f, 170.0f, 90.0f };
    l.backText = { 78.0f, 78.0f };

    l.effectRects[0] = { 32.0f, 200.0f, 320.0f, 185.0f };
    l.effectRects[1] = { 480.0f, 200.0f, 320.0f, 185.0f };
    l.effectRects[2] = { 928.0f, 200.0f, 320.0f, 185.0f };

    l.effectTexts[0] = { 78.0f, 280.0f };
    l.effectTexts[1] = { 585.0f, 280.0f };
    l.effectTexts[2] = { 1015.0f, 280.0f };

    l.effectViewBoardRect = { 460.0f, 640.0f, 360.0f, 70.0f };
    l.effectViewBoardText = { 555.0f, 655.0f };

    l.infoButtonRect = { 1080.0f, 40.0f, 160.0f, 60.0f };
    l.infoButtonText = { 1112.0f, 54.0f, 0.9f };

    l.previewPanelBg = { 700.0f, 120.0f, 540.0f, 450.0f };
    l.previewPanelTitle = { 735.0f, 140.0f, 1.0f };
    l.previewPanelText = { 725.0f, 185.0f, 0.78f };

    l.activateTitleBg = { 380.0f, 140.0f, 520.0f, 120.0f };

    l.activateYesRect = { 120.0f, 430.0f, 360.0f, 120.0f };
    l.activateYesText = { 225.0f, 478.0f };

    l.activateNoRect = { 800.0f, 430.0f, 360.0f, 120.0f };
    l.activateNoText = { 890.0f, 478.0f };

    l.activateViewBoardRect = { 460.0f, 590.0f, 360.0f, 90.0f };
    l.activateViewBoardText = { 555.0f, 615.0f };

    return l;
}