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
        MakeTutorialCard(9, 7, CardSuit::Club),
        MakeTutorialCard(5, 13, CardSuit::Spade),
    };

    battle_.SetPlayer(player_.get());
    battle_.SetEnemyManager(&enemyMgr_);
    battle_.SetTutorialOpeningHand(openingHand);
    battle_.Initialize(app, animCamera_.get());
    if (Enemy* enemy = enemyMgr_.GetEnemy(0)) {
        enemy->SetMaxHp(141, true);
    }


    fieldUi_ = std::make_unique<FieldUi>();
    fieldUi_->Initialize(app);

    // プレイヤーHP数字
    playerHpText_ = std::make_unique<TextSprite>();
    playerHpText_->Initialize(app.SpriteCom(), app.Dx());
    playerHpText_->SetSize({ 1.0f, 1.0f, 1.0f });
    playerHpText_->SetPosition({ 140.0f, 12.5f });

    // 敵HP数字
    for (int i = 0; i < 3; i++) {
        auto text = std::make_unique<TextSprite>();
        text->Initialize(app.SpriteCom(), app.Dx());
        text->SetSize({ 1.0f, 1.0f, 1.0f });
        text->SetPosition({ 1000.0f, 40.0f + (i * 30.0f) });
        enemyHpTexts_.push_back(std::move(text));
    }

    // パワーブースト
    powerBoostBg_ = std::make_unique<Sprite>();
    powerBoostBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    powerBoostBg_->SetPosition({ 95.0f, 60.0f });
    powerBoostBg_->SetScale({ 32.0f, 32.0f, 1.0f });
    powerBoostBg_->SetColor({ 1.0f, 0.0f, 0.0f, 0.5f });

    powerBoostText_ = std::make_unique<TextSprite>();
    powerBoostText_->Initialize(app.SpriteCom(), app.Dx());
    powerBoostText_->SetSize({ 1.f, 1.f, 0.5f });
    powerBoostText_->SetPosition({ 88.f, 40.f });

    // ブロック
    blockBg_ = std::make_unique<Sprite>();
    blockBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    blockBg_->SetPosition({ 145.0f, 60.0f });
    blockBg_->SetScale({ 32.0f, 32.0f, 1.0f });
    blockBg_->SetColor({ 0.0f, 0.0f, 1.0f, 0.5f });

    blockText_ = std::make_unique<TextSprite>();
    blockText_->Initialize(app.SpriteCom(), app.Dx());
    blockText_->SetSize({ 1.f, 1.f, 0.5f });
    blockText_->SetPosition({ 138.f, 40.f });

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

    if (tutorial_) {
        battle_.SetTutorialPokerRestriction(
            tutorial_->IsForceActivateOnly(),
            tutorial_->IsForceDamageOnly()
        );
    }

    if (tutorial_ && fieldUi_) {
        if (tutorial_->IsForceDamageOnly()) {
            fieldUi_->SetForcedPokerHoverIndex(2); // damage
        } else {
            fieldUi_->SetForcedPokerHoverIndex(-1);
        }
    }

    bool lockGameplayInput = false;
    if (tutorial_) {
        lockGameplayInput = tutorial_->IsGameplayInputLocked();
    }

    battle_.SetTutorialInputLocked(lockGameplayInput);
    if (fieldUi_) {
        fieldUi_->SetTutorialInputLocked(lockGameplayInput);
    }

    battle_.Update(app, *fieldUi_, dt);

    if (tutorial_) {
        bool nextTutorial = false;

        if (input->IsKeyTrigger(DIK_N)) {
            nextTutorial = true;
        }

        bool imguiCapturingMouse = false;
#ifdef USE_IMGUI
        ImGuiIO& io = ImGui::GetIO();
        imguiCapturingMouse = io.WantCaptureMouse || ImGui::IsAnyItemActive();
#endif

        const bool blockTutorialClick =
            battle_.HasPokerChoiceUi() ||
            battle_.IsViewingBoardFromPokerUi() ||
            imguiCapturingMouse;

        if (input->IsMouseTrigger(0) && !blockTutorialClick) {
            nextTutorial = true;
        }

        if (nextTutorial) {
            using Step = TutorialManager::TutorialStep;
            Step step = tutorial_->GetStep();

            if (step == Step::Intro ||
                step == Step::ExplainEnergy ||
                step == Step::SkipPokerContinueTurn ||
                step == Step::EndAfterPoker ||
                step == Step::UiPlayerHp ||
                step == Step::UiEnemyHp ||
                step == Step::UiTurnText ||
                step == Step::UiHand ||
                step == Step::UiField ||
                step == Step::UiRoleText ||
                step == Step::UiEndTurn ||
                step == Step::UiDeckCount ||
                step == Step::UiEnemyIntentDamage ||
                step == Step::UiEnemyNextAction ||
                step == Step::UiFinished ||
                step == Step::ExplainCardAll) {
                tutorial_->NextStep();
            } else if (step == Step::Finished) {
                if (state_ == State::Idle) {
                    nextSceneName_ = "StageSelect";
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

    if (playerHpText_) {
        playerHpText_->SetText(battle_.GetPlayerHpTexts());
    }

    if (powerBoostText_) {
        powerBoostText_->SetText(battle_.GetPlayerPowerBoostText());
    }

    if (blockText_) {
        blockText_->SetText(battle_.GetPlayerBlockText());
    }

    std::vector<std::wstring> hpData = battle_.GetEnemyHpTexts();

    for (size_t i = 0; i < enemyHpTexts_.size(); i++) {
        if (i < hpData.size()) {
            enemyHpTexts_[i]->SetText(hpData[i]);
            enemyHpTexts_[i]->SetPosition({ 1025.0f, 10.0f + (i * 30.0f) });
        } else {
            enemyHpTexts_[i]->SetText(L"");
        }
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

    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0,
        float(WinApp::kClientWidth),
        float(WinApp::kClientHeight),
        0, 100
    );

    battle_.Draw2D(app);

    if (fieldUi_) {
        fieldUi_->Draw(app, battle_);
    }

    if (playerHpText_) {
        playerHpText_->Update(view, proj);
        playerHpText_->Draw();
    }

    if (powerBoostBg_) {
        powerBoostBg_->Update(view, proj);
        powerBoostBg_->Draw();
    }
    if (powerBoostText_) {
        powerBoostText_->Update(view, proj);
        powerBoostText_->Draw();
    }

    if (blockBg_) {
        blockBg_->Update(view, proj);
        blockBg_->Draw();
    }
    if (blockText_) {
        blockText_->Update(view, proj);
        blockText_->Draw();
    }

    for (auto& text : enemyHpTexts_) {
        if (text) {
            text->Update(view, proj);
            text->Draw();
        }
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