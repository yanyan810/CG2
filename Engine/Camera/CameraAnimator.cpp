#include "CameraAnimator.h"
#ifdef USE_IMGUI
#include <imgui.h>
#include <fstream>
#include <windows.h> // OutputDebugStringAのため
#include <stdio.h>   // sprintf_sのため
#include <nlohmann/json.hpp> 
using json = nlohmann::json;

void CameraAnimator::Initialize(Camera* camera) {
    camera_ = camera;
    currentTime_ = 0.0f;
}

Vector3 CameraAnimator::Lerp(const Vector3& a, const Vector3& b, float t) const {
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

bool CameraAnimator::LoadFromJson(const std::string& filepath) {
    // 1. ファイルを開く
    std::ifstream file(filepath);
    if (!file.is_open()) {
        // 見つからなかったらエラーログを出して終了
        OutputDebugStringA(("Error: Camera JSON Not Found -> " + filepath + "\n").c_str());
        return false;
    }

    // 2. JSONとして解析
    json j;
    file >> j;

    keyframes_.clear();

    currentTime_ = 0.0f;

    // 3. データを読み取る
    if (j.contains("loop")) {
        isLoop_ = j["loop"];
    }

    if (j.contains("keyframes")) {
        for (const auto& k : j["keyframes"]) {
            CameraKeyframe frame;
            frame.time = k["time"];

            // 位置 (pos) の配列 [x, y, z] を読み取る
            frame.pos.x = k["pos"][0];
            frame.pos.y = k["pos"][1];
            frame.pos.z = k["pos"][2];

            // 角度 (rot) の配列 [x, y, z] を読み取る
            frame.rot.x = k["rot"][0];
            frame.rot.y = k["rot"][1];
            frame.rot.z = k["rot"][2];

            keyframes_.push_back(frame);
        }
    }

    // 4. 最後のキーフレームの時間を maxTime_ として記録
    if (!keyframes_.empty()) {
        maxTime_ = keyframes_.back().time;
    } else {
        maxTime_ = 0.0f;
    }

    return true;
}

bool CameraAnimator::Update(float dt) {
    // カメラが無い、またはキーフレームが空っぽなら何もしない
    if (!camera_ || keyframes_.empty() || !isPlaying_) return false;

    // 時間を進める
    currentTime_ += dt;
    bool isFinished = false;
    // ループ処理
    if (currentTime_ > maxTime_) {
        isFinished = true;
        if (isLoop_) {
            currentTime_ = fmod(currentTime_, maxTime_);
        } else {
            currentTime_ = maxTime_;
        }
    }

    // 現在の時間が、どのキーフレームの間にあるかを探す
    for (size_t i = 0; i < keyframes_.size() - 1; ++i) {
        if (currentTime_ >= keyframes_[i].time && currentTime_ <= keyframes_[i + 1].time) {

            float duration = keyframes_[i + 1].time - keyframes_[i].time;
            if (duration <= 0.0f) break;

            float t = (currentTime_ - keyframes_[i].time) / duration;

            // 位置と角度を滑らかに計算
            Vector3 currentPos = Lerp(keyframes_[i].pos, keyframes_[i + 1].pos, t);
            Vector3 currentRot = Lerp(keyframes_[i].rot, keyframes_[i + 1].rot, t);

            // カメラ本体に適用！
            camera_->SetTranslate(currentPos);
            camera_->SetRotate(currentRot);
            break;
        }
    }
    return isFinished;
}
void CameraAnimator::DrawImGui() {
    ImGui::Text("--- Camera Animator ---");
    ImGui::Checkbox("Play Animation", &isPlaying_);
    ImGui::SliderFloat("Time", &currentTime_, 0.0f, maxTime_);

    // エディタ機能：現在のカメラ位置をJSON形式で出力するボタン
    if (ImGui::Button("Print Current Camera to Console")) {
        Vector3 p = camera_->GetTranslate();
        Vector3 r = camera_->GetRotate();
        char buf[256];
        sprintf_s(buf, "{ \"time\": %.1f, \"pos\": [%.2f, %.2f, %.2f], \"rot\": [%.2f, %.2f, %.2f] },\n",
            currentTime_, p.x, p.y, p.z, r.x, r.y, r.z);
        OutputDebugStringA(buf);
    }
    ImGui::Text("-----------------------");
}
#endif