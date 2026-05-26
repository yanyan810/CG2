#include "EnemyBattleStatusUI.h"

#include "Enemy.h"
#include "GameApp.h"
#include "WinApp.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>

void EnemyBattleStatusUI::Initialize(GameApp& app, size_t maxEnemies)
{
	Clear();

	for (size_t i = 0; i < maxEnemies; ++i) {
		auto gauge = std::make_unique<Sprite>();
		gauge->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
		gauge->SetColor({ 0.2f, 0.2f, 0.2f, 1.0f });
		gauge->SetScale({ 0.0f, 0.0f, 1.0f });
		gauge->SetPosition({ 0.0f, 0.0f });
		hpGauges_.push_back(std::move(gauge));

		auto icon = std::make_unique<Sprite>();
		icon->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
		icon->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
		icon->SetScale({ 0.0f, 0.0f, 1.0f });
		icon->SetPosition({ 0.0f, 0.0f });
		intentIcons_.push_back(std::move(icon));

		auto intentText = std::make_unique<TextSprite>();
		intentText->Initialize(app.SpriteCom(), app.Dx());
		intentText->SetText(L"");
		intentText->SetFontSize(20);
		intentText->SetSize({ 0.7f, 0.7f, 1.0f });
		intentText->SetAlpha(1.0f);
		intentText->SetColor({ 1.0f, 1.0f, 1.0f });
		intentText->SetPosition({ 0.0f, 0.0f });
		intentTexts_.push_back(std::move(intentText));

		auto countText = std::make_unique<TextSprite>();
		countText->Initialize(app.SpriteCom(), app.Dx());
		countText->SetText(L"");
		countText->SetFontSize(24);
		countText->SetSize({ 0.75f, 0.75f, 1.0f });
		countText->SetAlpha(1.0f);
		countText->SetColor({ 1.0f, 0.92f, 0.35f });
		countText->SetPosition({ 0.0f, 0.0f });
		intentCountTexts_.push_back(std::move(countText));

		auto hpText = std::make_unique<TextSprite>();
		hpText->Initialize(app.SpriteCom(), app.Dx());
		hpText->SetSize({ 1.0f, 1.0f, 1.0f });
		hpText->SetPosition({ 1000.0f, 40.0f + (static_cast<float>(i) * 30.0f) });
		hpTexts_.push_back(std::move(hpText));

		auto bcText = std::make_unique<TextSprite>();
		bcText->Initialize(app.SpriteCom(), app.Dx());
		bcText->SetSize({ 1.0f, 1.0f, 1.0f });
		bcText->SetColor({ 0.5f, 0.0f, 0.5f });
		bcTexts_.push_back(std::move(bcText));
	}

	Matrix4x4 viewMat = Matrix4x4::MakeIdentity4x4();
	const float width = static_cast<float>(WinApp::kClientWidth);
	const float height = static_cast<float>(WinApp::kClientHeight);
	Matrix4x4 projMat = Matrix4x4::MakeOrthographicMatrix(width, height);

	for (auto& gauge : hpGauges_) { if (gauge) gauge->Update(viewMat, projMat); }
	for (auto& icon : intentIcons_) { if (icon) icon->Update(viewMat, projMat); }
	for (auto& text : intentTexts_) { if (text) text->Update(viewMat, projMat); }
	for (auto& text : intentCountTexts_) { if (text) text->Update(viewMat, projMat); }
	for (auto& text : hpTexts_) { if (text) text->Update(viewMat, projMat); }
	for (auto& text : bcTexts_) { if (text) text->Update(viewMat, projMat); }
}

void EnemyBattleStatusUI::Clear()
{
	hpGauges_.clear();
	intentIcons_.clear();
	intentTexts_.clear();
	intentCountTexts_.clear();
	hpTexts_.clear();
	bcTexts_.clear();
}

void EnemyBattleStatusUI::UpdateLayout(
	std::vector<Enemy>& enemies,
	const std::vector<int>& actionCounts,
	const std::vector<bool>& actedByCount,
	const Matrix4x4& view,
	const Matrix4x4& proj)
{
	for (size_t i = 0; i < hpGauges_.size(); ++i) {
		if (i < enemies.size() && enemies[i].IsAlive()) {
			const float gaugeWidth = 200.0f;
			const float posX = 1000.0f;
			const float posY = 40.0f + (static_cast<float>(i) * 30.0f);

			hpGauges_[i]->SetScale({ gaugeWidth, 15.0f, 1.0f });
			hpGauges_[i]->SetPosition({ posX, posY });

			EnemyAction nextAct = enemies[i].GetBossAI().GetNextAction();
			const bool hasIntent = !nextAct.type.empty() || !nextAct.name.empty();
			const bool acted = i < actedByCount.size() && actedByCount[i];

			if (hasIntent) {
				if (acted) {
					intentIcons_[i]->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
				} else if (nextAct.type == "Attack") {
					intentIcons_[i]->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f });
				} else if (nextAct.type == "Heal") {
					intentIcons_[i]->SetColor({ 0.2f, 1.0f, 0.2f, 1.0f });
				} else if (nextAct.type == "Block") {
					intentIcons_[i]->SetColor({ 0.25f, 0.55f, 1.0f, 1.0f });
				} else {
					intentIcons_[i]->SetColor({ 0.8f, 0.8f, 0.8f, 1.0f });
				}

				intentIcons_[i]->SetScale({ 20.0f, 20.0f, 1.0f });
				intentIcons_[i]->SetPosition({ posX - 30.0f, posY });

				if (i < intentTexts_.size() && intentTexts_[i]) {
					intentTexts_[i]->SetText(acted ? L"" : GetIntentText_(nextAct));
					intentTexts_[i]->SetColor(GetIntentTextColor_(nextAct.type));
					intentTexts_[i]->SetPosition({ posX - 100.0f, posY - 10.0f });
				}

				if (i < intentCountTexts_.size() && intentCountTexts_[i]) {
					const int count = i < actionCounts.size() ? actionCounts[i] : 0;
					if (!acted && count > 0) {
						intentCountTexts_[i]->SetText(std::to_wstring(count));
						intentCountTexts_[i]->SetColor({ 0.0f, 0.0f, 0.0f });
						intentCountTexts_[i]->SetPosition({ posX - 37.0f, posY - 12.3f });
					} else {
						intentCountTexts_[i]->SetText(L"");
					}
				}
			} else {
				intentIcons_[i]->SetScale({ 0.0f, 0.0f, 1.0f });
				if (i < intentTexts_.size() && intentTexts_[i]) {
					intentTexts_[i]->SetText(L"");
				}
				if (i < intentCountTexts_.size() && intentCountTexts_[i]) {
					intentCountTexts_[i]->SetText(L"");
				}
			}
		} else {
			if (hpGauges_[i]) {
				hpGauges_[i]->SetScale({ 0.0f, 0.0f, 1.0f });
			}
			if (i < intentIcons_.size() && intentIcons_[i]) {
				intentIcons_[i]->SetScale({ 0.0f, 0.0f, 1.0f });
			}
			if (i < intentTexts_.size() && intentTexts_[i]) {
				intentTexts_[i]->SetText(L"");
			}
			if (i < intentCountTexts_.size() && intentCountTexts_[i]) {
				intentCountTexts_[i]->SetText(L"");
			}
		}
	}

	size_t visibleRow = 0;
	for (const auto& enemy : enemies) {
		if (!enemy.IsAlive() || visibleRow >= hpTexts_.size()) {
			continue;
		}
		hpTexts_[visibleRow]->SetText(GetHpText_(enemy));
		hpTexts_[visibleRow]->SetPosition({ 1025.0f, 10.0f + (static_cast<float>(visibleRow) * 30.0f) });
		bcTexts_[visibleRow]->SetText(GetBcText_(enemy));
		bcTexts_[visibleRow]->SetPosition({ 1200.0f, 10.0f + (static_cast<float>(visibleRow) * 30.0f) });
		++visibleRow;
	}
	for (size_t i = visibleRow; i < hpTexts_.size(); ++i) {
		if (hpTexts_[i]) {
			hpTexts_[i]->SetText(L"");
		}
		if (i < bcTexts_.size() && bcTexts_[i]) {
			bcTexts_[i]->SetText(L"");
		}
	}

	for (auto& gauge : hpGauges_) { if (gauge) gauge->Update(view, proj); }
	for (auto& icon : intentIcons_) { if (icon) icon->Update(view, proj); }
	for (auto& text : intentTexts_) { if (text) text->Update(view, proj); }
	for (auto& text : intentCountTexts_) { if (text) text->Update(view, proj); }
	for (auto& text : hpTexts_) { if (text) text->Update(view, proj); }
	for (auto& text : bcTexts_) { if (text) text->Update(view, proj); }
}

void EnemyBattleStatusUI::DrawGaugeAndIntent2D(
	GameApp& app,
	std::vector<Enemy>& enemies,
	const std::vector<bool>& actedByCount,
	float time,
	const Matrix4x4& view,
	const Matrix4x4& proj,
	const EnemyBloomSettings& bloomSettings)
{
	for (size_t i = 0; i < hpGauges_.size(); ++i) {
		if (i >= enemies.size() || !hpGauges_[i] || !enemies[i].IsAlive()) {
			continue;
		}

		const float maxHp = static_cast<float>(std::max(1, enemies[i].GetMaxHP()));
		const float hpRatio = std::clamp(static_cast<float>(enemies[i].GetHP()) / maxHp, 0.0f, 1.0f);
		const float shieldEnd = std::clamp(
			hpRatio + static_cast<float>(std::max(0, enemies[i].GetBlock())) / maxHp,
			0.0f,
			1.0f
		);

		Sprite::HealthGaugeParam param{};
		param.hpColor = { 0.92f, 0.18f, 0.17f, 1.0f };
		param.damageColor = { 1.0f, 0.55f, 0.08f, 1.0f };
		param.shieldColor = { 0.16f, 0.42f, 1.0f, 0.95f };
		param.bgColor = { 0.10f, 0.05f, 0.05f, 0.82f };
		param.borderColor = { 0.94f, 0.92f, 0.82f, 1.0f };
		param.shadowColor = { 0.0f, 0.0f, 0.0f, 0.46f };
		param.hpRatio = hpRatio;
		param.damageStartRatio = hpRatio;
		param.damageEndRatio = hpRatio;
		param.shieldStartRatio = hpRatio;
		param.shieldEndRatio = shieldEnd;
		param.skew = 0.045f;
		param.borderWidth = 0.035f;
		param.blink = 0.0f;
		param.glow = 0.035f;
		param.alpha = 1.0f;
		hpGauges_[i]->SetHealthGaugeParam(param);
		hpGauges_[i]->DrawHealthGauge();
	}

	if (bloomSettings.intentEnabled) {
		for (size_t i = 0; i < intentIcons_.size(); ++i) {
			if (i >= enemies.size() || !intentIcons_[i] || !enemies[i].IsAlive()) {
				continue;
			}
			const bool acted = i < actedByCount.size() && actedByCount[i];
			if (acted) {
				continue;
			}

			const EnemyAction nextAct = enemies[i].GetBossAI().GetNextAction();
			if (nextAct.type.empty() && nextAct.name.empty()) {
				continue;
			}

			Sprite* icon = intentIcons_[i].get();
			const Vector3 originalScale = icon->GetScale();
			const Vector4 originalColor = icon->GetColor();
			const float scalePulse = 1.12f + 0.08f * (0.5f + 0.5f * std::sin(time * 4.2f));

			icon->SetScale({
				originalScale.x * scalePulse,
				originalScale.y * scalePulse,
				originalScale.z
				});
			icon->SetColor(GetIntentGlowColor_(nextAct.type));
			app.DrawSpriteObjectPost(
				icon,
				view,
				proj,
				MakeIntentBloomParam_(app.ObjectPost()->GetParam(), nextAct.type, time, bloomSettings)
			);
			icon->SetScale(originalScale);
			icon->SetColor(originalColor);
			icon->Update(view, proj);
		}
	}

	for (auto& icon : intentIcons_) {
		if (icon) icon->Draw();
	}
	for (auto& text : intentCountTexts_) {
		if (text) text->Draw();
	}
	for (auto& text : intentTexts_) {
		if (text) text->Draw();
	}
}

void EnemyBattleStatusUI::DrawGaugeBloom(
	GameApp& app,
	std::vector<Enemy>& enemies,
	const Matrix4x4& view,
	const Matrix4x4& proj,
	float baseIntensity)
{
	const BloomParam baseParam = app.ObjectPost()->GetParam();
	const BloomParam hpParam = MakeHpGaugeBloomParam_(baseParam, baseIntensity);
	const BloomParam shieldParam = MakeHpGaugeBloomParam_(baseParam, baseIntensity * 1.15f);

	for (size_t i = 0; i < hpGauges_.size(); ++i) {
		if (i >= enemies.size() || !hpGauges_[i] || !enemies[i].IsAlive()) {
			continue;
		}

		const float maxHp = static_cast<float>(std::max(1, enemies[i].GetMaxHP()));
		const float hpRatio = std::clamp(static_cast<float>(enemies[i].GetHP()) / maxHp, 0.0f, 1.0f);
		const float shieldEnd = std::clamp(
			hpRatio + static_cast<float>(std::max(0, enemies[i].GetBlock())) / maxHp,
			0.0f,
			1.0f
		);

		Sprite::HealthGaugeParam gauge{};
		gauge.hpColor = { 1.0f, 0.20f, 0.18f, 0.54f };
		gauge.damageColor = { 1.0f, 0.55f, 0.08f, 0.54f };
		gauge.shieldColor = { 0.18f, 0.48f, 1.0f, 0.48f };
		gauge.bgColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		gauge.borderColor = { 0.94f, 0.92f, 0.82f, 0.35f };
		gauge.shadowColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		gauge.hpRatio = hpRatio;
		gauge.damageStartRatio = hpRatio;
		gauge.damageEndRatio = hpRatio;
		gauge.shieldStartRatio = hpRatio;
		gauge.shieldEndRatio = shieldEnd;
		gauge.skew = 0.045f;
		gauge.borderWidth = 0.035f;
		gauge.blink = 0.0f;
		gauge.glow = 0.08f;
		gauge.alpha = 0.32f;
		DrawHealthGaugeBloom_(app, hpGauges_[i].get(), gauge, view, proj, shieldEnd > hpRatio ? shieldParam : hpParam, 1.01f);
	}
}

void EnemyBattleStatusUI::DrawHpTexts2D(const Matrix4x4& view, const Matrix4x4& proj)
{
	for (auto& text : hpTexts_) {
		if (!text) {
			continue;
		}
		text->Update(view, proj);
		text->Draw();
	}
}

void EnemyBattleStatusUI::DrawBcTexts2D(const Matrix4x4& view, const Matrix4x4& proj)
{
	for (auto& text : bcTexts_) {
		if (!text) {
			continue;
		}
		text->Update(view, proj);
		text->Draw();
	}
}

std::wstring EnemyBattleStatusUI::Utf8ToWString_(const std::string& text)
{
	if (text.empty()) {
		return L"";
	}

	auto convert = [&text](UINT codePage, DWORD flags) -> std::wstring {
		const int size = MultiByteToWideChar(codePage, flags, text.c_str(), -1, nullptr, 0);
		if (size <= 1) {
			return L"";
		}

		std::wstring out(static_cast<size_t>(size - 1), L'\0');
		const int converted = MultiByteToWideChar(codePage, flags, text.c_str(), -1, out.data(), size);
		if (converted <= 0) {
			return L"";
		}
		return out;
	};

	std::wstring out = convert(CP_UTF8, MB_ERR_INVALID_CHARS);
	if (!out.empty()) {
		return out;
	}
	return convert(CP_ACP, 0);
}

std::wstring EnemyBattleStatusUI::GetIntentText_(const EnemyAction& action)
{
	std::wstring text = Utf8ToWString_(action.name);
	if (text.empty()) {
		text = Utf8ToWString_(action.type);
	}
	if (!text.empty() && action.value > 0) {
		text += L" ";
		text += std::to_wstring(action.value);
	}
	return text;
}

std::wstring EnemyBattleStatusUI::GetHpText_(const Enemy& enemy)
{
	std::wstring text = std::to_wstring(enemy.GetHP()) + L" / " + std::to_wstring(enemy.GetMaxHP());
	if (enemy.GetBlock() > 0) {
		text += L"  B " + std::to_wstring(enemy.GetBlock());
	}
	return text;
}

std::wstring EnemyBattleStatusUI::GetBcText_(const Enemy& enemy)
{
	const int bcPoint = enemy.GetBCPoint();
	return bcPoint == 0 ? L"" : std::to_wstring(bcPoint);
}

Vector3 EnemyBattleStatusUI::GetIntentTextColor_(const std::string& type)
{
	if (type == "Attack") {
		return { 1.0f, 0.25f, 0.25f };
	}
	if (type == "Block" || type == "BlockAll" || type == "BlockLowestAlly") {
		return { 0.25f, 0.55f, 1.0f };
	}
	if (type == "Heal" || type == "HealAll" || type == "HealLowestAlly") {
		return { 0.25f, 1.0f, 0.35f };
	}
	return { 0.85f, 0.85f, 0.85f };
}

Vector4 EnemyBattleStatusUI::GetIntentGlowColor_(const std::string& type)
{
	if (type == "Attack") {
		return { 1.0f, 0.18f, 0.14f, 1.0f };
	}
	if (type == "Block" || type == "BlockAll" || type == "BlockLowestAlly") {
		return { 0.24f, 0.58f, 1.0f, 1.0f };
	}
	if (type == "Heal" || type == "HealAll" || type == "HealLowestAlly") {
		return { 0.20f, 1.0f, 0.38f, 1.0f };
	}
	return { 0.82f, 0.82f, 0.82f, 1.0f };
}

BloomParam EnemyBattleStatusUI::MakeIntentBloomParam_(
	const BloomParam& baseParam,
	const std::string& type,
	float time,
	const EnemyBloomSettings& settings)
{
	const float pulse = settings.intentMinPulse +
		(1.0f - settings.intentMinPulse) * (0.5f + 0.5f * std::sin(time * 4.2f));
	BloomParam param = baseParam;
	param.threshold = 0.0f;
	param.intensity = settings.intentIntensity * pulse;
	param.vignetteIntensity = 0.0f;
	param.vignetteScale = 0.0f;
	param.chromAbAmount = type == "Attack" ? 0.003f : 0.001f;
	param.distortionAmount = 0.0f;
	param.noiseIntensity = 0.0f;
	param.scanlineIntensity = 0.0f;
	param.curvature = 0.0f;
	param.borderSharp = 0.0f;
	param.glitchAmount = type == "Attack" ? 0.002f : 0.0f;
	param.dissolveAmount = -1.0f;
	return param;
}

BloomParam EnemyBattleStatusUI::MakeHpGaugeBloomParam_(const BloomParam& baseParam, float intensity)
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

void EnemyBattleStatusUI::DrawHealthGaugeBloom_(
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
