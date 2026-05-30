#pragma once

#include "BattleController.h"
#include "Battle/BattleFieldViewController.h"
#include "Poker/PokerChoiceController.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <Windows.h>

namespace BattleControllerDetail {
inline float sPokerGlowRainbowTime = 0.0f;
inline Vector3 sFieldCardGlitterLocalOffset = { 0.0f, 3.0f, 0.0f };
inline float sFieldCardGlitterSpreadX = 2.0f;
inline float sFieldCardGlitterSpreadY = 0.0f;
inline float sFieldCardGlitterEmitInterval = 0.12f;
inline int sFieldCardGlitterNormalCount = 0;
inline int sFieldCardGlitterHighlightCount = 10;
inline bool sFieldFrameBloomEnabled = true;
inline float sFieldFrameBloomThreshold = 0.0f;
inline float sFieldFrameBloomIntensity = 1.1f;
inline float sFieldFrameBloomMinPulse = 0.0f;
inline float sFieldFrameBloomChromAb = 0.0f;
inline bool sHandPokerPreviewEnabled = true;
inline float sHandCardGlitterEmitInterval = 0.12f;
inline int sHandCardGlitterCount = 3;
inline float sHandFrameBloomIntensity = 1.1f;
inline bool sEnemyIntentBloomEnabled = true;
inline float sEnemyIntentBloomIntensity = 1.45f;
inline float sEnemyIntentBloomMinPulse = 0.45f;
inline bool sEnemyTargetBloomEnabled = true;
inline float sEnemyTargetBloomIntensity = 1.65f;
inline float sEnemyTargetBloomChromAb = 0.002f;
inline bool sHpGaugeBloomEnabled = true;
inline float sHpGaugeBloomIntensity = 0.42f;
inline float sHpGaugeBloomMinPulse = 0.65f;
inline float sHpDamageBlinkSpeed = 6.0f;
inline float sHpDamageBloomIntensity = 0.82f;
inline bool sPlayerBlockCarryOverEnabled = true;
inline float sPlayerBlockTurnDecayRate = 0.35f;
inline int sFrostBurstThreshold = 15;
inline int sFrostBurstMultiplier = 3;
inline float sStatusEffectApplyHeight = 1.35f;
inline float sStatusEffectIdleHeight = 1.28f;
inline float sStatusEffectCameraForwardOffset = 0.65f;

inline float EffectValueFloat_(const CardEffectDef& effect)
{
    if (effect.valueIsFloat) {
        return effect.valueFloat;
    }
    if (effect.valueFloat != 0.0f || effect.value == 0) {
        return effect.valueFloat;
    }
    return static_cast<float>(effect.value);
}

inline int EffectValueInt_(const CardEffectDef& effect)
{
    return std::max(0, static_cast<int>(std::lround(EffectValueFloat_(effect))));
}

inline int ScaleEffectAmount_(int baseValue, const CardEffectDef& effect)
{
    return std::max(0, static_cast<int>(std::lround(static_cast<float>(baseValue) * EffectValueFloat_(effect))));
}

inline std::wstring Utf8ToWString(const std::string& s)
{
    if (s.empty()) {
        return L"";
    }

    const int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (sizeNeeded <= 0) {
        return L"";
    }

    std::wstring result(sizeNeeded - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, result.data(), sizeNeeded);
    return result;
}

inline std::wstring FormatEffectValue_(const CardEffectDef& effect)
{
    const float value = EffectValueFloat_(effect);
    if (effect.valueIsFloat) {
        wchar_t buffer[32]{};
        swprintf_s(buffer, L"%.2f", value);
        std::wstring text = buffer;
        while (!text.empty() && text.back() == L'0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == L'.') {
            text.pop_back();
        }
        return text;
    }
    return std::to_wstring(EffectValueInt_(effect));
}

inline Vector4 HsvToRgb_(float hue, float saturation, float value)
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

inline BattleFieldViewController::FieldLayoutParams MakeFieldLayoutParams_(
    const BattleController::FieldCardLayout& layout)
{
    BattleFieldViewController::FieldLayoutParams params{};
    params.y = layout.y;
    params.z = layout.z;
    params.gap = layout.gap;
    params.scale = layout.scale;
    params.hoverYOffset = layout.hoverYOffset;
    params.hoverZOffset = layout.hoverZOffset;
    params.hoverScale = layout.hoverScale;
    return params;
}

inline BattleController::PokerMouseChoice ToPokerMouseChoice_(PokerChoiceController::Choice choice)
{
    switch (choice) {
    case PokerChoiceController::Choice::ActivateYes:       return BattleController::PokerMouseChoice::ActivateYes;
    case PokerChoiceController::Choice::ActivateNo:        return BattleController::PokerMouseChoice::ActivateNo;
    case PokerChoiceController::Choice::ActivateViewBoard: return BattleController::PokerMouseChoice::ActivateViewBoard;
    case PokerChoiceController::Choice::EffectAtkUp:       return BattleController::PokerMouseChoice::EffectAtkUp;
    case PokerChoiceController::Choice::EffectDraw:        return BattleController::PokerMouseChoice::EffectDraw;
    case PokerChoiceController::Choice::EffectDamage:      return BattleController::PokerMouseChoice::EffectDamage;
    case PokerChoiceController::Choice::EffectBack:        return BattleController::PokerMouseChoice::EffectBack;
    case PokerChoiceController::Choice::EffectViewBoard:   return BattleController::PokerMouseChoice::EffectViewBoard;
    case PokerChoiceController::Choice::ReturnFromBoard:   return BattleController::PokerMouseChoice::ReturnFromBoard;
    case PokerChoiceController::Choice::None:
    default:                                               return BattleController::PokerMouseChoice::None;
    }
}

inline BloomParam MakeEnemyTargetBloomParam_(const BloomParam& baseParam, float time)
{
    const float pulse = 0.72f + 0.28f * (0.5f + 0.5f * std::sin(time * 5.0f));
    BloomParam param = baseParam;
    param.threshold = 0.0f;
    param.intensity = sEnemyTargetBloomIntensity * pulse;
    param.vignetteIntensity = 0.0f;
    param.vignetteScale = 0.0f;
    param.chromAbAmount = sEnemyTargetBloomChromAb;
    param.distortionAmount = 0.0f;
    param.noiseIntensity = 0.0f;
    param.scanlineIntensity = 0.0f;
    param.curvature = 0.0f;
    param.borderSharp = 0.0f;
    param.glitchAmount = 0.0f;
    param.dissolveAmount = -1.0f;
    return param;
}
}
