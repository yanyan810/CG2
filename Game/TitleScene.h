#pragma once
#include "IScene.h"
#include <memory>
#include "Matrix4x4.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "Sprite.h"
#include "SpriteCommon.h"

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

    Vector3 imguiCamPos_={ 0.0f, 3.0f, -20.0f };
    Vector3 imguiCamRot_={ 0.0f, 0.0f, 0.0f };

    std::unique_ptr<Particle> particle_;

    std::unique_ptr<Object3d> skyDome_;

    std::unique_ptr < Sprite> bg_;

	std::unique_ptr < Sprite> pressStart_;

    enum class State {
        Idle,
        ExitClose
    };

    State state_ = State::Idle;

    float circle_ = 1.0f;   // ★Titleは最初から開いている
    float softness_ = 0.6f;

    const char* kNextScene_ = "Test"; // SPACE後に行く先

    //確認
    std::unique_ptr<Object3d> testObj_;
    std::unique_ptr<Object3d> terrainObj_;

    Vector3 testPos_{ 0.0f, 1.0f, 0.0f };
    Vector3 testRot_{ 0.0f, 0.0f, 0.0f };   // ラジアンなら 0.01刻み、度なら 1.0刻み
    Vector3 testScale_{ 2.0f, 2.0f, 2.0f };

    // 確認用パラメータ
    float shininess_ = 64.0f;
    int lightingMode_ = 1;     // 1:Lambert 2:HalfLambert 3:SpecOnly
    bool orbitCam_ = true;
    float orbitSpeed_ = 0.6f;
    float orbitRadius_ = 10.0f;
    float orbitT_ = 0.0f;

    Vector3 lightDir_ = { 0.0f, -1.0f, -1.0f };
    float   lightIntensity_ = 0.0f;
    Vector4 lightColor_ = { 1,1,1,1 };

    Vector3 pointPos_{ 0.0f, 3.0f, -3.0f };
    Vector4 pointColor_{ 1,1,1,1 };
    float   pointIntensity_ = 1.0f;
	float pointRadius_ = 10.0f;
    float pointDecay_ = 1.0f;

    // 確認用：Pointだけ見る
    int lightViewMode_ = 0; // 0:全部 1:Directionalのみ 2:Pointのみ
    
    // ---- SpotLight (Debug) ----
    Vector3 spotPos_ = { 0.0f, 3.0f, 0.0f };
    Vector3 spotDir_ = { 0.0f, -1.0f, 0.0f };      // ※正規化推奨
    float   spotIntensity_ = 2.0f;
    float   spotDistance_ = 15.0f;
    float   spotDecay_ = 1.0f;
    float spotAngleDeg_ = 30.0f;         // 外側
    float spotFalloffStartDeg_ = 15.0f;  // ★内側（外側より小さく）
    Vector3 spotColor_ = { 1.0f, 1.0f, 1.0f }; // RGB
    float spotCos;

};
