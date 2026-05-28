#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "StatusMenu.h"
#include "GameApp.h"
#include <fstream>
#include <codecvt>
#include <nlohmann/json.hpp> // ★JSONライブラリをインクルード

using json = nlohmann::json;

void StatusMenu::Initialize(GameApp& app, const Vector2& basePosition) {
    // 1. 親ボタン「状態異常->」の初期化
    parentButton_.Initialize(app, L"状態異常▶", "ParentStatus", basePosition, { 250.f, 50.f });

    // 2. 子ボタンの初期化（ひとまず親の下に並べる）
    // ボタンの高さ(50.f) + 隙間(5.f) を考慮して Y 座標をずらす
    Vector2 poisonPos = { basePosition.x + 30.f, basePosition.y + 55.f }; // 少し右にインデント
    poisonButton_.Initialize(app, L"毒", "SubPoison", poisonPos, { 200.f, 45.f });
    // さらにその下
    Vector2 freezePos = { basePosition.x + 30.f, basePosition.y + 105.f };
    freezeButton_.Initialize(app, L"凍結", "SubFreeze", freezePos, { 200.f, 45.f });

    // 3. 詳細テキストの初期化
    detailText_ = std::make_unique<TextSprite>();
    detailText_->Initialize(app.SpriteCom(), app.Dx());
    detailText_->SetFontSize(24);
    detailText_->SetSize({1.f, 1.f, 1.f});
    // 詳細テキストの表示位置（子ボタンのさらに右側など）
    detailText_->SetPosition({ basePosition.x + 250.f, basePosition.y + 55.f });

    LoadDescriptions("resources/cards/BadConditionDesc.json");
}

void StatusMenu::Update(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj) {
    // --- 1. 親ボタンの更新 ---
    parentButton_.Update(app, view, proj);

    // 親ボタンが押されたら子ボタンの表示/非表示を切り替える
    if (parentButton_.IsPressed()) {
        isChildrenVisible_ = !isChildrenVisible_;

        // メニューを閉じた時は詳細テキストも消す
        if (!isChildrenVisible_) {
            activeDetailIndex_ = 0;
        }
    }

    if (isChildrenVisible_) {
        // 開いているときは「状態異常▼」にする
        parentButton_.SetTextString(L"状態異常▼");
    } else {
        // 閉じているときは「状態異常->」に戻す
        parentButton_.SetTextString(L"状態異常▶");
    }

    // --- 2. 子ボタンの更新（展開時のみ） ---
    if (isChildrenVisible_) {
        Vector2 parentPos = parentButton_.GetPosition();

        // 最初の位置計算（現在の activeDetailIndex_ に基づく）
        Vector2 poisonPos = { parentPos.x + 30.f, parentPos.y + 55.f };
        poisonButton_.SetPosition(poisonPos);

        float freezeYOffset = 105.f;
        if (activeDetailIndex_ == 1) {
            freezeYOffset += 90.f;
        }
        Vector2 freezePos = { parentPos.x + 30.f, parentPos.y + freezeYOffset };
        freezeButton_.SetPosition(freezePos);

        // 各ボタンの当たり判定・内部更新
        poisonButton_.Update(app, view, proj);
        freezeButton_.Update(app, view, proj);

        // --- クリック判定 ---
        bool stateChanged = false; // ★状態が変わったかどうかのフラグ

        if (poisonButton_.IsPressed()) {
            activeDetailIndex_ = (activeDetailIndex_ == 1) ? 0 : 1;
            stateChanged = true;
        }

        if (freezeButton_.IsPressed()) {
            activeDetailIndex_ = (activeDetailIndex_ == 2) ? 0 : 2;
            stateChanged = true;
        }

        // ★追加：もしボタンが押されて状態が変わったなら、
        // 1フレーム待たずに「その場で即座に」凍結ボタンの座標を再計算して上書きする！
        if (stateChanged) {
            // カウントを一旦リセット
            currentNewLineCount_ = 0;

            if (activeDetailIndex_ != 0) {
                auto it = descriptions_.find(activeDetailIndex_);
                if (it != descriptions_.end()) {
                    const std::wstring& text = it->second;
                    // ★文字列内の L'\n' の数をカウントする
                    currentNewLineCount_ = (int)std::count(text.begin(), text.end(), L'\n');
                }
            }

            // 確定した新しい改行数で、即座に凍結ボタンの位置を再計算して上書き
            freezeYOffset = 105.f;
            if (activeDetailIndex_ == 1) {
                freezeYOffset += 45.f + (currentNewLineCount_ * 30.f);
            }
            freezePos = { parentPos.x + 30.f, parentPos.y + freezeYOffset };
            freezeButton_.SetPosition(freezePos);

            // 行列を強制再更新
            freezeButton_.Update(app, view, proj);
        }
    }

    if (activeDetailIndex_ != 0) {
        // マップから現在のID（1か2）の説明文を検索して取得
        auto it = descriptions_.find(activeDetailIndex_);
        if (it != descriptions_.end()) {
            detailText_->SetText(it->second); // ファイルから読み込んだ文章をセット
        } else {
            detailText_->SetText(L"説明がありません。");
        }

        // 表示座標の調整
        if (activeDetailIndex_ == 1) {
            Vector2 poisonPos = poisonButton_.GetPosition();
            detailText_->SetPosition({ poisonPos.x + 15.f, poisonPos.y + 50.f });
        } else if (activeDetailIndex_ == 2) {
            Vector2 freezePos = freezeButton_.GetPosition();
            detailText_->SetPosition({ freezePos.x + 15.f, freezePos.y + 50.f });
        }

        detailText_->Update(view, proj);
    }

    // --- 3. 詳細テキストの文字列更新 ---
    if (activeDetailIndex_ == 1) {
        detailText_->SetText(L"【毒状態】\n毎ターン最大HPの10%の\nダメージを受ける。");

        // 毒の下にテキストを配置（毒ボタンから少し右にずらすと見やすいです）
        Vector2 poisonPos = poisonButton_.GetPosition();
        detailText_->SetPosition({ poisonPos.x + 15.f, poisonPos.y + 35.f });

    } else if (activeDetailIndex_ == 2) {
        detailText_->SetText(L"【凍結状態】\n行動不能になる。\n物理攻撃を受けると解除。");

        // 凍結の下にテキストを配置
        Vector2 freezePos = freezeButton_.GetPosition();
        detailText_->SetPosition({ freezePos.x + 15.f, freezePos.y + 35.f });
    }

    if (activeDetailIndex_ != 0) {
        detailText_->Update(view, proj);
    }

    parentButton_.Update(app, view, proj);

}
void StatusMenu::Draw() {
    // 親ボタンは常に描画
    parentButton_.Draw();

    // 子ボタンが展開されている場合のみ描画
    if (isChildrenVisible_) {
        poisonButton_.Draw();
        freezeButton_.Draw();
    }

    if (isChildrenVisible_ && activeDetailIndex_ != 0) {
        if (detailText_) {
            detailText_->Draw();
        }
    }
}

void StatusMenu::DrawImGui() {
    if (ImGui::TreeNode("Status Menu Controller")) {
        ImGui::Checkbox("Is Children Visible", &isChildrenVisible_);
        ImGui::InputInt("Active Detail Index", &activeDetailIndex_);

        // 各ボタンのImGuiもネストして呼び出せるようにする
        parentButton_.DrawImGui();
        if (isChildrenVisible_) {
            poisonButton_.DrawImGui();
            freezeButton_.DrawImGui();
        }
        ImGui::TreePop();
    }
}

void StatusMenu::LoadDescriptions(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        // ファイルが開けなかった場合の保険
        return;
    }

    // 1. ファイルの中身を丸ごとJSONオブジェクトに読み込む（BOMも自動で処理されます）
    json j;
    try {
        file >> j;
    }
    catch (const json::parse_error& e) {
        // JSONの文法エラー（カンマの付け忘れなど）があった場合のデバッグ用
        OutputDebugStringA(e.what());
        return;
    }

    // UTF-8 から wstring への変換コンバーター
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    // 2. 配列形式のJSONをループで回してマップに格納する
    for (const auto& item : j) {
        // "id" 項目から数値を取得
        int id = item["id"].get<int>();

        // "description" 項目から文字列を取得
        std::string descriptionStr = item["description"].get<std::string>();

        // C++の TextSprite で扱えるように wstring に変換
        std::wstring wDescription = converter.from_bytes(descriptionStr);

        // マップ（descriptions_）に保存
        descriptions_[id] = wDescription;
    }
}