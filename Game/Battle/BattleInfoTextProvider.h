#pragma once

#include <cstddef>
#include <string>
#include <vector>

class EnemyManager;
class Player;
struct CardDef;
enum class PokerHandRank;

class BattleInfoTextProvider {
public:
	enum class OperationState {
		Basic,
		Preview,
		ChoosingFieldReplace,
	};

	enum class PokerChoiceState {
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

	struct ZoneCounts {
		int deck = 0;
		int hand = 0;
		int discard = 0;
		int field = 0;
	};

	struct TurnInfo {
		bool isPlayerTurn = true;
		int playerTurnCount = 0;
		int enemyTurnCount = 0;
	};

	static std::wstring BuildPokerChoiceUiText(PokerChoiceState state, PokerHandRank rank, const Bonus& bonus);
	static std::wstring BuildOperationText(OperationState state);
	static std::wstring BuildZoneCountText(const ZoneCounts& counts);
	static std::wstring BuildCurrentPokerHandText(PokerHandRank rank, std::size_t fieldCount);
	static std::wstring BuildTurnText(const TurnInfo& info);
	static std::wstring BuildEnergyText(int energy, int energyMax);
	static std::wstring BuildPlayerHpText(Player& player);
	static std::wstring BuildPlayerPowerBoostText(Player& player);
	static std::wstring BuildPlayerBlockText(Player& player);
	static std::vector<std::wstring> BuildEnemyHpTexts(EnemyManager* enemyMgr);
	static std::vector<std::wstring> BuildEnemyBcTexts(EnemyManager* enemyMgr);
	static std::wstring BuildPreviewCardDetailText(const CardDef* def);
	static std::wstring BuildPokerEffectPreviewText(
		const Bonus& bonus,
		const std::vector<std::wstring>& turnStartLines,
		const std::vector<std::wstring>& pokerActivatedLines);

private:
	static std::wstring GetPokerHandText_(PokerHandRank rank);
};