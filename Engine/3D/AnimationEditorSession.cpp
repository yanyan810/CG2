#include "AnimationEditorSession.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>
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
}

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

void AnimationEditorSession::DrawImGui(Object3d* target, Camera* editorCamera) {
#ifdef USE_IMGUI
    OutputDebugStringA("[AnimEditor] Session DrawImGui\n");

    if (!target) {
        OutputDebugStringA("[AnimEditor] target is null\n");
        return;
    }

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

    DrawToolbarWindow_(*target, *skeleton, layout);
    DrawHierarchyWindow_(*skeleton, layout);
    DrawInspectorWindow_(*target, *skeleton, layout);
    DrawTimelineWindow_(*target, *skeleton, layout);
    DrawPreviewOverlay_(*skeleton, layout);
    HandleViewportEditing_(*target, *skeleton, layout, editorCamera);
#else
    (void)target;
    (void)editorCamera;
#endif
}

#ifdef USE_IMGUI

void AnimationEditorSession::EnsureEditorStateInitialized_(Object3d& target, Model::Skeleton& skeleton) {
    Model* model = target.GetModel();
    if (!model) {
        return;
    }

    const bool rebuilt = pose_.EnsureInitialized(model);
    if (rebuilt) {
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
    pose_.ApplyToBoneOffsets(target.boneOffsets_, skeleton);
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

    ImGui::Begin("Animation Toolbar", nullptr, flags);

    ImGui::TextUnformatted("Animation Editor");
    if (selectedJointIndex_ >= 0 && selectedJointIndex_ < static_cast<int32_t>(skeleton.joints.size())) {
        ImGui::SameLine();
        ImGui::TextDisabled("| Selected: %s", skeleton.joints[selectedJointIndex_].name.c_str());
    }

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
        editorTime_ = std::clamp(editorTime_, 0.0f, editorMaxDuration_);
        if (!isTestingPlay_) {
            SampleClipAtCurrentTime_(target, skeleton);
        }
    }

    ImGui::SameLine();
    ImGui::Text("Time %.2f / %.2f", editorTime_, editorMaxDuration_);

    ImGui::SameLine(0.0f, 24.0f);
    if (ImGui::Button("Translate")) {
        currentGizmoOperation_ = ImGuizmo::TRANSLATE;
    }
    ImGui::SameLine();
    if (ImGui::Button("Rotate")) {
        currentGizmoOperation_ = ImGuizmo::ROTATE;
    }
    ImGui::SameLine();
    if (ImGui::Button("Scale")) {
        currentGizmoOperation_ = ImGuizmo::SCALE;
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

void AnimationEditorSession::DrawHierarchyWindow_(Model::Skeleton& skeleton, const LayoutRects& layout) {
    ImGui::SetNextWindowPos(ImVec2(layout.hierarchy.x, layout.hierarchy.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(layout.hierarchy.w, layout.hierarchy.h), ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Hierarchy", nullptr, flags);

    ImGui::TextUnformatted("Bones");
    ImGui::Separator();
    ImGui::TextDisabled("Select from list or click in preview");

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

        if (jointIndex == selectedJointIndex_) {
            nodeFlags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::PushID(jointIndex);
        const bool opened = ImGui::TreeNodeEx(joint.name.c_str(), nodeFlags);

        if (ImGui::IsItemClicked()) {
            selectedJointIndex_ = jointIndex;
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

    ImGui::Begin("Inspector", nullptr, flags);

    if (selectedJointIndex_ < 0 || selectedJointIndex_ >= static_cast<int32_t>(skeleton.joints.size())) {
        ImGui::TextWrapped("Select a bone from Hierarchy or click a joint in the preview.");
        ImGui::End();
        return;
    }

    const Model::Joint& joint = skeleton.joints[selectedJointIndex_];
    const AnimationEditorPose::JointPose& pose = pose_.GetJointPose(selectedJointIndex_);
    const AnimationEditorPose::JointPose& bindPose = pose_.GetBindPose(selectedJointIndex_);

    Vector3 translation = pose.translate;
    Vector3 rotation = pose_.GetJointRotationEulerDeg(selectedJointIndex_);
    Vector3 scale = pose.scale;
    const Vector3 bindRotation = pose_.GetBindRotationEulerDeg(selectedJointIndex_);

    ImGui::Text("Bone: %s", joint.name.c_str());
    ImGui::Separator();

    ImGui::BeginDisabled(isTestingPlay_);

    if (ImGui::Button("Reset To Bind Pose", ImVec2(-1.0f, 0.0f))) {
        pose_.ResetJointToBindPose(selectedJointIndex_);
        ApplyCurrentPoseToTarget_(target, skeleton);
    }

    if (ImGui::Button("Add Key (All Bones)", ImVec2(-1.0f, 0.0f))) {
        RecordCurrentPoseAsKeyframe_(skeleton);
    }

    if (ImGui::Button("Delete Key At Current Time", ImVec2(-1.0f, 0.0f))) {
        DeleteCurrentTimeKeyframe_();
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Local Transform");

    bool changed = false;

    if (ImGui::InputFloat3("Position", &translation.x, "%.3f")) {
        pose_.SetJointTranslate(selectedJointIndex_, translation);
        changed = true;
    }

    if (ImGui::InputFloat3("Rotation", &rotation.x, "%.3f")) {
        pose_.SetJointRotateEulerDeg(selectedJointIndex_, rotation);
        changed = true;
    }

    if (ImGui::InputFloat3("Scale", &scale.x, "%.3f")) {
        pose_.SetJointScale(selectedJointIndex_, scale);
        changed = true;
    }

    if (changed) {
        ApplyCurrentPoseToTarget_(target, skeleton);
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

    ImGui::Begin("Timeline", nullptr, flags);

    ImGui::BeginDisabled(isTestingPlay_);

    if (ImGui::SliderFloat("Current Time", &editorTime_, 0.0f, editorMaxDuration_, "%.2f sec")) {
        SampleClipAtCurrentTime_(target, skeleton);
    }

    if (ImGui::Button("Add Key (All Bones)")) {
        RecordCurrentPoseAsKeyframe_(skeleton);
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete Key")) {
        DeleteCurrentTimeKeyframe_();
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

    ImGui::Begin("Preview Overlay", nullptr, flags);
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




    if (ImGuizmo::IsUsing()) {
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

        pose_.SetJointTranslate(selectedJointIndex_, newTranslate);
        pose_.SetJointRotateQuaternion(selectedJointIndex_, newRotate);

        if (currentGizmoOperation_ == ImGuizmo::SCALE) {
            pose_.SetJointScale(selectedJointIndex_, extractedScale);
        } else {
            pose_.SetJointScale(selectedJointIndex_, currentPose.scale);
        }

        ApplyCurrentPoseToTarget_(target, skeleton);
    }

}

void AnimationEditorSession::RecordCurrentPoseAsKeyframe_(const Model::Skeleton& skeleton) {
    clipDocument_.SetDuration(editorMaxDuration_);
    Animation& editedAnim = clipDocument_.GetAnimation();

    const size_t jointCount = std::min(pose_.GetJointCount(), skeleton.joints.size());
    for (size_t i = 0; i < jointCount; ++i) {
        const std::string& boneName = skeleton.joints[i].name;
        const auto& pose = pose_.GetJointPose(static_cast<int32_t>(i));
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

    for (auto& pair : editedAnim.nodeAnimations) {
        auto& na = pair.second;

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
    }

    OutputDebugStringA("Keyframe Deleted!\n");
}

void AnimationEditorSession::ExportCurrentClip_(const Model::Skeleton& skeleton) {
    clipDocument_.SetDuration(editorMaxDuration_);
    clipDocument_.PrepareForExport(skeleton);

    if (clipDocument_.SaveToJson(exportFileName_)) {
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

            target.boneOffsets_.clear();
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
