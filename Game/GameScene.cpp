#include "GameScene.h"
#include "GameApp.h"
#include "Input.h"

static std::wstring Utf8ToWString(const std::string& s)
{
    if (s.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), size);
    return out;
}

void GameScene::OnEnter(GameApp& app) {
    // --------------------------------------------------
    // 1. カメラの初期化と設定
    // --------------------------------------------------
    camera_ = std::make_unique<Camera>();

    // ★カメラを原点(0, 0, 0)に配置し、少しだけ見下ろす角度に
    camera_->SetTranslate({ 0.0f, 4.0f, -40.0f }); // 高さを4.0fにして見下ろす
    camera_->SetRotate({ 0.15f, 0.0f, 0.0f });     // 軽く見下ろす角度
    app.ObjCom()->SetDefaultCamera(camera_.get());

    // --------------------------------------------------
    // 2. 背景（天球）の初期化
    // --------------------------------------------------
    skyDome_ = std::make_unique<Object3d>();
    skyDome_->Initialize(app.ObjCom(), app.Dx());
    skyDome_->SetModel("skydome/skydome.obj");
    skyDome_->SetCamera(camera_.get());
    skyDome_->SetEnableLighting(0);
    // ★カメラが原点になったので、天球の中心も原点にする
    skyDome_->SetTranslate({ 0.0f, 0.0f, 0.0f });
    skyDome_->SetScale({ 100.0f, 100.0f, 100.0f });

    // --------------------------------------------------
    // 3. プレイヤーとエネミーの初期化・配置
    // --------------------------------------------------
    // ★カメラが原点なので、キャラクターは Z 方向に押し出してカメラの前に置く
    const float charZ = 15.0f; // カメラから15.0f 奥

    // プレイヤーの配置（左側・右向き）
    player_ = std::make_unique<Player>();
    player_->Initialize(app.ObjCom(), app.Dx(), camera_.get());
    player_->SetSpawnPos({ -7.0f, 0.0f, charZ });
    player_->SetRotation({ 0.0f, 1.5708f, 0.0f });

    // エネミーの配置（右側・左向き）
    enemyMgr_.Initialize(app.ObjCom(), app.Dx(), camera_.get());
    enemyMgr_.Spawn(EnemyType::Boss, Vector3{ 7.0f, 0.0f, charZ });

    // --------------------------------------------------
    // 4. ライトの初期設定
    // --------------------------------------------------
    light_.lightingMode = 1;
    light_.dir = { 0.3f, -1.0f, 0.2f };
    light_.dirIntensity = 1.5f;
    light_.dirColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    // --------------------------------------------------
   //  5. バトルコントローラー
   // --------------------------------------------------
    battle_.Initialize(app, camera_.get());
    battle_.SetPlayer(player_.get());
    battle_.SetEnemy(enemyMgr_.GetBoss());

	// --------------------------------------------------
	// 6. 文字描画の初期化
	// --------------------------------------------------

    cardDescText_ = std::make_unique<TextSprite>();
    cardDescText_->Initialize(app.SpriteCom(), app.Dx());
    cardDescText_->SetPosition({ 40.0f, 620.0f });

    //白テクスチャ
    cardDescBg_ = std::make_unique<Sprite>();
    cardDescBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    cardDescBg_->SetPosition({ 20.0f, 60.0f });
    cardDescBg_->SetScale({ 900.0f, 180.0f, 1.0f });
    cardDescBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

}

void GameScene::OnExit(GameApp& app) {
}

void GameScene::Update(GameApp& app, float dt) {
    Input* input = app.GetInput();
    if (!input) return;

    camera_->Update();

    // ESCキーでタイトルへ戻る
    bool currEsc = input->IsKeyPressed(DIK_ESCAPE);
    if (currEsc && !prevEsc_) {
        RequestChangeScene_("Title");
    }
    prevEsc_ = currEsc;

    if (skyDome_) {
        skyDome_->Update(dt);
    }

    if (player_) {
        player_->Update(dt);
    }

    enemyMgr_.Update(dt);
    enemyMgr_.SetLighting(light_);

#ifdef USE_IMGUI

    ImGui::Begin("Battle Debug");
    battle_.DrawImGui();     // ★これ
    ImGui::End();
#endif

    battle_.Update(app, dt);

    if (battle_.HasPokerChoiceUi()) {
        cardDescText_->SetSize({ 1.0f,1.0f,1.0f });
        cardDescText_->SetPosition({ 40.0f, 80.0f });
        cardDescText_->SetText(battle_.GetPokerChoiceUiText());

        cardDescBg_->SetPosition({ 20.0f, 52.0f });
        cardDescBg_->SetScale({ 900.0f, 180.0f, 1.0f });
    } else {
        cardDescText_->SetSize({ 1.0f,1.0f,1.0f });
        cardDescText_->SetPosition({ 40.0f, 620.0f });

        const CardDef* def = battle_.GetPreviewCardDef();
        if (def) {
            cardDescText_->SetText(Utf8ToWString(def->desc));

            cardDescBg_->SetPosition({ 20.0f, 600.0f });
            cardDescBg_->SetScale({ 900.0f, 120.0f, 1.0f });
        } else if (battle_.ShouldShowOperationUi()) {
            cardDescText_->SetPosition({ 40.0f, 520.0f });
            cardDescText_->SetText(battle_.GetOperationUiText());

            cardDescBg_->SetPosition({ 20.0f, 500.0f });
            cardDescBg_->SetScale({ 900.0f, 280.0f, 1.0f });
        } else {
            cardDescText_->SetText(L"");
        }
    }

}

void GameScene::Draw3D(GameApp& app) {
    app.ObjCom()->SetGraphicsPipelineState();

    if (skyDome_) skyDome_->Draw();
    if (player_) player_->Draw();
    enemyMgr_.Draw();

    battle_.Draw3D(app);
}

void GameScene::Draw2D(GameApp& app) {
    app.SpriteCom()->SetGraphicsPipelineState();

    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0,
        float(WinApp::kClientWidth),
        float(WinApp::kClientHeight),
        0, 100
    );

    bool showDescBg = false;

    if (battle_.HasPokerChoiceUi()) {
        showDescBg = true;
    } else {
        const CardDef* def = battle_.GetPreviewCardDef();
        showDescBg = (def != nullptr) || battle_.ShouldShowOperationUi();
    }

    if (showDescBg && cardDescBg_) {
        cardDescBg_->Update(view, proj);
        cardDescBg_->Draw();
    }

    if (cardDescText_) {
        cardDescText_->Update(view, proj);
        cardDescText_->Draw();
    }
}

void GameScene::DrawImGui(GameApp& app) {
#ifdef USE_IMGUI
    ImGui::Begin("Battle Debug");
    battle_.DrawImGui();
    ImGui::End();
#endif
}