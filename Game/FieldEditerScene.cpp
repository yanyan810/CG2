#include "FieldEditerScene.h"

#include "GameApp.h"
#include "Input.h"
#include "WinApp.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "ModelManager.h"
#include <algorithm>
#include <cmath>
#include <imgui.h>

void FieldEditerScene::OnEnter(GameApp& app)
{
	input_ = app.GetInput();

	ModelManager::GetInstance()->LoadModel("skydome/skydome.obj");

	camera_ = std::make_unique<Camera>();
	camera_->SetTranslate({ 0.0f, 4.0f, -40.0f });
	camera_->SetRotate({ 0.15f, 0.0f, 0.0f });
	camera_->Update();
	app.ObjCom()->SetDefaultCamera(camera_.get());

	skyDome_ = std::make_unique<Object3d>();
	skyDome_->Initialize(app.ObjCom(), app.Dx());
	skyDome_->SetModel("skydome/skydome.obj");
	skyDome_->SetCamera(camera_.get());
	skyDome_->SetEnableLighting(0);
	skyDome_->SetScale({ 100.0f, 100.0f, 100.0f });

	battle_.Initialize(app, camera_.get());
	RebuildPreview_();

	// リザルトポップアップ初期化
	if (!resultPopup_) {
		resultPopup_ = std::make_unique<GameResultPopup>();
		resultPopup_->Initialize(app);
	} else {
		resultPopup_->Hide();
	}

	pausingUI_ = std::make_unique<PausingUI>();
	pausingUI_->Initialize(app);
}

void FieldEditerScene::OnExit(GameApp& app)
{
	(void)app;
	battle_.Finalize();
	skyDome_.reset();
	camera_.reset();
	input_ = nullptr;
	if (resultPopup_) { resultPopup_->Hide(); }
}

void FieldEditerScene::Update(GameApp& app, float dt)
{
	// ポップアップ表示中はポップアップの操作のみ受け付ける
	if (resultPopup_ && resultPopup_->IsVisible()) {
		resultPopup_->Update(app, dt);
		// FieldEditerではシーン遷移せずHideするだけ（デバッグ確認用）
		if (resultPopup_->GetAction() != ResultAction::None) {
			resultPopup_->ClearAction();
			resultPopup_->Hide();
		}
		return;
	}

	if (!input_) { return; }

	if (pausingUI_) {
		pausingUI_->Update(app, input_);
		if (pausingUI_->GetIsPaused()) {
			return;
		}
	}

	if (previewDirty_) {
		RebuildPreview_();
	}

	UpdateCamera_(dt);
	battle_.UpdateFieldEditorPreview(dt);
}

void FieldEditerScene::Draw3D(GameApp& app)
{
	battle_.DrawFieldEditorPreview3D(app);
}

void FieldEditerScene::DrawPostEffect3DLate(GameApp& app)
{
	battle_.DrawFieldEditorPostEffect3D(app);
}

void FieldEditerScene::Draw2D(GameApp& app)
{
	// リザルトポップアップを最前面に描画
	if (resultPopup_ && resultPopup_->IsVisible()) {
		resultPopup_->Draw2D(app);
	}

	if (pausingUI_) {
		pausingUI_->Draw(app);
	}
}

void FieldEditerScene::DrawSkydome(GameApp& app)
{
	(void)app;
	if (skyDome_) {
		skyDome_->Update(0.0f);
		skyDome_->Draw();
	}
}

void FieldEditerScene::DrawImGui(GameApp& app)
{
	(void)app;
#ifdef USE_IMGUI
	ImGui::Begin("FieldEditerScene");
	ImGui::Text("ESC : Title");

	// リザルトポップアップ デバッグ操作
	ImGui::Separator();
	ImGui::TextColored({ 1.0f, 0.9f, 0.2f, 1.0f }, "Result Popup Preview");
	if (ImGui::Button("Show GAME OVER")) {
		if (resultPopup_) { resultPopup_->Show(ResultKind::GameOver); }
	}
	ImGui::SameLine();
	if (ImGui::Button("Show GAME CLEAR")) {
		if (resultPopup_) { resultPopup_->Show(ResultKind::GameClear); }
	}
	ImGui::SameLine();
	if (ImGui::Button("Hide")) {
		if (resultPopup_) { resultPopup_->Hide(); }
	}
	ImGui::Separator();

	if (ImGui::DragInt("Preview Card Count", &previewCardCount_, 1.0f, 0, 5)) {
		previewCardCount_ = std::clamp(previewCardCount_, 0, 5);
		previewDirty_ = true;
	}
	if (ImGui::DragInt("First Card ID", &previewFirstCardId_, 1.0f, 1, 99)) {
		previewFirstCardId_ = std::max(1, previewFirstCardId_);
		previewDirty_ = true;
	}
	ImGui::DragFloat3("Camera Target", &cameraTarget_.x, 0.1f);
	ImGui::DragFloat("Camera Distance", &cameraDist_, 0.2f, 5.0f, 100.0f);
	ImGui::DragFloat("Camera Theta", &cameraTheta_, 0.01f);
	ImGui::DragFloat("Camera Phi", &cameraPhi_, 0.01f, -1.2f, 1.2f);
	if (ImGui::Button("Reset Camera")) {
		cameraDist_ = 40.0f;
		cameraTheta_ = 0.0f;
		cameraPhi_ = 0.15f;
		cameraTarget_ = { 0.0f, 0.0f, 5.0f };
	}
	ImGui::Separator();
	ImGui::Checkbox("Game Camera Mode (Split)", &useGameCameraMode_);
	if (useGameCameraMode_) {
		ImGui::DragFloat("Split Ratio", &splitRatio_, 0.005f, 0.1f, 0.9f);
		ImGui::DragFloat("Field Zoom", &fieldCameraZoom_, 0.01f, 0.1f, 5.0f);
		ImGui::DragFloat("Rot X Offset", &fieldCameraRotXOffset_, 0.005f, -1.0f, 1.0f);
	}
	ImGui::End();

	// リザルトポップアップのImGui（レイアウト調整・保存）
	if (resultPopup_) { resultPopup_->DrawImGui(); }

	if (pausingUI_) { pausingUI_->DrawImGui(); }

	battle_.DrawFieldSceneEditerImGui();
#endif
}

void FieldEditerScene::UpdateCamera_(float dt)
{
	(void)dt;
	if (!camera_ || !input_) {
		return;
	}

#ifdef USE_IMGUI
	const bool imguiWantsMouse = ImGui::GetIO().WantCaptureMouse;
#else
	constexpr bool imguiWantsMouse = false;
#endif

	if (!imguiWantsMouse) {
		if (input_->IsMousePressed(1)) {
			const POINT delta = input_->GetMouseDelta();
			cameraTheta_ += static_cast<float>(delta.x) * 0.005f;
			cameraPhi_ += static_cast<float>(delta.y) * 0.005f;
			cameraPhi_ = std::clamp(cameraPhi_, -1.2f, 1.2f);
		}

		const int32_t wheel = input_->GetWheel();
		if (wheel != 0) {
			cameraDist_ -= static_cast<float>(wheel) * 0.002f;
			cameraDist_ = std::clamp(cameraDist_, 5.0f, 100.0f);
		}
	}

	const float cosPhi = std::cos(cameraPhi_);
	const Vector3 pos = {
		cameraTarget_.x + std::sin(cameraTheta_) * cosPhi * cameraDist_,
		cameraTarget_.y + std::sin(cameraPhi_) * cameraDist_,
		cameraTarget_.z - std::cos(cameraTheta_) * cosPhi * cameraDist_,
	};

	camera_->SetTranslate(pos);
	camera_->SetRotate({ cameraPhi_, cameraTheta_, 0.0f });

	// --- GameSceneの画面分割を模倣したカメラ設定 ---
	if (useGameCameraMode_) {
		const int windowW = WinApp::kClientWidth;
		const int windowH = WinApp::kClientHeight;
		const int fieldHeight = windowH - static_cast<int>(windowH * splitRatio_);

		camera_->SetAspect(static_cast<float>(windowW) / fieldHeight);

		constexpr float kOrigFovY = 0.45f;
		const float zoomRatio = (static_cast<float>(fieldHeight) / windowH) / fieldCameraZoom_;
		const float newFovY = 2.0f * std::atan(zoomRatio * std::tan(kOrigFovY / 2.0f));
		camera_->SetFovY(newFovY);

		// 少し下を向けてカードが見切れないようにする
		const float rotX = cameraPhi_ + fieldCameraRotXOffset_;
		camera_->SetRotate({ rotX, cameraTheta_, 0.0f });

		// ProjectionShift：画面下半分にカードエリアを限定
		Matrix4x4 shiftField = Matrix4x4::MakeIdentity4x4();
		shiftField.m[1][1] = 1.0f - splitRatio_;
		shiftField.m[3][1] = -splitRatio_;
		camera_->SetProjectionShift(shiftField);
	} else {
		// シンプルモード：ProjectionShiftなし
		camera_->SetProjectionShift(Matrix4x4::MakeIdentity4x4());
	}

	camera_->Update();
}

void FieldEditerScene::RebuildPreview_()
{
	previewDirty_ = false;
	battle_.BuildFieldEditorPreview(previewCardCount_, previewFirstCardId_);
}
