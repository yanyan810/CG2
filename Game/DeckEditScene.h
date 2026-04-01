#pragma once
#include "IScene.h"
#include <vector>
#include <string>
#include <map>

#include "CardDatabase.h"

class DeckEditScene : public IScene {
public:
    void OnEnter(GameApp& app) override;
    void OnExit(GameApp& app) override;
    void Update(GameApp& app, float dt) override;
    void Draw3D(GameApp& app) override {}
    void Draw2D(GameApp& app) override;
    void DrawImGui(GameApp& app) override;

private:
    // カードID -> 枚数 の管理
    std::map<int, int> editingDeck_;
    int totalCount_ = 0;

    CardDatabase db_;

    // 内部計算用
    void RecalculateTotal();
};