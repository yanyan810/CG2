#pragma once
#include <vector>
#include <memory>
#include <string>
#include "Sprite.h"

class PausingUI;
class GameApp;
class Input;

class IPauseState {
public:
    virtual ~IPauseState() = default;
    virtual void Initialize(GameApp& app) = 0;
    virtual void Update(PausingUI* context, GameApp& app, Input* input) = 0;
    virtual void Draw(GameApp& app) = 0;
    virtual void DrawImGui() {}

protected:
    // この状態が管理するスプライトリスト
    std::vector<std::unique_ptr<Sprite>> sprites_;

    // 名前からスプライトを探して判定する共通関数
    Sprite* CheckMouseOverByName(Vector2 mousePos) {
        for (auto& sprite : sprites_) {
            if (sprite->IsMouseOver(mousePos)) {
                return sprite.get();
            }
        }
        return nullptr;
    }
};