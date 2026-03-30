#pragma once
#include <cmath>
#include <algorithm>

struct Quaternion {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
};

struct QuaternionTransform
{
    Vector3 scale{ 1.0f, 1.0f, 1.0f };
    Quaternion rotate{}; // (0,0,0,1)
    Vector3 translate{ 0.0f, 0.0f, 0.0f };
};

// 回転行列からクォータニオンを抽出する関数
static inline Quaternion MatrixToQuaternion(const Matrix4x4& m) {
    Quaternion q;
    float trace = m.m[0][0] + m.m[1][1] + m.m[2][2];
    if (trace > 0.0f) {
        float s = 0.5f / std::sqrt(trace + 1.0f);
        q.w = 0.25f / s;
        q.x = (m.m[1][2] - m.m[2][1]) * s;
        q.y = (m.m[2][0] - m.m[0][2]) * s;
        q.z = (m.m[0][1] - m.m[1][0]) * s;
    } else {
        if (m.m[0][0] > m.m[1][1] && m.m[0][0] > m.m[2][2]) {
            float s = 2.0f * std::sqrt(1.0f + m.m[0][0] - m.m[1][1] - m.m[2][2]);
            q.w = (m.m[1][2] - m.m[2][1]) / s;  q.x = 0.25f * s;
            q.y = (m.m[0][1] + m.m[1][0]) / s;  q.z = (m.m[2][0] + m.m[0][2]) / s;
        } else if (m.m[1][1] > m.m[2][2]) {
            float s = 2.0f * std::sqrt(1.0f + m.m[1][1] - m.m[0][0] - m.m[2][2]);
            q.w = (m.m[2][0] - m.m[0][2]) / s;  q.x = (m.m[0][1] + m.m[1][0]) / s;
            q.y = 0.25f * s;                    q.z = (m.m[1][2] + m.m[2][1]) / s;
        } else {
            float s = 2.0f * std::sqrt(1.0f + m.m[2][2] - m.m[0][0] - m.m[1][1]);
            q.w = (m.m[0][1] - m.m[1][0]) / s;  q.x = (m.m[2][0] + m.m[0][2]) / s;
            q.y = (m.m[1][2] + m.m[2][1]) / s;  q.z = 0.25f * s;
        }
    }
    float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len > 0.0001f) { q.x /= len; q.y /= len; q.z /= len; q.w /= len; } else { q = { 0.0f, 0.0f, 0.0f, 1.0f }; }
    return q;
}

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

static Matrix4x4 MakeRotateMatrixFromQuaternion_Row(const Quaternion& qRaw) {
    Quaternion q = Normalize(qRaw);

    const float x = q.x;
    const float y = q.y;
    const float z = q.z;
    const float w = q.w;

    Matrix4x4 m = Matrix4x4::MakeIdentity4x4();

    // row-vector 用（列ベクトル式の転置）
    m.m[0][0] = 1.0f - 2.0f * (y * y + z * z);
    m.m[0][1] = 2.0f * (x * y + z * w);
    m.m[0][2] = 2.0f * (x * z - y * w);

    m.m[1][0] = 2.0f * (x * y - z * w);
    m.m[1][1] = 1.0f - 2.0f * (x * x + z * z);
    m.m[1][2] = 2.0f * (y * z + x * w);

    m.m[2][0] = 2.0f * (x * z + y * w);
    m.m[2][1] = 2.0f * (y * z - x * w);
    m.m[2][2] = 1.0f - 2.0f * (x * x + y * y);

    return m;
}

static Matrix4x4 MakeAffineMatrix(
    const Vector3& scale,
    const Quaternion& rotate,
    const Vector3& translate) {
    // Scale
    Matrix4x4 S = Matrix4x4::MakeIdentity4x4();
    S.m[0][0] = scale.x;
    S.m[1][1] = scale.y;
    S.m[2][2] = scale.z;

    // Rotate
    Matrix4x4 R = MakeRotateMatrixFromQuaternion_Row(rotate);

    // S * R（row-vector想定）
    Matrix4x4 M = Matrix4x4::Multiply(S, R);

    // Translate（row-vector：最後の行）
    M.m[3][0] = translate.x;
    M.m[3][1] = translate.y;
    M.m[3][2] = translate.z;
    M.m[3][3] = 1.0f;

    return M;
}
