#pragma once
#include "../IPauseState.h"

#include "../../../Logic/DebugButton.h"
#include "../../../Logic/Button.h"
#include "../../UiLayout.h"

#include "../../../Logic/StatusMenu.h"

class GameApp;

class SelectionState : public IPauseState {
public:
	SelectionState(bool isTutorialMode = false) : isTutorialMode_(isTutorialMode) {}

	void Initialize(GameApp& app) override;

	void Update(PausingUI* context, GameApp& app, Input* input) override;

	void Draw(GameApp& app) override;
	void DrawImGui() override;

private:
	void SaveLayout_() const;
	void LoadLayout_();
	void ApplyLayout_();

	SelectionUiLayout layout_{};
	std::string layoutPath_ = "resources/ui/selection_ui_layout.json";
	bool isTutorialMode_ = false;

	std::unique_ptr<TextSprite> title_;

	std::unique_ptr<Button> resumeBtn_;
	std::unique_ptr<Button> helpBtn_;
	std::unique_ptr<Button> giveUpBtn_;

	// 状態異常に関する情報
	StatusMenu statusMenu_;

};