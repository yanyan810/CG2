#pragma once

#include <cstdint>

#include "Object3d.h"
#include "AnimationClipDocument.h"
#include "AnimationEditorPose.h"
#include "ImGuizmo.h"

class AnimationEditorSession {
public:
    void DrawImGui(Object3d* target, Camera* editorCamera);

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

private:
    LayoutRects ComputeLayout_() const;

    void EnsureEditorStateInitialized_(Object3d& target, Model::Skeleton& skeleton);

    void DrawToolbarWindow_(Object3d& target, Model::Skeleton& skeleton, const LayoutRects& layout);
    void DrawHierarchyWindow_(Model::Skeleton& skeleton, const LayoutRects& layout);
    void DrawInspectorWindow_(Object3d& target, Model::Skeleton& skeleton, const LayoutRects& layout);
    void DrawTimelineWindow_(Object3d& target, Model::Skeleton& skeleton, const LayoutRects& layout);
    void DrawPreviewOverlay_(Model::Skeleton& skeleton, const LayoutRects& layout);

    void HandleViewportEditing_(Object3d& target, Model::Skeleton& skeleton, const LayoutRects& layout, Camera* editorCamera);

    void ApplyCurrentPoseToTarget_(Object3d& target, const Model::Skeleton& skeleton);
    void SampleClipAtCurrentTime_(Object3d& target, const Model::Skeleton& skeleton);

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

private:
    int32_t selectedJointIndex_ = -1;

    ImGuizmo::OPERATION currentGizmoOperation_ = ImGuizmo::ROTATE;
    ImGuizmo::MODE currentGizmoMode_ = ImGuizmo::LOCAL;

    float editorTime_ = 0.0f;
    float editorMaxDuration_ = 2.0f;

    AnimationClipDocument clipDocument_;
    AnimationEditorPose pose_;

    bool isTestingPlay_ = false;
    char exportFileName_[256] = "Resources/CustomAnim/CustomAnim.json";
};
