#include "Battle/BattleInfoTextProvider.h"

#include "Card/CardDef.h"
#include "Card/CardEffectTextBuilder.h"
#include "Enemy.h"
#include "Player.h"
#include "Poker/PokerChoiceTextBuilder.h"
#include "Poker/PokerHandEvaluator.h"

#include <string>

namespace {
	PokerChoiceTextBuilder::ChoiceState ToPokerChoiceTextBuilderState_(BattleInfoTextProvider::PokerChoiceState state)
	{
		switch (state) {
		case BattleInfoTextProvider::PokerChoiceState::WaitingActivateChoice:
			return PokerChoiceTextBuilder::ChoiceState::WaitingActivateChoice;
		case BattleInfoTextProvider::PokerChoiceState::WaitingEffectChoice:
			return PokerChoiceTextBuilder::ChoiceState::WaitingEffectChoice;
		case BattleInfoTextProvider::PokerChoiceState::ViewingBoard:
			return PokerChoiceTextBuilder::ChoiceState::ViewingBoard;
		case BattleInfoTextProvider::PokerChoiceState::None:
		default:
			return PokerChoiceTextBuilder::ChoiceState::None;
		}
	}
}

std::wstring BattleInfoTextProvider::BuildPokerChoiceUiText(PokerChoiceState state, PokerHandRank rank, const Bonus& bonus)
{
	return PokerChoiceTextBuilder::BuildChoiceUiText(
		ToPokerChoiceTextBuilderState_(state),
		rank,
		{ bonus.atkUp, bonus.drawCount, bonus.damage });
}

std::wstring BattleInfoTextProvider::BuildOperationText(OperationState state)
{
	if (state == OperationState::ChoosingFieldReplace) {
		std::wstring text;
		text += L"カード交換\n";
		text += L"左クリック : 選択中の場カードと入れ替える\n";
		text += L"右クリック : 入れ替えず墓地へ送る\n";
		return text;
	}

	if (state == OperationState::Preview) {
		std::wstring text;
		text += L"カード選択中\n";
		text += L"左クリック : 使用する\n";
		text += L"右クリック : キャンセル\n";
		text += L"Tab : 操作説明を表示\n";
		return text;
	}

	std::wstring text;
	text += L"基本操作\n";
	text += L"左クリック＋上ドラッグ : カードをプレビュー\n";
	text += L"プレビュー中に左クリック : カードを使用\n";
	text += L"プレビュー中に右クリック : キャンセル\n";
	text += L"Enter : ターン終了\n";
	text += L"Tab : 操作説明を表示\n";
	return text;
}

std::wstring BattleInfoTextProvider::BuildZoneCountText(const ZoneCounts& counts)
{
	std::wstring text;
	text += L"山札 : " + std::to_wstring(counts.deck) + L"\n";
	text += L"手札 : " + std::to_wstring(counts.hand) + L"\n";
	text += L"墓地 : " + std::to_wstring(counts.discard) + L"\n";
	text += L"場   : " + std::to_wstring(counts.field) + L"\n";
	return text;
}

std::wstring BattleInfoTextProvider::BuildCurrentPokerHandText(PokerHandRank rank, std::size_t fieldCount)
{
	if (fieldCount < 5) {
		return L"役:       判定中";
	}

	if (rank == PokerHandRank::None) {
		return L"役:       なし";
	}

	return GetPokerHandText_(rank);
}

std::wstring BattleInfoTextProvider::BuildTurnText(const TurnInfo& info)
{
	if (info.isPlayerTurn) {
		return L"あなたのターン : " + std::to_wstring(info.playerTurnCount);
	}

	return L"あいてのターン : " + std::to_wstring(info.enemyTurnCount);
}

std::wstring BattleInfoTextProvider::BuildEnergyText(int energy, int energyMax)
{
	std::wstring text;
	text += std::to_wstring(energy) + L" / " + std::to_wstring(energyMax);
	return text;
}

std::wstring BattleInfoTextProvider::BuildPlayerHpText(Player& player)
{
	std::wstring text;
	text = std::to_wstring(player.GetHP()) + L" / " + std::to_wstring(player.GetMaxHP());
	return text;
}

std::wstring BattleInfoTextProvider::BuildPlayerPowerBoostText(Player& player)
{
	std::wstring text;
	text = std::to_wstring(player.GetBoostedPower());
	return text;
}

std::wstring BattleInfoTextProvider::BuildPlayerBlockText(Player& player)
{
	std::wstring text;
	text = std::to_wstring(player.GetBlock());
	return text;
}

std::vector<std::wstring> BattleInfoTextProvider::BuildEnemyHpTexts(EnemyManager* enemyMgr)
{
	std::vector<std::wstring> hpTexts;
	if (!enemyMgr) {
		return hpTexts;
	}

	auto& enemies = enemyMgr->GetEnemies();
	for (const auto& enemy : enemies) {
		if (!enemy.IsAlive()) {
			continue;
		}
		std::wstring text = std::to_wstring(enemy.GetHP()) + L" / " + std::to_wstring(enemy.GetMaxHP());
		if (enemy.GetBlock() > 0) {
			text += L"  B " + std::to_wstring(enemy.GetBlock());
		}
		hpTexts.push_back(text);
	}
	return hpTexts;
}

std::vector<std::wstring> BattleInfoTextProvider::BuildEnemyBcTexts(EnemyManager* enemyMgr)
{
	std::vector<std::wstring> bcTexts;
	if (!enemyMgr) {
		return bcTexts;
	}

	auto& enemies = enemyMgr->GetEnemies();
	for (const auto& enemy : enemies) {
		if (!enemy.IsAlive()) {
			continue;
		}
		const int bcPoint = enemy.GetBCPoint();
		bcTexts.push_back(bcPoint == 0 ? L"" : std::to_wstring(bcPoint));
	}
	return bcTexts;
}

std::wstring BattleInfoTextProvider::BuildPreviewCardDetailText(const CardDef* def)
{
	if (!def) {
		return L"";
	}

	return CardEffectTextBuilder::BuildPreviewCardDetailText(*def);
}

std::wstring BattleInfoTextProvider::BuildPokerEffectPreviewText(
	const Bonus& bonus,
	const std::vector<std::wstring>& turnStartLines,
	const std::vector<std::wstring>& pokerActivatedLines)
{
	return PokerChoiceTextBuilder::BuildEffectPreviewText(
		{ bonus.atkUp, bonus.drawCount, bonus.damage },
		turnStartLines,
		pokerActivatedLines);
}

std::wstring BattleInfoTextProvider::GetPokerHandText_(PokerHandRank rank)
{
	switch (rank) {
	case PokerHandRank::OnePair: return L"役: ワンペア";
	case PokerHandRank::TwoPair: return L"役: ツーペア";
	case PokerHandRank::ThreeOfAKind: return L"役: スリーカード";
	case PokerHandRank::Straight: return L"役: ストレート";
	case PokerHandRank::Flush: return L"役: フラッシュ";
	case PokerHandRank::FullHouse: return L"役: フルハウス";
	case PokerHandRank::FourOfAKind: return L"役: フォーカード";
	case PokerHandRank::StraightFlush: return L"役: ストレートフラッシュ";
	case PokerHandRank::RoyalStraightFlush: return L"役: ロイヤルストレートフラッシュ";
	default: return L"役: ?";
	}
}