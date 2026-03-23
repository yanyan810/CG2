#pragma once

class GameApp;

class IScene {
public:
    virtual ~IScene() = default;

    virtual void OnEnter(GameApp& app) {}
    virtual void OnExit(GameApp& app) {}
    virtual void Update(GameApp& app, float dt) {}

    virtual void Draw3D(GameApp& app) {}
    virtual void Draw2D(GameApp& app) {}
    virtual void DrawImGui(GameApp& app) {}

    virtual void DrawSkydome(GameApp& app) {}

    virtual void DrawPostEffect3D(GameApp& app) {}
    virtual void DrawPostEffect2D(GameApp& app) {}

    const char* GetRequestedScene_() const { return nextScene_; }
    void ClearRequestedScene_() { nextScene_ = nullptr; }

protected:
    void RequestChangeScene_(const char* name) { nextScene_ = name; }
  

private:
    const char* nextScene_ = nullptr;
};