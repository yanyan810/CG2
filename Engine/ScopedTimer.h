#pragma once
#include <Windows.h>
#include <chrono>
#include <string>

class ScopedTimer {
public:
    explicit ScopedTimer(const char* name)
        : name_(name), start_(std::chrono::high_resolution_clock::now()) {
    }

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();

        std::string s = std::string("[TIME] ") + name_ + ": " + std::to_string(us) + " us\n";
        OutputDebugStringA(s.c_str());
    }

private:
    const char* name_;
    std::chrono::high_resolution_clock::time_point start_;
};