#pragma once
#include "Button.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>


class GameApp;

class StatusMenu {
public:
    StatusMenu() = default;
    ~StatusMenu() = default;

    // 初期化（メニュー全体の基準座標などを渡す）
    void Initialize(GameApp& app, const Vector2& basePosition);

    // 更新処理（入力や状態の変更）
    void Update(GameApp& app, const Matrix4x4& view, const Matrix4x4& proj);

    // 描画処理
    void Draw();

    // デバッグ用ImGui
    void DrawImGui();

private:
    // メイン（親）ボタン
    Button parentButton_;

    // 子ボタン（毒、凍結）
    Button poisonButton_;
    Button freezeButton_;

    // 表示・選択状態を管理するフラグ
    bool isChildrenVisible_ = false; // 子ボタンを表示しているか
    int activeDetailIndex_ = 0;      // 0: なし, 1: 毒詳細, 2: 凍結詳細

    // 詳細テキスト表示用のオブジェクト（既存のTextSpriteを利用）
    std::unique_ptr<TextSprite> detailText_;

    std::unordered_map<int, std::wstring> descriptions_;

    int currentNewLineCount_ = 0;

private:

    void LoadDescriptions(const std::string& filePath);

};