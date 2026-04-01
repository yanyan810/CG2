#pragma once
#include <memory>
#include "SceneManager.h"
#include "Input.h"
#include "SpriteCommon.h"
#include "Bloom.h"

#include"CardInstance.h"

class WinApp;
class DirectXCommon;
class SrvManager;
class Object3dCommon;
class ParticleCommon;
class ImGuiManagaer;
class SkinningCommon;

class SceneManager;

class GameApp {
public:
    GameApp();
    ~GameApp();

    int Run();
    void RequestQuit() { quit_ = true; }

    // 共有システムにアクセス（GameScene から使う）
    WinApp* Win() const { return win_.get(); }
    DirectXCommon* Dx() const { return dx_.get(); }
    SrvManager* Srv() const { return srv_.get(); }
    SpriteCommon* SpriteCom() const { return spriteCommon_.get(); }
    Object3dCommon* ObjCom() const { return objCommon_.get(); }
    ParticleCommon* ParticleCom() const { return particleCommon_.get(); }
    ImGuiManagaer* ImGui() const { return imgui_.get(); }

    SkinningCommon* SkinCom() { return skinCom_.get(); }

    SceneManager& Scenes() { return *sceneMgr_; }

    void Update(float dt);

    void Draw3D();
    void Draw2D();
    void DrawImGui();
    void Draw();

    Input* GetInput() { return input_.get(); }
    const Input* GetInput() const { return input_.get(); }

    // デッキインスタンスの取得とセット
    const std::vector<CardInstance>& GetDeckInstances() const { return deckInstances_; }
    void SetDeckInstances(const std::vector<CardInstance>& instances) { deckInstances_ = instances; }
    void SetDeckInstancesFromId(const std::vector<int>& ids);

    //CardInstance MakeCardInstance(int defId);

private:
    bool Initialize_();
    void Finalize_();
    void WarmupAssets_();
private:
    bool quit_ = false;

    std::unique_ptr<WinApp> win_;
    std::unique_ptr<DirectXCommon> dx_;
    std::unique_ptr<SrvManager> srv_;

    std::unique_ptr<SpriteCommon> spriteCommon_;
    std::unique_ptr<Object3dCommon> objCommon_;
    std::unique_ptr<ParticleCommon> particleCommon_;
    std::unique_ptr<ImGuiManagaer> imgui_;

    std::unique_ptr<SceneManager> sceneMgr_;
    std::unique_ptr<Input> input_; 
    std::unique_ptr<SkinningCommon> skinCom_;

    std::unique_ptr<Bloom> bloom_;
    std::unique_ptr<RtvManager> rtv_;

    std::vector<CardInstance> deckInstances_;
};
