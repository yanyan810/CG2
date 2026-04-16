#pragma once
#include "../IPauseState.h"

class GameApp;

class SelectionState : public IPauseState {
public:

	void Initialize(GameApp& app) override;

	void Update(PausingUI* context, GameApp& app, Input* input) override;

	void Draw(GameApp& app) override;

private:

	std::unique_ptr<Sprite > baseSprite_;
};