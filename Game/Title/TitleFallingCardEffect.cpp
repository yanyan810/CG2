#include "TitleFallingCardEffect.h"

#include "Camera.h"
#include "Card3D.h"
#include "CardDatabase.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr std::size_t kMaxFallingCards = 22;
constexpr std::size_t kInitialCardCount = 10;
constexpr float kSpawnInterval = 0.28f;
constexpr float kDeleteMarginY = 180.0f;
constexpr float kPi = 3.1415926535f;
constexpr float kCardWorldZ = 10.0f;
constexpr float kCardDepthSpacing = 0.35f;
constexpr int kSpawnPositionAttempts = 18;
constexpr float kMinSpawnDistanceX = 300.0f;
constexpr float kMinSpawnDistanceY = 360.0f;
constexpr float kOverlapCheckBaseX = 330.0f;
constexpr float kOverlapCheckBaseY = 390.0f;
constexpr float kOverlapDepthStep = 15.0f;
constexpr float kMaxOverlapDepthOffset = 60.0f;
constexpr float kDepthFollowSpeed = 10.0f;
}

TitleFallingCardEffect::TitleFallingCardEffect()
	: randomEngine_(std::random_device{}())
{
}

TitleFallingCardEffect::~TitleFallingCardEffect() = default;

void TitleFallingCardEffect::Initialize(
	Object3dCommon* objCommon,
	DirectXCommon* dx,
	Camera* camera,
	CardDatabase* cardDb,
	float screenWidth,
	float screenHeight)
{
	objCommon_ = objCommon;
	dx_ = dx;
	camera_ = camera;
	cardDb_ = cardDb;
	screenWidth_ = screenWidth;
	screenHeight_ = screenHeight;

	LoadCardModels_();

	cards_.clear();
	cards_.reserve(kMaxFallingCards);
	for (std::size_t i = 0; i < kMaxFallingCards; ++i) {
		FallingCard card;
		card.depthOffset = static_cast<float>(i) * kCardDepthSpacing;
		card.model = std::make_unique<Card3D>();
		card.model->Setup(objCommon_, dx_, camera_);
		card.model->SetShowCost(false);
		if (!cardModels_.empty()) {
			const CardModelDef& modelDef = cardModels_.front();
			if (const CardDef* def = cardDb_->Find(modelDef.defId)) {
				card.model->SetCardData(*def, modelDef.instance);
			}
		}
		card.model->SetTransform({ 0.0f, -30.0f, kCardWorldZ + card.depthOffset }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });

		cards_.push_back(std::move(card));
	}

	Reset();
}

void TitleFallingCardEffect::Reset()
{
	spawnTimer_ = 0.0f;
	for (auto& card : cards_) {
		card.alive = false;
	}

	for (std::size_t i = 0; i < std::min(kInitialCardCount, cards_.size()); ++i) {
		SpawnCard_(true);
	}
}

void TitleFallingCardEffect::Update(float dt)
{
	spawnTimer_ -= dt;
	if (spawnTimer_ <= 0.0f && CountAlive_() < kMaxFallingCards) {
		SpawnCard_(false);
		spawnTimer_ = kSpawnInterval;
	}

	for (auto& card : cards_) {
		if (!card.alive) {
			continue;
		}

		card.basePosition.x += card.velocity.x * dt;
		card.basePosition.y += card.velocity.y * dt;
		card.rotation += card.rotationSpeed * dt;
		card.spinRotation += card.spinSpeed * dt;
		card.flutterPhase += card.flutterSpeed * dt;

		if (card.basePosition.y > screenHeight_ + kDeleteMarginY) {
			card.alive = false;
		}
	}

	ResolveCardDepth_();

	for (auto& card : cards_) {
		if (!card.alive) {
			continue;
		}

		const float flutterX = std::sin(card.flutterPhase) * card.flutterAmount;
		const float drawRotationZ = card.rotation + std::sin(card.flutterPhase * 0.7f) * 0.18f;
		const float drawRotationY = card.spinRotation + std::sin(card.flutterPhase * 0.85f) * 0.22f;
		const Vector2 drawPosition = {
			card.basePosition.x + flutterX,
			card.basePosition.y
		};
		const float worldZ = kCardWorldZ + card.depthOffset + card.overlapDepthOffset;
		const Vector3 worldPos = ScreenToWorld_(drawPosition, worldZ);
		const float baseDistance = (kCardWorldZ + card.depthOffset) - (camera_ ? camera_->GetTranslate().z : -40.0f);
		const float depthDistance = worldZ - (camera_ ? camera_->GetTranslate().z : -40.0f);
		const float depthScale = baseDistance > 0.001f ? depthDistance / baseDistance : 1.0f;
		const Vector3 worldScale = {
			card.scale * depthScale,
			card.scale * depthScale,
			card.scale * depthScale
		};
		card.model->SetTransform(worldPos, { 0.0f, drawRotationY, drawRotationZ }, worldScale);
		card.model->Update(dt);
	}
}

void TitleFallingCardEffect::Draw3D()
{
	for (auto& card : cards_) {
		if (!card.alive || !card.model) {
			continue;
		}
		card.model->Draw();
	}
}

void TitleFallingCardEffect::LoadCardModels_()
{
	cardModels_.clear();
	if (!cardDb_) {
		return;
	}

	for (int id = 1; id <= 40; ++id) {
		if (!cardDb_->Find(id)) {
			continue;
		}

		CardModelDef model{};
		model.defId = id;
		model.instance.defId = id;
		model.instance.number = (id - 1) % 13 + 1;
		model.instance.suit = static_cast<CardSuit>((id - 1) % 4);
		cardModels_.push_back(model);
	}
}

void TitleFallingCardEffect::SpawnCard_(bool initialSpread)
{
	auto it = std::find_if(cards_.begin(), cards_.end(), [](const FallingCard& card) {
		return !card.alive;
	});
	if (it == cards_.end() || cardModels_.empty() || !cardDb_) {
		return;
	}

	const int modelIndex = RandomInt_(0, static_cast<int>(cardModels_.size()) - 1);
	const CardModelDef& modelDef = cardModels_[static_cast<std::size_t>(modelIndex)];
	if (const CardDef* def = cardDb_->Find(modelDef.defId)) {
		it->model->SetCardData(*def, modelDef.instance);
	}

	it->alive = true;
	Vector2 spawnPosition{};
	for (int attempt = 0; attempt < kSpawnPositionAttempts; ++attempt) {
		spawnPosition = {
			RandomFloat_(-90.0f, screenWidth_ + 90.0f),
			initialSpread ? RandomFloat_(-screenHeight_ * 0.85f, screenHeight_ * 0.45f) : RandomFloat_(-170.0f, -50.0f)
		};

		if (IsSpawnPositionClear_(spawnPosition)) {
			break;
		}
	}
	it->basePosition = spawnPosition;
	it->velocity = {
		RandomFloat_(-18.0f, 18.0f),
		RandomFloat_(70.0f, 155.0f)
	};
	it->rotation = RandomFloat_(-kPi, kPi);
	it->rotationSpeed = RandomFloat_(-1.8f, 1.8f);
	it->spinRotation = RandomFloat_(0.0f, kPi * 2.0f);
	const float spinDirection = RandomInt_(0, 1) == 0 ? -1.0f : 1.0f;
	it->spinSpeed = spinDirection * RandomFloat_(kPi * 0.9f, kPi * 1.8f);
	it->flutterPhase = RandomFloat_(0.0f, kPi * 2.0f);
	it->flutterSpeed = RandomFloat_(1.8f, 4.2f);
	it->flutterAmount = RandomFloat_(16.0f, 46.0f);
	it->scale = RandomFloat_(0.86f, 1.0f);//カードの大きさのばらつき
	it->overlapDepthOffset = 0.0f;
	it->targetOverlapDepthOffset = 0.0f;
	it->modelIndex = modelIndex;
	it->model->SetShowCost(false);
	const float worldZ = kCardWorldZ + it->depthOffset;
	it->model->SetTransform(ScreenToWorld_(it->basePosition, worldZ), { 0.0f, it->spinRotation, it->rotation }, { it->scale, it->scale, it->scale });
}

bool TitleFallingCardEffect::IsSpawnPositionClear_(const Vector2& position) const
{
	for (const auto& card : cards_) {
		if (!card.alive) {
			continue;
		}

		const float dx = std::abs(card.basePosition.x - position.x);
		const float dy = std::abs(card.basePosition.y - position.y);
		if (dx < kMinSpawnDistanceX && dy < kMinSpawnDistanceY) {
			return false;
		}
	}

	return true;
}

void TitleFallingCardEffect::ResolveCardDepth_()
{
	for (auto& card : cards_) {
		card.targetOverlapDepthOffset = 0.0f;
	}

	for (std::size_t i = 0; i < cards_.size(); ++i) {
		FallingCard& a = cards_[i];
		if (!a.alive) {
			continue;
		}

		for (std::size_t j = i + 1; j < cards_.size(); ++j) {
			FallingCard& b = cards_[j];
			if (!b.alive) {
				continue;
			}

			const float avgScale = (a.scale + b.scale) * 0.5f;
			const float minX = kOverlapCheckBaseX * avgScale;
			const float minY = kOverlapCheckBaseY * avgScale;
			const float dx = b.basePosition.x - a.basePosition.x;
			const float dy = b.basePosition.y - a.basePosition.y;
			const float absDx = std::abs(dx);
			const float absDy = std::abs(dy);

			if (absDx >= minX || absDy >= minY) {
				continue;
			}

			FallingCard& behind = a.basePosition.y <= b.basePosition.y ? a : b;
			behind.targetOverlapDepthOffset = std::min(
				behind.targetOverlapDepthOffset + kOverlapDepthStep,
				kMaxOverlapDepthOffset);
		}
	}

	for (auto& card : cards_) {
		if (!card.alive) {
			continue;
		}

		const float follow = 1.0f - std::exp(-kDepthFollowSpeed * (1.0f / 60.0f));
		card.overlapDepthOffset += (card.targetOverlapDepthOffset - card.overlapDepthOffset) * follow;
	}
}

std::size_t TitleFallingCardEffect::CountAlive_() const
{
	return static_cast<std::size_t>(std::count_if(cards_.begin(), cards_.end(), [](const FallingCard& card) {
		return card.alive;
	}));
}

float TitleFallingCardEffect::RandomFloat_(float minValue, float maxValue)
{
	std::uniform_real_distribution<float> dist(minValue, maxValue);
	return dist(randomEngine_);
}

int TitleFallingCardEffect::RandomInt_(int minValue, int maxValue)
{
	std::uniform_int_distribution<int> dist(minValue, maxValue);
	return dist(randomEngine_);
}

Vector3 TitleFallingCardEffect::ScreenToWorld_(const Vector2& screenPosition, float worldZ) const
{
	const float distance = worldZ - (camera_ ? camera_->GetTranslate().z : -40.0f);
	const float fovY = camera_ ? camera_->GetFovY() : 0.45f;
	const float aspect = camera_ ? camera_->GetAspect() : (screenWidth_ / screenHeight_);
	const float visibleHeight = 2.0f * std::tan(fovY * 0.5f) * distance;
	const float visibleWidth = visibleHeight * aspect;
	const Vector3 cameraPos = camera_ ? camera_->GetTranslate() : Vector3{ 0.0f, 4.0f, -40.0f };

	return {
		cameraPos.x + (screenPosition.x / screenWidth_ - 0.5f) * visibleWidth,
		cameraPos.y + (0.5f - screenPosition.y / screenHeight_) * visibleHeight,
		worldZ
	};
}
