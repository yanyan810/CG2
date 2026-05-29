#pragma once

#include "IScene.h"
#include "Sprite.h"
#include "TextSprite.h"
#include "UiLayout.h"
#include "Object3d.h"
#include "Vector3.h"
#include "Camera.h"
#include "Card3D.h"
#include "LightingParam.h"
#include <vector>

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
	void Draw3D(GameApp& app) override;
	void DrawImGui(GameApp& app) override;
	void DrawSkydome(GameApp& app) override;
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
	int lastModelIndex_ = 0; // 追加: 最後に表示したモデルのインデックス
	bool isUsingMouse_ = true;

	struct ModelTransform {
		Vector3 position{ 0.0f, 0.0f, 0.0f };
		Vector3 rotation{ 0.0f, 0.0f, 0.0f };
		Vector3 scale{ 1.0f, 1.0f, 1.0f };
	};

	std::unique_ptr<Object3d> tutorialModel_;
	std::unique_ptr<Object3d> stageSelectModel_;
	std::unique_ptr<Object3d> deckEditModel_;
	std::unique_ptr<Object3d> tutorialFieldModel_;
	std::unique_ptr<Object3d> stageSelectGroundModel_;
	std::unique_ptr<Camera> camera_;

	ModelTransform tutorialTransform_;
	ModelTransform stageSelectTransform_;
	ModelTransform deckEditTransform_;
	ModelTransform tutorialFieldTransform_;
	ModelTransform stageSelectGroundTransform_;

	std::vector<std::unique_ptr<Card3D>> backgroundCards_;
	std::vector<std::unique_ptr<Card3D>> fullHouseCards_;

	Vector3 bgCardBasePos_{ 0.0f, 5.0f, 10.0f };
	Vector3 bgCardSpacing_{ 1.5f, 2.0f, 0.0f };
	Vector3 bgCardScale_{ 0.25f, 0.25f, 0.25f };
	Vector3 bgCardRot_{ 0.0f, 0.0f, 0.0f };
	int bgCardCols_ = 6;
	Vector3 bgCardColor_{ 0.3f, 0.3f, 0.3f };
	float bgCardScrollSpeed_ = 1.0f;
	float bgCardScrollY_ = 0.0f;

	Vector3 fhCardBasePos_{ 0.0f, 4.0f, -2.0f };
	Vector3 fhCardSpacing_{ 0.5f, 0.0f, 0.01f };
	Vector3 fhCardScale_{ 0.1f, 0.1f, 0.1f };
	Vector3 fhCardRot_{ 0.0f, 0.0f, 0.0f };
	float fhCardFanAngle_ = 0.15f;
	float fhCardArchHeight_ = 0.1f;
	Vector3 fhCardColor_{ 1.0f, 0.2f, 0.2f };

	std::unique_ptr<Object3d> baseFieldModel_;
	ModelTransform baseFieldTransform_;
	bool showBaseField_ = true;

	std::unique_ptr<Sprite> buttonBgSprite_;
	Vector3 buttonBgPos_{ 640.0f, 600.0f, 0.0f };
	Vector3 buttonBgScale_{ 1280.0f, 200.0f, 1.0f };
	Vector4 buttonBgColor_{ 1.0f, 1.0f, 1.0f, 0.5f };
	bool showButtonBg_ = false;

	std::unique_ptr<Object3d> skyDome_;

	LightingParam light_;
	void ApplyLighting_(Object3d* obj);

private:
	SelectSceneLayout layout_{};
	std::string layoutPath_ = "resources/ui/select_scene_layout.json";

	void ApplyLayout_();
	void SaveLayout_() const;
	void LoadLayout_();
};
