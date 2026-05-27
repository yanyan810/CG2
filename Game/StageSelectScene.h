#pragma once
#include "IScene.h"
#include <memory>
#include <vector>
#include <string>

#include "Sprite.h"
#include "MathStruct.h"
#include "UiLayout.h"
#include "TextSprite.h"
#include "Object3d.h"
#include "PropManager.h"

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
        std::wstring displayText;
        std::wstring descText;
        int stageId = 0;
        std::string stageConfigPath;
        Rect buttonRect{};
        Rect descRect{};
        std::unique_ptr<Sprite> buttonSprite;
    };

private:
    bool PointInRect_(float mx, float my, const Rect& rect) const;
    void SelectStageItem_(GameApp& app, const StageItem& item);
    void ChangeStage_(int delta);
    void ApplyCurrentStageToBattleItem_();
    void LoadStageFieldBackgrounds_(GameApp& app);
    void UpdateStageFieldBackground_();
    void DrawDescriptionText_(GameApp& app, const std::wstring& text, float x, float y);

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Object3d> skyDome_;
    PropManager* stageFieldProps_ = nullptr;
    std::string stageFieldConfigPath_;
    std::unique_ptr<Sprite> bossStageWarningOverlay_;
    std::unique_ptr<Sprite> titleSprite_;
    std::unique_ptr<Sprite> descBgTop_;
    std::unique_ptr<Sprite> descBgBottom_;

    std::vector<StageItem> stageItems_;
    std::vector<std::unique_ptr<TextSprite>> itemTextSprites_;
    std::unique_ptr<TextSprite> leftArrowText_;
    std::unique_ptr<TextSprite> rightArrowText_;

    std::unique_ptr<TextSprite> descTextSprite_;

    std::vector<std::unique_ptr<Sprite>> debugHitBgs_;
    bool showDebugHitBox_ = false;


    int hoverIndex_ = -1;
    int selectIndex_ = 0;

    float hoverScale_ = 1.05f;

    float circle_ = 0.0f;
    float softness_ = 0.6f;
    int currentStageId_ = 1;
    float bossWarningBurstTimer_ = 0.0f;
    float bossWarningBurstDuration_ = 0.8f;
    float bossShakeTimer_ = 0.0f;
    float bossShakeDuration_ = 0.4f;
    float bossShakeMagnitude_ = 6.0f;
    Rect leftArrowRect_{ 120.0f, 410.0f, 80.0f, 100.0f };
    Rect rightArrowRect_{ 1080.0f, 410.0f, 80.0f, 100.0f };

private:

    //UIレイアウト定義
    StageSelectLayout layout_{};
    std::string layoutPath_ = "resources/ui/stage_select_layout.json";

    std::wstring currentDescText_;
    Vector2 currentDescPos_{};

    void ApplyLayout_();
    void SaveLayout_() const;
    void LoadLayout_();

private:
    static std::vector<std::unique_ptr<PropManager>> stageFieldPropsCache_;
    static bool stageFieldPropsCacheReady_;

};
