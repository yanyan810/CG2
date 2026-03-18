#pragma once
#include "AABB.h"
#include "Matrix4x4.h"
#include "Mesh.h"
#include "Quaternion.h"

#include <numbers>
#include <algorithm>
#include <cmath>



static Vector4 MulRowVec4Mat4(const Vector4& v, const Matrix4x4& m)
{
    Vector4 o{};
    o.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + v.w * m.m[3][0];
    o.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + v.w * m.m[3][1];
    o.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + v.w * m.m[3][2];
    o.w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + v.w * m.m[3][3];
    return o;
}

static Vector2 WorldToScreen(const Vector3& pos, const Matrix4x4& vp, float screenW, float screenH, bool& outIsFront)
{
    // 行列と座標の掛け算（クリップ座標系への変換）
    Vector4 clip{};
    clip.x = pos.x * vp.m[0][0] + pos.y * vp.m[1][0] + pos.z * vp.m[2][0] + 1.0f * vp.m[3][0];
    clip.y = pos.x * vp.m[0][1] + pos.y * vp.m[1][1] + pos.z * vp.m[2][1] + 1.0f * vp.m[3][1];
    clip.z = pos.x * vp.m[0][2] + pos.y * vp.m[1][2] + pos.z * vp.m[2][2] + 1.0f * vp.m[3][2];
    clip.w = pos.x * vp.m[0][3] + pos.y * vp.m[1][3] + pos.z * vp.m[2][3] + 1.0f * vp.m[3][3];

    // カメラの前方にいるか（w > 0）を判定
    if (clip.w > 0.0f) {
        outIsFront = true;
        // NDC（正規化デバイス座標系）への変換
        float ndcX = clip.x / clip.w;
        float ndcY = clip.y / clip.w;
        // スクリーン座標系（ピクセル）への変換
        float sx = (ndcX + 1.0f) * 0.5f * screenW;
        float sy = (1.0f - ndcY) * 0.5f * screenH;
        return { sx, sy };
    }

    outIsFront = false;
    return { 0.0f, 0.0f };
}