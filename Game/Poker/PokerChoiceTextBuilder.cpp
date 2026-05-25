#include "PokerChoiceTextBuilder.h"
#include "PokerHandEvaluator.h"

#include <cstring>

std::wstring PokerChoiceTextBuilder::BuildChoiceUiText(ChoiceState state, PokerHandRank rank, const Bonus& bonus)
{
	if (state == ChoiceState::WaitingActivateChoice) {
		std::wstring text;
		text += L"ポーカー効果が発動可能です\n";
		text += L"役: ";
		text += GetHandNameText_(rank);
		text += L"\n";
		text += L"左クリック : 発動する\n";
		text += L"左クリック : 発動しない\n";
		text += L"左クリック : 場を見る\n";
		return text;
	}

	if (state == ChoiceState::WaitingEffectChoice) {
		std::wstring text;
		text += L"発動する効果を選んでください\n";
		text += L"左クリック : 戻る\n";
		text += L"左クリック : 次ターンATK UP (+" + std::to_wstring(bonus.atkUp) + L")\n";
		text += L"左クリック : " + std::to_wstring(bonus.drawCount) + L"枚ドロー\n";
		text += L"左クリック : " + std::to_wstring(bonus.damage) + L"ダメージ\n";
		text += L"左クリック : 場を見る\n";
		return text;
	}

	if (state == ChoiceState::ViewingBoard) {
		std::wstring text;
		text += L"場確認中\n";
		text += L"カードにマウスを乗せて確認できます\n";
		text += L"左クリック : 特殊効果選択に戻る\n";
		return text;
	}

	return L"";
}

std::wstring PokerChoiceTextBuilder::BuildEffectPreviewText(
	const Bonus& bonus,
	const std::vector<std::wstring>& turnStartLines,
	const std::vector<std::wstring>& pokerActivatedLines)
{
	std::wstring text;

	text += L"選択効果:\n";
	text += L"・このあと1つ選びます\n";
	text += L"  1. 次ターンATK UP +" + std::to_wstring(bonus.atkUp) + L"\n";
	text += L"  2. " + std::to_wstring(bonus.drawCount) + L"枚ドロー\n";
	text += L"  3. 敵単体に" + std::to_wstring(bonus.damage) + L"ダメージ\n";
	text += L"\n";

	text += L"ターン開始時:\n";
	AppendLinesOrNone_(text, turnStartLines);
	text += L"\n";

	text += L"特殊効果発動時:\n";
	AppendLinesOrNone_(text, pokerActivatedLines);

	return text;
}

std::wstring PokerChoiceTextBuilder::GetHandNameText_(PokerHandRank rank)
{
	const char* handName = PokerHandEvaluator::GetHandName(rank);
	return std::wstring(handName, handName + std::strlen(handName));
}

void PokerChoiceTextBuilder::AppendLinesOrNone_(std::wstring& text, const std::vector<std::wstring>& lines)
{
	if (lines.empty()) {
		text += L"・なし\n";
		return;
	}

	for (const auto& line : lines) {
		text += line + L"\n";
	}
}
