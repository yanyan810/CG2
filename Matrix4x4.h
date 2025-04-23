// Matrix4x4.h
#pragma once
#include <cmath>
#include "Vector3.h"

class Matrix4x4 {
public:
    float m[4][4];

    Matrix4x4();

    static Matrix4x4 MakeIdentity4x4();
    static Matrix4x4 Translation(const Vector3& translation);
    static Matrix4x4 Scale(const Vector3& scale);
    static Matrix4x4 RotateY(float angleRad);
    static Matrix4x4 RotateXYZ(float angleX, float angleY, float angleZ);
    static Matrix4x4 PerspectiveFov(float fovY, float aspect, float nearZ, float farZ);

    static Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
    static Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translation);
    static Matrix4x4 Inverse(const Matrix4x4& m);
};