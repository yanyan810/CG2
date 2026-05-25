#pragma once

#include "../UiLayout.h"

#include <algorithm>
#include <string>

class FieldUiPokerPreview {
public:
	enum class EffectKind {
		None,
		SingleDamage,
		AllDamage,
		Draw,
		Block,
		Heal
	};

	static EffectKind ClassifyEffectKind(const std::wstring& line)
	{
		if (line.find(L"敵単体に") != std::wstring::npos &&
			line.find(L"ダメージ") != std::wstring::npos) {
			return EffectKind::SingleDamage;
		}

		if (line.find(L"敵全体に") != std::wstring::npos &&
			line.find(L"ダメージ") != std::wstring::npos) {
			return EffectKind::AllDamage;
		}

		if (line.find(L"枚引く") != std::wstring::npos) {
			return EffectKind::Draw;
		}

		if (line.find(L"ブロック") != std::wstring::npos) {
			return EffectKind::Block;
		}

		if (line.find(L"回復") != std::wstring::npos) {
			return EffectKind::Heal;
		}

		return EffectKind::None;
	}

	static const UiPokerPreviewLineAnchor& GetEffectAnchor(
		EffectKind kind,
		const UiPokerPreviewEffectAnchors& anchors,
		int laneIndex)
	{
		laneIndex = (std::max)(0, (std::min)(laneIndex, 4));

		switch (kind) {
		case EffectKind::SingleDamage:
			return anchors.singleDamage.lanes[laneIndex];
		case EffectKind::AllDamage:
			return anchors.allDamage.lanes[laneIndex];
		case EffectKind::Draw:
			return anchors.draw.lanes[laneIndex];
		case EffectKind::Block:
			return anchors.block.lanes[laneIndex];
		case EffectKind::Heal:
			return anchors.heal.lanes[laneIndex];
		default:
			return anchors.none.lanes[laneIndex];
		}
	}
};
