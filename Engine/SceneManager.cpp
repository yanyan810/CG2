#include "SceneManager.h"
#include "IScene.h"
#include <cassert>

void SceneManager::Register(const std::string& name, Factory factory) {
    factories_[name] = std::move(factory);
}

void SceneManager::Change(GameApp& app, const std::string& name) {
    auto it = factories_.find(name);
    assert(it != factories_.end());

    if (current_) current_->OnExit(app);
    current_ = it->second();
    currentName_ = name;
    current_->OnEnter(app);
}

void SceneManager::Update(GameApp& app, float dt) {
    if (!current_) return;
    current_->Update(app, dt);

    if (current_->IsEndRequested()) {
        // GameApp 側で終了させる想定（app.RequestQuit() とか）
        return;
    }

    if (!current_->NextScene().empty()) {
        Change(app, current_->NextScene());
    }
}

void SceneManager::Draw(GameApp& app) {
    if (!current_) return;
    current_->Draw(app);
}
