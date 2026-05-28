#pragma once

#include <memory>
#include <random>
#include <vector>

#include "Vector3.h"
#include "CardInstance.h"

class Camera;
class Card3D;
class CardDatabase;
class DirectXCommon;
class Object3dCommon;

class TitleFallingCardEffect {
public:
	TitleFallingCardEffect();
	~TitleFallingCardEffect();

	void Initialize(
		Object3dCommon* objCommon,
		DirectXCommon* dx,
		Camera* camera,
		CardDatabase* cardDb,
		float screenWidth,
		float screenHeight);
	void Reset();
	void Update(float dt);
	void Draw3D();

private:
	struct CardModelDef {
		int defId = 0;
		CardInstance instance{};
	};

	struct FallingCard {
		std::unique_ptr<Card3D> model;
		bool alive = false;
		Vector2 basePosition{ 0.0f, 0.0f };
		Vector2 velocity{ 0.0f, 0.0f };
		float rotation = 0.0f;
		float rotationSpeed = 0.0f;
		float spinRotation = 0.0f;
		float spinSpeed = 0.0f;
		float flutterPhase = 0.0f;
		float flutterSpeed = 0.0f;
		float flutterAmount = 0.0f;
		float scale = 1.0f;
		float depthOffset = 0.0f;
		float overlapDepthOffset = 0.0f;
		float targetOverlapDepthOffset = 0.0f;
		int modelIndex = 0;
	};

	void LoadCardModels_();
	void SpawnCard_(bool initialSpread);
	bool IsSpawnPositionClear_(const Vector2& position) const;
	void ResolveCardDepth_();
	std::size_t CountAlive_() const;
	float RandomFloat_(float minValue, float maxValue);
	int RandomInt_(int minValue, int maxValue);
	Vector3 ScreenToWorld_(const Vector2& screenPosition, float worldZ) const;

	std::vector<FallingCard> cards_;
	std::vector<CardModelDef> cardModels_;
	std::mt19937 randomEngine_;
	Object3dCommon* objCommon_ = nullptr;
	DirectXCommon* dx_ = nullptr;
	Camera* camera_ = nullptr;
	CardDatabase* cardDb_ = nullptr;

	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;
	float spawnTimer_ = 0.0f;
};
