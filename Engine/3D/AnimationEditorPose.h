#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Animation.h"
#include "Model.h"

class AnimationEditorPose {
public:
    struct JointPose {
        Vector3 translate{ 0.0f, 0.0f, 0.0f };
        Quaternion rotate{ 0.0f, 0.0f, 0.0f, 1.0f };
        Vector3 scale{ 1.0f, 1.0f, 1.0f };
    };

public:
    void Clear();
    bool EnsureInitialized(Model* model);

    bool IsInitialized() const { return initialized_; }
    bool HasValidJoint(int32_t jointIndex) const;
    size_t GetJointCount() const { return currentLocalPoses_.size(); }

    void ResetToBindPose();
    void ResetJointToBindPose(int32_t jointIndex);

    void SampleFromAnimation(const Animation& animation, float time, const Model::Skeleton& skeleton);

    const JointPose& GetJointPose(int32_t jointIndex) const;
    JointPose& GetJointPose(int32_t jointIndex);

    const JointPose& GetBindPose(int32_t jointIndex) const;

    Vector3 GetJointRotationEulerDeg(int32_t jointIndex) const;
    Vector3 GetBindRotationEulerDeg(int32_t jointIndex) const;

    void SetJointTranslate(int32_t jointIndex, const Vector3& value);
    void SetJointRotateQuaternion(int32_t jointIndex, const Quaternion& value);
    void SetJointRotateEulerDeg(int32_t jointIndex, const Vector3& value);
    void SetJointScale(int32_t jointIndex, const Vector3& value);

    void SetJointPoseFromLocalMatrix(int32_t jointIndex, const Matrix4x4& localMatrix);

private:
    static Quaternion NormalizeSafe_(const Quaternion& q);
    static Vector3 SanitizeScale_(const Vector3& scale);
    static Vector3 QuaternionToEulerDeg_(const Quaternion& q);
    static Quaternion EulerDegToQuaternion_(const Vector3& rotDeg);
    static Vector3 ExtractTranslation_(const Matrix4x4& m);
    static Vector3 ExtractScale_(const Matrix4x4& m);
    static Quaternion ExtractRotationQuaternion_(const Matrix4x4& m);

private:
    Model* sourceModel_ = nullptr;
    bool initialized_ = false;

    std::vector<JointPose> bindLocalPoses_;
    std::vector<JointPose> currentLocalPoses_;
};
