#pragma once
#include "IScene.h"
#include <memory>
#include "Matrix4x4.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "Input.h"
#include "Sprite.h"
#include <array>     
#include <string>   
#include "SpriteCommon.h"
#include "Particle.h"
#include "VideoPlayerMF.h"

#include "Player.h"
#include "PlayerCombo.h"

#include "FieldUi.h"

#include "BattleController.h"

class Particle;
class Camera;

class TitleScene : public IScene {
public:
	void OnEnter(GameApp& app) override;
	void OnExit(GameApp& app) override;

	void Update(GameApp& app, float dt) override;
	void Draw3D(GameApp& app) override;
	void Draw2D(GameApp& app) override;
	void DrawImGui(GameApp& app) override;

	void DrawSkydome(GameApp& app) override;

	void DrawPostEffect3D(GameApp& app) override;
	void DrawPostEffect2D(GameApp& app) override;

private:
	//--------------------------------------------------------
	// タイトルの状態
	//--------------------------------------------------------
	enum class State {
		Idle,       // 入力待ち
		ExitClose   // 閉じ演出中
	};

	State state_ = State::Idle;

	//--------------------------------------------------------
	// カメラ
	// ※ 今回は3Dをほぼ描かないが、Sceneとして持っておく
	//--------------------------------------------------------
	std::unique_ptr<Camera> camera_;

	//--------------------------------------------------------
	// 3D表示用モデル
	//--------------------------------------------------------
	std::unique_ptr<Object3d> skyDome_; // 背景の天球


	//--------------------------------------------------------
	// 2D表示用スプライト
	//--------------------------------------------------------
	std::unique_ptr<Sprite> bg_;          // 背景画像
	std::unique_ptr<Sprite> pressStart_;  // Press Space画像

	//--------------------------------------------------------
	// 円形マスク演出用
	//--------------------------------------------------------
	float circle_ = 1.0f;     // 1.0 = 全開, 0.0 = 完全に閉じる
	float softness_ = 0.6f;

	//--------------------------------------------------------
	// 遷移先
	//--------------------------------------------------------
	const char* kNextScene_ = "Game";

	//先読み
	BattleController battle_;

};