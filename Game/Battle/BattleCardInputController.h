#pragma once

class HandView3D;
class Matrix4x4;

class BattleCardInputController {
public:
    struct CardInputSnapshot {
        int mouseX = 0;
        int mouseY = 0;
        bool leftTriggered = false;
        bool leftReleased = false;
        bool rightTriggered = false;
        bool leftPressed = false;
    };

    enum class HandInputAction {
        None = 0,
        StartDrag,
        OpenPreview,
        CancelPreview,
        ReturnToIdle,
    };

    struct HandInputDecision {
        HandInputAction action = HandInputAction::None;
        int handIndex = -1;
        float dragDx = 0.0f;
        float dragDy = 0.0f;
    };

    enum class FieldReplaceAction {
        None = 0,
        Replace,
        Cancel,
    };

    struct FieldReplaceDecision {
        FieldReplaceAction action = FieldReplaceAction::None;
        int hoverIndex = -1;
        bool replaceRequested = false;
        bool cancelRequested = false;
    };

    enum class TargetAction {
        None = 0,
        Confirm,
        Cancel,
        LockedCancel,
    };

    struct TargetDecision {
        TargetAction action = TargetAction::None;
        int targetIndex = -1;
        bool confirmRequested = false;
        bool cancelRequested = false;
    };

    static int PickHandIndexByMouse(
        const HandView3D& handView,
        const Matrix4x4& viewProjection,
        int mouseX,
        int mouseY,
        float screenWidth,
        float screenHeight);

    static HandInputDecision ResolveIdle(const CardInputSnapshot& input, int hoverIndex);
    static HandInputDecision ResolveDragging(
        const CardInputSnapshot& input,
        int selectedIndex,
        int dragStartMouseX,
        int dragStartMouseY,
        float previewThreshold);
    static HandInputDecision ResolvePreview(const CardInputSnapshot& input, int selectedIndex);
    static FieldReplaceDecision ResolveFieldReplaceInput(const CardInputSnapshot& input, int hoverIndex);
    static TargetDecision ResolveEnemyTargetInput(
        int hoverIndex,
        bool leftTriggered,
        bool rightTriggered,
        bool cancelLocked);
};