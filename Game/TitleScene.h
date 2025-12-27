#pragma once
#include "IScene.h"
#include <memory>
#include "Matrix4x4.h"

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
};
