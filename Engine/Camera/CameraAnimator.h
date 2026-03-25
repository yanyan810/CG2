#pragma once
#include <vector>
#include <string>
#include "Vector3.h"
#include "Camera.h"
#include "Input.h"

struct CameraKeyframe {
    float time;
    Vector3 pos;
    Vector3 rot;
};

class CameraAnimator {
public:
    void Initialize(Camera* camera, Input* input);
    void Initialize(Camera* camera);
    bool LoadFromJson(const std::string& filepath);

    // JSONに保存する関数
    void SaveToJson(const std::string& filepath);

    // 毎フレーム呼んでカメラを動かす
    bool Update(float dt);

    // デバッグ・エディタ用のUI
    void DrawImGui();
    // エディットモード中かどうかを外から知るための関数
    bool IsEditing() const { return !isPlaying_; }

	//ループするかどうか
    void SetLoop(bool value) { isLoop_ = value; }
    bool GetLoop() const { return isLoop_; }

	// 再生中かどうか
    void SetPlaying(bool value) { isPlaying_ = value; }
    bool GetPlaying() const { return isPlaying_; }

private:
    Camera* camera_ = nullptr;
    Input* input_ = nullptr;
    std::vector<CameraKeyframe> keyframes_;

    bool isLoop_ = true;
    bool isPlaying_ = true;
    float currentTime_ = 0.0f;
    float maxTime_ = 0.0f;

    char saveFilepath_[256] = "resources/camera/camera_idle_1.json";
    float newKeyframeTime_ = 0.0f;

    // 2つのベクトルを滑らかに繋ぐ計算
    Vector3 Lerp(const Vector3& a, const Vector3& b, float t) const;
};