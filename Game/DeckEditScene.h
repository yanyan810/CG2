#pragma once
#include "IScene.h"
#include <vector>
#include <string>
#include <map>

#include "Camera.h"

#include "CardDatabase.h"

class Card3D;

class DeckEditScene : public IScene {
public:
    void OnEnter(GameApp& app) override;
    void OnExit(GameApp& app) override;
    void Update(GameApp& app, float dt) override;
    void Draw3D(GameApp& app) override;
    void Draw2D(GameApp& app) override;
    void DrawImGui(GameApp& app) override;

private:
    std::unique_ptr<Camera> camera_;

    // カードID -> 枚数 の管理
    std::map<int, int> editingDeck_;
    int totalCount_ = 0;

    CardDatabase* cardDB_ = nullptr;

    // --- 3D表示用 ---
    std::vector<std::unique_ptr<Card3D>> cardModels_;

    // レイアウト計算用の定数
    const float kCardStartX = -3.0f;// カードの最初の座標
    const float kCardStartY = 1.5; // カードの最初の座標
    const float kCardSpacingX = 1.5f;// カードの間の横幅
    const float kCardSpacingY = 2.f; // カードの間の立幅
    const int kCardsPerRow = 4;

    void RebuildCardModels(GameApp& app); // モデルを再生成する関数
    // 内部計算用
    void RecalculateTotal();

    // どのインデックスがクリックされたか
    int PickCardIndex(GameApp& app);

    float scrollY_ = 0.0f;
};