#include "AnimationEditorPose.h"

#include <cmath>
#include <algorithm>

#include "AnimationEvaluate.h"

namespace {
    constexpr float kPi = 3.14159265f;

    AnimationEditorPose::JointPose MakeDefaultPose() {
        return AnimationEditorPose::JointPose{};
    }
}

void AnimationEditorPose::Clear() {
    sourceModel_ = nullptr;
    initialized_ = false;
    bindLocalPoses_.clear();
    currentLocalPoses_.clear();
}

bool AnimationEditorPose::EnsureInitialized(Model* model) {
    if (!model) {
        Clear();
        return false;
    }

    const Model::Skeleton& skeleton = model->GetSkeleton();
    const bool needsRebuild =
        (!initialized_) ||
        (sourceModel_ != model) ||
        (bindLocalPoses_.size() != skeleton.joints.size()) ||
        (currentLocalPoses_.size() != skeleton.joints.size());

    if (!needsRebuild) {
        return false;
    }

    sourceModel_ = model;
    initialized_ = true;

    bindLocalPoses_.clear();
    currentLocalPoses_.clear();
    bindLocalPoses_.reserve(skeleton.joints.size());
    currentLocalPoses_.reserve(skeleton.joints.size());

    for (const auto& joint : skeleton.joints) {
        JointPose pose{};
        pose.translate = joint.transform.translate;
        pose.rotate = NormalizeSafe_(joint.transform.rotate);
        pose.scale = SanitizeScale_(joint.transform.scale);

        bindLocalPoses_.push_back(pose);
        currentLocalPoses_.push_back(pose);
    }

    return true;
}

bool AnimationEditorPose::HasValidJoint(int32_t jointIndex) const {
    return jointIndex >= 0 &&
        static_cast<size_t>(jointIndex) < currentLocalPoses_.size();
}

void AnimationEditorPose::ResetToBindPose() {
    currentLocalPoses_ = bindLocalPoses_;
}

void AnimationEditorPose::ResetJointToBindPose(int32_t jointIndex) {
    if (!HasValidJoint(jointIndex)) {
        return;
    }
    currentLocalPoses_[jointIndex] = bindLocalPoses_[jointIndex];
}

void AnimationEditorPose::SampleFromAnimation(
    const Animation& animation,
    float time,
    const Model::Skeleton& skeleton) {
    if (!initialized_) {
        return;
    }

    currentLocalPoses_ = bindLocalPoses_;

    const size_t jointCount = std::min(currentLocalPoses_.size(), skeleton.joints.size());
    for (size_t i = 0; i < jointCount; ++i) {
        const std::string& boneName = skeleton.joints[i].name;
        auto it = animation.nodeAnimations.find(boneName);
        if (it == animation.nodeAnimations.end()) {
            continue;
        }

        const NodeAnimation& na = it->second;
        JointPose& pose = currentLocalPoses_[i];

        if (!na.translate.keyframes.empty()) {
            pose.translate = CalculateValue(na.translate.keyframes, time);
        }
        if (!na.rotate.keyframes.empty()) {
            pose.rotate = NormalizeSafe_(CalculateValue(na.rotate.keyframes, time));
        }
        if (!na.scale.keyframes.empty()) {
            pose.scale = SanitizeScale_(CalculateValue(na.scale.keyframes, time));
        }
    }
}

const AnimationEditorPose::JointPose& AnimationEditorPose::GetJointPose(int32_t jointIndex) const {
    static JointPose dummy = MakeDefaultPose();
    if (!HasValidJoint(jointIndex)) {
        return dummy;
    }
    return currentLocalPoses_[jointIndex];
}

AnimationEditorPose::JointPose& AnimationEditorPose::GetJointPose(int32_t jointIndex) {
    static JointPose dummy = MakeDefaultPose();
    if (!HasValidJoint(jointIndex)) {
        return dummy;
    }
    return currentLocalPoses_[jointIndex];
}

const AnimationEditorPose::JointPose& AnimationEditorPose::GetBindPose(int32_t jointIndex) const {
    static JointPose dummy = MakeDefaultPose();
    if (jointIndex < 0 || static_cast<size_t>(jointIndex) >= bindLocalPoses_.size()) {
        return dummy;
    }
    return bindLocalPoses_[jointIndex];
}

Vector3 AnimationEditorPose::GetJointRotationEulerDeg(int32_t jointIndex) const {
    if (!HasValidJoint(jointIndex)) {
        return { 0.0f, 0.0f, 0.0f };
    }
    return QuaternionToEulerDeg_(currentLocalPoses_[jointIndex].rotate);
}

Vector3 AnimationEditorPose::GetBindRotationEulerDeg(int32_t jointIndex) const {
    if (jointIndex < 0 || static_cast<size_t>(jointIndex) >= bindLocalPoses_.size()) {
        return { 0.0f, 0.0f, 0.0f };
    }
    return QuaternionToEulerDeg_(bindLocalPoses_[jointIndex].rotate);
}

void AnimationEditorPose::SetJointTranslate(int32_t jointIndex, const Vector3& value) {
    if (!HasValidJoint(jointIndex)) {
        return;
    }
    currentLocalPoses_[jointIndex].translate = value;
}

void AnimationEditorPose::SetJointRotateQuaternion(int32_t jointIndex, const Quaternion& value) {
    if (!HasValidJoint(jointIndex)) {
        return;
    }
    currentLocalPoses_[jointIndex].rotate = NormalizeSafe_(value);
}

void AnimationEditorPose::SetJointRotateEulerDeg(int32_t jointIndex, const Vector3& value) {
    if (!HasValidJoint(jointIndex)) {
        return;
    }
    currentLocalPoses_[jointIndex].rotate = EulerDegToQuaternion_(value);
}

void AnimationEditorPose::SetJointScale(int32_t jointIndex, const Vector3& value) {
    if (!HasValidJoint(jointIndex)) {
        return;
    }
    currentLocalPoses_[jointIndex].scale = SanitizeScale_(value);
}

void AnimationEditorPose::SetJointPoseFromLocalMatrix(int32_t jointIndex, const Matrix4x4& localMatrix) {
    if (!HasValidJoint(jointIndex)) {
        return;
    }

    JointPose& pose = currentLocalPoses_[jointIndex];
    pose.translate = ExtractTranslation_(localMatrix);
    pose.scale = SanitizeScale_(ExtractScale_(localMatrix));
    pose.rotate = NormalizeSafe_(ExtractRotationQuaternion_(localMatrix));
}

Quaternion AnimationEditorPose::NormalizeSafe_(const Quaternion& q) {
    float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len < 0.0001f || std::isnan(len)) {
        return { 0.0f, 0.0f, 0.0f, 1.0f };
    }

    Quaternion out{};
    out.x = q.x / len;
    out.y = q.y / len;
    out.z = q.z / len;
    out.w = q.w / len;
    return out;
}

Vector3 AnimationEditorPose::SanitizeScale_(const Vector3& scale) {
    Vector3 out = scale;
    if (std::abs(out.x) < 0.001f || std::isnan(out.x)) out.x = 1.0f;
    if (std::abs(out.y) < 0.001f || std::isnan(out.y)) out.y = 1.0f;
    if (std::abs(out.z) < 0.001f || std::isnan(out.z)) out.z = 1.0f;
    return out;
}

Vector3 AnimationEditorPose::QuaternionToEulerDeg_(const Quaternion& q) {
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

Quaternion AnimationEditorPose::EulerDegToQuaternion_(const Vector3& rotDeg) {
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
    return NormalizeSafe_(q);
}

Vector3 AnimationEditorPose::ExtractTranslation_(const Matrix4x4& m) {
    return { m.m[3][0], m.m[3][1], m.m[3][2] };
}

Vector3 AnimationEditorPose::ExtractScale_(const Matrix4x4& m) {
    Vector3 scale{};
    scale.x = std::sqrt(m.m[0][0] * m.m[0][0] + m.m[0][1] * m.m[0][1] + m.m[0][2] * m.m[0][2]);
    scale.y = std::sqrt(m.m[1][0] * m.m[1][0] + m.m[1][1] * m.m[1][1] + m.m[1][2] * m.m[1][2]);
    scale.z = std::sqrt(m.m[2][0] * m.m[2][0] + m.m[2][1] * m.m[2][1] + m.m[2][2] * m.m[2][2]);
    return scale;
}

Quaternion AnimationEditorPose::ExtractRotationQuaternion_(const Matrix4x4& m) {
    Matrix4x4 rot = m;

    Vector3 scale = ExtractScale_(m);
    if (scale.x > 0.0001f) {
        rot.m[0][0] /= scale.x; rot.m[0][1] /= scale.x; rot.m[0][2] /= scale.x;
    }
    if (scale.y > 0.0001f) {
        rot.m[1][0] /= scale.y; rot.m[1][1] /= scale.y; rot.m[1][2] /= scale.y;
    }
    if (scale.z > 0.0001f) {
        rot.m[2][0] /= scale.z; rot.m[2][1] /= scale.z; rot.m[2][2] /= scale.z;
    }

    rot.m[3][0] = 0.0f;
    rot.m[3][1] = 0.0f;
    rot.m[3][2] = 0.0f;
    rot.m[3][3] = 1.0f;

    return MatrixToQuaternion(rot);
}
