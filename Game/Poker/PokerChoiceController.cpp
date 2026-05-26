#include "PokerChoiceController.h"

#include "UI/UiLayout.h"

namespace {
	bool PointInRect_(int mx, int my, float x, float y, float w, float h)
	{
		return mx >= x && mx <= x + w &&
			my >= y && my <= y + h;
	}
}

PokerChoiceController::HoverResult PokerChoiceController::ResolveActivateHover(
	const PokerEffectChoiceLayout& layout,
	int mouseX,
	int mouseY,
	bool tutorialActivateOnly)
{
	if (PointInRect_(mouseX, mouseY,
		layout.infoButtonRect.x, layout.infoButtonRect.y,
		layout.infoButtonRect.w, layout.infoButtonRect.h)) {
		return { Choice::None, true };
	}

	if (tutorialActivateOnly) {
		return { Choice::ActivateYes, false };
	}

	if (PointInRect_(mouseX, mouseY,
		layout.activateYesRect.x, layout.activateYesRect.y,
		layout.activateYesRect.w, layout.activateYesRect.h)) {
		return { Choice::ActivateYes, false };
	}

	if (PointInRect_(mouseX, mouseY,
		layout.activateNoRect.x, layout.activateNoRect.y,
		layout.activateNoRect.w, layout.activateNoRect.h)) {
		return { Choice::ActivateNo, false };
	}

	if (PointInRect_(mouseX, mouseY,
		layout.activateViewBoardRect.x, layout.activateViewBoardRect.y,
		layout.activateViewBoardRect.w, layout.activateViewBoardRect.h)) {
		return { Choice::ActivateViewBoard, false };
	}

	return {};
}


PokerChoiceController::ActivateDecision PokerChoiceController::ResolveActivateChoice(
	const PokerEffectChoiceLayout& layout,
	int mouseX,
	int mouseY,
	bool leftTriggered,
	bool yesTriggered,
	bool noTriggered,
	bool tutorialActivateOnly)
{
	ActivateDecision result{};
	result.hover = ResolveActivateHover(layout, mouseX, mouseY, tutorialActivateOnly);

	if (result.hover.infoHovered && leftTriggered) {
		result.action = ActivateAction::ToggleInfo;
		return result;
	}

	if (yesTriggered || (leftTriggered && result.hover.choice == Choice::ActivateYes)) {
		result.action = ActivateAction::Activate;
		return result;
	}

	if (!tutorialActivateOnly &&
		(noTriggered || (leftTriggered && result.hover.choice == Choice::ActivateNo))) {
		result.action = ActivateAction::Skip;
		return result;
	}

	if (!tutorialActivateOnly &&
		leftTriggered && result.hover.choice == Choice::ActivateViewBoard) {
		result.action = ActivateAction::ViewBoard;
		return result;
	}

	return result;
}
PokerChoiceController::HoverResult PokerChoiceController::ResolveEffectHover(
	const PokerEffectChoiceLayout& layout,
	int mouseX,
	int mouseY,
	bool tutorialDamageOnly)
{
	if (PointInRect_(mouseX, mouseY,
		layout.infoButtonRect.x, layout.infoButtonRect.y,
		layout.infoButtonRect.w, layout.infoButtonRect.h)) {
		return { Choice::None, true };
	}

	if (tutorialDamageOnly) {
		return { Choice::EffectDamage, false };
	}

	if (PointInRect_(mouseX, mouseY,
		layout.backRect.x, layout.backRect.y,
		layout.backRect.w, layout.backRect.h)) {
		return { Choice::EffectBack, false };
	}

	if (PointInRect_(mouseX, mouseY,
		layout.effectRects[0].x, layout.effectRects[0].y,
		layout.effectRects[0].w, layout.effectRects[0].h)) {
		return { Choice::EffectAtkUp, false };
	}

	if (PointInRect_(mouseX, mouseY,
		layout.effectRects[1].x, layout.effectRects[1].y,
		layout.effectRects[1].w, layout.effectRects[1].h)) {
		return { Choice::EffectDamage, false };
	}

	if (PointInRect_(mouseX, mouseY,
		layout.effectRects[2].x, layout.effectRects[2].y,
		layout.effectRects[2].w, layout.effectRects[2].h)) {
		return { Choice::EffectDraw, false };
	}

	if (PointInRect_(mouseX, mouseY,
		layout.effectViewBoardRect.x, layout.effectViewBoardRect.y,
		layout.effectViewBoardRect.w, layout.effectViewBoardRect.h)) {
		return { Choice::EffectViewBoard, false };
	}

	return {};
}


PokerChoiceController::EffectDecision PokerChoiceController::ResolveEffectChoice(
	const PokerEffectChoiceLayout& layout,
	int mouseX,
	int mouseY,
	bool leftTriggered,
	bool noTriggered,
	bool tutorialDamageOnly)
{
	EffectDecision result{};
	result.hover = ResolveEffectHover(layout, mouseX, mouseY, tutorialDamageOnly);

	if (result.hover.infoHovered && leftTriggered) {
		result.action = EffectAction::ToggleInfo;
		return result;
	}

	if (tutorialDamageOnly) {
		if (leftTriggered && result.hover.choice == Choice::EffectDamage) {
			result.action = EffectAction::Damage;
		}
		return result;
	}

	if (leftTriggered && result.hover.choice == Choice::EffectAtkUp) {
		result.action = EffectAction::AtkUp;
		return result;
	}

	if (leftTriggered && result.hover.choice == Choice::EffectDraw) {
		result.action = EffectAction::Draw;
		return result;
	}

	if (leftTriggered && result.hover.choice == Choice::EffectDamage) {
		result.action = EffectAction::Damage;
		return result;
	}

	if (noTriggered || (leftTriggered && result.hover.choice == Choice::EffectBack)) {
		result.action = EffectAction::Back;
		return result;
	}

	if (leftTriggered && result.hover.choice == Choice::EffectViewBoard) {
		result.action = EffectAction::ViewBoard;
		return result;
	}

	return result;
}
PokerChoiceController::Choice PokerChoiceController::ResolveViewBoardHover(
	const PokerEffectChoiceLayout& layout,
	int mouseX,
	int mouseY)
{
	if (PointInRect_(mouseX, mouseY,
		layout.backRect.x, layout.backRect.y,
		layout.backRect.w, layout.backRect.h)) {
		return Choice::ReturnFromBoard;
	}

	return Choice::None;
}