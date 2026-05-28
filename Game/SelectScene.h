#pragma once

#include "IScene.h"
#include "Sprite.h"
#include "TextSprite.h"

#include <array>
#include <memory>
#include <string>

class GameApp;

class SelectScene : public IScene {
public:
	void OnEnter(GameApp& app) override;
	void OnExit(GameApp& app) override;
	void Update(GameApp& app, float dt) override;
	void Draw2D(GameApp& app) override;
	void DrawImGui(GameApp& app) override;

private:
	struct Rect {
		float x;
		float y;
		float w;
		float h;
	};

	struct MenuItem {
		std::string sceneName;
		std::wstring label;
		std::wstring description;
		Rect rect{};
		std::unique_ptr<Sprite> border;
		std::unique_ptr<Sprite> bg;
		std::unique_ptr<TextSprite> text;
	};

	bool PointInRect_(float mx, float my, const Rect& rect) const;
	void SelectCurrent_(GameApp& app);

private:
	std::unique_ptr<Sprite> bg_;
	std::unique_ptr<Sprite> titleBoxBorder_;
	std::unique_ptr<Sprite> titleBoxBg_;
	std::unique_ptr<Sprite> backButtonBorder_;
	std::unique_ptr<Sprite> backButtonBg_;
	std::unique_ptr<TextSprite> backButtonText_;
	std::unique_ptr<TextSprite> titleText_;
	std::unique_ptr<TextSprite> descText_;
	std::array<MenuItem, 3> menuItems_{};
	Rect backButtonRect_{ 150.0f, 110.0f, 180.0f, 70.0f };
	Rect titleBoxRect_{ 480.0f, 145.0f, 320.0f, 88.0f };
	Vector3 selectTextOutlineColor_{ 0.0f, 0.0f, 0.0f };
	float selectTextOutlineWidth_ = 1.5f;
	int hoverIndex_ = -1;
	int selectIndex_ = 0;
};
