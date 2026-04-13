#include "CameraAnimator.h"
#include "Matrix4x4.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include <fstream>
#include <algorithm>
#include <cmath>
#include <windows.h> // OutputDebugStringAのため
#include <stdio.h>   // sprintf_sのため
#include <nlohmann/json.hpp> 
using json = nlohmann::json;

void CameraAnimator::Initialize(Camera* camera, Input* input) {
    camera_ = camera;
    input_ = input;
    currentTime_ = 0.0f;
}

void CameraAnimator::Initialize(Camera* camera) {
    Initialize(camera, nullptr);
}

void CameraAnimator::SetCurrentTime(float value) {
    if (value < 0.0f) {
        currentTime_ = 0.0f;
        return;
    }

    currentTime_ = maxTime_ > 0.0f ? std::min(value, maxTime_) : value;
}

void CameraAnimator::SetMaxTime(float value) {
    maxTime_ = std::max(value, 0.1f);
    if (currentTime_ > maxTime_) {
        currentTime_ = maxTime_;
    }
}

void CameraAnimator::SampleAtTime(float time) {
    if (!camera_) {
        return;
    }

    SetCurrentTime(time);

    if (keyframes_.empty()) {
        return;
    }

    if (keyframes_.size() == 1 || currentTime_ <= keyframes_.front().time) {
        camera_->SetTranslate(keyframes_.front().pos);
        camera_->SetRotate(keyframes_.front().rot);
        camera_->SetFovY(keyframes_.front().fov);
        return;
    }

    if (currentTime_ >= keyframes_.back().time) {
        camera_->SetTranslate(keyframes_.back().pos);
        camera_->SetRotate(keyframes_.back().rot);
        camera_->SetFovY(keyframes_.back().fov);
        return;
    }

    for (size_t i = 0; i + 1 < keyframes_.size(); ++i) {
        if (currentTime_ < keyframes_[i].time || currentTime_ > keyframes_[i + 1].time) {
            continue;
        }

        float duration = keyframes_[i + 1].time - keyframes_[i].time;
        if (duration <= 0.0f) {
            camera_->SetTranslate(keyframes_[i].pos);
            camera_->SetRotate(keyframes_[i].rot);
            camera_->SetFovY(keyframes_[i].fov);
            return;
        }

        float t = (currentTime_ - keyframes_[i].time) / duration;
        camera_->SetTranslate(Lerp(keyframes_[i].pos, keyframes_[i + 1].pos, t));
        camera_->SetRotate(Lerp(keyframes_[i].rot, keyframes_[i + 1].rot, t));
        camera_->SetFovY(LerpFloat(keyframes_[i].fov, keyframes_[i + 1].fov, t));
        return;
    }
}

void CameraAnimator::AddOrUpdateKeyframe(float time) {
    if (!camera_) {
        return;
    }

    CameraKeyframe keyframe{};
    keyframe.time = time;
    keyframe.pos = camera_->GetTranslate();
    keyframe.rot = camera_->GetRotate();
    keyframe.fov = camera_->GetFovY();

    auto it = std::find_if(
        keyframes_.begin(),
        keyframes_.end(),
        [time](const CameraKeyframe& key) {
            return std::abs(key.time - time) < 0.001f;
        });

    if (it != keyframes_.end()) {
        *it = keyframe;
    } else {
        keyframes_.push_back(keyframe);
        std::sort(
            keyframes_.begin(),
            keyframes_.end(),
            [](const CameraKeyframe& a, const CameraKeyframe& b) {
                return a.time < b.time;
            });
    }

    maxTime_ = keyframes_.empty() ? std::max(maxTime_, 0.1f) : std::max(keyframes_.back().time, 0.1f);
}

void CameraAnimator::DeleteKeyframeAt(float time) {
    keyframes_.erase(
        std::remove_if(
            keyframes_.begin(),
            keyframes_.end(),
            [time](const CameraKeyframe& key) {
                return std::abs(key.time - time) < 0.001f;
            }),
        keyframes_.end());

    maxTime_ = keyframes_.empty() ? std::max(maxTime_, 0.1f) : std::max(keyframes_.back().time, 0.1f);
    if (currentTime_ > maxTime_) {
        currentTime_ = maxTime_;
    }
}

Vector3 CameraAnimator::Lerp(const Vector3& a, const Vector3& b, float t) const {
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

float CameraAnimator::LerpFloat(float a, float b, float t) const {
    return a + (b - a) * t;
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
            frame.fov = k.contains("fov") ? k["fov"].get<float>() : (camera_ ? camera_->GetFovY() : 0.45f);

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
    if (!camera_ || keyframes_.empty()) return false;

#ifdef USE_IMGUI

    // エディットモード（停止中
    if (!isPlaying_) {
        if (input_) {
            Vector3 pos = camera_->GetTranslate();
            Vector3 rot = camera_->GetRotate();
            float moveSpeed = 0.5f;
            float rotSpeed = 0.005f;

            // 1. 右クリックドラッグで視点回転
            if (input_->IsMousePressed(1)) { // 1 = 右クリック
                POINT delta = input_->GetMouseDelta();
                rot.x += delta.y * rotSpeed; // ピッチ（上下）
                rot.y += delta.x * rotSpeed; // ヨー（左右）
            }

            // 2. WASDで移動
            Vector3 localMove = { 0.0f, 0.0f, 0.0f };
            if (input_->IsKeyPressed(DIK_W)) localMove.z += moveSpeed;
            if (input_->IsKeyPressed(DIK_S)) localMove.z -= moveSpeed;
            if (input_->IsKeyPressed(DIK_A)) localMove.x -= moveSpeed;
            if (input_->IsKeyPressed(DIK_D)) localMove.x += moveSpeed;
            if (input_->IsKeyPressed(DIK_SPACE)) localMove.y += moveSpeed;
            if (input_->IsKeyPressed(DIK_LSHIFT)) localMove.y -= moveSpeed;

            // カメラのヨー（左右回転）とピッチ（上下回転）から、正面と右のベクトルを計算
            float yaw = rot.y;
            float pitch = rot.x;

            Vector3 forward = { std::sin(yaw) * std::cos(pitch), -std::sin(pitch), std::cos(yaw) * std::cos(pitch) };
            Vector3 right = { std::cos(yaw), 0.0f, -std::sin(yaw) };
            Vector3 up = { std::sin(yaw) * std::sin(pitch), std::cos(pitch), std::cos(yaw) * std::sin(pitch) };

            // ワールド座標の移動量に加算
            pos.x += localMove.z * forward.x + localMove.x * right.x + localMove.y * up.x;
            pos.y += localMove.z * forward.y + localMove.x * right.y + localMove.y * up.y;
            pos.z += localMove.z * forward.z + localMove.x * right.z + localMove.y * up.z;

            camera_->SetTranslate(pos);
            camera_->SetRotate(rot);
        }
        return false;
    }
#endif

    // 通常の再生処理
    currentTime_ += dt;
    bool isFinished = false;

    if (currentTime_ >= maxTime_) {
        isFinished = true;
        if (isLoop_) {
            currentTime_ = fmod(currentTime_, maxTime_);
        } else {
            currentTime_ = maxTime_;
        }
    }

    for (size_t i = 0; i < keyframes_.size() - 1; ++i) {
        if (currentTime_ >= keyframes_[i].time && currentTime_ <= keyframes_[i + 1].time) {
            float duration = keyframes_[i + 1].time - keyframes_[i].time;
            if (duration <= 0.0f) break;

            float t = (currentTime_ - keyframes_[i].time) / duration;
            Vector3 currentPos = Lerp(keyframes_[i].pos, keyframes_[i + 1].pos, t);
            Vector3 currentRot = Lerp(keyframes_[i].rot, keyframes_[i + 1].rot, t);
            float currentFov = LerpFloat(keyframes_[i].fov, keyframes_[i + 1].fov, t);

            camera_->SetTranslate(currentPos);
            camera_->SetRotate(currentRot);
            camera_->SetFovY(currentFov);
            break;
        }
    }

    return isFinished;
}

void CameraAnimator::SaveToJson(const std::string& filepath) {
    json j;
    j["loop"] = isLoop_;

    json jKeyframes = json::array();
    for (const auto& k : keyframes_) {
        json kf;
        kf["time"] = k.time;
        kf["pos"] = { k.pos.x, k.pos.y, k.pos.z };
        kf["rot"] = { k.rot.x, k.rot.y, k.rot.z };
        kf["fov"] = k.fov;
        jKeyframes.push_back(kf);
    }
    j["keyframes"] = jKeyframes;

    // ファイルに書き出し
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
        OutputDebugStringA((">>> SUCCESS: Saved Camera to " + filepath + "\n").c_str());
    } else {
        OutputDebugStringA((">>> ERROR: Failed to save " + filepath + "\n").c_str());
    }
}

#ifdef USE_IMGUI

void CameraAnimator::DrawImGui() {
    if (!camera_) return;

    ImGui::Text("=== Camera Animator Editor ===");
    ImGui::Checkbox("Play Animation", &isPlaying_);
    ImGui::Checkbox("Loop", &isLoop_);

    ImGui::Separator();

    if (isPlaying_) {
        // 再生中は進捗スライダーを表示
        ImGui::SliderFloat("Time", &currentTime_, 0.0f, maxTime_);
    } else {
        // ==========================================
        // 停止中：自由移動 ＆ キーフレーム追加モード
        // ==========================================
        ImGui::Text("[ Edit Mode ]");

        // 1. カメラを自由に動かせるスライダー
        Vector3 cPos = camera_->GetTranslate();
        Vector3 cRot = camera_->GetRotate();
        float cFov = camera_->GetFovY();
        if (ImGui::DragFloat3("Camera Pos", &cPos.x, 0.1f)) camera_->SetTranslate(cPos);
        if (ImGui::DragFloat3("Camera Rot", &cRot.x, 0.01f)) camera_->SetRotate(cRot);
        if (ImGui::DragFloat("Camera Fov", &cFov, 0.001f, 0.1f, 3.0f)) camera_->SetFovY(cFov);

        ImGui::Spacing();

        // 2. 現在のカメラ位置を新しいキーフレームとして登録
        ImGui::InputFloat("Set Time (sec)", &newKeyframeTime_);
        if (ImGui::Button("Add Keyframe Here!")) {
            CameraKeyframe kf{};
            kf.time = newKeyframeTime_;
            kf.pos = cPos;
            kf.rot = cRot;
            kf.fov = camera_->GetFovY();
            keyframes_.push_back(kf);

            // 時間順に並び替える
            std::sort(keyframes_.begin(), keyframes_.end(), [](const CameraKeyframe& a, const CameraKeyframe& b) {
                return a.time < b.time;
                });

            // maxTimeを更新
            if (!keyframes_.empty()) maxTime_ = keyframes_.back().time;

            // 次の打ち込みやすいように時間を+2秒しておく
            newKeyframeTime_ += 2.0f;
        }
    }

    ImGui::Separator();

    // ==========================================
    // セーブ機能
    // ==========================================
    ImGui::InputText("Save Path", saveFilepath_, sizeof(saveFilepath_));
    if (ImGui::Button("SAVE TO JSON", ImVec2(150, 30))) {
        SaveToJson(saveFilepath_);
    }

    ImGui::Separator();

    // ==========================================
    // 登録されているキーフレームの一覧と削除
    // ==========================================
    if (ImGui::TreeNode("Keyframes List")) {
        for (size_t i = 0; i < keyframes_.size(); ++i) {
            ImGui::PushID((int)i);
            ImGui::Text("Keyframe %d (%.1f sec)", i, keyframes_[i].time);
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                keyframes_.erase(keyframes_.begin() + i);
                if (!keyframes_.empty()) maxTime_ = keyframes_.back().time;
                else maxTime_ = 0.0f;
                i--; // インデックスのズレを補正
            } else {
                // 微調整用のスライダー
                ImGui::DragFloat3("Pos", &keyframes_[i].pos.x, 0.1f);
                ImGui::DragFloat3("Rot", &keyframes_[i].rot.x, 0.01f);
            }
            ImGui::PopID();
            ImGui::Separator();
        }
        ImGui::TreePop();
    }
    ImGui::Text("==============================");
}
#endif
