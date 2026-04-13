#include "TutorialManager.h"
#include "BattleController.h"

void TutorialManager::Initialize() {
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
		// 特殊効果UIが開いたら、次の説明へ自動で進む
		if (battle.HasPokerChoiceUi()) {
			Advance_();
		}

		break;

	case TutorialStep::ChoosePokerEffect:
		// 「場を見る」に入ったら、専用説明へ切り替える
		if (battle.IsViewingBoardFromPokerUi()) {
			step_ = TutorialStep::ViewingBoardFromPoker;
			UpdateMessage_();
			return;
		}

		// 特殊効果UIが本当に閉じたら、発動したかスキップしたかを見る
		if (!battle.HasPokerChoiceUi()) {

			// 発動した場合：場が消費されている
			if (battle.GetFieldCount() == 0) {
				skippedPokerOnce_ = false;
				sawEnemyTurn_ = !battle.IsPlayerTurn(); // もう敵ターンなら記録
				step_ = TutorialStep::EndAfterPoker;
			}
			// スキップした場合：まだ自分ターンで、場は残っている
			else if (battle.IsPlayerTurn() && battle.GetFieldCount() == 5) {
				skippedPokerOnce_ = true;
				step_ = TutorialStep::SkipPokerContinueTurn;
			}

			UpdateMessage_();
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
		// 発動後、敵ターンを確認して、次の自分ターンに戻ったら完了
		if (!battle.IsPlayerTurn()) {
			sawEnemyTurn_ = true;
		}

		if (sawEnemyTurn_ && battle.IsPlayerTurn()) {
			step_ = TutorialStep::Finished;
			UpdateMessage_();
			return;
		}
		break;

	case TutorialStep::Finished:

		break;
	}

	UpdateMessage_();
}

void TutorialManager::Advance_() {
	if (step_ == TutorialStep::Finished) {
		return;
	}

	step_ = static_cast<TutorialStep>(static_cast<int>(step_) + 1);
}

void TutorialManager::UpdateMessage_() {
	switch (step_) {
	case TutorialStep::Intro:
		message_ = L"チュートリアルです\n基本の流れを一通り確認しましょう";
		break;

	case TutorialStep::HoverHand:
		message_ = L"下にあるのが手札です\nカードにマウスを乗せてみましょう";
		break;

	case TutorialStep::PlayCard:
		message_ = L"カードを1枚使って\n場に出してみましょう";
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
		message_ = L"次の自分のターン開始時に\n役が成立していると\n特殊効果を発動できます";
		break;

	case TutorialStep::ChoosePokerEffect:
		message_ = L"特殊効果を1つ選んで\n発動してみましょう";
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

	case TutorialStep::EndAfterPoker:
		message_ = L"発動したらターンが進みます\n敵のターンに入るのを確認しましょう";
		break;

	case TutorialStep::Finished:
		message_ = L"チュートリアル完了です\n左クリックでタイトルに戻ります";
		break;

	}
}

TutorialManager::FocusType TutorialManager::GetFocusType() const {
	switch (step_) {
	case TutorialStep::HoverHand:
		return FocusType::HandArea;

	case TutorialStep::PlayCard:
	case TutorialStep::FillField:
	case TutorialStep::ExplainPokerReady:
		return FocusType::FieldArea;

	case TutorialStep::ExplainEnergy:
		return FocusType::EnergyArea;

	case TutorialStep::EndPlayerTurn:
	case TutorialStep::SkipPokerEndTurn:
		return FocusType::EndTurnButtonArea;

	case TutorialStep::WaitEnemyTurn:
	case TutorialStep::SkipPokerWaitEnemyTurn:
	case TutorialStep::EndAfterPoker:
		return FocusType::EnemyTurnArea;

	case TutorialStep::ChoosePokerEffect:
		return FocusType::None;

	case TutorialStep::ViewingBoardFromPoker:
		return FocusType::PokerBackButtonArea;

	case TutorialStep::SkipPokerContinueTurn:
		return FocusType::PokerViewBoardButtonArea;

	default:
		return FocusType::None;
	}
}