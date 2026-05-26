#pragma once

#include "MathStruct.h"
#include "Object3d.h"

#include <array>
#include <memory>
#include <vector>

class Camera;
class DirectXCommon;
class Object3dCommon;

class DamagePopupUI {
public:
	void Initialize(Object3dCommon* objCom, DirectXCommon* dx, Camera* camera);
	void Clear();
	void Update(float dt);
	void Draw3D();
	void SpawnDamage(const Vector3& pos, int damage, bool isPlayer = false);

private:
	struct DamagePopup {
		int damage = 0;
		Vector3 pos;
		float timer = 60.0f;
		std::vector<std::unique_ptr<Object3d>> digitModels;
	};

	Object3dCommon* objCom_ = nullptr;
	DirectXCommon* dx_ = nullptr;
	Camera* camera_ = nullptr;

	std::array<std::unique_ptr<Object3d>, 10> digitModelPool_;
	std::vector<DamagePopup> damagePopups_;
};
