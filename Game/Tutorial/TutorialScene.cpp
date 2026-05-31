#include "TutorialScene.h"
#include "GameApp.h"
#include "Input.h"
#include "WinApp.h"
#include "TextureManager.h"
#include "AudioManager.h"
#include <algorithm>
#include <cstring>
#include "AnimationJsonSerializer.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

namespace {
    constexpr float kSceneStartFadeDuration = 0.75f;
    constexpr const char* kTutorialFieldConfigPath = "resources/configs/stage_fields/tutorial_field.json";
    constexpr int kMenuActionInactive = -2;
    constexpr int kMenuActionTitle = -3;
    constexpr int kMenuActionRoot = -4;
    constexpr Vector2 kDefenseUiTextureSize{ 64.0f, 64.0f };
    constexpr Vector2 kPowerupUiTextureSize{ 48.0f, 48.0f };
    constexpr Vector2 kPlayerHpTextPosition{ 172.0f, 14.0f };
    constexpr int kPlayerHpTextFontSize = 28;
    constexpr float kPlayerHpOutlineThickness = 2.0f;
    constexpr Vector2 kPowerupUiPosition{ 498.0f, 10.0f };
    constexpr Vector2 kPowerupUiSize{ 48.0f, 48.0f };
    constexpr Vector2 kPowerBoostTextPosition{ 510.0f, 18.0f };
    constexpr Vector2 kDefenseUiPosition{ 426.0f, 2.0f };
    constexpr Vector2 kDefenseUiSize{ 64.0f, 64.0f };
    constexpr Vector2 kBlockTextPosition{ 443.0f, 18.0f };
    constexpr int kBlockTextFontSize = 28;
    constexpr float kBlockOutlineThickness = 2.0f;
    constexpr TutorialManager::TutorialStep kEditableTutorialSteps[] = {
        TutorialManager::TutorialStep::Intro,
        TutorialManager::TutorialStep::UiPlayerHp,
        TutorialManager::TutorialStep::UiPlayerBlock,
        TutorialManager::TutorialStep::UiPlayerPowerBoost,
        TutorialManager::TutorialStep::UiEnemyIntentDamage,
        TutorialManager::TutorialStep::UiEnemyHp,
        TutorialManager::TutorialStep::UiEnemyNextAction,
        TutorialManager::TutorialStep::UiTurnText,
        TutorialManager::TutorialStep::UiHand,
        TutorialManager::TutorialStep::UiField,
        TutorialManager::TutorialStep::UiRoleText,
        TutorialManager::TutorialStep::UiEndTurn,
        TutorialManager::TutorialStep::UiDeckCount,
        TutorialManager::TutorialStep::UiPokerHandHelp,
        TutorialManager::TutorialStep::UiFinished,
        TutorialManager::TutorialStep::HoverHand,
        TutorialManager::TutorialStep::ExplainCardCost,
        TutorialManager::TutorialStep::ExplainCardSuit,
        TutorialManager::TutorialStep::ExplainCardNumber,
        TutorialManager::TutorialStep::ExplainCardAll,
        TutorialManager::TutorialStep::PlayCard,
        TutorialManager::TutorialStep::ChooseEnemyTarget,
        TutorialManager::TutorialStep::ExplainEnergy,
        TutorialManager::TutorialStep::FillField,
        TutorialManager::TutorialStep::EndPlayerTurn,
        TutorialManager::TutorialStep::WaitEnemyTurn,
        TutorialManager::TutorialStep::ExplainPokerReady,
        TutorialManager::TutorialStep::ChoosePokerEffect,
        TutorialManager::TutorialStep::SkipPokerContinueTurn,
        TutorialManager::TutorialStep::SkipPokerEndTurn,
        TutorialManager::TutorialStep::SkipPokerWaitEnemyTurn,
        TutorialManager::TutorialStep::ViewingBoardFromPoker,
        TutorialManager::TutorialStep::EndAfterPoker,
        TutorialManager::TutorialStep::Finished
    };
    constexpr std::array<Vector2, 8> kOutlineDirections{
        Vector2{ -1.0f, 0.0f },
        Vector2{ 1.0f, 0.0f },
        Vector2{ 0.0f, -1.0f },
        Vector2{ 0.0f, 1.0f },
        Vector2{ -1.0f, -1.0f },
        Vector2{ 1.0f, -1.0f },
        Vector2{ -1.0f, 1.0f },
        Vector2{ 1.0f, 1.0f },
    };

    float SmoothStep01_(float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
}

void TutorialScene::InitializeTutorialMenu_(GameApp& app)
{
    tutorialMenuButtons_.clear();

    auto addButton = [&](float x, float y, float w, float h, const std::wstring& label,
        TutorialManager::TutorialChapter chapter, int nextPage) {
            TutorialMenuButton button{};
            button.rect = { x, y, w, h };
            button.label = label;
            button.chapter = chapter;
            button.nextPage = nextPage;
            button.bg = std::make_unique<Sprite>();
            button.bg->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
            button.bg->SetAnchorPoint({ 0.0f, 0.0f });
            button.text = std::make_unique<TextSprite>();
            button.text->Initialize(app.SpriteCom(), app.Dx());
            button.text->SetText(label);
            button.text->SetFontSize(28);
            button.text->SetSize({ 1.0f, 1.0f, 1.0f });
            tutorialMenuButtons_.push_back(std::move(button));
        };

    if (tutorialMenuPage_ == 0) {
        addButton(40.0f, 40.0f, 160.0f, 50.0f, L"\x623b\x308b", TutorialManager::TutorialChapter::Full, kMenuActionTitle);
        addButton(240.0f, 350.0f, 410.0f, 70.0f, L"チュートリアル", TutorialManager::TutorialChapter::Full, -1);
        addButton(300.0f, 470.0f, 410.0f, 70.0f, L"フィールド編", TutorialManager::TutorialChapter::Full, 1);
        addButton(300.0f, 620.0f, 410.0f, 70.0f, L"バトル編", TutorialManager::TutorialChapter::Full, 2);
    } else if (tutorialMenuPage_ == 1) {
        addButton(40.0f, 40.0f, 160.0f, 50.0f, L"\x623b\x308b", TutorialManager::TutorialChapter::Full, kMenuActionRoot);
        addButton(250.0f, 355.0f, 410.0f, 70.0f, L"フィールド編", TutorialManager::TutorialChapter::Full, -2);
        addButton(315.0f, 485.0f, 410.0f, 70.0f, L"画面のUI説明", TutorialManager::TutorialChapter::FieldUi, -1);
        addButton(315.0f, 595.0f, 410.0f, 70.0f, L"カードの説明", TutorialManager::TutorialChapter::Card, -1);
    } else {
        addButton(40.0f, 40.0f, 160.0f, 50.0f, L"\x623b\x308b", TutorialManager::TutorialChapter::Full, kMenuActionRoot);
        addButton(240.0f, 352.0f, 410.0f, 70.0f, L"バトル編", TutorialManager::TutorialChapter::Full, -2);
        addButton(325.0f, 610.0f, 410.0f, 70.0f, L"特殊効果説明編", TutorialManager::TutorialChapter::SpecialEffect, -1);
        addButton(325.0f, 720.0f, 410.0f, 70.0f, L"実践編", TutorialManager::TutorialChapter::Practice, -1);
    }
}

void TutorialScene::ShowTutorialMenu_(int page)
{
    tutorialMenuPage_ = page;
    tutorialMenuVisible_ = true;
}

void TutorialScene::ReturnToTitle_()
{
    nextSceneName_ = "Select";
    state_ = State::ExitClose;
}

void TutorialScene::ReturnToTutorialMenu_(GameApp& app)
{
    ShowTutorialMenu_(0);
    InitializeTutorialMenu_(app);
}

void TutorialScene::StartTutorialChapter_(GameApp& app, TutorialManager::TutorialChapter chapter)
{
    if (!tutorial_) {
        return;
    }

    InitializeTutorialContent_(app);
    if (tutorial_->GetEditableStepMessage(TutorialManager::TutorialStep::PlayCard).empty()) {
    tutorial_->SetStepMessage(
        TutorialManager::TutorialStep::PlayCard,
        L"手札の Attack! を使ってみましょう\nカードを上に出してから\n左クリックで使用します"
    );
    }
    if (tutorial_->GetEditableStepMessage(TutorialManager::TutorialStep::ChooseEnemyTarget).empty()) {
    tutorial_->SetStepMessage(
        TutorialManager::TutorialStep::ChooseEnemyTarget,
        L"敵を選択してください"
    );
    }
    if (chapter == TutorialManager::TutorialChapter::Full) {
        tutorial_->Reset();
    } else {
        tutorial_->StartChapter(chapter);
    }
    lastTutorialStep_ = tutorial_->GetStep();
    cardExplainInputBlockTimer_ = lastTutorialStep_ == TutorialManager::TutorialStep::ExplainCardAll ? 1.0f : 0.0f;
    tutorialMenuVisible_ = false;
}

void TutorialScene::UpdateTutorialMenu_(GameApp& app)
{
    if (!tutorialMenuVisible_) {
        return;
    }

    Input* input = app.GetInput();
    if (!input) {
        return;
    }

    const POINT mouse = input->GetMousePosition();
    const float mx = static_cast<float>(mouse.x);
    const float my = static_cast<float>(mouse.y);
    const bool click = input->IsMouseTrigger(0);

    for (auto& button : tutorialMenuButtons_) {
        button.hovered =
            mx >= button.rect.x && mx <= button.rect.x + button.rect.w &&
            my >= button.rect.y && my <= button.rect.y + button.rect.h;

        if (button.hovered && click && button.nextPage != kMenuActionInactive) {
            if (button.nextPage >= 0) {
                ShowTutorialMenu_(button.nextPage);
                InitializeTutorialMenu_(app);
            } else if (button.nextPage == kMenuActionTitle) {
                ReturnToTitle_();
            } else if (button.nextPage == kMenuActionRoot) {
                ReturnToTutorialMenu_(app);
            } else {
                StartTutorialChapter_(app, button.chapter);
            }
            return;
        }
    }
}

void TutorialScene::DrawTutorialMenu_(GameApp& app)
{
    if (!tutorialMenuVisible_) {
        return;
    }

    app.SpriteCom()->SetGraphicsPipelineState();
    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0,
        float(WinApp::kClientWidth),
        float(WinApp::kClientHeight),
        0, 100
    );

    if (tutorialMenuBg_) {
        tutorialMenuBg_->SetPosition({ 0.0f, 0.0f });
        tutorialMenuBg_->SetScale({ float(WinApp::kClientWidth), float(WinApp::kClientHeight), 1.0f });
        tutorialMenuBg_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        tutorialMenuBg_->Update(view, proj);
        tutorialMenuBg_->Draw();
    }

    for (auto& button : tutorialMenuButtons_) {
        if (button.bg) {
            button.bg->SetColor(button.nextPage == -2
                ? Vector4{ 0.92f, 0.92f, 0.92f, 0.95f }
                : button.hovered
                ? Vector4{ 0.85f, 0.92f, 1.0f, 0.96f }
                : Vector4{ 0.92f, 0.92f, 0.92f, 0.95f });
            button.bg->Update(view, proj);
            button.bg->Draw();
        }
        if (button.text) {
            button.text->SetPosition({ button.rect.x + button.rect.w * 0.34f, button.rect.y + 20.0f });
            button.text->SetColor({ 0.0f, 0.0f, 0.0f });
            button.text->Update(view, proj);
            button.text->Draw();
        }
    }
}

void TutorialScene::InitializeTutorialContent_(GameApp& app)
{
    if (tutorialContentInitialized_) {
        return;
    }

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

    tutorialFieldProps_ = std::make_unique<PropManager>();
    tutorialFieldProps_->Initialize(app.ObjCom(), app.Dx(), animCamera_.get());
    tutorialFieldProps_->LoadFromJson(kTutorialFieldConfigPath);

    const float charZ = 15.0f;

    player_ = std::make_unique<Player>();
    player_->Initialize(app.ObjCom(), app.Dx(), animCamera_.get());
    player_->SetSpawnPos({ -7.0f, 0.0f, charZ });
    player_->SetRotation({ 0.0f, 1.5708f, 0.0f });

#ifndef _DEBUG
	if (player_ && player_->GetObject3d() && player_->GetObject3d()->GetModel()) {
		struct CustomAnimationFile {
			const char* path;
			const char* name;
		};

		static const CustomAnimationFile kCustomAnimationFiles[] = {
			{ "resources/CustomAnim/CustomAnim.json", "CustomAnim" },
			{ "resources/CustomAnim/CustomAnim_attack_1.json", "CustomAnim_attack_1" },
			{ "resources/CustomAnim/CustomAnim_attack_2.json", "CustomAnim_attack_2" },
			{ "resources/CustomAnim/CustomAnim_attack_3.json", "CustomAnim_attack_3" },
			{ "resources/CustomAnim/CustomAnim_attack_received_1.json", "CustomAnim_attack_received_1" },
			{ "resources/CustomAnim/CustomAnim_attack_received_2.json", "CustomAnim_attack_received_2" },
		};

		bool loadedDefaultCustomAnim = false;
		for (const auto& customAnimationFile : kCustomAnimationFiles) {
			Animation animation{};
			if (!AnimationJsonSerializer::LoadFromJson(customAnimationFile.path, animation)) {
				continue;
			}

			player_->GetObject3d()->GetModel()->AddAnimation(customAnimationFile.name, animation);
			if (std::string(customAnimationFile.name) == "CustomAnim") {
				loadedDefaultCustomAnim = true;
			}
		}

		if (loadedDefaultCustomAnim) {
			player_->GetObject3d()->PlayAnimation("CustomAnim", true);
		}
	}
#endif

    particleManager_ = ModelParticleManager::GetInstance();
    particleManager_->ClearParticles();

    trailManager_ = std::make_unique<TrailManager>();
    trailManager_->Initialize(app.Dx(), app.ObjCom(), "resources/gradation.png");
    testTrail_ = trailManager_->CreateInstance();
    testTrail_->SetIsPermanent(true);
    player_->SetTrailInstance(testTrail_);

    TrailConfig config;
    trailConfig_.maxPoints = 200;
    trailConfig_.interpolationSteps = 8;
    trailConfig_.startColor = { 1, 1, 1, 1 };
    trailConfig_.endColor = { 1, 0, 0, 0.2f };
    player_->SetTrailConfig(config);

    particleManager_->RegisterEffect("sword_trail", "sword_particle.json");
    particleManager_->RegisterEffect("player_fire", "fire_particle.json");
    particleManager_->RegisterEffect("fireExplosive", "fireExplosive.json");
    particleManager_->RegisterEffect("particle_image", "0.json");
	
    particleManager_->RegisterEffect("Vacuum_Fly", "Vacuum_Fly.json");
    particleManager_->RegisterEffect("Vacuum_Hit", "Vacuum_Hit.json");
	
    particleManager_->RegisterEffect("Flare_Fly", "Flare_Fly.json");
    particleManager_->RegisterEffect("Flare_Hit", "Flare_Hit.json");
	
    particleManager_->RegisterEffect("Air_Fly", "Air_Fly.json");
    particleManager_->RegisterEffect("Air_Hit", "Air_Hit.json");

    effectSequencer_ = std::make_unique<EffectSequencer>();
    effectSequencer_->Initialize(
        app.ObjCom(), app.Dx(), animCamera_.get(),
        particleManager_, trailManager_.get()
    );

    if (player_) {
        player_->GetEffectSequencer().Initialize(
            app.ObjCom(), app.Dx(), animCamera_.get(),
            particleManager_, trailManager_.get()
        );

        player_->AddAttackMove({ "CustomAnim_attack_1", "attack_1.json", 0.1f });
        player_->AddAttackMove({ "CustomAnim_attack_2", "attack_2.json", 0.15f });
        player_->AddAttackMove({ "CustomAnim_attack_3", "attack_3.json", 0.2f });
    }

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
        MakeTutorialCard(13, 13, CardSuit::Spade),
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
        enemy->SetMaxHp(100, true);
    }

    fieldUi_ = std::make_unique<FieldUi>();
    fieldUi_->Initialize(app);

    playerHpText_ = std::make_unique<TextSprite>();
    playerHpText_->Initialize(app.SpriteCom(), app.Dx());
    playerHpText_->SetFontSize(kPlayerHpTextFontSize);
    playerHpText_->SetSize({ 1.0f, 1.0f, 1.0f });
    playerHpText_->SetPosition(kPlayerHpTextPosition);
    for (auto& outlineText : playerHpOutlineTexts_) {
        outlineText = std::make_unique<TextSprite>();
        outlineText->Initialize(app.SpriteCom(), app.Dx());
        outlineText->SetFontSize(kPlayerHpTextFontSize);
        outlineText->SetSize({ 1.0f, 1.0f, 1.0f });
    }

    for (int i = 0; i < 3; i++) {
        auto text = std::make_unique<TextSprite>();
        text->Initialize(app.SpriteCom(), app.Dx());
        text->SetSize({ 1.0f, 1.0f, 1.0f });
        text->SetPosition({ 1000.0f, 40.0f + (i * 30.0f) });
        enemyHpTexts_.push_back(std::move(text));
    }

    powerBoostBg_ = std::make_unique<Sprite>();
    powerBoostBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/gauge/Powerup_UI.png");
    powerBoostBg_->SetPosition(kPowerupUiPosition);
    powerBoostBg_->SetScale({
        kPowerupUiSize.x / kPowerupUiTextureSize.x,
        kPowerupUiSize.y / kPowerupUiTextureSize.y,
        1.0f
        });
    powerBoostBg_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    powerBoostText_ = std::make_unique<TextSprite>();
    powerBoostText_->Initialize(app.SpriteCom(), app.Dx());
    powerBoostText_->SetSize({ 1.f, 1.f, 0.5f });
    powerBoostText_->SetPosition(kPowerBoostTextPosition);

    blockBg_ = std::make_unique<Sprite>();
    blockBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/gauge/Defense_UI.png");
    blockBg_->SetPosition(kDefenseUiPosition);
    blockBg_->SetScale({
        kDefenseUiSize.x / kDefenseUiTextureSize.x,
        kDefenseUiSize.y / kDefenseUiTextureSize.y,
        1.0f
        });
    blockBg_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    blockText_ = std::make_unique<TextSprite>();
    blockText_->Initialize(app.SpriteCom(), app.Dx());
    blockText_->SetFontSize(kBlockTextFontSize);
    blockText_->SetSize({ 1.f, 1.f, 0.5f });
    blockText_->SetPosition(kBlockTextPosition);
    for (auto& outlineText : blockOutlineTexts_) {
        outlineText = std::make_unique<TextSprite>();
        outlineText->Initialize(app.SpriteCom(), app.Dx());
        outlineText->SetFontSize(kBlockTextFontSize);
        outlineText->SetSize({ 1.f, 1.f, 0.5f });
    }

    highlightFilter_ = std::make_unique<Sprite>();
    highlightFilter_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    highlightFilter_->SetPosition({ 0.0f, 0.0f });
    highlightFilter_->SetScale({ 1280.0f, 1280.0f, 1.0f });
    highlightFilter_->SetColor({ 0.0f, 0.0f, 0.0f, 0.8f });

    tutorialUi_ = std::make_unique<TutorialUi>();
    tutorialUi_->Initialize(app);

    pausingUI_ = std::make_unique<PausingUI>();
    pausingUI_->Initialize(app);
    pausingUI_->SetTutorialExitMode(true);

    pokerHandHelpView_ = std::make_unique<PokerHandHelpView>();
    pokerHandHelpView_->Initialize(app.SpriteCom(), app.Dx());

    tutorialContentInitialized_ = true;
}

void TutorialScene::OnEnter(GameApp& app) {
    AudioManager::GetInstance()->PlayBGM("BGM_Tutorial");

    tutorialContentInitialized_ = false;
    tutorial_ = std::make_unique<TutorialManager>();
    tutorial_->Initialize();

    tutorialMenuBg_ = std::make_unique<Sprite>();
    tutorialMenuBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    tutorialMenuBg_->SetAnchorPoint({ 0.0f, 0.0f });
    tutorialMenuPage_ = 0;
    tutorialMenuVisible_ = false;

    startFadeMask_ = std::make_unique<Sprite>();
    startFadeMask_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    startFadeMask_->SetAnchorPoint({ 0.0f, 0.0f });
    startFadeMask_->SetPosition({ 0.0f, 0.0f });
    startFadeMask_->SetScale({ float(WinApp::kClientWidth), float(WinApp::kClientHeight), 1.0f });
    startFadeMask_->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    startFadeDuration_ = kSceneStartFadeDuration;
    startFadeTimer_ = 0.0f;
    startFadeActive_ = true;

    state_ = State::EnterOpen;
    circle_ = 0.0f;
    softness_ = 0.6f;
    nextSceneName_ = "Select";
    prevEsc_ = false;
    lastTutorialStep_ = TutorialManager::TutorialStep::Intro;
    cardExplainInputBlockTimer_ = 0.0f;
    StartTutorialChapter_(app, TutorialManager::TutorialChapter::Full);
}
void TutorialScene::OnExit(GameApp& app) {
    (void)app;
    tutorialUi_.reset();
    pausingUI_.reset();
    tutorial_.reset();
    tutorialMenuBg_.reset();
    tutorialMenuButtons_.clear();
    pokerHandHelpView_.reset();
    fieldUi_.reset();
    startFadeMask_.reset();
    highlightFilter_.reset();
    for (auto& outlineText : playerHpOutlineTexts_) {
        outlineText.reset();
    }
    for (auto& outlineText : blockOutlineTexts_) {
        outlineText.reset();
    }
    startFadeActive_ = false;
    if (particleManager_) {
        particleManager_->ClearParticles();
        particleManager_ = nullptr;
    }
    fieldParticleManager_.reset();

    player_.reset();
    skyDome_.reset();
    tutorialFieldProps_.reset();
    cameraAnim_.reset();
    animCamera_.reset();
    camera_.reset();

    if (tutorialContentInitialized_) {
        battle_.Finalize();
    }
    tutorialContentInitialized_ = false;
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

    if (tutorialMenuVisible_) {
        battle_.SetTutorialInputLocked(true);
        if (fieldUi_) {
            fieldUi_->SetTutorialInputLocked(true);
        }
        UpdateTutorialMenu_(app);
        return;
    }

    if (pausingUI_) {
        pausingUI_->Update(app, input);
        if (pausingUI_->GetIsSceneChangeRequested()) {
            ReturnToTitle_();
            return;
        }
        if (pausingUI_->GetIsPaused()) {
            battle_.SetTutorialInputLocked(true);
            if (fieldUi_) {
                fieldUi_->SetTutorialInputLocked(true);
            }
            return;
        }
    }

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

    if (tutorialFieldProps_) {
        Camera* propCamera = animCamera_.get();
        if (battle_.IsActionSequencePlaying()) {
            if (Camera* actionCamera = battle_.GetActionCamera()) {
                propCamera = actionCamera;
            }
        }
        tutorialFieldProps_->SetCamera(propCamera);
        tutorialFieldProps_->Update(dt);
    }

    enemyMgr_.UpdateCamera(animCamera_.get());
    enemyMgr_.Update(dt);

    if (tutorial_) {
        battle_.SetTutorialPokerRestriction(
            tutorial_->IsForceActivateOnly(),
            tutorial_->IsForceDamageOnly()
        );
        const TutorialManager::TutorialStep step = tutorial_->GetStep();
        if (step != lastTutorialStep_) {
            lastTutorialStep_ = step;
            cardExplainInputBlockTimer_ =
                step == TutorialManager::TutorialStep::ExplainCardAll ? 1.0f : 0.0f;
        } else if (cardExplainInputBlockTimer_ > 0.0f) {
            cardExplainInputBlockTimer_ = std::max(0.0f, cardExplainInputBlockTimer_ - dt);
        }

        const bool forceEnemyTargetCard =
            step == TutorialManager::TutorialStep::HoverHand ||
            step == TutorialManager::TutorialStep::ExplainCardCost ||
            step == TutorialManager::TutorialStep::ExplainCardSuit ||
            step == TutorialManager::TutorialStep::ExplainCardNumber ||
            step == TutorialManager::TutorialStep::ExplainCardAll ||
            step == TutorialManager::TutorialStep::PlayCard;
        battle_.SetTutorialForcedEnemyTargetCardId(forceEnemyTargetCard ? 1 : -1);
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
    lockGameplayInput = lockGameplayInput || input->IsKeyPressed(DIK_TAB);

    battle_.SetTutorialInputLocked(lockGameplayInput);
    if (fieldUi_) {
        fieldUi_->SetTutorialInputLocked(lockGameplayInput);
    }

    if (tutorial_) {
        bool nextTutorial = false;

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
            if (step == Step::ExplainCardAll && cardExplainInputBlockTimer_ > 0.0f) {
                nextTutorial = false;
            }

            if (nextTutorial && (step == Step::Intro ||
                step == Step::ExplainEnergy ||
                step == Step::SkipPokerContinueTurn ||
                step == Step::EndAfterPoker ||
                step == Step::UiPlayerHp ||
                step == Step::UiPlayerBlock ||
                step == Step::UiPlayerPowerBoost ||
                step == Step::UiEnemyHp ||
                step == Step::UiTurnText ||
                step == Step::UiHand ||
                step == Step::UiField ||
                step == Step::UiRoleText ||
                step == Step::UiEndTurn ||
                step == Step::UiDeckCount ||
                step == Step::UiPokerHandHelp ||
                step == Step::UiEnemyIntentDamage ||
                step == Step::UiEnemyNextAction ||
                step == Step::UiFinished ||
                step == Step::ExplainCardAll)) {
                tutorial_->NextStep();
            } else if (nextTutorial && step == Step::Finished) {
                nextSceneName_ = "Select";
                state_ = State::ExitClose;
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

    if (pokerHandHelpView_) {
        pokerHandHelpView_->Update(app.GetInput());
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
    if (particleManager_) {
        particleManager_->Dispatch(1.0f / 60.0f, animCamera_.get());
    }
}

void TutorialScene::Draw3D(GameApp& app) {
    app.Dx()->SetBackBuffer();

    int windowW = WinApp::kClientWidth;
    int windowH = WinApp::kClientHeight;
    if (tutorialMenuVisible_) {
        app.Dx()->SetViewport(0, 0, windowW, windowH);
        app.Dx()->SetScissorRect(0, 0, windowW, windowH);
        app.Dx()->ClearDepthBuffer();
        return;
    }

    const bool isBattleAnimationPlaying = battle_.IsActionSequencePlaying();
    int battleHeight = isBattleAnimationPlaying ? windowH : static_cast<int>(windowH * splitRatio_);

    app.Dx()->SetViewport(0, 0, windowW, windowH);
    app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);
    app.ObjCom()->SetGraphicsPipelineState();
    app.Dx()->ClearDepthBuffer();

    if (tutorialFieldProps_) {
        tutorialFieldProps_->Draw3D();
    }
    if (player_) {
        player_->Draw();
    }
    if (isBattleAnimationPlaying) {
        if (Enemy* actionTarget = battle_.GetActionTarget()) {
            actionTarget->Draw();
        }
    } else {
        enemyMgr_.Draw();
    }
    if (particleManager_) {
        particleManager_->Draw();
        app.ObjCom()->SetGraphicsPipelineState();
    }
    enemyMgr_.DrawShieldBloom(app);
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

    auto drawMasks = [&]() {
        app.SpriteCom()->DrawCircleMask(circle_, softness_);
        if (startFadeActive_ && startFadeMask_) {
            app.SpriteCom()->SetGraphicsPipelineState();
            startFadeMask_->Update(view, proj);
            startFadeMask_->Draw();
        }
    };

    if (tutorialMenuVisible_) {
        DrawTutorialMenu_(app);
        drawMasks();
        return;
    }

    if (pausingUI_) {
        pausingUI_->Draw(app);
    }

    if (pausingUI_ && pausingUI_->GetIsPaused()) {
        drawMasks();
        return;
    }

    if (battle_.IsActionSequencePlaying()) {
        battle_.Draw2D(app, startFadeActive_);
        drawMasks();
        return;
    }

    battle_.Draw2D(app, startFadeActive_);

    if (fieldUi_) {
        fieldUi_->Draw(app, battle_);
    }

    if (playerHpText_) {
        playerHpText_->SetPosition(kPlayerHpTextPosition);
        playerHpText_->SetColor({ 1.0f, 1.0f, 1.0f });
        playerHpText_->SetAlpha(1.0f);
        for (size_t i = 0; i < playerHpOutlineTexts_.size(); ++i) {
            auto& outlineText = playerHpOutlineTexts_[i];
            if (!outlineText) {
                continue;
            }
            outlineText->SetText(battle_.GetPlayerHpTexts());
            outlineText->SetPosition({
                kPlayerHpTextPosition.x + kOutlineDirections[i].x * kPlayerHpOutlineThickness,
                kPlayerHpTextPosition.y + kOutlineDirections[i].y * kPlayerHpOutlineThickness
                });
            outlineText->SetColor({ 0.0f, 0.0f, 0.0f });
            outlineText->SetAlpha(1.0f);
            outlineText->Update(view, proj);
            outlineText->Draw();
        }
        playerHpText_->Update(view, proj);
        playerHpText_->Draw();
    }

    if (powerBoostBg_) {
        powerBoostBg_->SetPosition(kPowerupUiPosition);
        powerBoostBg_->SetScale({
            kPowerupUiSize.x / kPowerupUiTextureSize.x,
            kPowerupUiSize.y / kPowerupUiTextureSize.y,
            1.0f
            });
        powerBoostBg_->Update(view, proj);
        powerBoostBg_->Draw();
    }
    if (powerBoostText_) {
        powerBoostText_->SetPosition(kPowerBoostTextPosition);
        powerBoostText_->Update(view, proj);
        powerBoostText_->Draw();
    }

    if (blockBg_) {
        blockBg_->SetPosition(kDefenseUiPosition);
        blockBg_->SetScale({
            kDefenseUiSize.x / kDefenseUiTextureSize.x,
            kDefenseUiSize.y / kDefenseUiTextureSize.y,
            1.0f
            });
        blockBg_->Update(view, proj);
        blockBg_->Draw();
    }
    if (blockText_) {
        blockText_->SetPosition(kBlockTextPosition);
        blockText_->SetColor({ 1.0f, 1.0f, 1.0f });
        blockText_->SetAlpha(1.0f);
        for (size_t i = 0; i < blockOutlineTexts_.size(); ++i) {
            auto& outlineText = blockOutlineTexts_[i];
            if (!outlineText) {
                continue;
            }
            outlineText->SetText(battle_.GetPlayerBlockText());
            outlineText->SetPosition({
                kBlockTextPosition.x + kOutlineDirections[i].x * kBlockOutlineThickness,
                kBlockTextPosition.y + kOutlineDirections[i].y * kBlockOutlineThickness
                });
            outlineText->SetColor({ 0.0f, 0.0f, 0.0f });
            outlineText->SetAlpha(1.0f);
            outlineText->Update(view, proj);
            outlineText->Draw();
        }
        blockText_->Update(view, proj);
        blockText_->Draw();
    }

    for (auto& text : enemyHpTexts_) {
        if (text) {
            text->Update(view, proj);
            text->Draw();
        }
    }

    if (tutorialUi_ && tutorial_ && !tutorialMenuVisible_) {
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

    DrawTutorialMenu_(app);

    // 円形マスク描画
    if (pokerHandHelpView_) {
        pokerHandHelpView_->Draw(view, proj);
    }

    drawMasks();
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

        if (ImGui::CollapsingHeader("Message Editor")) {
            auto loadMessageBuffer = [&]() {
                const std::string text = TutorialManager::WStringToUtf8(
                    tutorial_->GetEditableStepMessage(messageEditorStep_)
                );
                strncpy_s(messageEditBuffer_.data(), messageEditBuffer_.size(), text.c_str(), _TRUNCATE);
                messageEditDirty_ = false;
                messageEditorInitialized_ = true;
            };

            if (!messageEditorInitialized_) {
                messageEditorStep_ = tutorial_->GetStep();
                loadMessageBuffer();
            }

            ImGui::Text("Path: %s", tutorial_->GetMessagePath().c_str());

            if (ImGui::Button("Use Current Step")) {
                messageEditorStep_ = tutorial_->GetStep();
                loadMessageBuffer();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reload JSON")) {
                tutorial_->ReloadMessages();
                loadMessageBuffer();
            }

            const char* currentKey = TutorialManager::GetStepKey(messageEditorStep_);
            if (ImGui::BeginCombo("Step", currentKey && currentKey[0] ? currentKey : "(unknown)")) {
                for (TutorialManager::TutorialStep step : kEditableTutorialSteps) {
                    const char* key = TutorialManager::GetStepKey(step);
                    const bool selected = step == messageEditorStep_;
                    if (ImGui::Selectable(key && key[0] ? key : "(unknown)", selected)) {
                        messageEditorStep_ = step;
                        loadMessageBuffer();
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::InputTextMultiline(
                "Message",
                messageEditBuffer_.data(),
                messageEditBuffer_.size(),
                ImVec2(520.0f, 140.0f),
                ImGuiInputTextFlags_AllowTabInput
            )) {
                tutorial_->SetStepMessage(
                    messageEditorStep_,
                    TutorialManager::Utf8ToWString(messageEditBuffer_.data())
                );
                messageEditDirty_ = true;
            }

            if (ImGui::Button("Apply")) {
                tutorial_->SetStepMessage(
                    messageEditorStep_,
                    TutorialManager::Utf8ToWString(messageEditBuffer_.data())
                );
                messageEditDirty_ = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Save JSON")) {
                tutorial_->SetStepMessage(
                    messageEditorStep_,
                    TutorialManager::Utf8ToWString(messageEditBuffer_.data())
                );
                messageEditDirty_ = !tutorial_->SaveMessages();
            }
            ImGui::SameLine();
            ImGui::Text("%s", messageEditDirty_ ? "edited" : "saved/applied");
        }
    }

    if (tutorialContentInitialized_) {
        battle_.DrawImGui();
    }
    if (player_) {
        player_->DrawShieldImGui();
    }

    ImGui::End();

    if (fieldUi_) {
        ImGui::Begin("FieldUi Debug");
        fieldUi_->DrawImGui();
        ImGui::End();

        ImGui::Begin("Cost Meter Editor");
        fieldUi_->DrawCostMeterImGui();
        ImGui::End();
    }

    if (tutorialFieldProps_) {
        tutorialFieldProps_->DrawImGui("Tutorial Field Props", kTutorialFieldConfigPath);
    }

    if (tutorialContentInitialized_) {
        ImGui::Begin("Camera Setup (Tutorial)");
        ImGui::SliderFloat("Split Ratio", &splitRatio_, 0.1f, 0.9f);
        ImGui::SliderFloat("Field Camera Zoom", &fieldCameraZoom_, 0.1f, 3.0f);
        ImGui::SliderFloat("Field Camera RotX Offset", &fieldCameraRotXOffset_, -0.5f, 0.5f);
        ImGui::SliderFloat("Battle Camera Zoom", &battleCameraZoom_, 0.1f, 3.0f);
        ImGui::SliderFloat("Battle Camera RotX Offset", &battleCameraRotXOffset_, -0.5f, 0.5f);
        ImGui::End();
    }

    if (player_) {
        Vector3 startPos = player_->GetPos() + Vector3(0.0f, 1.0f, 0.0f);
        Vector3 targetPos = { 7.0f, 1.0f, 15.0f };
        if (!enemyMgr_.GetEnemies().empty()) {
            targetPos = enemyMgr_.GetEnemies().front().GetPos() + Vector3(0.0f, 1.0f, 0.0f);
        }
        player_->GetEffectSequencer().DrawImGuiEditor(startPos, targetPos);
    }

    if (tutorialContentInitialized_) {
        ImGui::Begin("Particle Object Post (Tutorial)");
        ImGui::Checkbox("Enable Particle Object Post", &particleObjectPostEnabled_);
        ImGui::DragFloat("Post Threshold", &particleObjectPostParam_.threshold, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Post Intensity", &particleObjectPostParam_.intensity, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Chromatic Aberration", &particleObjectPostParam_.chromAbAmount, 0.001f, 0.0f, 0.1f);
        ImGui::DragFloat("Distortion", &particleObjectPostParam_.distortionAmount, 0.001f, 0.0f, 0.2f);
        ImGui::DragFloat("Noise", &particleObjectPostParam_.noiseIntensity, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Glitch", &particleObjectPostParam_.glitchAmount, 0.001f, 0.0f, 0.2f);
        if (ImGui::Button("Reset Particle Object Post")) {
            ResetParticleObjectPostParam_();
        }
        ImGui::End();
    }

    if (tutorialUi_ && tutorial_) {
        tutorialUi_->DrawImGui(*tutorial_);
    }
    if (pokerHandHelpView_) {
        pokerHandHelpView_->DrawImGui();
    }

    if (pausingUI_) {
        pausingUI_->DrawImGui();
    }

#else
    (void)app;
#endif
}

void TutorialScene::DrawSkydome(GameApp& app) {
    if (tutorialMenuVisible_) {
        return;
    }

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
    if (tutorialMenuVisible_) {
        return;
    }

    int windowW = WinApp::kClientWidth;
    int windowH = WinApp::kClientHeight;
    int battleHeight = battle_.IsActionSequencePlaying() ? windowH : static_cast<int>(windowH * splitRatio_);
    app.Dx()->SetViewport(0, 0, windowW, windowH);
    app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);

    if (particleManager_) {
        if (particleObjectPostEnabled_) {
            app.DrawModelParticlesObjectPostToBloomScene(particleManager_, particleObjectPostParam_);
            app.Dx()->SetScissorRect(0, 0, windowW, battleHeight);
        } else {
            particleManager_->Draw();
        }
    }
    app.ObjCom()->SetGraphicsPipelineState();
    if (player_) {
        player_->DrawPostEffect(app);
    }
    app.Dx()->SetScissorRect(0, 0, windowW, windowH);
}

void TutorialScene::DrawPostEffect2D(GameApp& app) {
    (void)app;
}

void TutorialScene::ResetParticleObjectPostParam_()
{
    particleObjectPostParam_ = {};
    particleObjectPostParam_.threshold = 0.0f;
    particleObjectPostParam_.intensity = 1.25f;
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
