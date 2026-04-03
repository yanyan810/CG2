#pragma once
#include <string>
#include "Animation.h"
#include "Model.h"

class AnimationClipDocument {
public:
    AnimationClipDocument() = default;

    void Clear();
    void ReplaceWith(const Animation& animation);

    Animation& GetAnimation() { return animation_; }
    const Animation& GetAnimation() const { return animation_; }

    float GetDuration() const { return animation_.duration; }
    void SetDuration(float duration);

    bool HasValidClip() const { return animation_.duration > 0.0f; }

    bool LoadFromJson(const std::string& filepath);
    bool SaveToJson(const std::string& filepath) const;

    void PrepareForExport(const Model::Skeleton& skeleton);

private:
    static void SortKeyframes_(NodeAnimation& nodeAnimation);

private:
    Animation animation_{};
};
