#pragma once

#include "GameApp.h"
#include "IScene.h"
#include "TextSprite.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class Sprite;
class IScene;

class GameLoadingScene : public IScene {
public:
	void OnEnter(GameApp& app) override;
	void OnExit(GameApp& app) override;
	void Update(GameApp& app, float dt) override;
	void Draw2D(GameApp& app) override;

private:
	void BuildStageLoadSteps_(GameApp& app);
	void BuildStageSelectLoadSteps_(GameApp& app);
	void BuildTutorialLoadSteps_(GameApp& app);
	void BuildStageInfo_(GameApp& app);
	void UpdateText_();
	bool LoadCurrentStep_(GameApp& app);
	float GetProgress_(GameApp& app) const;
	void PrepareGameScene_(GameApp& app);
	void UpdateSprites_(GameApp& app);
	void LoadTips_();
	void SelectRandomTips_(int maxCount);
	void ApplyCurrentTip_();
	void ChangeTip_(int direction);
	bool HandleTipInput_(GameApp& app);
	bool IsPointInRect_(const POINT& point, float x, float y, float w, float h) const;

	std::vector<std::string> modelPaths_;
	std::vector<std::function<void()>> stageLoadSteps_;
	size_t loadIndex_ = 0;
	int waitFrames_ = 0;
	float elapsed_ = 0.0f;
	float displayProgress_ = 0.0f;
	bool loadComplete_ = false;
	bool stageScenePrepared_ = false;
	bool stagePrepareFrameShown_ = false;
	GameApp::LoadingMode mode_ = GameApp::LoadingMode::BootToTitle;
	std::unique_ptr<IScene> preparedGameScene_;
	std::unique_ptr<Sprite> background_;
	std::unique_ptr<Sprite> barBack_;
	std::unique_ptr<Sprite> barFill_;
	std::unique_ptr<Sprite> barGlow_;
	std::vector<std::unique_ptr<Sprite>> flowOrbs_;
	std::unique_ptr<Sprite> barHeadGlow_;
	std::unique_ptr<Sprite> barHeadCore_;
	std::unique_ptr<Sprite> stageInfoPanel_;
	std::unique_ptr<TextSprite> loadingText_;
	std::unique_ptr<TextSprite> percentText_;
	std::unique_ptr<TextSprite> stageTitleText_;
	std::unique_ptr<TextSprite> enemyInfoText_;
	std::unique_ptr<TextSprite> tipTitleText_;
	std::unique_ptr<TextSprite> tipText_;
	std::unique_ptr<TextSprite> tipLeftArrowText_;
	std::unique_ptr<TextSprite> tipRightArrowText_;
	std::unique_ptr<TextSprite> tipCounterText_;
	std::unique_ptr<TextSprite> readyText_;
	std::wstring stageTitle_;
	std::wstring enemyInfo_;
	std::wstring tipTextValue_;
	std::vector<std::wstring> allTips_;
	std::vector<std::wstring> displayedTips_;
	size_t currentTipIndex_ = 0;
};
