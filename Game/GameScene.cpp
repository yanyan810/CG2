#include "GameScene.h"
#include "GameApp.h"

#include "Camera.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "Particle.h"
#include "ParticleCommon.h"
#include "TextureManager.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "WinApp.h"
#include "Matrix4x4.h"

#include <d3d12.h>

GameScene::~GameScene() = default;

static float RandRange(float min, float max) {
    return min + (max - min) * (float(rand()) / float(RAND_MAX));
}

void GameScene::OnEnter(GameApp& app) {
    // テクスチャやモデルのロード（必要なものをここで）
  //  TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

    input_ = app.GetInput();
    assert(input_); // ここでnullなら初期化順が悪い

    camera_ = std::make_unique<Camera>();

    // 斜め上・少し後ろ
    camera_->SetTranslate({
        0.0f,   // X
        20.0f,   // Y（高さ）
       -50.0f   // Z（後ろ）
        });

    // 少し下向き（ラジアン）
    camera_->SetRotate({
        0.35f,  // X回転（見下ろし）
        0.0f,   // Y
        0.0f
        });


    // 近接3（0,1,2秒）
    for (int i = 0; i < 3; ++i) {
        enemyMgr_.QueueSpawn(EnemyType::Melee, 1.0f * i);
    }

    // シューター3（0.5,1.5,2.5秒）
    for (int i = 0; i < 3; ++i) {
        enemyMgr_.QueueSpawn(EnemyType::Shooter, 0.5f + 1.0f * i);
    }


    // ObjCommon は GameApp が持つ。カメラ設定だけここで。
    app.ObjCom()->SetDefaultCamera(camera_.get());

    sprite_ = std::make_unique<Sprite>();
   // sprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/uvChecker.png");
    sprite_->AdjustTextureSize();

    //player
    player_ = std::make_unique<Player>();
    player_->Initialize(app.ObjCom(), app.Dx(), camera_.get());
    player_->SetSpawnPos({ -12.0f, 0.0f, 15.0f });

    //enemy
    enemyMgr_.Initialize(app.ObjCom(), app.Dx(), camera_.get());

    enemyMgr_.Spawn(EnemyType::Boss, Vector3{ 0.0f, 0.0f, 15.0f });


    // テストスポーン
 /*   enemyMgr_.Spawn(EnemyType::Melee, { 5.0f, 0.0f ,15.0f});
    enemyMgr_.Spawn(EnemyType::Shooter, { 9.0f, 0.0f ,15.0f});*/
    // bossはステージ5で

    TextureManager::GetInstance()->LoadTexture("resources/white1x1.png");

    hpBack_ = std::make_unique<Sprite>();
    hpBack_->Initialize(app.SpriteCom(), app.Dx(), "resources/white1x1.png");
    hpBack_->SetPosition({ 30.0f, 30.0f });
    hpBack_->SetScale({ 300.0f, 20.0f ,1.0f}); // 背景バー

    hpFill_ = std::make_unique<Sprite>();
    hpFill_->Initialize(app.SpriteCom(), app.Dx(), "resources/white1x1.png");
    hpFill_->SetPosition({ 30.0f, 30.0f });
    hpFill_->SetScale({ 300.0f, 20.0f,1.0f }); // 中身バー（毎フレーム幅変える）

    // 0..9ロード
    for (int i = 0; i < 10; ++i) {
        char path[256];
        sprintf_s(path, "resources/ui/num/%d.png", i);
        TextureManager::GetInstance()->LoadTexture(path);
    }

    // 3桁分Sprite作る
    for (int i = 0; i < 3; ++i) {
        hpDigits_[i] = std::make_unique<Sprite>();
        hpDigits_[i]->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/num/0.png");

        // アンカーを左上にすると位置合わせが楽（好み）
        hpDigits_[i]->SetAnchorPoint({ 0.0f, 0.0f });
    }

    for (int i = 0; i < 3; ++i) {
        if (!hpDigits_[i]) continue;
        hpDigits_[i]->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f }); // ★黒
    }

    hpBack_->SetColor({ 0.2f, 0.2f, 0.2f, 1.0f }); // 濃いグレー
    hpFill_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 白

    auto* mgr = ModelManager::GetInstance();
    mgr->LoadModel("ground/ground.obj");

    ground_ = std::make_unique<Object3d>();
    ground_->Initialize(app.ObjCom(), app.Dx());
    ground_->SetCamera(camera_.get());
    ground_->SetModel("ground/ground.obj");

    ground_->SetTranslate({ 0.0f, 0.0f, 50.0f });
    ground_->SetScale({ 1.0f, 1.0f, 1.0f });
    ground_->SetRotate({ 0.0f, 0.0f, 0.0f });
 

}

void GameScene::OnExit(GameApp& /*app*/) {
    player_.reset(); // ★追加
    particle_.reset();
    objB_.reset();
    objA_.reset();
    sprite_.reset();
    camera_.reset();
    debugTitleParticle_.reset();
}


void GameScene::Update(GameApp& app, float dt) {
    // ESC で終了（Input クラス持ってるなら差し替え）
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        app.RequestQuit();
        return;
    }

    if (!input_) return; // 念のため

    camera_->Update();

    ground_->Update(dt);

    /*if (particle_) {
        particle_->SpawnParticle();
        particle_->Update();
    }*/

  

  /*  debugTitleParticle_->SpawnParticle();
    debugTitleParticle_->Update();*/

    if (player_) {
        player_->Update(dt, *input_, enemyMgr_);
    }

    //if (player_.IsDead()) {
    //    app.ChangeScene(std::make_unique<GameOverScene>()); // あなたの方式に合わせて
    //    return;
    //}


    // enemyMgr_ に渡す playerPos を Player から取る
    Vector2 playerPos2D{ 0.0f, 0.0f };
    if (player_) {
        playerPos2D = player_->GetPos2D();
    }

    float playerZ = 15.0f;
    if (player_) {
        playerZ = player_->GetZ(); // 追加したgetter
    }
    enemyMgr_.Update(dt, playerPos2D, playerZ,*player_);

    if (player_ && hpFill_) {
        int hp = player_->GetHP();      // ← getter作る（無ければ）
        int maxHp = player_->GetMaxHP();// ← getter作る（無ければ）

        float t = (maxHp > 0) ? (float(hp) / float(maxHp)) : 0.0f;
        t = std::clamp(t, 0.0f, 1.0f);

        const float fullW = 300.0f;
        hpFill_->SetScale({ fullW * t, 20.0f ,1.0f});
    }

    if (player_) {
        UpdateHPDigits_(player_->GetHP());
    }

    if (player_->IsDead()) {
        RequestChangeScene_("GameOver");
        return;
    }

    if (enemyMgr_.IsBossDefeated()) {
        RequestChangeScene_("GameClear");
        return;
    }

}

void GameScene::Draw(GameApp& app) {
    auto* dx = app.Dx();
    auto* srv = app.Srv();
    auto* cmd = dx->GetCommandList();


    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 3D
    app.ObjCom()->SetGraphicsPipelineState();
    //if (objA_) objA_->Draw();
    //if (objB_) objB_->Draw();

    if (ground_) ground_->Draw();

    //player
    if (player_) player_->Draw();
  
    enemyMgr_.Draw();

   

    // Particle
    app.ParticleCom()->SetGraphicsPipelineState();
  
    //

    // 2D sprite
    app.SpriteCom()->SetGraphicsPipelineState();

    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0, 0, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0, 100);

    // ★ 背景
    if (hpBack_) {
        hpBack_->Update(view, proj);
        hpBack_->Draw();
    }
    // ★ 中身
    if (hpFill_) {
        hpFill_->Update(view, proj);
        hpFill_->Draw();
    }

    // 数字（後 = 上に乗る）
    for (int i = 0; i < 3; ++i) {
        if (!hpDigits_[i]) continue;
        hpDigits_[i]->Update(view, proj);
        hpDigits_[i]->Draw();
    }
   
}

void GameScene::SpawnEnemyFromOutside_(EnemyType type) {
    // ===== 画面範囲（調整用） =====
    const float screenLeft = -12.0f;
    const float screenRight = 12.0f;
    const float outsidePad = 3.0f;   // 画面外にどれだけ出すか
    const float z = 15.0f;

    // 左 or 右 をランダム
    bool fromLeft = (rand() % 2) == 0;

    float x;
    if (fromLeft) {
        x = RandRange(screenLeft - outsidePad - 3.0f,
            screenLeft - outsidePad);
    } else {
        x = RandRange(screenRight + outsidePad,
            screenRight + outsidePad + 3.0f);
    }

    // Y は少しランダムに
    float y = RandRange(-1.0f, 1.0f);

    enemyMgr_.Spawn(type, Vector3{ x, y, z });
}

void GameScene::UpdateHPDigits_(int hp) {
    hp = std::clamp(hp, 0, 999);

    int d0 = (hp / 100) % 10;
    int d1 = (hp / 10) % 10;
    int d2 = (hp / 1) % 10;

    // 表示する桁（先頭ゼロ消し）
    bool show0 = (hp >= 100);
    bool show1 = (hp >= 10);
    bool show2 = true;

    // 右詰めでバーの上に置く（例：バー右端付近）
    const float baseX = 128.0f*1.5f; // ★バー右端
    const float baseY = hpBarPos_.y - 18.0f;              // ★バーの上

    const float w = 16.0f;     // 1桁幅（画像に合わせて調整）
    const float h = 20.0f;     // 1桁高さ
    const float sp = 2.0f;     // 桁間

    auto setDigit = [&](int idx, int digit, float x, float y, bool visible) {
        if (!hpDigits_[idx]) return;
        if (!visible) {
            hpDigits_[idx]->SetPosition({ -9999.0f, -9999.0f });
            return;
        }
        char path[256];
        sprintf_s(path, "resources/ui/num/%d.png", digit);
        hpDigits_[idx]->SetTextureFilePath(path);

        hpDigits_[idx]->SetPosition({ x, y });
        hpDigits_[idx]->SetScale({ 1.0f, 1.0f, 1.0f }); // 必要なら
        // サイズは Sprite の size_ が textureサイズになるので、
        // ここで見た目の大きさを変えたいなら scale_ を使うのが簡単
        // 例：hpDigits_[idx]->SetScale({ w/texW, h/texH, 1 });
        //
        // ただあなたのSpriteは size_ = textureCutSize_ でピクセルサイズになるので、
        // 「固定サイズ」にしたいなら Sprite に SetSize を足すのがベスト。
        };

    // 右詰め配置：一の位を一番右
    float x2 = baseX - (w);
    float x1 = x2 - (w + sp);
    float x0 = x1 - (w + sp);

    // ※あなたのSpriteは size_ を直接変えられないので、今は scale_ で縮める運用が楽。
    // ここでは position だけ決めて、見た目サイズは画像の元サイズに依存します。
    // 数字画像が大きい場合は scale_ を 0.5 などに。
    setDigit(0, d0, x0, baseY, show0);
    setDigit(1, d1, x1, baseY, show1);
    setDigit(2, d2, x2, baseY, show2);

    // もし数字がデカいなら
    for (int i = 0; i < 3; ++i) {
        if (hpDigits_[i]) hpDigits_[i]->SetScale({ 0.5f, 0.5f, 1.0f }); // 好みで調整
    }
}
