#include "TutorialManager.h"
#include "BattleController.h"
#include <fstream>
#include <iomanip>
#include <Windows.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::wstring TutorialManager::Utf8ToWString(const std::string& s) {
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

std::string TutorialManager::WStringToUtf8(const std::wstring& s) {
	if (s.empty()) {
		return "";
	}

	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (sizeNeeded <= 0) {
		return "";
	}

	std::string result;
	result.resize(sizeNeeded - 1);
	WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, result.data(), sizeNeeded, nullptr, nullptr);
	return result;
}

const char* TutorialManager::StepToKey_(TutorialStep step) {
	switch (step) {
	case TutorialStep::Intro: return "Intro";
	case TutorialStep::UiPlayerBlock: return "UiPlayerBlock";
	case TutorialStep::UiPlayerPowerBoost: return "UiPlayerPowerBoost";
	case TutorialStep::HoverHand: return "HoverHand";
	case TutorialStep::ExplainCardCost: return "ExplainCardCost";
	case TutorialStep::ExplainCardSuit: return "ExplainCardSuit";
	case TutorialStep::ExplainCardNumber: return "ExplainCardNumber";
	case TutorialStep::ExplainCardAll: return "ExplainCardAll";
	case TutorialStep::PlayCard: return "PlayCard";
	case TutorialStep::ChooseEnemyTarget: return "ChooseEnemyTarget";
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
	case TutorialStep::UiEnemyIntentDamage: return "UiEnemyIntentDamage";
	case TutorialStep::UiEnemyHp: return "UiEnemyHp";
	case TutorialStep::UiEnemyNextAction: return "UiEnemyNextAction";
	case TutorialStep::UiTurnText: return "UiTurnText";
	case TutorialStep::UiHand: return "UiHand";
	case TutorialStep::UiField: return "UiField";
	case TutorialStep::UiRoleText: return "UiRoleText";
	case TutorialStep::UiEndTurn: return "UiEndTurn";
	case TutorialStep::UiDeckCount: return "UiDeckCount";
	case TutorialStep::UiPokerHandHelp: return "UiPokerHandHelp";
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
			messageTable_[it.key()] = Utf8ToWString(it.value().get<std::string>());
		}
	}

	return true;
}

bool TutorialManager::SaveMessages(const std::string& path) const {
	const std::string outPath = path.empty() ? messagePath_ : path;
	std::ofstream ofs(outPath);
	if (!ofs.is_open()) {
		return false;
	}

	json messages = json::object();
	for (const auto& [key, text] : messageTable_) {
		messages[key] = WStringToUtf8(text);
	}
	for (const auto& [stepValue, text] : overrideMessages_) {
		const char* key = StepToKey_(static_cast<TutorialStep>(stepValue));
		if (key && key[0] != '\0') {
			messages[key] = WStringToUtf8(text);
		}
	}

	json root;
	root["messages"] = messages;
	ofs << std::setw(2) << root << '\n';
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

std::wstring TutorialManager::GetEditableStepMessage(TutorialStep step) const {
	auto it = overrideMessages_.find(static_cast<int>(step));
	if (it != overrideMessages_.end()) {
		return it->second;
	}

	std::wstring tableMessage = GetMessageFromTable_(step);
	if (!tableMessage.empty()) {
		return tableMessage;
	}

	if (step_ == step) {
		return message_;
	}

	return L"";
}

void TutorialManager::Initialize() {
	LoadMessages(messagePath_);
	Reset();
}

void TutorialManager::Reset() {
	chapter_ = TutorialChapter::Full;
	step_ = TutorialStep::Intro;
	isActive_ = true;
	sawEnemyTurn_ = false;
	skippedPokerOnce_ = false;
	UpdateMessage_();
}

void TutorialManager::StartChapter(TutorialChapter chapter) {
	chapter_ = chapter;
	step_ = GetChapterStartStep_(chapter_);
	isActive_ = true;
	sawEnemyTurn_ = false;
	skippedPokerOnce_ = false;
	UpdateMessage_();
}

void TutorialManager::NextStep() {
	Advance_();
	FinishIfPastChapterEnd_();
	UpdateMessage_();
}

void TutorialManager::Update(BattleController& battle) {
	if (!isActive_) {
		return;
	}

	// 指定したステップのときのみターン終了ボタンを押せるようにする
	if (step_ == TutorialStep::EndPlayerTurn || 
		step_ == TutorialStep::SkipPokerEndTurn || 
		step_ == TutorialStep::Finished) {
		battle.SetTutorialEndTurnLocked(false);
	} else {
		battle.SetTutorialEndTurnLocked(true);
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
		if (battle.IsChoosingEnemyTarget()) {
			Advance_();
		} else if (battle.GetFieldCount() >= 1) {
			Advance_();
		}
		break;

	case TutorialStep::ChooseEnemyTarget:
		if (!battle.IsChoosingEnemyTarget() && battle.GetFieldCount() >= 1) {
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
		FinishIfPastChapterEnd_();
		UpdateMessage_();
		return;
	}

	// ExplainCardAll の次は PlayCard
	if (step_ == TutorialStep::ExplainCardAll) {
		step_ = TutorialStep::PlayCard;
		FinishIfPastChapterEnd_();
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

	FinishIfPastChapterEnd_();
	UpdateMessage_();
}

void TutorialManager::FinishIfPastChapterEnd_() {
	if (chapter_ == TutorialChapter::Full) {
		return;
	}

	if (IsPastChapterEnd_(step_)) {
		step_ = TutorialStep::Finished;
		isActive_ = true;
	}
}

bool TutorialManager::IsPastChapterEnd_(TutorialStep step) const {
	const TutorialStep end = GetChapterEndStep_(chapter_);
	return static_cast<int>(step) > static_cast<int>(end);
}

TutorialManager::TutorialStep TutorialManager::GetChapterStartStep_(TutorialChapter chapter) const {
	switch (chapter) {
	case TutorialChapter::FieldUi:
		return TutorialStep::UiPlayerHp;
	case TutorialChapter::Card:
		return TutorialStep::HoverHand;
	case TutorialChapter::SpecialEffect:
		return TutorialStep::FillField;
	case TutorialChapter::Practice:
		return TutorialStep::HoverHand;
	case TutorialChapter::Full:
	default:
		return TutorialStep::Intro;
	}
}

TutorialManager::TutorialStep TutorialManager::GetChapterEndStep_(TutorialChapter chapter) const {
	switch (chapter) {
	case TutorialChapter::FieldUi:
		return TutorialStep::UiFinished;
	case TutorialChapter::Card:
		return TutorialStep::ExplainEnergy;
	case TutorialChapter::SpecialEffect:
		return TutorialStep::EndAfterPoker;
	case TutorialChapter::Practice:
	case TutorialChapter::Full:
	default:
		return TutorialStep::Finished;
	}
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

	case TutorialStep::UiPlayerBlock:
		message_ = L"HP\u306e\u6a2a\u306e\u76fe\u306f\u30d6\u30ed\u30c3\u30af\u3067\u3059\n\u6575\u304b\u3089\u53d7\u3051\u308b\u30c0\u30e1\u30fc\u30b8\u3092\u6e1b\u3089\u3057\u307e\u3059";
		break;

	case TutorialStep::UiPlayerPowerBoost:
		message_ = L"HP\u306e\u6a2a\u306e\u5251\u306f\u653b\u6483\u529b\u30a2\u30c3\u30d7\u3067\u3059\n\u6b21\u306e\u653b\u6483\u30c0\u30e1\u30fc\u30b8\u304c\u5897\u3048\u307e\u3059";
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

	case TutorialStep::UiPokerHandHelp:
		message_ = L"左下の役確認にマウスを乗せると\nポーカー役の強さを確認できます";
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

	case TutorialStep::UiPlayerBlock:
		return FocusType::PlayerBlockArea;

	case TutorialStep::UiPlayerPowerBoost:
		return FocusType::PlayerPowerBoostArea;

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

	case TutorialStep::UiPokerHandHelp:
		return FocusType::PokerHandHelpArea;

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
	case TutorialStep::UiPlayerBlock:
	case TutorialStep::UiPlayerPowerBoost:
	case TutorialStep::UiEnemyHp:
	case TutorialStep::UiTurnText:
	case TutorialStep::UiHand:
	case TutorialStep::UiField:
	case TutorialStep::UiRoleText:
	case TutorialStep::UiEndTurn:
	case TutorialStep::UiDeckCount:
	case TutorialStep::UiPokerHandHelp:
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
	case TutorialStep::ExplainCardAll:
	case TutorialStep::ExplainEnergy:
	case TutorialStep::SkipPokerContinueTurn:
	case TutorialStep::EndAfterPoker:
		return true;
	default:
		return false;
	}
}
