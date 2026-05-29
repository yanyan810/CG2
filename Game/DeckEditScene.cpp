#include "DeckEditScene.h"
#include "GameApp.h"
// カード情報参照用
#include <imgui.h>
#include <algorithm>

#include "CardInstance.h"
#include "Card3D.h"
#include "AudioManager.h"
#include "Logic/CardPreview.h"
#include "../Game/Card/DeckLoader.h"

std::string DeckEditScene::returnSceneName_ = "StageSelect";

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

	saveDeckButton_ = std::make_unique<Button>();
	saveDeckButton_->Initialize(app, L"保存して戻る", "SaveDeckAndChangeScene", { 1025.f, 450.f });
	saveDeckButton_->SetScale({ 175.f, 100.f });
	saveDeckButton_->SetTextOffset({ 10.f,15.f });
	saveDeckButton_->SetNormalColor({ 0.086f, 0.447f, 0.969f, 1.0f });
	saveDeckButton_->SetHoverColor({ 0.0f, 0.149f, 0.710f, 1.0f });

	nosaveButton_ = std::make_unique<Button>();
	nosaveButton_->Initialize(app, L"保存せず戻る", "NoSaveDeckAndChangeScene", { 1025.f, 600.f });
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
	warningText_->SetPosition({ 1000.0f, 430.0f });

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
	baseBg_ = std::make_unique<Sprite>();
	baseBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	baseBg_->SetPosition({ 0.f, 0.f });
	baseBg_->SetScale({ 1280.f, 720.f, 1.0f });
	baseBg_->SetColor({ 0.1f, 0.1f, 0.1f, 1.f });


	auto addButtonWithDesc = [&](const std::wstring& label, const std::string& path, Vector2 pos) {
		auto btn = std::make_unique<Button>();
		btn->Initialize(app, label, path, pos);
		deckTemplateButtons_.push_back(std::move(btn));

		// カスタム進むボタンなどの特殊な文字列以外はJSONとしてロード
		if (path != "CUSTOM_EDIT") {
			DeckDef tempDef{};
			if (DeckLoader::LoadFromJson(path, tempDef)) {
				std::string descA = tempDef.description;

				// 日本語（UTF-8）を壊さずに正しく wstring（UTF-16）に変換する
				std::wstring descW = L"";
				if (!descA.empty()) {
					int size_needed = MultiByteToWideChar(CP_UTF8, 0, &descA[0], (int)descA.size(), NULL, 0);
					descW.resize(size_needed);
					MultiByteToWideChar(CP_UTF8, 0, &descA[0], (int)descA.size(), &descW[0], size_needed);
				}
				deckTemplateDescriptions_.push_back(descW);

			} else {
				deckTemplateDescriptions_.push_back(L"デッキデータの読み込みに失敗しました。");

			}
		} else {
			// CUSTOM_EDIT 用のダミー説明文（空文字など）
			deckTemplateDescriptions_.push_back(L"");
		}
		};

	// ラベル、ファイルパス（兼識別名）、座標
	Vector2 tempDeckBtnBasePos = { 480.f,150.f };
	addButtonWithDesc(L"デフォルトデッキ", "resources/deck/deck_default.json", { tempDeckBtnBasePos.x, tempDeckBtnBasePos.y });
	addButtonWithDesc(L"毒デッキ", "resources/deck/deck_poison.json", { tempDeckBtnBasePos.x, tempDeckBtnBasePos.y + 150.f });
	addButtonWithDesc(L"凍結デッキ", "resources/deck/deck_frost.json", { tempDeckBtnBasePos.x, tempDeckBtnBasePos.y + 300.f });
	addButtonWithDesc(L"選択せず進む", "CUSTOM_EDIT", { 50.f,  50.f });


	// --- ツールチップUIの初期化 ---
	templateTooltipBg_ = std::make_unique<Sprite>();
	templateTooltipBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
	templateTooltipBg_->SetScale({ 450.f, 150.f, 1.0f }); // テキスト量に合わせてサイズ調整
	templateTooltipBg_->SetColor({ 0.05f, 0.05f, 0.05f, 0.95f }); // 深い黒（透過あり）

	templateTooltipText_ = std::make_unique<TextSprite>();
	templateTooltipText_->Initialize(app.SpriteCom(), app.Dx());
	templateTooltipText_->SetFontSize(30);
	templateTooltipText_->SetSize({ 1.f, 1.f, 1.f });
	templateTooltipText_->SetColor({ 1.0f, 1.0f, 1.0f });

	UpdateSprites(app);

	isSelectingTemplateDeck_ = true;

	statusMenu_.Initialize(app, { 100.f, 300.f });
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


	Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
	Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);


	baseBg_->Update(view, proj);


	for (auto& card : cardModels_) {
		card->Update(dt);
	}

	if (isSelectingTemplateDeck_) {
		input->SetWheel(0);

		bool isAnyButtonHovered = false;

		statusMenu_.Update(app, view, proj);

		// インデックス付きでループを回す
		for (size_t i = 0; i < deckTemplateButtons_.size(); ++i) {
			auto& btn = deckTemplateButtons_[i];
			btn->Update(app, view, proj);

			// マウスオーバーの判定
			if (btn->IsMouseOver() && btn->GetName() != "CUSTOM_EDIT") {
				// キャッシュされた説明文が空でなければ表示する
				if (!deckTemplateDescriptions_[i].empty()) {
					isAnyButtonHovered = true;

					// テキストを設定
					templateTooltipText_->SetText(deckTemplateDescriptions_[i]);

					// ボタンの座標とサイズを取得して、右隣に固定する
					Vector2 btnPos = btn->GetPosition();
					Vector2 btnSize = btn->GetScale(); // ボタンの横幅・縦幅

					// ボタンの左端(btnPos.x) + 横幅(btnSize.x) + 少しの隙間(20px)
					Vector2 popupPos = { btnPos.x + btnSize.x + 20.f, btnPos.y };

					templateTooltipBg_->SetPosition(popupPos);
					// テキストは枠の内側に少し余白（パディング）を作る
					templateTooltipText_->SetPosition({ popupPos.x + 15.f, popupPos.y + 15.f });
				}
			}

			// クリックされたあとの挙動
			if (btn->IsPressed()) {
				AudioManager::GetInstance()->PlaySE("SE_Tap");
				if (btn->GetName() == "CUSTOM_EDIT") {
					isSelectingTemplateDeck_ = false;
				} else {
					DeckDef tempDeckDef{};
					std::string err;

					// ここはシーン遷移時の一度きりなので、直接ロードでOK
					if (DeckLoader::LoadFromJson(btn->GetName(), tempDeckDef) &&
						DeckLoader::ValidateDeck(tempDeckDef, *app.GetCardDB(), err)) {

						editingDeck_.clear();
						for (const auto& e : tempDeckDef.cards) {
							editingDeck_[e.id] = e.count;
						}

						RecalculateTotal();
						RebuildCardModels(app);
						isSelectingTemplateDeck_ = false;
					} else {
						assert(false && "Template deck load failed.");
					}
				}
				break;
			}
		}

		// ホバー中の場合のみツールチップの行列更新を行う
		if (isAnyButtonHovered) {
			templateTooltipBg_->Update(view, proj);
			templateTooltipText_->Update(view, proj);
		}

		// 表示フラグを更新
		isHoverd_ = isAnyButtonHovered;

		// テンプレ画面選択中は、後半のエディット用ホバー判定に進ませずここで終了する
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

	if (saveDeckButton_->IsPressed() && isDeckValid_) {
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
		RequestChangeScene_(returnSceneName_.c_str());

		return;
	}

	// デッキ枚数足りないなら強制灰色
	if (!isDeckValid_) {
		saveDeckButton_->SetHoverColor({ 0.2f,0.2f,0.2f,1.f });
		saveDeckButton_->SetNormalColor({ 0.2f,0.2f,0.2f,1.f });
	} else {
		saveDeckButton_->SetNormalColor({ 0.086f, 0.447f, 0.969f, 1.0f });
		saveDeckButton_->SetHoverColor({ 0.0f, 0.149f, 0.710f, 1.0f });
	}

	if (nosaveButton_->IsPressed()) {
		// --- シーン遷移 ---
		AudioManager::GetInstance()->PlaySE("SE_Tap");
		RequestChangeScene_(returnSceneName_.c_str());

		return;
	}

	// ---  スプライトの更新 ---
	UpdateSprites(app);

	// --- 2. スクロール量の更新 ---
	float scrollSpeed = 1.0f; // ホイール1目盛りあたりの移動量

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

		// 計算されたスクロールオフセットを適用
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
			cardModels_[i]->SetFrameColor({ 0.2f, 0.2f, 0.2f, 1.0f }); // かなり暗く
		} else if (isDeckFull && currentCount > 0) {
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
			int cardId = idx + 1;
			int currentCount = editingDeck_[cardId];

			float x = (idx % kCardsPerRow) * kCardSpacingX + kCardStartX;
			float y = -(idx / kCardsPerRow) * kCardSpacingY + kCardStartY + scrollY_;
			Vector3 currentPos = { x, y, 0.0f };
			Vector3 baseRot = cardModels_[idx]->GetModelFixRot();
			float defaultScl = 0.25f;

			if (leftClick) {
				if (currentCount < 4 && totalCount_ < 40) {
					editingDeck_[cardId]++;
					AudioManager::GetInstance()->PlaySE("SE_CardFlick");

					cardModels_[idx]->SetTransform(currentPos, baseRot, { 0.4f, 0.4f, 0.4f });
					cardModels_[idx]->SetTargetTransform(currentPos, baseRot, { defaultScl, defaultScl, defaultScl });
				}
			} else if (rightClick) {
				if (currentCount > 0) {
					editingDeck_[cardId]--;
					AudioManager::GetInstance()->PlaySE("SE_CardFlick");

					cardModels_[idx]->SetTransform(currentPos, baseRot, { 0.1f, 0.1f, 0.1f });
					cardModels_[idx]->SetTargetTransform(currentPos, baseRot, { defaultScl, defaultScl, defaultScl });
				}
			}
			RecalculateTotal();
		}
	}

	for (auto& card : cardModels_) {
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
		std::vector<int> finalDeck;
		finalDeck.reserve(40);
		for (auto const& [id, count] : editingDeck_) {
			for (int j = 0; j < count; ++j) {
				finalDeck.push_back(id);
			}
		}

		app.SetDeckInstancesFromId(finalDeck);

		AudioManager::GetInstance()->PlaySE("SE_Tap");
		RequestChangeScene_("Game");

		return;
	}
}

void DeckEditScene::Draw3D(GameApp& app) {
	if (baseBg_) baseBg_->Draw();
	if (!isSelectingTemplateDeck_) {
		
		for (auto& card : cardModels_) {
			card->Draw();
		}
	}
}

void DeckEditScene::Draw2D(GameApp& app) {

	

	if (isSelectingTemplateDeck_) {
		
		for (const auto& btn : deckTemplateButtons_) {
			btn->Draw();
		}
		if (isHoverd_) {
			if (templateTooltipBg_) templateTooltipBg_->Draw();
			if (templateTooltipText_) templateTooltipText_->Draw();
		}

		statusMenu_.Draw();

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

	// 通常エディット画面のポップアップはテンプレ画面を抜けているときだけ描画するように制限
	if (isHoverd_ && !isSelectingTemplateDeck_ && !nosaveButton_->IsMouseOver())
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
		statusMenu_.DrawImGui();
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

	int index = 0;
	for (int i = 1; i <= cardCount_; ++i) {

		const CardDef* def = cardDB_->Find(i);
		if (!def) continue;

		auto card = std::make_unique<Card3D>();
		card->Setup(app.ObjCom(), app.Dx(), camera_.get());

		CardInstance inst;
		inst.defId = def->id;
		inst.number = 1;
		inst.suit = CardSuit::Spade;

		card->SetIsPreview(true);
		card->SetCardData(*def, inst);
		card->SetIsHand(false);

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

	float sw = 1280.0f;
	float sh = 720.0f;

	for (int i = 0; i < (int)cardModels_.size(); ++i) {
		Vector2 screenPos;
		if (WorldToScreen(cardModels_[i]->GetWorldPos(), viewProj, sw, sh, screenPos)) {
			float dx = std::abs(screenPos.x - (float)mousePos.x);
			float dy = std::abs(screenPos.y - (float)mousePos.y);
			if (dx < 40.0f && dy < 60.0f) {
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

	saveDeckButton_->Update(app, view, proj);
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

	Vector3 worldPos = cardModels_[cardIdx]->GetWorldPos();

	Vector2 screenPos;
	Matrix4x4 viewProj = camera_->GetViewMatrix() * camera_->GetProjectionMatrix();
	float sw = 1280.0f;
	float sh = 720.0f;

	if (WorldToScreen(worldPos, viewProj, sw, sh, screenPos)) {
		float offsetX = 80.0f;
		float offsetY = -150.0f;

		return { screenPos.x + offsetX, screenPos.y + offsetY };
	}

	return { 0, 0 };
}

void DeckEditScene::RerollDeckData(GameApp& app) {
	cardDB_ = app.GetCardDB();
	cardCount_ = cardDB_->GetCardCount();

	auto const& deck = app.GetDeckInstances();
	for (auto const& card : deck) {
		editingDeck_[card.defId]++;
	}
	RecalculateTotal();
	RebuildCardModels(app);
}