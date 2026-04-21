#include "TutorialManager.h"
#include "BattleController.h"
#include <fstream>
#include <Windows.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::wstring TutorialManager::Utf8ToWString_(const std::string& s) {
	if (s.empty()) {
		return L"";
	}

	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	if (sizeNeeded <= 0) {
		return L"";
	}

	std::wstring result;
	result.resize(sizeNeeded - 1);
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, result.data(), sizeNeeded);
	return result;
}

const char* TutorialManager::StepToKey_(TutorialStep step) {
	switch (step) {
	case TutorialStep::Intro: return "Intro";
	case TutorialStep::HoverHand: return "HoverHand";
	case TutorialStep::PlayCard: return "PlayCard";
	case TutorialStep::ExplainEnergy: return "ExplainEnergy";
	case TutorialStep::FillField: return "FillField";
	case TutorialStep::EndPlayerTurn: return "EndPlayerTurn";
	case TutorialStep::WaitEnemyTurn: return "WaitEnemyTurn";
	case TutorialStep::ExplainPokerReady: return "ExplainPokerReady";
	case TutorialStep::ChoosePokerEffect: return "ChoosePokerEffect";
	case TutorialStep::SkipPokerContinueTurn: return "SkipPokerContinueTurn";
	case TutorialStep::SkipPokerEndTurn: return "SkipPokerEndTurn";
	case TutorialStep::SkipPokerWaitEnemyTurn: return "SkipPokerWaitEnemyTurn";
	case TutorialStep::ViewingBoardFromPoker: return "ViewingBoardFromPoker";
	case TutorialStep::EndAfterPoker: return "EndAfterPoker";
	case TutorialStep::UiPlayerHp: return "UiPlayerHp";
	case TutorialStep::UiEnemyHp: return "UiEnemyHp";
	case TutorialStep::UiTurnText: return "UiTurnText";
	case TutorialStep::UiHand: return "UiHand";
	case TutorialStep::UiField: return "UiField";
	case TutorialStep::UiRoleText: return "UiRoleText";
	case TutorialStep::UiEndTurn: return "UiEndTurn";
	case TutorialStep::UiDeckCount: return "UiDeckCount";
	case TutorialStep::UiFinished: return "UiFinished";
	case TutorialStep::Finished: return "Finished";
	default: return "";
	}
}

bool TutorialManager::LoadMessages(const std::string& path) {
	std::ifstream ifs(path);
	if (!ifs.is_open()) {
		return false;
	}

	json j;
	ifs >> j;

	messageTable_.clear();

	if (!j.contains("messages") || !j["messages"].is_object()) {
		return false;
	}

	for (auto it = j["messages"].begin(); it != j["messages"].end(); ++it) {
		if (it.value().is_string()) {
			messageTable_[it.key()] = Utf8ToWString_(it.value().get<std::string>());
		}
	}

	return true;
}

std::wstring TutorialManager::GetMessageFromTable_(TutorialStep step) const {
	const char* key = StepToKey_(step);
	auto it = messageTable_.find(key);
	if (it != messageTable_.end()) {
		return it->second;
	}
	return L"";
}

void TutorialManager::SetMessageForCurrentStep(const std::wstring& text) {
	message_ = text;
}

void TutorialManager::SetStepMessage(TutorialStep step, const std::wstring& text) {
	overrideMessages_[static_cast<int>(step)] = text;

	if (step_ == step) {
		message_ = text;
	}
}

std::wstring TutorialManager::GetStepMessage(TutorialStep step) const {
	auto it = overrideMessages_.find(static_cast<int>(step));
	if (it != overrideMessages_.end()) {
		return it->second;
	}

	return L"";
}

void TutorialManager::Initialize() {
	LoadMessages(messagePath_);
	Reset();
}

void TutorialManager::Reset() {
	step_ = TutorialStep::Intro;
	isActive_ = true;
	sawEnemyTurn_ = false;
	skippedPokerOnce_ = false;
	UpdateMessage_();
}

void TutorialManager::NextStep() {
	Advance_();
	UpdateMessage_();
}

void TutorialManager::Update(BattleController& battle) {
	if (!isActive_) {
		return;
	}

	switch (step_) {
	case TutorialStep::Intro:
		// 説明だけなので手動送り
		break;

	case TutorialStep::HoverHand:
		// 手札にホバーしたら進む
		if (battle.IsCardPreviewing()) {
			Advance_();
		}
		break;

	case TutorialStep::PlayCard:
		// まず1枚場に出したら進む
		if (battle.GetFieldCount() >= 1) {
			Advance_();
		}
		break;

	case TutorialStep::ExplainEnergy:
		// 説明だけなので手動送り
		break;

	case TutorialStep::FillField:
		// 場に5枚そろったら進む
		if (battle.GetFieldCount() >= 5) {
			Advance_();
		}
		break;

	case TutorialStep::EndPlayerTurn:
		// プレイヤーターンが終わって敵ターンに入ったら進む
		if (!battle.IsPlayerTurn()) {
			sawEnemyTurn_ = true;
			Advance_();
		}
		break;

	case TutorialStep::WaitEnemyTurn:
		// 敵ターンを経由して、次のプレイヤーターンが始まったら進む
		if (sawEnemyTurn_ && battle.IsPlayerTurn()) {
			Advance_();
		}
		break;

	case TutorialStep::ExplainPokerReady:
	{
		// 特殊効果UIが開いている間はこのステップを維持
		// 「発動する」を押して効果選択に入ったら次へ
		if (battle.IsWaitingEffectChoice()) {
			Advance_();
		}
	}
	break;

	case TutorialStep::ChoosePokerEffect:
		// 「場を見る」に入ったら、専用説明へ切り替える
		if (battle.IsViewingBoardFromPokerUi()) {
			step_ = TutorialStep::ViewingBoardFromPoker;
			UpdateMessage_();
			return;
		}

		// 特殊効果UIが閉じたら、実際の結果フラグで判定する
		if (!battle.HasPokerChoiceUi()) {
			auto result = battle.GetLastPokerTutorialResult();

			if (result == BattleController::PokerTutorialResult::Activated) {
				skippedPokerOnce_ = false;
				sawEnemyTurn_ = !battle.IsPlayerTurn();
				step_ = TutorialStep::EndAfterPoker;
				battle.ClearLastPokerTutorialResult();
				UpdateMessage_();
				return;
			} else if (result == BattleController::PokerTutorialResult::Skipped) {
				skippedPokerOnce_ = true;
				step_ = TutorialStep::SkipPokerContinueTurn;
				battle.ClearLastPokerTutorialResult();
				UpdateMessage_();
				return;
			}
		}
		break;


	case TutorialStep::SkipPokerContinueTurn:
		// 説明だけなので手動送り
		break;

	case TutorialStep::SkipPokerEndTurn:
		// 自分でターン終了したら進む
		if (!battle.IsPlayerTurn()) {
			sawEnemyTurn_ = true;
			Advance_();
		}
		break;

	case TutorialStep::SkipPokerWaitEnemyTurn:
		// 敵ターンを経て次の自分ターンになったら完了
		if (sawEnemyTurn_ && battle.IsPlayerTurn()) {
			step_ = TutorialStep::Finished;
			UpdateMessage_();
			return;
		}
		break;

	case TutorialStep::ViewingBoardFromPoker:
		// 場を見るを抜けたら、特殊効果選択に戻る
		if (!battle.IsViewingBoardFromPokerUi()) {
			step_ = TutorialStep::ChoosePokerEffect;
			UpdateMessage_();
			return;
		}
		break;

	case TutorialStep::EndAfterPoker:
		// 説明だけなので手動送り
		break;

	case TutorialStep::Finished:

		break;
	}

	UpdateMessage_();
}

void TutorialManager::Advance_() {
	// HoverHand の次は個別説明を飛ばして一括説明へ
	if (step_ == TutorialStep::HoverHand) {
		step_ = TutorialStep::ExplainCardAll;
		UpdateMessage_();
		return;
	}

	// ExplainCardAll の次は PlayCard
	if (step_ == TutorialStep::ExplainCardAll) {
		step_ = TutorialStep::PlayCard;
		UpdateMessage_();
		return;
	}

	step_ = static_cast<TutorialStep>(static_cast<int>(step_) + 1);

	// 不要ステップ飛ばす
	if (step_ == TutorialStep::SkipPokerContinueTurn ||
		step_ == TutorialStep::SkipPokerEndTurn ||
		step_ == TutorialStep::SkipPokerWaitEnemyTurn) {
		step_ = TutorialStep::EndAfterPoker;
	}

	UpdateMessage_();
}

void TutorialManager::UpdateMessage_() {
	auto it = overrideMessages_.find(static_cast<int>(step_));
	if (it != overrideMessages_.end() && !it->second.empty()) {
		message_ = it->second;
		return;
	}

	{
		std::wstring jsonMessage = GetMessageFromTable_(step_);
		if (!jsonMessage.empty()) {
			message_ = jsonMessage;
			return;
		}
	}
	switch (step_) {
	case TutorialStep::Intro:
		message_ = L"チュートリアルです\nまずは画面の見方から確認しましょう\nクリックで次へ進みます";
		break;

	case TutorialStep::HoverHand:
		message_ = L"下にあるのが手札です\nカードにマウスを乗せてみましょう";
		break;

	case TutorialStep::PlayCard:
		message_ = L"カードを1枚使って\n場に出してみましょう";
		break;

	case TutorialStep::ExplainCardAll:
		message_ = L"カードにはコスト・マーク・数字があります\n丸で示している場所を確認しましょう";
		break;

	case TutorialStep::ExplainEnergy:
		message_ = L"カードにはコストがあります\n使うと減り 足りないと使えません";
		break;

	case TutorialStep::FillField:
		message_ = L"場に5枚そろえると\nポーカー役が作れます\n5枚まで出してみましょう";
		break;

	case TutorialStep::EndPlayerTurn:
		message_ = L"5枚そろったらターン終了です\n敵ターンへ進めましょう";
		break;

	case TutorialStep::WaitEnemyTurn:
		message_ = L"今は敵のターンです\n次の自分のターンを待ちましょう";
		break;

	case TutorialStep::ExplainPokerReady:
		message_ = L"今回は発動するを選びましょう";
		break;

	case TutorialStep::ChoosePokerEffect:
		message_ = L"ダメージを選んでみましょう";
		break;

	case TutorialStep::EndAfterPoker:
		message_ = L"敵を倒しました！\nチュートリアルはこれで終了です\n左クリックでタイトルに戻ります";
		break;

	case TutorialStep::ViewingBoardFromPoker:
		message_ = L"場のカードを確認できます\n戻ると選択に戻ります";
		break;

	case TutorialStep::SkipPokerContinueTurn:
		message_ = L"今回は発動しないを選びました\nこの場合 特殊効果は使わず\nそのまま自分のターンを続けます";
		break;

	case TutorialStep::SkipPokerEndTurn:
		message_ = L"発動しなかった場合は\n次の自分のターンになるまで\n特殊効果は使えません\nターン終了してみましょう";
		break;

	case TutorialStep::SkipPokerWaitEnemyTurn:
		message_ = L"今は敵のターンです\n次の自分のターンになるまで\n特殊効果は再発動できません";
		break;
		
	case TutorialStep::UiPlayerHp:
		message_ = L"左上があなたのHPです\n0になると負けになります";
		break;

	case TutorialStep::UiEnemyIntentDamage:
		message_ = L"この赤い表示がある場合は予告ダメージです\n次の敵ターンで受けるダメージです";
		break;

	case TutorialStep::UiEnemyHp:
		message_ = L"右上が敵のHPです\n0にすると勝ちです";
		break;

	case TutorialStep::UiEnemyNextAction:
		message_ = L"この赤い四角は敵の次の行動です\n何をしてくるかの目印になります";
		break;

	case TutorialStep::UiTurnText:
		message_ = L"上中央には今のターンが表示されます\n自分のターンか確認しましょう";
		break;

	case TutorialStep::UiHand:
		message_ = L"下に並んでいるのが手札です\nここからカードを選びます";
		break;

	case TutorialStep::UiField:
		message_ = L"中央が場です\nここにカードを出して役を作ります";
		break;

	case TutorialStep::UiRoleText:
		message_ = L"ここに現在の役が表示されます\n5枚そろうと判定されます";
		break;

	case TutorialStep::UiEndTurn:
		message_ = L"行動が終わったら\n右の End Turn を押します";
		break;

	case TutorialStep::UiDeckCount:
		message_ = L"左下は残り枚数の表示です\n残りが少ないと選択肢も減ります";
		break;

	case TutorialStep::UiFinished:
		message_ = L"UIの説明は以上です\n左クリックで先へ進みましょう";
		break;


	case TutorialStep::Finished:
		message_ = L"チュートリアル完了です\n左クリックでタイトルに戻ります";
		break;
	}
}

bool TutorialManager::IsForceActivateOnly() const {
	return step_ == TutorialStep::ExplainPokerReady;
}

bool TutorialManager::IsForceDamageOnly() const {
	return step_ == TutorialStep::ChoosePokerEffect;
}

TutorialManager::FocusType TutorialManager::GetFocusType() const {
	switch (step_) {
	case TutorialStep::HoverHand:
		return FocusType::HandArea;

	case TutorialStep::PlayCard:
	case TutorialStep::FillField:
	case TutorialStep::ExplainPokerReady:
		return FocusType::None;

	case TutorialStep::ExplainEnergy:
		return FocusType::EnergyArea;

	case TutorialStep::EndPlayerTurn:
	case TutorialStep::SkipPokerEndTurn:
		return FocusType::EndTurnButtonArea;

	case TutorialStep::WaitEnemyTurn:
	case TutorialStep::SkipPokerWaitEnemyTurn:
	case TutorialStep::EndAfterPoker:
		return FocusType::None;

	case TutorialStep::ExplainCardAll:
		return FocusType::None;

	case TutorialStep::ChoosePokerEffect:
		return FocusType::None;

	case TutorialStep::ViewingBoardFromPoker:
		return FocusType::PokerBackButtonArea;

	case TutorialStep::SkipPokerContinueTurn:
		return FocusType::PokerViewBoardButtonArea;

	case TutorialStep::UiPlayerHp:
		return FocusType::PlayerHpArea;

	case TutorialStep::UiEnemyHp:
		return FocusType::EnemyHpArea;

	case TutorialStep::UiTurnText:
		return FocusType::TurnTextArea;

	case TutorialStep::UiHand:
		return FocusType::HandArea;

	case TutorialStep::UiField:
		return FocusType::FieldArea;

	case TutorialStep::UiRoleText:
		return FocusType::RoleTextArea;

	case TutorialStep::UiEndTurn:
		return FocusType::EndTurnButtonArea;

	case TutorialStep::UiDeckCount:
		return FocusType::DeckCountArea;

	case TutorialStep::UiFinished:
		return FocusType::None;

	case TutorialStep::UiEnemyIntentDamage:
		return FocusType::PlayerIncomingDamageArea;

	case TutorialStep::UiEnemyNextAction:
		return FocusType::EnemyNextActionArea;

	default:
		return FocusType::None;
	}
}

bool TutorialManager::ReloadMessages() {
	bool ok = LoadMessages(messagePath_);
	if (ok) {
		UpdateMessage_(); // 今のstepの表示を即更新
	}
	return ok;
}

bool TutorialManager::IsUiExplanationStep() const {
	switch (step_) {
	case TutorialStep::UiPlayerHp:
	case TutorialStep::UiEnemyHp:
	case TutorialStep::UiTurnText:
	case TutorialStep::UiHand:
	case TutorialStep::UiField:
	case TutorialStep::UiRoleText:
	case TutorialStep::UiEndTurn:
	case TutorialStep::UiDeckCount:
	case TutorialStep::UiEnemyIntentDamage:
	case TutorialStep::UiEnemyNextAction:
	case TutorialStep::UiFinished:
		return true;
	default:
		return false;
	}
}

bool TutorialManager::IsGameplayInputLocked() const {
	// UI説明中はゲーム側入力を止める
	if (IsUiExplanationStep()) {
		return true;
	}

	// 説明だけ読む系も必要ならここに追加
	switch (step_) {
	case TutorialStep::Intro:
	case TutorialStep::ExplainEnergy:
	case TutorialStep::SkipPokerContinueTurn:
	case TutorialStep::EndAfterPoker:
		return true;
	default:
		return false;
	}
}