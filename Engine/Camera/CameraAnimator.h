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
    float fov;
    bool hasWorldMatrix = false;
    Matrix4x4 worldMatrix{};
};

class CameraAnimator {
public:
    struct StateSnapshot {
        std::vector<CameraKeyframe> keyframes;
        bool isLoop = true;
        bool isPlaying = true;
        bool isDirty = false;
        float currentTime = 0.0f;
        float maxTime = 0.0f;
        Vector3 cameraPos{ 0.0f, 0.0f, 0.0f };
        Vector3 cameraRot{ 0.0f, 0.0f, 0.0f };
        float cameraFov = 0.45f;
    };

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
    float GetCurrentTime() const { return currentTime_; }
    float GetMaxTime() const { return maxTime_; }
    void SetCurrentTime(float value);
    void SetMaxTime(float value);
    const std::vector<CameraKeyframe>& GetKeyframes() const { return keyframes_; }
    void SampleAtTime(float time);
    void AddOrUpdateKeyframe(float time);
    void DeleteKeyframeAt(float time);
    char* GetSaveFilepathBuffer() { return saveFilepath_; }
    const char* GetSaveFilepath() const { return saveFilepath_; }
    StateSnapshot CaptureState() const;
    void RestoreState(const StateSnapshot& snapshot);
    bool IsDirty() const { return isDirty_; }
    void SetDirty(bool value) { isDirty_ = value; }

private:
    Camera* camera_ = nullptr;
    Input* input_ = nullptr;
    std::vector<CameraKeyframe> keyframes_;

    bool isLoop_ = true;
    bool isPlaying_ = true;
    bool isDirty_ = false;
    float currentTime_ = 0.0f;
    float maxTime_ = 0.0f;

    char saveFilepath_[256] = "resources/camera/camera_idle_1.json";
    float newKeyframeTime_ = 0.0f;

    // 2つのベクトルを滑らかに繋ぐ計算
    Vector3 Lerp(const Vector3& a, const Vector3& b, float t) const;
    float LerpFloat(float a, float b, float t) const;
};
