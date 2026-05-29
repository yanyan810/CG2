#pragma once
#include <memory>
#include <vector>
#include "IPauseState.h"
#include "Sprite.h"
#include "TextSprite.h"

#include "../../Logic/StatusMenu.h"

class GameApp;
class Input;

class PausingUI
{
public:
    void Initialize(GameApp& app);
    void Update(GameApp& app, Input* input);
    void Draw(GameApp& app);
    //void DrawImGui();

    // 状態遷移用メソッド
    void ChangeState(std::unique_ptr<IPauseState> newState, GameApp& app);

    // ポーズ状態の取得・設定
    bool GetIsPaused() const { return isPaused_; }
    void SetIsPaused(bool paused) { isPaused_ = paused; }

	bool GetIsSceneChangeRequested() const { return isSceneChanageReqested_; }
	void RequestSceneChange(bool paused) { isSceneChanageReqested_ = paused; }

    void SetTutorialExitMode(bool mode) { tutorialExitMode_ = mode; }
    bool IsTutorialExitMode() const { return tutorialExitMode_; }

private:
    // 状態管理
    bool isPaused_ = false;

	bool isSceneChanageReqested_ = false;
    bool tutorialExitMode_ = false;

    std::unique_ptr<IPauseState> currentState_;

    // 全状態共通の背景（暗幕など）
    std::unique_ptr<Sprite> pausingBg_;

	std::unique_ptr<Sprite> pauseButton_;
	std::unique_ptr<TextSprite> pauseButtonText_;

    // 状態異常に関する情報
    StatusMenu statusMenu_;


};