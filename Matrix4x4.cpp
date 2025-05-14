// Matrix4x4.cpp
#include "Matrix4x4.h"

Matrix4x4::Matrix4x4() {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m[i][j] = 0.0f;
}

Matrix4x4 Matrix4x4::MakeIdentity4x4() {
    Matrix4x4 mat;
    for (int i = 0; i < 4; ++i)
        mat.m[i][i] = 1.0f;
    return mat;
}

Matrix4x4 Matrix4x4::Translation(const Vector3& translation) {
    Matrix4x4 mat = MakeIdentity4x4();
    mat.m[3][0] = translation.x;
    mat.m[3][1] = translation.y;
    mat.m[3][2] = translation.z;
    return mat;
}

Matrix4x4 Matrix4x4::Scale(const Vector3& scale) {
    Matrix4x4 mat = {};
    mat.m[0][0] = scale.x;
    mat.m[1][1] = scale.y;
    mat.m[2][2] = scale.z;
    mat.m[3][3] = 1.0f;
    return mat;
}

Matrix4x4 Matrix4x4::RotateY(float angleRad) {
    Matrix4x4 mat = MakeIdentity4x4();
    mat.m[0][0] = cosf(angleRad);
    mat.m[0][2] = sinf(angleRad);
    mat.m[2][0] = -sinf(angleRad);
    mat.m[2][2] = cosf(angleRad);
    return mat;
}

Matrix4x4 Matrix4x4::RotateXYZ(float angleX, float angleY, float angleZ) {
    float cx = cosf(angleX), sx = sinf(angleX);
    float cy = cosf(angleY), sy = sinf(angleY);
    float cz = cosf(angleZ), sz = sinf(angleZ);

    Matrix4x4 rx = MakeIdentity4x4();
    rx.m[1][1] = cx;
    rx.m[1][2] = sx;
    rx.m[2][1] = -sx;
    rx.m[2][2] = cx;

    Matrix4x4 ry = MakeIdentity4x4();
    ry.m[0][0] = cy;
    ry.m[0][2] = -sy;
    ry.m[2][0] = sy;
    ry.m[2][2] = cy;

    Matrix4x4 rz = MakeIdentity4x4();
    rz.m[0][0] = cz;
    rz.m[0][1] = sz;
    rz.m[1][0] = -sz;
    rz.m[1][1] = cz;

    return Multiply(Multiply(rx, ry), rz);
}

Matrix4x4 Matrix4x4::PerspectiveFov(float fovY, float aspect, float nearZ, float farZ) {
    Matrix4x4 mat = {};
    float f = 1.0f / tanf(fovY / 2.0f);
    mat.m[0][0] = f / aspect;
    mat.m[1][1] = f;
    mat.m[2][2] = farZ / (farZ - nearZ);
    mat.m[2][3] = 1.0f;
    mat.m[3][2] = -nearZ * farZ / (farZ - nearZ);
    return mat;
}

Matrix4x4 Matrix4x4::Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
    Matrix4x4 result;
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            result.m[y][x] =
                m1.m[y][0] * m2.m[0][x] +
                m1.m[y][1] * m2.m[1][x] +
                m1.m[y][2] * m2.m[2][x] +
                m1.m[y][3] * m2.m[3][x];
        }
    }
    return result;
}

Matrix4x4 Matrix4x4::MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translation) {
    Matrix4x4 s = Scale(scale);
    Matrix4x4 r = RotateXYZ(rotate.x,rotate.y,rotate.z);
    Matrix4x4 t = Translation(translation);
    return Multiply(Multiply(s, r), t);
}

Matrix4x4 Matrix4x4::Inverse(const Matrix4x4& m) {
    float det =
        m.m[0][0] * (m.m[1][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
            m.m[1][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) +
            m.m[1][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1])) -
        m.m[0][1] * (m.m[1][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
            m.m[1][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
            m.m[1][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0])) +
        m.m[0][2] * (m.m[1][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) -
            m.m[1][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
            m.m[1][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0])) -
        m.m[0][3] * (m.m[1][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]) -
            m.m[1][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]) +
            m.m[1][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]));

    float invDet = 1.0f / det;

    Matrix4x4 result;

    result.m[0][0] = invDet * (m.m[1][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
        m.m[1][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) +
        m.m[1][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]));

    result.m[0][1] = -invDet * (m.m[0][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
        m.m[0][2] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) +
        m.m[0][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]));

    result.m[0][2] = invDet * (m.m[0][1] * (m.m[1][2] * m.m[3][3] - m.m[1][3] * m.m[3][2]) -
        m.m[0][2] * (m.m[1][1] * m.m[3][3] - m.m[1][3] * m.m[3][1]) +
        m.m[0][3] * (m.m[1][1] * m.m[3][2] - m.m[1][2] * m.m[3][1]));

    result.m[0][3] = -invDet * (m.m[0][1] * (m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2]) -
        m.m[0][2] * (m.m[1][1] * m.m[2][3] - m.m[1][3] * m.m[2][1]) +
        m.m[0][3] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1]));

    result.m[1][0] = -invDet * (m.m[1][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
        m.m[1][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
        m.m[1][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]));

    result.m[1][1] = invDet * (m.m[0][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]) -
        m.m[0][2] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
        m.m[0][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]));

    result.m[1][2] = -invDet * (m.m[0][0] * (m.m[1][2] * m.m[3][3] - m.m[1][3] * m.m[3][2]) -
        m.m[0][2] * (m.m[1][0] * m.m[3][3] - m.m[1][3] * m.m[3][0]) +
        m.m[0][3] * (m.m[1][0] * m.m[3][2] - m.m[1][2] * m.m[3][0]));

    result.m[1][3] = -invDet * (m.m[0][0] * (m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2]) -
        m.m[0][2] * (m.m[1][0] * m.m[2][3] - m.m[1][3] * m.m[2][0]) +
        m.m[0][3] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0]));

    result.m[2][0] = invDet * (m.m[1][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) -
        m.m[1][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
        m.m[1][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]));

    result.m[2][1] = -invDet * (m.m[0][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]) -
        m.m[0][1] * (m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0]) +
        m.m[0][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]));

    result.m[2][2] = invDet * (m.m[0][0] * (m.m[1][1] * m.m[3][3] - m.m[1][3] * m.m[3][1]) -
        m.m[0][1] * (m.m[1][0] * m.m[3][3] - m.m[1][3] * m.m[3][0]) +
        m.m[0][3] * (m.m[1][0] * m.m[3][1] - m.m[1][1] * m.m[3][0]));

    result.m[2][3] = -invDet * (m.m[0][0] * (m.m[1][1] * m.m[2][3] - m.m[1][3] * m.m[2][1]) -
        m.m[0][1] * (m.m[1][0] * m.m[2][3] - m.m[1][3] * m.m[2][0]) +
        m.m[0][3] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0]));


    result.m[3][0] = -invDet * (m.m[1][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]) -
        m.m[1][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]) +
        m.m[1][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]));

    result.m[3][1] = invDet * (m.m[0][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]) -
        m.m[0][1] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]) +
        m.m[0][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]));

    result.m[3][2] = -invDet * (m.m[0][0] * (m.m[1][1] * m.m[3][2] - m.m[1][2] * m.m[3][1]) -
        m.m[0][1] * (m.m[1][0] * m.m[3][2] - m.m[1][2] * m.m[3][0]) +
        m.m[0][2] * (m.m[1][0] * m.m[3][1] - m.m[1][1] * m.m[3][0]));

    result.m[3][3] = invDet * (m.m[0][0] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1]) -
        m.m[0][1] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0]) +
        m.m[0][2] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0]));



    return result;
}

Matrix4x4 Matrix4x4::MakePerspectivFovMatrix(float fovY, float aspect, float nearZ, float farZ) {
    Matrix4x4 mat = {};
    float f = 1.0f / tanf(fovY / 2.0f);

    mat.m[0][0] = f / aspect;
    mat.m[1][1] = f;
    mat.m[2][2] = farZ / (farZ - nearZ);  
    mat.m[2][3] = 1.0f;
    mat.m[3][2] = (-nearZ * farZ) / (farZ - nearZ);
    return mat;
}

Matrix4x4 Matrix4x4::MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
    Matrix4x4 mat = {};

    float width = right - left;
    float height = top - bottom;
    float depth = farClip - nearClip;

    if (depth == 0.0f) { depth = 0.0001f; }  // ゼロ除算回避

    mat.m[0][0] = 2.0f / width;
    mat.m[1][1] = 2.0f / height;
    mat.m[2][2] = -2.0f / depth;
    mat.m[3][0] = -(right + left) / width;
    mat.m[3][1] = -(top + bottom) / height;
    mat.m[3][2] = -(farClip + nearClip) / depth;
    mat.m[3][3] = 1.0f;

    return mat;
}
