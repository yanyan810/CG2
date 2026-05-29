#include "PlayerBattleStatusUI.h"

#include "GameApp.h"
#include "TextureManager.h"

#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

namespace {
	constexpr Vector2 kDefenseUiTextureSize{ 64.0f, 64.0f };
	constexpr Vector2 kPowerupUiTextureSize{ 48.0f, 48.0f };

	constexpr std::array<Vector2, 8> kOutlineDirections{
		Vector2{ -1.0f, 0.0f },
		Vector2{ 1.0f, 0.0f },
		Vector2{ 0.0f, -1.0f },
		Vector2{ 0.0f, 1.0f },
		Vector2{ -1.0f, -1.0f },
		Vector2{ 1.0f, -1.0f },
		Vector2{ -1.0f, 1.0f },
		Vector2{ 1.0f, 1.0f },
	};
}

void PlayerBattleStatusUI::Initialize(GameApp& app)
{
	hpGauge_ = std::make_unique<Sprite>();
	hpGauge_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	hpGauge_->SetColor({ 0.2f, 0.2f, 0.2f, 1.0f });
	ApplyHpGaugeLayout_();

	hpText_ = std::make_unique<TextSprite>();
	hpText_->Initialize(app.SpriteCom(), app.Dx());
	hpText_->SetFontSize(hpTextFontSize_);
	hpText_->SetSize({ 1.0f, 1.0f, 1.0f });
	hpText_->SetPosition(hpTextPosition_);
	for (auto& outlineText : hpOutlineTexts_) {
		outlineText = std::make_unique<TextSprite>();
		outlineText->Initialize(app.SpriteCom(), app.Dx());
		outlineText->SetFontSize(hpTextFontSize_);
		outlineText->SetSize({ 1.0f, 1.0f, 1.0f });
	}

	TextureManager::GetInstance()->LoadTexture("resources/ui/gauge/Powerup_UI.png");
	powerBoostBg_ = std::make_unique<Sprite>();
	powerBoostBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/gauge/Powerup_UI.png");
	powerBoostBg_->SetPosition(powerupUiPosition_);
	powerBoostBg_->SetScale({
		powerupUiSize_.x / kPowerupUiTextureSize.x,
		powerupUiSize_.y / kPowerupUiTextureSize.y,
		1.0f
		});
	powerBoostBg_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	powerBoostText_ = std::make_unique<TextSprite>();
	powerBoostText_->Initialize(app.SpriteCom(), app.Dx());
	powerBoostText_->SetSize({ 1.0f, 1.0f, 0.5f });
	powerBoostText_->SetPosition(powerBoostTextPosition_);

	TextureManager::GetInstance()->LoadTexture("resources/ui/gauge/Defense_UI.png");
	blockBg_ = std::make_unique<Sprite>();
	blockBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/gauge/Defense_UI.png");
	blockBg_->SetPosition(defenseUiPosition_);
	blockBg_->SetScale({
		defenseUiSize_.x / kDefenseUiTextureSize.x,
		defenseUiSize_.y / kDefenseUiTextureSize.y,
		1.0f
		});
	blockBg_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	blockText_ = std::make_unique<TextSprite>();
	blockText_->Initialize(app.SpriteCom(), app.Dx());
	blockText_->SetFontSize(blockTextFontSize_);
	blockText_->SetSize({ 1.0f, 1.0f, 0.5f });
	blockText_->SetPosition(blockTextPosition_);
	for (auto& outlineText : blockOutlineTexts_) {
		outlineText = std::make_unique<TextSprite>();
		outlineText->Initialize(app.SpriteCom(), app.Dx());
		outlineText->SetFontSize(blockTextFontSize_);
		outlineText->SetSize({ 1.0f, 1.0f, 0.5f });
	}
}

void PlayerBattleStatusUI::SetTexts(
	const std::wstring& hpText,
	const std::wstring& blockText,
	const std::wstring& powerBoostText)
{
	if (hpText_) {
		hpText_->SetText(hpText);
		for (auto& outlineText : hpOutlineTexts_) {
			if (outlineText) {
				outlineText->SetText(hpText);
			}
		}
	}

	if (blockText_) {
		blockText_->SetText(blockText);
		for (auto& outlineText : blockOutlineTexts_) {
			if (outlineText) {
				outlineText->SetText(blockText);
			}
		}
	}

	if (powerBoostText_) {
		powerBoostText_->SetText(powerBoostText);
	}
}

void PlayerBattleStatusUI::DrawHpGauge(
	int hp,
	int maxHp,
	int block,
	int incomingDamage,
	float time,
	float damageBlinkSpeed)
{
	if (!hpGauge_) {
		return;
	}

	const float maxHpValue = static_cast<float>(std::max(1, maxHp));
	const float currentRatio = std::clamp(static_cast<float>(hp) / maxHpValue, 0.0f, 1.0f);
	const int damage = std::max(0, incomingDamage);
	const float predictedRatio = std::clamp(
		(static_cast<float>(hp) - static_cast<float>(damage)) / maxHpValue,
		0.0f,
		currentRatio
	);
	const float shieldEnd = std::clamp(
		currentRatio + static_cast<float>(std::max(0, block)) / maxHpValue,
		0.0f,
		1.0f
	);
	const float blink = 0.5f + 0.5f * std::sin(time * damageBlinkSpeed);

	Sprite::HealthGaugeParam param{};
	param.hpColor = { 0.17f, 0.78f, 0.18f, 1.0f };
	param.damageColor = { 1.0f, 0.04f, 0.02f, 1.0f };
	param.shieldColor = { 0.13f, 0.40f, 1.0f, 0.95f };
	param.bgColor = { 0.04f, 0.07f, 0.04f, 0.82f };
	param.borderColor = { 0.96f, 0.95f, 0.86f, 1.0f };
	param.shadowColor = { 0.0f, 0.0f, 0.0f, 0.52f };
	param.hpRatio = damage > 0 ? predictedRatio : currentRatio;
	param.damageStartRatio = damage > 0 ? predictedRatio : currentRatio;
	param.damageEndRatio = currentRatio;
	param.shieldStartRatio = currentRatio;
	param.shieldEndRatio = shieldEnd;
	param.skew = 0.045f;
	param.borderWidth = 0.018f;
	param.blink = damage > 0 ? blink : 0.0f;
	param.glow = 0.04f;
	param.alpha = 1.0f;
	hpGauge_->SetHealthGaugeParam(param);
	hpGauge_->DrawHealthGauge();
}

void PlayerBattleStatusUI::DrawHpGaugeBloom(
	GameApp& app,
	const Matrix4x4& view,
	const Matrix4x4& proj,
	int hp,
	int maxHp,
	int block,
	int incomingDamage,
	float time,
	float hpBloomIntensity,
	float damageBloomIntensity,
	float damageBlinkSpeed)
{
	if (!hpGauge_) {
		return;
	}

	const float maxHpValue = static_cast<float>(std::max(1, maxHp));
	const float currentRatio = std::clamp(static_cast<float>(hp) / maxHpValue, 0.0f, 1.0f);
	const int damage = std::max(0, incomingDamage);
	const float predictedRatio = std::clamp(
		(static_cast<float>(hp) - static_cast<float>(damage)) / maxHpValue,
		0.0f,
		currentRatio
	);
	const float shieldEnd = std::clamp(
		currentRatio + static_cast<float>(std::max(0, block)) / maxHpValue,
		0.0f,
		1.0f
	);
	const float blink = 0.5f + 0.5f * std::sin(time * damageBlinkSpeed);

	Sprite::HealthGaugeParam gauge{};
	gauge.hpColor = { 0.25f, 1.0f, 0.28f, 0.60f };
	gauge.damageColor = { 1.0f, 0.02f, 0.02f, 0.86f };
	gauge.shieldColor = { 0.15f, 0.42f, 1.0f, 0.38f };
	gauge.bgColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	gauge.borderColor = { 0.9f, 0.95f, 0.86f, 0.42f };
	gauge.shadowColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	gauge.hpRatio = damage > 0 ? predictedRatio : currentRatio;
	gauge.damageStartRatio = damage > 0 ? predictedRatio : currentRatio;
	gauge.damageEndRatio = currentRatio;
	gauge.shieldStartRatio = currentRatio;
	gauge.shieldEndRatio = shieldEnd;
	gauge.skew = 0.045f;
	gauge.borderWidth = 0.018f;
	gauge.blink = damage > 0 ? blink : 0.0f;
	gauge.glow = 0.07f;
	gauge.alpha = 0.30f;

	const BloomParam baseParam = app.ObjectPost()->GetParam();
	const BloomParam bloomParam =
		damage > 0
		? MakeHpGaugeBloomParam_(baseParam, damageBloomIntensity * (0.65f + 0.35f * blink))
		: MakeHpGaugeBloomParam_(baseParam, hpBloomIntensity);
	DrawHealthGaugeBloom_(app, hpGauge_.get(), gauge, view, proj, bloomParam, 1.015f);
}

void PlayerBattleStatusUI::DrawStatus2D(const Matrix4x4& view, const Matrix4x4& proj)
{
	DrawOutlinedText_(
		hpText_.get(),
		hpOutlineTexts_,
		view,
		proj,
		hpTextPosition_,
		hpTextColor_,
		hpOutlineEnabled_,
		hpOutlineColor_,
		hpOutlineThickness_);

	if (powerupUiVisible_ && powerBoostText_) {
		if (powerBoostBg_) {
			powerBoostBg_->SetPosition(powerupUiPosition_);
			powerBoostBg_->SetScale({
				powerupUiSize_.x / kPowerupUiTextureSize.x,
				powerupUiSize_.y / kPowerupUiTextureSize.y,
				1.0f
				});
			powerBoostBg_->Update(view, proj);
			powerBoostBg_->Draw();
		}
		powerBoostText_->SetPosition(powerBoostTextPosition_);
		powerBoostText_->Update(view, proj);
		powerBoostText_->Draw();
	}

	if (blockText_) {
		if (blockBg_) {
			blockBg_->SetPosition(defenseUiPosition_);
			blockBg_->SetScale({
				defenseUiSize_.x / kDefenseUiTextureSize.x,
				defenseUiSize_.y / kDefenseUiTextureSize.y,
				1.0f
				});
			blockBg_->Update(view, proj);
			blockBg_->Draw();
		}

		DrawOutlinedText_(
			blockText_.get(),
			blockOutlineTexts_,
			view,
			proj,
			blockTextPosition_,
			blockTextColor_,
			blockOutlineEnabled_,
			blockOutlineColor_,
			blockOutlineThickness_);
	}
}

BloomParam PlayerBattleStatusUI::MakeHpGaugeBloomParam_(const BloomParam& baseParam, float intensity)
{
	BloomParam param = baseParam;
	param.threshold = 0.0f;
	param.intensity = intensity;
	param.vignetteIntensity = 0.0f;
	param.vignetteScale = 0.0f;
	param.chromAbAmount = 0.0f;
	param.distortionAmount = 0.0f;
	param.noiseIntensity = 0.0f;
	param.scanlineIntensity = 0.0f;
	param.curvature = 0.0f;
	param.borderSharp = 0.0f;
	param.glitchAmount = 0.0f;
	param.dissolveAmount = -1.0f;
	return param;
}

void PlayerBattleStatusUI::DrawHealthGaugeBloom_(
	GameApp& app,
	Sprite* sprite,
	const Sprite::HealthGaugeParam& gaugeParam,
	const Matrix4x4& view,
	const Matrix4x4& proj,
	const BloomParam& bloomParam,
	float scalePulse)
{
	if (!sprite) {
		return;
	}

	const Vector3 originalScale = sprite->GetScale();
	sprite->SetScale({
		originalScale.x * scalePulse,
		originalScale.y * scalePulse,
		originalScale.z
		});
	sprite->Update(view, proj);
	sprite->SetHealthGaugeParam(gaugeParam);
	app.ObjectPost()->SetParam(bloomParam);
	app.BeginObjectPostEffect();
	sprite->DrawHealthGauge();
	app.EndObjectPostEffect();
	app.SpriteCom()->SetGraphicsPipelineState();
	sprite->SetScale(originalScale);
	sprite->Update(view, proj);
}

void PlayerBattleStatusUI::ApplyHpGaugeLayout_()
{
	if (hpGauge_) {
		hpGauge_->SetPosition(hpGaugePosition_);
		hpGauge_->SetScale({ hpGaugeSize_.x, hpGaugeSize_.y, 1.0f });
	}
}

void PlayerBattleStatusUI::DrawOutlinedText_(
	TextSprite* text,
	std::array<std::unique_ptr<TextSprite>, 8>& outlines,
	const Matrix4x4& view,
	const Matrix4x4& proj,
	const Vector2& position,
	const Vector4& color,
	bool outlineEnabled,
	const Vector4& outlineColor,
	float outlineThickness)
{
	if (!text) {
		return;
	}

	text->SetPosition(position);
	text->SetColor({ color.x, color.y, color.z });
	text->SetAlpha(color.w);

	if (outlineEnabled) {
		for (size_t i = 0; i < outlines.size(); ++i) {
			auto& outlineText = outlines[i];
			if (!outlineText) {
				continue;
			}
			outlineText->SetPosition({
				position.x + kOutlineDirections[i].x * outlineThickness,
				position.y + kOutlineDirections[i].y * outlineThickness
				});
			outlineText->SetColor({
				outlineColor.x,
				outlineColor.y,
				outlineColor.z
				});
			outlineText->SetAlpha(outlineColor.w);
			outlineText->Update(view, proj);
			outlineText->Draw();
		}
	}

	text->Update(view, proj);
	text->Draw();
}

#ifdef USE_IMGUI
void PlayerBattleStatusUI::DrawImGuiControls()
{
	if (ImGui::CollapsingHeader("HP Gauge", ImGuiTreeNodeFlags_DefaultOpen)) {
		bool layoutChanged = false;
		layoutChanged |= ImGui::DragFloat2("HP Shader Position", &hpGaugePosition_.x, 1.0f);
		layoutChanged |= ImGui::DragFloat2("HP Shader Size", &hpGaugeSize_.x, 1.0f, 1.0f, 2000.0f);

		if (layoutChanged) {
			hpGaugeSize_.x = std::max(1.0f, hpGaugeSize_.x);
			hpGaugeSize_.y = std::max(1.0f, hpGaugeSize_.y);
			ApplyHpGaugeLayout_();
		}
	}

	if (ImGui::CollapsingHeader("HP Number", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat2("HP Number Position", &hpTextPosition_.x, 1.0f);
		if (ImGui::DragInt("HP Number Font Size", &hpTextFontSize_, 1.0f, 8, 128)) {
			hpTextFontSize_ = std::max(8, hpTextFontSize_);
			if (hpText_) {
				hpText_->SetFontSize(hpTextFontSize_);
			}
			for (auto& outlineText : hpOutlineTexts_) {
				if (outlineText) {
					outlineText->SetFontSize(hpTextFontSize_);
				}
			}
		}
		ImGui::ColorEdit4("HP Number Color", &hpTextColor_.x);
		ImGui::Checkbox("HP Outline Enabled", &hpOutlineEnabled_);
		ImGui::DragFloat("HP Outline Thickness", &hpOutlineThickness_, 0.1f, 0.0f, 12.0f);
		ImGui::ColorEdit4("HP Outline Color", &hpOutlineColor_.x);
	}

	if (ImGui::CollapsingHeader("Defense UI", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat2("Defense Image Position", &defenseUiPosition_.x, 1.0f);
		ImGui::DragFloat2("Defense Image Size", &defenseUiSize_.x, 1.0f, 1.0f, 512.0f);
		defenseUiSize_.x = std::max(1.0f, defenseUiSize_.x);
		defenseUiSize_.y = std::max(1.0f, defenseUiSize_.y);
		ImGui::DragFloat2("Defense Number Position", &blockTextPosition_.x, 1.0f);
		if (ImGui::DragInt("Defense Number Font Size", &blockTextFontSize_, 1.0f, 8, 128)) {
			blockTextFontSize_ = std::max(8, blockTextFontSize_);
			if (blockText_) {
				blockText_->SetFontSize(blockTextFontSize_);
			}
			for (auto& outlineText : blockOutlineTexts_) {
				if (outlineText) {
					outlineText->SetFontSize(blockTextFontSize_);
				}
			}
		}
		ImGui::ColorEdit4("Defense Number Color", &blockTextColor_.x);
		ImGui::Checkbox("Defense Outline Enabled", &blockOutlineEnabled_);
		ImGui::DragFloat("Defense Outline Thickness", &blockOutlineThickness_, 0.1f, 0.0f, 12.0f);
		ImGui::ColorEdit4("Defense Outline Color", &blockOutlineColor_.x);
	}

	if (ImGui::CollapsingHeader("Powerup UI", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Powerup UI Visible", &powerupUiVisible_);
		ImGui::DragFloat2("Powerup Image Position", &powerupUiPosition_.x, 1.0f);
		ImGui::DragFloat2("Powerup Image Size", &powerupUiSize_.x, 1.0f, 1.0f, 512.0f);
		powerupUiSize_.x = std::max(1.0f, powerupUiSize_.x);
		powerupUiSize_.y = std::max(1.0f, powerupUiSize_.y);
	}
}
#endif
