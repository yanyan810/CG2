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
	static Matrix4x4 RotateX(float angleRad);
	static Matrix4x4 RotateY(float angleRad);
	static Matrix4x4 RotateZ(float angleRad);
	static Matrix4x4 RotateXYZ(float angleX, float angleY, float angleZ);
	static Matrix4x4 PerspectiveFov(float fovY, float aspect, float nearZ, float farZ);
	static Matrix4x4 MakeScaleMatrix(const Matrix4x4& m);

	static Matrix4x4 MakeRotateZMatrix(float angleRad);
		
	static Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
	static Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translation);
	static Matrix4x4 Inverse(const Matrix4x4& m);

	static Matrix4x4 MakePerspectivFovMatrix(float fovY, float aspect, float nearZ, float farZ);
	static Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

	static Matrix4x4 MakeViewMatrix(const Vector3& eye, const Vector3& target, const Vector3& up);

	//クロス積
	static Vector3 Cross(const Vector3& v1, const Vector3& v2);


	//正規化
	static Vector3 Normalize(const Vector3& vector);

	static Vector3 Subtract(const Vector3& v1, const Vector3& v2);
	static Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearZ, float farZ);

	// ★ 追加：行列の掛け算オペレータ
	Matrix4x4 operator*(const Matrix4x4& rhs) const;
	Matrix4x4& operator*=(const Matrix4x4& rhs);

};