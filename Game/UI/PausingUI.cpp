#include "PausingUI.h"
#include "GameApp.h"

void PausingUI::Initialize(GameApp& app)
{

	// --- スプライトの初期化 ---

	// ポーズメニューの背景とテキスト
	pausingBg_ = std::make_unique<Sprite>();
	pausingBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	pausingBg_->SetPosition({ 0.0f,	0.0f });
	pausingBg_->SetScale({ 1280.0f, 720.0f, 1.0f });
	pausingBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.8f });

	pausingSprite_ = std::make_unique<Sprite>();
	pausingSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/PauseMenu.png");
	pausingSprite_->SetPosition({ 0.0f,	0.0f });
	pausingSprite_->SetScale({ 1.0f, 1.0f, 1.0f });
	pausingSprite_->SetColor({ 1.f, 1.f, 1.f, 1.f });


	// 再開の背景とテキスト
	resumeBg_ = std::make_unique<Sprite>();
	resumeBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	resumeBg_->SetPosition({ 530.0f, 210.0f });
	resumeBg_->SetScale({ 200.0f, 60.0f, 1.0f });
	resumeBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.9f });
	resumeBg_->SetName("Resume");

	// デッキ確認の背景とテキスト
	deckCheckBg_ = std::make_unique<Sprite>();
	deckCheckBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	deckCheckBg_->SetPosition({ 472.0f, 325.0f });
	deckCheckBg_->SetScale({ 320.0f, 60.0f, 1.0f });
	deckCheckBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.9f });
	deckCheckBg_->SetName("DeckCheck");
		
	// 降参確認の背景とテキスト
	giveUpCheckBg_ = std::make_unique<Sprite>();
	giveUpCheckBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	giveUpCheckBg_->SetPosition({ 530.0f, 440.0f });
	giveUpCheckBg_->SetScale({ 200.0f, 60.0f, 1.0f });
	giveUpCheckBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.9f });
	giveUpCheckBg_->SetName("GiveUp");

	// チュートリアル確認の背景とテキスト
	tutrialCheckBg_ = std::make_unique<Sprite>();
	tutrialCheckBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	tutrialCheckBg_->SetPosition({ 383.0f, 554.0f });
	tutrialCheckBg_->SetScale({ 500.0f, 60.0f, 1.0f });
	tutrialCheckBg_->SetColor({ 0.0f, 0.0f, 0.0f, 0.9f });
	tutrialCheckBg_->SetName("Tutorial");

	interactiveSprites_.clear();
    interactiveSprites_.push_back(resumeBg_.get());
    interactiveSprites_.push_back(deckCheckBg_.get());
    interactiveSprites_.push_back(giveUpCheckBg_.get());
    interactiveSprites_.push_back(tutrialCheckBg_.get());

}

void PausingUI::Update(GameApp& app, Input* input) {
	if (input->IsKeyTrigger(DIK_TAB)) {
		isPaused_ = true;
	}

	if (!isPaused_) return;

	POINT mousePoint = input->GetMousePosition();
	Vector2 mousePos = { static_cast<float>(mousePoint.x), static_cast<float>(mousePoint.y) };

	// 行列の作成などは共通
	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0, 100);

	// 全スプライトの更新（個別に書いても良いですが、ここもリスト化すると楽です）
	pausingBg_->Update(view, proj);
	pausingSprite_->Update(view, proj);
	for (auto* sprite : interactiveSprites_) {
		sprite->Update(view, proj);
	}

	// --- 名前ベースの当たり判定処理 ---
	for (auto* sprite : interactiveSprites_) {
		if (sprite->IsMouseOver(mousePos)) {
			// 名前を取得して判定
			std::string name = sprite->GetName();

			// ホバー時の共通処理
			sprite->SetColor({ 0.5f, 0.5f, 0.5f, 0.9f });

			// 降参ボタンは赤
			if (name == "GiveUp") {
				sprite->SetColor({ 0.8f, 0.2f, 0.2f, 0.9f });
			}

			// クリック時の名前判定
			if (input->IsMouseTrigger(0)) {
				

				if (name == "Resume") {
					isPaused_ = false;
				} else if (name == "DeckCheck") {
					// デッキ確認処理
				} else if (name == "GiveUp") {
					sprite->SetColor({ 0.8f, 0.2f, 0.2f, 0.9f }); // 特別な色
					// 降参処理
				} else if (name == "Tutorial") {
					// チュートリアル処理
				}
			}
		} else {
			// マウスが乗っていない時はデフォルト色
			sprite->SetColor({ 0.0f, 0.0f, 0.0f, 0.9f });
		}
	}
}

void PausingUI::Draw(GameApp& app)
{
	if (!isPaused_) {
		return; //	ポーズしてないなら更新も描画も必要ないので抜ける
	}

	if (pausingBg_)pausingBg_->Draw();
	if (resumeBg_)resumeBg_->Draw();
	if (deckCheckBg_)deckCheckBg_->Draw();
	if (giveUpCheckBg_)giveUpCheckBg_->Draw();
	if (tutrialCheckBg_)tutrialCheckBg_->Draw();
	if (pausingSprite_)pausingSprite_->Draw();
}


void PausingUI::DrawImGui() {
#ifdef _DEBUG // デバッグビルド時のみ表示
	if (ImGui::Begin("Pause UI Editor")) {

		auto EditSprite = [](const char* label, Sprite* sprite) {
			if (sprite && ImGui::TreeNode(label)) {
				// 座標の編集
				Vector2 pos = sprite->GetPosition();
				float p[2] = { pos.x, pos.y };
				if (ImGui::DragFloat2("Position", p, 1.0f)) {
					sprite->SetPosition({ p[0], p[1] });
				}

				// サイズ（スケール）の編集
				// Spriteクラスのscale_はVector3なので3要素で扱う
				Vector3 scale = sprite->GetScale();
				float s[3] = { scale.x, scale.y, scale.z };
				if (ImGui::DragFloat3("Scale", s, 1.0f, 0.0f, 2000.0f)) {
					sprite->SetScale({ s[0], s[1], s[2] });
				}

				// 色の編集
				Vector4 color = sprite->GetColor();
				float c[4] = { color.x, color.y, color.z, color.w };
				if (ImGui::ColorEdit4("Color", c)) {
					sprite->SetColor({ c[0], c[1], c[2], c[3] });
				}

				ImGui::TreePop();
				ImGui::Separator();
			}
			};

		// 各スプライトを個別に編集
		EditSprite("Pause Background", pausingBg_.get());
		EditSprite("Pause Menu Texture", pausingSprite_.get());
		EditSprite("Resume Button BG", resumeBg_.get());
		EditSprite("Deck Check BG", deckCheckBg_.get());
		EditSprite("Give Up BG", giveUpCheckBg_.get());
		EditSprite("Tutorial BG", tutrialCheckBg_.get());
	}
	ImGui::End();
#endif
}