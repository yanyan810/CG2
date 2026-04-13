#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "Object3d.h"
#include "CameraAnimator.h"
#include "AnimationClipDocument.h"
#include "AnimationEditorPose.h"
#include "ImGuizmo.h"

class AnimationEditorSession {
public:
    struct EditorContext {
        Object3d* animationTarget = nullptr;
        Camera* editorCamera = nullptr;
        Camera* cameraTarget = nullptr;
        CameraAnimator* cameraAnimator = nullptr;
        bool canEditAnimation = false;
        bool canEditCamera = false;
        bool editCameraMode = false;
        std::function<void()> switchToAnimation;
        std::function<void()> switchToCamera;
        const std::vector<std::string>* cameraFiles = nullptr;
        int currentCameraIndex = -1;
        bool* randomCameraEnabled = nullptr;
        bool* sameCameraLoopEnabled = nullptr;
        std::function<void()> reloadCameraFiles;
        std::function<void(int)> loadCameraByIndex;
        std::function<void()> playRandomCamera;
    };

    struct WindowVisibility {
        bool toolbar = true;
        bool hierarchy = true;
        bool inspector = true;
        bool timeline = true;
        bool preview = true;
    };

    void DrawImGui(const EditorContext& context);
    void DrawImGui(Object3d* target, Camera* editorCamera);
    WindowVisibility& GetWindowVisibility() { return windowVisibility_; }
    const WindowVisibility& GetWindowVisibility() const { return windowVisibility_; }

private:
    struct Ray {
        Vector3 origin;
        Vector3 direction;
    };

    struct Rect {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    struct LayoutRects {
        Rect toolbar;
        Rect hierarchy;
        Rect inspector;
        Rect timeline;
        Rect preview;
    };

    struct JointSelectionState {
        std::vector<int32_t> selectedIndices;
        int32_t activeIndex = -1;
    };

    struct AnimationHistoryEntry {
        AnimationEditorPose pose;
        AnimationClipDocument clipDocument;
        bool isDirty = false;
        float editorTime = 0.0f;
        float editorMaxDuration = 2.0f;
        int32_t selectedJointIndex = -1;
    };

    struct CameraHistoryEntry {
        CameraAnimator::StateSnapshot state;
    };

    struct AnimationClipboard {
        struct PoseEntry {
            Vector3 translate{ 0.0f, 0.0f, 0.0f };
            Quaternion rotate{ 0.0f, 0.0f, 0.0f, 1.0f };
            Vector3 scale{ 1.0f, 1.0f, 1.0f };
        };

        bool hasValue = false;
        std::vector<PoseEntry> poses;
    };

    struct CameraClipboard {
        bool hasValue = false;
        Vector3 position{ 0.0f, 0.0f, 0.0f };
        Vector3 rotation{ 0.0f, 0.0f, 0.0f };
        float fov = 0.45f;
    };

private:
    LayoutRects ComputeLayout_() const;

    void EnsureEditorStateInitialized_(Object3d& target, Model::Skeleton& skeleton);
    void ResetAnimationHistory_();
    void ResetCameraHistory_();
    void EnsureAnimationHistoryTarget_(Object3d* target);
    void EnsureCameraHistoryTarget_(Camera* target, CameraAnimator* animator);
    void SetAnimationDirty_(bool value);

    void DrawToolbarWindow_(Object3d& target, Model::Skeleton& skeleton, const LayoutRects& layout);
    void DrawHierarchyWindow_(Model::Skeleton& skeleton, const LayoutRects& layout, const EditorContext& context);
    void DrawInspectorWindow_(Object3d& target, Model::Skeleton& skeleton, const LayoutRects& layout);
    void DrawTimelineWindow_(Object3d& target, Model::Skeleton& skeleton, const LayoutRects& layout);
    void DrawPreviewOverlay_(Model::Skeleton& skeleton, const LayoutRects& layout);
    void DrawCameraToolbarWindow_(Camera& target, CameraAnimator& animator, const LayoutRects& layout);
    void DrawCameraHierarchyWindow_(const LayoutRects& layout, const EditorContext& context);
    void DrawCameraInspectorWindow_(Camera& target, CameraAnimator& animator, const LayoutRects& layout);
    void DrawCameraTimelineWindow_(Camera& target, CameraAnimator& animator, const LayoutRects& layout);
    void DrawCameraPreviewOverlay_(const LayoutRects& layout);

    void HandleViewportEditing_(Object3d& target, Model::Skeleton& skeleton, const LayoutRects& layout, Camera* editorCamera);

    void ApplyCurrentPoseToTarget_(Object3d& target, const Model::Skeleton& skeleton);
    void SampleClipAtCurrentTime_(Object3d& target, const Model::Skeleton& skeleton);
    AnimationHistoryEntry CaptureAnimationHistory_() const;
    void RestoreAnimationHistory_(const AnimationHistoryEntry& entry, Object3d& target, const Model::Skeleton& skeleton);
    void PushAnimationUndo_(const AnimationHistoryEntry& entry);
    bool CanUndoAnimation_() const;
    bool CanRedoAnimation_() const;
    void UndoAnimation_(Object3d& target, const Model::Skeleton& skeleton);
    void RedoAnimation_(Object3d& target, const Model::Skeleton& skeleton);

    CameraHistoryEntry CaptureCameraHistory_(CameraAnimator& animator) const;
    void RestoreCameraHistory_(const CameraHistoryEntry& entry, Camera& target, CameraAnimator& animator);
    void PushCameraUndo_(const CameraHistoryEntry& entry);
    bool CanUndoCamera_() const;
    bool CanRedoCamera_() const;
    void UndoCamera_(Camera& target, CameraAnimator& animator);
    void RedoCamera_(Camera& target, CameraAnimator& animator);
    void CopyAnimationSelection_(const Model::Skeleton& skeleton);
    void PasteAnimationSelection_(Object3d& target, const Model::Skeleton& skeleton);
    void CopyCameraState_(Camera& target, CameraAnimator& animator);
    void PasteCameraState_(Camera& target, CameraAnimator& animator);

    void RecordCurrentPoseAsKeyframe_(const Model::Skeleton& skeleton);
    void DeleteCurrentTimeKeyframe_();

    void ExportCurrentClip_(const Model::Skeleton& skeleton);
    void LoadClipToEdit_(Object3d& target, const Model::Skeleton& skeleton);
    void LoadClipAndPlay_(Object3d& target);
    void StopTestPlay_(Object3d& target, const Model::Skeleton& skeleton);

    static Vector3 QuaternionToEulerDeg_(const Quaternion& q);
    static Quaternion EulerDegToQuaternion_(const Vector3& rotDeg);

    static Ray ComputePickingRay_(
        float mouseX,
        float mouseY,
        float screenW,
        float screenH,
        const Matrix4x4& viewMat,
        const Matrix4x4& projMat);

    static bool RaySphereIntersect_(
        const Ray& ray,
        const Vector3& center,
        float radius,
        float& outDist);

    static bool IsPointInsideRect_(float px, float py, const Rect& rect);
    static bool IsCameraContext_(const EditorContext& context);
    void DrawEditorNavigationSection_(const EditorContext& context);

private:
    static constexpr size_t kMaxHistoryEntries_ = 64;

    int32_t selectedJointIndex_ = -1;
    JointSelectionState jointSelectionState_;

    ImGuizmo::OPERATION currentGizmoOperation_ = ImGuizmo::ROTATE;
    ImGuizmo::MODE currentGizmoMode_ = ImGuizmo::LOCAL;

    float editorTime_ = 0.0f;
    float editorMaxDuration_ = 2.0f;

    AnimationClipDocument clipDocument_;
    AnimationEditorPose pose_;
    WindowVisibility windowVisibility_;
    std::vector<AnimationHistoryEntry> animationUndoStack_;
    std::vector<AnimationHistoryEntry> animationRedoStack_;
    std::optional<AnimationHistoryEntry> pendingAnimationInspectorHistory_;
    std::optional<AnimationHistoryEntry> pendingAnimationGizmoHistory_;
    AnimationClipboard animationClipboard_;
    std::vector<CameraHistoryEntry> cameraUndoStack_;
    std::vector<CameraHistoryEntry> cameraRedoStack_;
    std::optional<CameraHistoryEntry> pendingCameraInspectorHistory_;
    CameraClipboard cameraClipboard_;
    bool wasUsingGizmo_ = false;
    bool animationDirty_ = false;
    Object3d* historyAnimationTarget_ = nullptr;
    Camera* historyCameraTarget_ = nullptr;
    CameraAnimator* historyCameraAnimator_ = nullptr;

    bool isTestingPlay_ = false;
    char exportFileName_[256] = "Resources/CustomAnim/CustomAnim.json";
};
