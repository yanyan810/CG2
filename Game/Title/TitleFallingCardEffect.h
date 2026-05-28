#pragma once

#include <memory>
#include <random>
#include <string>
#include <array>
#include <vector>

#include "Vector3.h"

class DirectXCommon;
class Matrix4x4;
class Sprite;
class SpriteCommon;

class TitleFallingCardEffect {
public:
	TitleFallingCardEffect();
	~TitleFallingCardEffect();

	void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dx, float screenWidth, float screenHeight);
	void Reset();
	void Update(float dt);
	void Draw(const Matrix4x4& view, const Matrix4x4& proj);

private:
	struct CardVisualDef {
		std::string artTexturePath;
		int rank = 1;
		int suit = 0;
	};

	struct FallingCard {
		std::unique_ptr<Sprite> frameSprite;
		std::unique_ptr<Sprite> artSprite;
		std::array<std::unique_ptr<Sprite>, 2> rankSprites;
		std::array<std::unique_ptr<Sprite>, 4> suitSprites;
		bool alive = false;
		Vector2 basePosition{ 0.0f, 0.0f };
		Vector2 velocity{ 0.0f, 0.0f };
		float rotation = 0.0f;
		float rotationSpeed = 0.0f;
		float flutterPhase = 0.0f;
		float flutterSpeed = 0.0f;
		float flutterAmount = 0.0f;
		float scale = 1.0f;
		float alpha = 1.0f;
		int visualIndex = 0;
		int rank = 1;
		int suit = 0;
	};

	void LoadCardVisuals_();
	void SpawnCard_(bool initialSpread);
	std::size_t CountAlive_() const;
	float RandomFloat_(float minValue, float maxValue);
	int RandomInt_(int minValue, int maxValue);

	std::vector<FallingCard> cards_;
	std::vector<CardVisualDef> cardVisuals_;
	std::mt19937 randomEngine_;

	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;
	float spawnTimer_ = 0.0f;
};
