#pragma once
#include "Matrix4x4.h"
#include "WinApp.h"
class Camera
{

public:

	Camera();

	void Update();

	//setter
	void SetRotate(const Vector3& r) { transform_.rotate = r; ClearWorldMatrixOverride(); }
	void SetTranslate(const Vector3& t) { transform_.translate = t; ClearWorldMatrixOverride(); }
	void SetFovY(float fovY) { fovY_ = fovY; }
	void SetWorldMatrixOverride(const Matrix4x4& worldMatrix) { worldMatrixOverride_ = worldMatrix; useWorldMatrixOverride_ = true; }
	void ClearWorldMatrixOverride() { useWorldMatrixOverride_ = false; }
	void SetAspect(float aspect) { aspect_ = aspect; }
	void SetNearZ(float nearZ) { nearZ_ = nearZ; }
	void SetFarClip(float farZ) { farZ_ = farZ; }
	//getter
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
	const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }
	const Vector3& GetRotate() const { return transform_.rotate; }
	const Vector3& GetTranslate() const { return transform_.translate; }
	float GetFovY() const { return fovY_; }
	float GetAspect() const { return aspect_; }
	bool UsesWorldMatrixOverride() const { return useWorldMatrixOverride_; }
	const Transform GetTransform() const { return transform_; }

	void SetProjectionShift(const Matrix4x4& shift) { projectionShift_ = shift; }

private:

	Transform transform_;
	Matrix4x4 worldMatrix_;
	Matrix4x4 viewMatrix_;

	float fovY_;
	float aspect_;
	float nearZ_;
	float farZ_;
	Matrix4x4 projectionMatrix_;
	Matrix4x4 viewProjectionMatrix_;

	Matrix4x4 projectionShift_ = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 worldMatrixOverride_ = Matrix4x4::MakeIdentity4x4();
	bool useWorldMatrixOverride_ = false;

};

