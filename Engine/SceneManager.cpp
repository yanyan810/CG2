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

void SceneManager::ChangeToPrepared(GameApp& app, const std::string& name, std::unique_ptr<IScene> scene) {
    assert(scene && "prepared scene must not be null");

    if (current_) {
        current_->OnExit(app);
    }

    current_ = std::move(scene);
    currentName_ = name;
}

void SceneManager::RequestPreparedChange(const std::string& name, std::unique_ptr<IScene> scene) {
    assert(scene && "prepared scene must not be null");
    pendingPreparedName_ = name;
    pendingPreparedScene_ = std::move(scene);
}

void SceneManager::Update(GameApp& app, float dt) {
    if (!current_) return;

    current_->Update(app, dt);

    if (pendingPreparedScene_) {
        ChangeToPrepared(app, pendingPreparedName_, std::move(pendingPreparedScene_));
        pendingPreparedName_.clear();
        current_->Update(app, 0.0f);
        return;
    }

    const char* next = current_->GetRequestedScene_();
    if (next && next[0] != '\0') {
        current_->ClearRequestedScene_();
        Change(app, next);
        current_->Update(app, 0.0f);
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

void SceneManager::DrawSkydome(GameApp& app)
{
    if (current_) {
        current_->DrawSkydome(app);
    }
}

void SceneManager::DrawPostEffect3D(GameApp& app)
{
    if (current_) {
        current_->DrawPostEffect3D(app);
    }
}

void SceneManager::DrawPostEffect2D(GameApp& app)
{
    if (current_) {
        current_->DrawPostEffect2D(app);
    }
}

void SceneManager::DrawPostEffect3DLate(GameApp& app)
{
    if (current_) {
        current_->DrawPostEffect3DLate(app);
    }
}

void SceneManager::DrawImGui(GameApp& app)
{
    if (current_) {
        current_->DrawImGui(app);
    }
}
