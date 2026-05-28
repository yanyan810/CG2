#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "SceneManager.h"
#include "Input.h"
#include "SpriteCommon.h"
#include "Bloom.h"
#include "ObjectPostEffect.h"
#include "AudioManager.h"
#include "Matrix4x4.h"
#include "UI/BattleActionDirector.h"

#include"CardInstance.h"
#include "TextSprite.h"

#include <string>
#include <unordered_map>
#include <vector>

class CardDatabase;
class WinApp;
class DirectXCommon;
class SrvManager;
class Object3dCommon;
class ParticleCommon;
class ImGuiManagaer;
class SkinningCommon;
class Sprite;
class ModelParticleManager;

class SceneManager;

class GameApp {
public:
    enum class LoadingMode {
        BootToTitle,
        StageToGame,
    };

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
    ObjectPostEffect* ObjectPost() const { return objectPostEffect_.get(); }

    SceneManager& Scenes() { return *sceneMgr_; }

    void Update(float dt);

    void Draw3D();
    void Draw2D();
    void DrawImGui();
    void Draw();
    void BeginObjectPostEffect();
    void EndObjectPostEffect();
    void EndObjectPostEffectToBloomScene();
    void DrawSpriteObjectPost(Sprite* sprite, const Matrix4x4& view, const Matrix4x4& proj, const BloomParam& param);
    void DrawModelParticlesObjectPost(ModelParticleManager* particles, const BloomParam& param);
    void DrawModelParticlesObjectPostToBloomScene(ModelParticleManager* particles, const BloomParam& param, int clipHeight = 0);
    void SetRadialBlur(float strength);
    void ResetRadialBlur();
    int clipHeight = 0;

    Input* GetInput() { return input_.get(); }
    const Input* GetInput() const { return input_.get(); }

    // デッキインスタンスの取得とセット
    const std::vector<CardInstance>& GetDeckInstances() const { return deckInstances_; }
    void SetDeckInstances(const std::vector<CardInstance>& instances) { deckInstances_ = instances; }
    void SetDeckInstancesFromId(const std::vector<int>& ids);

    void SetSelectedStage(int stageId, const std::string& configPath);
    int GetSelectedStageId() const { return selectedStageId_; }
    const std::string& GetSelectedStageConfigPath() const { return selectedStageConfigPath_; }
    std::string GetSelectedStageFieldConfigPath() const;

    CardDatabase* GetCardDB() { return cardDB_.get(); }

    const ActionSequenceProfile* FindActionSequenceProfile(const std::string& name) const;
    const ActionSequenceProfile* PickCardUseSequenceProfile() const;
    const ActionSequenceProfile* PickCardEffectSequenceProfile(
        int cardId,
        const std::vector<std::string>& effectTypes) const;

	void LoadDeck(const std::string& deckConfigPath);
    void BeginStartupLoading();
    bool LoadStartupStep();
    float GetStartupLoadingProgress() const;
    void SetLoadingMode(LoadingMode mode) { loadingMode_ = mode; }
    LoadingMode GetLoadingMode() const { return loadingMode_; }

private:
    bool Initialize_();
    void Finalize_();
    void WarmupAssets_();
    void BuildStartupLoadSteps_();
    void LoadActionSequenceProfiles_();
    const ActionSequenceProfile* PickSequenceFromNames_(const std::vector<std::string>& names) const;
    std::string NormalizeActionSequenceEffectType_(const std::string& effectType) const;
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
    std::unique_ptr<ObjectPostEffect> objectPostEffect_;
    std::unique_ptr<RtvManager> rtv_;

    std::vector<CardInstance> deckInstances_;
    int selectedStageId_ = 1;
    std::string selectedStageConfigPath_ = "resources/stages/stage01.json";

    std::unique_ptr<CardDatabase> cardDB_;

    std::unordered_map<std::string, ActionSequenceProfile> actionSequenceProfiles_;
    std::vector<std::string> cardUseSequenceNames_;
    std::unordered_map<std::string, std::vector<std::string>> effectSequenceNames_;
    std::unordered_map<int, std::vector<std::string>> cardSequenceNames_;
    std::vector<std::function<void()>> startupLoadSteps_;
    size_t startupLoadIndex_ = 0;
    LoadingMode loadingMode_ = LoadingMode::BootToTitle;
};
