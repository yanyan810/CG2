#include "TitleFallingCardEffect.h"

#include "DirectXCommon.h"
#include "Matrix4x4.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

#include <algorithm>
#include <cmath>
#include <fstream>

#include "externals/nlohmann/json.hpp"

namespace {
constexpr std::size_t kMaxFallingCards = 32;
constexpr std::size_t kInitialCardCount = 14;
constexpr float kSpawnInterval = 0.18f;
constexpr float kDeleteMarginY = 180.0f;
constexpr float kPi = 3.1415926535f;
constexpr const char* kCardFrameTexture = "resources/cards/art/normalCard.png";
constexpr const char* kWhiteTexture = "resources/ui/white.png";

const char* const kCardDataJsons[] = {
	"resources/cards/data/UtilityAttack.json",
	"resources/cards/data/UtilitySupport.json",
	"resources/cards/data/Poison.json",
	"resources/cards/data/Frost.json",
	"resources/cards/cards.json",
};

const char* const kFallbackCardTextures[] = {
	"resources/cards/art/Attack!.png",
	"resources/cards/art/Fire.png",
	"resources/cards/art/Blocking.png",
	"resources/cards/art/Heal.png",
	"resources/cards/art/QuickDraw.png",
	"resources/cards/art/Doping.png",
	"resources/cards/art/ShieldBash.png",
	"resources/cards/art/CrescentMoon.png",
};

std::string GetRankTexturePath(int digit)
{
	return "resources/ui/num/" + std::to_string(std::clamp(digit, 0, 9)) + ".png";
}

Vector2 ApplyLocalOffset(const Vector2& center, const Vector2& local, float rotation, float scale, float flipScale)
{
	const float x = local.x * scale * flipScale;
	const float y = local.y * scale;
	const float c = std::cos(rotation);
	const float s = std::sin(rotation);
	return {
		center.x + x * c - y * s,
		center.y + x * s + y * c
	};
}

Vector4 GetSuitColor(int suit, float alpha)
{
	switch (suit % 4) {
	case 0: return { 0.04f, 0.04f, 0.04f, alpha };
	case 1: return { 0.95f, 0.08f, 0.08f, alpha };
	case 2: return { 0.35f, 0.78f, 1.0f, alpha };
	default: return { 0.42f, 0.08f, 0.62f, alpha };
	}
}

void ConfigureSprite(Sprite* sprite, const Vector2& position, float rotation, const Vector3& scale, const Vector4& color)
{
	if (!sprite) {
		return;
	}

	sprite->SetPosition(position);
	sprite->SetRotation({ 0.0f, 0.0f, rotation });
	sprite->SetScale(scale);
	sprite->SetColor(color);
}

void HideSprite(Sprite* sprite)
{
	if (sprite) {
		sprite->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	}
}
}

TitleFallingCardEffect::TitleFallingCardEffect()
	: randomEngine_(std::random_device{}())
{
}

TitleFallingCardEffect::~TitleFallingCardEffect() = default;

void TitleFallingCardEffect::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dx, float screenWidth, float screenHeight)
{
	screenWidth_ = screenWidth;
	screenHeight_ = screenHeight;

	TextureManager::GetInstance()->LoadTexture(kCardFrameTexture);
	TextureManager::GetInstance()->LoadTexture(kWhiteTexture);
	for (int i = 0; i <= 9; ++i) {
		TextureManager::GetInstance()->LoadTexture(GetRankTexturePath(i));
	}
	LoadCardVisuals_();

	cards_.clear();
	cards_.reserve(kMaxFallingCards);
	for (std::size_t i = 0; i < kMaxFallingCards; ++i) {
		FallingCard card;
		card.frameSprite = std::make_unique<Sprite>();
		card.frameSprite->Initialize(spriteCommon, dx, kCardFrameTexture);
		card.frameSprite->SetAnchorPoint({ 0.5f, 0.5f });

		card.artSprite = std::make_unique<Sprite>();
		card.artSprite->Initialize(spriteCommon, dx, cardVisuals_.empty() ? kWhiteTexture : cardVisuals_.front().artTexturePath);
		card.artSprite->SetAnchorPoint({ 0.5f, 0.5f });

		for (auto& rankSprite : card.rankSprites) {
			rankSprite = std::make_unique<Sprite>();
			rankSprite->Initialize(spriteCommon, dx, GetRankTexturePath(0));
			rankSprite->SetAnchorPoint({ 0.5f, 0.5f });
		}

		for (auto& suitSprite : card.suitSprites) {
			suitSprite = std::make_unique<Sprite>();
			suitSprite->Initialize(spriteCommon, dx, kWhiteTexture);
			suitSprite->SetAnchorPoint({ 0.5f, 0.5f });
		}

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
		card.flutterPhase += card.flutterSpeed * dt;

		const float flutterX = std::sin(card.flutterPhase) * card.flutterAmount;
		const float flipScale = 0.72f + 0.28f * std::abs(std::cos(card.flutterPhase * 0.85f));
		const float drawRotation = card.rotation + std::sin(card.flutterPhase * 0.7f) * 0.18f;
		const Vector2 drawPosition = {
			card.basePosition.x + flutterX,
			card.basePosition.y
		};

		ConfigureSprite(card.frameSprite.get(), drawPosition, drawRotation, { card.scale * flipScale, card.scale, 1.0f }, { 1.0f, 1.0f, 1.0f, card.alpha });
		ConfigureSprite(
			card.artSprite.get(),
			ApplyLocalOffset(drawPosition, { 0.0f, 28.0f }, drawRotation, card.scale, flipScale),
			drawRotation,
			{ card.scale * 0.34f * flipScale, card.scale * 0.34f, 1.0f },
			{ 1.0f, 1.0f, 1.0f, card.alpha });

		const Vector4 suitColor = GetSuitColor(card.suit, card.alpha);
		const Vector2 rankBase = { 330.0f, -380.0f };
		if (card.rank >= 10) {
			card.rankSprites[0]->SetTextureFilePath(GetRankTexturePath(1));
			card.rankSprites[1]->SetTextureFilePath(GetRankTexturePath(0));
			ConfigureSprite(card.rankSprites[0].get(), ApplyLocalOffset(drawPosition, { rankBase.x - 34.0f, rankBase.y }, drawRotation, card.scale, flipScale), drawRotation, { card.scale * 0.62f * flipScale, card.scale * 0.62f, 1.0f }, suitColor);
			ConfigureSprite(card.rankSprites[1].get(), ApplyLocalOffset(drawPosition, { rankBase.x + 34.0f, rankBase.y }, drawRotation, card.scale, flipScale), drawRotation, { card.scale * 0.62f * flipScale, card.scale * 0.62f, 1.0f }, suitColor);
		} else {
			card.rankSprites[0]->SetTextureFilePath(GetRankTexturePath(card.rank));
			ConfigureSprite(card.rankSprites[0].get(), ApplyLocalOffset(drawPosition, rankBase, drawRotation, card.scale, flipScale), drawRotation, { card.scale * 0.72f * flipScale, card.scale * 0.72f, 1.0f }, suitColor);
			HideSprite(card.rankSprites[1].get());
		}

		const Vector2 suitBase = { 360.0f, -275.0f };
		for (auto& suitSprite : card.suitSprites) {
			HideSprite(suitSprite.get());
		}

		switch (card.suit % 4) {
		case 1:
			ConfigureSprite(card.suitSprites[0].get(), ApplyLocalOffset(drawPosition, { suitBase.x - 32.0f, suitBase.y - 12.0f }, drawRotation, card.scale, flipScale), drawRotation, { card.scale * 64.0f * flipScale, card.scale * 64.0f, 1.0f }, suitColor);
			ConfigureSprite(card.suitSprites[1].get(), ApplyLocalOffset(drawPosition, { suitBase.x + 32.0f, suitBase.y - 12.0f }, drawRotation, card.scale, flipScale), drawRotation, { card.scale * 64.0f * flipScale, card.scale * 64.0f, 1.0f }, suitColor);
			ConfigureSprite(card.suitSprites[2].get(), ApplyLocalOffset(drawPosition, { suitBase.x, suitBase.y + 32.0f }, drawRotation, card.scale, flipScale), drawRotation + kPi * 0.25f, { card.scale * 78.0f * flipScale, card.scale * 78.0f, 1.0f }, suitColor);
			break;
		case 3:
			ConfigureSprite(card.suitSprites[0].get(), ApplyLocalOffset(drawPosition, { suitBase.x, suitBase.y - 30.0f }, drawRotation, card.scale, flipScale), drawRotation, { card.scale * 68.0f * flipScale, card.scale * 68.0f, 1.0f }, suitColor);
			ConfigureSprite(card.suitSprites[1].get(), ApplyLocalOffset(drawPosition, { suitBase.x - 36.0f, suitBase.y + 14.0f }, drawRotation, card.scale, flipScale), drawRotation, { card.scale * 68.0f * flipScale, card.scale * 68.0f, 1.0f }, suitColor);
			ConfigureSprite(card.suitSprites[2].get(), ApplyLocalOffset(drawPosition, { suitBase.x + 36.0f, suitBase.y + 14.0f }, drawRotation, card.scale, flipScale), drawRotation, { card.scale * 68.0f * flipScale, card.scale * 68.0f, 1.0f }, suitColor);
			ConfigureSprite(card.suitSprites[3].get(), ApplyLocalOffset(drawPosition, { suitBase.x, suitBase.y + 58.0f }, drawRotation, card.scale, flipScale), drawRotation, { card.scale * 24.0f * flipScale, card.scale * 62.0f, 1.0f }, suitColor);
			break;
		case 0:
			ConfigureSprite(card.suitSprites[0].get(), ApplyLocalOffset(drawPosition, { suitBase.x, suitBase.y - 12.0f }, drawRotation, card.scale, flipScale), drawRotation + kPi * 0.25f, { card.scale * 84.0f * flipScale, card.scale * 84.0f, 1.0f }, suitColor);
			ConfigureSprite(card.suitSprites[1].get(), ApplyLocalOffset(drawPosition, { suitBase.x, suitBase.y + 54.0f }, drawRotation, card.scale, flipScale), drawRotation, { card.scale * 24.0f * flipScale, card.scale * 70.0f, 1.0f }, suitColor);
			break;
		default:
			ConfigureSprite(card.suitSprites[0].get(), ApplyLocalOffset(drawPosition, suitBase, drawRotation, card.scale, flipScale), drawRotation + kPi * 0.25f, { card.scale * 94.0f * flipScale, card.scale * 94.0f, 1.0f }, suitColor);
			break;
		}

		if (card.basePosition.y > screenHeight_ + kDeleteMarginY) {
			card.alive = false;
		}
	}
}

void TitleFallingCardEffect::Draw(const Matrix4x4& view, const Matrix4x4& proj)
{
	for (auto& card : cards_) {
		if (!card.alive || !card.frameSprite || !card.artSprite) {
			continue;
		}
		card.frameSprite->Update(view, proj);
		card.frameSprite->Draw();

		card.artSprite->Update(view, proj);
		card.artSprite->Draw();

		for (auto& rankSprite : card.rankSprites) {
			if (rankSprite) {
				rankSprite->Update(view, proj);
				rankSprite->Draw();
			}
		}

		for (auto& suitSprite : card.suitSprites) {
			if (suitSprite) {
				suitSprite->Update(view, proj);
				suitSprite->Draw();
			}
		}
	}
}

void TitleFallingCardEffect::LoadCardVisuals_()
{
	cardVisuals_.clear();
	auto* textureManager = TextureManager::GetInstance();

	auto addVisual = [this, textureManager](const std::string& artTexturePath, int sourceIndex) {
		if (artTexturePath.empty()) {
			return;
		}

		textureManager->LoadTexture(artTexturePath);
		if (!textureManager->HasTexture(artTexturePath)) {
			return;
		}

		CardVisualDef visual{};
		visual.artTexturePath = artTexturePath;
		visual.rank = sourceIndex % 10 + 1;
		visual.suit = sourceIndex % 4;
		cardVisuals_.push_back(visual);
	};

	int sourceIndex = 0;
	for (const char* jsonPath : kCardDataJsons) {
		std::ifstream ifs(jsonPath);
		if (!ifs.is_open()) {
			continue;
		}

		nlohmann::json root;
		try {
			ifs >> root;
		} catch (...) {
			continue;
		}

		if (!root.contains("cards") || !root["cards"].is_array()) {
			continue;
		}

		for (const auto& jCard : root["cards"]) {
			const std::string artTexturePath = jCard.value("artTex", "");
			addVisual(artTexturePath, sourceIndex++);
		}
	}

	if (!cardVisuals_.empty()) {
		return;
	}

	for (const char* texturePath : kFallbackCardTextures) {
		addVisual(texturePath, sourceIndex++);
	}

	if (cardVisuals_.empty()) {
		textureManager->LoadTexture(kWhiteTexture);
		CardVisualDef visual{};
		visual.artTexturePath = kWhiteTexture;
		cardVisuals_.push_back(visual);
	}
}

void TitleFallingCardEffect::SpawnCard_(bool initialSpread)
{
	auto it = std::find_if(cards_.begin(), cards_.end(), [](const FallingCard& card) {
		return !card.alive;
	});
	if (it == cards_.end() || cardVisuals_.empty()) {
		return;
	}

	const int visualIndex = RandomInt_(0, static_cast<int>(cardVisuals_.size()) - 1);
	const CardVisualDef& visual = cardVisuals_[static_cast<std::size_t>(visualIndex)];
	it->artSprite->SetTextureFilePath(visual.artTexturePath);

	it->alive = true;
	it->basePosition = {
		RandomFloat_(-90.0f, screenWidth_ + 90.0f),
		initialSpread ? RandomFloat_(-screenHeight_ * 0.85f, screenHeight_ * 0.45f) : RandomFloat_(-170.0f, -50.0f)
	};
	it->velocity = {
		RandomFloat_(-18.0f, 18.0f),
		RandomFloat_(70.0f, 155.0f)
	};
	it->rotation = RandomFloat_(-kPi, kPi);
	it->rotationSpeed = RandomFloat_(-1.8f, 1.8f);
	it->flutterPhase = RandomFloat_(0.0f, kPi * 2.0f);
	it->flutterSpeed = RandomFloat_(1.8f, 4.2f);
	it->flutterAmount = RandomFloat_(16.0f, 46.0f);
	it->scale = RandomFloat_(0.115f, 0.18f);
	it->alpha = RandomFloat_(0.86f, 0.96f);
	it->visualIndex = visualIndex;
	it->rank = visual.rank;
	it->suit = visual.suit;
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
