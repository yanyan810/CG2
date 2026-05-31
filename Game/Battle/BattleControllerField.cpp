#include "BattleController.h"
#include "Battle/BattleControllerShared.h"
#include "Battle/BattleFieldViewController.h"
#include "Battle/BattleDebugImGui.h"
#include "Battle/BattleRenderView.h"
#include "Battle/BattleInfoTextProvider.h"
#include "Battle/BattleCardInputController.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include "GameApp.h"
#include "WinApp.h"
#include <Windows.h>
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "MathStruct.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <set>
#include <random>
#include <filesystem>
#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include "Player.h"
#include "Enemy.h"

#include "Card/CardEffectTextBuilder.h"
#include "Card/CardEffectExecutor.h"
#include "FieldUi.h"
#include "Audio/BattleSfxPlayer.h"
#include "Camera.h"
#include "ModelParticleManager.h"
#include "Poker/PokerChoiceQuery.h"
#include "Poker/PokerChoiceController.h"
#include "Poker/PokerChoiceTextBuilder.h"

using namespace BattleControllerDetail;
void BattleController::RebuildFieldView_()
{
	const int n = (int)field_.size();
	if (n <= 0) {
		fieldViews_.clear();
		return;
	}

	while (fieldViews_.size() < field_.size()) {
		int i = (int)fieldViews_.size();
		const CardDef* def = db_.Find(field_[i].defId);

		auto card = std::make_unique<Card3D>();
		if (def) card->Initialize(objCom_, dx_, cam_, *def, field_[i]);
		card->SetIsHand(false);
		card->SetTransform({ 0.0f, -10.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.15f, 1.15f, 1.15f });
		fieldViews_.push_back(std::move(card));
	}

	const float y = fieldCardLayout_.y;
	const float z = fieldCardLayout_.z;
	const float gap = fieldCardLayout_.gap;
	const float startX = -gap * 0.5f * (n - 1);

	for (int i = 0; i < n; ++i) {
		if (!fieldViews_[i]) continue;

		Vector3 pos{ startX + gap * i, y, z };
		Vector3 rot{ 0.0f, 0.0f, 0.0f };
		Vector3 scl{ fieldCardLayout_.scale, fieldCardLayout_.scale, fieldCardLayout_.scale };

		fieldViews_[i]->SetTargetTransform(pos, rot, scl, false);

	}

	if (field_.size() == 5) {
		currentPoker_ = EvaluatePokerHand_();
	} else {
		currentPoker_.rank = PokerHandRank::None;
		currentPoker_.power = 0;
	}

	float intensity = 0.0f;
	if (currentPoker_.rank == PokerHandRank::None) {
		intensity = 0.0f;
	} else if (currentPoker_.rank <= PokerHandRank::TwoPair) {
		intensity = 5.0f;
	} else if (currentPoker_.rank <= PokerHandRank::FullHouse) {
		intensity = 10.0f;
	} else {
		intensity = 15.0f;
	}

	std::array<bool, 5> mask = GetPokerHighlightMask_();

	for (int i = 0; i < (int)fieldViews_.size(); ++i) {
		if (i < 5 && mask[i]) {
			fieldViews_[i]->SetGlitter(intensity);

			if (currentPoker_.rank != PokerHandRank::None) {
				// final frame color is applied in RefreshAllFieldCardTransforms_()
			}
		} else {
			fieldViews_[i]->SetGlitter(0.0f);
			// final frame color is applied in RefreshAllFieldCardTransforms_()
		}
	}

	fieldLayoutDirty_ = true;
	RefreshAllFieldCardTransforms_(0.0f);
}

void BattleController::UpdateFieldCardTransform_(int index, bool hovered, float dt)
{
	if (BattleFieldViewController::UpdateFieldCardTransform(
		fieldViews_,
		MakeFieldLayoutParams_(fieldCardLayout_),
		index,
		hovered,
		dt)) {
		fieldLayoutDirty_ = true;
	}
}

void BattleController::RefreshAllFieldCardTransforms_(float dt)
{
	BattleFieldViewController::TransformContext transformContext{};
	transformContext.fieldViews = &fieldViews_;
	transformContext.layout = MakeFieldLayoutParams_(fieldCardLayout_);
	transformContext.hoverIndex = fieldReplaceHoverIndex_;
	transformContext.choosingFieldReplace = cardState_ == CardInputState::ChoosingFieldReplace;
	transformContext.viewingBoardFromPokerUi = pokerChoiceState_ == PokerChoiceState::ViewingBoardFromPokerUi;
	if (BattleFieldViewController::RefreshFieldCardTransforms(transformContext, dt)) {
		fieldLayoutDirty_ = true;
	}

	if (field_.size() == 5) {
		currentPoker_ = EvaluatePokerHand_();
	} else {
		currentPoker_.rank = PokerHandRank::None;
		currentPoker_.power = 0;
	}

	UpdateFieldReplacePreviewEffects_();
	UpdateFieldFrameEffects_();
}

void BattleController::UpdateFieldFrameEffects_()
{
	BattleFieldViewController::FrameEffectContext frameContext{};
	frameContext.fieldViews = &fieldViews_;
	frameContext.currentRank = currentPoker_.rank;
	frameContext.pokerGlowTime = sPokerGlowRainbowTime;
	frameContext.highlightMask = GetPokerHighlightMask_();
	frameContext.inReplacePreview = cardState_ == CardInputState::ChoosingFieldReplace && hasPendingCard_;
	frameContext.replacePreviewRanks = &fieldReplacePreviewRanks_;
	frameContext.replacePreviewActive = &fieldReplacePreviewActive_;
	BattleFieldViewController::ApplyFieldFrameEffects(frameContext);
}

void BattleController::UpdateFieldReplacePreviewEffects_()
{
	BattleFieldViewController::ReplacePreviewContext context{};
	context.field = &field_;
	context.fieldViewCount = fieldViews_.size();
	context.pendingCard = &pendingCard_;
	context.choosingFieldReplace = cardState_ == CardInputState::ChoosingFieldReplace;
	context.hasPendingCard = hasPendingCard_;
	context.hoverIndex = fieldReplaceHoverIndex_;
	context.currentRank = currentPoker_.rank;

	const BattleFieldViewController::ReplacePreviewResult result =
		BattleFieldViewController::BuildFieldReplacePreview(context);
	fieldReplacePreviewRanks_ = result.ranks;
	fieldReplacePreviewActive_ = result.active;
}

void BattleController::EmitFieldCardGlitter_(float dt)
{
	BattleFieldViewController::GlitterContext context{};
	context.fieldViews = &fieldViews_;
	context.particleManager = fieldParticleManager_;
	context.emitTimer = &fieldCardGlitterEmitTimer_;
	context.dt = dt;
	context.emitInterval = sFieldCardGlitterEmitInterval;
	context.normalCount = sFieldCardGlitterNormalCount;
	context.highlightCount = sFieldCardGlitterHighlightCount;
	context.localOffset = &sFieldCardGlitterLocalOffset;
	context.spreadX = sFieldCardGlitterSpreadX;
	context.spreadY = sFieldCardGlitterSpreadY;
	context.currentRank = currentPoker_.rank;
	context.pokerGlowTime = sPokerGlowRainbowTime;
	context.highlightMask = GetPokerHighlightMask_();
	context.choosingFieldReplace = cardState_ == CardInputState::ChoosingFieldReplace;
	context.replacePreviewRanks = &fieldReplacePreviewRanks_;
	context.replacePreviewActive = &fieldReplacePreviewActive_;
	BattleFieldViewController::EmitFieldCardGlitter(context);
}
void BattleController::UpdateFieldEditorPreview(float dt)
{
	sPokerGlowRainbowTime += dt;
	RefreshAllFieldCardTransforms_(dt);

	for (auto& card : fieldViews_) {
		if (card) {
			card->Update(dt);
		}
	}

	if (propManager_) {
		propManager_->Update(dt);
	}
}

void BattleController::BuildFieldEditorPreview(int cardCount, int firstCardId)
{
	cardCount = std::clamp(cardCount, 0, 5);
	firstCardId = std::max(1, firstCardId);

	field_.clear();
	fieldViews_.clear();
	currentPoker_ = {};
	fieldReplacePreviewRanks_.clear();
	fieldReplacePreviewActive_.clear();
	cardState_ = CardInputState::Idle;
	pokerChoiceState_ = PokerChoiceState::None;

	for (int i = 0; i < cardCount; ++i) {
		int defId = firstCardId + i;
		if (!db_.Find(defId)) {
			defId = 1;
		}

		CardInstance card{};
		card.defId = defId;
		card.number = (i % 13) + 1;
		card.suit = static_cast<CardSuit>(i % 4);
		field_.push_back(card);
	}

	RebuildFieldView_();
}

void BattleController::DrawFieldEditorPreview3D(GameApp& app)
{
	(void)app;
	for (auto& card : fieldViews_) {
		if (card) {
			card->Draw();
		}
	}

	if (propManager_) {
		propManager_->Draw3D();
	}
}

void BattleController::DrawFieldEditorPostEffect3D(GameApp& app)
{
	if (propManager_) {
		propManager_->DrawPostEffect3D(app);
	}
}

bool BattleController::LoadFieldCardLayout(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open()) {
		return false;
	}

	nlohmann::json json;
	file >> json;

	if (json.contains("y")) fieldCardLayout_.y = json["y"].get<float>();
	if (json.contains("z")) fieldCardLayout_.z = json["z"].get<float>();
	if (json.contains("gap")) fieldCardLayout_.gap = json["gap"].get<float>();
	if (json.contains("scale")) fieldCardLayout_.scale = json["scale"].get<float>();
	if (json.contains("hoverYOffset")) fieldCardLayout_.hoverYOffset = json["hoverYOffset"].get<float>();
	if (json.contains("hoverZOffset")) fieldCardLayout_.hoverZOffset = json["hoverZOffset"].get<float>();
	if (json.contains("hoverScale")) fieldCardLayout_.hoverScale = json["hoverScale"].get<float>();

	fieldCardLayout_.gap = std::max(0.0f, fieldCardLayout_.gap);
	fieldCardLayout_.scale = std::max(0.01f, fieldCardLayout_.scale);
	fieldCardLayout_.hoverScale = std::max(0.01f, fieldCardLayout_.hoverScale);
	fieldLayoutDirty_ = true;
	RefreshAllFieldCardTransforms_(0.0f);
	return true;
}

bool BattleController::SaveFieldCardLayout(const std::string& path) const
{
	const std::filesystem::path filePath(path);
	const std::filesystem::path dir = filePath.parent_path();
	if (!dir.empty() && !std::filesystem::exists(dir)) {
		std::filesystem::create_directories(dir);
	}

	nlohmann::json json;
	json["y"] = fieldCardLayout_.y;
	json["z"] = fieldCardLayout_.z;
	json["gap"] = fieldCardLayout_.gap;
	json["scale"] = fieldCardLayout_.scale;
	json["hoverYOffset"] = fieldCardLayout_.hoverYOffset;
	json["hoverZOffset"] = fieldCardLayout_.hoverZOffset;
	json["hoverScale"] = fieldCardLayout_.hoverScale;

	std::ofstream file(path);
	if (!file.is_open()) {
		return false;
	}

	file << json.dump(4);
	return true;
}

#ifdef USE_IMGUI
void BattleController::DrawFieldSceneEditerImGui()
{
	ImGui::Begin("FieldSceneEditer");

	bool layoutChanged = false;
	layoutChanged |= ImGui::DragFloat("Field Y", &fieldCardLayout_.y, 0.05f);
	layoutChanged |= ImGui::DragFloat("Field Z", &fieldCardLayout_.z, 0.05f);
	layoutChanged |= ImGui::DragFloat("Card Gap", &fieldCardLayout_.gap, 0.05f, 0.0f, 20.0f);
	layoutChanged |= ImGui::DragFloat("Card Scale", &fieldCardLayout_.scale, 0.01f, 0.01f, 5.0f);
	layoutChanged |= ImGui::DragFloat("Hover Y Offset", &fieldCardLayout_.hoverYOffset, 0.01f);
	layoutChanged |= ImGui::DragFloat("Hover Z Offset", &fieldCardLayout_.hoverZOffset, 0.01f);
	layoutChanged |= ImGui::DragFloat("Hover Scale", &fieldCardLayout_.hoverScale, 0.01f, 0.01f, 5.0f);

	fieldCardLayout_.gap = std::max(0.0f, fieldCardLayout_.gap);
	fieldCardLayout_.scale = std::max(0.01f, fieldCardLayout_.scale);
	fieldCardLayout_.hoverScale = std::max(0.01f, fieldCardLayout_.hoverScale);

	if (layoutChanged) {
		fieldLayoutDirty_ = true;
		RefreshAllFieldCardTransforms_(0.0f);
	}

	if (ImGui::Button("Save Field Layout")) {
		SaveFieldCardLayout(fieldCardLayoutPath_);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Field Layout")) {
		LoadFieldCardLayout(fieldCardLayoutPath_);
	}

	ImGui::Separator();
	ImGui::Text("Preview Cards: %d", static_cast<int>(field_.size()));
	ImGui::Text("Layout File: %s", fieldCardLayoutPath_.c_str());

	if (propManager_) {
		ImGui::Separator();
		propManager_->DrawImGui();
	}

	ImGui::End();
}
#endif

uint64_t BattleController::BuildHandPokerPreviewSignature_() const
{
	uint64_t hash = 1469598103934665603ull;
	auto mix = [&hash](uint64_t value) {
		hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
		hash *= 1099511628211ull;
	};

	mix(sHandPokerPreviewEnabled ? 1ull : 0ull);
	mix(static_cast<uint64_t>(currentPoker_.rank));
	mix(static_cast<uint64_t>(field_.size()));
	for (const auto& card : field_) {
		mix(static_cast<uint64_t>(card.defId));
		mix(static_cast<uint64_t>(card.number));
		mix(static_cast<uint64_t>(card.suit));
	}

	const auto& hand = deckZone_.GetHand();
	mix(static_cast<uint64_t>(hand.size()));
	for (const auto& card : hand) {
		mix(static_cast<uint64_t>(card.defId));
		mix(static_cast<uint64_t>(card.number));
		mix(static_cast<uint64_t>(card.suit));
	}

	mix(static_cast<uint64_t>(handView_.GetCardCount()));
	mix(static_cast<uint64_t>(tutorialForcedEnemyTargetCardId_ + 1));
	return hash;
}

bool BattleController::IsTutorialForcedCardActive_() const
{
	return tutorialForcedEnemyTargetCardId_ > 0;
}

bool BattleController::IsTutorialForcedCardAllowed_(int handIndex) const
{
	if (!IsTutorialForcedCardActive_()) {
		return true;
	}

	const auto& hand = deckZone_.GetHand();
	if (handIndex < 0 || handIndex >= static_cast<int>(hand.size())) {
		return false;
	}

	return hand[handIndex].defId == tutorialForcedEnemyTargetCardId_;
}

void BattleController::UpdateHandPokerPreviewEffects_()
{
	const uint64_t signature = BuildHandPokerPreviewSignature_();
	if (signature == handPreviewSignature_) {
		return;
	}
	handPreviewSignature_ = signature;

	const auto& hand = deckZone_.GetHand();
	handPreviewRanks_.assign(hand.size(), PokerHandRank::None);

	if (IsTutorialForcedCardActive_()) {
		const int handCount = std::min<int>(static_cast<int>(hand.size()), handView_.GetCardCount());
		for (int handIndex = 0; handIndex < handCount; ++handIndex) {
			if (IsTutorialForcedCardAllowed_(handIndex)) {
				handView_.SetCardEffect(handIndex, { 1.0f, 0.86f, 0.18f, 1.0f }, 1.8f);
			} else {
				handView_.SetCardEffect(handIndex, { 0.18f, 0.18f, 0.20f, 0.55f }, 0.0f);
			}
		}
		return;
	}

	if (!sHandPokerPreviewEnabled || hand.empty()) {
		handView_.ClearCardEffects();
		return;
	}

	bool anyPreview = false;
	const int handCount = std::min<int>(static_cast<int>(hand.size()), handView_.GetCardCount());

	for (int handIndex = 0; handIndex < handCount; ++handIndex) {
		PokerHandRank bestRank = PokerHandRank::None;

		if (field_.size() < 5) {
			std::vector<CardInstance> candidate = field_;
			candidate.push_back(hand[handIndex]);
			if (candidate.size() == 5) {
				bestRank = EvaluatePokerHandForCards_(candidate).rank;
			}
		} else if (field_.size() == 5) {
			for (int replaceIndex = 0; replaceIndex < 5; ++replaceIndex) {
				std::vector<CardInstance> candidate = field_;
				candidate[replaceIndex] = hand[handIndex];
				const PokerHandRank rank = EvaluatePokerHandForCards_(candidate).rank;
				if (rank > bestRank) {
					bestRank = rank;
				}
			}
		}

		if (bestRank <= currentPoker_.rank) {
			bestRank = PokerHandRank::None;
		}

		handPreviewRanks_[handIndex] = bestRank;

		if (bestRank != PokerHandRank::None) {
			const Vector4 color = BattleFieldViewController::GetPokerFrameColor(bestRank, sPokerGlowRainbowTime);
			handView_.SetCardEffect(handIndex, color, BattleFieldViewController::GetPokerGlitterIntensity(bestRank));
			anyPreview = true;
		} else {
			handView_.ResetCardEffect(handIndex);
		}
	}

	if (!anyPreview) {
		handView_.ClearCardEffects();
	}
}

void BattleController::EmitHandCardGlitter_(float dt)
{
	if (!sHandPokerPreviewEnabled || handPreviewRanks_.empty()) {
		handCardGlitterEmitTimer_ = 0.0f;
		return;
	}

	handCardGlitterEmitTimer_ += dt;
	if (handCardGlitterEmitTimer_ < sHandCardGlitterEmitInterval) {
		return;
	}
	handCardGlitterEmitTimer_ = 0.0f;

	ModelParticleManager* particles = fieldParticleManager_;
	if (!particles) {
		return;
	}

	const int handCount = std::min<int>(static_cast<int>(handPreviewRanks_.size()), handView_.GetCardCount());
	for (int i = 0; i < handCount; ++i) {
		const PokerHandRank rank = handPreviewRanks_[i];
		if (rank == PokerHandRank::None) {
			continue;
		}

		Card3D* card = handView_.GetCard(i);
		if (!card) {
			continue;
		}

		const Vector4 color = BattleFieldViewController::GetPokerFrameColor(rank, sPokerGlowRainbowTime);
		const uint32_t emitCount = static_cast<uint32_t>(std::max(0, sHandCardGlitterCount));
		for (uint32_t emitIndex = 0; emitIndex < emitCount; ++emitIndex) {
			const Vector3 localOffset = {
				sFieldCardGlitterLocalOffset.x + Rand(-sFieldCardGlitterSpreadX, sFieldCardGlitterSpreadX),
				sFieldCardGlitterLocalOffset.y + Rand(-sFieldCardGlitterSpreadY, sFieldCardGlitterSpreadY),
				sFieldCardGlitterLocalOffset.z
			};
			particles->Emit("card_glitter", card->GetWorldPointFromLocal(localOffset), 1u, color);
		}
	}
}

int BattleController::PickFieldIndexByMouse_(int mouseX, int mouseY) const
{
	return BattleFieldViewController::PickFieldIndexByMouse(
		fieldViews_,
		*cam_,
		mouseX,
		mouseY,
		(float)WinApp::kClientWidth,
		(float)WinApp::kClientHeight);
}

