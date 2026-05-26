#pragma once

#include <string>

struct CardDef;
struct CardEffectDef;
struct CardSubEffectDef;
enum class SubEffectTrigger;

class CardEffectTextBuilder {
public:
	static std::wstring GetSubEffectTriggerText(SubEffectTrigger trigger);
	static std::wstring GetSubEffectConditionText(const CardSubEffectDef& sub);
	static std::wstring GetEffectValueText(const CardEffectDef& effect);
	static std::wstring GetBaseEffectSummaryText(const CardDef& def);
	static std::wstring BuildPreviewCardDetailText(const CardDef& def);
};
