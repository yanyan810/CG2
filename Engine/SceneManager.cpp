#include "SceneManager.h"
#include "IScene.h"
#include <cassert>

void SceneManager::Register(const std::string& name, Factory factory) {
    factories_[name] = std::move(factory);
}

void SceneManager::Change(GameApp& app, const std::string& name) {
    auto it = factories_.find(name);
    assert(it != factories_.end());

    if (current_) {
        current_->OnExit(app);
    }

    current_ = it->second();
    currentName_ = name;
    current_->OnEnter(app);
}

void SceneManager::Update(GameApp& app, float dt) {
    if (!current_) return;

    current_->Update(app, dt);

    const char* next = current_->GetRequestedScene_();
    if (next && next[0] != '\0') {
        current_->ClearRequestedScene_();
        Change(app, next);
    }
}

void SceneManager::Draw3D(GameApp& app)
{
    if (current_) {
        current_->Draw3D(app);
    }
}

void SceneManager::Draw2D(GameApp& app)
{
    if (current_) {
        current_->Draw2D(app);
    }
}

void SceneManager::DrawImGui(GameApp& app)
{
    if (current_) {
        current_->DrawImGui(app);
    }
}