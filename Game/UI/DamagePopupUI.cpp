#include "DamagePopupUI.h"

#include "DirectXCommon.h"
#include "Object3dCommon.h"

#include <string>
#include <utility>

void DamagePopupUI::Initialize(Object3dCommon* objCom, DirectXCommon* dx, Camera* camera)
{
	objCom_ = objCom;
	dx_ = dx;
	camera_ = camera;

	for (int d = 0; d <= 9; ++d) {
		std::string path = "cards/models/";
		path += static_cast<char>('0' + d);
		path += ".obj";

		auto obj = std::make_unique<Object3d>();
		obj->Initialize(objCom_, dx_);
		obj->SetModel(path);
		obj->SetCamera(camera_);
		digitModelPool_[d] = std::move(obj);
	}

	Clear();
}

void DamagePopupUI::Clear()
{
	damagePopups_.clear();
}

void DamagePopupUI::Update(float dt)
{
	for (auto it = damagePopups_.begin(); it != damagePopups_.end(); ) {
		it->timer -= 1.0f;
		it->pos.y += 0.05f;
		if (it->timer <= 0.0f) {
			it = damagePopups_.erase(it);
			continue;
		}

		const float gap = 0.8f;
		const int count = static_cast<int>(it->digitModels.size());
		const float startX = -gap * 0.5f * (count - 1);
		for (size_t i = 0; i < it->digitModels.size(); ++i) {
			Vector3 digitPos = it->pos;
			digitPos.x += startX + gap * i;
			digitPos.z -= 1.0f;

			it->digitModels[i]->SetTranslate(digitPos);
			it->digitModels[i]->SetScale({ 0.8f, 0.8f, 0.8f });
			it->digitModels[i]->SetRotate({ 0.0f, 0.0f, 0.0f });
			it->digitModels[i]->Update(dt);
		}
		++it;
	}
}

void DamagePopupUI::Draw3D()
{
	for (auto& popup : damagePopups_) {
		for (auto& obj : popup.digitModels) {
			if (obj) {
				obj->Draw();
			}
		}
	}
}

void DamagePopupUI::SpawnDamage(const Vector3& pos, int damage, bool isPlayer)
{
	(void)isPlayer;

	DamagePopup p;
	p.damage = damage;
	p.pos = pos;
	p.pos.y += 2.0f;
	p.timer = 60.0f;

	std::string dmgStr = std::to_string(damage);
	for (char c : dmgStr) {
		if (c >= '0' && c <= '9') {
			int digit = c - '0';

			auto obj = std::make_unique<Object3d>();
			obj->Initialize(objCom_, dx_);
			if (digitModelPool_[digit]) {
				obj->SetModel(digitModelPool_[digit]->GetModel());
			}
			obj->SetCamera(camera_);

			p.digitModels.push_back(std::move(obj));
		}
	}

	damagePopups_.push_back(std::move(p));
}
