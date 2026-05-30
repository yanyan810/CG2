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
#include "TextSprite.h"
#include <memory>
#include "UI/BattleActionDirector.h"
#include "UI/DamagePopupUI.h"
#include "UI/EnemyBattleStatusUI.h"
#include "UI/PlayerBattleStatusUI.h"
#include "Poker/PokerHandEvaluator.h"
#include "Battle/EnemyActionCountSystem.h"
#include "Battle/BattleDeckZone.h"
#include "Battle/CardTargetingController.h"

class GameApp;
class Camera;
class Object3dCommon;
class DirectXCommon;
class SpriteCommon;

class Player;
class Enemy;
struct EnemyAction;
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
		ChoosingEnemyTarget,
		ExecutingSequence
	};

	enum class PokerChoiceState
	{
		None,
		WaitingActivateChoice,
		WaitingEffectChoice,
		ViewingBoardFromPokerUi
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

	enum class PokerTutorialResult
	{
		None = 0,
		Activated,
		Skipped
	};

	void Initialize(GameApp& app, Camera* camera);
	void Update(GameApp& app, FieldUi& fieldUi, float dt);
	void UpdateClearTransitionVisuals(float dt);
	void PrepareForClearTransition();
	void DrawPostEffect3D(GameApp& app);
	void Draw3D(GameApp& app);
	void DrawFieldFrameBloom(GameApp& app);
	void DrawDamagePopups3D(GameApp& app);
	void DrawField3D(GameApp& app);
	void DrawCardArea3D(GameApp& app);
	void DrawBattleOverlay3D(GameApp& app);
	void DrawPreviewCard3D(GameApp& app);
	void Draw2D(GameApp& app);
	void DrawPlayerBattleStatusUI(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj);
	void DrawEnemyBattleStatusHpTexts(const Matrix4x4& view, const Matrix4x4& proj);
	void DrawEnemyBattleStatusBcTexts(const Matrix4x4& view, const Matrix4x4& proj);
	void DrawHpGaugeBloom_(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj);
	Camera* GetActionCamera() const;
	Enemy* GetActionTarget() const;
	bool IsActionSequencePlaying() const {
		return actionDirector_.IsPlaying() || cardState_ == CardInputState::ExecutingSequence;
	}

	const CardDef* GetPreviewCardDef() const;

	bool HasPokerChoiceUi() const;
	std::wstring GetPokerChoiceUiText() const;

	std::wstring GetOperationUiText() const;
	bool ShouldShowOperationUi() const;

	std::wstring GetZoneCountUiText() const;
	int GetDeckCount() const { return static_cast<int>(deckZone_.GetDeckCount()); }
	int GetHandCount() const { return static_cast<int>(deckZone_.GetHandCount()); }
	int GetDiscardCount() const { return static_cast<int>(deckZone_.GetDiscardCount()); }
	int GetFieldCount() const { return static_cast<int>(field_.size()); }
	int GetEnergy() const { return energy_; }
	int GetEnergyMax() const { return energyMax_; }
	std::wstring GetCurrentPokerHandUiText() const;
	std::wstring GetTurnUiText() const;
	std::wstring GetEnergyText() const;
	std::wstring GetPlayerHpTexts() const;
	std::vector<std::wstring> GetEnemyHpTexts() const;
	std::vector<std::wstring> GetEnemyBCTexts() const;
	std::wstring GetPlayerPowerBoostText()const;
	std::wstring GetPlayerBlockText()const;

	int GetPokerMouseChoiceIndex() const;
	bool IsWaitingActivateChoice() const;
	bool IsWaitingEffectChoice() const;
	bool IsViewingBoardFromPokerUi() const;

	bool IsAllEnemiesDead() const;

	int GetDisplayEffectValue(const CardEffectDef& effect, bool applyAttackBuff = true) const;

#ifdef USE_IMGUI
	void DrawImGui();
	void DrawPlayerHudImGuiControls();
#endif

	using PokerHandRank = ::PokerHandRank;
	using PokerHandResult = ::PokerHandResult;

	struct FieldCardLayout {
		float y = -5.0f;
		float z = 5.0f;
		float gap = 5.0f;
		float scale = 1.15f;
		float hoverYOffset = 0.18f;
		float hoverZOffset = -0.08f;
		float hoverScale = 1.18f;
	};

	void SetPlayer(Player* player);
	void SetEnemyManager(EnemyManager* enemyMgr);
	void SetFieldParticleManager(ModelParticleManager* particleMgr) { fieldParticleManager_ = particleMgr; }
	void SetBattleParticleManager(ModelParticleManager* particleMgr) { battleParticleManager_ = particleMgr; }

	void UpdateFieldCardTransform_(int index, bool hovered, float dt);
	void RefreshAllFieldCardTransforms_(float dt);
	void EmitFieldCardGlitter_(float dt);
	void UpdateFieldEditorPreview(float dt);
	void BuildFieldEditorPreview(int cardCount, int firstCardId);
	void DrawFieldEditorPreview3D(GameApp& app);
	void DrawFieldEditorPostEffect3D(GameApp& app);
	bool LoadFieldCardLayout(const std::string& path);
	bool SaveFieldCardLayout(const std::string& path) const;
	const FieldCardLayout& GetFieldCardLayout() const { return fieldCardLayout_; }
#ifdef USE_IMGUI
	void DrawFieldSceneEditerImGui();
#endif

	PokerBonus GetCurrentPokerBonusForUi() const;

	PokerHandRank GetCurrentPokerRankForUi() const { return currentPoker_.rank; }

	void Preload(GameApp& app);

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

	void SetDebugPreviewBuffEnabled(bool enabled) { useDebugPreviewBuff_ = enabled; }

	void Finalize() {
		fieldViews_.clear();
		discardView_.reset();
		costDigitModels_.clear();

		enemyStatusUi_.Clear();
		enemyActionCountSystem_.Clear();

		deckZone_.Clear();
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
	//=====================
	bool IsPokerReady() const { return currentPoker_.rank != PokerHandRank::None; }
	bool IsCardPreviewing() const { return cardState_ == CardInputState::Preview; }
	bool IsChoosingEnemyTarget() const { return cardState_ == CardInputState::ChoosingEnemyTarget; }
	bool IsChoosingFieldReplace() const { return cardState_ == CardInputState::ChoosingFieldReplace; }

	void SetTutorialOpeningHand(const std::vector<CardInstance>& cards);

	PokerTutorialResult GetLastPokerTutorialResult() const { return lastPokerTutorialResult_; }
	void ClearLastPokerTutorialResult() { lastPokerTutorialResult_ = PokerTutorialResult::None; }

	void SetTutorialPokerRestriction(bool activateOnly, bool damageOnly);
	void SetTutorialForcedEnemyTargetCardId(int cardDefId);

	bool IsTutorialPokerTargetCancelLocked() const { return tutorialLockPokerTargetingCancel_; }

	void SetTutorialInputLocked(bool locked) { tutorialInputLocked_ = locked; }
	bool IsTutorialInputLocked() const { return tutorialInputLocked_; }

	void SetTutorialEndTurnLocked(bool locked) { tutorialEndTurnLocked_ = locked; }
	bool IsTutorialEndTurnLocked() const { return tutorialEndTurnLocked_; }


private:
	//=====================
	//=====================
	std::vector<CardInstance> tutorialOpeningHand_;
	bool useTutorialOpeningHand_ = false;
	PokerTutorialResult lastPokerTutorialResult_ = PokerTutorialResult::None;

	bool tutorialActivateOnly_ = false;
	bool tutorialDamageOnly_ = false;
	bool tutorialLockPokerTargetingCancel_ = false;
	int tutorialForcedEnemyTargetCardId_ = -1;

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

	BattleDeckZone deckZone_;
	void SetupDeck(const std::vector<CardInstance>& initialDeck);

	const std::vector<CardInstance>& GetDeck() const { return deckZone_.GetDeck(); }
	std::vector<CardInstance> field_;
	std::vector<std::unique_ptr<Card3D>> fieldViews_;
	FieldCardLayout fieldCardLayout_{};
	std::string fieldCardLayoutPath_ = "resources/configs/fieldCardLayout.json";
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
	ModelParticleManager* battleParticleManager_ = nullptr;
	std::vector<float> poisonIdleEffectTimers_;
	std::vector<float> frostIdleEffectTimers_;

	bool prevY_ = false;
	bool prevN_ = false;
	bool prev1_ = false;
	bool prev2_ = false;
	bool prev3_ = false;
	bool prevEnter_ = false;
	bool prevL_ = false;
	bool prevR_ = false;

	bool pokerChoiceJustOpened_ = false;

	int nextTurnAtkUp_ = 0;
	int currentTurnAtkUp_ = 0;
	int currentEnemyIndex_ = 0;
	int pendingDamage_ = 0;
	bool isPokerDamageTargeting_ = false;
	int pendingCardHandIndex_ = -1;
	Player* player_ = nullptr;
	EnemyManager* enemyMgr_ = nullptr;

	bool operationUiVisible_ = false;

	int fieldReplaceHoverIndex_ = -1;
	int prevFieldReplaceHoverIndex_ = -1;
	bool fieldLayoutDirty_ = true;

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

	DamagePopupUI damagePopupUi_;

	bool assetsPreloaded_ = false;
	bool cardDbLoaded_ = false;

	std::vector<CardInstance> prebuiltDeck_;

	float deltaTime_ = 0.0f;

	bool pokerQuickPreviewVisible_ = false;

	int debugPreviewPowerBoost_ = 0;
	int debugPreviewCurrentTurnAtkUp_ = 0;
	int debugPreviewNextTurnAtkUp_ = 0;
	bool useDebugPreviewBuff_ = true;

private:

	SpriteCommon* spriteCom_ = nullptr;
	BattleActionDirector actionDirector_;
	std::vector<const ActionSequenceProfile*> actionSequenceQueue_;
	size_t actionSequenceIndex_ = 0;
	Enemy* actionSequenceTarget_ = nullptr;
	const CardDef* actionSequenceCardDef_ = nullptr;
	CardInstance actionSequenceCard_{};
	bool actionSequenceDamageApplied_ = false;

	void ExecutePendingAttack_(Enemy& targetEnemy);
	void ExecuteEnemyAction_(Enemy& enemy, const EnemyAction& action);
	void OnPlayerCardUsed_();
	std::vector<std::string> CollectEffectTypes_(const CardDef& def) const;
	bool BeginCardActionSequence_(GameApp& app, const CardDef& def, const CardInstance& card, Enemy& targetEnemy);
	bool StartNextActionSequence_();
	bool ApplyFrostBeforeEnemyAction_(Enemy& enemy);

	PlayerBattleStatusUI playerStatusUi_;
	EnemyBattleStatusUI enemyStatusUi_;

	int playerLastHp_ = -1;
	EnemyActionCountSystem enemyActionCountSystem_;
	CardTargetingController cardTargetingController_;

	std::unique_ptr<Sprite> highlightFilter_;

	void StartPlayerTurn_();
	void DrawTurnStartCards_();
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
	void ApplyDamageEffect_(const CardEffectDef& effect, int targetIndex, bool applyAttackBuff);
	void ApplyDamageCrescentEffect_(const CardEffectDef& effect, int targetIndex, bool applyAttackBuff);
	void ApplyDamageByBlockEffect_(const CardEffectDef& effect, int targetIndex, bool applyAttackBuff);
	void ApplyDamageAllEffect_(const CardEffectDef& effect, bool applyAttackBuff);
	void ApplyDrawEffect_(const CardEffectDef& effect);
	void ApplyBlockEffect_(const CardEffectDef& effect);
	void ApplyPowerBoostEffect_(const CardEffectDef& effect);
	void ApplyNextTurnAtkUpEffect_(const CardEffectDef& effect);
	void ApplyHealEffect_(const CardEffectDef& effect);
	void ApplyHealByBlockEffect_(const CardEffectDef& effect);
	void ApplyHealByLowCostInHandEffect_(const CardEffectDef& effect);
	void ApplyVampireBuffEffect_(const CardEffectDef& effect);
	void ApplySelfDamageEffect_(const CardEffectDef& effect);
	void ApplyPoisonEffect_(const CardEffectDef& effect, int targetIndex);
	void ApplyPoisonAllEffect_(const CardEffectDef& effect);
	void ApplyPoisonAmplifyEffect_(const CardEffectDef& effect);
	void ApplyPoisonDamageEffect_(const CardEffectDef& effect);
	void ApplyPoisonDrawEffect_(const CardEffectDef& effect);
	void ApplyPoisonRemoveEffect_();
	void ApplyPoisonHealEffect_(const CardEffectDef& effect);
	void ApplyFrostEffect_(const CardEffectDef& effect, int targetIndex);
	void ApplyFrostAllEffect_(const CardEffectDef& effect);
	void ApplyFrostBlockEffect_(const CardEffectDef& effect);
	void ApplyFrostDamageEffect_(const CardEffectDef& effect);
	void ApplyFrostSubtractEffect_(const CardEffectDef& effect);
	void ApplyFrostBiteEffect_(const CardEffectDef& effect);
	void ApplyFrostAmplifyEffect_(const CardEffectDef& effect);
	void ApplyFrostDrawEffect_(const CardEffectDef& effect, int targetIndex);
	void ApplyChangeNumberEffect_(const CardEffectDef& effect);
	void ApplyChangeSuitEffect_(const CardEffectDef& effect);
	void TriggerSubEffectsForField_(SubEffectTrigger trigger, PokerHandRank rank);
	void TriggerSubEffectsForCard_(const CardInstance& card, SubEffectTrigger trigger, PokerHandRank rank);

	void RebuildDiscardView_();

	void ShuffleDeck_();

	void RebuildCostView_(float dt);
	void UpdateCostViewTransform_(float dt);

	std::array<bool, 5> GetPokerHighlightMask_() const;

	void PreloadCardAssets_();

	int CalcFinalAttackDamage_(int baseDamage) const;
	int ApplyDamageToEnemy_(Enemy& enemy, int damage);
	Enemy* ResolveTargetEnemy_(int targetIndex) const;
	void ApplyFrostBiteToEnemyIfActive_(Enemy& enemy);

	void SpawnDamagePopup(const Vector3& pos, int damage, bool isPlayer = false);

	std::wstring GetSubEffectTriggerText_(SubEffectTrigger trigger) const;
	std::wstring GetSubEffectConditionText_(const CardSubEffectDef& sub) const;
	std::wstring GetEffectValueText_(const CardEffectDef& effect) const;
	std::wstring GetBaseEffectSummaryText_(const CardDef& def) const;

	int CalcTotalIncomingDamage() const;

	void UpdateHpGauges();

	void UpdateLogic_(GameApp& app, FieldUi& fieldUi, float dt);
	void UpdateVisuals_(float dt);
	void UpdateEnemyStatusLayout_();
	void UpdateFieldFrameEffects_();
	void UpdateHandPokerPreviewEffects_();
	void UpdateFieldReplacePreviewEffects_();
	void EmitHandCardGlitter_(float dt);
	void EmitPoisonAppliedEffect_(Enemy& enemy, int poisonPoint);
	void UpdatePoisonIdleEffects_(float dt);
	void EmitFrostAppliedEffect_(Enemy& enemy, int frostPoint, bool playCastAnim = true);
	void UpdateFrostIdleEffects_(float dt);
	Vector3 CalcStatusEffectEmitPos_(const Enemy& enemy, float heightOffset) const;
	uint64_t BuildHandPokerPreviewSignature_() const;
	bool IsTutorialForcedCardActive_() const;
	bool IsTutorialForcedCardAllowed_(int handIndex) const;

	std::vector<PokerHandRank> handPreviewRanks_;
	std::vector<PokerHandRank> fieldReplacePreviewRanks_;
	std::vector<bool> fieldReplacePreviewActive_;
	float handCardGlitterEmitTimer_ = 0.0f;
	uint64_t handPreviewSignature_ = 0;
};
