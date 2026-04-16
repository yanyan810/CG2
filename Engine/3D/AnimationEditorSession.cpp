#include "AnimationEditorSession.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

namespace {
    constexpr float kPi = 3.14159265f;
    constexpr float kUiPadding = 8.0f;
    constexpr float kToolbarHeight = 86.0f;
    constexpr float kTimelineHeight = 220.0f;
    constexpr float kPanelWidth = 320.0f;
    constexpr float kPreviewMinWidth = 320.0f;
    constexpr float kJointPickRadius = 0.30f;

    template <class TKeyframe>
    bool HasKeyAtTime(const std::vector<TKeyframe>& keyframes, float time) {
        for (const auto& k : keyframes) {
            if (std::abs(k.time - time) < 0.001f) {
                return true;
            }
        }
        return false;
    }

    template <class TEntry>
    void TrimHistoryStack(std::vector<TEntry>& stack, size_t maxEntries) {
        if (stack.size() > maxEntries) {
            stack.erase(stack.begin(), stack.begin() + (stack.size() - maxEntries));
        }
    }

    template <class TKeyframe, class TValue>
    void UpsertKeyAtTime(std::vector<TKeyframe>& keyframes, float time, const TValue& value) {
        auto it = std::find_if(
            keyframes.begin(),
            keyframes.end(),
            [time](const TKeyframe& key) {
                return std::abs(key.time - time) < 0.001f;
            });

        if (it != keyframes.end()) {
            it->value = value;
        } else {
            keyframes.push_back({ time, value });
            std::sort(
                keyframes.begin(),
                keyframes.end(),
                [](const TKeyframe& a, const TKeyframe& b) {
                    return a.time < b.time;
                });
        }
    }
}

namespace fs = std::filesystem;

Vector3 AnimationEditorSession::QuaternionToEulerDeg_(const Quaternion& q) {
    Vector3 rot{};

    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    rot.x = std::atan2(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (std::abs(sinp) >= 1.0f) {
        rot.y = std::copysign(kPi / 2.0f, sinp);
    } else {
        rot.y = std::asin(sinp);
    }

    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    rot.z = std::atan2(siny_cosp, cosy_cosp);

    const float rad2deg = 180.0f / kPi;
    rot.x *= rad2deg;
    rot.y *= rad2deg;
    rot.z *= rad2deg;
    return rot;
}

Quaternion AnimationEditorSession::EulerDegToQuaternion_(const Vector3& rotDeg) {
    const float radX = rotDeg.x * (kPi / 180.0f);
    const float radY = rotDeg.y * (kPi / 180.0f);
    const float radZ = rotDeg.z * (kPi / 180.0f);

    const float cx = std::cos(radX * 0.5f);
    const float sx = std::sin(radX * 0.5f);
    const float cy = std::cos(radY * 0.5f);
    const float sy = std::sin(radY * 0.5f);
    const float cz = std::cos(radZ * 0.5f);
    const float sz = std::sin(radZ * 0.5f);

    Quaternion q{};
    q.w = cx * cy * cz + sx * sy * sz;
    q.x = sx * cy * cz - cx * sy * sz;
    q.y = cx * sy * cz + sx * cy * sz;
    q.z = cx * cy * sz - sx * sy * cz;
    return Normalize(q);
}

AnimationEditorSession::Ray AnimationEditorSession::ComputePickingRay_(
    float mouseX,
    float mouseY,
    float screenW,
    float screenH,
    const Matrix4x4& viewMat,
    const Matrix4x4& projMat) {
    float x = (2.0f * mouseX) / screenW - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenH;

    Matrix4x4 viewProj = Matrix4x4::Multiply(viewMat, projMat);
    Matrix4x4 invViewProj = Matrix4x4::Inverse(viewProj);

    Vector4 clipStart{ x, y, 0.0f, 1.0f };
    Vector4 clipEnd{ x, y, 1.0f, 1.0f };

    Vector4 worldStart = MulRowVec4Mat4(clipStart, invViewProj);
    Vector4 worldEnd = MulRowVec4Mat4(clipEnd, invViewProj);

    worldStart.x /= worldStart.w;
    worldStart.y /= worldStart.w;
    worldStart.z /= worldStart.w;

    worldEnd.x /= worldEnd.w;
    worldEnd.y /= worldEnd.w;
    worldEnd.z /= worldEnd.w;

    Vector3 origin{ worldStart.x, worldStart.y, worldStart.z };
    Vector3 end{ worldEnd.x, worldEnd.y, worldEnd.z };

    Vector3 dir = end - origin;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len > 0.0f) {
        dir.x /= len;
        dir.y /= len;
        dir.z /= len;
    }

    return { origin, dir };
}

bool AnimationEditorSession::RaySphereIntersect_(
    const Ray& ray,
    const Vector3& center,
    float radius,
    float& outDist) {
    Vector3 oc = ray.origin - center;

    float a = ray.direction.x * ray.direction.x +
        ray.direction.y * ray.direction.y +
        ray.direction.z * ray.direction.z;

    float b = 2.0f * (
        oc.x * ray.direction.x +
        oc.y * ray.direction.y +
        oc.z * ray.direction.z);

    float c = (oc.x * oc.x + oc.y * oc.y + oc.z * oc.z) - radius * radius;

    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return false;
    }

    float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    if (t < 0.0f) {
        t = (-b + std::sqrt(discriminant)) / (2.0f * a);
    }
    if (t < 0.0f) {
        return false;
    }

    outDist = t;
    return true;
}

bool AnimationEditorSession::IsPointInsideRect_(float px, float py, const Rect& rect) {
    return px >= rect.x && px <= rect.x + rect.w &&
        py >= rect.y && py <= rect.y + rect.h;
}

bool AnimationEditorSession::IsCameraContext_(const EditorContext& context) {
    return context.animationTarget == nullptr &&
        context.cameraTarget != nullptr &&
        context.cameraAnimator != nullptr;
}

AnimationEditorSession::LayoutRects AnimationEditorSession::ComputeLayout_() const {
    LayoutRects layout{};

#ifdef USE_IMGUI
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    const float viewportX = viewport->Pos.x;
    const float viewportY = viewport->Pos.y;
    const float viewportW = viewport->Size.x;
    const float viewportH = viewport->Size.y;

    float sideWidth = kPanelWidth;
    const float maxSideWidth = std::max(260.0f, (viewportW - (kUiPadding * 4.0f) - kPreviewMinWidth) * 0.5f);
    sideWidth = std::min(sideWidth, maxSideWidth);

    const float contentTop = viewportY + kUiPadding;
    const float contentLeft = viewportX + kUiPadding;
    const float contentRight = viewportX + viewportW - kUiPadding;
    const float contentBottom = viewportY + viewportH - kUiPadding;

    layout.toolbar = {
        contentLeft,
        contentTop,
        viewportW - kUiPadding * 2.0f,
        kToolbarHeight
    };

    const float panelsTop = layout.toolbar.y + layout.toolbar.h + kUiPadding;
    const float panelsBottom = contentBottom - kTimelineHeight - kUiPadding;
    const float panelsHeight = std::max(160.0f, panelsBottom - panelsTop);

    layout.hierarchy = {
        contentLeft,
        panelsTop,
        sideWidth,
        panelsHeight
    };

    layout.inspector = {
        contentRight - sideWidth,
        panelsTop,
        sideWidth,
        panelsHeight
    };

    layout.timeline = {
        contentLeft,
        contentBottom - kTimelineHeight,
        viewportW - kUiPadding * 2.0f,
        kTimelineHeight
    };

    layout.preview = {
        layout.hierarchy.x + layout.hierarchy.w + kUiPadding,
        panelsTop,
        std::max(120.0f, layout.inspector.x - (layout.hierarchy.x + layout.hierarchy.w + kUiPadding * 2.0f)),
        panelsHeight
    };
#endif

    return layout;
}

void AnimationEditorSession::DrawImGui(const EditorContext& context) {
#ifdef USE_IMGUI
    OutputDebugStringA("[AnimEditor] Session DrawImGui\n");

    enum class ShortcutCommand {
        None,
        Undo,
        Redo,
        Save,
        Copy,
        Paste,
        SelectAll,
    };

    auto pollShortcutCommand = []() -> ShortcutCommand {
        const ImGuiIO& io = ImGui::GetIO();
        if (io.WantTextInput || !io.KeyCtrl) {
            return ShortcutCommand::None;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            return ShortcutCommand::Save;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_C, false)) {
            return ShortcutCommand::Copy;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
            return ShortcutCommand::Paste;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_A, false)) {
            return ShortcutCommand::SelectAll;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
            return ShortcutCommand::Redo;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
            return io.KeyShift ? ShortcutCommand::Redo : ShortcutCommand::Undo;
        }

        return ShortcutCommand::None;
    };

    if (IsCameraContext_(context)) {
        Camera* cameraTarget = context.cameraTarget;
        CameraAnimator* cameraAnimator = context.cameraAnimator;

        if (!cameraTarget || !cameraAnimator) {
            return;
        }

        EnsureCameraHistoryTarget_(cameraTarget, cameraAnimator);

        const LayoutRects layout = ComputeLayout_();
        if (windowVisibility_.toolbar) {
            DrawCameraToolbarWindow_(*cameraTarget, *cameraAnimator, layout);
        }
        if (windowVisibility_.hierarchy) {
            DrawCameraHierarchyWindow_(layout, context);
        }
        if (windowVisibility_.inspector) {
            DrawCameraInspectorWindow_(*cameraTarget, *cameraAnimator, layout);
        }
        if (windowVisibility_.timeline) {
            DrawCameraTimelineWindow_(*cameraTarget, *cameraAnimator, layout);
        }
        if (windowVisibility_.preview) {
            DrawCameraPreviewOverlay_(layout);
        }

        const ShortcutCommand shortcutCommand = pollShortcutCommand();
        if (shortcutCommand == ShortcutCommand::Undo) {
            UndoCamera_(*cameraTarget, *cameraAnimator);
        } else if (shortcutCommand == ShortcutCommand::Redo) {
            RedoCamera_(*cameraTarget, *cameraAnimator);
        } else if (shortcutCommand == ShortcutCommand::Save) {
            cameraAnimator->SaveToJson(cameraAnimator->GetSaveFilepath());
        } else if (shortcutCommand == ShortcutCommand::Copy) {
            CopyCameraState_(*cameraTarget, *cameraAnimator);
        } else if (shortcutCommand == ShortcutCommand::Paste) {
            PasteCameraState_(*cameraTarget, *cameraAnimator);
        }
        return;
    }

    Object3d* target = context.animationTarget;
    Camera* editorCamera = context.editorCamera ? context.editorCamera : context.cameraTarget;

    if (!target) {
        OutputDebugStringA("[AnimEditor] target is null\n");
        return;
    }

    EnsureAnimationHistoryTarget_(target);

    Model::Skeleton* skeleton = target->GetSkeleton();
    if (!skeleton) {
        OutputDebugStringA("[AnimEditor] skeleton is null\n");
        return;
    }

    if (skeleton->joints.empty()) {
        OutputDebugStringA("[AnimEditor] skeleton joints empty\n");
        return;
    }

    if (selectedJointIndex_ >= static_cast<int32_t>(skeleton->joints.size())) {
        selectedJointIndex_ = -1;
    }

    EnsureEditorStateInitialized_(*target, *skeleton);

    const LayoutRects layout = ComputeLayout_();

    if (windowVisibility_.toolbar) {
        DrawToolbarWindow_(*target, *skeleton, layout);
    }
    if (windowVisibility_.hierarchy) {
        DrawHierarchyWindow_(*skeleton, layout, context);
    }
    if (windowVisibility_.inspector) {
        DrawInspectorWindow_(*target, *skeleton, layout);
    }
    if (windowVisibility_.timeline) {
        DrawTimelineWindow_(*target, *skeleton, layout);
    }
    if (windowVisibility_.preview) {
        DrawPreviewOverlay_(*skeleton, layout);
    }

    const ShortcutCommand shortcutCommand = pollShortcutCommand();
    if (shortcutCommand == ShortcutCommand::Undo) {
        UndoAnimation_(*target, *skeleton);
    } else if (shortcutCommand == ShortcutCommand::Redo) {
        RedoAnimation_(*target, *skeleton);
    } else if (shortcutCommand == ShortcutCommand::Save) {
        const Model::Skeleton& exportSkeleton = target->GetModel() ? target->GetModel()->GetSkeleton() : *skeleton;
        ExportCurrentClip_(exportSkeleton);
    } else if (shortcutCommand == ShortcutCommand::Copy) {
        CopyAnimationSelection_(*skeleton);
    } else if (shortcutCommand == ShortcutCommand::Paste) {
        PasteAnimationSelection_(*target, *skeleton);
    } else if (shortcutCommand == ShortcutCommand::SelectAll) {
        jointSelectionState_.selectedIndices.clear();
        jointSelectionState_.selectedIndices.reserve(skeleton->joints.size());
        for (int32_t i = 0; i < static_cast<int32_t>(skeleton->joints.size()); ++i) {
            jointSelectionState_.selectedIndices.push_back(i);
        }

        if (jointSelectionState_.activeIndex < 0 && !jointSelectionState_.selectedIndices.empty()) {
            jointSelectionState_.activeIndex = jointSelectionState_.selectedIndices.front();
        }
        selectedJointIndex_ = jointSelectionState_.activeIndex;
    }

    HandleViewportEditing_(*target, *skeleton, layout, editorCamera);
#else
  //  (void)target;
    //(void)editorCamera;
#endif
}

void AnimationEditorSession::DrawImGui(Object3d* target, Camera* editorCamera) {
    EditorContext context{};
    context.animationTarget = target;
    context.editorCamera = editorCamera;
    context.cameraTarget = editorCamera;
    context.cameraAnimator = nullptr;
    DrawImGui(context);
}

#ifdef USE_IMGUI

void AnimationEditorSession::ResetAnimationHistory_() {
    animationUndoStack_.clear();
    animationRedoStack_.clear();
    pendingAnimationInspectorHistory_.reset();
    pendingAnimationGizmoHistory_.reset();
    wasUsingGizmo_ = false;
    animationDirty_ = false;
}

void AnimationEditorSession::ResetCameraHistory_() {
    cameraUndoStack_.clear();
    cameraRedoStack_.clear();
    pendingCameraInspectorHistory_.reset();
}

void AnimationEditorSession::EnsureAnimationHistoryTarget_(Object3d* target) {
    if (historyAnimationTarget_ != target) {
        historyAnimationTarget_ = target;
        ResetAnimationHistory_();
    }
}

void AnimationEditorSession::EnsureCameraHistoryTarget_(Camera* target, CameraAnimator* animator) {
    if (historyCameraTarget_ != target || historyCameraAnimator_ != animator) {
        historyCameraTarget_ = target;
        historyCameraAnimator_ = animator;
        ResetCameraHistory_();
    }
}

void AnimationEditorSession::SetAnimationDirty_(bool value) {
    animationDirty_ = value;
}

AnimationEditorSession::AnimationHistoryEntry AnimationEditorSession::CaptureAnimationHistory_() const {
    AnimationHistoryEntry entry{};
    entry.pose = pose_;
    entry.clipDocument = clipDocument_;
    entry.isDirty = animationDirty_;
    entry.editorTime = editorTime_;
    entry.editorMaxDuration = editorMaxDuration_;
    entry.selectedJointIndex = selectedJointIndex_;
    return entry;
}

void AnimationEditorSession::RestoreAnimationHistory_(
    const AnimationHistoryEntry& entry,
    Object3d& target,
    const Model::Skeleton& skeleton) {
    if (isTestingPlay_) {
        target.StopAnimation();
        isTestingPlay_ = false;
    }

    pose_ = entry.pose;
    clipDocument_ = entry.clipDocument;
    animationDirty_ = entry.isDirty;
    editorTime_ = entry.editorTime;
    editorMaxDuration_ = entry.editorMaxDuration;
    selectedJointIndex_ = entry.selectedJointIndex;
    ApplyCurrentPoseToTarget_(target, skeleton);
}

void AnimationEditorSession::PushAnimationUndo_(const AnimationHistoryEntry& entry) {
    animationUndoStack_.push_back(entry);
    TrimHistoryStack(animationUndoStack_, kMaxHistoryEntries_);
    animationRedoStack_.clear();
}

bool AnimationEditorSession::CanUndoAnimation_() const {
    return !animationUndoStack_.empty();
}

bool AnimationEditorSession::CanRedoAnimation_() const {
    return !animationRedoStack_.empty();
}

void AnimationEditorSession::UndoAnimation_(Object3d& target, const Model::Skeleton& skeleton) {
    if (animationUndoStack_.empty()) {
        return;
    }

    animationRedoStack_.push_back(CaptureAnimationHistory_());
    TrimHistoryStack(animationRedoStack_, kMaxHistoryEntries_);

    AnimationHistoryEntry entry = animationUndoStack_.back();
    animationUndoStack_.pop_back();
    RestoreAnimationHistory_(entry, target, skeleton);
}

void AnimationEditorSession::RedoAnimation_(Object3d& target, const Model::Skeleton& skeleton) {
    if (animationRedoStack_.empty()) {
        return;
    }

    animationUndoStack_.push_back(CaptureAnimationHistory_());
    TrimHistoryStack(animationUndoStack_, kMaxHistoryEntries_);

    AnimationHistoryEntry entry = animationRedoStack_.back();
    animationRedoStack_.pop_back();
    RestoreAnimationHistory_(entry, target, skeleton);
}

AnimationEditorSession::CameraHistoryEntry AnimationEditorSession::CaptureCameraHistory_(CameraAnimator& animator) const {
    CameraHistoryEntry entry{};
    entry.state = animator.CaptureState();
    return entry;
}

void AnimationEditorSession::RestoreCameraHistory_(
    const CameraHistoryEntry& entry,
    Camera& target,
    CameraAnimator& animator) {
    animator.RestoreState(entry.state);
    target.Update();
}

void AnimationEditorSession::PushCameraUndo_(const CameraHistoryEntry& entry) {
    cameraUndoStack_.push_back(entry);
    TrimHistoryStack(cameraUndoStack_, kMaxHistoryEntries_);
    cameraRedoStack_.clear();
}

bool AnimationEditorSession::CanUndoCamera_() const {
    return !cameraUndoStack_.empty();
}

bool AnimationEditorSession::CanRedoCamera_() const {
    return !cameraRedoStack_.empty();
}

void AnimationEditorSession::UndoCamera_(Camera& target, CameraAnimator& animator) {
    if (cameraUndoStack_.empty()) {
        return;
    }

    cameraRedoStack_.push_back(CaptureCameraHistory_(animator));
    TrimHistoryStack(cameraRedoStack_, kMaxHistoryEntries_);

    CameraHistoryEntry entry = cameraUndoStack_.back();
    cameraUndoStack_.pop_back();
    RestoreCameraHistory_(entry, target, animator);
}

void AnimationEditorSession::RedoCamera_(Camera& target, CameraAnimator& animator) {
    if (cameraRedoStack_.empty()) {
        return;
    }

    cameraUndoStack_.push_back(CaptureCameraHistory_(animator));
    TrimHistoryStack(cameraUndoStack_, kMaxHistoryEntries_);

    CameraHistoryEntry entry = cameraRedoStack_.back();
   cameraRedoStack_.pop_back();
    RestoreCameraHistory_(entry, target, animator);
}

void AnimationEditorSession::CopyAnimationSelection_(const Model::Skeleton& skeleton) {
    animationClipboard_.hasValue = false;
    animationClipboard_.poses.clear();

    std::vector<int32_t> sourceIndices = jointSelectionState_.selectedIndices;
    if (sourceIndices.empty()) {
        if (selectedJointIndex_ >= 0 &&
            selectedJointIndex_ < static_cast<int32_t>(skeleton.joints.size()) &&
            pose_.HasValidJoint(selectedJointIndex_)) {
            sourceIndices.push_back(selectedJointIndex_);
        }
    }

    if (sourceIndices.empty()) {
        return;
    }

    animationClipboard_.poses.reserve(sourceIndices.size());

    for (int32_t jointIndex : sourceIndices) {
        if (jointIndex < 0 ||
            jointIndex >= static_cast<int32_t>(skeleton.joints.size()) ||
            !pose_.HasValidJoint(jointIndex)) {
            continue;
        }

        const std::string& boneName = skeleton.joints[jointIndex].name;
        const auto& currentPose = pose_.GetJointPose(jointIndex);

        AnimationClipboard::PoseEntry entry{};
        entry.translate = currentPose.translate;
        entry.rotate = currentPose.rotate;
        entry.scale = currentPose.scale;

        auto animIt = clipDocument_.GetAnimation().nodeAnimations.find(boneName);
        if (animIt != clipDocument_.GetAnimation().nodeAnimations.end()) {
            const NodeAnimation& na = animIt->second;

            auto findTranslate = std::find_if(
                na.translate.keyframes.begin(),
                na.translate.keyframes.end(),
                [&](const KeyframeVector3& key) {
                    return std::abs(key.time - editorTime_) < 0.001f;
                });
            if (findTranslate != na.translate.keyframes.end()) {
                entry.translate = findTranslate->value;
            }

            auto findRotate = std::find_if(
                na.rotate.keyframes.begin(),
                na.rotate.keyframes.end(),
                [&](const KeyframeQuaternion& key) {
                    return std::abs(key.time - editorTime_) < 0.001f;
                });
            if (findRotate != na.rotate.keyframes.end()) {
                entry.rotate = findRotate->value;
            }

            auto findScale = std::find_if(
                na.scale.keyframes.begin(),
                na.scale.keyframes.end(),
                [&](const KeyframeVector3& key) {
                    return std::abs(key.time - editorTime_) < 0.001f;
                });
            if (findScale != na.scale.keyframes.end()) {
                entry.scale = findScale->value;
            }
        }

        animationClipboard_.poses.push_back(entry);
    }

    animationClipboard_.hasValue = !animationClipboard_.poses.empty();
}

void AnimationEditorSession::PasteAnimationSelection_(Object3d& target, const Model::Skeleton& skeleton) {
    if (!animationClipboard_.hasValue || animationClipboard_.poses.empty()) {
        return;
    }

    std::vector<int32_t> targetIndices = jointSelectionState_.selectedIndices;
    if (targetIndices.empty()) {
        if (selectedJointIndex_ >= 0 &&
            selectedJointIndex_ < static_cast<int32_t>(skeleton.joints.size()) &&
            pose_.HasValidJoint(selectedJointIndex_)) {
            targetIndices.push_back(selectedJointIndex_);
        }
    }

    if (targetIndices.empty()) {
        return;
    }

    PushAnimationUndo_(CaptureAnimationHistory_());

    const size_t applyCount = std::min(targetIndices.size(), animationClipboard_.poses.size());
    for (size_t i = 0; i < applyCount; ++i) {
        const int32_t jointIndex = targetIndices[i];
        if (jointIndex < 0 ||
            jointIndex >= static_cast<int32_t>(skeleton.joints.size()) ||
            !pose_.HasValidJoint(jointIndex)) {
            continue;
        }

        const auto& entry = animationClipboard_.poses[i];

        pose_.SetJointTranslate(jointIndex, entry.translate);
        pose_.SetJointRotateQuaternion(jointIndex, entry.rotate);
        pose_.SetJointScale(jointIndex, entry.scale);

        auto& nodeAnimation = clipDocument_.GetAnimation().nodeAnimations[skeleton.joints[jointIndex].name];
        UpsertKeyAtTime(nodeAnimation.translate.keyframes, editorTime_, entry.translate);
        UpsertKeyAtTime(nodeAnimation.rotate.keyframes, editorTime_, entry.rotate);
        UpsertKeyAtTime(nodeAnimation.scale.keyframes, editorTime_, entry.scale);
    }

    SetAnimationDirty_(true);
    SampleClipAtCurrentTime_(target, skeleton);
}

void AnimationEditorSession::CopyCameraState_(Camera& target, CameraAnimator& animator) {
    cameraClipboard_.hasValue = false;
    cameraClipboard_.position = target.GetTranslate();
    cameraClipboard_.rotation = target.GetRotate();
    cameraClipboard_.fov = target.GetFovY();

    for (const auto& key : animator.GetKeyframes()) {
        if (std::abs(key.time - animator.GetCurrentTime()) < 0.001f) {
            cameraClipboard_.position = key.pos;
            cameraClipboard_.rotation = key.rot;
            cameraClipboard_.fov = key.fov;
            break;
        }
    }

    cameraClipboard_.hasValue = true;
}

void AnimationEditorSession::PasteCameraState_(Camera& target, CameraAnimator& animator) {
    if (!cameraClipboard_.hasValue) {
        return;
    }

    PushCameraUndo_(CaptureCameraHistory_(animator));

    target.SetTranslate(cameraClipboard_.position);
    target.SetRotate(cameraClipboard_.rotation);
    target.SetFovY(cameraClipboard_.fov);
    target.Update();

    animator.AddOrUpdateKeyframe(animator.GetCurrentTime());
}

void AnimationEditorSession::EnsureEditorStateInitialized_(Object3d& target, Model::Skeleton& skeleton) {
    Model* model = target.GetModel();
    if (!model) {
        return;
    }

    const bool rebuilt = pose_.EnsureInitialized(model);
    if (rebuilt) {
        ResetAnimationHistory_();
        if (clipDocument_.HasValidClip()) {
            pose_.SampleFromAnimation(clipDocument_.GetAnimation(), editorTime_, skeleton);
        } else {
            pose_.ResetToBindPose();
        }
    }

    if (!isTestingPlay_) {
        ApplyCurrentPoseToTarget_(target, skeleton);
    }
}

void AnimationEditorSession::ApplyCurrentPoseToTarget_(Object3d& target, const Model::Skeleton& skeleton) {
    target.ApplyAnimationEditorPosePreview(pose_, skeleton);
}

void AnimationEditorSession::SampleClipAtCurrentTime_(Object3d& target, const Model::Skeleton& skeleton) {
    if (!clipDocument_.HasValidClip()) {
        pose_.ResetToBindPose();
        ApplyCurrentPoseToTarget_(target, skeleton);
        return;
    }

    clipDocument_.SetDuration(editorMaxDuration_);
    pose_.SampleFromAnimation(clipDocument_.GetAnimation(), editorTime_, skeleton);
    ApplyCurrentPoseToTarget_(target, skeleton);
}

void AnimationEditorSession::DrawToolbarWindow_(Object3d& target, Model::Skeleton& skeleton, const LayoutRects& layout) {
    ImGui::SetNextWindowPos(ImVec2(layout.toolbar.x, layout.toolbar.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.toolbar.w, layout.toolbar.h), ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Animation Toolbar", &windowVisibility_.toolbar, flags)) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Animation Editor");
    if (selectedJointIndex_ >= 0 && selectedJointIndex_ < static_cast<int32_t>(skeleton.joints.size())) {
        ImGui::SameLine();
        ImGui::TextDisabled("| Selected: %s", skeleton.joints[selectedJointIndex_].name.c_str());
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!CanUndoAnimation_());
    if (ImGui::Button("Undo")) {
        UndoAnimation_(target, skeleton);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!CanRedoAnimation_());
    if (ImGui::Button("Redo")) {
        RedoAnimation_(target, skeleton);
    }
    ImGui::EndDisabled();

    const std::string animationFileLabel =
        fs::path(exportFileName_).filename().string() + (animationDirty_ ? "*" : "");
    ImGui::Text("File: %s", animationFileLabel.c_str());

    ImGui::PushItemWidth(360.0f);
    ImGui::InputText("Clip Path", exportFileName_, IM_ARRAYSIZE(exportFileName_));
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("Load JSON")) {
        LoadClipToEdit_(target, skeleton);
    }

    ImGui::SameLine();
    if (ImGui::Button("Save JSON")) {
        const Model::Skeleton& exportSkeleton = target.GetModel() ? target.GetModel()->GetSkeleton() : skeleton;
        ExportCurrentClip_(exportSkeleton);
    }

    ImGui::SameLine();
    if (isTestingPlay_) {
        if (ImGui::Button("Stop Preview")) {
            StopTestPlay_(target, skeleton);
        }
    } else {
        if (ImGui::Button("Test Play")) {
            LoadClipAndPlay_(target);
        }
    }

    ImGui::Separator();

    if (ImGui::DragFloat("Length", &editorMaxDuration_, 0.05f, 0.1f, 60.0f, "%.2f sec")) {
        editorMaxDuration_ = std::max(editorMaxDuration_, 0.1f);
        clipDocument_.SetDuration(editorMaxDuration_);
        SetAnimationDirty_(true);
        editorTime_ = std::clamp(editorTime_, 0.0f, editorMaxDuration_);
        if (!isTestingPlay_) {
            SampleClipAtCurrentTime_(target, skeleton);
        }
    }

    ImGui::SameLine();
    ImGui::Text("Time %.2f / %.2f", editorTime_, editorMaxDuration_);

    if (currentGizmoOperation_ == ImGuizmo::SCALE) {
        currentGizmoOperation_ = ImGuizmo::ROTATE;
    }

    ImGui::SameLine(0.0f, 24.0f);
    if (ImGui::Button("Translate")) {
        currentGizmoOperation_ = ImGuizmo::TRANSLATE;
    }
    ImGui::SameLine();
    if (ImGui::Button("Rotate")) {
        currentGizmoOperation_ = ImGuizmo::ROTATE;
    }

    ImGui::SameLine(0.0f, 18.0f);
    if (ImGui::RadioButton("Local", currentGizmoMode_ == ImGuizmo::LOCAL)) {
        currentGizmoMode_ = ImGuizmo::LOCAL;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("World", currentGizmoMode_ == ImGuizmo::WORLD)) {
        currentGizmoMode_ = ImGuizmo::WORLD;
    }

    ImGui::End();
}

void AnimationEditorSession::DrawCameraToolbarWindow_(Camera& target, CameraAnimator& animator, const LayoutRects& layout) {
    ImGui::SetNextWindowPos(ImVec2(layout.toolbar.x, layout.toolbar.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.toolbar.w, layout.toolbar.h), ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Camera Toolbar", &windowVisibility_.toolbar, flags)) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Camera Editor");
    ImGui::SameLine();
    ImGui::BeginDisabled(!CanUndoCamera_());
    if (ImGui::Button("Undo")) {
        UndoCamera_(target, animator);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!CanRedoCamera_());
    if (ImGui::Button("Redo")) {
        RedoCamera_(target, animator);
    }
    ImGui::EndDisabled();

    const std::string cameraFileLabel =
        fs::path(animator.GetSaveFilepath()).filename().string() + (animator.IsDirty() ? "*" : "");
    ImGui::Text("File: %s", cameraFileLabel.c_str());

    bool isPlaying = animator.GetPlaying();
    if (ImGui::Checkbox("Play Camera", &isPlaying)) {
        animator.SetPlaying(isPlaying);
        if (isPlaying) {
            animator.SampleAtTime(animator.GetCurrentTime());
            target.Update();
        }
    }

    ImGui::SameLine();
    bool isLoop = animator.GetLoop();
    if (ImGui::Checkbox("Loop", &isLoop)) {
        animator.SetLoop(isLoop);
        animator.SetDirty(true);
    }

    ImGui::PushItemWidth(360.0f);
    ImGui::InputText("Camera Path", animator.GetSaveFilepathBuffer(), 256);
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("Load Camera")) {
        if (animator.LoadFromJson(animator.GetSaveFilepath())) {
            animator.SampleAtTime(animator.GetCurrentTime());
            target.Update();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Save Camera")) {
        animator.SaveToJson(animator.GetSaveFilepath());
    }

    ImGui::Separator();

    float duration = std::max(animator.GetMaxTime(), 0.1f);
    if (ImGui::DragFloat("Length", &duration, 0.05f, 0.1f, 60.0f, "%.2f sec")) {
        animator.SetMaxTime(duration);
    }

    ImGui::SameLine();
    ImGui::Text("Time %.2f / %.2f", animator.GetCurrentTime(), std::max(animator.GetMaxTime(), 0.1f));

    ImGui::End();
}

void AnimationEditorSession::DrawEditorNavigationSection_(const EditorContext& context) {
    ImGui::TextUnformatted("Editor Target");

    if (context.canEditAnimation) {
        const bool isAnimation = !context.editCameraMode;
        if (ImGui::RadioButton("Animation", isAnimation) && context.switchToAnimation) {
            context.switchToAnimation();
        }
    }

    if (context.canEditAnimation && context.canEditCamera) {
        ImGui::SameLine();
    }

    if (context.canEditCamera) {
        const bool isCamera = context.editCameraMode;
        if (ImGui::RadioButton("Camera", isCamera) && context.switchToCamera) {
            context.switchToCamera();
        }
    }

    if (context.cameraFiles || context.reloadCameraFiles || context.playRandomCamera) {
        ImGui::Separator();
        ImGui::TextUnformatted("Camera File Browser");

        if (context.reloadCameraFiles && ImGui::Button("Reload Camera Files")) {
            context.reloadCameraFiles();
        }

        if (context.randomCameraEnabled) {
            ImGui::Checkbox("Random Camera Change", context.randomCameraEnabled);
        }
        if (context.sameCameraLoopEnabled) {
            ImGui::Checkbox("Same Camera Loop", context.sameCameraLoopEnabled);
            if (*context.sameCameraLoopEnabled && context.randomCameraEnabled) {
                *context.randomCameraEnabled = false;
            }
        }
        if (context.cameraAnimator && context.sameCameraLoopEnabled) {
            context.cameraAnimator->SetLoop(*context.sameCameraLoopEnabled);
        }

        const int fileCount = context.cameraFiles ? static_cast<int>(context.cameraFiles->size()) : 0;
        ImGui::Text("Camera Files: %d", fileCount);
        ImGui::Text("Current Index: %d", context.currentCameraIndex);

        if (context.cameraFiles && context.loadCameraByIndex) {
            for (int i = 0; i < static_cast<int>(context.cameraFiles->size()); ++i) {
                std::string label = fs::path((*context.cameraFiles)[i]).filename().string();
                const bool selected = (i == context.currentCameraIndex);
                if (ImGui::Selectable(label.c_str(), selected)) {
                    context.loadCameraByIndex(i);
                }
            }
        }

        if (context.playRandomCamera && ImGui::Button("Play Random Camera")) {
            context.playRandomCamera();
        }
    }
}

void AnimationEditorSession::DrawCameraHierarchyWindow_(const LayoutRects& layout, const EditorContext& context) {
    ImGui::SetNextWindowPos(ImVec2(layout.hierarchy.x, layout.hierarchy.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.hierarchy.w, layout.hierarchy.h), ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Hierarchy", &windowVisibility_.hierarchy, flags)) {
        ImGui::End();
        return;
    }
    DrawEditorNavigationSection_(context);
    ImGui::Separator();
    ImGui::TextUnformatted("Targets");
    ImGui::Separator();
    ImGui::Selectable("Camera", true);
    ImGui::TextDisabled("Camera editing uses Inspector and Timeline.");
    ImGui::End();
}

void AnimationEditorSession::DrawCameraInspectorWindow_(Camera& target, CameraAnimator& animator, const LayoutRects& layout) {
    ImGui::SetNextWindowPos(ImVec2(layout.inspector.x, layout.inspector.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.inspector.w, layout.inspector.h), ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Inspector", &windowVisibility_.inspector, flags)) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Camera");
    ImGui::Separator();

    bool isPlaying = animator.GetPlaying();
    ImGui::BeginDisabled(isPlaying);

    if (ImGui::Button("Add Key At Current Time", ImVec2(-1.0f, 0.0f))) {
        PushCameraUndo_(CaptureCameraHistory_(animator));
        animator.AddOrUpdateKeyframe(animator.GetCurrentTime());
    }

    if (ImGui::Button("Delete Key At Current Time", ImVec2(-1.0f, 0.0f))) {
        PushCameraUndo_(CaptureCameraHistory_(animator));
        animator.DeleteKeyframeAt(animator.GetCurrentTime());
        animator.SampleAtTime(animator.GetCurrentTime());
        target.Update();
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Camera Transform");

    Vector3 position = target.GetTranslate();
    Vector3 rotation = target.GetRotate();
    float fov = target.GetFovY();
    bool changed = false;
    bool editCompleted = false;

    if (ImGui::InputFloat3("Position", &position.x, "%.3f")) {
        if (!pendingCameraInspectorHistory_.has_value()) {
            pendingCameraInspectorHistory_ = CaptureCameraHistory_(animator);
        }
        target.SetTranslate(position);
        changed = true;
    }
    editCompleted = editCompleted || ImGui::IsItemDeactivatedAfterEdit();
    if (ImGui::InputFloat3("Rotation", &rotation.x, "%.3f")) {
        if (!pendingCameraInspectorHistory_.has_value()) {
            pendingCameraInspectorHistory_ = CaptureCameraHistory_(animator);
        }
        target.SetRotate(rotation);
        changed = true;
    }
    editCompleted = editCompleted || ImGui::IsItemDeactivatedAfterEdit();
    if (ImGui::InputFloat("FOV", &fov, 0.001f, 0.01f, "%.3f")) {
        if (!pendingCameraInspectorHistory_.has_value()) {
            pendingCameraInspectorHistory_ = CaptureCameraHistory_(animator);
        }
        fov = std::clamp(fov, 0.1f, 3.0f);
        target.SetFovY(fov);
        changed = true;
    }
    editCompleted = editCompleted || ImGui::IsItemDeactivatedAfterEdit();

    if (changed) {
        animator.SetDirty(true);
        target.Update();
    }

    if (editCompleted && pendingCameraInspectorHistory_.has_value()) {
        PushCameraUndo_(pendingCameraInspectorHistory_.value());
        pendingCameraInspectorHistory_.reset();
    }

    ImGui::EndDisabled();

    const auto& keys = animator.GetKeyframes();
    bool hasKey = false;
    for (const auto& key : keys) {
        if (std::abs(key.time - animator.GetCurrentTime()) < 0.001f) {
            hasKey = true;
            break;
        }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Current Time Key");
    ImGui::BulletText("Camera Key: %s", hasKey ? "Yes" : "No");

    ImGui::End();
}

void AnimationEditorSession::DrawCameraTimelineWindow_(Camera& target, CameraAnimator& animator, const LayoutRects& layout) {
    ImGui::SetNextWindowPos(ImVec2(layout.timeline.x, layout.timeline.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.timeline.w, layout.timeline.h), ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Timeline", &windowVisibility_.timeline, flags)) {
        ImGui::End();
        return;
    }

    float currentTime = animator.GetCurrentTime();
    const float maxTime = std::max(animator.GetMaxTime(), 0.1f);
    if (ImGui::SliderFloat("Current Time", &currentTime, 0.0f, maxTime, "%.2f sec")) {
        animator.SetPlaying(false);
        animator.SampleAtTime(currentTime);
        target.Update();
    }

    ImGui::BeginDisabled(animator.GetPlaying());

    if (ImGui::Button("Add Camera Key")) {
        PushCameraUndo_(CaptureCameraHistory_(animator));
        animator.AddOrUpdateKeyframe(animator.GetCurrentTime());
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete Camera Key")) {
        PushCameraUndo_(CaptureCameraHistory_(animator));
        animator.DeleteKeyframeAt(animator.GetCurrentTime());
        animator.SampleAtTime(animator.GetCurrentTime());
        target.Update();
    }

    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::TextUnformatted("Track: Camera");

    const auto& keys = animator.GetKeyframes();
    auto drawKeyStrip = [&](const char* label, ImU32 color) {
        ImGui::Text("%s", label);
        ImGui::SameLine();

        const float stripWidth = ImGui::GetContentRegionAvail().x;
        const float stripHeight = 20.0f;
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImVec2 size(stripWidth, stripHeight);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(36, 39, 44, 255), 4.0f);
        drawList->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(72, 76, 84, 255), 4.0f);

        const float centerY = p.y + size.y * 0.5f;
        drawList->AddLine(
            ImVec2(p.x + 8.0f, centerY),
            ImVec2(p.x + size.x - 8.0f, centerY),
            IM_COL32(110, 110, 120, 255),
            1.0f);

        const float usableWidth = std::max(1.0f, size.x - 16.0f);
        const float currentX = p.x + 8.0f + (animator.GetCurrentTime() / maxTime) * usableWidth;
        drawList->AddLine(
            ImVec2(currentX, p.y + 2.0f),
            ImVec2(currentX, p.y + size.y - 2.0f),
            IM_COL32(255, 235, 120, 255),
            2.0f);

        for (const auto& key : keys) {
            const float x = p.x + 8.0f + (key.time / maxTime) * usableWidth;
            drawList->AddCircleFilled(ImVec2(x, centerY), 4.0f, color);
        }

        ImGui::Dummy(size);
    };

    drawKeyStrip("Position", IM_COL32(90, 170, 255, 255));
    drawKeyStrip("Rotation", IM_COL32(255, 140, 90, 255));
    drawKeyStrip("FOV", IM_COL32(180, 220, 120, 255));

    ImGui::Separator();
    ImGui::TextDisabled("Camera keys use the current camera position, rotation, and FOV.");

    ImGui::End();
}

void AnimationEditorSession::DrawCameraPreviewOverlay_(const LayoutRects& layout) {
    ImGui::SetNextWindowPos(
        ImVec2(layout.preview.x + 12.0f, layout.preview.y + 12.0f),
        ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.45f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_AlwaysAutoResize;

    if (!ImGui::Begin("Camera Preview Overlay", &windowVisibility_.preview, flags)) {
        ImGui::End();
        return;
    }
    ImGui::TextUnformatted("Camera Preview");
    ImGui::Separator();
    ImGui::TextDisabled("Edit Position / Rotation / FOV in Inspector.");
    ImGui::TextDisabled("Scrub and key camera motion in Timeline.");
    ImGui::End();
}

void AnimationEditorSession::DrawHierarchyWindow_(Model::Skeleton& skeleton, const LayoutRects& layout, const EditorContext& context) {
    ImGui::SetNextWindowPos(ImVec2(layout.hierarchy.x, layout.hierarchy.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.hierarchy.w, layout.hierarchy.h), ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Hierarchy", &windowVisibility_.hierarchy, flags)) {
        ImGui::End();
        return;
    }

    DrawEditorNavigationSection_(context);
    ImGui::Separator();

    ImGui::TextUnformatted("Bones");
    ImGui::Separator();
    ImGui::TextDisabled("Select from list or click in preview");

    auto isJointSelected = [&](int jointIndex) {
        return std::find(
            jointSelectionState_.selectedIndices.begin(),
            jointSelectionState_.selectedIndices.end(),
            jointIndex) != jointSelectionState_.selectedIndices.end();
    };

    auto isActiveJoint = [&](int jointIndex) {
        return jointSelectionState_.activeIndex == jointIndex;
    };

    std::function<void(int)> drawBoneNode = [&](int jointIndex) {
        if (jointIndex < 0 || jointIndex >= static_cast<int>(skeleton.joints.size())) {
            return;
        }

        auto& joint = skeleton.joints[jointIndex];

        ImGuiTreeNodeFlags nodeFlags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_DefaultOpen;

        if (joint.children.empty()) {
            nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        const bool selected = isJointSelected(jointIndex);
        const bool active = isActiveJoint(jointIndex);

        if (selected) {
            nodeFlags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::PushID(jointIndex);
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.90f, 0.55f, 0.18f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.98f, 0.65f, 0.22f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.82f, 0.46f, 0.12f, 1.0f));
        } else if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.24f, 0.52f, 0.88f, 0.70f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.30f, 0.60f, 0.96f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.22f, 0.48f, 0.82f, 0.95f));
        }
        const bool opened = ImGui::TreeNodeEx(joint.name.c_str(), nodeFlags);
        const bool treeItemClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        if (selected) {
            ImGui::SameLine();
            if (active) {
                ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.30f, 1.0f), "[Active]");
            } else {
                ImGui::TextColored(ImVec4(0.55f, 0.78f, 1.0f, 1.0f), "[Selected]");
            }
        }

        if (treeItemClicked) {
            const bool ctrlPressed = ImGui::GetIO().KeyCtrl;

            if (ctrlPressed) {
                if (jointSelectionState_.selectedIndices.empty() &&
                    selectedJointIndex_ >= 0 &&
                    selectedJointIndex_ < static_cast<int32_t>(skeleton.joints.size())) {
                    jointSelectionState_.selectedIndices.push_back(selectedJointIndex_);
                    if (jointSelectionState_.activeIndex < 0) {
                        jointSelectionState_.activeIndex = selectedJointIndex_;
                    }
                }

                auto it = std::find(
                    jointSelectionState_.selectedIndices.begin(),
                    jointSelectionState_.selectedIndices.end(),
                    jointIndex);

                if (it != jointSelectionState_.selectedIndices.end()) {
                    jointSelectionState_.selectedIndices.erase(it);
                    if (jointSelectionState_.activeIndex == jointIndex) {
                        jointSelectionState_.activeIndex =
                            jointSelectionState_.selectedIndices.empty()
                            ? -1
                            : jointSelectionState_.selectedIndices.back();
                    }
                } else {
                    jointSelectionState_.selectedIndices.push_back(jointIndex);
                    jointSelectionState_.activeIndex = jointIndex;
                }
            } else {
                jointSelectionState_.selectedIndices.clear();
                jointSelectionState_.selectedIndices.push_back(jointIndex);
                jointSelectionState_.activeIndex = jointIndex;
            }

            selectedJointIndex_ = jointSelectionState_.activeIndex;
        }

        if (active || selected) {
            ImGui::PopStyleColor(3);
        }

        if (opened && !joint.children.empty()) {
            for (int childIndex : joint.children) {
                drawBoneNode(childIndex);
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
        };

    drawBoneNode(skeleton.root);

    ImGui::End();
}

void AnimationEditorSession::DrawInspectorWindow_(Object3d& target, Model::Skeleton& skeleton, const LayoutRects& layout) {
    ImGui::SetNextWindowPos(ImVec2(layout.inspector.x, layout.inspector.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.inspector.w, layout.inspector.h), ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Inspector", &windowVisibility_.inspector, flags)) {
        ImGui::End();
        return;
    }

    int32_t inspectorJointIndex = selectedJointIndex_;
    if (jointSelectionState_.activeIndex >= 0 &&
        jointSelectionState_.activeIndex < static_cast<int32_t>(skeleton.joints.size())) {
        inspectorJointIndex = jointSelectionState_.activeIndex;
    }

    if (inspectorJointIndex < 0 || inspectorJointIndex >= static_cast<int32_t>(skeleton.joints.size())) {
        ImGui::TextWrapped("Select a bone from Hierarchy or click a joint in the preview.");
        ImGui::End();
        return;
    }

    std::vector<int32_t> targetIndices = jointSelectionState_.selectedIndices;
    if (targetIndices.empty()) {
        targetIndices.push_back(inspectorJointIndex);
    }

    const Model::Joint& joint = skeleton.joints[inspectorJointIndex];
    const AnimationEditorPose::JointPose& pose = pose_.GetJointPose(inspectorJointIndex);
    const AnimationEditorPose::JointPose& bindPose = pose_.GetBindPose(inspectorJointIndex);

    Vector3 translation = pose.translate;
    Vector3 rotation = pose_.GetJointRotationEulerDeg(inspectorJointIndex);
    Vector3 scale = pose.scale;
    const Vector3 bindRotation = pose_.GetBindRotationEulerDeg(inspectorJointIndex);

    ImGui::Text("Bone: %s", joint.name.c_str());
    ImGui::Text("Selected Count: %d", static_cast<int>(targetIndices.size()));
    if (targetIndices.size() > 1) {
        ImGui::TextColored(ImVec4(0.55f, 0.78f, 1.0f, 1.0f), "Multi-edit mode");
        ImGui::TextDisabled("Changes apply to all selected bones.");
    } else {
        ImGui::TextDisabled("Single bone edit");
    }
    ImGui::Separator();

    ImGui::BeginDisabled(isTestingPlay_);

      if (ImGui::Button("Reset To Bind Pose", ImVec2(-1.0f, 0.0f))) {
          PushAnimationUndo_(CaptureAnimationHistory_());
          pose_.ResetJointToBindPose(inspectorJointIndex);
          SetAnimationDirty_(true);
          ApplyCurrentPoseToTarget_(target, skeleton);
      }

    if (ImGui::Button("Add Key (All Bones)", ImVec2(-1.0f, 0.0f))) {
        PushAnimationUndo_(CaptureAnimationHistory_());
        RecordCurrentPoseAsKeyframe_(skeleton);
        SetAnimationDirty_(true);
    }

    if (ImGui::Button("Delete Key At Current Time", ImVec2(-1.0f, 0.0f))) {
        PushAnimationUndo_(CaptureAnimationHistory_());
        DeleteCurrentTimeKeyframe_();
        SetAnimationDirty_(true);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Local Transform");

    bool changed = false;
    bool editCompleted = false;

      if (ImGui::InputFloat3("Position", &translation.x, "%.3f")) {
          if (!pendingAnimationInspectorHistory_.has_value()) {
              pendingAnimationInspectorHistory_ = CaptureAnimationHistory_();
          }
          for (int32_t jointIndex : targetIndices) {
              if (jointIndex >= 0 && pose_.HasValidJoint(jointIndex)) {
                  pose_.SetJointTranslate(jointIndex, translation);
              }
          }
          changed = true;
      }
    editCompleted = editCompleted || ImGui::IsItemDeactivatedAfterEdit();

      if (ImGui::InputFloat3("Rotation", &rotation.x, "%.3f")) {
          if (!pendingAnimationInspectorHistory_.has_value()) {
              pendingAnimationInspectorHistory_ = CaptureAnimationHistory_();
          }
          for (int32_t jointIndex : targetIndices) {
              if (jointIndex >= 0 && pose_.HasValidJoint(jointIndex)) {
                  pose_.SetJointRotateEulerDeg(jointIndex, rotation);
              }
          }
          changed = true;
      }
    editCompleted = editCompleted || ImGui::IsItemDeactivatedAfterEdit();

      if (ImGui::InputFloat3("Scale", &scale.x, "%.3f")) {
          if (!pendingAnimationInspectorHistory_.has_value()) {
              pendingAnimationInspectorHistory_ = CaptureAnimationHistory_();
          }
          for (int32_t jointIndex : targetIndices) {
              if (jointIndex >= 0 && pose_.HasValidJoint(jointIndex)) {
                  pose_.SetJointScale(jointIndex, scale);
              }
          }
          changed = true;
      }
    editCompleted = editCompleted || ImGui::IsItemDeactivatedAfterEdit();

    if (changed) {
        SetAnimationDirty_(true);
        ApplyCurrentPoseToTarget_(target, skeleton);
    }

    if (editCompleted && pendingAnimationInspectorHistory_.has_value()) {
        PushAnimationUndo_(pendingAnimationInspectorHistory_.value());
        pendingAnimationInspectorHistory_.reset();
    }

    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::TextUnformatted("Bind Pose");
    ImGui::BulletText("Position: %.3f  %.3f  %.3f", bindPose.translate.x, bindPose.translate.y, bindPose.translate.z);
    ImGui::BulletText("Rotation: %.3f  %.3f  %.3f", bindRotation.x, bindRotation.y, bindRotation.z);
    ImGui::BulletText("Scale: %.3f  %.3f  %.3f", bindPose.scale.x, bindPose.scale.y, bindPose.scale.z);

    ImGui::Separator();

    auto it = clipDocument_.GetAnimation().nodeAnimations.find(joint.name);
    if (it == clipDocument_.GetAnimation().nodeAnimations.end()) {
        ImGui::TextDisabled("No keys for this bone.");
    } else {
        const NodeAnimation& na = it->second;
        const bool hasPos = HasKeyAtTime(na.translate.keyframes, editorTime_);
        const bool hasRot = HasKeyAtTime(na.rotate.keyframes, editorTime_);
        const bool hasScale = HasKeyAtTime(na.scale.keyframes, editorTime_);

        ImGui::TextUnformatted("Current Time Key");
        ImGui::BulletText("Position: %s", hasPos ? "Yes" : "No");
        ImGui::BulletText("Rotation: %s", hasRot ? "Yes" : "No");
        ImGui::BulletText("Scale: %s", hasScale ? "Yes" : "No");
    }

    ImGui::End();
}

void AnimationEditorSession::DrawTimelineWindow_(Object3d& target, Model::Skeleton& skeleton, const LayoutRects& layout) {
    ImGui::SetNextWindowPos(ImVec2(layout.timeline.x, layout.timeline.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.timeline.w, layout.timeline.h), ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Timeline", &windowVisibility_.timeline, flags)) {
        ImGui::End();
        return;
    }

    ImGui::BeginDisabled(isTestingPlay_);

    if (ImGui::SliderFloat("Current Time", &editorTime_, 0.0f, editorMaxDuration_, "%.2f sec")) {
        SampleClipAtCurrentTime_(target, skeleton);
    }

    if (ImGui::Button("Add Key (All Bones)")) {
        PushAnimationUndo_(CaptureAnimationHistory_());
        RecordCurrentPoseAsKeyframe_(skeleton);
        SetAnimationDirty_(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete Key")) {
        PushAnimationUndo_(CaptureAnimationHistory_());
        DeleteCurrentTimeKeyframe_();
        SetAnimationDirty_(true);
    }

    ImGui::EndDisabled();

    ImGui::Separator();

    if (selectedJointIndex_ < 0 || selectedJointIndex_ >= static_cast<int32_t>(skeleton.joints.size())) {
        ImGui::TextWrapped("Select a bone to inspect its keys.");
        ImGui::End();
        return;
    }

    const std::string& boneName = skeleton.joints[selectedJointIndex_].name;
    ImGui::Text("Bone: %s", boneName.c_str());

    auto animIt = clipDocument_.GetAnimation().nodeAnimations.find(boneName);
    if (animIt == clipDocument_.GetAnimation().nodeAnimations.end()) {
        ImGui::TextDisabled("No keyframes recorded for this bone.");
        ImGui::End();
        return;
    }

    auto drawKeyStrip = [&](const char* label, const auto& keyframes, ImU32 color) {
        ImGui::Text("%s", label);
        ImGui::SameLine();

        const float stripWidth = ImGui::GetContentRegionAvail().x;
        const float stripHeight = 20.0f;
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImVec2 size(stripWidth, stripHeight);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(36, 39, 44, 255), 4.0f);
        drawList->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(72, 76, 84, 255), 4.0f);

        const float centerY = p.y + size.y * 0.5f;
        drawList->AddLine(
            ImVec2(p.x + 8.0f, centerY),
            ImVec2(p.x + size.x - 8.0f, centerY),
            IM_COL32(110, 110, 120, 255),
            1.0f);

        const float usableWidth = std::max(1.0f, size.x - 16.0f);
        const float currentX = p.x + 8.0f + (editorTime_ / std::max(editorMaxDuration_, 0.0001f)) * usableWidth;
        drawList->AddLine(
            ImVec2(currentX, p.y + 2.0f),
            ImVec2(currentX, p.y + size.y - 2.0f),
            IM_COL32(255, 235, 120, 255),
            2.0f);

        for (const auto& key : keyframes) {
            const float x = p.x + 8.0f + (key.time / std::max(editorMaxDuration_, 0.0001f)) * usableWidth;
            drawList->AddCircleFilled(ImVec2(x, centerY), 4.0f, color);
        }

        ImGui::Dummy(size);
        };

    const NodeAnimation& na = animIt->second;
    drawKeyStrip("Position", na.translate.keyframes, IM_COL32(90, 170, 255, 255));
    drawKeyStrip("Rotation", na.rotate.keyframes, IM_COL32(255, 140, 90, 255));
    drawKeyStrip("Scale", na.scale.keyframes, IM_COL32(120, 220, 120, 255));

    ImGui::Separator();
    ImGui::TextDisabled("This is a minimal key overview. Full dope-sheet editing can come later.");

    ImGui::End();
}

void AnimationEditorSession::DrawPreviewOverlay_(Model::Skeleton& skeleton, const LayoutRects& layout) {
    ImGui::SetNextWindowPos(
        ImVec2(layout.preview.x + 12.0f, layout.preview.y + 12.0f),
        ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.45f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_AlwaysAutoResize;

    if (!ImGui::Begin("Preview Overlay", &windowVisibility_.preview, flags)) {
        ImGui::End();
        return;
    }
    ImGui::TextUnformatted("Preview");
    ImGui::Separator();
    ImGui::TextDisabled("Click joint to select");
    ImGui::TextDisabled("Use toolbar for gizmo mode");
    if (selectedJointIndex_ >= 0 && selectedJointIndex_ < static_cast<int32_t>(skeleton.joints.size())) {
        ImGui::Text("Selected: %s", skeleton.joints[selectedJointIndex_].name.c_str());
    } else {
        ImGui::TextDisabled("Selected: None");
    }
    ImGui::End();
}

void AnimationEditorSession::HandleViewportEditing_(Object3d& target, Model::Skeleton& skeleton, const LayoutRects& layout, Camera* editorCamera) {
    OutputDebugStringA("[AnimEditor] ENTER HandleViewportEditing\n");
    Camera* cam = editorCamera ? editorCamera : target.GetCamera();
    if (!cam) {
        return;
    }


    {
        OutputDebugStringA("[AnimEditor] START\n");
        char buf[512];
        std::snprintf(
            buf,
            sizeof(buf),
            "[AnimEditor] camera=%p camPos=(%.3f, %.3f, %.3f)\n",
            static_cast<void*>(cam),
            cam->GetTranslate().x,
            cam->GetTranslate().y,
            cam->GetTranslate().z);
        OutputDebugStringA(buf);
    }

    ImGuiIO& io = ImGui::GetIO();
    const ImGuiViewport* mainVp = ImGui::GetMainViewport();

    Rect renderRect{};
    renderRect.x = mainVp->Pos.x;
    renderRect.y = mainVp->Pos.y;
    renderRect.w = mainVp->Size.x;
    renderRect.h = mainVp->Size.y;

    const Rect& gizmoRect = renderRect;

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();
    ImGuizmo::SetRect(gizmoRect.x, gizmoRect.y, gizmoRect.w, gizmoRect.h);

    const bool mouseInGizmoRect =
        IsPointInsideRect_(io.MousePos.x, io.MousePos.y, gizmoRect);



    Matrix4x4 viewMat = cam->GetViewMatrix();
    Matrix4x4 projMat = cam->GetProjectionMatrix();

    {
        OutputDebugStringA("[AnimEditor] START\n");
        Matrix4x4 vpFromParts = Matrix4x4::Multiply(viewMat, projMat);
        const Matrix4x4& vpFromCamera = cam->GetViewProjectionMatrix();

        float maxDiff = 0.0f;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                float d = std::abs(vpFromParts.m[r][c] - vpFromCamera.m[r][c]);
                if (d > maxDiff) {
                    maxDiff = d;
                }
            }
        }

        char buf[1024];
        std::snprintf(
            buf,
            sizeof(buf),
            "[AnimEditor] viewRow3=(%.3f, %.3f, %.3f, %.3f) projRow3=(%.3f, %.3f, %.3f, %.3f) vpMaxDiff=%.6f\n",
            viewMat.m[3][0], viewMat.m[3][1], viewMat.m[3][2], viewMat.m[3][3],
            projMat.m[3][0], projMat.m[3][1], projMat.m[3][2], projMat.m[3][3],
            maxDiff);
        OutputDebugStringA(buf);
    }



    if (ImGui::IsMouseClicked(0) &&
        mouseInGizmoRect &&
        !io.WantCaptureMouse &&
        !ImGuizmo::IsOver()) {


        Ray ray = ComputePickingRay_(
            io.MousePos.x - gizmoRect.x,
            io.MousePos.y - gizmoRect.y,
            gizmoRect.w,
            gizmoRect.h,
            viewMat,
            projMat);




        Matrix4x4 objectWorld = target.GetWorldMatrix();

        float closestDist = 999999.0f;
        int hitJointIndex = -1;

        for (size_t i = 0; i < skeleton.joints.size(); ++i) {
            const auto& joint = skeleton.joints[i];

            Matrix4x4 finalWorld = Matrix4x4::Multiply(joint.skeletonSpaceMatrix, objectWorld);
            Vector3 jointPos = {
                finalWorld.m[3][0],
                finalWorld.m[3][1],
                finalWorld.m[3][2]
            };

            float hitDist = 0.0f;
            if (RaySphereIntersect_(ray, jointPos, kJointPickRadius, hitDist)) {
                if (hitDist < closestDist) {
                    closestDist = hitDist;
                    hitJointIndex = static_cast<int>(i);
                }
            }
        }

        if (hitJointIndex != -1) {
            selectedJointIndex_ = hitJointIndex;
        }
    }

    if (selectedJointIndex_ < 0 ||
        selectedJointIndex_ >= static_cast<int32_t>(skeleton.joints.size()) ||
        isTestingPlay_) {
        wasUsingGizmo_ = false;
        pendingAnimationGizmoHistory_.reset();
        return;
    }

    const auto& joint = skeleton.joints[selectedJointIndex_];

    Matrix4x4 objectWorld = target.GetWorldMatrix();
    Matrix4x4 boneSkeletonSpace = joint.skeletonSpaceMatrix;
    Matrix4x4 boneFinalWorld = Matrix4x4::Multiply(boneSkeletonSpace, objectWorld);

    Vector3 axisLen{};
    axisLen.x = std::sqrt(
        boneFinalWorld.m[0][0] * boneFinalWorld.m[0][0] +
        boneFinalWorld.m[0][1] * boneFinalWorld.m[0][1] +
        boneFinalWorld.m[0][2] * boneFinalWorld.m[0][2]);
    axisLen.y = std::sqrt(
        boneFinalWorld.m[1][0] * boneFinalWorld.m[1][0] +
        boneFinalWorld.m[1][1] * boneFinalWorld.m[1][1] +
        boneFinalWorld.m[1][2] * boneFinalWorld.m[1][2]);
    axisLen.z = std::sqrt(
        boneFinalWorld.m[2][0] * boneFinalWorld.m[2][0] +
        boneFinalWorld.m[2][1] * boneFinalWorld.m[2][1] +
        boneFinalWorld.m[2][2] * boneFinalWorld.m[2][2]);

    Matrix4x4 gizmoWorld = boneFinalWorld;

    if (axisLen.x > 0.0001f) {
        gizmoWorld.m[0][0] /= axisLen.x;
        gizmoWorld.m[0][1] /= axisLen.x;
        gizmoWorld.m[0][2] /= axisLen.x;
    }
    if (axisLen.y > 0.0001f) {
        gizmoWorld.m[1][0] /= axisLen.y;
        gizmoWorld.m[1][1] /= axisLen.y;
        gizmoWorld.m[1][2] /= axisLen.y;
    }
    if (axisLen.z > 0.0001f) {
        gizmoWorld.m[2][0] /= axisLen.z;
        gizmoWorld.m[2][1] /= axisLen.z;
        gizmoWorld.m[2][2] /= axisLen.z;
    }

    gizmoWorld.m[0][3] = 0.0f;
    gizmoWorld.m[1][3] = 0.0f;
    gizmoWorld.m[2][3] = 0.0f;
    gizmoWorld.m[3][3] = 1.0f;

    {
        const float det3x3 =
            gizmoWorld.m[0][0] * (gizmoWorld.m[1][1] * gizmoWorld.m[2][2] - gizmoWorld.m[1][2] * gizmoWorld.m[2][1]) -
            gizmoWorld.m[0][1] * (gizmoWorld.m[1][0] * gizmoWorld.m[2][2] - gizmoWorld.m[1][2] * gizmoWorld.m[2][0]) +
            gizmoWorld.m[0][2] * (gizmoWorld.m[1][0] * gizmoWorld.m[2][1] - gizmoWorld.m[1][1] * gizmoWorld.m[2][0]);

        char buf[256];
        std::snprintf(
            buf,
            sizeof(buf),
            "[AnimEditor] gizmoWorld det3x3=%.6f\n",
            det3x3);
        OutputDebugStringA(buf);

        if (det3x3 < 0.0f) {
            gizmoWorld.m[2][0] = -gizmoWorld.m[2][0];
            gizmoWorld.m[2][1] = -gizmoWorld.m[2][1];
            gizmoWorld.m[2][2] = -gizmoWorld.m[2][2];
        }
    }

    float view16[16];
    float proj16[16];
    float gizmoMat[16];

    viewMat.ToFloat16(view16);
    projMat.ToFloat16(proj16);
    gizmoWorld.ToFloat16(gizmoMat);



    ImGuizmo::Manipulate(
        view16,
        proj16,
        currentGizmoOperation_,
        ImGuizmo::LOCAL,
        gizmoMat);

    const bool isUsingGizmo = ImGuizmo::IsUsing();
    if (isUsingGizmo && !wasUsingGizmo_ && !pendingAnimationGizmoHistory_.has_value()) {
        pendingAnimationGizmoHistory_ = CaptureAnimationHistory_();
    }

    if (isUsingGizmo) {
        Matrix4x4 newBoneFinalWorld = Matrix4x4::FromFloat16(gizmoMat);


        Matrix4x4 parentWorld = objectWorld;
        if (joint.parent.has_value() && joint.parent.value() >= 0) {
            const int32_t parentIndex = joint.parent.value();
            parentWorld = Matrix4x4::Multiply(
                skeleton.joints[parentIndex].skeletonSpaceMatrix,
                objectWorld);
        }

        Matrix4x4 newLocal = Matrix4x4::Multiply(
            newBoneFinalWorld,
            Matrix4x4::Inverse(parentWorld));

        const auto& currentPose = pose_.GetJointPose(selectedJointIndex_);

        Vector3 newTranslate = {
            newLocal.m[3][0],
            newLocal.m[3][1],
            newLocal.m[3][2]
        };

        Vector3 extractedScale{};
        extractedScale.x = std::sqrt(
            newLocal.m[0][0] * newLocal.m[0][0] +
            newLocal.m[0][1] * newLocal.m[0][1] +
            newLocal.m[0][2] * newLocal.m[0][2]);
        extractedScale.y = std::sqrt(
            newLocal.m[1][0] * newLocal.m[1][0] +
            newLocal.m[1][1] * newLocal.m[1][1] +
            newLocal.m[1][2] * newLocal.m[1][2]);
        extractedScale.z = std::sqrt(
            newLocal.m[2][0] * newLocal.m[2][0] +
            newLocal.m[2][1] * newLocal.m[2][1] +
            newLocal.m[2][2] * newLocal.m[2][2]);

        Matrix4x4 rotationOnly = newLocal;

        if (extractedScale.x > 0.0001f) {
            rotationOnly.m[0][0] /= extractedScale.x;
            rotationOnly.m[0][1] /= extractedScale.x;
            rotationOnly.m[0][2] /= extractedScale.x;
        }
        if (extractedScale.y > 0.0001f) {
            rotationOnly.m[1][0] /= extractedScale.y;
            rotationOnly.m[1][1] /= extractedScale.y;
            rotationOnly.m[1][2] /= extractedScale.y;
        }
        if (extractedScale.z > 0.0001f) {
            rotationOnly.m[2][0] /= extractedScale.z;
            rotationOnly.m[2][1] /= extractedScale.z;
            rotationOnly.m[2][2] /= extractedScale.z;
        }

        rotationOnly.m[0][3] = 0.0f;
        rotationOnly.m[1][3] = 0.0f;
        rotationOnly.m[2][3] = 0.0f;
        rotationOnly.m[3][0] = 0.0f;
        rotationOnly.m[3][1] = 0.0f;
        rotationOnly.m[3][2] = 0.0f;
        rotationOnly.m[3][3] = 1.0f;

        Quaternion newRotate = MatrixToQuaternion(rotationOnly);
        newRotate = Normalize(newRotate);

        if (currentGizmoOperation_ == ImGuizmo::SCALE) {
            auto CalcAxisLen = [](const Matrix4x4& m) {
                Vector3 s{};
                s.x = std::sqrt(
                    m.m[0][0] * m.m[0][0] +
                    m.m[0][1] * m.m[0][1] +
                    m.m[0][2] * m.m[0][2]);
                s.y = std::sqrt(
                    m.m[1][0] * m.m[1][0] +
                    m.m[1][1] * m.m[1][1] +
                    m.m[1][2] * m.m[1][2]);
                s.z = std::sqrt(
                    m.m[2][0] * m.m[2][0] +
                    m.m[2][1] * m.m[2][1] +
                    m.m[2][2] * m.m[2][2]);
                return s;
                };

            Vector3 parentScale = CalcAxisLen(parentWorld);
            Vector3 worldScale = CalcAxisLen(newBoneFinalWorld);

            char buf[1024];
            std::snprintf(
                buf,
                sizeof(buf),
                "[AnimEditor][Scale] current=(%.3f, %.3f, %.3f) parent=(%.3f, %.3f, %.3f) world=(%.3f, %.3f, %.3f) local=(%.3f, %.3f, %.3f)\n",
                currentPose.scale.x, currentPose.scale.y, currentPose.scale.z,
                parentScale.x, parentScale.y, parentScale.z,
                worldScale.x, worldScale.y, worldScale.z,
                extractedScale.x, extractedScale.y, extractedScale.z);
            OutputDebugStringA(buf);
        }

        pose_.SetJointTranslate(selectedJointIndex_, newTranslate);
        pose_.SetJointRotateQuaternion(selectedJointIndex_, newRotate);

        if (currentGizmoOperation_ == ImGuizmo::SCALE) {
            pose_.SetJointScale(selectedJointIndex_, extractedScale);
        }
        else {
        pose_.SetJointScale(selectedJointIndex_, currentPose.scale);
        }


        SetAnimationDirty_(true);
        ApplyCurrentPoseToTarget_(target, skeleton);
    }

    if (!isUsingGizmo && wasUsingGizmo_ && pendingAnimationGizmoHistory_.has_value()) {
        PushAnimationUndo_(pendingAnimationGizmoHistory_.value());
        pendingAnimationGizmoHistory_.reset();
    }

    wasUsingGizmo_ = isUsingGizmo;

}

void AnimationEditorSession::RecordCurrentPoseAsKeyframe_(const Model::Skeleton& skeleton) {
    clipDocument_.SetDuration(editorMaxDuration_);
    Animation& editedAnim = clipDocument_.GetAnimation();

    std::vector<int32_t> targetIndices = jointSelectionState_.selectedIndices;
    if (targetIndices.empty()) {
        if (selectedJointIndex_ >= 0 &&
            selectedJointIndex_ < static_cast<int32_t>(skeleton.joints.size()) &&
            pose_.HasValidJoint(selectedJointIndex_)) {
            targetIndices.push_back(selectedJointIndex_);
        } else {
            const size_t jointCount = std::min(pose_.GetJointCount(), skeleton.joints.size());
            targetIndices.reserve(jointCount);
            for (size_t i = 0; i < jointCount; ++i) {
                targetIndices.push_back(static_cast<int32_t>(i));
            }
        }
    }

    for (int32_t jointIndex : targetIndices) {
        if (jointIndex < 0 ||
            jointIndex >= static_cast<int32_t>(skeleton.joints.size()) ||
            !pose_.HasValidJoint(jointIndex)) {
            continue;
        }

        const std::string& boneName = skeleton.joints[jointIndex].name;
        const auto& pose = pose_.GetJointPose(jointIndex);
        auto& na = editedAnim.nodeAnimations[boneName];

        const Vector3 safePos = pose.translate;
        const Quaternion safeRot = Normalize(pose.rotate);
        const Vector3 safeScale = {
            (std::abs(pose.scale.x) < 0.001f || std::isnan(pose.scale.x)) ? 1.0f : pose.scale.x,
            (std::abs(pose.scale.y) < 0.001f || std::isnan(pose.scale.y)) ? 1.0f : pose.scale.y,
            (std::abs(pose.scale.z) < 0.001f || std::isnan(pose.scale.z)) ? 1.0f : pose.scale.z
        };

        auto itT = std::find_if(
            na.translate.keyframes.begin(),
            na.translate.keyframes.end(),
            [&](const KeyframeVector3& k) { return std::abs(k.time - editorTime_) < 0.001f; });

        if (itT != na.translate.keyframes.end()) {
            itT->value = safePos;
        } else {
            na.translate.keyframes.push_back({ editorTime_, safePos });
        }

        auto itR = std::find_if(
            na.rotate.keyframes.begin(),
            na.rotate.keyframes.end(),
            [&](const KeyframeQuaternion& k) { return std::abs(k.time - editorTime_) < 0.001f; });

        if (itR != na.rotate.keyframes.end()) {
            itR->value = safeRot;
        } else {
            na.rotate.keyframes.push_back({ editorTime_, safeRot });
        }

        auto itS = std::find_if(
            na.scale.keyframes.begin(),
            na.scale.keyframes.end(),
            [&](const KeyframeVector3& k) { return std::abs(k.time - editorTime_) < 0.001f; });

        if (itS != na.scale.keyframes.end()) {
            itS->value = safeScale;
        } else {
            na.scale.keyframes.push_back({ editorTime_, safeScale });
        }

        std::sort(
            na.translate.keyframes.begin(),
            na.translate.keyframes.end(),
            [](const KeyframeVector3& a, const KeyframeVector3& b) { return a.time < b.time; });

        std::sort(
            na.rotate.keyframes.begin(),
            na.rotate.keyframes.end(),
            [](const KeyframeQuaternion& a, const KeyframeQuaternion& b) { return a.time < b.time; });

        std::sort(
            na.scale.keyframes.begin(),
            na.scale.keyframes.end(),
            [](const KeyframeVector3& a, const KeyframeVector3& b) { return a.time < b.time; });
    }

    OutputDebugStringA("Keyframe Recorded!\n");
}

void AnimationEditorSession::DeleteCurrentTimeKeyframe_() {
    Animation& editedAnim = clipDocument_.GetAnimation();

    auto eraseAtCurrentTime = [&](NodeAnimation& na) {
        auto isCurrentTime = [&](const auto& k) {
            return std::abs(k.time - editorTime_) < 0.001f;
        };

        na.translate.keyframes.erase(
            std::remove_if(na.translate.keyframes.begin(), na.translate.keyframes.end(), isCurrentTime),
            na.translate.keyframes.end());

        na.rotate.keyframes.erase(
            std::remove_if(na.rotate.keyframes.begin(), na.rotate.keyframes.end(), isCurrentTime),
            na.rotate.keyframes.end());

        na.scale.keyframes.erase(
            std::remove_if(na.scale.keyframes.begin(), na.scale.keyframes.end(), isCurrentTime),
            na.scale.keyframes.end());
    };

    if (!jointSelectionState_.selectedIndices.empty()) {
        std::vector<std::string> selectedBoneNames;
        selectedBoneNames.reserve(jointSelectionState_.selectedIndices.size());
        for (int32_t jointIndex : jointSelectionState_.selectedIndices) {
            if (jointIndex >= 0 && historyAnimationTarget_ && historyAnimationTarget_->GetSkeleton() &&
                jointIndex < static_cast<int32_t>(historyAnimationTarget_->GetSkeleton()->joints.size())) {
                selectedBoneNames.push_back(historyAnimationTarget_->GetSkeleton()->joints[jointIndex].name);
            }
        }

        for (const std::string& boneName : selectedBoneNames) {
            auto it = editedAnim.nodeAnimations.find(boneName);
            if (it != editedAnim.nodeAnimations.end()) {
                eraseAtCurrentTime(it->second);
            }
        }
    } else {
        for (auto& pair : editedAnim.nodeAnimations) {
            auto& na = pair.second;
            eraseAtCurrentTime(na);
        }
    }

    OutputDebugStringA("Keyframe Deleted!\n");
}

void AnimationEditorSession::ExportCurrentClip_(const Model::Skeleton& skeleton) {
    clipDocument_.SetDuration(editorMaxDuration_);
    clipDocument_.PrepareForExport(skeleton);

    if (clipDocument_.SaveToJson(exportFileName_)) {
        SetAnimationDirty_(false);
        OutputDebugStringA(("Animation Exported to: " + std::string(exportFileName_) + "\n").c_str());
    } else {
        OutputDebugStringA("Failed to open file for writing!\n");
    }
}

void AnimationEditorSession::LoadClipToEdit_(Object3d& target, const Model::Skeleton& skeleton) {
    AnimationClipDocument loadedDocument;
    if (loadedDocument.LoadFromJson(exportFileName_) && loadedDocument.HasValidClip()) {
        clipDocument_.ReplaceWith(loadedDocument.GetAnimation());
        editorMaxDuration_ = clipDocument_.GetDuration();
        editorTime_ = 0.0f;
        isTestingPlay_ = false;
        SetAnimationDirty_(false);

        pose_.SampleFromAnimation(clipDocument_.GetAnimation(), editorTime_, skeleton);
        ApplyCurrentPoseToTarget_(target, skeleton);

        OutputDebugStringA("Animation Loaded for Editing!\n");
    }
}

void AnimationEditorSession::LoadClipAndPlay_(Object3d& target) {
    AnimationClipDocument playDocument;
    if (playDocument.LoadFromJson(exportFileName_) && playDocument.HasValidClip()) {
        if (target.GetModel()) {
            target.GetModel()->AddAnimation("CustomAnim", playDocument.GetAnimation());
            target.PlayAnimation("CustomAnim", true);

            target.ClearBonePreviewOffsets();
            isTestingPlay_ = true;

            OutputDebugStringA("Animation Loaded and Playing!\n");
        }
    }
}

void AnimationEditorSession::StopTestPlay_(Object3d& target, const Model::Skeleton& skeleton) {
    target.StopAnimation();
    isTestingPlay_ = false;
    ApplyCurrentPoseToTarget_(target, skeleton);
}

#endif
