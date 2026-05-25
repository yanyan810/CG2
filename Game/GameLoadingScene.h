#pragma once

#include "IScene.h"
#include "TextSprite.h"

#include <memory>
#include <string>
#include <vector>

class GameApp;

class GameLoadingScene : public IScene {
public:
	void OnEnter(GameApp& app) override;
	void OnExit(GameApp& app) override;
	void Update(GameApp& app, float dt) override;
	void Draw2D(GameApp& app) override;

private:
	void CollectLoadTargets_(GameApp& app);
	void UpdateText_();
	void LoadCurrentTarget_();

	std::vector<std::string> modelPaths_;
	size_t loadIndex_ = 0;
	int waitFrames_ = 0;
	std::unique_ptr<TextSprite> loadingText_;
};
