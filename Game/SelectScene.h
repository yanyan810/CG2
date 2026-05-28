#pragma once

#include "IScene.h"
#include "Sprite.h"
#include "TextSprite.h"
#include "UiLayout.h"

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
	Rect backButtonRect_{};
	Rect titleBoxRect_{};
	Vector3 selectTextOutlineColor_{ 0.0f, 0.0f, 0.0f };
	float selectTextOutlineWidth_ = 1.5f;
	int hoverIndex_ = -1;
	int selectIndex_ = 0;
	bool isUsingMouse_ = true;

private:
	SelectSceneLayout layout_{};
	std::string layoutPath_ = "resources/ui/select_scene_layout.json";

	void ApplyLayout_();
	void SaveLayout_() const;
	void LoadLayout_();
};
