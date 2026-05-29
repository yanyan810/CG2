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

struct UiCustomDescImageLayout {
    float x = 0.0f;
    float y = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

struct UiPreviewImageItem {
    float x = 0.0f;
    float y = 0.0f;
    float scale = 0.75f;
};

struct UiPreviewNumberItem {
    float x = 0.0f;
    float y = 0.0f;
    float scale = 0.28f;
    float spacing = 28.0f;
};

struct UiPokerPreviewLineAnchor {
    float x = 0.0f;
    float y = 0.0f;
};

struct UiPokerPreviewPatternLayout {
    float labelScale = 0.75f;

    float prefixOffsetX = 0.0f;
    float prefixOffsetY = 0.0f;

    float numberOffsetX = 185.0f;
    float numberOffsetY = 8.0f;
    float numberScale = 0.28f;
    float numberSpacing = 28.0f;

    float suffixOffsetX = 20.0f;
    float suffixOffsetY = 0.0f;

    float leadingLabelOffsetX = 0.0f;
    float leadingLabelOffsetY = 0.0f;
    float leadingAdvanceX = 0.0f;
};

struct UiPokerPreviewPatternSet {
    UiPokerPreviewPatternLayout singleDamage;
    UiPokerPreviewPatternLayout allDamage;
    UiPokerPreviewPatternLayout draw;
    UiPokerPreviewPatternLayout block;
    UiPokerPreviewPatternLayout heal;
};

struct UiPokerPreviewLinesLayout {
    UiPokerPreviewLineAnchor lanes[5];
};

struct UiPokerPreviewEffectAnchors {
    UiPokerPreviewLinesLayout singleDamage;
    UiPokerPreviewLinesLayout allDamage;
    UiPokerPreviewLinesLayout draw;
    UiPokerPreviewLinesLayout block;
    UiPokerPreviewLinesLayout heal;
    UiPokerPreviewLinesLayout none;
};

struct UiPokerPreviewImageLayout {
    UiPreviewImageItem rank;

    UiPreviewImageItem atkLabel;
    UiPreviewNumberItem atkValue;

    UiPreviewImageItem drawLabel;
    UiPreviewNumberItem drawValue;

    UiPreviewImageItem damageLabel;
    UiPreviewNumberItem damageValue;

    UiPreviewImageItem turnStartLabel;
    UiPreviewImageItem turnStartNoneLabel;

    UiPreviewImageItem activatedLabel;
    UiPreviewImageItem activatedNoneLabel;

    float turnStartLineStepY = 62.0f;
    float activatedLineStepY = 62.0f;

    UiPokerPreviewEffectAnchors turnStartEffectAnchors;
    UiPokerPreviewEffectAnchors activatedEffectAnchors;

    UiPokerPreviewPatternSet turnStartPatterns;
    UiPokerPreviewPatternSet activatedPatterns;
};

struct PokerEffectChoiceLayout {
    UiVec2 titleImage;

    UiRect activateTitleBg;
    UiRect activateYesRect;
    UiVec2 activateYesImage;
    UiRect activateNoRect;
    UiVec2 activateNoImage;
    UiRect activateViewBoardRect;
    UiVec2 activateViewBoardImage;

    UiRect effectTitleBg;

    UiRect backRect;
    UiVec2 backImage;

    UiRect effectRects[3];
    UiVec2 effectImages[3];

    UiRect effectViewBoardRect;
    UiVec2 effectViewBoardImage;

    UiPokerPreviewImageLayout previewImages;

    UiRect infoButtonRect;
    UiText infoButtonImage;

    UiRect previewPanelBg;
    UiText previewPanelTitleImage;
    UiText previewPanelText;

    // 霑ｽ蜉
    UiText activateTitleText;
    UiText activateYesText;
    UiText activateNoText;
    UiText activateViewBoardText;
    UiText infoButtonText;

    UiText effectTitleText;
    UiText backText;
    UiText effectTexts[3];
    UiText effectViewBoardText;
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
    UiImageItem particle;   // 縺ｫ / 縺ｯ
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
    UiImageItem particle;   // 縺ｫ / 縺ｯ
    UiImageItem effectType;
    UiNumberItem value;
};

struct UiCardDescCustomLayout {
    UiImageItem titleBasicEffect;
    UiImageItem separator;
    UiCardDescBaseRowLayout baseRows[3];
    UiCardDescSubBlockLayout subBlocks[3];
};

struct UiCostMeterLayout {
    float pipOriginX = 1083.0f;
    float pipOriginY = 243.0f;
    UiVec2 pipOffsets[10];
    float pipScale = 1.0f;
    float pipGapX = 18.0f;
    float pipGapY = 16.0f;
    float pipRadius = 7.5f;
    bool postEffectEnabled = true;
    float postThreshold = 0.0f;
    float postIntensity = 1.8f;
    float postChromAbAmount = 0.003f;
    float postDistortionAmount = 0.0f;
    float postNoiseIntensity = 0.0f;
    float filledLightIntensity = 1.65f;
    float emptyLightIntensity = 0.45f;
    float emptyColorR = 0.04f;
    float emptyColorG = 0.12f;
    float emptyColorB = 0.08f;
    float lightColorR = 1.0f;
    float lightColorG = 1.0f;
    float lightColorB = 1.0f;
    float currentTextX = 1218.0f;
    float currentTextY = 250.0f;
    float maxTextX = 1220.0f;
    float maxTextY = 275.0f;
    float currentTextScale = 0.85f;
    float maxTextScale = 0.72f;
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
    UiCostMeterLayout costMeter;

    UiRect endTurnBg;
    UiText endTurnText;

    UiText deckLabelImage;
    UiText discardLabelImage;
    UiText handLabelImage;

    UiRect overlay;

    UiCardDescImageLayout cardDescImage;      // 譌｢蟄・
    UiCardDescCustomLayout cardDescCustom;    // 霑ｽ蜉
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

struct SelectSceneLayout {
    UiRect titleBoxRect{ 480.0f, 145.0f, 320.0f, 88.0f };
    UiRect tutorialButtonRect{ 820.0f, 200.0f, 300.0f, 90.0f };
    UiRect battleButtonRect{ 820.0f, 320.0f, 300.0f, 90.0f };
    UiRect deckEditButtonRect{ 820.0f, 440.0f, 300.0f, 90.0f };
    UiRect backButtonRect{ 150.0f, 110.0f, 180.0f, 70.0f };
};

struct StageSelectLayout {
    UiVec2 titlePos{ 470.0f, 80.0f };

    UiRect tutorialButtonRect{ 320.0f, 300.0f, 340.0f, 120.0f };
    UiRect battleButtonRect{ 320.0f, 500.0f, 340.0f, 120.0f };
    UiRect deckEditButtonRect{ 780.0f, 500.0f, 340.0f, 120.0f };

    UiRect backButtonRect{ 24.0f, 24.0f, 140.0f, 56.0f };
    UiRect leftArrowRect{ 120.0f, 410.0f, 80.0f, 100.0f };
    UiRect rightArrowRect{ 1080.0f, 410.0f, 80.0f, 100.0f };

    UiRect stages1_5Rect{ 320.0f, 200.0f, 400.0f, 100.0f };
    UiRect stages6_9Rect{ 320.0f, 200.0f, 400.0f, 100.0f };
    UiRect bossStageRect{ 320.0f, 200.0f, 400.0f, 100.0f };

    UiVec2 battleTitleTextOffset{ 58.0f, 38.0f };

    UiVec2 tutorialButtonVisualPos{ 320.0f, 300.0f };
    UiVec2 battleButtonVisualPos{ 320.0f, 500.0f };
	UiVec2 deckEditButtonVisualPos{ 580.0f, 500.0f };

    UiVec2 tutorialDescBgPos{ 760.0f, 300.0f };
    UiVec2 battleDescBgPos{ 760.0f, 500.0f };
    UiVec2 deckEditDescBgPos{ 1160.0f, 500.0f };

    UiText tutorialDescText{ 780.0f, 325.0f, 1.0f };
    UiText battleDescText{ 780.0f, 525.0f, 1.0f };
    UiText deckEditDescText{ 300.0f, 475.0f, 1.0f };
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

    l.previewImages.rank = { 725.0f, 185.0f, 0.75f };

    l.previewImages.atkLabel = { 725.0f, 235.0f, 0.75f };
    l.previewImages.atkValue = { 905.0f, 243.0f, 0.28f, 28.0f };

    l.previewImages.drawLabel = { 725.0f, 295.0f, 0.75f };
    l.previewImages.drawValue = { 905.0f, 303.0f, 0.28f, 28.0f };

    l.previewImages.damageLabel = { 725.0f, 355.0f, 0.75f };
    l.previewImages.damageValue = { 905.0f, 363.0f, 0.28f, 28.0f };

    l.previewImages.turnStartLabel = { 725.0f, 425.0f, 0.75f };
    l.previewImages.turnStartNoneLabel = { 725.0f, 475.0f, 0.75f };

    l.previewImages.activatedLabel = { 725.0f, 525.0f, 0.75f };
    l.previewImages.activatedNoneLabel = { 725.0f, 585.0f, 0.75f };

    l.previewImages.turnStartLineStepY = 62.0f;
    l.previewImages.activatedLineStepY = 62.0f;

    l.activateTitleText = { 510.0f, 170.0f, 1.0f };
    l.activateYesText = { 235.0f, 470.0f, 1.0f };
    l.activateNoText = { 915.0f, 470.0f, 1.0f };
    l.activateViewBoardText = { 555.0f, 615.0f, 1.0f };
    l.infoButtonText = { 1098.0f, 54.0f, 0.9f };

    l.effectTitleText = { 515.0f, 170.0f, 1.0f };
    l.backText = { 78.0f, 78.0f, 1.0f };
    l.effectTexts[0] = { 95.0f, 280.0f, 0.95f };   // ATK UP
    l.effectTexts[1] = { 545.0f, 280.0f, 0.95f };  // 繝繝｡繝ｼ繧ｸ
    l.effectTexts[2] = { 993.0f, 280.0f, 0.95f };  // 繝峨Ο繝ｼ
    l.effectViewBoardText = { 555.0f, 655.0f, 1.0f };

    // 繧ｿ繝ｼ繝ｳ髢句ｧ区凾
    for (int i = 0; i < 5; ++i) {
        float y = 475.0f + 62.0f * i;

        l.previewImages.turnStartEffectAnchors.singleDamage.lanes[i] = { 725.0f, y };
        l.previewImages.turnStartEffectAnchors.allDamage.lanes[i] = { 725.0f, y };
        l.previewImages.turnStartEffectAnchors.draw.lanes[i] = { 725.0f, y };
        l.previewImages.turnStartEffectAnchors.block.lanes[i] = { 725.0f, y };
        l.previewImages.turnStartEffectAnchors.heal.lanes[i] = { 725.0f, y };
        l.previewImages.turnStartEffectAnchors.none.lanes[i] = { 725.0f, y };
    }

    // 迚ｹ谿雁柑譫懃匱蜍墓凾
    for (int i = 0; i < 5; ++i) {
        float y = 585.0f + 62.0f * i;

        l.previewImages.activatedEffectAnchors.singleDamage.lanes[i] = { 725.0f, y };
        l.previewImages.activatedEffectAnchors.allDamage.lanes[i] = { 725.0f, y };
        l.previewImages.activatedEffectAnchors.draw.lanes[i] = { 725.0f, y };
        l.previewImages.activatedEffectAnchors.block.lanes[i] = { 725.0f, y };
        l.previewImages.activatedEffectAnchors.heal.lanes[i] = { 725.0f, y };
        l.previewImages.activatedEffectAnchors.none.lanes[i] = { 725.0f, y };
    }

    // -------------------------
    // TurnStart Patterns
    // -------------------------
    l.previewImages.turnStartPatterns.singleDamage.labelScale = 0.75f;
    l.previewImages.turnStartPatterns.singleDamage.prefixOffsetX = 0.0f;
    l.previewImages.turnStartPatterns.singleDamage.prefixOffsetY = 0.0f;
    l.previewImages.turnStartPatterns.singleDamage.numberOffsetX = 185.0f;
    l.previewImages.turnStartPatterns.singleDamage.numberOffsetY = 8.0f;
    l.previewImages.turnStartPatterns.singleDamage.numberScale = 0.28f;
    l.previewImages.turnStartPatterns.singleDamage.numberSpacing = 28.0f;
    l.previewImages.turnStartPatterns.singleDamage.suffixOffsetX = 20.0f;
    l.previewImages.turnStartPatterns.singleDamage.suffixOffsetY = 0.0f;

    l.previewImages.turnStartPatterns.allDamage = l.previewImages.turnStartPatterns.singleDamage;

    l.previewImages.turnStartPatterns.draw.labelScale = 0.75f;
    l.previewImages.turnStartPatterns.draw.prefixOffsetX = 0.0f;
    l.previewImages.turnStartPatterns.draw.prefixOffsetY = 0.0f;
    l.previewImages.turnStartPatterns.draw.numberOffsetX = 0.0f;
    l.previewImages.turnStartPatterns.draw.numberOffsetY = 8.0f;
    l.previewImages.turnStartPatterns.draw.numberScale = 0.28f;
    l.previewImages.turnStartPatterns.draw.numberSpacing = 28.0f;
    l.previewImages.turnStartPatterns.draw.suffixOffsetX = 20.0f;
    l.previewImages.turnStartPatterns.draw.suffixOffsetY = 0.0f;
    l.previewImages.turnStartPatterns.draw.leadingLabelOffsetX = 0.0f;
    l.previewImages.turnStartPatterns.draw.leadingLabelOffsetY = 0.0f;
    l.previewImages.turnStartPatterns.draw.leadingAdvanceX = 165.0f;

    l.previewImages.turnStartPatterns.block.labelScale = 0.75f;
    l.previewImages.turnStartPatterns.block.prefixOffsetX = 0.0f;
    l.previewImages.turnStartPatterns.block.prefixOffsetY = 0.0f;
    l.previewImages.turnStartPatterns.block.numberOffsetX = 185.0f;
    l.previewImages.turnStartPatterns.block.numberOffsetY = 8.0f;
    l.previewImages.turnStartPatterns.block.numberScale = 0.28f;
    l.previewImages.turnStartPatterns.block.numberSpacing = 28.0f;
    l.previewImages.turnStartPatterns.block.suffixOffsetX = 20.0f;
    l.previewImages.turnStartPatterns.block.suffixOffsetY = 0.0f;

    l.previewImages.turnStartPatterns.heal = l.previewImages.turnStartPatterns.block;

    // -------------------------
    // Activated Patterns
    // -------------------------
    l.previewImages.activatedPatterns = l.previewImages.turnStartPatterns;

    return l;
}

//=========================
//繝√Η繝ｼ繝医Μ繧｢繝ｫ逕ｨ
//=========================

struct TutorialUiLayout {
    UiRect messageBg;
    UiVec2 messageText;
    float explainCardMessageOffsetY = 550.0f;

    UiRect darkOverlay;

    // FieldUi縺ｫ蟆ら畑rect縺後↑縺・ｂ縺ｮ縺縺第戟縺､
    UiRect handArea;
    UiRect fieldArea;

    UiRect playerHpArea;
    UiRect playerBlockArea;
    UiRect playerPowerBoostArea;
    UiRect enemyHpArea;
    UiRect turnTextArea;
    UiRect roleTextArea;
    UiRect deckCountArea;
    UiRect pokerHandHelpArea;

    UiRect playerIncomingDamageArea;
    UiRect enemyNextActionArea;

};

inline TutorialUiLayout MakeDefaultTutorialUiLayout()
{
    TutorialUiLayout l{};

    l.messageBg = { 20.0f, 52.0f, 900.0f, 140.0f };
    l.messageText = { 40.0f, 80.0f };
    l.explainCardMessageOffsetY = 550.0f;

    l.darkOverlay = { 0.0f, 0.0f, 1280.0f, 720.0f };

    // 縺ｨ繧翫≠縺医★縺ｮ蛻晄悄蛟､縲ゅ≠縺ｨ縺ｧImGui縺ｧ蜷医ｏ縺帙ｋ
    l.handArea = { 170.0f, 560.0f, 950.0f, 140.0f };
    l.fieldArea = { 200.0f, 330.0f, 900.0f, 190.0f };

    l.playerHpArea = { 120.0f, 100.0f, 420.0f, 80.0f };
    l.enemyHpArea = { 1540.0f, 100.0f, 380.0f, 80.0f };
    l.turnTextArea = { 860.0f, 80.0f, 420.0f, 120.0f };
    l.roleTextArea = { 900.0f, 530.0f, 360.0f, 120.0f };
    l.deckCountArea = { 80.0f, 700.0f, 340.0f, 140.0f };

    l.playerHpArea = { 80.0f, 30.0f, 251.0f, 29.0f };
    l.playerBlockArea = { 425.0f, 2.0f, 64.0f, 64.0f };
    l.playerPowerBoostArea = { 496.0f, 8.0f, 56.0f, 56.0f };
    l.playerIncomingDamageArea = { 320.0f, 20.0f, 90.0f, 60.0f };

    l.enemyHpArea = { 997.0f, 30.0f, 208.0f, 31.0f };
    l.enemyNextActionArea = { 930.0f, 18.0f, 80.0f, 70.0f };
    return l;
}
