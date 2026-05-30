#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include "StatusMenu.h"
#include "GameApp.h"
#include <fstream>
#include <codecvt>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void StatusMenu::Initialize(GameApp& app, const Vector2& basePosition) {
    parentButton_[0].Initialize(app, "ParentStatus", basePosition, "resources/ui/white.png", "resources/ui/text/BadCondition_1.png");
    parentButton_[0].SetBgScale({205.f,83.f});

    parentButton_[1].Initialize(app, "ParentStatus", basePosition, "resources/ui/white.png", "resources/ui/text/BadCondition_2.png");
    parentButton_[1].SetBgScale({205.f,83.f});

    Vector2 poisonPos = { basePosition.x + 30.f, basePosition.y + 1000.f };
    poisonButton_.Initialize(app, "SubPoison", poisonPos, "resources/ui/white.png", "resources/ui/text/BadConditionPoison.png");
    poisonButton_.SetBgScale({ 136.f,50.f });

    Vector2 freezePos = { basePosition.x + 30.f, basePosition.y + 1500.f };
    freezeButton_.Initialize(app, "SubFreeze", freezePos, "resources/ui/white.png", "resources/ui/text/BadConditionFrozen.png");
    freezeButton_.SetBgScale({ 136.f,50.f });

    // 205,85

    detailText_ = std::make_unique<TextSprite>();
    detailText_->Initialize(app.SpriteCom(), app.Dx());
    detailText_->SetFontSize(24);
    detailText_->SetSize({ 1.f, 1.f, 1.f });
    detailText_->SetPosition({ basePosition.x + 250.f, basePosition.y + 55.f });


    LoadDescriptions("resources/cards/BadConditionDesc.json");
}
void StatusMenu::Update(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj) {
    parentButton_[isChildrenVisible_].Update(app, view, proj);

    if (parentButton_[isChildrenVisible_].IsPressed()) {
        isChildrenVisible_ = !isChildrenVisible_;

        if (!isChildrenVisible_) {
            activeDetailIndex_ = 0;
        }
    }

    //if (isChildrenVisible_) {
    //} else {
    //}

    if (isChildrenVisible_) {
        Vector2 parentPos = parentButton_[isChildrenVisible_].GetPosition();

        Vector2 poisonPos = { parentPos.x + 30.f, parentPos.y + 80.f };
        poisonButton_.SetPosition(poisonPos);

        float freezeYOffset = 130.f;
        if (activeDetailIndex_ == 1) {
            freezeYOffset += 90.f;
        }
        Vector2 freezePos = { parentPos.x + 30.f, parentPos.y + freezeYOffset };
        freezeButton_.SetPosition(freezePos);

        poisonButton_.Update(app, view, proj);
        freezeButton_.Update(app, view, proj);

        bool stateChanged = false;

        if (poisonButton_.IsPressed()) {
            activeDetailIndex_ = (activeDetailIndex_ == 1) ? 0 : 1;
            stateChanged = true;
        }

        if (freezeButton_.IsPressed()) {
            activeDetailIndex_ = (activeDetailIndex_ == 2) ? 0 : 2;
            stateChanged = true;
        }

        if (stateChanged) {
            currentNewLineCount_ = 0;

            if (activeDetailIndex_ != 0) {
                auto it = descriptions_.find(activeDetailIndex_);
                if (it != descriptions_.end()) {
                    const std::wstring& text = it->second;
                    currentNewLineCount_ = (int)std::count(text.begin(), text.end(), L'\n');
                }
            }

            freezeYOffset = 105.f;
            if (activeDetailIndex_ == 1) {
                freezeYOffset += 35.f + (currentNewLineCount_ * 30.f);

            }
            freezePos = { parentPos.x + 30.f, parentPos.y + freezeYOffset };
            freezeButton_.SetPosition(freezePos);

            freezeButton_.Update(app, view, proj);
        }
    }

    if (activeDetailIndex_ != 0) {
        auto it = descriptions_.find(activeDetailIndex_);
        if (it != descriptions_.end()) {
            detailText_->SetText(it->second);
        } else {
            detailText_->SetText(L"\u8aac\u660e\u304c\u3042\u308a\u307e\u305b\u3093\u3002");
        }

        if (activeDetailIndex_ == 1) {
            Vector2 poisonPos = poisonButton_.GetPosition();
            detailText_->SetPosition({ poisonPos.x + 15.f, poisonPos.y + 45.f });
        } else if (activeDetailIndex_ == 2) {
            Vector2 freezePos = freezeButton_.GetPosition();
            detailText_->SetPosition({ freezePos.x + 15.f, freezePos.y + 45.f });
        }

        detailText_->Update(view, proj);
    }

   
    if (activeDetailIndex_ != 0) {
        detailText_->Update(view, proj);
    }

    parentButton_[isChildrenVisible_].Update(app, view, proj);

}
void StatusMenu::Draw() {
    parentButton_[isChildrenVisible_].Draw();

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
#ifdef USE_IMGUI
    if (ImGui::TreeNode("Status Menu Controller")) {
        ImGui::Checkbox("Is Children Visible", &isChildrenVisible_);
        ImGui::InputInt("Active Detail Index", &activeDetailIndex_);

        parentButton_[isChildrenVisible_ ? 1 : 0].DrawImGui();
        if (isChildrenVisible_) {
            poisonButton_.DrawImGui();
            freezeButton_.DrawImGui();
        }
        ImGui::TreePop();
    }
#endif
}

void StatusMenu::LoadDescriptions(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return;
    }

    json j;
    try {
        file >> j;
    }
    catch (const json::parse_error& e) {
        OutputDebugStringA(e.what());
        return;
    }

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

    for (const auto& item : j) {
        int id = item["id"].get<int>();

        std::string descriptionStr = item["description"].get<std::string>();

        std::wstring wDescription = converter.from_bytes(descriptionStr);

        descriptions_[id] = wDescription;
    }
}
