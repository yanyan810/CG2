#pragma once
#include "IScene.h"
#include <memory>
#include <vector>
#include <string>

#include "Sprite.h"
#include "MathStruct.h"
#include "UiLayout.h"
#include "TextSprite.h"

class Camera;
class GameApp;

class StageSelectScene : public IScene {
public:
    void OnEnter(GameApp& app) override;
    void OnExit(GameApp& app) override;

    void Update(GameApp& app, float dt) override;
    void Draw3D(GameApp& app) override;
    void Draw2D(GameApp& app) override;
    void DrawImGui(GameApp& app) override;

    void DrawSkydome(GameApp& app) override;
    void DrawPostEffect3D(GameApp& app) override;
    void DrawPostEffect2D(GameApp& app) override;

private:
    struct Rect {
        float x;
        float y;
        float w;
        float h;
    };

    struct StageItem {
        std::string sceneName;
        std::wstring descText;
        Rect buttonRect{};
        Rect descRect{};
        std::unique_ptr<Sprite> buttonSprite;
    };

private:
    bool PointInRect_(float mx, float my, const Rect& rect) const;
    void DrawDescriptionText_(GameApp& app, const std::wstring& text, float x, float y);

private:
    std::unique_ptr<Camera> camera_;

    std::unique_ptr<Sprite> bg_;
    std::unique_ptr<Sprite> titleSprite_;
    std::unique_ptr<Sprite> descBgTop_;
    std::unique_ptr<Sprite> descBgBottom_;

    std::vector<StageItem> stageItems_;

    std::unique_ptr<TextSprite> descTextSprite_;

    std::vector<std::unique_ptr<Sprite>> debugHitBgs_;
    bool showDebugHitBox_ = true;


    int hoverIndex_ = -1;
    int selectIndex_ = 0;

    float hoverScale_ = 1.05f;

    float circle_ = 0.0f;
    float softness_ = 0.6f;

private:

    //UIレイアウト定義
    StageSelectLayout layout_{};
    std::string layoutPath_ = "resources/ui/stage_select_layout.json";

    std::wstring currentDescText_;
    Vector2 currentDescPos_{};

    void ApplyLayout_();
    void SaveLayout_() const;
    void LoadLayout_();

};