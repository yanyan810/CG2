#pragma once
#include <string>
#include <vector>

class ParticleEditor {
public:
    static ParticleEditor* GetInstance();

    void Initialize();
    void Update();
    void DrawImGui();
    void Save(const std::string& filename);
    void Load(const std::string& filename);

private:
    ParticleEditor() = default;
    ~ParticleEditor() = default;
    ParticleEditor(const ParticleEditor&) = delete;
    ParticleEditor& operator=(const ParticleEditor&) = delete;

    void ScanResources();

    std::vector<std::string> modelFiles_;
    std::vector<std::string> textureFiles_;
    std::vector<std::string> jsonFiles_;
    int selectedJsonIndex_ = -1;
    bool isResourcesScanned_ = false;
    bool isLoadRequested_ = false;
    std::string loadFileName_ = "";

    char saveFileName_[256] = "particles.json";
    char newGroupName_[256] = "NewParticle";
};
