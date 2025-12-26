#pragma once
#pragma once
#include <string>

class GameApp;

class IScene {
public:
    virtual ~IScene() = default;

    virtual void OnEnter(GameApp& app) {}   // シーン開始時
    virtual void OnExit(GameApp& app) {}    // シーン終了時

    virtual void Update(GameApp& app, float dt) = 0;
    virtual void Draw(GameApp& app) = 0;

    // 切替要求（必要なら使う）
    const std::string& NextScene() const { return nextScene_; }
    bool IsEndRequested() const { return endRequested_; }

protected:
    void RequestChangeScene_(const std::string& name) { nextScene_ = name; }
    void RequestEnd_() { endRequested_ = true; }

private:
    std::string nextScene_;
    bool endRequested_ = false;
};
