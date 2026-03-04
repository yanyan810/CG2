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