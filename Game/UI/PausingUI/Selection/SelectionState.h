#pragma once
#include "../IPauseState.h"

#include "../../../Logic/DebugButton.h"

class GameApp;

class SelectionState : public IPauseState {
public:

	void Initialize(GameApp& app) override;

	void Update(PausingUI* context, GameApp& app, Input* input) override;

	void Draw(GameApp& app) override;

private:

	std::unique_ptr<TextSprite> title_;

	std::vector<std::unique_ptr<DebugButton>> buttons_;

};