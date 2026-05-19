#pragma once

#include "IScene.h"
#include "BattleController.h"
#include "Camera.h"
#include "Object3d.h"
#include "GameResultPopup.h"
#include <memory>

class GameApp;
class Input;

class FieldEditerScene : public IScene {
public:
	void OnEnter(GameApp& app) override;
	void OnExit(GameApp& app) override;
	void Update(GameApp& app, float dt) override;
	void Draw3D(GameApp& app) override;
	void Draw2D(GameApp& app) override;         // ポップアップ描画用
	void DrawPostEffect3DLate(GameApp& app) override;
	void DrawImGui(GameApp& app) override;
	void DrawSkydome(GameApp& app) override;

private:
	void UpdateCamera_(float dt);
	void RebuildPreview_();

	Input* input_ = nullptr;
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<Object3d> skyDome_;
	BattleController battle_;

	// リザルトポップアップ（デバッグ表示用）
	std::unique_ptr<GameResultPopup> resultPopup_;

	int previewCardCount_ = 5;
	int previewFirstCardId_ = 1;
	bool previewDirty_ = false;

	float cameraDist_ = 40.0f;
	float cameraTheta_ = 0.0f;
	float cameraPhi_ = 0.15f;
	Vector3 cameraTarget_ = { 0.0f, 0.0f, 5.0f };

	// GameSceneの画面分割設定に合わせたプレビュー補正
	bool  useGameCameraMode_   = true;    // true: ゲームと同じ分割カメラ設定を模倣
	float splitRatio_          = 0.465f;  // GameScene::splitRatio_ と合わせる
	float fieldCameraZoom_     = 1.0f;    // GameScene::fieldCameraZoom_
	float fieldCameraRotXOffset_ = 0.08f; // GameScene::fieldCameraRotXOffset_
};
