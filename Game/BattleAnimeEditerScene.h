#pragma once
#include "IScene.h"
#include <memory>
#include "WinApp.h"
#include "Matrix4x4.h"
#include "Input.h"
#include "Camera/Camera.h"
#include "player/Player.h"
#include "enemy/Enemy.h"
#include "UI/BattleActionDirector.h"
#include "LightingParam.h"
#include "AnimationEditorSession.h"
#include "Camera/CameraAnimator.h"
#include <vector>
#include <string>
#include <filesystem>

class BattleAnimeEditerScene : public IScene {
public:
    void OnEnter(GameApp& app) override;
    void OnExit(GameApp& app) override;
    void Update(GameApp& app, float dt) override;

    void Draw3D(GameApp& app) override;
    void Draw2D(GameApp& app) override;
    void DrawImGui(GameApp& app) override;
    void DrawSkydome(GameApp& app) override;
    void DrawPostEffect3D(GameApp& app) override;

private:
    Input* input_ = nullptr;
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Player> player_;
    std::unique_ptr<Object3d> ground_;
    std::unique_ptr<Object3d> skyDome_;
    EnemyManager enemyMgr_;
    BattleActionDirector actionDirector_;
    LightingParam light_;

    // Debug camera control
    float cameraDist_ = 20.0f;
    float cameraTheta_ = 0.0f;
    float cameraPhi_ = 0.5f;
    Vector3 cameraTarget_ = {0.0f, 0.0f, 0.0f};

    bool isRightClickDragging_ = false;
    POINT lastMousePos_ = {};

    // 編集関連
    enum class EditorTargetKind {
        Animation,
        Camera,
    };
    EditorTargetKind editorTargetKind_ = EditorTargetKind::Animation;
    AnimationEditorSession animationEditor_;
    Object3d* animationEditTarget_ = nullptr;
    Camera* cameraEditTarget_ = nullptr;
    
    std::unique_ptr<CameraAnimator> cameraAnim_;

    std::vector<std::string> cameraFiles_;
    int currentCameraIndex_ = -1;
    bool randomCameraEnabled_ = false;
    bool sameCameraLoopEnabled_ = false;

    std::vector<std::string> animationFiles_;
    std::vector<std::string> sequenceFiles_;
    int currentSequenceIndex_ = -1;
    bool liveCameraSyncEnabled_ = false;
    std::string liveCameraPath_;
    std::filesystem::file_time_type liveCameraLastWriteTime_{};
    float liveCameraPollTimer_ = 0.0f;
    std::string liveCameraStatus_ = "Live camera sync is waiting for a sequence.";

    void ReloadCameraFileList_();
    void ReloadAnimationFileList_();
    void ReloadSequenceFileList_();
    bool LoadCameraByIndex_(int index);
    bool LoadCameraByPath_(const std::string& path);
    bool LoadSequenceByIndex_(int index);
    bool LoadSequenceByPath_(const std::string& path);
    void RefreshLiveCameraPath_();
    bool ReloadLiveCameraIfNeeded_(bool force);
    void ApplyProfilePositions_(Enemy* targetEnemy, bool applyCamera);
    void DrawSequenceDebugWindow_(GameApp& app, Enemy* targetEnemy);
    AnimationEditorSession::EditorContext BuildEditorContext_();
};
