#include "AnimationJsonSerializer.h"

#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

bool AnimationJsonSerializer::LoadFromJson(const std::string& filepath, Animation& outAnimation) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    json j;
    file >> j;

    Animation loaded;
    loaded.duration = j.value("duration", 0.0f);

    if (j.contains("nodeAnimations") && j["nodeAnimations"].is_object()) {
        for (auto& [boneName, jNode] : j["nodeAnimations"].items()) {
            auto& na = loaded.nodeAnimations[boneName];

            if (jNode.contains("translate") && jNode["translate"].is_array()) {
                for (const auto& jKey : jNode["translate"]) {
                    Vector3 v{};
                    v.x = jKey["value"][0].get<float>();
                    v.y = jKey["value"][1].get<float>();
                    v.z = jKey["value"][2].get<float>();
                    na.translate.keyframes.push_back({ jKey["time"].get<float>(), v });
                }
            }

            if (jNode.contains("rotate") && jNode["rotate"].is_array()) {
                for (const auto& jKey : jNode["rotate"]) {
                    Quaternion q{};
                    q.x = jKey["value"][0].get<float>();
                    q.y = jKey["value"][1].get<float>();
                    q.z = jKey["value"][2].get<float>();
                    q.w = jKey["value"][3].get<float>();
                    na.rotate.keyframes.push_back({ jKey["time"].get<float>(), q });
                }
            }

            if (jNode.contains("scale") && jNode["scale"].is_array()) {
                for (const auto& jKey : jNode["scale"]) {
                    Vector3 s{};
                    s.x = jKey["value"][0].get<float>();
                    s.y = jKey["value"][1].get<float>();
                    s.z = jKey["value"][2].get<float>();

                    if (s.x == 0.0f && s.y == 0.0f && s.z == 0.0f) {
                        s = { 1.0f, 1.0f, 1.0f };
                    }

                    na.scale.keyframes.push_back({ jKey["time"].get<float>(), s });
                }
            }
        }
    }

    outAnimation = loaded;
    return true;
}

bool AnimationJsonSerializer::SaveToJson(const std::string& filepath, const Animation& animation) {
    json j;
    j["duration"] = animation.duration;
    j["nodeAnimations"] = json::object();

    for (const auto& pair : animation.nodeAnimations) {
        const std::string& boneName = pair.first;
        const NodeAnimation& na = pair.second;

        if (na.translate.keyframes.empty() &&
            na.rotate.keyframes.empty() &&
            na.scale.keyframes.empty()) {
            continue;
        }

        json jNode;

        json jTrans = json::array();
        for (const auto& k : na.translate.keyframes) {
            jTrans.push_back({
                { "time", k.time },
                { "value", { k.value.x, k.value.y, k.value.z } }
                });
        }
        jNode["translate"] = jTrans;

        json jRot = json::array();
        for (const auto& k : na.rotate.keyframes) {
            jRot.push_back({
                { "time", k.time },
                { "value", { k.value.x, k.value.y, k.value.z, k.value.w } }
                });
        }
        jNode["rotate"] = jRot;

        json jScale = json::array();
        for (const auto& k : na.scale.keyframes) {
            jScale.push_back({
                { "time", k.time },
                { "value", { k.value.x, k.value.y, k.value.z } }
                });
        }
        jNode["scale"] = jScale;

        j["nodeAnimations"][boneName] = jNode;
    }

    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    file << j.dump(4);
    return true;
}
