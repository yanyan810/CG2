#include "TutorialScene.h"
#include "GameApp.h"
#include "Input.h"
#include "WinApp.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

void TutorialScene::OnEnter(GameApp& app) {
    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate({ 0.0f, 4.0f, -40.0f });
    camera_->SetRotate({ 0.15f, 0.0f, 0.0f });
    app.ObjCom()->SetDefaultCamera(camera_.get());

    animCamera_ = std::make_unique<Camera>();
    animCamera_->SetTranslate({ 0.0f, 4.0f, -40.0f });
    animCamera_->SetRotate({ 0.15f, 0.0f, 0.0f });

    cameraAnim_ = std::make_unique<CameraAnimator>();
    cameraAnim_->Initialize(animCamera_.get(), app.GetInput());

    skyDome_ = std::make_unique<Object3d>();
    skyDome_->Initialize(app.ObjCom(), app.Dx());
    skyDome_->SetModel("skydome/skydome.obj");
    skyDome_->SetCamera(animCamera_.get());
    skyDome_->SetEnableLighting(0);
    skyDome_->SetTranslate({ 0.0f, 0.0f, 0.0f });
    skyDome_->SetScale({ 100.0f, 100.0f, 100.0f });

    const float charZ = 15.0f;

    player_ = std::make_unique<Player>();
    player_->Initialize(app.ObjCom(), app.Dx(), animCamera_.get());
    player_->SetSpawnPos({ -7.0f, 0.0f, charZ });
    player_->SetRotation({ 0.0f, 1.5708f, 0.0f });

    enemyMgr_.Initialize(app.ObjCom(), app.Dx(), animCamera_.get());
    enemyMgr_.Spawn(EnemyType::Slime, { 7.0f, 0.0f, 15.0f });

    auto MakeTutorialCard = [](int defId, int number, CardSuit suit) {
        CardInstance c{};
        c.defId = defId;
        c.number = number;
        c.suit = suit;
        return c;
        };

    std::vector<CardInstance> openingHand = {
        MakeTutorialCard(1, 7, CardSuit::Spade),
        MakeTutorialCard(2, 7, CardSuit::Heart),
        MakeTutorialCard(3, 3, CardSuit::Diamond),
        MakeTutorialCard(4, 10, CardSuit::Club),
        MakeTutorialCard(5, 13, CardSuit::Spade),
    };

    battle_.SetPlayer(player_.get());
    battle_.SetEnemyManager(&enemyMgr_);
    battle_.SetTutorialOpeningHand(openingHand);
    battle_.Initialize(app, animCamera_.get());

    fieldUi_ = std::make_unique<FieldUi>();
    fieldUi_->Initialize(app);

    tutorial_ = std::make_unique<TutorialManager>();
    tutorial_->Initialize();

    tutorialUi_ = std::make_unique<TutorialUi>();
    tutorialUi_->Initialize(app);

    // 円マスク開始設定
    state_ = State::EnterOpen;
    circle_ = 0.0f;
    softness_ = 0.6f;
    nextSceneName_ = "Title";
    prevEsc_ = false;
}
void TutorialScene::OnExit(GameApp& app) {
    (void)app;
    tutorialUi_.reset();
    tutorial_.reset();
    fieldUi_.reset();

    player_.reset();
    skyDome_.reset();
    cameraAnim_.reset();
    animCamera_.reset();
    camera_.reset();

    battle_.Finalize();
}

void TutorialScene::Update(GameApp& app, float dt) {
    Input* input = app.GetInput();
    if (!input) {
        return;
    }

    // ---------------------------------
    // 円マスク状態更新
    // ---------------------------------
    switch (state_) {
    case State::EnterOpen:
        circle_ += 1.8f * dt;
        if (circle_ >= 1.0f) {
            circle_ = 1.0f;
            state_ = State::Idle;
        }
        break;

    case State::ExitClose:
        circle_ -= 1.8f * dt;
        if (circle_ <= 0.0f) {
            circle_ = 0.0f;
            RequestChangeScene_(nextSceneName_);
            return;
        }
        break;

    case State::Idle:
    default:
        break;
    }

    bool currEsc = input->IsKeyPressed(DIK_ESCAPE);
    if (currEsc && !prevEsc_) {
        if (state_ == State::Idle) {
            nextSceneName_ = "Title";
            state_ = State::ExitClose;
        }
        return;
    }
    prevEsc_ = currEsc;

    if (cameraAnim_) {
        cameraAnim_->Update(dt);
    }
    if (animCamera_) {
        animCamera_->Update();
    }

    if (skyDome_) {
        skyDome_->SetCamera(animCamera_.get());
        skyDome_->Update(dt);
    }

    if (player_) {
        player_->SetCamera(animCamera_.get());
        player_->Update(dt);
    }

    enemyMgr_.UpdateCamera(animCamera_.get());
    enemyMgr_.Update(dt);

    battle_.Update(app, *fieldUi_, dt);

    if (tutorial_) {
        bool nextTutorial = false;

        if (input->IsKeyTrigger(DIK_N)) {
            nextTutorial = true;
        }

        const bool blockTutorialClick =
            battle_.HasPokerChoiceUi() ||
            battle_.IsViewingBoardFromPokerUi();

        if (input->IsMouseTrigger(0) && !blockTutorialClick) {
            nextTutorial = true;
        }

        if (nextTutorial) {
            using Step = TutorialManager::TutorialStep;
            Step step = tutorial_->GetStep();

            if (step == Step::Intro ||
                step == Step::ExplainEnergy ||
                step == Step::SkipPokerContinueTurn) {
                tutorial_->NextStep();
            } else if (step == Step::Finished) {
                if (state_ == State::Idle) {
                    nextSceneName_ = "Title";
                    state_ = State::ExitClose;
                }
                return;
            }
        }

        tutorial_->Update(battle_);
    }

    if (fieldUi_) {
        fieldUi_->Update(app, battle_);
    }

    if (tutorialUi_ && tutorial_ && fieldUi_) {
        tutorialUi_->Update(app, *tutorial_, battle_, *fieldUi_);
    }
}

void TutorialScene::Draw3D(GameApp& app) {
    app.Dx()->SetBackBuffer();
    app.Dx()->SetViewport(WinApp::kClientWidth, WinApp::kClientHeight);

    app.ObjCom()->SetGraphicsPipelineState();

    if (player_) {
        player_->Draw();
    }

    battle_.Draw3D(app);
    enemyMgr_.Draw();
}

void TutorialScene::Draw2D(GameApp& app) {
    app.SpriteCom()->SetGraphicsPipelineState();

    battle_.Draw2D(app);

    if (fieldUi_) {
        fieldUi_->Draw(app, battle_);
    }

    if (tutorialUi_ && tutorial_) {
        tutorialUi_->Draw(app, *tutorial_, battle_);
    }

    // 円形マスク描画
    app.SpriteCom()->DrawCircleMask(circle_, softness_);
}

void TutorialScene::DrawImGui(GameApp& app) {
#ifdef USE_IMGUI
    ImGui::Begin("Tutorial Debug");

    if (tutorial_) {
        ImGui::Text("Tutorial Active: %s", tutorial_->IsActive() ? "true" : "false");
        ImGui::Text("Step: %d", static_cast<int>(tutorial_->GetStep()));

        if (ImGui::Button("Next Step")) {
            tutorial_->NextStep();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            tutorial_->Reset();
        }
    }

    battle_.DrawImGui();

    ImGui::End();

    if (fieldUi_) {
        ImGui::Begin("FieldUi Debug");
        fieldUi_->DrawImGui();
        ImGui::End();
    }

    if (tutorialUi_ && tutorial_) {
        tutorialUi_->DrawImGui(*tutorial_);
    }

#else
    (void)app;
#endif
}

void TutorialScene::DrawSkydome(GameApp& app) {
    app.ObjCom()->SetGraphicsPipelineState();
    if (skyDome_) {
        skyDome_->Draw();
    }
}

void TutorialScene::DrawPostEffect3D(GameApp& app) {
    (void)app;
}

void TutorialScene::DrawPostEffect2D(GameApp& app) {
    (void)app;
}