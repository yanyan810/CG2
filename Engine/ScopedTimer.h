#pragma once
#include <Windows.h>
#include <chrono>
#include <string>

// ============================================================
// ScopedTimer - スコープ終了時に経過時間をデバッグ出力
//
// 使い方:
//   { ScopedTimer t("label"); ... }           // 常に出力（マイクロ秒）
//   { ScopedTimer t("label", 5.0f); ... }     // 5ms超えたときだけ出力
// ============================================================
class ScopedTimer {
public:
    // thresholdMs = 0 なら常に出力、>0 なら超えたときだけ出力
    explicit ScopedTimer(const char* name, float thresholdMs = 0.0f)
        : name_(name)
        , thresholdMs_(thresholdMs)
        , start_(std::chrono::high_resolution_clock::now())
    {}

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        float ms = std::chrono::duration<float, std::milli>(end - start_).count();

        if (thresholdMs_ > 0.0f && ms < thresholdMs_) {
            return; // 閾値未満はスキップ
        }

        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "[ScopedTimer] %-42s : %7.3f ms%s\n",
            name_,
            ms,
            (thresholdMs_ > 0.0f ? "  <<< SPIKE >>>" : "")
        );
        OutputDebugStringA(buf);
    }

private:
    const char* name_;
    float thresholdMs_;
    std::chrono::high_resolution_clock::time_point start_;
};