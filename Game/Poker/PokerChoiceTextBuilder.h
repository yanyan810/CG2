#pragma once
#include <string>
#include <vector>

enum class PokerHandRank;

class PokerChoiceTextBuilder {
public:
	enum class ChoiceState {
		None,
		WaitingActivateChoice,
		WaitingEffectChoice,
		ViewingBoard,
	};

	struct Bonus {
		int atkUp = 0;
		int drawCount = 0;
		int damage = 0;
	};

	static std::wstring BuildChoiceUiText(ChoiceState state, PokerHandRank rank, const Bonus& bonus);
	static std::wstring BuildEffectPreviewText(
		const Bonus& bonus,
		const std::vector<std::wstring>& turnStartLines,
		const std::vector<std::wstring>& pokerActivatedLines);

private:
	static std::wstring GetHandNameText_(PokerHandRank rank);
	static void AppendLinesOrNone_(std::wstring& text, const std::vector<std::wstring>& lines);
};
