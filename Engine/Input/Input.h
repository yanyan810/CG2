#pragma once

#include <dinput.h>
#include <Windows.h>
#include "WinApp.h"

class Input {
public:
    // 初期化
    void Initialize(WinApp* winApp);

    // 更新処理（毎フレーム呼び出し）
    void Update();

    // トリガー（今回押されたが前回押されていない）
    bool IsKeyTrigger(BYTE keyCode) const;

    // 押しっぱなし
    bool IsKeyPressed(BYTE keyCode) const;

    // 離した瞬間
    bool IsKeyReleased(BYTE keyCode) const;

	//マウス関連の関数を追加
    bool IsMousePressed(int button) const;
    bool IsMouseTrigger(int button) const;
    bool IsMouseReleased(int button) const;

    POINT GetMousePosition() const { return mousePos_; }
    POINT GetMouseDelta() const { return mouseDelta_; }

    void UpdateMouseDelta();

private:
    IDirectInput8* directInput_ = nullptr;
    IDirectInputDevice8* keyboardDevice_ = nullptr;

    BYTE keys_[256]{};
    BYTE prevKeys_[256]{};

	// マウス関連の変数を追加
    BYTE mouseButtons_[2]{};      // 0:左 1:右
    BYTE prevMouseButtons_[2]{};

    POINT mousePos_{};
    POINT prevMousePos_{};
    POINT mouseDelta_{};

    bool firstMouseUpdate_ = true;
    bool cameraControlEnabled_ = false;
    bool prevToggleKeyState_ = false;
    bool justEnteredCameraMode_ = false;

    WinApp* winApp_ = nullptr;
};