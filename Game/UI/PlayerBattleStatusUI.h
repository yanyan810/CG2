#pragma once

#include "BloomConstantBuffer.h"
#include "MathStruct.h"
#include "Sprite.h"
#include "TextSprite.h"

#include <array>
#include <memory>
#include <string>

class GameApp;

class PlayerBattleStatusUI {
public:
	void Initialize(GameApp& app);
	void SetTexts(const std::wstring& hpText, const std::wstring& blockText, const std::wstring& powerBoostText);
	void DrawHpGauge(
		const Matrix4x4& view,
		const Matrix4x4& proj,
		int hp,
		int maxHp,
		int block,
		int incomingDamage,
		float time,
		float damageBlinkSpeed);
	void DrawHpGaugeBloom(
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
		float damageBlinkSpeed);
	void DrawStatus2D(const Matrix4x4& view, const Matrix4x4& proj);

#ifdef USE_IMGUI
	void DrawImGuiControls();
#endif

private:
	static BloomParam MakeHpGaugeBloomParam_(const BloomParam& baseParam, float intensity);
	static void DrawHealthGaugeBloom_(
		GameApp& app,
		Sprite* sprite,
		const Sprite::HealthGaugeParam& gaugeParam,
		const Matrix4x4& view,
		const Matrix4x4& proj,
		const BloomParam& bloomParam,
		float scalePulse = 1.0f);

	void ApplyHpGaugeLayout_();
	void DrawOutlinedText_(
		TextSprite* text,
		std::array<std::unique_ptr<TextSprite>, 8>& outlines,
		const Matrix4x4& view,
		const Matrix4x4& proj,
		const Vector2& position,
		const Vector4& color,
		bool outlineEnabled,
		const Vector4& outlineColor,
		float outlineThickness);

	std::unique_ptr<Sprite> hpGauge_;
	Vector2 hpGaugePosition_{ 67.0f, 25.0f };
	Vector2 hpGaugeSize_{ 337.0f, 18.0f };

	std::unique_ptr<TextSprite> hpText_;
	std::array<std::unique_ptr<TextSprite>, 8> hpOutlineTexts_;
	Vector2 hpTextPosition_{ 172.0f, 14.0f };
	int hpTextFontSize_ = 28;
	Vector4 hpTextColor_{ 1.0f, 1.0f, 1.0f, 1.0f };
	bool hpOutlineEnabled_ = true;
	Vector4 hpOutlineColor_{ 0.0f, 0.0f, 0.0f, 1.0f };
	float hpOutlineThickness_ = 2.0f;

	std::unique_ptr<Sprite> powerBoostBg_;
	std::unique_ptr<TextSprite> powerBoostText_;
	Vector2 powerupUiPosition_{ 498.0f, 10.0f };
	Vector2 powerupUiSize_{ 48.0f, 48.0f };
	Vector2 powerBoostTextPosition_{ 510.0f, 18.0f };
	bool powerupUiVisible_ = true;

	std::unique_ptr<Sprite> blockBg_;
	std::unique_ptr<TextSprite> blockText_;
	std::array<std::unique_ptr<TextSprite>, 8> blockOutlineTexts_;
	Vector2 defenseUiPosition_{ 426.0f, 2.0f };
	Vector2 defenseUiSize_{ 64.0f, 64.0f };
	Vector2 blockTextPosition_{ 443.0f, 18.0f };
	int blockTextFontSize_ = 28;
	Vector4 blockTextColor_{ 1.0f, 1.0f, 1.0f, 1.0f };
	bool blockOutlineEnabled_ = true;
	Vector4 blockOutlineColor_{ 0.0f, 0.0f, 0.0f, 1.0f };
	float blockOutlineThickness_ = 2.0f;
};
