#pragma once

#include "IScene.h"
#include "TextSprite.h"

#include <memory>

class Sprite;
class GameApp;

class StartupLoadingScene : public IScene {
public:
	void OnEnter(GameApp& app) override;
	void OnExit(GameApp& app) override;
	void Update(GameApp& app, float dt) override;
	void Draw2D(GameApp& app) override;

private:
	void UpdateSprites_(GameApp& app);

	std::unique_ptr<Sprite> background_;
	std::unique_ptr<Sprite> barBack_;
	std::unique_ptr<Sprite> barFill_;
	std::unique_ptr<Sprite> barGlow_;
	std::unique_ptr<TextSprite> loadingText_;
	std::unique_ptr<TextSprite> percentText_;
	float displayProgress_ = 0.0f;
	float elapsed_ = 0.0f;
	bool loadComplete_ = false;
};
