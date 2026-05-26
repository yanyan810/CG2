#pragma once
#include "IScene.h"
#include <vector>
#include <string>
#include <map>

#include "Camera.h"

#include "CardDatabase.h"
#include "Object3d.h"

#include "Sprite.h"
#include "TextSprite.h"

#include "Logic/Button.h"

class Card3D;



class DeckEditScene : public IScene {
public:
    void OnEnter(GameApp& app) override;
    void OnExit(GameApp& app) override;
    void Update(GameApp& app, float dt) override;
    void Draw3D(GameApp& app) override;
    void Draw2D(GameApp& app) override;
    void DrawImGui(GameApp& app) override;
    void DrawSkydome(GameApp& app) override;

private:

    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Object3d> skyDome_;

	CardDatabase* cardDB_ = nullptr; // カードデータベースへのポインタ

	std::map<int, int> editingDeck_; // カードIDと枚数のマップ
	int totalCount_ = 0;             // デッキ内のカードの合計枚数
;
    std::vector<std::unique_ptr<Card3D>> cardModels_;

    // レイアウト計算用の定数
    const float kCardStartX = -3.0f;// カードの最初の座標
    const float kCardStartY = 1.5; // カードの最初の座標
    const float kCardSpacingX = 1.5f;// カードの間の横幅
    const float kCardSpacingY = 2.f; // カードの間の立幅
    const int kCardsPerRow = 4;      

    float scrollY_ = 0.0f;
    const float kInitialScrollY = 2.0f; // 最初のスクロール量
    float kMaxScrollY = 20.0f;    // 上限 (カードリストの長さに応じて調整)
    const float kMinScrollY = 2.0f;

    bool isDeckValid_ = false;

    bool isHoverd_ = false;

    std::unique_ptr<Button> nosaveButton_;
    std::unique_ptr<Button> saveDeckButton_;
    std::unique_ptr<TextSprite> warningText_;
    std::unique_ptr<TextSprite> countText_;
    const std::wstring countTextSup_ = L"デッキ枚数\n";
	std::unique_ptr<TextSprite> controlHintText_;
	std::unique_ptr<Sprite> cardPreviewBg_;
    std::unique_ptr<TextSprite> cardPreviewText_;

    std::unique_ptr<Sprite> selectingTemplateDeckBg_ = nullptr;
    std::vector< std::unique_ptr<Button>> deckTemplateButtons_;

    bool isSelectingTemplateDeck_ = true;

	int cardCount_ = 0;

    void RebuildCardModels(GameApp& app);
    void RecalculateTotal();
    void UpdateSprites(GameApp& app);

    int PickCardIndex(GameApp& app);

    const CardDef* GetHoveredCardDef(GameApp& app);

    Vector2 GetPopupPosition(GameApp& app, int cardIdx);

    void RerollDeckData(GameApp& app);

};
