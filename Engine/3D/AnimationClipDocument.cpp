#include "AnimationClipDocument.h"

#include <algorithm>
#include "AnimationJsonSerializer.h"

void AnimationClipDocument::Clear() {
    animation_ = Animation{};
}

void AnimationClipDocument::ReplaceWith(const Animation& animation) {
    animation_ = animation;
}

void AnimationClipDocument::SetDuration(float duration) {
    animation_.duration = (duration < 0.0f) ? 0.0f : duration;
}

bool AnimationClipDocument::LoadFromJson(const std::string& filepath) {
    Animation loaded;
    if (!AnimationJsonSerializer::LoadFromJson(filepath, loaded)) {
        return false;
    }

    animation_ = loaded;
    return true;
}

bool AnimationClipDocument::SaveToJson(const std::string& filepath) const {
    return AnimationJsonSerializer::SaveToJson(filepath, animation_);
}

void AnimationClipDocument::PrepareForExport(const Model::Skeleton& skeleton) {
    for (const auto& joint : skeleton.joints) {
        auto& na = animation_.nodeAnimations[joint.name];

        if (na.translate.keyframes.empty()) {
            na.translate.keyframes.push_back({ 0.0f, joint.transform.translate });
        }

        if (na.rotate.keyframes.empty()) {
            na.rotate.keyframes.push_back({ 0.0f, joint.transform.rotate });
        }

        if (na.scale.keyframes.empty()) {
            Vector3 initScale = joint.transform.scale;
            if (initScale.x == 0.0f) {
                initScale = { 1.0f, 1.0f, 1.0f };
            }
            na.scale.keyframes.push_back({ 0.0f, initScale });
        }

        if (na.translate.keyframes.size() == 1) {
            na.translate.keyframes.push_back({ animation_.duration, na.translate.keyframes[0].value });
        }

        if (na.rotate.keyframes.size() == 1) {
            na.rotate.keyframes.push_back({ animation_.duration, na.rotate.keyframes[0].value });
        }

        if (na.scale.keyframes.size() == 1) {
            na.scale.keyframes.push_back({ animation_.duration, na.scale.keyframes[0].value });
        }

        SortKeyframes_(na);
    }
}

void AnimationClipDocument::SortKeyframes_(NodeAnimation& nodeAnimation) {
    std::sort(
        nodeAnimation.translate.keyframes.begin(),
        nodeAnimation.translate.keyframes.end(),
        [](const KeyframeVector3& a, const KeyframeVector3& b) {
            return a.time < b.time;
        });

    std::sort(
        nodeAnimation.rotate.keyframes.begin(),
        nodeAnimation.rotate.keyframes.end(),
        [](const KeyframeQuaternion& a, const KeyframeQuaternion& b) {
            return a.time < b.time;
        });

    std::sort(
        nodeAnimation.scale.keyframes.begin(),
        nodeAnimation.scale.keyframes.end(),
        [](const KeyframeVector3& a, const KeyframeVector3& b) {
            return a.time < b.time;
        });
}
