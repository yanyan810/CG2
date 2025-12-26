#pragma once
#include "IScene.h"
#include <memory>

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
    std::unique_ptr<Particle> particle_;
};
