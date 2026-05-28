#include "DamagePopupUI.h"

#include "DirectXCommon.h"
#include "Object3dCommon.h"

#include <cmath>
#include <string>
#include <utility>

namespace {
	constexpr float kOverlapCheckRadiusX = 2.2f;
	constexpr float kOverlapCheckRadiusZ = 2.2f;
	constexpr float kLaneOffsetScaleX = 1.5f;
	constexpr float kLaneOffsetScaleY = 1.5f;

	Vector3 GetPopupLaneOffset_(int lane)
	{
		Vector3 offset{};

		switch (lane % 8) {
		case 0: offset = { 0.0f, 0.0f, 0.0f }; break;
		case 1: offset = { -1.05f, 0.18f, 0.12f }; break;
		case 2: offset = { 1.05f, 0.36f,-0.12f }; break;
		case 3: offset = { -0.55f, 0.54f, 0.58f }; break;
		case 4: offset = { 0.55f, 0.72f, 0.58f }; break;
		case 5: offset = { -1.45f, 0.90f,-0.35f }; break;
		case 6: offset = { 1.45f, 1.08f, 0.35f }; break;
		default: offset = { 0.0f, 1.26f,-0.65f }; break;
		}

		offset.x *= kLaneOffsetScaleX;
		offset.y *= kLaneOffsetScaleY;

		return offset;
	}
}

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

		const float gap = 0.7f;
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

	int overlapCount = 0;
	for (const auto& popup : damagePopups_) {
		if (std::fabs(popup.pos.x - pos.x) <= kOverlapCheckRadiusX &&
			std::fabs(popup.pos.z - pos.z) <= kOverlapCheckRadiusZ) {
			++overlapCount;
		}
	}
	const Vector3 laneOffset = GetPopupLaneOffset_(overlapCount);

	DamagePopup p;
	p.damage = damage;
	p.pos = pos;
	p.pos.x += laneOffset.x;
	p.pos.y += 2.0f;
	p.pos.y += laneOffset.y;
	p.pos.z += laneOffset.z;
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
