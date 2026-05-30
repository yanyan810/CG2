#pragma once
#include "../IPauseState.h"

#include "../../../Logic/DebugButton.h"
#include "../../../Logic/Button.h"

#include "../../../Logic/StatusMenu.h"

class GameApp;

class SelectionState : public IPauseState {
public:
	SelectionState(bool isTutorialMode = false) : isTutorialMode_(isTutorialMode) {}

	void Initialize(GameApp& app) override;

	void Update(PausingUI* context, GameApp& app, Input* input) override;

	void Draw(GameApp& app) override;

private:
	bool isTutorialMode_ = false;

	std::unique_ptr<TextSprite> title_;

	std::vector<std::unique_ptr<Button>> buttons_;

	// 状態異常に関する情報
	StatusMenu statusMenu_;

};