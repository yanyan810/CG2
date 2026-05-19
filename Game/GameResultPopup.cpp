#include "GameResultPopup.h"
#include "GameApp.h"
#include "TextureManager.h"
#include "WinApp.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

using json = nlohmann::json;

// ============================================================
// Initialize
// ============================================================
void GameResultPopup::Initialize(GameApp& app) {
    // テクスチャ読み込み
    TextureManager::GetInstance()->LoadTexture("resources/ui/text/gameover.png");
    TextureManager::GetInstance()->LoadTexture("resources/ui/text/gameclear.png");
    TextureManager::GetInstance()->LoadTexture("resources/ui/text/continue.png");
    TextureManager::GetInstance()->LoadTexture("resources/ui/text/yes.png");
    TextureManager::GetInstance()->LoadTexture("resources/ui/text/no.png");
    TextureManager::GetInstance()->LoadTexture("resources/ui/text/title.png");
    TextureManager::GetInstance()->LoadTexture("resources/ui/text/select.png");
    TextureManager::GetInstance()->LoadTexture("resources/ui/white.png");

    // 半透明黒背景（white.pngを黒く塗る）
    bgSprite_ = std::make_unique<Sprite>();
    bgSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    bgSprite_->SetAnchorPoint({ 0.0f, 0.0f });
    bgSprite_->SetPosition({ 0.0f, 0.0f });
    bgSprite_->SetScale({
        static_cast<float>(WinApp::kClientWidth),
        static_cast<float>(WinApp::kClientHeight),
        1.0f
    });
    bgSprite_->SetColor({ 0.0f, 0.0f, 0.0f, layout_.bgAlpha });

    // GameOver
    gameOverSprite_ = std::make_unique<Sprite>();
    gameOverSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/gameover.png");
    gameOverSprite_->SetAnchorPoint({ 0.5f, 0.5f });

    // GameClear
    gameClearSprite_ = std::make_unique<Sprite>();
    gameClearSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/gameclear.png");
    gameClearSprite_->SetAnchorPoint({ 0.5f, 0.5f });

    // Continue
    continueSprite_ = std::make_unique<Sprite>();
    continueSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/continue.png");
    continueSprite_->SetAnchorPoint({ 0.5f, 0.5f });

    // YES
    yesSprite_ = std::make_unique<Sprite>();
    yesSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/yes.png");
    yesSprite_->SetAnchorPoint({ 0.5f, 0.5f });

    // NO
    noSprite_ = std::make_unique<Sprite>();
    noSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/no.png");
    noSprite_->SetAnchorPoint({ 0.5f, 0.5f });

    // TITLE
    titleSprite_ = std::make_unique<Sprite>();
    titleSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/title.png");
    titleSprite_->SetAnchorPoint({ 0.5f, 0.5f });

    // SELECT
    selectSprite_ = std::make_unique<Sprite>();
    selectSprite_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/text/select.png");
    selectSprite_->SetAnchorPoint({ 0.5f, 0.5f });

    LoadLayout();
}

// ============================================================
// Show / Hide
// ============================================================
void GameResultPopup::Show(ResultKind kind) {
    visible_      = true;
    kind_         = kind;
    action_       = ResultAction::None;
    hoveredButton_= -1;
    fadeAlpha_    = 0.0f;
    // GameClear は最初から Title/Select ボタン画面
    phase_ = (kind == ResultKind::GameClear) ? ResultPopupPhase::Buttons
                                              : ResultPopupPhase::Continue;
}

void GameResultPopup::Hide() {
    visible_ = false;
}

// ============================================================
// Update
// ============================================================
void GameResultPopup::Update(GameApp& app, float dt) {
    if (!visible_) { return; }

    // フェードイン
    fadeAlpha_ = std::min(fadeAlpha_ + 3.0f * dt, 1.0f);

    const Input* input = app.GetInput();
    if (!input) { return; }

    const POINT mousePos = input->GetMousePosition();
    const Vector2 mouse{
        static_cast<float>(mousePos.x),
        static_cast<float>(mousePos.y)
    };

    hoveredButton_ = -1;

    if (phase_ == ResultPopupPhase::Continue) {
        // YES / NO
        if (IsHoveredLayout_(layout_.yes, mouse)) {
            hoveredButton_ = 0;
            if (input->IsMouseTrigger(0)) {
                action_ = ResultAction::Retry;
            }
        }
        if (IsHoveredLayout_(layout_.no, mouse)) {
            hoveredButton_ = 1;
            if (input->IsMouseTrigger(0)) {
                phase_ = ResultPopupPhase::Buttons;
                hoveredButton_ = -1;
            }
        }
    } else {
        // TITLE / SELECT
        if (IsHoveredLayout_(layout_.title, mouse)) {
            hoveredButton_ = 0;
            if (input->IsMouseTrigger(0)) {
                action_ = ResultAction::GoTitle;
            }
        }
        if (IsHoveredLayout_(layout_.select, mouse)) {
            hoveredButton_ = 1;
            if (input->IsMouseTrigger(0)) {
                action_ = ResultAction::GoStageSelect;
            }
        }
    }
}

// ============================================================
// Draw2D
// ============================================================
void GameResultPopup::Draw2D(GameApp& app) {
    if (!visible_) { return; }

    app.SpriteCom()->SetGraphicsPipelineState();

    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(
        0.0f, 0.0f,
        static_cast<float>(WinApp::kClientWidth),
        static_cast<float>(WinApp::kClientHeight),
        0.0f, 100.0f
    );

    const float a = fadeAlpha_;

    // ── 背景 ──
    if (bgSprite_) {
        bgSprite_->SetColor({ 0.0f, 0.0f, 0.0f, layout_.bgAlpha * a });
        bgSprite_->Update(view, proj);
        bgSprite_->Draw();
    }

    if (phase_ == ResultPopupPhase::Continue) {
        // ── GAME OVER ──
        if (gameOverSprite_) {
            DrawSprite_(*gameOverSprite_, layout_.gameOver, view, proj, a);
        }
        // ── CONTINUE? ──
        if (continueSprite_) {
            DrawSprite_(*continueSprite_, layout_.cont, view, proj, a);
        }
        // ── YES ──
        if (yesSprite_) {
            const float boost = (hoveredButton_ == 0) ? 1.15f : 1.0f;
            DrawSprite_(*yesSprite_, layout_.yes, view, proj, a, boost);
        }
        // ── NO ──
        if (noSprite_) {
            const float boost = (hoveredButton_ == 1) ? 1.15f : 1.0f;
            DrawSprite_(*noSprite_, layout_.no, view, proj, a, boost);
        }
    } else {
        // ── GAME OVER or GAME CLEAR タイトル ──
        if (kind_ == ResultKind::GameOver && gameOverSprite_) {
            DrawSprite_(*gameOverSprite_, layout_.gameOver, view, proj, a);
        }
        if (kind_ == ResultKind::GameClear && gameClearSprite_) {
            DrawSprite_(*gameClearSprite_, layout_.gameClear, view, proj, a);
        }
        // ── TITLE ──
        if (titleSprite_) {
            const float boost = (hoveredButton_ == 0) ? 1.15f : 1.0f;
            DrawSprite_(*titleSprite_, layout_.title, view, proj, a, boost);
        }
        // ── SELECT ──
        if (selectSprite_) {
            const float boost = (hoveredButton_ == 1) ? 1.15f : 1.0f;
            DrawSprite_(*selectSprite_, layout_.select, view, proj, a, boost);
        }
    }
}

// ============================================================
// DrawImGui
// ============================================================
void GameResultPopup::DrawImGui() {
#ifdef USE_IMGUI
    if (!ImGui::Begin("Game Result Popup")) {
        ImGui::End();
        return;
    }

    ImGui::Text("Visible: %s  Phase: %s",
        visible_ ? "true" : "false",
        (phase_ == ResultPopupPhase::Continue) ? "Continue" : "Buttons");

    ImGui::Separator();
    ImGui::DragFloat("BG Alpha", &layout_.bgAlpha, 0.01f, 0.0f, 1.0f);

    auto editItem = [](const char* label, PopupSpriteLayout& item) {
        ImGui::PushID(label);
        ImGui::TextColored({ 1.0f, 0.85f, 0.3f, 1.0f }, "%s", label);
        ImGui::DragFloat2("Pos",    &item.x,      1.0f);
        ImGui::DragFloat("Width",   &item.scaleX,  1.0f, 1.0f, 2000.0f);
        ImGui::DragFloat("Height",  &item.scaleY,  1.0f, 1.0f, 2000.0f);
        ImGui::PopID();
    };

    if (ImGui::CollapsingHeader("Game Over")) {
        editItem("GameOver Title", layout_.gameOver);
        editItem("Continue",       layout_.cont);
        editItem("YES",            layout_.yes);
        editItem("NO",             layout_.no);
    }
    if (ImGui::CollapsingHeader("Game Clear")) {
        editItem("GameClear Title", layout_.gameClear);
        editItem("TITLE Button",    layout_.title);
        editItem("SELECT Button",   layout_.select);
    }
    if (ImGui::CollapsingHeader("Buttons (shared with GameOver NO phase)")) {
        editItem("TITLE Button",  layout_.title);
        editItem("SELECT Button", layout_.select);
    }

    ImGui::Separator();
    if (ImGui::Button("Save Layout")) { SaveLayout(); }
    ImGui::SameLine();
    if (ImGui::Button("Load Layout")) { LoadLayout(); }

    ImGui::Separator();
    // 判定内容をゲーム画面上に可視化
    ImGui::Checkbox("Show Hitboxes", &debugShowHitboxes_);
    ImGui::SliderFloat("Hit Scale", &layout_.hitScale, 0.1f, 2.0f);
    // 現在の判定範囲を数値で表示
    ImGui::Separator();
    ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "Hitbox sizes (px at scale x%.2f):", layout_.hitScale);
    auto showSize = [&](const char* label, const PopupSpriteLayout& item) {
        ImGui::Text("  %s: center(%.0f, %.0f)  hit(%.0f x %.0f)",
            label,
            item.x, item.y,
            item.scaleX * layout_.hitScale,
            item.scaleY * layout_.hitScale);
    };
    if (phase_ == ResultPopupPhase::Continue) {
        showSize("YES", layout_.yes);
        showSize("NO",  layout_.no);
    } else {
        showSize("TITLE",  layout_.title);
        showSize("SELECT", layout_.select);
    }

    ImGui::Separator();
    ImGui::Text("-- Debug --");
    if (ImGui::Button("Show GameOver"))  { Show(ResultKind::GameOver); }
    ImGui::SameLine();
    if (ImGui::Button("Show GameClear")) { Show(ResultKind::GameClear); }
    ImGui::SameLine();
    if (ImGui::Button("Hide"))           { Hide(); }

    ImGui::End();

    // ゲーム画面に判定矩形をオーバーレイ
    DrawHitboxOverlay_();
#endif
}

// ============================================================
// SaveLayout / LoadLayout
// ============================================================
void GameResultPopup::SaveLayout() const {
    std::filesystem::create_directories(
        std::filesystem::path(kLayoutPath).parent_path());

    auto saveItem = [](const PopupSpriteLayout& item) {
        return json::object({
            {"x",      item.x},
            {"y",      item.y},
            {"scaleX", item.scaleX},
            {"scaleY", item.scaleY}
        });
    };

    json j;
    j["bgAlpha"]   = layout_.bgAlpha;
    j["hitScale"]  = layout_.hitScale;
    j["gameOver"]  = saveItem(layout_.gameOver);
    j["gameClear"] = saveItem(layout_.gameClear);
    j["cont"]      = saveItem(layout_.cont);
    j["yes"]       = saveItem(layout_.yes);
    j["no"]        = saveItem(layout_.no);
    j["title"]     = saveItem(layout_.title);
    j["select"]    = saveItem(layout_.select);

    std::ofstream ofs(kLayoutPath);
    if (ofs.is_open()) { ofs << j.dump(4); }
}

void GameResultPopup::LoadLayout() {
    std::ifstream ifs(kLayoutPath);
    if (!ifs.is_open()) { return; }

    json j;
    ifs >> j;

    auto loadItem = [&](const char* key, PopupSpriteLayout& item) {
        if (!j.contains(key)) { return; }
        const auto& o = j[key];
        if (o.contains("x"))      item.x      = o["x"];
        if (o.contains("y"))      item.y      = o["y"];
        if (o.contains("scaleX")) item.scaleX = o["scaleX"];
        if (o.contains("scaleY")) item.scaleY = o["scaleY"];
    };

    if (j.contains("bgAlpha"))  layout_.bgAlpha  = j["bgAlpha"];
    if (j.contains("hitScale")) layout_.hitScale = j["hitScale"];
    loadItem("gameOver",  layout_.gameOver);
    loadItem("gameClear", layout_.gameClear);
    loadItem("cont",      layout_.cont);
    loadItem("yes",       layout_.yes);
    loadItem("no",        layout_.no);
    loadItem("title",     layout_.title);
    loadItem("select",    layout_.select);
}

// ============================================================
// private helpers
// ============================================================
// layout の中心座標・スケールから直接 AABB 判定
bool GameResultPopup::IsHoveredLayout_(const PopupSpriteLayout& layout,
                                       const Vector2& mouse) const {
    const float hw = layout.scaleX * layout_.hitScale * 0.5f;
    const float hh = layout.scaleY * layout_.hitScale * 0.5f;
    return mouse.x >= layout.x - hw && mouse.x <= layout.x + hw &&
           mouse.y >= layout.y - hh && mouse.y <= layout.y + hh;
}

bool GameResultPopup::IsHovered_(const Sprite& sp, const Vector2& mouse) const {
    return sp.IsMouseOver(mouse);
}

// 判定矩形をゲーム画面にオーバーレイ描画
void GameResultPopup::DrawHitboxOverlay_() const {
#ifdef USE_IMGUI
    if (!debugShowHitboxes_ || !visible_) { return; }

    auto* draw = ImGui::GetForegroundDrawList();

    // ボタン哯る：対象の layout を当たり判定矩形で表示
    auto drawBox = [&](const PopupSpriteLayout& layout,
                       ImU32 color, const char* label) {
        const float hw = layout.scaleX * layout_.hitScale * 0.5f;
        const float hh = layout.scaleY * layout_.hitScale * 0.5f;
        ImVec2 tl{ layout.x - hw, layout.y - hh };
        ImVec2 br{ layout.x + hw, layout.y + hh };
        draw->AddRect(tl, br, color, 0.0f, 0, 2.0f);
        // ラベル
        draw->AddText({ tl.x + 2, tl.y + 2 }, color, label);
        // 中心点
        draw->AddCircleFilled({ layout.x, layout.y }, 4.0f, color);
        // 小数点以下のW/H表示
        char sizeStr[64];
        snprintf(sizeStr, sizeof(sizeStr),
            "%.0fx%.0f (hit:%.0fx%.0f)",
            layout.scaleX, layout.scaleY,
            layout.scaleX * layout_.hitScale,
            layout.scaleY * layout_.hitScale);
        draw->AddText({ tl.x + 2, br.y - 16 }, color, sizeStr);
    };

    // 表示中のフェーズに応じて色分け
    if (phase_ == ResultPopupPhase::Continue) {
        drawBox(layout_.yes, IM_COL32(0, 255, 80,  220), "YES");
        drawBox(layout_.no,  IM_COL32(255, 80,  0, 220), "NO");
    } else {
        drawBox(layout_.title,  IM_COL32(100, 180, 255, 220), "TITLE");
        drawBox(layout_.select, IM_COL32(255, 200,  50, 220), "SELECT");
    }
#endif
}

void GameResultPopup::DrawSprite_(Sprite& sp,
                                   const PopupSpriteLayout& layout,
                                   const Matrix4x4& view,
                                   const Matrix4x4& proj,
                                   float alpha,
                                   float scaleBoost)
{
    // layout.scaleX/Y =「画面上のピクセル幅/高さ」として解釈
    // テクスチャの実サイズで割って SetScale に渡す
    const auto& meta = TextureManager::GetInstance()->GetMetaData(sp.GetTextureFilePath());
    const float texW = (meta.width  > 0) ? static_cast<float>(meta.width)  : 1.0f;
    const float texH = (meta.height > 0) ? static_cast<float>(meta.height) : 1.0f;

    sp.SetPosition({ layout.x, layout.y });
    sp.SetScale({
        layout.scaleX * scaleBoost / texW,
        layout.scaleY * scaleBoost / texH,
        1.0f
    });
    sp.SetColor({ 1.0f, 1.0f, 1.0f, alpha });
    sp.Update(view, proj);
    sp.Draw();
}
