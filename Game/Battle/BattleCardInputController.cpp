#include "BattleCardInputController.h"

#include "Card/CardDef.h"
#include "UI/HandView3D.h"

#include <algorithm>
#include <cmath>

namespace {
    float EffectValueFloat(const CardEffectDef& effect)
    {
        if (effect.valueIsFloat) {
            return effect.valueFloat;
        }
        if (effect.valueFloat != 0.0f || effect.value == 0) {
            return effect.valueFloat;
        }
        return static_cast<float>(effect.value);
    }

    int EffectValueInt(const CardEffectDef& effect)
    {
        return std::max(0, static_cast<int>(std::lround(EffectValueFloat(effect))));
    }

    int ScaleEffectAmount(int baseValue, const CardEffectDef& effect)
    {
        return std::max(0, static_cast<int>(std::lround(static_cast<float>(baseValue) * EffectValueFloat(effect))));
    }
}

int BattleCardInputController::PickHandIndexByMouse(
    const HandView3D& handView,
    const Matrix4x4& viewProjection,
    int mouseX,
    int mouseY,
    float screenWidth,
    float screenHeight)
{
    return handView.PickIndexByMouse(mouseX, mouseY, viewProjection, screenWidth, screenHeight);
}

BattleCardInputController::HandInputDecision BattleCardInputController::ResolveIdle(
    const CardInputSnapshot& input,
    int hoverIndex)
{
    HandInputDecision decision{};
    if (input.leftTriggered && hoverIndex >= 0) {
        decision.action = HandInputAction::StartDrag;
        decision.handIndex = hoverIndex;
    }
    return decision;
}

BattleCardInputController::HandInputDecision BattleCardInputController::ResolveDragging(
    const CardInputSnapshot& input,
    int selectedIndex,
    int dragStartMouseX,
    int dragStartMouseY,
    float previewThreshold)
{
    HandInputDecision decision{};
    decision.handIndex = selectedIndex;
    decision.dragDx = static_cast<float>(input.mouseX - dragStartMouseX);
    decision.dragDy = static_cast<float>(input.mouseY - dragStartMouseY);

    if (!input.leftReleased) {
        return decision;
    }

    decision.action = (decision.dragDy <= -previewThreshold)
        ? HandInputAction::OpenPreview
        : HandInputAction::ReturnToIdle;
    return decision;
}

BattleCardInputController::HandInputDecision BattleCardInputController::ResolvePreview(
    const CardInputSnapshot& input,
    int selectedIndex)
{
    HandInputDecision decision{};
    decision.handIndex = selectedIndex;
    if (input.rightTriggered) {
        decision.action = HandInputAction::CancelPreview;
    }
    return decision;
}
BattleCardInputController::FieldReplaceDecision BattleCardInputController::ResolveFieldReplaceInput(
    const CardInputSnapshot& input,
    int hoverIndex)
{
    FieldReplaceDecision decision{};
    decision.hoverIndex = hoverIndex;
    decision.replaceRequested = input.leftTriggered;
    decision.cancelRequested = input.rightTriggered;

    if (decision.cancelRequested) {
        decision.action = FieldReplaceAction::Cancel;
    } else if (decision.replaceRequested) {
        decision.action = FieldReplaceAction::Replace;
    }
    return decision;
}
BattleCardInputController::TargetDecision BattleCardInputController::ResolveEnemyTargetInput(
    int hoverIndex,
    bool leftTriggered,
    bool rightTriggered,
    bool cancelLocked)
{
    TargetDecision decision{};
    decision.targetIndex = hoverIndex;
    decision.confirmRequested = leftTriggered;
    decision.cancelRequested = rightTriggered;

    if (rightTriggered && cancelLocked) {
        decision.action = TargetAction::LockedCancel;
    } else if (rightTriggered) {
        decision.action = TargetAction::Cancel;
    } else if (leftTriggered) {
        decision.action = TargetAction::Confirm;
    }
    return decision;
}

BattleCardInputController::TargetRequirement BattleCardInputController::AnalyzeTargetRequirement(
    const CardDef& def,
    int playerBlock,
    int attackBuff,
    int playerTurnCount)
{
    TargetRequirement requirement{};
    int baseDamage = 0;
    int hitCount = 0;

    for (const auto& effect : def.effects) {
        if (effect.type == "Damage") {
            requirement.needsTarget = true;
            baseDamage += EffectValueInt(effect);
            hitCount++;
        } else if (effect.type == "DamageCrescent") {
            requirement.needsTarget = true;
            int value = EffectValueInt(effect);
            if (playerTurnCount % 2 != 0) {
                value += 3;
            }
            baseDamage += value;
            hitCount++;
        } else if (effect.type == "DamageByBlock") {
            requirement.needsTarget = true;
            baseDamage += ScaleEffectAmount(playerBlock, effect);
            hitCount++;
        } else if (effect.type == "Poison" || effect.type == "Frost") {
            requirement.needsTarget = true;
            hitCount++;
        } else if (effect.type == "FrostDraw") {
            requirement.needsTarget = true;
        }
    }

    requirement.pendingDamage = baseDamage + (attackBuff * hitCount);
    return requirement;
}
