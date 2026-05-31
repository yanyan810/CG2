// StatusMenu.h
#pragma once
#include "Button.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include"TextSprite.h"

class GameApp;

class StatusMenu {
public:
    StatusMenu() = default;
    ~StatusMenu() = default;

    void Initialize(GameApp& app, const Vector2& basePosition);
    void SetPosition(const Vector2& pos);
    void Update(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj);
    void Draw();
    void DrawImGui();

private:
    Button parentButton_[2];
    Button poisonButton_;
    Button freezeButton_;

    bool isChildrenVisible_ = false;
    int activeDetailIndex_ = 0;

    std::unique_ptr<TextSprite> detailText_;
    std::unordered_map<int, std::wstring> descriptions_;
    int currentNewLineCount_ = 0;

private:
    void LoadDescriptions(const std::string& filePath);
};
