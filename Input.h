#pragma once

#include <dinput.h>
#include <Windows.h>

class Input {
public:
    // 初期化
    void Initialize(HINSTANCE hInstance, HWND hwnd);

    // 更新処理（毎フレーム呼び出し）
    void Update();

    // トリガー（今回押されたが前回押されていない）
    bool IsKeyTrigger(BYTE keyCode) const;

    // 押しっぱなし
    bool IsKeyPressed(BYTE keyCode) const;

    // 離した瞬間
    bool IsKeyReleased(BYTE keyCode) const;

private:
    IDirectInput8* directInput_ = nullptr;
    IDirectInputDevice8* keyboardDevice_ = nullptr;
    BYTE keys_[256]{};
    BYTE prevKeys_[256]{};
};
