#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

class BattleActionDirector;
class Card3D;
class DamagePopupUI;
class EnemyBattleStatusUI;
class EnemyManager;
class GameApp;
class HandView3D;
class Player;
class PlayerBattleStatusUI;
class PropManager;
class Sprite;
class Matrix4x4;
struct BloomParam;
enum class PokerHandRank;

class BattleRenderView {
public:
	struct CardAreaContext {
		Card3D* discardView = nullptr;
		HandView3D* handView = nullptr;
		DamagePopupUI* damagePopupUi = nullptr;
		bool choosingFieldReplace = false;
		Sprite* highlightFilter = nullptr;
		Card3D* pendingCardView = nullptr;
		const std::vector<std::unique_ptr<Card3D>>* fieldViews = nullptr;
	};

	struct BattleOverlayContext {
		GameApp* app = nullptr;
		Sprite* highlightFilter = nullptr;
		EnemyManager* enemyMgr = nullptr;
		bool choosingEnemyTarget = false;
		bool enemyTargetBloomEnabled = false;
		const BloomParam* enemyTargetBloomParam = nullptr;
	};

	struct FieldFrameBloomContext {
		GameApp* app = nullptr;
		const std::vector<std::unique_ptr<Card3D>>* fieldViews = nullptr;
		HandView3D* handView = nullptr;
		const std::vector<bool>* fieldReplacePreviewActive = nullptr;
		const std::vector<PokerHandRank>* handPreviewRanks = nullptr;
		std::array<bool, 5> pokerHighlightMask{};
		PokerHandRank currentPokerRank{};
		bool inReplacePreview = false;
		bool enabled = false;
		float threshold = 0.0f;
		float intensity = 0.0f;
		float handIntensity = 0.0f;
		float minPulse = 0.0f;
		float chromAb = 0.0f;
		float time = 0.0f;
	};

	struct Draw2DContext {
		GameApp* app = nullptr;
		BattleActionDirector* actionDirector = nullptr;
		PlayerBattleStatusUI* playerStatusUi = nullptr;
		EnemyBattleStatusUI* enemyStatusUi = nullptr;
		Player* player = nullptr;
		EnemyManager* enemyMgr = nullptr;
		const std::vector<bool>* enemyActedFlags = nullptr;
		int incomingDamage = 0;
		float time = 0.0f;
		float hpDamageBlinkSpeed = 0.0f;
		bool hpGaugeBloomEnabled = false;
		float hpGaugeBloomIntensity = 0.0f;
		float hpDamageBloomIntensity = 0.0f;
		bool enemyIntentBloomEnabled = false;
		float enemyIntentBloomIntensity = 0.0f;
		float enemyIntentBloomMinPulse = 0.0f;
	};

	static void DrawDamagePopups3D(BattleActionDirector& actionDirector, DamagePopupUI& damagePopupUi);
	static void DrawCardArea3D(const CardAreaContext& context);
	static void DrawField3D(PropManager* propManager);
	static void DrawBattleOverlay3D(const BattleOverlayContext& context);
	static void DrawPostEffect3D(HandView3D& handView, GameApp& app);
	static void DrawFieldFrameBloom(const FieldFrameBloomContext& context);
	static void Draw2D(const Draw2DContext& context);
	static void DrawHpGaugeBloom(const Draw2DContext& context, const Matrix4x4& view, const Matrix4x4& proj);
	static void DrawPlayerBattleStatusUI(
		PlayerBattleStatusUI& playerStatusUi,
		const std::wstring& hpText,
		const std::wstring& blockText,
		const std::wstring& powerBoostText,
		const Matrix4x4& view,
		const Matrix4x4& proj);
	static void DrawEnemyBattleStatusHpTexts(EnemyBattleStatusUI& enemyStatusUi, const Matrix4x4& view, const Matrix4x4& proj);
	static void DrawEnemyBattleStatusBcTexts(EnemyBattleStatusUI& enemyStatusUi, const Matrix4x4& view, const Matrix4x4& proj);
};