#include "DeckEditScene.h"
#include "GameApp.h"
// カード情報参照用
#include <imgui.h>
#include <algorithm>

#include"CardInstance.h"
#include"Card3D.h"
#include "AudioManager.h"
#include"Logic/CardPreview.h"
#include "../Game/Card/DeckLoader.h"


void DeckEditScene::OnEnter(GameApp& app) {
	AudioManager::GetInstance()->PlayBGM("BGM_DeckEdit");

	camera_ = std::make_unique<Camera>();
	camera_->SetTranslate({ 0.0f, 4.0f, -10.0f });
	camera_->SetRotate({ 0.15f, 0.0f, 0.0f });
	camera_->Update();
	app.ObjCom()->SetDefaultCamera(camera_.get());

	skyDome_ = std::make_unique<Object3d>();
	skyDome_->Initialize(app.ObjCom(), app.Dx());
	skyDome_->SetModel("skydome/skydome.obj");
	skyDome_->SetCamera(camera_.get());
	skyDome_->SetEnableLighting(0);
	skyDome_->SetTranslate({ 0.0f, 0.0f, 0.0f });
	skyDome_->SetScale({ 100.0f, 100.0f, 100.0f });

	// 必要変数の初期化
	totalCount_ = 0;
	editingDeck_.clear();

	// 1. GameAppから CardInstance型でデッキを取得
	RerollDeckData(app);

	//// ボタンの背景スプライト（判定用）
	//saveDeckButtonBg_ = std::make_unique<Sprite>();
	//saveDeckButtonBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	//saveDeckButtonBg_->SetPosition({ 1025.f, 600.f }); // 画面右下あたり
	//saveDeckButtonBg_->SetScale({ 175.f, 100.f, 1.0f });
	//saveDeckButtonBg_->SetColor({ 0.f, 0.f, 0.3f, 0.8f }); // 暗めのグレー
	//saveDeckButtonBg_->SetColor({ 0.f, 0.f, 0.3f, 0.8f }); // 暗めのグレー

	//// 「ここを押して」テキスト
	//saveDeckButtonText_ = std::make_unique<TextSprite>();
	//saveDeckButtonText_->Initialize(app.SpriteCom(), app.Dx());
	//saveDeckButtonText_->SetText(L"保存して戻る");
	//saveDeckButtonText_->SetFontSize(24);
	//saveDeckButtonText_->SetSize({ 1.f, 1.f, 1.f });
	//saveDeckButtonText_->SetPosition({ 1060.0f, 620.0f });

	// 10,15
	saveDeckButton_ = std::make_unique<Button>();
	saveDeckButton_->Initialize(app, L"保存して戻る", "SaveDeckAndChangeScene", { 1025.f, 600.f });
	saveDeckButton_->SetScale({ 175.f, 100.f });
	saveDeckButton_->SetTextOffset({ 10.f,15.f });
	saveDeckButton_->SetNormalColor({ 0.086f, 0.447f, 0.969f, 1.0f });
	saveDeckButton_->SetHoverColor({ 0.0f, 0.149f, 0.710f, 1.0f });

	nosaveButton_ = std::make_unique<Button>();
	nosaveButton_->Initialize(app, L"保存せず戻る", "NoSaveDeckAndChangeScene", { 1025.f, 450.f });
	nosaveButton_->SetScale({ 175.f, 100.f });
	nosaveButton_->SetTextOffset({ 10.f,15.f });
	nosaveButton_->SetNormalColor({ 0.765f, 0.0f, 0.0f, 0.8f });
	nosaveButton_->SetHoverColor({ 0.251f, 0.0f, 0.0f, 0.8f });


	warningText_ = std::make_unique<TextSprite>();
	warningText_->Initialize(app.SpriteCom(), app.Dx());
	warningText_->SetText(L"デッキの枚数が足りません");
	warningText_->SetFontSize(24);
	warningText_->SetSize({ 1.f, 1.f, 1.f });
	warningText_->SetColor({ 1.0f, 0.0f, 0.0f });
	warningText_->SetPosition({ 1000.0f, 580.0f });

	countText_ = std::make_unique<TextSprite>();
	countText_->Initialize(app.SpriteCom(), app.Dx());
	countText_->SetFontSize(30);
	countText_->SetSize({ 1.f, 1.f, 1.f });
	countText_->SetPosition({ 1050.0f, 50.0f });
	countText_->SetText(countTextSup_ + L"0 / 40");

	controlHintText_ = std::make_unique<TextSprite>();
	controlHintText_->Initialize(app.SpriteCom(), app.Dx());
	controlHintText_->SetFontSize(30);
	controlHintText_->SetSize({ 1.f, 1.f, 1.f });
	controlHintText_->SetPosition({ 1000.0f, 200.0f });
	controlHintText_->SetText(L"カードを\n左クリック : 1枚追加\n右クリック : 1枚削除\nできます\n\nスクロールで\n上下に動かす");

	cardPreviewBg_ = std::make_unique<Sprite>();
	cardPreviewBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	cardPreviewBg_->SetPosition({ 0.f, 0.f }); // 画面右下あたり
	cardPreviewBg_->SetScale({ 300.f, 300.f, 1.0f });
	cardPreviewBg_->SetColor({ 0.1f, 0.1f, 0.1f, 1.f }); // 暗めのグレー

	cardPreviewText_ = std::make_unique<TextSprite>();
	cardPreviewText_->Initialize(app.SpriteCom(), app.Dx());
	cardPreviewText_->SetFontSize(30);
	cardPreviewText_->SetSize({ 1.f, 1.f, 1.f });
	cardPreviewText_->SetPosition({ 1000.0f, 200.0f });
	cardPreviewText_->SetText(L"");

	scrollY_ = kInitialScrollY;

	// Templateデッキ選択時
	selectingTemplateDeckBg_ = std::make_unique<Sprite>();
	selectingTemplateDeckBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	selectingTemplateDeckBg_->SetPosition({ 0.f, 0.f });
	selectingTemplateDeckBg_->SetScale({ 1280.f, 720.f, 1.0f });
	selectingTemplateDeckBg_->SetColor({ 0.1f, 0.1f, 0.1f, 0.72f });

	auto defaultBtn = std::make_unique<Button>();
	defaultBtn->Initialize(app, L"デフォルトデッキ", "resources/deck/deck_default.json", { 480.f, 250.f });
	deckTemplateButtons_.push_back(std::move(defaultBtn));

	auto poisonBtn = std::make_unique<Button>();
	poisonBtn->Initialize(app, L"毒デッキ", "resources/deck/deck_poison.json", { 480.f, 450.f });
	deckTemplateButtons_.push_back(std::move(poisonBtn));

	auto frostBtn = std::make_unique<Button>();
	frostBtn->Initialize(app, L"凍結デッキ", "resources/deck/deck_frost.json", { 480.f, 650.f });
	deckTemplateButtons_.push_back(std::move(frostBtn));

	auto customBtn = std::make_unique<Button>();
	customBtn->Initialize(app, L"選択せず進む", "CUSTOM_EDIT", { 50.f, 50.f });
	deckTemplateButtons_.push_back(std::move(customBtn));

}

void DeckEditScene::OnExit(GameApp& app) {
	(void)app;
	skyDome_.reset();
}

void DeckEditScene::Update(GameApp& app, float dt) {

	if (skyDome_) {
		skyDome_->SetCamera(camera_.get());
		skyDome_->Update(dt);
	}

	Input* input = app.GetInput();
	POINT mouse = input->GetMousePosition();
	Vector2 mousePos = { (float)mouse.x, (float)mouse.y };


	if (isSelectingTemplateDeck_) {
		input->SetWheel(0);

		Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
		Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);

		selectingTemplateDeckBg_->Update(view, proj);

		for (const auto& btn : deckTemplateButtons_) {
			btn->Update(app, view, proj);


			// クリックされたあとの「シーン固有の挙動」だけをScene側で書く
			if (btn->IsPressed()) {
				AudioManager::GetInstance()->PlaySE("SE_Tap");
				if (btn->GetName() == "CUSTOM_EDIT") {
					isSelectingTemplateDeck_ = false;
				} else {
					// 1. テンプレファイルを一時的な構造体にロードする
					DeckDef tempDeckDef{};
					std::string err;

					// app.LoadDeck の中身と同じように DeckLoader を直接使う
					if (DeckLoader::LoadFromJson(btn->GetName(), tempDeckDef) &&
						DeckLoader::ValidateDeck(tempDeckDef, *app.GetCardDB(), err)) {

						// 2. 現在の編集用マップをクリアし、テンプレの内容で数え直す
						editingDeck_.clear();
						for (const auto& e : tempDeckDef.cards) {
							// 各カードの ID と枚数を設定
							editingDeck_[e.id] = e.count;
						}

						// 3. 編集中の合計枚数を再計算し、3Dモデルを再生成
						RecalculateTotal();
						RebuildCardModels(app);

						isSelectingTemplateDeck_ = false;
					} else {
						// ロード失敗時の安全弁
						assert(false && "Template deck load failed.");
					}
				}
				break;
			}
		}
		return;
	}

	float wheel = float(input->GetWheel()); // 奥に回すとプラス、手前に回すとマイナス
	input->SetWheel(0);

	// デッキの枚数が40枚ちょうどかどうか
	isDeckValid_ = (totalCount_ == 40);



	if (countText_) {
		// 表示する文字列を作成
		std::wstring countStr = countTextSup_ + std::to_wstring(totalCount_) + L" / 40";
		countText_->SetText(countStr);

		// 40枚ちょうどなら緑、それ以外は白
		if (isDeckValid_) {
			countText_->SetColor({ 1.0f, 1.0f, 1.0f }); // 白
		} else {
			countText_->SetColor({ 1.0f, 0.0f, 0.0f }); // 赤
		}


	}

	if (saveDeckButton_->IsPressed()&&isDeckValid_) {
		// --- vector<int> 形式に変換 ---
		std::vector<int> finalDeck;
		finalDeck.reserve(40);
		for (auto const& [id, count] : editingDeck_) {
			for (int j = 0; j < count; ++j) {
				finalDeck.push_back(id);
			}
		}

		// --- GameAppに情報を渡す ---
		app.SetDeckInstancesFromId(finalDeck);

		// --- シーン遷移 ---
		AudioManager::GetInstance()->PlaySE("SE_Tap");
		RequestChangeScene_("StageSelect");

		return;
	}

	if (nosaveButton_->IsPressed()) {
		
		// --- シーン遷移 ---
		AudioManager::GetInstance()->PlaySE("SE_Tap");
		RequestChangeScene_("StageSelect");

		return;
	}

	// ---  スプライトの更新 ---
	UpdateSprites(app);

	// --- 2. スクロール量の更新 ---
	float scrollSpeed = 1.0f; // ホイール1目盛りあたりの移動量（調整してください）

	if (wheel != 0) {
		scrollY_ -= (static_cast<float>(wheel) / 120.0f) * scrollSpeed;
		scrollY_ = std::clamp(scrollY_, kMinScrollY, kMaxScrollY);

	}

	int index = 0;
	for (auto& card : cardModels_) {
		int row = index / kCardsPerRow;
		int col = index % kCardsPerRow;

		// 本来のレイアウト位置
		float x = kCardStartX + (col * kCardSpacingX);
		float y = kCardStartY - (row * kCardSpacingY);

		// ★計算されたスクロールオフセットを適用
		y += scrollY_;

		// カードの座標を即座に更新
		card->SetTargetTransform(
			{ x, y, 0.0f },
			card->GetModelFixRot(),
			{ 0.25f, 0.25f, 0.25f },
			false // instant を false にすることで補間を有効化
		);

		card->Update(dt);
		index++;
	}

	for (int i = 0; i < (int)cardModels_.size(); ++i) {
		int cardId = i + 1; // IDが1から始まる前提
		int currentCount = editingDeck_[cardId];

		// 条件1: そのカード自体がすでに4枚ある
		bool isIndividualMax = (currentCount >= 4);

		// 条件2: デッキ全体がすでに40枚ある
		bool isDeckFull = (totalCount_ >= 40);

		if (isIndividualMax || (isDeckFull && currentCount == 0)) {
			// 完全に上限、または空きがないのに0枚のカード
			cardModels_[i]->SetFrameColor({ 0.2f, 0.2f, 0.2f, 1.0f }); // かなり暗く
		} else if (isDeckFull && currentCount > 0) {
			// デッキは満杯だが、そのカード自体はデッキに入っている（減らせる）状態
			cardModels_[i]->SetFrameColor({ 0.4f, 0.4f, 0.4f, 1.0f }); // やや暗く
		} else {
			cardModels_[i]->ResetFrameColor(); // 通常時
		}

		cardModels_[i]->SetCount(currentCount);
	}

	// --- クリック判定 ---
	bool leftClick = input->IsMouseTrigger(0);  // 左クリック
	bool rightClick = input->IsMouseTrigger(1); // 右クリック

	if (leftClick || rightClick) {
		int idx = PickCardIndex(app);
		if (idx != -1) {
			// cardModels_ の並び順が CardDatabase の ID と対応している前提
			// (RebuildCardModels で 1~20 まで回している場合)
			int cardId = idx + 1;
			int currentCount = editingDeck_[cardId];

			// 現在の基本位置と回転を取得
			 // (RebuildCardModelsで設定した値をベースにする)
			float x = (idx % kCardsPerRow) * kCardSpacingX + kCardStartX;
			float y = -(idx / kCardsPerRow) * kCardSpacingY + kCardStartY + scrollY_;
			Vector3 currentPos = { x, y, 0.0f };
			Vector3 baseRot = cardModels_[idx]->GetModelFixRot();
			float defaultScl = 0.25f;

			if (leftClick) {
				if (currentCount < 4 && totalCount_ < 40) {
					editingDeck_[cardId]++;
					AudioManager::GetInstance()->PlaySE("SE_CardFlick");

					// --- 演出 ---
					// 1. 現在の値を「強制的に」大きくする (SetTransform)
					cardModels_[idx]->SetTransform(currentPos, baseRot, { 0.4f, 0.4f, 0.4f });
					// 2. 目標の値を「通常」に戻す (SetTargetTransform)
					// Card3D::Update 内の lerp によって、0.4f から 0.25f へ滑らかに戻ります
					cardModels_[idx]->SetTargetTransform(currentPos, baseRot, { defaultScl, defaultScl, defaultScl });
				}
			} else if (rightClick) {
				if (currentCount > 0) {
					editingDeck_[cardId]--;
					AudioManager::GetInstance()->PlaySE("SE_CardFlick");

					// 1. 現在の値を「強制的に」小さくする
					cardModels_[idx]->SetTransform(currentPos, baseRot, { 0.1f, 0.1f, 0.1f });
					// 2. 目標の値を通常に戻す
					cardModels_[idx]->SetTargetTransform(currentPos, baseRot, { defaultScl, defaultScl, defaultScl });
				}
			}
			RecalculateTotal();
		}
	}

	for (auto& card : cardModels_) {
		// 必要に応じて少し回転させるなど
		// Vector3 rot = card->GetWorldPos(); // 実際は回転プロパティが必要
		card->Update(dt);
	}

	int hoveredIdx = PickCardIndex(app);

	if (hoveredIdx != -1) {
		// カードに乗っている場合のみ、座標計算とテキスト更新を行う
		isHoverd_ = true;

		// 座標を計算
		Vector2 popupPos = GetPopupPosition(app, hoveredIdx);
		cardPreviewBg_->SetPosition(popupPos);
		cardPreviewText_->SetPosition(popupPos);

		// カード情報を取得してテキストを設定
		const CardDef* hoveredDef = cardDB_->Find(hoveredIdx + 1);
		cardPreviewText_->SetText(CardPreview::GetPreviewCardDetailText(hoveredDef));
	} else {
		// 乗っていない場合はフラグを折る
		isHoverd_ = false;
	}

	//================
	//Dキーを押したとき
	//================

	bool deckEditTrig = input->IsKeyTrigger(DIK_D);

	if (deckEditTrig && totalCount_ == 40) {
		// --- vector<int> 形式に変換 ---
		std::vector<int> finalDeck;
		finalDeck.reserve(40);
		for (auto const& [id, count] : editingDeck_) {
			for (int j = 0; j < count; ++j) {
				finalDeck.push_back(id);
			}
		}

		// --- GameAppに情報を渡す ---
		app.SetDeckInstancesFromId(finalDeck);

		// --- シーン遷移 ---
		AudioManager::GetInstance()->PlaySE("SE_Tap");
		RequestChangeScene_("Game");

		return;
	}
}

void DeckEditScene::Draw3D(GameApp& app) {
	for (auto& card : cardModels_) {
		card->Draw();
	}
}
void DeckEditScene::Draw2D(GameApp& app) {

	if (isSelectingTemplateDeck_) {
		if (selectingTemplateDeckBg_) selectingTemplateDeckBg_->Draw();
		for (const auto& btn : deckTemplateButtons_) {
			btn->Draw();
		}
		return;
	}

	saveDeckButton_->Draw();
	nosaveButton_->Draw();

	if (countText_) {
		countText_->Draw();
	}
	// 40枚ないときだけ警告を表示
	if (totalCount_ < 40 && warningText_) {
		warningText_->Draw();
	}
	if (controlHintText_) {
		controlHintText_->Draw();
	}

	if (isHoverd_&&!nosaveButton_->IsMouseOver())
	{
		if (cardPreviewBg_) {
			cardPreviewBg_->Draw();
		}
		if (cardPreviewText_) {
			cardPreviewText_->Draw();
		}
	}
}

void DeckEditScene::DrawImGui(GameApp& app) {
	if (isSelectingTemplateDeck_) {
		for (const auto& btn : deckTemplateButtons_) {
			btn->DrawImGui();
		}
		return;
	}

}

void DeckEditScene::DrawSkydome(GameApp& app) {
	(void)app;
	if (skyDome_) {
		skyDome_->Draw();
	}
}

void DeckEditScene::RebuildCardModels(GameApp& app) {

	cardModels_.clear();

	// データベースにあるカードを順番に並べる（例：ID 1〜20）
	int index = 0;

	for (int i = 1; i <= cardCount_; ++i) {

		const CardDef* def = cardDB_->Find(i);
		if (!def) continue;

		auto card = std::make_unique<Card3D>();
		// GameAppから必要な共通クラスを取得してSetup
		card->Setup(app.ObjCom(), app.Dx(), camera_.get());

		// 表示用のダミーインスタンスを作成
		CardInstance inst;
		inst.defId = def->id;
		inst.number = 1; // プレビュー用なので適当な数値
		inst.suit = CardSuit::Spade;

		card->SetIsPreview(true);

		card->SetCardData(*def, inst);
		card->SetIsHand(false); // 手札レイアウト（持ち上げ等）を無効化

		// グリッド配置の計算
		float x = (index % kCardsPerRow) * kCardSpacingX + kCardStartX;
		float y = -(index / kCardsPerRow) * kCardSpacingY + kCardStartY;
		card->SetTransform({ x, y, 10.0f }, { 0, 0, 0 }, { 0.25f, 0.25f, 0.25f });

		cardModels_.push_back(std::move(card));
		index++;
	}
}

bool WorldToScreen(const Vector3& w, const Matrix4x4& viewProj, float sw, float sh, Vector2& out) {
	Vector4 clip = {
		w.x * viewProj.m[0][0] + w.y * viewProj.m[1][0] + w.z * viewProj.m[2][0] + viewProj.m[3][0],
		w.x * viewProj.m[0][1] + w.y * viewProj.m[1][1] + w.z * viewProj.m[2][1] + viewProj.m[3][1],
		w.x * viewProj.m[0][2] + w.y * viewProj.m[1][2] + w.z * viewProj.m[2][2] + viewProj.m[3][2],
		w.x * viewProj.m[0][3] + w.y * viewProj.m[1][3] + w.z * viewProj.m[2][3] + viewProj.m[3][3]
	};
	if (clip.w <= 0.0001f) return false;
	out.x = ((clip.x / clip.w) * 0.5f + 0.5f) * sw;
	out.y = (-(clip.y / clip.w) * 0.5f + 0.5f) * sh;
	return true;
}

int DeckEditScene::PickCardIndex(GameApp& app) {
	auto mousePos = app.GetInput()->GetMousePosition();
	Matrix4x4 viewProj = camera_->GetViewMatrix() * camera_->GetProjectionMatrix();

	float sw = 1280.0f; // WinAppなどの定数があればそれを使用
	float sh = 720.0f;

	for (int i = 0; i < (int)cardModels_.size(); ++i) {
		Vector2 screenPos;
		if (WorldToScreen(cardModels_[i]->GetWorldPos(), viewProj, sw, sh, screenPos)) {
			// カードの当たり判定サイズ（画面上のピクセル範囲）を調整
			float dx = std::abs(screenPos.x - (float)mousePos.x);
			float dy = std::abs(screenPos.y - (float)mousePos.y);
			if (dx < 40.0f && dy < 60.0f) { // 判定の広さ
				return i;
			}
		}
	}
	return -1;
}

void DeckEditScene::RecalculateTotal() {
	totalCount_ = 0;
	for (auto const& [id, count] : editingDeck_) {
		totalCount_ += count;
	}
}

void DeckEditScene::UpdateSprites(GameApp& app) {

	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);

	saveDeckButton_->Update(app,view, proj);
	nosaveButton_->Update(app, view, proj);

	if (warningText_ && !isDeckValid_) {
		warningText_->Update(view, proj);
	}
	if (countText_) {
		countText_->Update(view, proj);
	}
	if (controlHintText_) {
		controlHintText_->Update(view, proj);
	}
	if (cardPreviewBg_) {
		cardPreviewBg_->Update(view, proj);
	}
	if (cardPreviewText_) {
		cardPreviewText_->Update(view, proj);
	}
}

Vector2 DeckEditScene::GetPopupPosition(GameApp& app, int cardIdx) {
	if (cardIdx == -1 || cardIdx >= (int)cardModels_.size()) return { 0, 0 };

	// 1. カードの3D世界座標を取得
	Vector3 worldPos = cardModels_[cardIdx]->GetWorldPos();

	// 2. スクリーン座標(ピクセル)に変換
	Vector2 screenPos;
	Matrix4x4 viewProj = camera_->GetViewMatrix() * camera_->GetProjectionMatrix();
	float sw = 1280.0f; // WinApp::kClientWidth 等
	float sh = 720.0f;  // WinApp::kClientHeight 等

	if (WorldToScreen(worldPos, viewProj, sw, sh, screenPos)) {
		// 3. カードの右側にオフセットを加える
		// dx < 40.0f という判定から、カードの半幅は約40pxと推測
		float offsetX = 80.0f;  // カードの右端からさらにどれくらい離すか
		float offsetY = -150.0f; // 少し上に表示したい場合などの調整

		return { screenPos.x + offsetX, screenPos.y + offsetY };
	}

	return { 0, 0 };
}

const CardDef* DeckEditScene::GetHoveredCardDef(GameApp& app) {
	int idx = PickCardIndex(app);
	if (idx != -1 && idx < (int)cardModels_.size()) {
		isHoverd_ = true;
		return cardDB_->Find(idx + 1);
	}
	return nullptr;
}

void DeckEditScene::RerollDeckData(GameApp& app) {
	const auto& currentInstances = app.GetDeckInstances();

	cardDB_ = app.GetCardDB();

	// 2. ID(int) だけを抽出して枚数をカウント
	editingDeck_.clear();

	for (const auto& inst : currentInstances) {
		editingDeck_[inst.defId]++;
	}

	// 合計枚数を計算
	RecalculateTotal();

	cardCount_ = cardDB_->GetCardCount();

	RebuildCardModels(app);
}
