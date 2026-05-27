#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

class GameApp;
class IScene;

class SceneManager {
public:
    using Factory = std::function<std::unique_ptr<IScene>()>;

    void Register(const std::string& name, Factory factory);
    void Change(GameApp& app, const std::string& name);
    void ChangeToPrepared(GameApp& app, const std::string& name, std::unique_ptr<IScene> scene);
    void RequestPreparedChange(const std::string& name, std::unique_ptr<IScene> scene);

    void Update(GameApp& app, float dt);

    void Draw3D(GameApp& app);
    void Draw2D(GameApp& app);
    void DrawImGui(GameApp& app);

    void DrawSkydome(GameApp& app);

    void DrawPostEffect3D(GameApp& app);
    void DrawPostEffect2D(GameApp& app);
    void DrawPostEffect3DLate(GameApp& app);

    IScene* Current() { return current_.get(); }

private:
    std::unordered_map<std::string, Factory> factories_;
    std::unique_ptr<IScene> current_;
    std::unique_ptr<IScene> pendingPreparedScene_;
    std::string currentName_;
    std::string pendingPreparedName_;
};
