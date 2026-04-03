#pragma once
#include <string>
#include "Animation.h"

class AnimationJsonSerializer {
public:
    static bool LoadFromJson(const std::string& filepath, Animation& outAnimation);
    static bool SaveToJson(const std::string& filepath, const Animation& animation);
};
