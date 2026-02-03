#pragma once
#include "IScene.h"
#include <memory>
#include "Matrix4x4.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "Sprite.h"
#include <array>     
#include <string>   
#include "SpriteCommon.h"
#include "Particle.h"
#include "VideoPlayerMF.h"

#include "Player.h"
#include "PlayerCombo.h"

class Particle;
class Camera;

class TitleScene : public IScene {
public:
	void OnEnter(GameApp& app) override;
	void OnExit(GameApp& app) override;

	void Update(GameApp& app, float dt) override;
	void Draw(GameApp& app) override;

private:

private:
	void DrawImGui_ModelSwitchersOneWindow();
	void DrawImGui_ModelSwitchBlock(const char* header,
		const char* comboLabel,
		int& index,
		const char* const* paths,
		int count,
		const char* const* labels,
		int labelCount);


	bool prevSpace_ = false;

	std::unique_ptr<Camera> camera_;

	Vector3 imguiCamPos_ = { 0.0f, 3.0f, -20.0f };
	Vector3 imguiCamRot_ = { 0.0f, 0.0f, 0.0f };

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
	std::unique_ptr<Object3d> nodeObj_;

	Vector3 testPos_{ 0.0f, 1.0f, 0.0f };
	Vector3 testRot_{ 0.0f, 0.0f, 0.0f };   // ラジアンなら 0.01刻み、度なら 1.0刻み
	Vector3 testScale_{ 2.0f, 2.0f, 2.0f };

	// 確認用パラメータ
	float shininess_ = 64.0f;
	int lightingMode_ = 2;     // 1:Lambert 2:HalfLambert 3:SpecOnly
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

	// ============================
	//Animation_Node 切替
	// ============================
	std::array<const char*, 6> nodeModelPaths_ = {
		"Animation_Node/Animation_Node_00.gltf",
		"Animation_Node/Animation_Node_01.gltf",
		"Animation_Node/Animation_Node_02.gltf",
		"Animation_Node/Animation_Node_03.gltf",
		"Animation_Node/Animation_Node_04.gltf",
		"Animation_Node/Animation_Node_05.gltf",
	};

	int nodeModelIndex_ = 0;       // ★初期は00
	int nodeModelIndexPrev_ = 0;   // ★変更検知用


	// 追加：Misc側
	std::unique_ptr<Object3d> nodeMiscObj_;

	// ============================
	// Animation_NodeMisc 切替（00～08）
	// ============================
	std::array<const char*, 9> nodeMiscModelPaths_ = {
		"Animation_NodeMisc/Animation_NodeMisc_00.gltf",
		"Animation_NodeMisc/Animation_NodeMisc_01.gltf",
		"Animation_NodeMisc/Animation_NodeMisc_02.gltf",
		"Animation_NodeMisc/Animation_NodeMisc_03.gltf",
		"Animation_NodeMisc/Animation_NodeMisc_04.gltf",
		"Animation_NodeMisc/Animation_NodeMisc_05.gltf",
		"Animation_NodeMisc/Animation_NodeMisc_06.gltf",
		"Animation_NodeMisc/Animation_NodeMisc_07.gltf",
		"Animation_NodeMisc/Animation_NodeMisc_08.gltf",
	};

	int nodeMiscModelIndex_ = 0;       // 初期
	int nodeMiscModelIndexPrev_ = 0;   // 変更検知用

	// ============================
// Animation_Skin 切替（00～11）
// ============================
	std::array<const char*, 12> skinModelPaths_ = {
		"Animation_Skin/Animation_Skin_00.gltf",
		"Animation_Skin/Animation_Skin_01.gltf",
		"Animation_Skin/Animation_Skin_02.gltf",
		"Animation_Skin/Animation_Skin_03.glb",
		"Animation_Skin/Animation_Skin_04.gltf",
		"Animation_Skin/Animation_Skin_05.gltf",
		"Animation_Skin/Animation_Skin_06.gltf",
		"Animation_Skin/Animation_Skin_07.gltf",
		"Animation_Skin/Animation_Skin_08.gltf",
		"Animation_Skin/Animation_Skin_09.gltf",
		"Animation_Skin/Animation_Skin_10.gltf",
		"Animation_Skin/Animation_Skin_11.gltf",
	};

	int skinModelIndex_ = 0;
	int skinModelIndexPrev_ = 0;

	// 追加：Skin側
	std::unique_ptr<Object3d> skinObj_;

	// ============================
// Mesh_Primitives（00のみ）
// ============================
	std::array<const char*, 1> meshPrimPaths_ = {
		"Mesh_Primitives/Mesh_Primitives_00.gltf",
	};
	int meshPrimIndex_ = 0;
	int meshPrimIndexPrev_ = 0;
	std::unique_ptr<Object3d> meshPrimObj_;

	// ============================
	// Material_AlphaBlend（00～06）
	// ============================
	std::array<const char*, 7> alphaBlendPaths_ = {
		"Material_AlphaBlend/Material_AlphaBlend_00.gltf",
		"Material_AlphaBlend/Material_AlphaBlend_01.gltf",
		"Material_AlphaBlend/Material_AlphaBlend_02.gltf",
		"Material_AlphaBlend/Material_AlphaBlend_03.gltf",
		"Material_AlphaBlend/Material_AlphaBlend_04.gltf",
		"Material_AlphaBlend/Material_AlphaBlend_05.gltf",
		"Material_AlphaBlend/Material_AlphaBlend_06.gltf",
	};
	int alphaBlendIndex_ = 0;
	int alphaBlendIndexPrev_ = 0;
	std::unique_ptr<Object3d> alphaBlendObj_;

	// ============================
	// Texture_Sampler（00～13）
	// ============================
	std::array<const char*, 14> texSamplerPaths_ = {
		"Texture_Sampler/Texture_Sampler_00.gltf",
		"Texture_Sampler/Texture_Sampler_01.gltf",
		"Texture_Sampler/Texture_Sampler_02.gltf",
		"Texture_Sampler/Texture_Sampler_03.gltf",
		"Texture_Sampler/Texture_Sampler_04.gltf",
		"Texture_Sampler/Texture_Sampler_05.gltf",
		"Texture_Sampler/Texture_Sampler_06.gltf",
		"Texture_Sampler/Texture_Sampler_07.gltf",
		"Texture_Sampler/Texture_Sampler_08.gltf",
		"Texture_Sampler/Texture_Sampler_09.gltf",
		"Texture_Sampler/Texture_Sampler_10.gltf",
		"Texture_Sampler/Texture_Sampler_11.gltf",
		"Texture_Sampler/Texture_Sampler_12.gltf",
		"Texture_Sampler/Texture_Sampler_13.gltf",
	};
	int texSamplerIndex_ = 0;
	int texSamplerIndexPrev_ = 0;
	std::unique_ptr<Object3d> texSamplerObj_;

	enum class EditTarget {
		SkyDome,
		VideoPlane,
		Particle,
		Ground,      // 追加
		TitlePlayer, // 追加
		BG,
		PressStart,
	};



	int editTarget_ = 0; // ImGui Combo用（EditTarget を int で持つ）

	struct SRT {
		Vector3 pos{ 0.0f, 1.0f, 0.0f };
		Vector3 rot{ 0.0f, 0.0f, 0.0f };
		Vector3 scale{ 2.0f, 2.0f, 2.0f };
	};

	SRT srtTest_{};
	SRT srtTerrain_{};
	SRT srtNode_{};
	SRT srtNodeMisc_{};
	SRT srtSkin_{};
	SRT srtMeshPrim_{};
	SRT srtAlphaBlend_{};
	SRT srtTexSampler_{};
	SRT srtSky_{};
	SRT srtVideo_{};
	SRT srtParticle_{};
	SRT srtBG_{};
	SRT srtPress_{};
	

	// === Assimp plane 切替 ===
	std::array<const char*, 2> assimpPlanePaths_ = {
		"plane.obj",
		"plane.gltf",
	};
	int assimpPlaneIndex_ = 0;
	int assimpPlaneIndexPrev_ = 0;

	std::unique_ptr<Object3d> videoPlane_;
	std::unique_ptr<Player> titlePlayer;
	std::unique_ptr<VideoPlayerMF> video_; // もしくは値型でもOK
	bool enableVideo_ = true;              // ImGuiでON/OFFできるように

	// ===== Lighting params =====
	LightingParam light_;

	std::unique_ptr<Object3d> ground_;

	// TitleScene.h の private に追加
	bool showVideo_ = true;        // true: Video表示 / false: Player表示
	float showTimer_ = 0.0f;       // 経過時間
	float switchInterval_ = 2.0f;  // 何秒ごとに切り替えるか（好み）
	float dt_;

	float switchT_ = 0.0f;
	float playerSec_ = 28.0f; // 前半プレイヤー秒
	float videoSec_ = 28.0f; // 後半動画秒

	SRT srtGround_{};
	SRT srtPlayer_{};

};
