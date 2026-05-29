#include "Button.h"
#include "GameApp.h"
#include "imgui.h"

void Button::Initialize(GameApp& app, const std::string& name, const Vector2& position, const Vector2& scale, const std::string& bgPath, const std::string& framePath) {
    name_ = name;
    isPressed_ = false;
    position_ = position;
    scale_ = scale;

    // 背景スプライトの生成
    bg_ = std::make_unique<Sprite>();
    bg_->Initialize(app.SpriteCom(), app.Dx(), bgPath);
    bg_->SetName(name + "_bg");
    bg_->SetPosition(position_);
    bg_->SetScale({ scale_.x, scale_.y, 1.0f });
    bg_->SetColor(normalColor_);

    // 枠（兼 文字）スプライトの生成
    frame_ = std::make_unique<Sprite>();
    frame_->Initialize(app.SpriteCom(), app.Dx(), framePath);
    frame_->SetName(name + "_frame");
    frame_->SetPosition(position_);
    frame_->SetScale({ scale_.x, scale_.y, 1.0f });
    frame_->SetColor(frameColor_);
}

void Button::SetPosition(const Vector2& position) {
    position_ = position;
    if (bg_) bg_->SetPosition(position_);
    if (frame_) frame_->SetPosition(position_);
}

void Button::SetScale(const Vector2& scale) {
    scale_ = scale;
    if (bg_) bg_->SetScale({ scale_.x, scale_.y, 1.0f });
    if (frame_) frame_->SetScale({ scale_.x, scale_.y, 1.0f });
}

void Button::SetFrameColor(const Vector4& frameColor) {
    frameColor_ = frameColor;
    if (frame_) frame_->SetColor(frameColor_);
}

void Button::Update(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj) {
    Input* input = app.GetInput();
    POINT mouse = input->GetMousePosition();

    // マウスオーバー判定 (AABB)
    isMouseOver_ = (mouse.x >= position_.x && mouse.x <= position_.x + scale_.x &&
        mouse.y >= position_.y && mouse.y <= position_.y + scale_.y);

    if (isMouseOver_) {
        if (bg_) bg_->SetColor(hoverColor_);
        isPressed_ = input->IsMouseTrigger(0);
    } else {
        if (bg_) bg_->SetColor(normalColor_);
        isPressed_ = false;
    }

    // 行列更新
    if (bg_) bg_->Update(view, proj);
    if (frame_) frame_->Update(view, proj);
}

void Button::Draw() {
    // 背景を下に、文字入りの枠を上に描画
    if (bg_) bg_->Draw();
    if (frame_) frame_->Draw();
}

void Button::DrawImGui() {
    std::string label = "Button: " + name_;
    if (ImGui::TreeNode(label.c_str())) {
        // 座標調整
        if (ImGui::DragFloat2("Position", &position_.x, 1.0f)) {
            SetPosition(position_);
        }
        // スケール調整
        if (ImGui::DragFloat2("Scale", &scale_.x, 1.0f, 0.0f, 2000.0f)) {
            SetScale(scale_);
        }
        ImGui::TreePop();
    }
}