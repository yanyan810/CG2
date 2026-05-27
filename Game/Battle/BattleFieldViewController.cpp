#include "BattleFieldViewController.h"

#include "Camera.h"
#include "Card3D.h"
#include "MathStruct.h"
#include "ModelParticleManager.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
	Vector4 HsvToRgb_(float hue, float saturation, float value)
	{
		hue = std::fmod(hue, 360.0f);
		if (hue < 0.0f) {
			hue += 360.0f;
		}

		const float c = value * saturation;
		const float x = c * (1.0f - std::abs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
		const float m = value - c;

		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;

		if (hue < 60.0f) {
			r = c; g = x; b = 0.0f;
		} else if (hue < 120.0f) {
			r = x; g = c; b = 0.0f;
		} else if (hue < 180.0f) {
			r = 0.0f; g = c; b = x;
		} else if (hue < 240.0f) {
			r = 0.0f; g = x; b = c;
		} else if (hue < 300.0f) {
			r = x; g = 0.0f; b = c;
		} else {
			r = c; g = 0.0f; b = x;
		}

		return { r + m, g + m, b + m, 1.0f };
	}

	Vector4 LerpColor_(const Vector4& a, const Vector4& b, float t)
	{
		t = std::clamp(t, 0.0f, 1.0f);
		return {
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t,
			a.w + (b.w - a.w) * t
		};
	}

	PokerHandRank GetReplacePreviewRank_(const std::vector<PokerHandRank>* ranks, int index)
	{
		if (!ranks || index < 0 || index >= static_cast<int>(ranks->size())) {
			return PokerHandRank::None;
		}
		return (*ranks)[index];
	}

	bool IsReplacePreviewActive_(const std::vector<bool>* active, int index)
	{
		return active && index >= 0 && index < static_cast<int>(active->size()) && (*active)[index];
	}
}

int BattleFieldViewController::PickFieldIndexByMouse(
	const std::vector<std::unique_ptr<Card3D>>& fieldViews,
	const Camera& camera,
	int mouseX,
	int mouseY,
	float screenWidth,
	float screenHeight)
{
	const Matrix4x4& vp = camera.GetViewProjectionMatrix();

	int best = -1;
	float bestD2 = 80.0f * 80.0f;

	for (int i = 0; i < (int)fieldViews.size(); ++i) {
		Vector3 w = fieldViews[i]->GetWorldPos();

		Vector4 clip{};
		clip.x = w.x * vp.m[0][0] + w.y * vp.m[1][0] + w.z * vp.m[2][0] + 1.0f * vp.m[3][0];
		clip.y = w.x * vp.m[0][1] + w.y * vp.m[1][1] + w.z * vp.m[2][1] + 1.0f * vp.m[3][1];
		clip.z = w.x * vp.m[0][2] + w.y * vp.m[1][2] + w.z * vp.m[2][2] + 1.0f * vp.m[3][2];
		clip.w = w.x * vp.m[0][3] + w.y * vp.m[1][3] + w.z * vp.m[2][3] + 1.0f * vp.m[3][3];

		if (clip.w <= 0.0f) {
			continue;
		}

		const float ndcX = clip.x / clip.w;
		const float ndcY = clip.y / clip.w;

		const float sx = (ndcX * 0.5f + 0.5f) * screenWidth;
		const float sy = (-ndcY * 0.5f + 0.5f) * screenHeight;

		const float dx = sx - (float)mouseX;
		const float dy = sy - (float)mouseY;
		const float d2 = dx * dx + dy * dy;

		if (d2 < bestD2) {
			bestD2 = d2;
			best = i;
		}
	}

	return best;
}

bool BattleFieldViewController::UpdateFieldCardTransform(
	std::vector<std::unique_ptr<Card3D>>& fieldViews,
	const FieldLayoutParams& layout,
	int index,
	bool hovered,
	float dt)
{
	(void)dt;

	if (index < 0 || index >= static_cast<int>(fieldViews.size()) || !fieldViews[index]) {
		return false;
	}

	const int fieldCount = static_cast<int>(fieldViews.size());
	if (fieldCount <= 0) {
		return false;
	}

	const float startX = -layout.gap * 0.5f * (fieldCount - 1);

	Vector3 pos{ startX + layout.gap * index, layout.y, layout.z };
	Vector3 rot{ 0.0f, 0.0f, 0.0f };
	Vector3 scl{ layout.scale, layout.scale, layout.scale };

	if (hovered) {
		pos.y += layout.hoverYOffset;
		pos.z += layout.hoverZOffset;
		scl = { layout.hoverScale, layout.hoverScale, layout.hoverScale };
	}

	fieldViews[index]->SetTargetTransform(pos, rot, scl, false);

	Vector3 curPos = fieldViews[index]->GetWorldPos();
	const float distSq = (curPos.x - pos.x) * (curPos.x - pos.x) +
		(curPos.y - pos.y) * (curPos.y - pos.y) +
		(curPos.z - pos.z) * (curPos.z - pos.z);
	return distSq > 0.00001f;
}

bool BattleFieldViewController::RefreshFieldCardTransforms(const TransformContext& context, float dt)
{
	if (!context.fieldViews) {
		return false;
	}

	bool layoutDirty = false;
	for (int i = 0; i < static_cast<int>(context.fieldViews->size()); ++i) {
		const bool hovered =
			(context.choosingFieldReplace && i == context.hoverIndex) ||
			(context.viewingBoardFromPokerUi && i == context.hoverIndex);

		if (UpdateFieldCardTransform(*context.fieldViews, context.layout, i, hovered, dt)) {
			layoutDirty = true;
		}
	}

	return layoutDirty;
}

void BattleFieldViewController::ApplyFieldFrameEffects(const FrameEffectContext& context)
{
	if (!context.fieldViews) {
		return;
	}

	const Vector4 frameColor = GetPokerFrameColor(context.currentRank, context.pokerGlowTime);
	for (int i = 0; i < static_cast<int>(context.fieldViews->size()); ++i) {
		auto& card = (*context.fieldViews)[i];
		if (!card) {
			continue;
		}

		const PokerHandRank replacePreviewRank = GetReplacePreviewRank_(context.replacePreviewRanks, i);
		const bool replacePreviewActive = IsReplacePreviewActive_(context.replacePreviewActive, i);

		if (replacePreviewActive) {
			card->SetFrameColor(GetPokerTransitionColor(context.currentRank, replacePreviewRank, context.pokerGlowTime));
			const float previewIntensity = std::max(
				GetPokerGlitterIntensity(context.currentRank),
				GetPokerGlitterIntensity(replacePreviewRank));
			card->SetGlitter(previewIntensity > 0.0f ? previewIntensity : 4.0f);
		} else if (!context.inReplacePreview && i < 5 && context.highlightMask[i] && context.currentRank != PokerHandRank::None) {
			card->SetFrameColor(frameColor);
			card->SetGlitter(GetPokerGlitterIntensity(context.currentRank));
		} else {
			card->ResetFrameColor();
			card->SetGlitter(0.0f);
		}
	}
}

BattleFieldViewController::ReplacePreviewResult BattleFieldViewController::BuildFieldReplacePreview(const ReplacePreviewContext& context)
{
	ReplacePreviewResult result{};
	result.ranks.assign(context.fieldViewCount, PokerHandRank::None);
	result.active.assign(context.fieldViewCount, false);

	if (!context.choosingFieldReplace ||
		!context.hasPendingCard ||
		!context.field ||
		!context.pendingCard ||
		context.field->size() != 5 ||
		context.fieldViewCount < 5) {
		return result;
	}

	PokerHandRank bestRank = PokerHandRank::None;
	std::array<PokerHandRank, 5> ranks{};
	ranks.fill(PokerHandRank::None);

	for (int replaceIndex = 0; replaceIndex < 5; ++replaceIndex) {
		std::vector<CardInstance> candidate = *context.field;
		candidate[replaceIndex] = *context.pendingCard;
		const PokerHandRank rank = PokerHandEvaluator::Evaluate(candidate).rank;
		ranks[replaceIndex] = rank;
		if (rank > bestRank) {
			bestRank = rank;
		}
	}

	for (int replaceIndex = 0; replaceIndex < 5; ++replaceIndex) {
		result.ranks[replaceIndex] = ranks[replaceIndex];
	}

	if (context.hoverIndex >= 0 && context.hoverIndex < 5) {
		result.active[context.hoverIndex] = true;
		return result;
	}

	for (int replaceIndex = 0; replaceIndex < 5; ++replaceIndex) {
		if (ranks[replaceIndex] == bestRank && bestRank != PokerHandRank::None) {
			result.active[replaceIndex] = true;
		}
	}

	return result;
}

void BattleFieldViewController::EmitFieldCardGlitter(const GlitterContext& context)
{
	if (!context.fieldViews || context.fieldViews->empty()) {
		if (context.emitTimer) {
			*context.emitTimer = 0.0f;
		}
		return;
	}

	if (!context.emitTimer) {
		return;
	}

	*context.emitTimer += context.dt;
	if (*context.emitTimer < context.emitInterval) {
		return;
	}
	*context.emitTimer = 0.0f;

	ModelParticleManager* particles = context.particleManager;
	if (!particles || !context.localOffset) {
		return;
	}

	const bool hasPoker = context.currentRank != PokerHandRank::None;
	const Vector4 pokerColor = GetPokerFrameColor(context.currentRank, context.pokerGlowTime);
	const bool useReplacePreview = context.choosingFieldReplace &&
		context.replacePreviewRanks &&
		!context.replacePreviewRanks->empty();

	for (int i = 0; i < static_cast<int>(context.fieldViews->size()); ++i) {
		auto& card = (*context.fieldViews)[i];
		if (!card) {
			continue;
		}

		PokerHandRank replacePreviewRank = PokerHandRank::None;
		if (context.replacePreviewRanks && i < static_cast<int>(context.replacePreviewRanks->size())) {
			replacePreviewRank = (*context.replacePreviewRanks)[i];
		}
		const bool replacePreviewActive =
			context.replacePreviewActive &&
			i < static_cast<int>(context.replacePreviewActive->size()) &&
			(*context.replacePreviewActive)[i];

		const bool highlighted = replacePreviewActive || (hasPoker && i < 5 && context.highlightMask[i]);
		if (useReplacePreview && !replacePreviewActive) {
			continue;
		}

		const uint32_t emitCount = static_cast<uint32_t>(std::max(0, highlighted ? context.highlightCount : context.normalCount));
		const Vector4 highlightColor = replacePreviewActive
			? GetPokerTransitionColor(context.currentRank, replacePreviewRank, context.pokerGlowTime)
			: pokerColor;

		for (uint32_t emitIndex = 0; emitIndex < emitCount; ++emitIndex) {
			const Vector3 localOffset = {
				context.localOffset->x + Rand(-context.spreadX, context.spreadX),
				context.localOffset->y + Rand(-context.spreadY, context.spreadY),
				context.localOffset->z
			};
			Vector3 pos = card->GetWorldPointFromLocal(localOffset);
			const Vector4 color = highlighted ? highlightColor : Vector4{ 1.0f, 1.0f, 1.0f, 0.55f };
			particles->Emit("card_glitter", pos, 1u, color);
		}
	}
}

Vector4 BattleFieldViewController::GetPokerFrameColor(PokerHandRank rank, float time)
{
	switch (rank) {
	case PokerHandRank::OnePair:
	case PokerHandRank::TwoPair:
		return { 1.0f, 0.85f, 0.20f, 1.0f };

	case PokerHandRank::ThreeOfAKind:
	case PokerHandRank::Straight:
	case PokerHandRank::Flush:
		return { 0.25f, 0.95f, 0.35f, 1.0f };

	case PokerHandRank::FullHouse:
		return { 0.25f, 0.60f, 1.0f, 1.0f };

	case PokerHandRank::FourOfAKind:
	case PokerHandRank::StraightFlush:
		return { 1.0f, 0.25f, 0.20f, 1.0f };

	case PokerHandRank::RoyalStraightFlush:
		return HsvToRgb_(time * 120.0f, 0.9f, 1.0f);

	case PokerHandRank::None:
	default:
		return { 1.0f, 1.0f, 1.0f, 1.0f };
	}
}

Vector4 BattleFieldViewController::GetPokerTransitionColor(PokerHandRank beforeRank, PokerHandRank afterRank, float time)
{
	const Vector4 beforeColor = GetPokerFrameColor(beforeRank, time);
	const Vector4 afterColor = GetPokerFrameColor(afterRank, time);
	const float pulse = 0.5f + 0.5f * std::sin(time * 1.6f);
	return LerpColor_(beforeColor, afterColor, pulse);
}

float BattleFieldViewController::GetPokerGlitterIntensity(PokerHandRank rank)
{
	if (rank == PokerHandRank::None) {
		return 0.0f;
	}
	if (rank <= PokerHandRank::TwoPair) {
		return 5.0f;
	}
	if (rank <= PokerHandRank::FullHouse) {
		return 10.0f;
	}
	return 15.0f;
}
