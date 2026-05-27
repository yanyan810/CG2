#include "Enemy.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "GameApp.h"
#include "GeometryGenerator.h"
#include "ModelManager.h"
#include <cmath>
#include <algorithm>

namespace {
	constexpr float kFacePlayerYaw = -1.5708f;
	constexpr float kImportedEnemyYawCorrection = 1.5708f;

	float GetEnemyYaw_(EnemyType type)
	{
		switch (type) {
		case EnemyType::Goblin:
		case EnemyType::Golem:
		case EnemyType::Needle:
			return kFacePlayerYaw + kImportedEnemyYawCorrection;
		default:
			return kFacePlayerYaw;
		}
	}

	Vector3 GetEnemyScale_(EnemyType type)
	{
		switch (type) {
		case EnemyType::Slime:
			return { 1.5f, 1.5f, 1.5f };
		case EnemyType::Goblin:
			return { 0.45f, 0.45f, 0.45f };
		case EnemyType::Golem:
			return { 0.35f, 0.35f, 0.35f };
		default:
			return { 1.0f, 1.0f, 1.0f };
		}
	}

	Model::ModelData MakeShieldPrimitiveModelData_(const std::vector<Model::VertexData>& vertices) {
		Model::ModelData modelData{};
		modelData.materials.push_back({ "" });

		Model::MeshData mesh{};
		mesh.materialIndex = 0;
		mesh.vertices = vertices;
		mesh.skinned = false;
		mesh.startVertex = 0;
		mesh.vertexCount = static_cast<uint32_t>(vertices.size());
		mesh.startIndex = 0;
		mesh.indexCount = static_cast<uint32_t>(vertices.size());
		modelData.meshes.push_back(std::move(mesh));

		modelData.indices.resize(vertices.size());
		for (uint32_t i = 0; i < static_cast<uint32_t>(vertices.size()); ++i) {
			modelData.indices[i] = i;
		}

		modelData.rootNode.name = "EnemyShieldHexRoot";
		modelData.rootNode.localMatrix = Matrix4x4::MakeIdentity4x4();
		modelData.rootNode.meshIndices.push_back(0);
		return modelData;
	}

	std::vector<Vector2> GenerateShieldHexOffsets_(int requestedCount) {
		const int count = std::clamp(requestedCount, 1, 61);
		int radius = 0;
		while (1 + 3 * radius * (radius + 1) < count) {
			++radius;
		}

		std::vector<Vector2> offsets;
		offsets.reserve(1 + 3 * radius * (radius + 1));
		for (int q = -radius; q <= radius; ++q) {
			for (int r = -radius; r <= radius; ++r) {
				const int s = -q - r;
				if (std::abs(s) > radius) {
					continue;
				}
				offsets.push_back({
					(static_cast<float>(q) + static_cast<float>(r) * 0.5f) * 0.52f,
					static_cast<float>(r) * 0.45f
					});
			}
		}

		std::sort(offsets.begin(), offsets.end(), [](const Vector2& a, const Vector2& b) {
			const float da = a.x * a.x + a.y * a.y;
			const float db = b.x * b.x + b.y * b.y;
			if (std::abs(da - db) > 0.0001f) {
				return da < db;
			}
			if (std::abs(a.y - b.y) > 0.0001f) {
				return a.y > b.y;
			}
			return a.x < b.x;
			});

		if (static_cast<int>(offsets.size()) > count) {
			offsets.resize(count);
		}
		return offsets;
	}
}

// ==========================================
// Enemy 本体
// ==========================================
void Enemy::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam,
	EnemyType type, const Vector3& spawnXYZ)
{
	objCommon_ = objCommon;
	dx_ = dx;
	cam_ = cam;
	type_ = type;
	pos_ = spawnXYZ;

	alive_ = true;
	hp_ = 300;
	block_ = 0;

	model_ = std::make_unique<Object3d>();
	model_->Initialize(objCommon_, dx_);
	model_->SetCamera(cam_);

	if (type_ == EnemyType::Boss) {
		model_->SetModel("enemy/boss/Boss.obj");
		ai_.LoadPattern("resources/cards/Boos.json");
	} else if (type_ == EnemyType::Slime) {

		model_->SetModel("slime/slime.obj");
		ai_.LoadPattern("resources/cards/Slime.json");
	} else if (type_ == EnemyType::Goblin) {
		model_->SetModel("Goblin/Goblin.obj");
		ai_.LoadPattern("resources/cards/Goblin.json");
	} else if (type_ == EnemyType::Golem) {
		model_->SetModel("Golem/Golem.obj");
		ai_.LoadPattern("resources/cards/Golem.json");
	} else if (type_ == EnemyType::Needle) {
		model_->SetModel("needle/needle.obj");
		ai_.LoadPattern("resources/cards/needle.json");
	}
	model_->SetScale(GetEnemyScale_(type_));

	// エネミーを左側（プレイヤー側）に向かせる
	rot_ = { 0.0f, GetEnemyYaw_(type_), 0.0f };

	if (model_->HasAnimation()) {
		model_->PlayAnimation("Idle", true);
	}

	// 読み込んだJSONの最大HPを、現在のHPにセットする
	maxHp_ = ai_.GetMaxHP();
	hp_ = maxHp_;

	basePos_ = pos_;

	model_->SetTranslate(pos_);
	model_->SetRotate(rot_);
	model_->Update(0.0f);
	InitializeShieldEffect_();

}

void Enemy::ApplyStageConfig(const StageEnemyConfig& config)
{
	if (!config.behaviorJson.empty()) {
		ai_.LoadPattern(config.behaviorJson);
	}

	if (config.maxHp > 0) {
		SetMaxHp(config.maxHp, true);
	}

	if (config.hp >= 0) {
		SetHp(config.hp);
	}
}

void Enemy::PlayAttackAnim(const Vector3& targetPos) {
	animState_ = AnimState::AttackForward;
	animTimer_ = 0.0f;
	animDuration_ = 0.2f;
	startPos_ = basePos_;
	targetPos_ = Lerp(basePos_, targetPos, 0.8f);
}

void Enemy::PlayDamageAnim() {
	animState_ = AnimState::Damage;
	animTimer_ = 0.0f;
	animDuration_ = 0.15f;
	startPos_ = basePos_;
	// ボスは右側にいる想定なので、右(Xのプラス方向)に下がる
	targetPos_ = { basePos_.x + 2.0f, basePos_.y, basePos_.z };
}

void Enemy::Update(float dt)
{
	if (!alive_) return;

	// 動き（アニメーション）の計算
	if (animState_ != AnimState::Idle) {
		animTimer_ += dt;
		float t = animTimer_ / animDuration_;
		if (t > 1.0f) t = 1.0f;

		float easeT = t * (2.0f - t);
		pos_ = Lerp(startPos_, targetPos_, easeT);

		if (animTimer_ >= animDuration_) {
			if (animState_ == AnimState::AttackForward) {
				animState_ = AnimState::AttackReturn;
				animTimer_ = 0.0f;
				animDuration_ = 0.3f;
				startPos_ = pos_;
				targetPos_ = basePos_;
			} else if (animState_ == AnimState::AttackReturn || animState_ == AnimState::Damage) {
				animState_ = AnimState::Idle;
				pos_ = basePos_;
			}
		}
	}

	// 赤点滅（フラッシュ）の計算
	if (flashTimer_ > 0.0f) {
		flashTimer_ -= dt;
		model_->SetMaterialColor({ 1.0f, 0.2f, 0.2f, 1.0f });
	} else if (isHighlighted_) {
		model_->SetMaterialColor({ 1.5f, 1.5f, 0.5f, 1.0f });
	} else {
		model_->SetMaterialColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
	isHighlighted_ = false;
	// AIの更新（やられ判定など最低限の処理）
	ai_.Update(*this, dt);

	// 死亡判定
	if (hp_ <= 0) {
		alive_ = false;
		return;
	}

	if (model_) {
		// 座標と回転を適用
		model_->SetTranslate(pos_);
		model_->SetRotate(rot_);

		// アニメーション更新
		model_->Update(dt);
	}
	UpdateShieldEffect_(dt);

	// 当たり判定用の箱（カードバトルで使うかもしれないので残しておく）
	body_.min = { pos_.x - 1.5f, pos_.y, pos_.z - 1.5f };
	body_.max = { pos_.x + 1.5f, pos_.y + 3.0f, pos_.z + 1.5f };
}

void Enemy::Draw()
{
	if (!alive_ || !model_) return;
	model_->Draw();
	DrawShield_(shieldColor_, 1.0f);
}

void Enemy::DrawShieldBloom(GameApp& app)
{
	if (!alive_ || !ShouldDrawShield_() || shieldCells_.empty()) {
		return;
	}

	BloomParam param = app.ObjectPost()->GetParam();
	param.threshold = 0.0f;
	param.intensity = shieldBloomIntensity_;
	param.vignetteIntensity = 0.0f;
	param.vignetteScale = 0.0f;
	param.distortionAmount = 0.0f;
	param.chromAbAmount = shieldBloomChromAb_;
	param.isGrayscale = 0.0f;
	param.isInverted = 0.0f;
	param.noiseIntensity = 0.0f;
	param.scanlineIntensity = 0.0f;
	param.curvature = 0.0f;
	param.borderSharp = 0.0f;
	param.glitchAmount = 0.0f;
	param.dissolveAmount = -1.0f;

	app.ObjectPost()->SetParam(param);
	app.BeginObjectPostEffect();
	DrawShield_(shieldBloomColor_, shieldBloomScale_);
	app.EndObjectPostEffect();
	app.ObjCom()->SetGraphicsPipelineState();
}

int Enemy::Damage(int damage) {
	if (damage <= 0) {
		return 0;
	}

	const int blockBefore = block_;
	if (block_ > 0) {
		const int blocked = block_ < damage ? block_ : damage;
		block_ -= blocked;
		damage -= blocked;
	}

	if (blockBefore > 0 && block_ == 0) {
		const int visibleCells = std::max(blockBefore, static_cast<int>(std::ceil(shieldDisplayCount_)));
		TriggerShieldBreak_(visibleCells);
	}

	if (damage <= 0) {
		return 0;
	}

	const int beforeHp = hp_;
	hp_ -= damage;
	if (hp_ <= 0) {
		hp_ = 0;
		alive_ = false;
	}
	return beforeHp - hp_;
}

void Enemy::AddBlock(int value) {
	if (value <= 0) {
		return;
	}

	block_ += value;
	if (block_ < 0) {
		block_ = 0;
	}
	if (block_ > 0) {
		shieldBreakActive_ = false;
		shieldBreakCellCount_ = 0;
	}
}

void Enemy::ResetBlock() {
	block_ = 0;
}

void Enemy::InitializeShieldEffect_() {
	const std::string modelKey = "EnemyShieldHexRing";
	shieldHexModel_ = ModelManager::GetInstance()->FindModel(modelKey);
	if (!shieldHexModel_) {
		auto vertices = GeometryGenerator::GenerateHexRingTriListXY(1.0f, 0.82f);
		shieldHexModel_ = ModelManager::GetInstance()->CreatePrimitiveModel(modelKey, MakeShieldPrimitiveModelData_(vertices));
	}

	EnsureShieldCellCount_();
}

void Enemy::EnsureShieldCellCount_() {
	constexpr int kMaxShieldCellCount = 61;
	shieldCellCount_ = std::clamp(shieldCellCount_, 1, 61);
	while (static_cast<int>(shieldCells_.size()) < kMaxShieldCellCount) {
		auto cell = std::make_unique<Object3d>();
		cell->Initialize(objCommon_, dx_);
		cell->SetModel(shieldHexModel_);
		cell->SetCamera(cam_);
		cell->SetEnableLighting(0);
		cell->SetMaterialColor(shieldColor_);
		shieldCells_.push_back(std::move(cell));
	}
}

int Enemy::GetTargetShieldCellCount_() const {
	if (block_ > 0) {
		return std::clamp(block_, 1, 61);
	}
	return 0;
}

void Enemy::TriggerShieldBreak_(int cellCount) {
	EnsureShieldCellCount_();

	shieldBreakActive_ = true;
	shieldBreakTimer_ = 0.0f;
	shieldBreakCellCount_ = std::clamp(cellCount, 1, 61);
	shieldDisplayCount_ = 0.0f;
	shieldVisibleTimer_ = 1.0f;

	const std::vector<Vector2> hexOffsets = GenerateShieldHexOffsets_(shieldBreakCellCount_);
	shieldBreakBasePositions_.resize(shieldBreakCellCount_);
	shieldBreakVelocities_.resize(shieldBreakCellCount_);
	shieldBreakRotations_.resize(shieldBreakCellCount_);
	shieldBreakAngularVelocities_.resize(shieldBreakCellCount_);

	const Vector3 center = {
		pos_.x + shieldOffset_.x,
		pos_.y + shieldOffset_.y,
		pos_.z + shieldOffset_.z
	};
	const Vector3 groupRotation = {
		shieldRotation_.x,
		shieldRotation_.y + shieldTiltY_ * std::sinf(shieldTimer_ * 1.8f),
		shieldRotation_.z
	};
	const Matrix4x4 groupRotationMatrix = Matrix4x4::RotateXYZ(
		groupRotation.x,
		groupRotation.y,
		groupRotation.z);

	for (int i = 0; i < shieldBreakCellCount_; ++i) {
		const Vector2& offset = hexOffsets[i];
		const Vector3 localOffset = {
			offset.x * shieldSpacingX_ / 0.52f,
			offset.y * shieldSpacingY_ / 0.45f,
			0.0f
		};
		const Vector3 rotatedOffset = Matrix4x4::TransformNormal(localOffset, groupRotationMatrix);
		shieldBreakBasePositions_[i] = {
			center.x + rotatedOffset.x,
			center.y + rotatedOffset.y,
			center.z + rotatedOffset.z
		};

		const float angle = static_cast<float>(i) * 1.37f;
		const float side = (i % 2 == 0) ? 1.0f : -1.0f;
		shieldBreakVelocities_[i] = {
			-0.85f + std::cosf(angle) * 1.2f,
			2.1f + 0.18f * static_cast<float>(i % 5),
			std::sinf(angle) * 1.15f + side * 0.35f
		};
		shieldBreakRotations_[i] = groupRotation;
		shieldBreakAngularVelocities_[i] = {
			2.2f + 0.17f * static_cast<float>(i % 4),
			side * (3.0f + 0.11f * static_cast<float>(i % 7)),
			-1.7f + 0.19f * static_cast<float>(i % 6)
		};
	}
}

void Enemy::UpdateShieldEffect_(float dt) {
	EnsureShieldCellCount_();
	shieldTimer_ += dt;

	if (shieldBreakActive_) {
		shieldBreakTimer_ += dt;
		const float duration = std::max(0.1f, shieldBreakDuration_);
		const float life = std::clamp(1.0f - shieldBreakTimer_ / duration, 0.0f, 1.0f);

		for (int i = 0; i < shieldBreakCellCount_ && i < static_cast<int>(shieldCells_.size()); ++i) {
			Object3d* cell = shieldCells_[i].get();
			const Vector3& base = shieldBreakBasePositions_[i];
			const Vector3& velocity = shieldBreakVelocities_[i];
			const Vector3& baseRotation = shieldBreakRotations_[i];
			const Vector3& angularVelocity = shieldBreakAngularVelocities_[i];
			const float t = shieldBreakTimer_;
			cell->SetTranslate({
				base.x + velocity.x * t,
				base.y + velocity.y * t - 0.5f * shieldBreakGravity_ * t * t,
				base.z + velocity.z * t
				});
			cell->SetRotate({
				baseRotation.x + angularVelocity.x * t,
				baseRotation.y + angularVelocity.y * t,
				baseRotation.z + angularVelocity.z * t
				});
			const float scale = shieldBaseScale_ * (0.35f + 0.65f * life);
			cell->SetScale({ scale, scale, scale });
			cell->SetMaterialColor({
				shieldColor_.x,
				shieldColor_.y,
				shieldColor_.z,
				shieldColor_.w * life
				});
			cell->Update(dt);
		}

		if (shieldBreakTimer_ >= duration) {
			shieldBreakActive_ = false;
			shieldBreakCellCount_ = 0;
			shieldVisibleTimer_ = 0.0f;
		}
		return;
	}

	const int targetCellCount = GetTargetShieldCellCount_();
	if (shieldDisplayCount_ < static_cast<float>(targetCellCount)) {
		shieldDisplayCount_ = std::min(static_cast<float>(targetCellCount), shieldDisplayCount_ + shieldBuildSpeed_ * dt);
	} else if (shieldDisplayCount_ > static_cast<float>(targetCellCount)) {
		shieldDisplayCount_ = std::max(static_cast<float>(targetCellCount), shieldDisplayCount_ - shieldReduceSpeed_ * dt);
	}

	if (shieldDisplayCount_ > 0.01f || targetCellCount > 0) {
		shieldVisibleTimer_ = std::min(shieldVisibleTimer_ + dt * 8.0f, 1.0f);
	} else {
		shieldVisibleTimer_ = std::max(shieldVisibleTimer_ - dt * 6.0f, 0.0f);
	}

	const int activeCellCount = std::clamp(static_cast<int>(std::ceil(shieldDisplayCount_)), 0, 61);
	if (activeCellCount <= 0 || shieldVisibleTimer_ <= 0.0f || shieldCells_.empty()) {
		return;
	}

	const std::vector<Vector2> hexOffsets = GenerateShieldHexOffsets_(activeCellCount);
	const float pulse = 0.5f + 0.5f * std::sinf(shieldTimer_ * shieldPulseSpeed_);
	const float globalAppear = shieldVisibleTimer_ * shieldVisibleTimer_ * (3.0f - 2.0f * shieldVisibleTimer_);
	const Vector3 center = {
		pos_.x + shieldOffset_.x,
		pos_.y + shieldOffset_.y,
		pos_.z + shieldOffset_.z
	};
	const Vector3 groupRotation = {
		shieldRotation_.x,
		shieldRotation_.y + shieldTiltY_ * std::sinf(shieldTimer_ * 1.8f),
		shieldRotation_.z
	};
	const Matrix4x4 groupRotationMatrix = Matrix4x4::RotateXYZ(
		groupRotation.x,
		groupRotation.y,
		groupRotation.z);

	for (int i = 0; i < activeCellCount && i < static_cast<int>(shieldCells_.size()) && i < static_cast<int>(hexOffsets.size()); ++i) {
		const Vector2& offset = hexOffsets[i];
		Object3d* cell = shieldCells_[i].get();
		const Vector3 localOffset = {
			offset.x * shieldSpacingX_ / 0.52f,
			offset.y * shieldSpacingY_ / 0.45f,
			0.0f
		};
		const Vector3 rotatedOffset = Matrix4x4::TransformNormal(localOffset, groupRotationMatrix);
		cell->SetTranslate({
			center.x + rotatedOffset.x,
			center.y + rotatedOffset.y,
			center.z + rotatedOffset.z
			});
		cell->SetRotate(groupRotation);
		const float cellAppearRaw = std::clamp(shieldDisplayCount_ - static_cast<float>(i), 0.0f, 1.0f);
		const float cellAppear = cellAppearRaw * cellAppearRaw * (3.0f - 2.0f * cellAppearRaw);
		const float baseScale = shieldBaseScale_ * globalAppear * cellAppear;
		const float cellPulse = 1.0f + shieldPulseScale_ * std::sinf(shieldTimer_ * shieldPulseSpeed_ + static_cast<float>(i) * 0.55f);
		cell->SetScale({ baseScale * cellPulse, baseScale * cellPulse, baseScale * cellPulse });
		cell->SetMaterialColor({
			shieldColor_.x,
			shieldColor_.y * (0.78f + 0.22f * pulse),
			shieldColor_.z * (0.78f + 0.22f * pulse),
			shieldColor_.w * cellAppear * (0.82f + 0.18f * pulse)
			});
		cell->Update(dt);
	}
}

bool Enemy::ShouldDrawShield_() const {
	return shieldBreakActive_ || shieldDisplayCount_ > 0.01f || GetTargetShieldCellCount_() > 0;
}

void Enemy::DrawShield_(const Vector4& color, float scaleMultiplier) {
	if (!ShouldDrawShield_() || shieldVisibleTimer_ <= 0.0f || shieldCells_.empty()) {
		return;
	}

	const float alpha = std::clamp(shieldVisibleTimer_, 0.0f, 1.0f);
	const int drawCount = shieldBreakActive_
		? std::clamp(shieldBreakCellCount_, 0, static_cast<int>(shieldCells_.size()))
		: std::clamp(static_cast<int>(std::ceil(shieldDisplayCount_)), 0, static_cast<int>(shieldCells_.size()));
	for (int i = 0; i < drawCount; ++i) {
		auto& cell = shieldCells_[i];
		if (!cell) {
			continue;
		}
		const float cellAlpha = shieldBreakActive_
			? std::clamp(1.0f - shieldBreakTimer_ / std::max(0.1f, shieldBreakDuration_), 0.0f, 1.0f)
			: std::clamp(shieldDisplayCount_ - static_cast<float>(i), 0.0f, 1.0f);
		if (cellAlpha <= 0.0f) {
			continue;
		}
		const Vector3 baseScale = cell->GetScale();
		const Vector4 drawColor = {
			color.x,
			color.y,
			color.z,
			color.w * alpha * cellAlpha
		};
		cell->SetMaterialColor(drawColor);
		cell->SetScale({
			baseScale.x * scaleMultiplier,
			baseScale.y * scaleMultiplier,
			baseScale.z * scaleMultiplier
			});
		cell->Update(0.0f);
		cell->Draw();
		cell->SetScale(baseScale);
		cell->Update(0.0f);
	}
}

void Enemy::SetLighting(const LightingParam& p)
{
	light_ = p;
	if (!model_) return;

	model_->SetEnableLighting(light_.lightingMode);
	model_->SetDirection(light_.dir);
	model_->SetIntensity(light_.dirIntensity);
	model_->SetLightColor(light_.dirColor);
	model_->SetPointLightPos(light_.pointPos);
	model_->SetPointLightIntensity(light_.pointIntensity);
	model_->SetPointLightColor(light_.pointColor);
	model_->SetPointLightRadius(light_.pointRadius);
	model_->SetPointLightDecay(light_.pointDecay);
	model_->SetSpotLightPos(light_.spotPos);
	model_->SetSpotLightDirection(light_.spotDir);
	model_->SetSpotLightIntensity(light_.spotIntensity);
}

//チュートリアル用にHP設定

void Enemy::SetMaxHp(int maxHp, bool healToFull)
{
	maxHp_ = (std::max)(1, maxHp);

	if (healToFull) {
		hp_ = maxHp_;
	} else {
		hp_ = (std::min)(hp_, maxHp_);
	}
}

void Enemy::SetHp(int hp)
{
	hp_ = (std::clamp)(hp, 0, maxHp_);
}


// ==========================================
// EnemyManager
// ==========================================
void EnemyManager::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam)
{
	objCommon_ = objCommon;
	dx_ = dx;
	cam_ = cam;
	enemies_.clear();
}

void EnemyManager::Spawn(EnemyType type, const Vector3& pos)
{
	Enemy e{};
	e.Initialize(objCommon_, dx_, cam_, type, pos);
	enemies_.push_back(std::move(e));
}

void EnemyManager::SpawnWithConfig(const StageEnemyConfig& config)
{
	Enemy e{};
	e.Initialize(objCommon_, dx_, cam_, config.type, config.position);
	e.ApplyStageConfig(config);
	enemies_.push_back(std::move(e));
}

void EnemyManager::Update(float dt)
{
	// 全てのエネミーを更新
	for (auto& e : enemies_) {
		e.Update(dt);
	}

	// 死んだエネミーをリストから削除
	enemies_.erase(
		std::remove_if(enemies_.begin(), enemies_.end(),
			[](const Enemy& e) { return !e.IsAlive(); }),
		enemies_.end()
	);
}

void EnemyManager::Draw()
{
	for (auto& e : enemies_) {
		e.Draw();
	}
}

void EnemyManager::DrawShieldBloom(GameApp& app)
{
	for (auto& e : enemies_) {
		e.DrawShieldBloom(app);
	}
}

void EnemyManager::SetLighting(const LightingParam& p)
{
	for (auto& e : enemies_) {
		e.SetLighting(p);
	}
}

Enemy* EnemyManager::GetBoss()
{
	for (auto& e : enemies_) {
		if (e.IsAlive()) return &e;
	}
	return nullptr;
}
int EnemyManager::PickEnemyByMouse(int mouseX, int mouseY, const Matrix4x4& viewProj, float screenWidth, float screenHeight)
{
	int closestIndex = -1;
	float minDepth = 1.0f; // 1.0f(一番奥) ～ 0.0f(一番手前)

	for (int i = 0; i < (int)enemies_.size(); ++i) {
		if (!enemies_[i].IsAlive()) continue;

		// 敵の足元座標を少し上にずらし、体の中央付近をターゲットにする
		Vector3 pos = enemies_[i].GetPos();
		pos.y += 4.0f;
		pos.x += 2.0f;
		// 3Dのワールド座標を、2Dの画面座標に変換する計算
		float w = pos.x * viewProj.m[0][3] + pos.y * viewProj.m[1][3] + pos.z * viewProj.m[2][3] + viewProj.m[3][3];
		if (w <= 0.0f) continue; // カメラより後ろにいる場合は無視

		float cx = (pos.x * viewProj.m[0][0] + pos.y * viewProj.m[1][0] + pos.z * viewProj.m[2][0] + viewProj.m[3][0]) / w;
		float cy = (pos.x * viewProj.m[0][1] + pos.y * viewProj.m[1][1] + pos.z * viewProj.m[2][1] + viewProj.m[3][1]) / w;
		float cz = (pos.x * viewProj.m[0][2] + pos.y * viewProj.m[1][2] + pos.z * viewProj.m[2][2] + viewProj.m[3][2]) / w;

		float screenX = (cx + 1.0f) * 0.5f * screenWidth;
		float screenY = (1.0f - cy) * 0.5f * screenHeight;

		// マウス座標との距離を計算（半径 120 ピクセル以内ならクリック成功とみなす）
		float dx = screenX - (float)mouseX;
		float dy = screenY - (float)mouseY;
		if (dx * dx + dy * dy < 120.0f * 120.0f) {
			// 重なっている場合は、より手前にいる敵を優先する
			if (cz < minDepth) {
				minDepth = cz;
				closestIndex = i;
			}
		}
	}
	return closestIndex; // 誰もクリックされていなければ -1 を返す
}

Enemy* EnemyManager::GetEnemy(size_t index)
{
	if (index >= enemies_.size()) {
		return nullptr;
	}
	return &enemies_[index];
}

const Enemy* EnemyManager::GetEnemy(size_t index) const
{
	if (index >= enemies_.size()) {
		return nullptr;
	}
	return &enemies_[index];
}

void Enemy::SetBC(BadCondition condition) {
	if (badCondition_ == condition) {
		return;
	}
	badConditionPoint_ = 0; // 状態異常ポイントのリセット
	badCondition_ = condition;
}

// 状態異常ポイントの加算
void Enemy::AddBC(int value) {
	SetBC(badConditionPoint_ + value);
}

void Enemy::SubtractBC(int value) {
	SetBC(badConditionPoint_ - value);
}

void  Enemy::AmplifyBC(int value) {
	SetBC(badConditionPoint_ * value);
}

// 状態異常ダメージの適用
void  Enemy::DamageBC(int count) {
	Damage(badConditionPoint_ * count);
}

void  Enemy::RemoveBC() {
	SetBC(BadCondition::kNone);
	SetBC(0);
}

// ターン終了時の状態異常ダメージ処理　いったんあとで
void Enemy::TurnEndApplyBC() {

	// 毒のポイントを半減（切り上げ）
	switch (badCondition_)
	{
	case BadCondition::kPoison:

		Damage(badConditionPoint_);
		badConditionPoint_ = (badConditionPoint_ + 1) / 2;

		break;
	case BadCondition::kFrost:
		break;

	}

	// ポイントが0以下になったら状態異常を解除
	if (badConditionPoint_ <= 0) {
		badConditionPoint_ = 0;
		badCondition_ = BadCondition::kNone;
	}

}
