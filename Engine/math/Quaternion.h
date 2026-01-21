#pragma once
#include <cmath>
#include <algorithm>

struct Quaternion {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
};

static inline float Dot(const Quaternion& a, const Quaternion& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static inline Quaternion Normalize(const Quaternion& q) {
    const float len2 = Dot(q, q);
    if (len2 <= 0.0f) { return Quaternion{}; }
    const float inv = 1.0f / std::sqrt(len2);
    Quaternion out{};
    out.x = q.x * inv;
    out.y = q.y * inv;
    out.z = q.z * inv;
    out.w = q.w * inv;
    return out;
}

static inline Quaternion Negate(const Quaternion& q) {
    Quaternion out{};
    out.x = -q.x; out.y = -q.y; out.z = -q.z; out.w = -q.w;
    return out;
}

// 安定版Slerp（角度が小さい時はnlerp）
static inline Quaternion Slerp(const Quaternion& aRaw, const Quaternion& bRaw, float t) {
    t = std::clamp(t, 0.0f, 1.0f);

    Quaternion a = Normalize(aRaw);
    Quaternion b = Normalize(bRaw);

    float cosTheta = Dot(a, b);

    // 反対向きなら b を反転（最短経路）
    if (cosTheta < 0.0f) {
        b = Negate(b);
        cosTheta = -cosTheta;
    }

    // ほぼ同方向：nlerp
    if (cosTheta > 0.9995f) {
        Quaternion out{};
        out.x = a.x + (b.x - a.x) * t;
        out.y = a.y + (b.y - a.y) * t;
        out.z = a.z + (b.z - a.z) * t;
        out.w = a.w + (b.w - a.w) * t;
        return Normalize(out);
    }

    // slerp
    const float theta = std::acos(std::clamp(cosTheta, -1.0f, 1.0f));
    const float sinTheta = std::sin(theta);
    const float w0 = std::sin((1.0f - t) * theta) / sinTheta;
    const float w1 = std::sin(t * theta) / sinTheta;

    Quaternion out{};
    out.x = a.x * w0 + b.x * w1;
    out.y = a.y * w0 + b.y * w1;
    out.z = a.z * w0 + b.z * w1;
    out.w = a.w * w0 + b.w * w1;
    return out;
}
