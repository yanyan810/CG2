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

struct UiCardDescImageLayout {
    float baseEffectOffsetX = 0.0f;
    float baseEffectOffsetY = 0.0f;
    float baseEffectScale = 1.0f;

    float baseEffectTypeOffsetX = 0.0f;
    float baseEffectTypeOffsetY = 42.0f;
    float baseEffectTypeScale = 1.0f;

    float baseColonOffsetX = 170.0f;
    float baseColonOffsetY = 42.0f;
    float baseColonScale = 1.0f;

    float baseValueOffsetX = 210.0f;
    float baseValueOffsetY = 42.0f;
    float baseValueScale = 0.35f;
    float baseValueSpacing = 28.0f;

    float separatorOffsetX = 0.0f;
    float separatorOffsetY = 92.0f;
    float separatorWidth = 320.0f;
    float separatorHeight = 2.0f;

    float triggerOffsetX = 0.0f;
    float triggerOffsetY = 0.0f;
    float triggerScale = 1.0f;

    float rankOffsetX = 0.0f;
    float rankOffsetY = 154.0f;
    float rankScale = 1.0f;

    float suffixOffsetX = 170.0f;
    float suffixOffsetY = 154.0f;
    float suffixScale = 1.0f;

    float subEffectTypeOffsetX = 0.0f;
    float subEffectTypeOffsetY = 196.0f;
    float subEffectTypeScale = 1.0f;

    float subColonOffsetX = 170.0f;
    float subColonOffsetY = 196.0f;
    float subColonScale = 1.0f;

    float subValueOffsetX = 210.0f;
    float subValueOffsetY = 196.0f;
    float subValueScale = 0.35f;
    float subValueSpacing = 28.0f;
};

struct PokerEffectChoiceLayout {
    // タイトル画像
    UiVec2 titleImage;

    // 発動確認UI
    UiRect activateTitleBg;
    UiRect activateYesRect;
    UiVec2 activateYesImage;
    UiRect activateNoRect;
    UiVec2 activateNoImage;
    UiRect activateViewBoardRect;
    UiVec2 activateViewBoardImage;

    // 効果選択UI
    UiRect effectTitleBg;

    UiRect backRect;
    UiVec2 backImage;

    UiRect effectRects[3];
    UiVec2 effectImages[3];

    UiRect effectViewBoardRect;
    UiVec2 effectViewBoardImage;

    // 共通
    UiRect infoButtonRect;
    UiText infoButtonImage;

    UiRect previewPanelBg;
    UiText previewPanelTitleImage;
    UiText previewPanelText;
};

struct UiImageItem {
    float x = 0.0f;
    float y = 0.0f;
    float scale = 1.0f;
};

struct UiNumberItem {
    float x = 0.0f;
    float y = 0.0f;
    float scale = 1.0f;
    float spacing = 28.0f;
};

struct UiCardDescBaseRowLayout {
    UiImageItem target;
    UiImageItem particle;   // に / は
    UiImageItem effectType;
    UiNumberItem value;

    UiImageItem special1;   // blockCountBlue
    UiImageItem special2;   // x1
    float specialAdvance = 250.0f;
};

struct UiCardDescSubBlockLayout {
    UiImageItem trigger;
    UiImageItem rank;
    UiImageItem suffix;

    UiImageItem target;
    UiImageItem particle;   // に / は
    UiImageItem effectType;
    UiNumberItem value;
};

struct UiCardDescCustomLayout {
    UiImageItem titleBasicEffect;
    UiImageItem separator;
    UiCardDescBaseRowLayout baseRows[3];
    UiCardDescSubBlockLayout subBlocks[3];
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

    UiText deckLabelImage;
    UiText discardLabelImage;
    UiText handLabelImage;

    UiRect overlay;

    UiCardDescImageLayout cardDescImage;      // 既存
    UiCardDescCustomLayout cardDescCustom;    // 追加
};

struct UiNumber {
    float x = 0.0f;
    float y = 0.0f;
    float scale = 1.0f;
    float spacing = 32.0f;
};

struct UiNumberRelative {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float scale = 1.0f;
    float spacing = 32.0f;
};

struct UiNumberLayout {
    UiNumber deckCount;
    UiNumber discardCount;
    UiNumber handCount;

    UiNumberRelative effectValue[3];
};

inline PokerEffectChoiceLayout MakeDefaultPokerEffectChoiceLayout()
{
    PokerEffectChoiceLayout l{};

    l.titleImage = { 420.0f, 120.0f };

    l.effectTitleBg = { 360.0f, 110.0f, 560.0f, 90.0f };

    l.backRect = { 30.0f, 40.0f, 170.0f, 90.0f };
    l.backImage = { 78.0f, 78.0f };

    l.effectRects[0] = { 32.0f, 200.0f, 320.0f, 185.0f };
    l.effectRects[1] = { 480.0f, 200.0f, 320.0f, 185.0f };
    l.effectRects[2] = { 928.0f, 200.0f, 320.0f, 185.0f };

    l.effectImages[0] = { 78.0f, 280.0f };
    l.effectImages[1] = { 585.0f, 280.0f };
    l.effectImages[2] = { 1015.0f, 280.0f };

    l.effectViewBoardRect = { 460.0f, 640.0f, 360.0f, 70.0f };
    l.effectViewBoardImage = { 555.0f, 655.0f };

    l.infoButtonRect = { 1080.0f, 40.0f, 160.0f, 60.0f };
    l.infoButtonImage = { 1112.0f, 54.0f, 0.9f };

    l.previewPanelBg = { 700.0f, 120.0f, 540.0f, 450.0f };
    l.previewPanelTitleImage = { 735.0f, 140.0f, 1.0f };
    l.previewPanelText = { 725.0f, 185.0f, 0.78f };

    l.activateTitleBg = { 380.0f, 140.0f, 520.0f, 120.0f };

    l.activateYesRect = { 120.0f, 430.0f, 360.0f, 120.0f };
    l.activateYesImage = { 225.0f, 478.0f };

    l.activateNoRect = { 800.0f, 430.0f, 360.0f, 120.0f };
    l.activateNoImage = { 890.0f, 478.0f };

    l.activateViewBoardRect = { 460.0f, 590.0f, 360.0f, 90.0f };
    l.activateViewBoardImage = { 555.0f, 615.0f };

    return l;
}