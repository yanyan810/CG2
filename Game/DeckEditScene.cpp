#include "DeckEditScene.h"
#include "GameApp.h"
 // カード情報参照用
#include <imgui.h>
#include <algorithm>

#include"CardInstance.h"
#include"Card3D.h"

void DeckEditScene::OnEnter(GameApp& app) {
    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate({ 0.0f, 4.0f, -40.0f });
    camera_->SetRotate({ 0.15f, 0.0f, 0.0f });
    app.ObjCom()->SetDefaultCamera(camera_.get());

    // 必要変数の初期化
    totalCount_ = 0;
    editingDeck_.clear();

    // カードリスト読み込み
    db_.LoadFromJson("resources/cards/cards.json");

    // 1. GameAppから CardInstance型でデッキを取得
    const auto& currentInstances = app.GetDeckInstances();

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

    ImGui::Separator();

    // カードリストをもとに表示
    ImGui::BeginChild("CardList", ImVec2(0, -50), true);

    // 仮にID 1〜100までループ（実際はCardDatabaseの中身に合わせて調整）
    for (int i = 1; i <= 20; ++i) {
        auto cardDef = db_.Find(i);
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
        const CardDef* def = db_.Find(i);
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