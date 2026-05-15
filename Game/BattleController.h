#pragma once
#include <vector>
#include <cstdint>
#include "PropManager.h"
#include "CardDatabase.h"
#include "HandView3D.h"
#include "CardInstance.h"
#include "Input.h"
#include "DeckDef.h"
#include "DeckLoader.h"
#include "Card3D.h"
#include <array>
#include "Sprite.h"
#include <memory>

class GameApp;
class Camera;
class Object3dCommon;
class DirectXCommon;
class SpriteCommon;

class Player;
class Enemy;
class EnemyManager;
class FieldUi;
class ModelParticleManager;

class BattleController {
public:

	enum class CardInputState {
		Idle,
		Dragging,
		Preview,
		ChoosingFieldReplace,
		ChoosingEnemyTarget
	};

	enum class PokerChoiceState
	{
		None,
		WaitingActivateChoice, // 発動する/しない
		WaitingEffectChoice,   // 発動すると決めた後、どの効果か選ぶ
		ViewingBoardFromPokerUi // 場を見る専用
	};

	struct PokerBonus {
		int atkUp = 0;
		int drawCount = 0;
		int damage = 0;
	};

	enum class PokerMouseChoice {
		None = 0,
		ActivateYes,
		ActivateNo,
		ActivateViewBoard,
		EffectAtkUp,
		EffectDraw,
		EffectDamage,
		EffectBack,
		EffectViewBoard,
		ReturnFromBoard,
	};

	//チュートリアル用
	enum class PokerTutorialResult
	{
		None = 0,
		Activated,
		Skipped
	};

	void Initialize(GameApp& app, Camera* camera);
	void Update(GameApp& app, FieldUi& fieldUi, float dt);
	void DrawPostEffect3D(GameApp& app);
	void Draw3D(GameApp& app);
	void DrawFieldFrameBloom(GameApp& app);
	void DrawPreviewCard3D(GameApp& app);
	void Draw2D(GameApp& app);

	const CardDef* GetPreviewCardDef() const;

	bool HasPokerChoiceUi() const;
	std::wstring GetPokerChoiceUiText() const;

	std::wstring GetOperationUiText() const;
	bool ShouldShowOperationUi() const;

	//ゾーンごとのカード枚数表示用
	std::wstring GetZoneCountUiText() const;
	int GetDeckCount() const { return static_cast<int>(deck_.size()); }
	int GetHandCount() const { return static_cast<int>(hand_.size()); }
	int GetDiscardCount() const { return static_cast<int>(discard_.size()); }
	int GetFieldCount() const { return static_cast<int>(field_.size()); }
	std::wstring GetCurrentPokerHandUiText() const;
	std::wstring GetTurnUiText() const;
	std::wstring GetEnergyText() const;
	std::wstring GetPlayerHpTexts() const;
	std::vector<std::wstring> GetEnemyHpTexts() const;
	std::wstring GetPlayerPowerBoostText()const;
	std::wstring GetPlayerBlockText()const;

	//マウス選択関連
	int GetPokerMouseChoiceIndex() const;
	bool IsWaitingActivateChoice() const;
	bool IsWaitingEffectChoice() const;
	bool IsViewingBoardFromPokerUi() const;

	bool IsAllEnemiesDead() const;

	// カード効果の値を、現在のバトル状況を考慮して計算する関数
	int GetDisplayEffectValue(const CardEffectDef& effect, bool applyAttackBuff = true) const;

#ifdef USE_IMGUI
	void DrawImGui();
#endif

	//役
	enum class PokerHandRank {
		None = 0,
		OnePair,
		TwoPair,
		ThreeOfAKind,
		Straight,
		Flush,
		FullHouse,
		FourOfAKind,
		StraightFlush,
		RoyalStraightFlush,
	};

	struct PokerHandResult {
		PokerHandRank rank = PokerHandRank::None;
		int power = 0;
	};

	void SetPlayer(Player* player);
	void SetEnemyManager(EnemyManager* enemyMgr);
	void SetFieldParticleManager(ModelParticleManager* particleMgr) { fieldParticleManager_ = particleMgr; }

	void UpdateFieldCardTransform_(int index, bool hovered, float dt);
	void RefreshAllFieldCardTransforms_(float dt);
	void EmitFieldCardGlitter_(float dt);

	PokerBonus GetCurrentPokerBonusForUi() const;

	PokerHandRank GetCurrentPokerRankForUi() const { return currentPoker_.rank; }

	//先読み関数
	void Preload(GameApp& app);

	//ポーカーのサブ効果のUI
	bool IsPokerQuickPreviewVisible() const { return pokerQuickPreviewVisible_; }
	std::wstring GetPokerQuickPreviewText() const { return GetPokerEffectPreviewText(); }
	void SetPokerQuickPreviewVisible(bool visible);

	CardInputState GetNowCardInputState()const { return cardState_; }

	std::wstring GetPokerEffectPreviewText() const;

	std::wstring GetPreviewCardDetailText() const;

	std::vector<std::wstring> CollectSubEffectPreviewLines_(
		SubEffectTrigger trigger,
		PokerHandRank rank
	) const;

	//デバッグ用パワーアップ
	void SetDebugPreviewBuffEnabled(bool enabled) { useDebugPreviewBuff_ = enabled; }

	void Finalize() {
		fieldViews_.clear();
		discardView_.reset();
		costDigitModels_.clear();

		playerHpBg_.reset();
		playerHpFg_.reset();
		enemyHpBgs_.clear();
		enemyHpFgs_.clear();

		deck_.clear();
		hand_.clear();
		discard_.clear();
		field_.clear();

		player_ = nullptr;
		enemyMgr_ = nullptr;
		cam_ = nullptr;
		objCom_ = nullptr;
		dx_ = nullptr;
		spriteCom_ = nullptr;
		fieldParticleManager_ = nullptr;
	}

	bool IsPlayerTargeting() const {
		return cardState_ == CardInputState::ChoosingEnemyTarget;
		/*  cardState_ == CardInputState::Dragging ||
		  cardState_ == CardInputState::Preview;*/
	}

	bool IsPlayerTurn() const { return turn_ == TurnState::Player; }
	bool IsEndTurnButtonHovered() const { return endTurnButtonHovered_; }

	const CardDef* FindCardDef(int id) const;


public:

	//=====================
	// チュートリアル用
	//=====================
	bool IsPokerReady() const { return currentPoker_.rank != PokerHandRank::None; }
	bool IsCardPreviewing() const { return cardState_ == CardInputState::Preview; }
	bool IsChoosingEnemyTarget() const { return cardState_ == CardInputState::ChoosingEnemyTarget; }
	bool IsChoosingFieldReplace() const { return cardState_ == CardInputState::ChoosingFieldReplace; }

	void SetTutorialOpeningHand(const std::vector<CardInstance>& cards);

	PokerTutorialResult GetLastPokerTutorialResult() const { return lastPokerTutorialResult_; }
	void ClearLastPokerTutorialResult() { lastPokerTutorialResult_ = PokerTutorialResult::None; }

	void SetTutorialPokerRestriction(bool activateOnly, bool damageOnly);

	bool IsTutorialPokerTargetCancelLocked() const { return tutorialLockPokerTargetingCancel_; }

	// チュートリアルで、カードのドラッグやプレビューを一切できなくする（カードをクリックして効果を発動するだけの段階などで使う）
	void SetTutorialInputLocked(bool locked) { tutorialInputLocked_ = locked; }
	bool IsTutorialInputLocked() const { return tutorialInputLocked_; }

	// チュートリアルでターン終了ボタンを押せなくする
	void SetTutorialEndTurnLocked(bool locked) { tutorialEndTurnLocked_ = locked; }
	bool IsTutorialEndTurnLocked() const { return tutorialEndTurnLocked_; }


private:
	//=====================
	// チュートリアル用
	//=====================
	std::vector<CardInstance> tutorialOpeningHand_;
	bool useTutorialOpeningHand_ = false;
	PokerTutorialResult lastPokerTutorialResult_ = PokerTutorialResult::None;

	bool tutorialActivateOnly_ = false;
	bool tutorialDamageOnly_ = false;
	bool tutorialLockPokerTargetingCancel_ = false;

	bool tutorialInputLocked_ = false;
	bool tutorialEndTurnLocked_ = false;

private:
	enum class TurnState { Player, Enemy };
	TurnState turn_ = TurnState::Player;

	Camera* cam_ = nullptr;

	CardDatabase db_;
	HandView3D handView_;

	Object3dCommon* objCom_ = nullptr;
	DirectXCommon* dx_ = nullptr;

	std::vector<CardInstance> deck_;
	void SetupDeck(const std::vector<CardInstance>& initialDeck);

	const std::vector<CardInstance>& GetDeck() const { return deck_; }
	std::vector<CardInstance> hand_;
	std::vector<CardInstance> discard_;
	std::vector<CardInstance> field_;
	std::vector<std::unique_ptr<Card3D>> fieldViews_;
	std::unique_ptr<Card3D> discardView_;

	int energyMax_ = 10;
	int energy_ = 10;

	float enemyWait_ = 0.0f;

	CardInputState cardState_ = CardInputState::Idle;

	int selectedIndex_ = -1;
	CardInstance pendingCard_;
	bool hasPendingCard_ = false;
	std::unique_ptr<Card3D> pendingCardView_;

	POINT dragStartMouse_{};
	float dragDx_ = 0.0f;
	float dragDy_ = 0.0f;

	PokerChoiceState pokerChoiceState_ = PokerChoiceState::None;
	PokerChoiceState pokerReturnState_ = PokerChoiceState::None;
	PokerHandResult currentPoker_;
	float fieldCardGlitterEmitTimer_ = 0.0f;
	ModelParticleManager* fieldParticleManager_ = nullptr;

	//キー用
	bool prevY_ = false;
	bool prevN_ = false;
	bool prev1_ = false;
	bool prev2_ = false;
	bool prev3_ = false;
	bool prevEnter_ = false;
	bool prevL_ = false;
	bool prevR_ = false;

	//ポーカー選択UI用
	bool pokerChoiceJustOpened_ = false;

	int nextTurnAtkUp_ = 0;      // 次の自分ターン開始時に受け取る予約分
	int currentTurnAtkUp_ = 0;   // 今の自分ターン中だけ有効なATK UP
	int currentEnemyIndex_ = 0;
	int pendingDamage_ = 0;
	bool isPokerDamageTargeting_ = false;
	int pendingCardHandIndex_ = -1;
	Player* player_ = nullptr;
	EnemyManager* enemyMgr_ = nullptr;

	//タブを押しているとき様
	bool operationUiVisible_ = false;

	//場のカード入れ替え用
	int fieldReplaceHoverIndex_ = -1;
	int prevFieldReplaceHoverIndex_ = -1;
	bool fieldLayoutDirty_ = true;

	//エンドボタン用
	bool endTurnButtonHovered_ = false;

	std::unique_ptr<Object3d> costLabel_;
	std::vector<std::unique_ptr<Object3d>> costDigitModels_;
	
	std::unique_ptr<PropManager> propManager_;

	int prevEnergy_ = -1;
	int prevEnergyMax_ = -1;

	PokerMouseChoice pokerMouseChoice_ = PokerMouseChoice::None;
	bool prevMouseLeftForPoker_ = false;

	int playerTurnCount_ = 0;
	int enemyTurnCount_ = 0;

	struct DamagePopup {
		int damage = 0;
		Vector3 pos;
		float timer = 60.0f; // 60フレーム（約1秒）画面に留まる
		std::vector<std::unique_ptr<Object3d>> digitModels; // 3Dモデルの配列
	};
	std::vector<DamagePopup> damagePopups_;

	//先読み変数
	bool assetsPreloaded_ = false;
	bool cardDbLoaded_ = false;

	//先読み用のデッキ（カードDBとモデルを先に読み込むため）
	std::vector<CardInstance> prebuiltDeck_;

	//デルタタイム
	float deltaTime_ = 0.0f;

	bool pokerQuickPreviewVisible_ = false;

	//デバッグ用パワーアップ
	int debugPreviewPowerBoost_ = 0;
	int debugPreviewCurrentTurnAtkUp_ = 0;
	int debugPreviewNextTurnAtkUp_ = 0;
	bool useDebugPreviewBuff_ = true;

private:

	SpriteCommon* spriteCom_ = nullptr;

	std::unique_ptr<Sprite> playerHpBg_; // プレイヤーHP背景
	std::unique_ptr<Sprite> playerHpFg_; // プレイヤーHP中身(緑)

	std::unique_ptr<Sprite> playerHpPredict_;

	std::unique_ptr<Sprite> playerBlockPredict_;

	std::vector<std::unique_ptr<Sprite>> enemyHpBgs_;
	std::vector<std::unique_ptr<Sprite>> enemyHpFgs_;

	std::vector<std::unique_ptr<Sprite>> enemyIntentIcons_;

	std::unique_ptr<Sprite> highlightFilter_;

	void StartPlayerTurn_();
	void DrawUntilFive_();
	bool DrawOne_();
	void RebuildFieldView_();
	int PickFieldIndexByMouse_(int mouseX, int mouseY) const;
	void DrawCards_(int count);
	void ApplyCardEffects_(const CardDef& def, int targetIndex = -1);
	PokerHandResult EvaluatePokerHand_() const;
	PokerHandResult EvaluatePokerHandForCards_(const std::vector<CardInstance>& cards) const;
	const char* GetPokerHandName_(PokerHandRank rank) const;

	PokerBonus GetPokerBonus_(PokerHandRank rank) const;
	void ConsumeFieldCards_();

	void HandlePokerActivateChoice_(FieldUi& fieldUi, POINT mouse, bool lTrig, bool yTrig, bool nTrig);
	void HandlePokerEffectChoice_(FieldUi& fieldUi, POINT mouse, bool lTrig, bool nTrig);
	void HandlePokerViewBoard_(FieldUi& fieldUi, POINT mouse, bool lTrig, float dt);

	PokerHandRank ParsePokerRankString_(const std::string& s) const;
	bool IsRankAtLeast_(PokerHandRank a, PokerHandRank b) const;
	bool IsRankInFamily_(PokerHandRank rank, const std::string& family) const;
	bool DoesSubEffectConditionMatch_(const CardSubEffectDef& sub, PokerHandRank rank) const;

	void ApplyEffectsList_(const std::vector<CardEffectDef>& effects, int targetIndex = -1, bool applyAttackBuff = true);
	void TriggerSubEffectsForField_(SubEffectTrigger trigger, PokerHandRank rank);
	void TriggerSubEffectsForCard_(const CardInstance& card, SubEffectTrigger trigger, PokerHandRank rank);

	//墓地用
	void RebuildDiscardView_();

	//デッキシャッフル用
	void ShuffleDeck_();

	//コスト描画用
	void RebuildCostView_(float dt);
	void UpdateCostViewTransform_(float dt);

	//役に応じた強調表示マスクを取得
	std::array<bool, 5> GetPokerHighlightMask_() const;

	void PreloadCardAssets_();

	int CalcFinalAttackDamage_(int baseDamage) const;
	void ApplyDamageToEnemy_(Enemy& enemy, int damage);

	void SpawnDamagePopup(const Vector3& pos, int damage, bool isPlayer = false);

	std::wstring GetSubEffectTriggerText_(SubEffectTrigger trigger) const;
	std::wstring GetSubEffectConditionText_(const CardSubEffectDef& sub) const;
	std::wstring GetEffectValueText_(const CardEffectDef& effect) const;
	std::wstring GetBaseEffectSummaryText_(const CardDef& def) const;

	int CalcTotalIncomingDamage() const;

	void UpdateHpGauges();

	void UpdateLogic_(GameApp& app, FieldUi& fieldUi, float dt);
	void UpdateVisuals_(float dt);
	void UpdateHandPokerPreviewEffects_();
	void UpdateFieldReplacePreviewEffects_();
	void EmitHandCardGlitter_(float dt);
	uint64_t BuildHandPokerPreviewSignature_() const;

	std::vector<PokerHandRank> handPreviewRanks_;
	std::vector<PokerHandRank> fieldReplacePreviewRanks_;
	std::vector<bool> fieldReplacePreviewActive_;
	float handCardGlitterEmitTimer_ = 0.0f;
	uint64_t handPreviewSignature_ = 0;
};
