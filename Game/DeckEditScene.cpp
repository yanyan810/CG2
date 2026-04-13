#include "DeckEditScene.h"
#include "GameApp.h"
 // カード情報参照用
#include <imgui.h>
#include <algorithm>

#include"CardInstance.h"
#include"Card3D.h"

void DeckEditScene::OnEnter(GameApp& app) {
    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate({ 0.0f, 4.0f, -10.0f });
    camera_->SetRotate({ 0.15f, 0.0f, 0.0f });
    camera_->Update();
    app.ObjCom()->SetDefaultCamera(camera_.get());

    // 必要変数の初期化
    totalCount_ = 0;
    editingDeck_.clear();

    // 1. GameAppから CardInstance型でデッキを取得
    const auto& currentInstances = app.GetDeckInstances();

   cardDB_ = app.GetCardDB();

    // 2. ID(int) だけを抽出して枚数をカウント
    for (const auto& inst : currentInstances) {
        editingDeck_[inst.defId]++;
    }

    // 合計枚数を計算
    RecalculateTotal();

    RebuildCardModels(app);
}

void DeckEditScene::RecalculateTotal() {
    totalCount_ = 0;
    for (auto const& [id, count] : editingDeck_) {
        totalCount_ += count;
    }
}

void DeckEditScene::OnExit(GameApp& app) {

}

void DeckEditScene::Update(GameApp& app, float dt) {
    Input* input = app.GetInput();

    float wheel = float(input->GetWheel()); // 奥に回すとプラス、手前に回すとマイナス
    input->SetWheel(0);

    // --- 2. スクロール量の更新 ---
    float scrollSpeed = 1.0f; // ホイール1目盛りあたりの移動量（調整してください）

    if (wheel != 0) {
        
        scrollY_ -= (static_cast<float>(wheel) / 120.0f) * scrollSpeed;
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
}

void DeckEditScene::Draw3D(GameApp& app) {
    for (auto& card : cardModels_) {
        card->Draw();
    }
}
void DeckEditScene::Draw2D(GameApp& app) {

}

void DeckEditScene::DrawImGui(GameApp& app) {
    ImGui::Begin("Deck Editor");

    // 現在の合計枚数表示
    if (totalCount_ == 40) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Total: %d / 40 (OK!)", totalCount_);
    } else {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Total: %d / 40 (Need exactly 40)", totalCount_);
    }


    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
        1000.0f / ImGui::GetIO().Framerate,
        ImGui::GetIO().Framerate);

    ImGui::Separator();

    // カードリストをもとに表示
    ImGui::BeginChild("CardList", ImVec2(0, -50), true);

    // 仮にID 1〜100までループ（実際はCardDatabaseの中身に合わせて調整）
    for (int i = 1; i <= 20; ++i) {
        auto cardDef = cardDB_->Find(i);
        if (!cardDef) continue;

        int currentCount = editingDeck_[cardDef->id];

        ImGui::PushID(cardDef->id);
        ImGui::Text("%-15s [%d/4]", cardDef->name.c_str(), currentCount);
        ImGui::SameLine(ImGui::GetWindowWidth() - 100);

        // 重複4枚制限
        if (currentCount >= 4) ImGui::BeginDisabled();
        if (ImGui::Button("+")) {
            RecalculateTotal();
            if (totalCount_ < 40) {
                editingDeck_[cardDef->id]++;
                RecalculateTotal();
            }
            
        }
        if (currentCount >= 4) ImGui::EndDisabled();

        ImGui::SameLine();

        if (currentCount <= 0) ImGui::BeginDisabled();
        if (ImGui::Button("-")) {
            editingDeck_[cardDef->id]--;
            RecalculateTotal();
        }
        if (currentCount <= 0) ImGui::EndDisabled();

        ImGui::PopID();
    }
    ImGui::EndChild();

    // 下部の決定ボタン
    bool canSave = (totalCount_ == 40);
    if (!canSave) ImGui::BeginDisabled();

    if (ImGui::Button("Save and Go to Battle", ImVec2(-1, 40))) {
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
        RequestChangeScene_("Game");
    }

    if (!canSave) ImGui::EndDisabled();

    ImGui::End();
}

void DeckEditScene::RebuildCardModels(GameApp& app) {
    cardModels_.clear();

    // データベースにあるカードを順番に並べる（例：ID 1〜20）
    int index = 0;
    for (int i = 1; i <= 20; ++i) {
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