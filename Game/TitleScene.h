#pragma once
#include "IScene.h"
#include <memory>
#include "Matrix4x4.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "Sprite.h"
#include "SpriteCommon.h"

class Particle;
class Camera;

class TitleScene : public IScene {
public:
    void OnEnter(GameApp& app) override;
    void OnExit(GameApp& app) override;

    void Update(GameApp& app, float dt) override;
    void Draw(GameApp& app) override;

private:
    bool prevSpace_ = false;

    std::unique_ptr<Camera> camera_;

    Vector3 imguiCamPos_={ 0.0f, 3.0f, -20.0f };
    Vector3 imguiCamRot_={ 0.0f, 0.0f, 0.0f };

    std::unique_ptr<Particle> particle_;

    std::unique_ptr<Object3d> skyDome_;

    std::unique_ptr < Sprite> bg_;

	std::unique_ptr < Sprite> pressStart_;

    enum class State {
        Idle,
        ExitClose
    };

    State state_ = State::Idle;

    float circle_ = 1.0f;   // ★Titleは最初から開いている
    float softness_ = 0.6f;

    const char* kNextScene_ = "Test"; // SPACE後に行く先

    //確認
    std::unique_ptr<Object3d> testObj_;

    // 確認用パラメータ
    float shininess_ = 64.0f;
    int lightingMode_ = 1;     // 1:Lambert 2:HalfLambert 3:SpecOnly
    bool orbitCam_ = true;
    float orbitSpeed_ = 0.6f;
    float orbitRadius_ = 10.0f;
    float orbitT_ = 0.0f;

};
