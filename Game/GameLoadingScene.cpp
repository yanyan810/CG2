#include "GameLoadingScene.h"

#include "GameApp.h"
#include "Matrix4x4.h"
#include "ModelManager.h"
#include "WinApp.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

using json = nlohmann::json;

void GameLoadingScene::OnEnter(GameApp& app)
{
	loadIndex_ = 0;
	waitFrames_ = 0;
	CollectLoadTargets_(app);

	loadingText_ = std::make_unique<TextSprite>();
	loadingText_->Initialize(app.SpriteCom(), app.Dx());
	loadingText_->SetFontSize(36);
	loadingText_->SetSize({ 1.0f, 1.0f, 1.0f });
	loadingText_->SetPosition({ 460.0f, 330.0f });
	loadingText_->SetColor({ 1.0f, 1.0f, 1.0f });
	UpdateText_();
}

void GameLoadingScene::OnExit(GameApp& app)
{
	(void)app;
	loadingText_.reset();
	modelPaths_.clear();
	loadIndex_ = 0;
	waitFrames_ = 0;
}

void GameLoadingScene::CollectLoadTargets_(GameApp& app)
{
	modelPaths_.clear();
	std::unordered_set<std::string> seen;

	const auto addModel = [&](const std::string& path) {
		if (!path.empty() && seen.insert(path).second) {
			modelPaths_.push_back(path);
		}
		};

	const std::string fieldConfigPath = app.GetSelectedStageFieldConfigPath();
	std::ifstream file(fieldConfigPath);
	if (file.is_open()) {
		json props;
		try {
			file >> props;
			if (props.is_array()) {
				for (const auto& prop : props) {
					addModel(prop.value("modelPath", ""));
				}
			}
		} catch (...) {
			modelPaths_.clear();
			seen.clear();
		}
	}

	if (modelPaths_.empty() && app.GetSelectedStageId() >= 1 && app.GetSelectedStageId() <= 4) {
		addModel("Field/ForestField/forestFIeld.obj");
		addModel("Field/ForestField/glassField.obj");
	}
}

void GameLoadingScene::UpdateText_()
{
	if (!loadingText_) {
		return;
	}

	std::wstring text = L"Loading battle field";
	if (!modelPaths_.empty()) {
		text += L" ";
		text += std::to_wstring((std::min)(loadIndex_, modelPaths_.size()));
		text += L"/";
		text += std::to_wstring(modelPaths_.size());
	}
	text += L"...";
	loadingText_->SetText(text);
}

void GameLoadingScene::LoadCurrentTarget_()
{
	if (loadIndex_ >= modelPaths_.size()) {
		RequestChangeScene_("Game");
		return;
	}

	const std::string& path = modelPaths_[loadIndex_];
	ModelManager* modelManager = ModelManager::GetInstance();
	if (!modelManager->FindModel(path)) {
		modelManager->LoadModel(path);
	}

	++loadIndex_;
	UpdateText_();
}

void GameLoadingScene::Update(GameApp& app, float dt)
{
	(void)app;
	(void)dt;

	if (waitFrames_ < 1) {
		++waitFrames_;
		return;
	}

	LoadCurrentTarget_();
}

void GameLoadingScene::Draw2D(GameApp& app)
{
	(void)app;
	if (!loadingText_) {
		return;
	}

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
		0, 0,
		static_cast<float>(WinApp::kClientWidth),
		static_cast<float>(WinApp::kClientHeight),
		0, 100
	);

	loadingText_->Update(view, proj);
	loadingText_->Draw();
}
