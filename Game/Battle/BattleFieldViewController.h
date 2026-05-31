#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include "Poker/PokerHandEvaluator.h"

class Camera;
class Card3D;
class ModelParticleManager;
struct Vector3;
struct Vector4;

class BattleFieldViewController {
public:
	struct FieldLayoutParams {
		float y = -5.0f;
		float z = 5.0f;
		float gap = 5.0f;
		float scale = 1.15f;
		float hoverYOffset = 0.18f;
		float hoverZOffset = -0.08f;
		float hoverScale = 1.18f;
	};

	struct TransformContext {
		std::vector<std::unique_ptr<Card3D>>* fieldViews = nullptr;
		FieldLayoutParams layout{};
		int hoverIndex = -1;
		bool choosingFieldReplace = false;
		bool viewingBoardFromPokerUi = false;
	};

	struct FrameEffectContext {
		std::vector<std::unique_ptr<Card3D>>* fieldViews = nullptr;
		PokerHandRank currentRank = PokerHandRank::None;
		float pokerGlowTime = 0.0f;
		std::array<bool, 5> highlightMask{};
		bool inReplacePreview = false;
		const std::vector<PokerHandRank>* replacePreviewRanks = nullptr;
		const std::vector<bool>* replacePreviewActive = nullptr;
	};

	struct ReplacePreviewContext {
		const std::vector<CardInstance>* field = nullptr;
		size_t fieldViewCount = 0;
		const CardInstance* pendingCard = nullptr;
		bool choosingFieldReplace = false;
		bool hasPendingCard = false;
		int hoverIndex = -1;
		PokerHandRank currentRank = PokerHandRank::None;
	};

	struct ReplacePreviewResult {
		std::vector<PokerHandRank> ranks;
		std::vector<bool> active;
	};

	struct GlitterContext {
		std::vector<std::unique_ptr<Card3D>>* fieldViews = nullptr;
		ModelParticleManager* particleManager = nullptr;
		float* emitTimer = nullptr;
		float dt = 0.0f;
		float emitInterval = 0.0f;
		int normalCount = 0;
		int highlightCount = 0;
		const Vector3* localOffset = nullptr;
		float spreadX = 0.0f;
		float spreadY = 0.0f;
		PokerHandRank currentRank = PokerHandRank::None;
		float pokerGlowTime = 0.0f;
		std::array<bool, 5> highlightMask{};
		bool choosingFieldReplace = false;
		const std::vector<PokerHandRank>* replacePreviewRanks = nullptr;
		const std::vector<bool>* replacePreviewActive = nullptr;
	};

	static int PickFieldIndexByMouse(
		const std::vector<std::unique_ptr<Card3D>>& fieldViews,
		const Camera& camera,
		int mouseX,
		int mouseY,
		float screenWidth,
		float screenHeight);

	static bool UpdateFieldCardTransform(
		std::vector<std::unique_ptr<Card3D>>& fieldViews,
		const FieldLayoutParams& layout,
		int index,
		bool hovered,
		float dt);

	static bool RefreshFieldCardTransforms(const TransformContext& context, float dt);
	static void ApplyFieldFrameEffects(const FrameEffectContext& context);
	static ReplacePreviewResult BuildFieldReplacePreview(const ReplacePreviewContext& context);
	static void EmitFieldCardGlitter(const GlitterContext& context);

	static Vector4 GetPokerFrameColor(PokerHandRank rank, float time);
	static Vector4 GetPokerTransitionColor(PokerHandRank beforeRank, PokerHandRank afterRank, float time);
	static float GetPokerGlitterIntensity(PokerHandRank rank);
};
