#pragma once

struct PokerEffectChoiceLayout;

class PokerChoiceController {
public:
	enum class Choice {
		None = 0,
		ActivateYes,
		ActivateNo,
		ActivateViewBoard,
		EffectAtkUp,
		EffectDraw,
		EffectDamage,
		EffectBack,
		EffectViewBoard,
		ReturnFromBoard,
	};

	struct HoverResult {
		Choice choice = Choice::None;
		bool infoHovered = false;
	};

	enum class ActivateAction {
		None = 0,
		ToggleInfo,
		Activate,
		Skip,
		ViewBoard,
	};

	struct ActivateDecision {
		HoverResult hover{};
		ActivateAction action = ActivateAction::None;
	};

	static HoverResult ResolveActivateHover(
		const PokerEffectChoiceLayout& layout,
		int mouseX,
		int mouseY,
		bool tutorialActivateOnly);

	static ActivateDecision ResolveActivateChoice(
		const PokerEffectChoiceLayout& layout,
		int mouseX,
		int mouseY,
		bool leftTriggered,
		bool yesTriggered,
		bool noTriggered,
		bool tutorialActivateOnly);

	static HoverResult ResolveEffectHover(
		const PokerEffectChoiceLayout& layout,
		int mouseX,
		int mouseY,
		bool tutorialDamageOnly);

	static Choice ResolveViewBoardHover(
		const PokerEffectChoiceLayout& layout,
		int mouseX,
		int mouseY);
};