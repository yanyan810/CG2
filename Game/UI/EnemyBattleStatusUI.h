#pragma once

#include "BloomConstantBuffer.h"
#include "MathStruct.h"
#include "Sprite.h"
#include "TextSprite.h"

#include <memory>
#include <string>
#include <vector>

class Enemy;
class GameApp;
struct EnemyAction;

class EnemyBattleStatusUI {
public:
	struct EnemyBloomSettings {
		bool intentEnabled = true;
		float intentIntensity = 1.8f;
		float intentMinPulse = 0.45f;
	};

	void Initialize(GameApp& app, size_t maxEnemies = 3);
	void Clear();

	void UpdateLayout(
		std::vector<Enemy>& enemies,
		const std::vector<int>& actionCounts,
		const std::vector<bool>& actedByCount,
		const Matrix4x4& view,
		const Matrix4x4& proj);

	void DrawGaugeAndIntent2D(
		GameApp& app,
		std::vector<Enemy>& enemies,
		const std::vector<bool>& actedByCount,
		float time,
		const Matrix4x4& view,
		const Matrix4x4& proj,
		const EnemyBloomSettings& bloomSettings);

	void DrawGaugeBloom(
		GameApp& app,
		std::vector<Enemy>& enemies,
		const Matrix4x4& view,
		const Matrix4x4& proj,
		float baseIntensity);

	void DrawHpTexts2D(const Matrix4x4& view, const Matrix4x4& proj);
	void DrawBcTexts2D(const Matrix4x4& view, const Matrix4x4& proj);

private:
	static std::wstring Utf8ToWString_(const std::string& text);
	static std::wstring GetIntentText_(const EnemyAction& action);
	static std::wstring GetHpText_(const Enemy& enemy);
	static void SetStatusItem_(
		Sprite* icon,
		TextSprite* text,
		int value,
		const Vector2& iconPosition,
		const Vector2& textPosition,
		const Vector3& textColor);
	static Vector3 GetIntentTextColor_(const std::string& type);
	static Vector4 GetIntentGlowColor_(const std::string& type);
	static BloomParam MakeIntentBloomParam_(
		const BloomParam& baseParam,
		const std::string& type,
		float time,
		const EnemyBloomSettings& settings);
	static BloomParam MakeHpGaugeBloomParam_(const BloomParam& baseParam, float intensity);
	static void DrawHealthGaugeBloom_(
		GameApp& app,
		Sprite* sprite,
		const Sprite::HealthGaugeParam& gaugeParam,
		const Matrix4x4& view,
		const Matrix4x4& proj,
		const BloomParam& bloomParam,
		float scalePulse = 1.0f);

	std::vector<std::unique_ptr<Sprite>> hpGauges_;
	std::vector<std::unique_ptr<Sprite>> intentIcons_;
	std::vector<std::unique_ptr<TextSprite>> intentTexts_;
	std::vector<std::unique_ptr<TextSprite>> intentCountTexts_;
	std::vector<std::unique_ptr<TextSprite>> hpTexts_;
	std::vector<std::unique_ptr<Sprite>> blockIcons_;
	std::vector<std::unique_ptr<Sprite>> poisonIcons_;
	std::vector<std::unique_ptr<Sprite>> frostIcons_;
	std::vector<std::unique_ptr<TextSprite>> blockTexts_;
	std::vector<std::unique_ptr<TextSprite>> poisonTexts_;
	std::vector<std::unique_ptr<TextSprite>> frostTexts_;
};
