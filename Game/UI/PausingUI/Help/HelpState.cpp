#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "HelpState.h"
#include "Sprite.h"
#include "GameApp.h"
#include "../PausingUI.h"
#include "AudioManager.h"
#include "../Selection/SelectionState.h"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include "TextureManager.h"
#include <codecvt>
#include <filesystem>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

void HelpState::Initialize(GameApp& app) {
    LoadLayout_();

    backButton_ = std::make_unique<Button>();
    backButton_->Initialize(app, "BackButton", { layout_.backButtonRect.x, layout_.backButtonRect.y }, "resources/ui/white.png", "resources/ui/text/return.png");
    backButton_->SetFrameSize({ layout_.backButtonFrameSize.x, layout_.backButtonFrameSize.y });
    backButton_->SetBgSize({ layout_.backButtonRect.w, layout_.backButtonRect.h });
    backButton_->SetNormalColor({ 0.1f, 0.1f, 0.1f, 0.9f });
    backButton_->SetHoverColor({ 0.4f, 0.4f, 0.4f, 0.9f });

    // 選択可能な画像リストを構築（初回の1回だけスキャンする）
    static std::vector<std::string> s_cachedImages;
    // Scrollbar UI elements will be initialized after image scan
    if (s_cachedImages.empty()) {
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator("resources")) {
                if (entry.is_regular_file() && entry.path().extension() == ".png") {
                    std::string path = entry.path().string();
                    std::replace(path.begin(), path.end(), '\\', '/');
                    s_cachedImages.push_back(path);
                }
            }
        } catch (...) {}
    }
    availableImages_ = s_cachedImages;

    // Initialize scrollbar sprites (using white texture)
    scrollBarBg_ = std::make_unique<Sprite>();
    scrollBarBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    scrollBarBg_->SetColor({ 0.2f, 0.2f, 0.2f, 0.5f });

    scrollBarHandle_ = std::make_unique<Sprite>();
    scrollBarHandle_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    scrollBarHandle_->SetColor({ 0.8f, 0.8f, 0.8f, 0.8f });

    // JSONからヘルプ項目を読み込む
    std::ifstream ifs("resources/ui/help_items.json");
    if (ifs.is_open()) {
        try {
            nlohmann::json j;
            ifs >> j;
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            
            for (const auto& item : j) {
                std::wstring title = converter.from_bytes(item.value("title", ""));
                std::wstring text = converter.from_bytes(item.value("text", ""));
                std::string imagePath = item.value("imagePath", "resources/ui/Help.png");
                Vector2 imagePos = { item.value("imagePosX", 640.0f), item.value("imagePosY", 360.0f) };
                Vector2 imageScale = { item.value("imageScaleX", 1.0f), item.value("imageScaleY", 1.0f) };
                helpItems_.push_back({ title, text, imagePath, imagePos, imageScale });
            }
        } catch (const nlohmann::json::parse_error& e) {
            OutputDebugStringA(e.what());
        }
    }
    
    // 読み込み失敗時はデフォルト設定
    if (helpItems_.empty()) {
        helpItems_ = {
            { L"エラー", L"help_items.json の読み込みに失敗しました。", "resources/ui/Help.png" }
        };
    }

    float startY = layout_.itemButtonStart.y;
    for (size_t i = 0; i < helpItems_.size(); ++i) {
        auto btn = std::make_unique<Button>();
        btn->Initialize(app, "ItemBtn_" + std::to_string(i), { layout_.itemButtonStart.x, startY + i * layout_.itemButtonStepY }, "resources/ui/white.png");
        btn->SetFrameSize({ layout_.itemButtonBgScale.x, layout_.itemButtonBgScale.y }); // Keep frame size matching background for now
        btn->SetBgSize({ layout_.itemButtonBgScale.x, layout_.itemButtonBgScale.y });
        btn->SetNormalColor({ 0.2f, 0.2f, 0.2f, 0.9f });
        btn->SetHoverColor({ 0.5f, 0.5f, 0.5f, 0.9f });
        btn->SetFrameColor({ 1.0f, 1.0f, 1.0f, 0.0f }); // Hide the default frame.png
        itemButtons_.push_back(std::move(btn));

        auto txt = std::make_unique<TextSprite>();
        txt->Initialize(app.SpriteCom(), app.Dx());
        txt->SetFontSize(static_cast<uint32_t>(layout_.itemButtonTextSize));
        txt->SetText(helpItems_[i].title);
        txt->SetPosition({ layout_.itemButtonStart.x + layout_.itemButtonTextOffset.x, startY + i * layout_.itemButtonStepY + layout_.itemButtonTextOffset.y });
        txt->SetSize({ 1.0f, 1.0f, 1.0f });
        itemTexts_.push_back(std::move(txt));
    }

    float totalHeight = helpItems_.size() * layout_.itemButtonStepY;
    maxScrollY_ = std::max(0.0f, totalHeight - 500.0f);
    scrollY_ = 0.0f;

    TextureManager::GetInstance()->LoadTexture(helpItems_[0].imagePath);

    photoBg_ = std::make_unique<Sprite>();
    photoBg_->Initialize(app.SpriteCom(), app.Dx(), helpItems_[0].imagePath);
    // 中央揃えにするためアンカーを(0.5, 0.5)にする
    photoBg_->SetAnchorPoint({ 0.5f, 0.5f });
    photoBg_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); 
    // 画像全体のUVに戻す
    const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(helpItems_[0].imagePath);
    photoBg_->SetTextureTopLeft({0.0f, 0.0f});
    photoBg_->SetTextureCutSize({ static_cast<float>(metadata.width), static_cast<float>(metadata.height) });

    textBg_ = std::make_unique<Sprite>();
    textBg_->Initialize(app.SpriteCom(), app.Dx(), "resources/ui/white.png");
    textBg_->SetAnchorPoint({ 0.0f, 0.0f });
    textBg_->SetPosition({ layout_.textBgRect.x, layout_.textBgRect.y });
    textBg_->SetScale({ layout_.textBgRect.w, layout_.textBgRect.h, 1.0f });
    textBg_->SetColor({ 0.1f, 0.1f, 0.1f, 0.9f });

    descText_ = std::make_unique<TextSprite>();
    descText_->Initialize(app.SpriteCom(), app.Dx());
    descText_->SetFontSize(static_cast<uint32_t>(layout_.descTextSize));
    descText_->SetPosition({ layout_.textBgRect.x + layout_.descTextOffset.x, layout_.textBgRect.y + layout_.descTextOffset.y });
    descText_->SetSize({ 1.0f, 1.0f, 1.0f });
    descText_->SetText(helpItems_[0].text);
    
    ApplyLayout_();
}

void HelpState::Update(PausingUI* context, GameApp& app, Input* input) {
    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);

    // スクロール処理（ホイール）
    int wheel = input->GetWheel();
    if (wheel != 0) {
        float scrollSpeed = 0.5f;
        scrollY_ -= (static_cast<float>(wheel) / 120.0f) * 60.0f * scrollSpeed;
        scrollY_ = std::clamp(scrollY_, 0.0f, maxScrollY_);
    }

    // クリックでスクロールバーを操作
    // UI座標系はピクセルベースで左上が (0,0)
    POINT mousePos = input->GetMousePosition();
    float barX = layout_.itemButtonStart.x + layout_.itemButtonBgScale.x + 5.0f;
    const float trackStartY = 100.0f;
    const float trackHeight = 550.0f; // 650 - 100
    // トラック領域にマウスがあるか判定
    bool overTrack = (mousePos.x >= barX && mousePos.x <= barX + 5.0f &&
                     mousePos.y >= trackStartY && mousePos.y <= trackStartY + trackHeight);
    
    // Start dragging when mouse is pressed on track
    if (overTrack && input->IsMousePressed(0) && !isScrolling_) {
        isScrolling_ = true;
        scrollStartY_ = static_cast<float>(mousePos.y);
        scrollStartPos_ = scrollY_;
    }

    // End dragging when mouse button is released
    if (isScrolling_ && input->IsMouseReleased(0)) {
        isScrolling_ = false;
    }

    // While dragging, update scroll position based on mouse movement
    if (isScrolling_) {
        float deltaY = static_cast<float>(mousePos.y) - scrollStartY_;
        float perc = deltaY / trackHeight;
        scrollY_ = std::clamp(scrollStartPos_ + perc * maxScrollY_, 0.0f, maxScrollY_);
    }

    // ボタンの位置をスクロールに合わせて更新
    float startY = layout_.itemButtonStart.y;
    for (size_t i = 0; i < itemButtons_.size(); ++i) {
        float yPos = startY + i * layout_.itemButtonStepY - scrollY_;
        itemButtons_[i]->SetPosition({ layout_.itemButtonStart.x, yPos });
        itemButtons_[i]->Update(app, view, proj);

        itemTexts_[i]->SetPosition({ layout_.itemButtonStart.x + layout_.itemButtonTextOffset.x, yPos + layout_.itemButtonTextOffset.y });
        itemTexts_[i]->Update(view, proj);

        // 画面外のボタンは押せないようにする（簡易判定）
        if (yPos > 100.0f && yPos < 650.0f) {
            if (itemButtons_[i]->IsMouseOver() && selectedIndex_ != static_cast<int>(i)) {
                selectedIndex_ = static_cast<int>(i);
                descText_->SetText(helpItems_[i].text);
                
                // 画像を更新
                TextureManager::GetInstance()->LoadTexture(helpItems_[i].imagePath);
                photoBg_->SetTextureFilePath(helpItems_[i].imagePath);
                
                // 画像全体のUVに戻す
                const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(helpItems_[i].imagePath);
                photoBg_->SetTextureTopLeft({0.0f, 0.0f});
                photoBg_->SetTextureCutSize({ static_cast<float>(metadata.width), static_cast<float>(metadata.height) });
                
                ApplyLayout_(); // 画像や設定が変わったのでスケールを再計算
            }
        }
    }

    backButton_->Update(app, view, proj);
    if (backButton_->IsPressed()) {
        AudioManager::GetInstance()->PlaySE("SE_Pop");
        context->ChangeState(std::make_unique<SelectionState>(context->IsTutorialExitMode()), app);
        return;
    }

    photoBg_->Update(view, proj);
    textBg_->Update(view, proj);
    descText_->Update(view, proj);
}

void HelpState::Draw(GameApp& app) {
    // Prepare view and projection matrices for UI drawing
    Matrix4x4 view = Matrix4x4::MakeIdentity4x4();
    Matrix4x4 proj = Matrix4x4::MakeOrthographicMatrix(0, 0, (float)WinApp::kClientWidth, (float)WinApp::kClientHeight, 0, 100);
    for (size_t i = 0; i < itemButtons_.size(); ++i) {
        float yPos = itemButtons_[i]->GetPosition().y;
        if (yPos > 100.0f && yPos < 650.0f) {
            itemButtons_[i]->Draw();
            itemTexts_[i]->Draw();
        }
    }

    backButton_->Draw();
    photoBg_->Draw();
    textBg_->Draw();
    descText_->Draw();

    // Draw scroll bar (track and knob)
    // Define visual area for scrolling (same as item button area)
    const float trackStartY = 100.0f;
    const float trackEndY = 650.0f;
    const float trackHeight = trackEndY - trackStartY;
    // Compute total height of items
    float totalHeight = static_cast<float>(helpItems_.size()) * layout_.itemButtonStepY;
    // Visible portion height (fixed to 500 as used in maxScrollY calculation)
    const float visibleHeight = 500.0f;
    // Proportion of visible area to total height
    float proportion = (totalHeight > 0.0f) ? std::min(1.0f, visibleHeight / totalHeight) : 1.0f;
    float handleHeight = trackHeight * proportion;
    float handlePosY = trackStartY;
    if (maxScrollY_ > 0.0f) {
        handlePosY += (scrollY_ / maxScrollY_) * (trackHeight - handleHeight);
    }
    // Position the bar on the right side of the item list
    float barX = layout_.itemButtonStart.x + layout_.itemButtonBgScale.x + 5.0f;
    scrollBarBg_->SetPosition({ barX, trackStartY });
    scrollBarBg_->SetScale({ 5.0f, trackHeight, 1.0f });
    scrollBarBg_->Update(view, proj);
    scrollBarBg_->Draw();

    scrollBarHandle_->SetPosition({ barX, handlePosY });
    scrollBarHandle_->SetScale({ 5.0f, handleHeight, 1.0f });
    scrollBarHandle_->Update(view, proj);
    scrollBarHandle_->Draw();
}

void HelpState::ApplyLayout_() {
    backButton_->SetPosition({ layout_.backButtonRect.x, layout_.backButtonRect.y });
    backButton_->SetBgSize({ layout_.backButtonRect.w, layout_.backButtonRect.h });
    backButton_->SetFrameSize({ layout_.backButtonFrameSize.x, layout_.backButtonFrameSize.y });
    backButton_->SetFrameOffset({ layout_.backButtonFrameOffset.x, layout_.backButtonFrameOffset.y });

    // 指定された位置とスケールを適用
    photoBg_->SetPosition(helpItems_[selectedIndex_].imagePos);
    Vector2 pTexSize = photoBg_->GetTextureCutSize();
    if (pTexSize.x > 0 && pTexSize.y > 0) {
        // 元のテクスチャサイズに対する倍率をそのまま掛ける
        photoBg_->SetScale({ helpItems_[selectedIndex_].imageScale.x, helpItems_[selectedIndex_].imageScale.y, 1.0f });
    }

    textBg_->SetPosition({ layout_.textBgRect.x, layout_.textBgRect.y });
    Vector2 tTexSize = textBg_->GetTextureCutSize();
    if (tTexSize.x > 0 && tTexSize.y > 0) {
        textBg_->SetScale({ layout_.textBgRect.w / tTexSize.x, layout_.textBgRect.h / tTexSize.y, 1.0f });
    }

    descText_->SetPosition({ layout_.textBgRect.x + layout_.descTextOffset.x, layout_.textBgRect.y + layout_.descTextOffset.y });
    descText_->SetFontSize(static_cast<uint32_t>(layout_.descTextSize));

    for (size_t i = 0; i < itemButtons_.size(); ++i) {
        itemButtons_[i]->SetBgSize({ layout_.itemButtonBgScale.x, layout_.itemButtonBgScale.y });
        itemButtons_[i]->SetFrameSize({ layout_.itemButtonBgScale.x, layout_.itemButtonBgScale.y });
        itemTexts_[i]->SetFontSize(static_cast<uint32_t>(layout_.itemButtonTextSize));
    }
    
    float totalHeight = helpItems_.size() * layout_.itemButtonStepY;
    maxScrollY_ = std::max(0.0f, totalHeight - 500.0f);
    scrollY_ = std::clamp(scrollY_, 0.0f, maxScrollY_);
}

void HelpState::SaveLayout_() const {
    nlohmann::json j;
    j["backButtonRect"] = { {"x", layout_.backButtonRect.x}, {"y", layout_.backButtonRect.y}, {"w", layout_.backButtonRect.w}, {"h", layout_.backButtonRect.h} };
    j["backButtonFrameSize"] = { {"x", layout_.backButtonFrameSize.x}, {"y", layout_.backButtonFrameSize.y} };
    j["backButtonFrameOffset"] = { {"x", layout_.backButtonFrameOffset.x}, {"y", layout_.backButtonFrameOffset.y} };
    j["itemButtonStart"] = { {"x", layout_.itemButtonStart.x}, {"y", layout_.itemButtonStart.y} };
    j["itemButtonStepY"] = layout_.itemButtonStepY;
    j["itemButtonBgScale"] = { {"x", layout_.itemButtonBgScale.x}, {"y", layout_.itemButtonBgScale.y} };
    j["itemButtonTextOffset"] = { {"x", layout_.itemButtonTextOffset.x}, {"y", layout_.itemButtonTextOffset.y} };
    j["itemButtonTextSize"] = layout_.itemButtonTextSize;
    j["photoBgRect"] = { {"x", layout_.photoBgRect.x}, {"y", layout_.photoBgRect.y}, {"w", layout_.photoBgRect.w}, {"h", layout_.photoBgRect.h} };
    j["textBgRect"] = { {"x", layout_.textBgRect.x}, {"y", layout_.textBgRect.y}, {"w", layout_.textBgRect.w}, {"h", layout_.textBgRect.h} };
    j["descTextOffset"] = { {"x", layout_.descTextOffset.x}, {"y", layout_.descTextOffset.y} };
    j["descTextSize"] = layout_.descTextSize;

    std::ofstream ofs(layoutPath_);
    if (ofs.is_open()) {
        ofs << j.dump(4);
    }
}

void HelpState::LoadLayout_() {
    std::ifstream ifs(layoutPath_);
    if (!ifs.is_open()) return;

    nlohmann::json j;
    ifs >> j;

    if (j.contains("backButtonRect")) {
        layout_.backButtonRect.x = j["backButtonRect"].value("x", layout_.backButtonRect.x);
        layout_.backButtonRect.y = j["backButtonRect"].value("y", layout_.backButtonRect.y);
        layout_.backButtonRect.w = j["backButtonRect"].value("w", layout_.backButtonRect.w);
        layout_.backButtonRect.h = j["backButtonRect"].value("h", layout_.backButtonRect.h);
    }
    if (j.contains("backButtonFrameSize")) {
        layout_.backButtonFrameSize.x = j["backButtonFrameSize"].value("x", layout_.backButtonFrameSize.x);
        layout_.backButtonFrameSize.y = j["backButtonFrameSize"].value("y", layout_.backButtonFrameSize.y);
    }
    if (j.contains("backButtonFrameOffset")) {
        layout_.backButtonFrameOffset.x = j["backButtonFrameOffset"].value("x", layout_.backButtonFrameOffset.x);
        layout_.backButtonFrameOffset.y = j["backButtonFrameOffset"].value("y", layout_.backButtonFrameOffset.y);
    }
    if (j.contains("itemButtonStart")) {
        layout_.itemButtonStart.x = j["itemButtonStart"].value("x", layout_.itemButtonStart.x);
        layout_.itemButtonStart.y = j["itemButtonStart"].value("y", layout_.itemButtonStart.y);
    }
    layout_.itemButtonStepY = j.value("itemButtonStepY", layout_.itemButtonStepY);
    if (j.contains("itemButtonBgScale")) {
        layout_.itemButtonBgScale.x = j["itemButtonBgScale"].value("x", layout_.itemButtonBgScale.x);
        layout_.itemButtonBgScale.y = j["itemButtonBgScale"].value("y", layout_.itemButtonBgScale.y);
    }
    if (j.contains("itemButtonTextOffset")) {
        layout_.itemButtonTextOffset.x = j["itemButtonTextOffset"].value("x", layout_.itemButtonTextOffset.x);
        layout_.itemButtonTextOffset.y = j["itemButtonTextOffset"].value("y", layout_.itemButtonTextOffset.y);
    }
    layout_.itemButtonTextSize = j.value("itemButtonTextSize", layout_.itemButtonTextSize);
    if (j.contains("photoBgRect")) {
        layout_.photoBgRect.x = j["photoBgRect"].value("x", layout_.photoBgRect.x);
        layout_.photoBgRect.y = j["photoBgRect"].value("y", layout_.photoBgRect.y);
        layout_.photoBgRect.w = j["photoBgRect"].value("w", layout_.photoBgRect.w);
        layout_.photoBgRect.h = j["photoBgRect"].value("h", layout_.photoBgRect.h);
    }
    if (j.contains("textBgRect")) {
        layout_.textBgRect.x = j["textBgRect"].value("x", layout_.textBgRect.x);
        layout_.textBgRect.y = j["textBgRect"].value("y", layout_.textBgRect.y);
        layout_.textBgRect.w = j["textBgRect"].value("w", layout_.textBgRect.w);
        layout_.textBgRect.h = j["textBgRect"].value("h", layout_.textBgRect.h);
    }
    if (j.contains("descTextOffset")) {
        layout_.descTextOffset.x = j["descTextOffset"].value("x", layout_.descTextOffset.x);
        layout_.descTextOffset.y = j["descTextOffset"].value("y", layout_.descTextOffset.y);
    }
    layout_.descTextSize = j.value("descTextSize", layout_.descTextSize);
}

void HelpState::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::Begin("Help UI Layout");
    bool changed = false;

    if (ImGui::CollapsingHeader("Back Button")) {
        changed |= ImGui::DragFloat2("Pos", &layout_.backButtonRect.x);
        changed |= ImGui::DragFloat2("Bg Size", &layout_.backButtonRect.w);
        changed |= ImGui::DragFloat2("Frame Size", &layout_.backButtonFrameSize.x);
        changed |= ImGui::DragFloat2("Frame Offset", &layout_.backButtonFrameOffset.x);
    }
    if (ImGui::CollapsingHeader("Item Buttons")) {
        changed |= ImGui::DragFloat2("Start Pos", &layout_.itemButtonStart.x);
        changed |= ImGui::DragFloat("Step Y", &layout_.itemButtonStepY);
        changed |= ImGui::DragFloat2("Bg Size", &layout_.itemButtonBgScale.x);
        changed |= ImGui::DragFloat2("Text Offset", &layout_.itemButtonTextOffset.x);
        changed |= ImGui::DragFloat("Text Size", &layout_.itemButtonTextSize);
    }
    if (ImGui::CollapsingHeader("Photo Background")) {
        changed |= ImGui::DragFloat2("Photo Pos", &layout_.photoBgRect.x);
        changed |= ImGui::DragFloat2("Photo Size", &layout_.photoBgRect.w);
    }
    if (ImGui::CollapsingHeader("Text Background")) {
        changed |= ImGui::DragFloat2("Text Bg Pos", &layout_.textBgRect.x);
        changed |= ImGui::DragFloat2("Text Bg Size", &layout_.textBgRect.w);
        changed |= ImGui::DragFloat2("Text Offset", &layout_.descTextOffset.x);
        changed |= ImGui::DragFloat("Desc Text Size", &layout_.descTextSize);
    }

    if (ImGui::CollapsingHeader("Selected Help Item Image Adjust", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (selectedIndex_ >= 0 && selectedIndex_ < helpItems_.size()) {
            ImGui::Text("Item: %ws", helpItems_[selectedIndex_].title.c_str());
            
            bool itemChanged = false;
            
            // ドロップダウンでの選択
            if (ImGui::BeginCombo("Select Image", helpItems_[selectedIndex_].imagePath.c_str())) {
                for (const auto& imgPath : availableImages_) {
                    bool isSelected = (helpItems_[selectedIndex_].imagePath == imgPath);
                    if (ImGui::Selectable(imgPath.c_str(), isSelected)) {
                        helpItems_[selectedIndex_].imagePath = imgPath;
                        itemChanged = true;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            char imagePathBuf[256];
            strncpy_s(imagePathBuf, sizeof(imagePathBuf), helpItems_[selectedIndex_].imagePath.c_str(), _TRUNCATE);
            if (ImGui::InputText("Image Path", imagePathBuf, sizeof(imagePathBuf))) {
                helpItems_[selectedIndex_].imagePath = imagePathBuf;
                itemChanged = true;
            }

            itemChanged |= ImGui::DragFloat2("Image Pos", &helpItems_[selectedIndex_].imagePos.x, 1.0f);
            itemChanged |= ImGui::DragFloat2("Image Scale", &helpItems_[selectedIndex_].imageScale.x, 0.01f);
            
            if (itemChanged) {
                TextureManager::GetInstance()->LoadTexture(helpItems_[selectedIndex_].imagePath);
                photoBg_->SetTextureFilePath(helpItems_[selectedIndex_].imagePath);

                const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(helpItems_[selectedIndex_].imagePath);
                photoBg_->SetTextureTopLeft({0.0f, 0.0f});
                photoBg_->SetTextureCutSize({ static_cast<float>(metadata.width), static_cast<float>(metadata.height) });
                
                changed = true; // ApplyLayout_を呼ぶため
            }
            
            if (ImGui::Button("Save Help Items JSON")) {
                SaveHelpItems_();
            }
        }
    }

    if (changed) {
        ApplyLayout_();
    }

    if (ImGui::Button("Save Layout")) {
        SaveLayout_();
    }
    ImGui::End();
#endif
}

void HelpState::SaveHelpItems_() const {
    nlohmann::json j = nlohmann::json::array();
    
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    
    for (const auto& item : helpItems_) {
        nlohmann::json itemJson;
        itemJson["title"] = converter.to_bytes(item.title);
        itemJson["text"] = converter.to_bytes(item.text);
        itemJson["imagePath"] = item.imagePath;
        
        itemJson["imagePosX"] = item.imagePos.x;
        itemJson["imagePosY"] = item.imagePos.y;
        itemJson["imageScaleX"] = item.imageScale.x;
        itemJson["imageScaleY"] = item.imageScale.y;
        
        j.push_back(itemJson);
    }
    
    std::ofstream ofs("resources/ui/help_items.json");
    if (ofs.is_open()) {
        ofs << j.dump(4);
    }
}
