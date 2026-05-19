#include "TutorialScene.h"
#include "GameApp.h"
#include "Input.h"
#include "WinApp.h"
#include "TextureManager.h"
#include "AudioManager.h"
#include <algorithm>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

namespace {
    constexpr float kSceneStartFadeDuration = 0.75f;

    float SmoothStep01_(float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
}

void TutorialScene::OnEnter(GameApp& app) {
    AudioManager::GetInstance()->PlayBGM("BGM_Tutorial");

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
    battle_.Initialize(app, camera_.get());

    fieldParticleManager_ = std::make_unique<ModelParticleManager>();
    fieldParticleManager_->Initialize(app.Dx(), app.Srv(), 20000);
    fieldParticleManager_->RegisterEffect("card_glitter", "card_glitter.json");
    battle_.SetFieldParticleManager(fieldParticleManager_.get());
    ResetParticleObjectPostParam_();

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

    startFadeMask_ = std::make_unique<Sprite>();
    startFadeMask_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    startFadeMask_->SetAnchorPoint({ 0.0f, 0.0f });
    startFadeMask_->SetPosition({ 0.0f, 0.0f });
    const DirectX::TexMetadata& whiteMeta =
        TextureManager::GetInstance()->GetMetaData("resources/ui/white.png");
    startFadeMask_->SetScale({
        float(WinApp::kClientWidth) / float(whiteMeta.width),
        float(WinApp::kClientHeight) / float(whiteMeta.height),
        1.0f
        });
    startFadeMask_->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    startFadeDuration_ = kSceneStartFadeDuration;
    startFadeTimer_ = 0.0f;
    startFadeActive_ = true;

    // 円マスク開始設定
    state_ = State::EnterOpen;
    circle_ = 0.0f;
    softness_ = 0.6f;
    nextSceneName_ = "StageSelect";
    prevEsc_ = false;
}
void TutorialScene::OnExit(GameApp& app) {
    (void)app;
    tutorialUi_.reset();
    tutorial_.reset();
    fieldUi_.reset();
    startFadeMask_.reset();
    startFadeActive_ = false;
    fieldParticleManager_.reset();

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

    if (startFadeActive_) {
        startFadeTimer_ += dt;
        const float t = startFadeDuration_ > 0.0f ? startFadeTimer_ / startFadeDuration_ : 1.0f;
        const float alpha = 1.0f - SmoothStep01_(t);
        if (startFadeMask_) {
            startFadeMask_->SetColor({ 0.0f, 0.0f, 0.0f, alpha });
        }
        if (t >= 1.0f) {
            startFadeActive_ = false;
            if (startFadeMask_) {
                startFadeMask_->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
            }
        }
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
            nextSceneName_ = "StageSelect";
            state_ = State::ExitClose;
        }
        return;
    }
    prevEsc_ = currEsc;

    bool isTargeting = battle_.IsPlayerTargeting();

    if (isTargeting) {
        cameraBlend_ += dt * 5.0f;
        if (cameraBlend_ > 1.0f) cameraBlend_ = 1.0f;
    } else {
        cameraBlend_ -= dt * 5.0f;
        if (cameraBlend_ < 0.0f) cameraBlend_ = 0.0f;
    }

    if (cameraAnim_) {
        if (isTargeting) {
            cameraAnim_->Update(0.0f);
        } else {
            cameraAnim_->Update(dt);
        }
    }

    if (cameraBlend_ > 0.0f) {
        Vector3 animPos = animCamera_->GetTranslate();
        Vector3 animRot = animCamera_->GetRotate();

        Vector3 defaultPos = { 0.0f, 4.0f, -40.0f };
        Vector3 defaultRot = { 0.15f, 0.0f, 0.0f };

        float t = cameraBlend_;
        float easeT = t * t * (3.0f - 2.0f * t);

        Vector3 blendedPos = {
            animPos.x + (defaultPos.x - animPos.x) * easeT,
            animPos.y + (defaultPos.y - animPos.y) * easeT,
            animPos.z + (defaultPos.z - animPos.z) * easeT
        };
        Vector3 blendedRot = {
            animRot.x + (defaultRot.x - animRot.x) * easeT,
            animRot.y + (defaultRot.y - animRot.y) * easeT,
            animRot.z + (defaultRot.z - animRot.z) * easeT
        };

        animCamera_->SetTranslate(blendedPos);
        animCamera_->SetRotate(blendedRot);
    }
    if (camera_) {
        int windowW = WinApp::kClientWidth;
        int windowH = WinApp::kClientHeight;
        int fieldHeight = windowH - static_cast<int>(windowH * splitRatio_);
        camera_->SetAspect((float)windowW / fieldHeight);

        float origFovY = 0.45f;
        float zoomRatio = ((float)fieldHeight / windowH) / fieldCameraZoom_;
        float newFovY = 2.0f * std::atan(zoomRatio * std::tan(origFovY / 2.0f));
        camera_->SetFovY(newFovY);

        Vector3 rot = camera_->GetRotate();
        rot.x = 0.15f + fieldCameraRotXOffset_;
        camera_->SetRotate(rot);

        Matrix4x4 shiftField = Matrix4x4::MakeIdentity4x4();
        shiftField.m[1][1] = 1.0f - splitRatio_;
        shiftField.m[3][1] = -splitRatio_;
        camera_->SetProjectionShift(shiftField);

        camera_->Update();
    }

    if (animCamera_) {
        int windowW = WinApp::kClientWidth;
        int windowH = WinApp::kClientHeight;
        int battleHeight = static_cast<int>(windowH * splitRatio_);
        animCamera_->SetAspect((float)windowW / battleHeight);

        float origFovY = 0.45f;
        float zoomRatio = ((float)battleHeight / windowH) / battleCameraZoom_;
        float newFovY = 2.0f * std::atan(zoomRatio * std::tan(origFovY / 2.0f));
        animCamera_->SetFovY(newFovY);

        Vector3 rot = animCamera_->GetRotate();
        if (!cameraAnim_ || cameraAnim_->GetKeyframes().empty()) {
            rot.x = 0.15f;
        }
        rot.x += battleCameraRotXOffset_;
        animCamera_->SetRotate(rot);

        Matrix4x4 shiftBattle = Matrix4x4::MakeIdentity4x4();
        shiftBattle.m[1][1] = splitRatio_;
        shiftBattle.m[3][1] = 1.0f - splitRatio_;
        animCamera_->SetProjectionShift(shiftBattle);

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

    bool imguiCapturingMouse = false;
#ifdef USE_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    imguiCapturingMouse =
        io.WantCaptureMouse ||
        io.WantCaptureKeyboard ||
        ImGui::IsAnyItemActive() ||
        ImGui::IsAnyItemHovered();
#endif

    bool lockGameplayInput = false;
    if (tutorial_) {
        lockGameplayInput = tutorial_->IsGameplayInputLocked();
    }

    // ImGui操作中もゲーム側入力を止める
    lockGameplayInput = lockGameplayInput || imguiCapturingMouse;

    battle_.SetTutorialInputLocked(lockGameplayInput);
    if (fieldUi_) {
        fieldUi_->SetTutorialInputLocked(lockGameplayInput);
    }

    if (tutorial_) {
        bool nextTutorial = false;

        if (input->IsKeyTrigger(DIK_N)) {
            nextTutorial = true;
        }

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

    battle_.Update(app, *fieldUi_, dt);
    if (Camera* actionCamera = battle_.GetActionCamera()) {
        int windowW = WinApp::kClientWidth;
        int windowH = WinApp::kClientHeight;
        const bool isBattleAnimationPlaying = battle_.IsActionSequencePlaying();
        int battleHeight = isBattleAnimationPlaying ? windowH : static_cast<int>(windowH * splitRatio_);
        actionCamera->SetAspect((float)windowW / battleHeight);

        Matrix4x4 shiftBattle = Matrix4x4::MakeIdentity4x4();
        if (!isBattleAnimationPlaying) {
            float zoomRatio = ((float)battleHeight / windowH) / battleCameraZoom_;
            float correctedFovY = 2.0f * std::atan(zoomRatio * std::tan(actionCamera->GetFovY() / 2.0f));
            actionCamera->SetFovY(correctedFovY);
            shiftBattle.m[1][1] = splitRatio_;
            shiftBattle.m[3][1] = 1.0f - splitRatio_;
        }
        actionCamera->SetProjectionShift(shiftBattle);
        actionCamera->Update();
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

    if (fieldParticleManager_) {
        fieldParticleManager_->Dispatch(1.0f / 60.0f, camera_.get());
    }
}

void TutorialScene::Draw3D(GameApp& app) {
    app.Dx()->SetBackBuffer();

    int windowW = WinApp::kClientWidth;
    int windowH = WinApp::kClientHeight;
    const bool isBattleAnimationPlaying = battle_.IsActionSequencePlaying();
    int battleHeight = isBattleAnimationPlaying ? windowH : static_cast<int>(windowH * splitRatio_);

    app.Dx()->SetViewport(0, 0, windowW, windowH);
    app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);
    app.ObjCom()->SetGraphicsPipelineState();
    app.Dx()->ClearDepthBuffer();

    if (player_) {
        player_->Draw();
    }
    enemyMgr_.Draw();
    if (player_) {
        player_->DrawShieldBloom(app);
    }
    battle_.DrawDamagePopups3D(app);

    if (!isBattleAnimationPlaying) {
        app.Dx()->SetScissorRect(0, battleHeight, windowW, windowH);
        app.ObjCom()->SetGraphicsPipelineState();
        app.Dx()->ClearDepthBuffer();
        battle_.DrawField3D(app);

        app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);
        app.ObjCom()->SetGraphicsPipelineState();
        battle_.DrawBattleOverlay3D(app);

        app.Dx()->SetScissorRect(0, 0, windowW, windowH);
        app.ObjCom()->SetGraphicsPipelineState();
        app.Dx()->ClearDepthBuffer();
        battle_.DrawCardArea3D(app);

    battle_.DrawPostEffect3D(app);
    battle_.DrawFieldFrameBloom(app);

    app.Dx()->SetScissorRect(0, 0, windowW, windowH);
    app.Dx()->ClearDepthBuffer();
    if (fieldParticleManager_) {
        if (particleObjectPostEnabled_) {
            app.DrawModelParticlesObjectPost(fieldParticleManager_.get(), particleObjectPostParam_);
        } else {
            fieldParticleManager_->Draw();
            app.ObjCom()->SetGraphicsPipelineState();
        }
    }
}

    app.Dx()->SetScissorRect(0, 0, windowW, windowH);
    app.ObjCom()->SetGraphicsPipelineState();
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

    if (battle_.IsActionSequencePlaying()) {
        battle_.Draw2D(app);
        app.SpriteCom()->DrawCircleMask(circle_, softness_);
        if (startFadeActive_ && startFadeMask_) {
            app.SpriteCom()->SetGraphicsPipelineState();
            startFadeMask_->Update(view, proj);
            startFadeMask_->Draw();
        }
        return;
    }

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
        if (tutorial_->GetStep() == TutorialManager::TutorialStep::ExplainCardAll) {
            // 1. 暗転オーバーレイを描画 (2D)
            app.SpriteCom()->SetGraphicsPipelineState();
            tutorialUi_->DrawDimOverlay(app);

            // 2. 深度バッファをクリアして、3Dのプレビューカードを手前に再描画
            app.Dx()->ClearDepthBuffer();
            battle_.DrawPreviewCard3D(app);

            // 3. その上にチュートリアルの文字や矢印を描画 (2D)
            app.SpriteCom()->SetGraphicsPipelineState();
            tutorialUi_->Draw(app, *tutorial_, battle_);
        } else {
            // 通常の描画
            tutorialUi_->Draw(app, *tutorial_, battle_);
        }
    }

    // 円形マスク描画
    app.SpriteCom()->DrawCircleMask(circle_, softness_);

    if (startFadeActive_ && startFadeMask_) {
        app.SpriteCom()->SetGraphicsPipelineState();
        startFadeMask_->Update(view, proj);
        startFadeMask_->Draw();
    }
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
    if (player_) {
        player_->DrawShieldImGui();
    }

    ImGui::End();

    if (fieldUi_) {
        ImGui::Begin("FieldUi Debug");
        fieldUi_->DrawImGui();
        ImGui::End();
    }

    ImGui::Begin("Camera Setup (Tutorial)");
    ImGui::SliderFloat("Split Ratio", &splitRatio_, 0.1f, 0.9f);
    ImGui::SliderFloat("Field Camera Zoom", &fieldCameraZoom_, 0.1f, 3.0f);
    ImGui::SliderFloat("Field Camera RotX Offset", &fieldCameraRotXOffset_, -0.5f, 0.5f);
    ImGui::SliderFloat("Battle Camera Zoom", &battleCameraZoom_, 0.1f, 3.0f);
    ImGui::SliderFloat("Battle Camera RotX Offset", &battleCameraRotXOffset_, -0.5f, 0.5f);
    ImGui::End();

    if (tutorialUi_ && tutorial_) {
        tutorialUi_->DrawImGui(*tutorial_);
    }

#else
    (void)app;
#endif
}

void TutorialScene::DrawSkydome(GameApp& app) {
    int windowW = WinApp::kClientWidth;
    int windowH = WinApp::kClientHeight;
    int battleHeight = battle_.IsActionSequencePlaying() ? windowH : static_cast<int>(windowH * splitRatio_);
    app.Dx()->SetViewport(0, 0, windowW, windowH);
    app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);

    app.ObjCom()->SetGraphicsPipelineState();
    if (skyDome_) {
        skyDome_->Draw();
    }
    app.Dx()->SetScissorRect(0, 0, windowW, windowH);
}

void TutorialScene::DrawPostEffect3D(GameApp& app) {
    (void)app;
}

void TutorialScene::DrawPostEffect2D(GameApp& app) {
    (void)app;
}

void TutorialScene::ResetParticleObjectPostParam_()
{
    particleObjectPostParam_ = {};
    particleObjectPostParam_.threshold = 0.0f;
    particleObjectPostParam_.intensity = 1.7f;
    particleObjectPostParam_.vignetteIntensity = 0.0f;
    particleObjectPostParam_.vignetteScale = 0.0f;
    particleObjectPostParam_.distortionAmount = 0.0f;
    particleObjectPostParam_.chromAbAmount = 0.003f;
    particleObjectPostParam_.isGrayscale = 0.0f;
    particleObjectPostParam_.isInverted = 0.0f;
    particleObjectPostParam_.noiseIntensity = 0.0f;
    particleObjectPostParam_.scanlineIntensity = 0.0f;
    particleObjectPostParam_.scanlineFrequency = 100.0f;
    particleObjectPostParam_.curvature = 0.0f;
    particleObjectPostParam_.borderSharp = 0.0f;
    particleObjectPostParam_.glitchAmount = 0.0f;
    particleObjectPostParam_.dissolveAmount = -1.0f;
    particleObjectPostParam_.dissolveEdgeWidth = 0.08f;
    particleObjectPostParam_.dissolveEdgeIntensity = 2.0f;
    particleObjectPostParam_.dissolveNoiseScale = 36.0f;
    particleObjectPostParam_.dissolveEdgeColor = { 0.15f, 0.8f, 1.0f, 1.0f };
}
