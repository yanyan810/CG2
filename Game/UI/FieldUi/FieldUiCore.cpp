#include "../FieldUi.h"
#include "FieldUiNumberSprites.h"
#include "FieldUiPokerPreview.h"
#include "GameApp.h"
#include "TextSprite.h"
#include "Sprite.h"
#include "WinApp.h"
#include "CardDatabase.h"
#include "GeometryGenerator.h"
#include "ModelManager.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

using json = nlohmann::json;

namespace {
	constexpr float kCostMeterCameraDistance = 10.0f;
	constexpr float kCostMeterCameraFovY = 0.45f;

	Model::ModelData MakeCostSphereModelData()
	{
		std::vector<Model::VertexData> vertices =
			GeometryGenerator::GenerateSphereTriList(24, 12, 1.0f);

		Model::ModelData md{};
		md.materials.push_back({ "" });

		Model::MeshData mesh{};
		mesh.materialIndex = 0;
		mesh.vertices = vertices;
		mesh.skinned = false;
		mesh.startVertex = 0;
		mesh.vertexCount = static_cast<uint32_t>(vertices.size());
		mesh.startIndex = 0;
		mesh.indexCount = static_cast<uint32_t>(vertices.size());

		md.meshes.push_back(std::move(mesh));
		md.indices.resize(vertices.size());
		for (uint32_t i = 0; i < static_cast<uint32_t>(vertices.size()); ++i) {
			md.indices[i] = i;
		}

		md.rootNode.name = "CostSphereRoot";
		md.rootNode.localMatrix = Matrix4x4::MakeIdentity4x4();
		md.rootNode.meshIndices.push_back(0);
		return md;
	}
}
static std::wstring Utf8ToWStringLocal(const std::string& s)
{
	if (s.empty()) return L"";
	int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring out(size - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), size);
	return out;
}

std::wstring FieldUi::Utf8ToWString_(const std::string& s)
{
	return Utf8ToWStringLocal(s);
}

FieldUi::PokerPreviewEffectKind FieldUi::ClassifyPreviewEffectKind_(const std::wstring& line) const
{
	switch (FieldUiPokerPreview::ClassifyEffectKind(line)) {
	case FieldUiPokerPreview::EffectKind::SingleDamage:
		return PokerPreviewEffectKind::SingleDamage;
	case FieldUiPokerPreview::EffectKind::AllDamage:
		return PokerPreviewEffectKind::AllDamage;
	case FieldUiPokerPreview::EffectKind::Draw:
		return PokerPreviewEffectKind::Draw;
	case FieldUiPokerPreview::EffectKind::Block:
		return PokerPreviewEffectKind::Block;
	case FieldUiPokerPreview::EffectKind::Heal:
		return PokerPreviewEffectKind::Heal;
	default:
		return PokerPreviewEffectKind::None;
	}
}

const UiPokerPreviewLineAnchor& FieldUi::GetPreviewEffectAnchor_(
    PokerPreviewEffectKind kind,
    const UiPokerPreviewEffectAnchors& anchors,
    int laneIndex) const
{
	FieldUiPokerPreview::EffectKind previewKind = FieldUiPokerPreview::EffectKind::None;
	switch (kind) {
	case PokerPreviewEffectKind::SingleDamage:
		previewKind = FieldUiPokerPreview::EffectKind::SingleDamage;
		break;
	case PokerPreviewEffectKind::AllDamage:
		previewKind = FieldUiPokerPreview::EffectKind::AllDamage;
		break;
	case PokerPreviewEffectKind::Draw:
		previewKind = FieldUiPokerPreview::EffectKind::Draw;
		break;
	case PokerPreviewEffectKind::Block:
		previewKind = FieldUiPokerPreview::EffectKind::Block;
		break;
	case PokerPreviewEffectKind::Heal:
		previewKind = FieldUiPokerPreview::EffectKind::Heal;
		break;
	default:
		break;
	}

	return FieldUiPokerPreview::GetEffectAnchor(previewKind, anchors, laneIndex);
}


void FieldUi::SetTextScale_(TextSprite* text, float s)
{
	if (!text) return;
	text->SetSize({ s, s, 1.0f });
}

Vector4 FieldUi::GetCostTierColor_(int value) const
{
	if (value <= 10) {
		return { 0.24f, 1.0f, 0.34f, 1.0f };
	}

	const int tier = (std::max)(0, (value - 1) / 10);
	switch (tier % 5) {
	case 1: return { 0.18f, 0.78f, 1.0f, 1.0f };
	case 2: return { 0.72f, 0.35f, 1.0f, 1.0f };
	case 3: return { 1.0f, 0.72f, 0.22f, 1.0f };
	case 4: return { 1.0f, 0.28f, 0.2f, 1.0f };
	default: return { 0.24f, 1.0f, 0.34f, 1.0f };
	}
}

Vector3 FieldUi::CostMeterScreenToWorld_(float x, float y) const
{
	const float visibleH =
		2.0f * kCostMeterCameraDistance * std::tanf(kCostMeterCameraFovY * 0.5f);
	const float visibleW =
		visibleH * (float(WinApp::kClientWidth) / float(WinApp::kClientHeight));

	return {
		(x / float(WinApp::kClientWidth) - 0.5f) * visibleW,
		(0.5f - y / float(WinApp::kClientHeight)) * visibleH,
		0.0f
	};
}

void FieldUi::RebuildCostSphereModel_()
{
	const std::string modelKey =
		"FieldUiCostSphere_Generated_" + std::to_string(costSphereModelRevision_++);
	costSphereModel_ = ModelManager::GetInstance()->CreatePrimitiveModel(
		modelKey,
		MakeCostSphereModelData());

	for (auto& sphere : costPipSpheres_) {
		if (sphere) {
			sphere->SetModel(costSphereModel_);
		}
	}
}

void FieldUi::UpdateCostMeter_(const BattleController& battle)
{
	const int current = (std::max)(0, battle.GetEnergy());
	const int maximum = (std::max)(0, battle.GetEnergyMax());
	const int visiblePips = (std::min)(10, current);
	const Vector4 tierColor = GetCostTierColor_(current);

	const float scale = (std::max)(0.1f, layout_.costMeter.pipScale);
	const float pipW = 14.0f * scale;
	const float pipH = 10.0f * scale;
	const float gapX = layout_.costMeter.pipGapX * scale;
	const float gapY = layout_.costMeter.pipGapY * scale;
	const float rightX = layout_.costMeter.pipOriginX;
	const float topY = layout_.costMeter.pipOriginY;
	const float midX = rightX - gapX;
	const float leftX = rightX - gapX * 2.0f;
	const float upperY = topY + gapY;
	const float middleY = topY + gapY * 2.0f;
	const float lowerY = topY + gapY * 3.0f;
	const float nearBottomY = topY + gapY * 4.0f;
	const float bottomY = topY + gapY * 5.0f;

	const Vector2 pipPositions[10] = {
		{ rightX, topY },
		{ midX, topY },
		{ leftX, topY },
		{ leftX, upperY },
		{ leftX, middleY },
		{ leftX, lowerY },
		{ leftX, nearBottomY },
		{ leftX, bottomY },
		{ midX, bottomY },
		{ rightX, bottomY }
	};

	if (costMeterCamera_) {
		costMeterCamera_->Update();
	}

	const float pixelToWorld =
		(2.0f * kCostMeterCameraDistance * std::tanf(kCostMeterCameraFovY * 0.5f)) /
		float(WinApp::kClientHeight);
	const float sphereRadius = layout_.costMeter.pipRadius * scale * pixelToWorld;
	const Vector4 emptyColor = {
		layout_.costMeter.emptyColorR,
		layout_.costMeter.emptyColorG,
		layout_.costMeter.emptyColorB,
		1.0f
	};
	const Vector4 lightColor = {
		layout_.costMeter.lightColorR,
		layout_.costMeter.lightColorG,
		layout_.costMeter.lightColorB,
		1.0f
	};

	for (int i = 0; i < 10; ++i) {
		const bool filled = i < visiblePips;
		const Vector2 pos = {
			pipPositions[i].x + layout_.costMeter.pipOffsets[i].x,
			pipPositions[i].y + layout_.costMeter.pipOffsets[i].y
		};
		const Vector3 worldPos =
			CostMeterScreenToWorld_(pos.x + pipW * 0.5f, pos.y + pipH * 0.5f);

		if (costPipSpheres_[i]) {
			costPipSpheres_[i]->SetTranslate(worldPos);
			costPipSpheres_[i]->SetScale({ sphereRadius, sphereRadius, sphereRadius });
			costPipSpheres_[i]->SetRotate({ 0.0f, 0.0f, 0.0f });
			costPipSpheres_[i]->SetMaterialColor(filled
				? Vector4{ tierColor.x, tierColor.y, tierColor.z, 1.0f }
				: emptyColor);
			costPipSpheres_[i]->SetLightColor(filled
				? lightColor
				: Vector4{ 0.35f, 0.5f, 0.42f, 1.0f });
			costPipSpheres_[i]->SetIntensity(filled
				? layout_.costMeter.filledLightIntensity
				: layout_.costMeter.emptyLightIntensity);
			costPipSpheres_[i]->Update(0.0f);
		}
	}

	if (costCurrentText_) {
		if (current != lastCostCurrent_) {
			costCurrentText_->SetText(std::to_wstring(current));
		}
		costCurrentText_->SetPosition({ layout_.costMeter.currentTextX, layout_.costMeter.currentTextY });
		costCurrentText_->SetColor({ tierColor.x, tierColor.y, tierColor.z });
		SetTextScale_(costCurrentText_.get(), layout_.costMeter.currentTextScale);
	}
	if (costMaxText_) {
		if (maximum != lastCostMax_) {
			costMaxText_->SetText(L"/\n" + std::to_wstring(maximum));
		}
		costMaxText_->SetPosition({ layout_.costMeter.maxTextX, layout_.costMeter.maxTextY });
		costMaxText_->SetColor({ 0.78f, 0.94f, 0.86f });
		SetTextScale_(costMaxText_.get(), layout_.costMeter.maxTextScale);
	}

	lastCostCurrent_ = current;
	lastCostMax_ = maximum;
}

void FieldUi::ApplyFieldUiLayout_()
{

	if (cardDescBg_) {
		cardDescBg_->SetPosition({ layout_.cardDescBg.x, layout_.cardDescBg.y });
		cardDescBg_->SetScale({ layout_.cardDescBg.w, layout_.cardDescBg.h, 1.0f });
	}
	if (cardDescText_) {
		cardDescText_->SetPosition({ layout_.cardDescText.x, layout_.cardDescText.y });
		SetTextScale_(cardDescText_.get(), layout_.cardDescText.scale);
	}

	if (deckCountBg_) {
		deckCountBg_->SetPosition({ layout_.deckBg.x, layout_.deckBg.y });
		deckCountBg_->SetScale({ layout_.deckBg.w, layout_.deckBg.h, 1.0f });
	}

	// デッキ
	if (deckCountText_ && deckLabelText_) {
		deckCountText_->SetPosition({
			layout_.deckLabelImage.x + deckCountTextLayout_.offsetX,
			layout_.deckLabelImage.y + deckCountTextLayout_.offsetY
			});
		SetTextScale_(deckCountText_.get(), deckCountTextLayout_.scale);
	}

	if (discardCountBg_) {
		discardCountBg_->SetPosition({ layout_.discardBg.x, layout_.discardBg.y });
		discardCountBg_->SetScale({ layout_.discardBg.w, layout_.discardBg.h, 1.0f });
	}

	// 墓地
	if (discardCountText_ && discardLabelText_) {
		discardCountText_->SetPosition({
			layout_.discardLabelImage.x + discardCountTextLayout_.offsetX,
			layout_.discardLabelImage.y + discardCountTextLayout_.offsetY
			});
		SetTextScale_(discardCountText_.get(), discardCountTextLayout_.scale);
	}

	if (handCountBg_) {
		handCountBg_->SetPosition({ layout_.handBg.x, layout_.handBg.y });
		handCountBg_->SetScale({ layout_.handBg.w, layout_.handBg.h, 1.0f });
	}
	
	// 手札
	if (handCountText_ && handLabelText_) {
		handCountText_->SetPosition({
			layout_.handLabelImage.x + handCountTextLayout_.offsetX,
			layout_.handLabelImage.y + handCountTextLayout_.offsetY
			});
		SetTextScale_(handCountText_.get(), handCountTextLayout_.scale);
	}

	if (fieldCountBg_) {
		fieldCountBg_->SetPosition({ layout_.fieldBg.x, layout_.fieldBg.y });
		fieldCountBg_->SetScale({ layout_.fieldBg.w, layout_.fieldBg.h, 1.0f });
	}
	if (fieldCountText_) {
		fieldCountText_->SetPosition({ layout_.fieldText.x, layout_.fieldText.y });
		SetTextScale_(fieldCountText_.get(), layout_.fieldText.scale);
	}

	if (turnTextBg_) {
		turnTextBg_->SetPosition({ layout_.turnBg.x, layout_.turnBg.y });
		turnTextBg_->SetScale({ layout_.turnBg.w, layout_.turnBg.h, 1.0f });
	}
	if (turnText_) {
		turnText_->SetPosition({ layout_.turnText.x, layout_.turnText.y });
		SetTextScale_(turnText_.get(), layout_.turnText.scale);
	}

	if (costTextBg_) {
		costTextBg_->SetPosition({ layout_.costBg.x, layout_.costBg.y });
		costTextBg_->SetScale({ layout_.costBg.w, layout_.costBg.h, 1.0f });
		costTextBg_->SetColor({ 0.f, 0.f, 0.f, 0.f });
	}
	if (costText_) {
		costText_->SetPosition({ layout_.costText.x, layout_.costText.y });
		SetTextScale_(costText_.get(), layout_.costText.scale);
		costText_->SetAlpha(0.0f);
	}

	if (modalOverlayBg_) {
		modalOverlayBg_->SetPosition({ layout_.overlay.x, layout_.overlay.y });
		modalOverlayBg_->SetScale({ layout_.overlay.w, layout_.overlay.h, 1.0f });
	}

	if (endTurnButtonBg_) {
		endTurnButtonBg_->SetPosition({ layout_.endTurnBg.x, layout_.endTurnBg.y });
		endTurnButtonBg_->SetScale({ layout_.endTurnBg.w, layout_.endTurnBg.h, 1.0f });
	}

	if (endTurnButtonText_) {
		endTurnButtonText_->SetPosition({ layout_.endTurnText.x, layout_.endTurnText.y });
		SetTextScale_(endTurnButtonText_.get(), layout_.endTurnText.scale);
	}

	if (deckLabelImage_) {
		deckLabelImage_->SetPosition({ layout_.deckLabelImage.x, layout_.deckLabelImage.y });
		deckLabelImage_->SetScale({ layout_.deckLabelImage.scale, layout_.deckLabelImage.scale, 1.0f });
	}

	if (discardLabelImage_) {
		discardLabelImage_->SetPosition({ layout_.discardLabelImage.x, layout_.discardLabelImage.y });
		discardLabelImage_->SetScale({ layout_.discardLabelImage.scale, layout_.discardLabelImage.scale, 1.0f });
	}

	if (handLabelImage_) {
		handLabelImage_->SetPosition({ layout_.handLabelImage.x, layout_.handLabelImage.y });
		handLabelImage_->SetScale({ layout_.handLabelImage.scale, layout_.handLabelImage.scale, 1.0f });
	}

	if (deckLabelText_) {
		deckLabelText_->SetPosition({ layout_.deckLabelImage.x, layout_.deckLabelImage.y });
		SetTextScale_(deckLabelText_.get(), layout_.deckLabelImage.scale);
	}

	if (discardLabelText_) {
		discardLabelText_->SetPosition({ layout_.discardLabelImage.x, layout_.discardLabelImage.y });
		SetTextScale_(discardLabelText_.get(), layout_.discardLabelImage.scale);
	}

	if (handLabelText_) {
		handLabelText_->SetPosition({ layout_.handLabelImage.x, layout_.handLabelImage.y });
		SetTextScale_(handLabelText_.get(), layout_.handLabelImage.scale);
	}

	for (auto& [id, sprite] : cardDescSpriteCache_) {
		if (!sprite) continue;
		sprite->SetPosition({ layout_.cardDescText.x, layout_.cardDescText.y });
		SetTextScale_(sprite.get(), layout_.cardDescText.scale);
	}
}

TextSprite* FieldUi::GetOrCreateCardDescSprite_(GameApp& app, const CardDef& def)
{
	auto it = cardDescSpriteCache_.find(def.id);
	if (it != cardDescSpriteCache_.end()) {
		return it->second.get();
	}

	auto sprite = std::make_unique<TextSprite>();
	sprite->Initialize(app.SpriteCom(), app.Dx());
	sprite->SetText(Utf8ToWString_(def.desc));
	sprite->SetPosition({ layout_.cardDescText.x, layout_.cardDescText.y });
	SetTextScale_(sprite.get(), layout_.cardDescText.scale);

	TextSprite* raw = sprite.get();
	cardDescSpriteCache_[def.id] = std::move(sprite);
	return raw;
}

void FieldUi::UpdateNumberSprites_(
	std::array<std::unique_ptr<Sprite>, kMaxUiDigits>& digits,
	int value, float x, float y, float scale, float spacing)
{
	FieldUiNumberSprites::Update(digits, value, x, y, scale, spacing);
}

void FieldUi::UpdatePokerEffectValueSprites_(const BattleController& battle)
{
	if (!battle.IsWaitingEffectChoice()) {
		for (auto& option : pokerEffectValueDigits_) {
			for (auto& d : option) {
				if (d) d->SetColor({ 1.f,1.f,1.f,0.f });
			}
		}
		return;
	}

	const BattleController::PokerBonus bonus = battle.GetCurrentPokerBonusForUi();

	// 表示順に合わせる
	// effect[0] = atkUp
	// effect[1] = damage
	// effect[2] = draw
	int effect1Value = bonus.atkUp;
	int effect2Value = bonus.damage;
	int effect3Value = bonus.drawCount;

	UpdateNumberSprites_(pokerEffectValueDigits_[0], effect1Value,
		pokerEffectLayout_.effectImages[0].x + numberLayout_.effectValue[0].offsetX,
		pokerEffectLayout_.effectImages[0].y + numberLayout_.effectValue[0].offsetY,
		numberLayout_.effectValue[0].scale,
		numberLayout_.effectValue[0].spacing);

	UpdateNumberSprites_(pokerEffectValueDigits_[1], effect2Value,
		pokerEffectLayout_.effectImages[1].x + numberLayout_.effectValue[1].offsetX,
		pokerEffectLayout_.effectImages[1].y + numberLayout_.effectValue[1].offsetY,
		numberLayout_.effectValue[1].scale,
		numberLayout_.effectValue[1].spacing);

	UpdateNumberSprites_(pokerEffectValueDigits_[2], effect3Value,
		pokerEffectLayout_.effectImages[2].x + numberLayout_.effectValue[2].offsetX,
		pokerEffectLayout_.effectImages[2].y + numberLayout_.effectValue[2].offsetY,
		numberLayout_.effectValue[2].scale,
		numberLayout_.effectValue[2].spacing);
}



const UiCardDescCustomLayout& FieldUi::GetCardDescCustomLayout_(int cardId) const
{
	auto it = perCardDescCustomLayouts_.find(cardId);
	if (it != perCardDescCustomLayouts_.end()) {
		return it->second;
	}
	return layout_.cardDescCustom;
}

UiCardDescCustomLayout& FieldUi::GetOrCreateCardDescCustomLayout_(int cardId)
{
	auto it = perCardDescCustomLayouts_.find(cardId);
	if (it != perCardDescCustomLayouts_.end()) {
		return it->second;
	}

	perCardDescCustomLayouts_[cardId] = layout_.cardDescCustom;
	return perCardDescCustomLayouts_[cardId];
}

bool FieldUi::IsCustomDescCardId_(int cardId) const
{
	return cardId == 6 || cardId == 17 || cardId == 18 || cardId == 19;
}

const UiCustomDescImageLayout& FieldUi::GetCustomDescImageLayout_(int cardId) const
{
	auto it = perCardCustomDescImageLayouts_.find(cardId);
	if (it != perCardCustomDescImageLayouts_.end()) {
		return it->second;
	}

	static UiCustomDescImageLayout defaultLayout{};
	defaultLayout.x = layout_.cardDescBg.x;
	defaultLayout.y = layout_.cardDescBg.y;
	defaultLayout.scaleX = 1.0f;
	defaultLayout.scaleY = 1.0f;
	return defaultLayout;
}

UiCustomDescImageLayout& FieldUi::GetOrCreateCustomDescImageLayout_(int cardId)
{
	auto it = perCardCustomDescImageLayouts_.find(cardId);
	if (it != perCardCustomDescImageLayouts_.end()) {
		return it->second;
	}

	UiCustomDescImageLayout layout{};
	layout.x = layout_.cardDescBg.x;
	layout.y = layout_.cardDescBg.y;
	layout.scaleX = 1.0f;
	layout.scaleY = 1.0f;

	perCardCustomDescImageLayouts_[cardId] = layout;
	return perCardCustomDescImageLayouts_[cardId];
}

void FieldUi::SetDebugPokerPreviewVisible(bool v)
{
	debugShowPokerPreview_ = v;
}

void FieldUi::SetDebugPokerPreviewData(const DebugPokerPreviewData& data)
{
	debugPokerPreviewData_ = data;
}

void FieldUi::ClearDebugPokerPreviewData()
{
	debugPokerPreviewData_ = DebugPokerPreviewData{};
}

void FieldUi::Initialize(GameApp& app)
{
	app_ = &app;

	cardDescText_ = std::make_unique<TextSprite>();
	cardDescText_->Initialize(app.SpriteCom(), app.Dx());

	cardDescBg_ = std::make_unique<Sprite>();
	cardDescBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	cardDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

	deckCountText_ = std::make_unique<TextSprite>();
	deckCountText_->Initialize(app.SpriteCom(), app.Dx());

	deckCountBg_ = std::make_unique<Sprite>();
	deckCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	deckCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

	discardCountText_ = std::make_unique<TextSprite>();
	discardCountText_->Initialize(app.SpriteCom(), app.Dx());

	discardCountBg_ = std::make_unique<Sprite>();
	discardCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	discardCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

	handCountText_ = std::make_unique<TextSprite>();
	handCountText_->Initialize(app.SpriteCom(), app.Dx());

	handCountBg_ = std::make_unique<Sprite>();
	handCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	handCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

	fieldCountText_ = std::make_unique<TextSprite>();
	fieldCountText_->Initialize(app.SpriteCom(), app.Dx());

	fieldCountBg_ = std::make_unique<Sprite>();
	fieldCountBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	fieldCountBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.45f });

	deckLabelText_ = std::make_unique<TextSprite>();
	deckLabelText_->Initialize(app.SpriteCom(), app.Dx());
	deckLabelText_->SetText(L"デッキ :");

	discardLabelText_ = std::make_unique<TextSprite>();
	discardLabelText_->Initialize(app.SpriteCom(), app.Dx());
	discardLabelText_->SetText(L"墓地 :");

	handLabelText_ = std::make_unique<TextSprite>();
	handLabelText_->Initialize(app.SpriteCom(), app.Dx());
	handLabelText_->SetText(L"手札 :");


	for (int i = 0; i < 5; ++i) {
		pokerOptionBgs_[i] = std::make_unique<Sprite>();
		pokerOptionBgs_[i]->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
		pokerOptionBgs_[i]->SetColor({ 0.0f, 0.0f, 0.0f, 0.65f });
		pokerOptionBgs_[i]->SetScale({ 380.0f, 130.0f, 1.0f });


	}

	turnText_ = std::make_unique<TextSprite>();
	turnText_->Initialize(app.SpriteCom(), app.Dx());

	turnTextBg_ = std::make_unique<Sprite>();
	turnTextBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	turnTextBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

	costText_ = std::make_unique<TextSprite>();
	costText_->Initialize(app.SpriteCom(), app.Dx());
	costText_->SetAlpha(0.0f);

	costCurrentText_ = std::make_unique<TextSprite>();
	costCurrentText_->Initialize(app.SpriteCom(), app.Dx());
	costCurrentText_->SetFontSize(26);

	costMaxText_ = std::make_unique<TextSprite>();
	costMaxText_->Initialize(app.SpriteCom(), app.Dx());
	costMaxText_->SetFontSize(19);

	costTextBg_ = std::make_unique<Sprite>();
	costTextBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	costTextBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });

	costMeterCamera_ = std::make_unique<Camera>();
	costMeterCamera_->SetFovY(kCostMeterCameraFovY);
	costMeterCamera_->SetAspect(float(WinApp::kClientWidth) / float(WinApp::kClientHeight));
	costMeterCamera_->SetNearZ(0.1f);
	costMeterCamera_->SetFarClip(100.0f);
	costMeterCamera_->SetTranslate({ 0.0f, 0.0f, -kCostMeterCameraDistance });
	costMeterCamera_->Update();

	RebuildCostSphereModel_();

	for (int i = 0; i < 10; ++i) {
		costPipSpheres_[i] = std::make_unique<Object3d>();
		costPipSpheres_[i]->Initialize(app.ObjCom(), app.Dx());
		costPipSpheres_[i]->SetModel(costSphereModel_);
		costPipSpheres_[i]->SetCamera(costMeterCamera_.get());
		costPipSpheres_[i]->SetEnableLighting(2);
		costPipSpheres_[i]->SetDirection({ -0.45f, -0.8f, -0.35f });
		costPipSpheres_[i]->SetShininess(96.0f);
	}

	modalOverlayBg_ = std::make_unique<Sprite>();
	modalOverlayBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	modalOverlayBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.38f });

	pokerPreviewText_ = std::make_unique<TextSprite>();
	pokerPreviewText_->Initialize(app.SpriteCom(), app.Dx());
	pokerPreviewText_->SetSize({ 0.9f, 0.9f, 1.0f });
	pokerPreviewText_->SetPosition({ 160.0f, 250.0f });


	pokerPreviewBg_ = std::make_unique<Sprite>();
	pokerPreviewBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	pokerPreviewBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.82f });

	clickChoiceText_ = std::make_unique<TextSprite>();
	clickChoiceText_->Initialize(app.SpriteCom(), app.Dx());
	clickChoiceText_->SetText(L"");
	clickChoiceText_->SetSize({ 1.0f, 1.0f, 1.0f });
	clickChoiceText_->SetPosition({ 435.f,500.f });

	clickChoiceBg_ = std::make_unique<Sprite>();
	clickChoiceBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	clickChoiceBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.82f });
	clickChoiceBg_->SetScale({ 400.f,50.f,1.f });
	clickChoiceBg_->SetPosition({ 435.f,500.f });

	pokerActivateDescBg_ = std::make_unique<Sprite>();
	pokerActivateDescBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	pokerActivateDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.88f });

	pokerEffectDescBg_ = std::make_unique<Sprite>();
	pokerEffectDescBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	pokerEffectDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.88f });

	//ターン終了ボタンは常に同じ位置に表示する
	endTurnButtonText_ = std::make_unique<TextSprite>();
	endTurnButtonText_->Initialize(app.SpriteCom(), app.Dx());
	endTurnButtonText_->SetText(L"End\nTurn");

	//ターン終了用背景
	endTurnButtonBg_ = std::make_unique<Sprite>();
	endTurnButtonBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	endTurnButtonBg_->SetColor({ 0.1f, 0.3f, 0.95f, 0.95f });

	

	pokerTitleText_ = std::make_unique<TextSprite>();
	pokerTitleText_->Initialize(app.SpriteCom(), app.Dx());

	pokerInfoButtonText_ = std::make_unique<TextSprite>();
	pokerInfoButtonText_->Initialize(app.SpriteCom(), app.Dx());

	for (int i = 0; i < 5; ++i) {
		pokerOptionTexts_[i] = std::make_unique<TextSprite>();
		pokerOptionTexts_[i]->Initialize(app.SpriteCom(), app.Dx());
	}

	//==================
	// レイアウトの読み込みと適用
	//==================
	LoadPokerEffectChoiceLayout(pokerEffectLayoutPath_);
	LoadFieldUiLayout(layoutPath_);
	LoadCardShowUiLayout(cardShowLayoutPath_);
	LoadUiNumberLayout(numberLayoutPath_);
	ApplyFieldUiLayout_();

	//	cachedPokerBonusRank_ = BattleController::PokerHandRank::None;
}


