#pragma once
#include <cassert>
#include <vector>
#include <cmath>
#include <algorithm>

#include "MathStruct.h"
#include "Animation.h"   // Keyframe / Curve / NodeAnimation / Animation

// ---- Vector3 lerp ----
static inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return Vector3{
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

// =============================
// 任意時刻の値を取得（Vector3）
// =============================
static inline Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time) {
    assert(!keyframes.empty());

    // 例外処理（スライド通り）
    if (keyframes.size() == 1 || time <= keyframes.front().time) {
        return keyframes.front().value;
    }
    if (time >= keyframes.back().time) {
        return keyframes.back().value;
    }

    // time を挟む区間を探す（線形探索：まずはこれでOK）
    for (size_t i = 0; i + 1 < keyframes.size(); ++i) {
        const auto& k0 = keyframes[i];
        const auto& k1 = keyframes[i + 1];
        if (k0.time <= time && time <= k1.time) {
            const float t = (time - k0.time) / (k1.time - k0.time);
            return Lerp(k0.value, k1.value, t);
        }
    }

    // ここには基本来ない
    return keyframes.back().value;
}

// =============================
// 任意時刻の値を取得（Quaternion）
// =============================
static inline Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time) {
    assert(!keyframes.empty());

    if (keyframes.size() == 1 || time <= keyframes.front().time) {
        return Normalize(keyframes.front().value);
    }
    if (time >= keyframes.back().time) {
        return Normalize(keyframes.back().value);
    }

    for (size_t i = 0; i + 1 < keyframes.size(); ++i) {
        const auto& k0 = keyframes[i];
        const auto& k1 = keyframes[i + 1];
        if (k0.time <= time && time <= k1.time) {
            const float t = (time - k0.time) / (k1.time - k0.time);
            return Slerp(k0.value, k1.value, t);
        }
    }

    return Normalize(keyframes.back().value);
}

// =============================
// Quaternion -> 回転行列（row-vector/translationはm[3][0..2]）
// =============================
static inline Matrix4x4 MakeRotateMatrix(const Quaternion& qRaw) {
    Quaternion q = Normalize(qRaw);

    const float x = q.x, y = q.y, z = q.z, w = q.w;

    const float xx = x * x, yy = y * y, zz = z * z;
    const float xy = x * y, xz = x * z, yz = y * z;
    const float wx = w * x, wy = w * y, wz = w * z;

    Matrix4x4 m = Matrix4x4::MakeIdentity4x4();

    // 「基底ベクトルを列に詰める」配置（あなたの MulRowVec4Mat4 と整合）
    m.m[0][0] = 1.0f - 2.0f * (yy + zz);
    m.m[1][0] = 2.0f * (xy + wz);
    m.m[2][0] = 2.0f * (xz - wy);

    m.m[0][1] = 2.0f * (xy - wz);
    m.m[1][1] = 1.0f - 2.0f * (xx + zz);
    m.m[2][1] = 2.0f * (yz + wx);

    m.m[0][2] = 2.0f * (xz + wy);
    m.m[1][2] = 2.0f * (yz - wx);
    m.m[2][2] = 1.0f - 2.0f * (xx + yy);

    return m;
}

// =============================
// MakeAffineMatrix(scale, rotate, translate)
// ＝ 回転行列の各列にscaleを掛けて、平行移動をm[3][0..2]へ
// =============================
static inline Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate) {
    Matrix4x4 r = MakeRotateMatrix(rotate);
    Matrix4x4 m = Matrix4x4::MakeIdentity4x4();

    // col0 *= scale.x
    m.m[0][0] = r.m[0][0] * scale.x;
    m.m[1][0] = r.m[1][0] * scale.x;
    m.m[2][0] = r.m[2][0] * scale.x;

    // col1 *= scale.y
    m.m[0][1] = r.m[0][1] * scale.y;
    m.m[1][1] = r.m[1][1] * scale.y;
    m.m[2][1] = r.m[2][1] * scale.y;

    // col2 *= scale.z
    m.m[0][2] = r.m[0][2] * scale.z;
    m.m[1][2] = r.m[1][2] * scale.z;
    m.m[2][2] = r.m[2][2] * scale.z;

    // translation（row-vector想定：m[3][0..2]）
    m.m[3][0] = translate.x;
    m.m[3][1] = translate.y;
    m.m[3][2] = translate.z;

    return m;
}
