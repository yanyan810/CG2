#pragma once
#include "../IPauseState.h"
#include "TextSprite.h"

class GiveUpConfirmState : public IPauseState {
public:
    GiveUpConfirmState() = default;
    ~GiveUpConfirmState() override = default;

    void Initialize(GameApp& app) override;
    void Update(PausingUI* context, GameApp& app, Input* input) override;
    void Draw(GameApp& app) override;

private:
    // 背景画像（「本当に降参しますか？」などのテキストが含まれるテクスチャを想定）
    std::unique_ptr<Sprite> confirmBoard_;
    std::unique_ptr<TextSprite> titleText_;
    std::unique_ptr<TextSprite> yesText_;
    std::unique_ptr<TextSprite> noText_;

	bool pushedYes_ = false;
};
