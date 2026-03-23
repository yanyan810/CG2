#pragma once
#include <vector>
#include <string>
#include "Vector3.h"
#include "Camera.h"

struct CameraKeyframe {
    float time;
    Vector3 pos;
    Vector3 rot;
};

class CameraAnimator {
public:
    void Initialize(Camera* camera);
    bool LoadFromJson(const std::string& filepath);

    // 毎フレーム呼んでカメラを動かす
    bool Update(float dt);

    // デバッグ・エディタ用のUI
    void DrawImGui();

private:
    Camera* camera_ = nullptr;
    std::vector<CameraKeyframe> keyframes_;

    bool isLoop_ = true;
    bool isPlaying_ = true;
    float currentTime_ = 0.0f;
    float maxTime_ = 0.0f;

    // 2つのベクトルを滑らかに繋ぐ計算
    Vector3 Lerp(const Vector3& a, const Vector3& b, float t) const;
};