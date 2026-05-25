#include "Button.h"
#include "GameApp.h"
#include "imgui.h"

void Button::Initialize(GameApp& app, const std::wstring& text, const std::string& name, const Vector2& position, const Vector2& scale) {
    name_ = name;
    isPressed_ = false;

    // メンバ変数への初期値キャッシュ（ImGuiで編集可能にするため）
    position_ = position;
    textOffset_ = { 55.f, 0.f };
    normalColor_ = { 0.3f, 0.3f, 0.3f, 1.0f };
    hoverColor_ = { 0.2f, 0.5f, 0.4f, 1.0f };
    textString_ = text;

    // 背景
    bg_ = std::make_unique<Sprite>();
    bg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    bg_->SetName(name);
    bg_->SetPosition({ position_.x, position_.y });
    bg_->SetScale({ scale.x, scale.y, 1.0f });
    bg_->SetColor(normalColor_);

    // テキスト
    text_ = std::make_unique<TextSprite>();
    text_->Initialize(app.SpriteCom(), app.Dx());
    text_->SetFontSize(30);
    text_->SetSize({ 1.f, 1.f, 1.f });
    text_->SetPosition({ position_.x + textOffset_.x, position_.y + textOffset_.y });
    text_->SetText(textString_);
}

void Button::Update(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj) {
    isPressed_ = false; // 毎フレームリセット
    Input* input = app.GetInput();
    POINT mouse = input->GetMousePosition();
    Vector2 mousePos = { (float)mouse.x, (float)mouse.y };

    // ImGuiからの編集を反映するために毎フレーム座標を再設定
    if (bg_) {
        bg_->SetPosition(position_);
    }
    if (text_) {
        text_->SetPosition({ position_.x + textOffset_.x, position_.y + textOffset_.y });
    }

    // マウスホバー・クリック判定
    if (bg_ && bg_->IsMouseOver(mousePos)) {
        // マウスホバー時のエフェクト
        bg_->SetColor(hoverColor_);

        // 左クリックされたらフラグを立てる
        if (input->IsMouseTrigger(0)) {
            isPressed_ = true;
        }
    } else {
        // 通常時の色
        if (bg_) bg_->SetColor(normalColor_);
    }

    // 行列更新
    if (bg_)   bg_->Update(view, proj);
    if (text_) text_->Update(view, proj);
}

void Button::Draw() {
    if (bg_)   bg_->Draw();
    if (text_) text_->Draw();
}

// ★ 追加：DrawImGuiの実装
void Button::DrawImGui() {
    // ボタン個別の識別名でツリーを展開
    std::string label = "Button: " + name_;
    if (ImGui::TreeNode(label.c_str())) {

        // 1. 座標の調整
        if (ImGui::DragFloat2("Position", &position_.x, 1.0f)) {
            if (bg_) bg_->SetPosition(position_);
        }

        // 2. スケールの調整
        if (bg_) {
            Vector3 scale = bg_->GetScale();
            if (ImGui::DragFloat2("Scale", &scale.x, 1.0f, 0.0f, 2000.0f)) {
                bg_->SetScale(scale);
            }
        }

        // 3. テキストオフセットの調整
        ImGui::DragFloat2("Text Offset", &textOffset_.x, 1.0f);

        // 4. カラーの調整
        ImGui::ColorEdit4("Normal Color", &normalColor_.x);
        ImGui::ColorEdit4("Hover Color", &hoverColor_.x);

        // 5. テキスト情報（閲覧・簡易変更用）
        // ※std::wstringのImGui直接編集は少し複雑になるため、現在は確認用として配置
        // 必要に応じてTextSpriteのfontSize変更などをここに生やすことも可能です
        if (text_) {
            ImGui::Text("Text: [WString Active]");
        }

        // 6. デバッグ状態表示
        ImGui::Text("IsPressed: %s", isPressed_ ? "TRUE" : "FALSE");

        ImGui::TreePop();
    }
}