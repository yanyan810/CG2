#include "Input.h"
#include <cassert>
#include <cstring>

#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

void Input::Initialize(WinApp* winApp) {
    HRESULT hr;

    this->winApp_ = winApp;

    hr = DirectInput8Create(
        winApp->GetHInstance(),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8,
        (void**)&directInput_,
        nullptr
    );
    assert(SUCCEEDED(hr));

    hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboardDevice_, nullptr);
    assert(SUCCEEDED(hr));

    hr = keyboardDevice_->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(hr));

    hr = keyboardDevice_->SetCooperativeLevel(
        winApp->GetHwnd(),
        DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY
    );
    assert(SUCCEEDED(hr));

    keyboardDevice_->Acquire();
}

void Input::UpdateMouseDelta() {
    POINT currentMousePos;
    GetCursorPos(&currentMousePos);
    ScreenToClient(winApp_->GetHwnd(), &currentMousePos);

    mousePos_ = currentMousePos;

    if (firstMouseUpdate_) {
        mouseDelta_ = { 0, 0 };
        firstMouseUpdate_ = false;
    } else {
        mouseDelta_.x = currentMousePos.x - prevMousePos_.x;
        mouseDelta_.y = currentMousePos.y - prevMousePos_.y;
    }

    prevMousePos_ = currentMousePos;

    if (cameraControlEnabled_) {
        HWND hwnd = winApp_->GetHwnd();
        RECT rect;
        GetClientRect(hwnd, &rect);

        POINT center;
        center.x = (rect.right - rect.left) / 2;
        center.y = (rect.bottom - rect.top) / 2;

        POINT current;
        GetCursorPos(&current);
        ScreenToClient(hwnd, &current);

        mouseDelta_.x = current.x - center.x;
        mouseDelta_.y = current.y - center.y;

        POINT screenCenter = center;
        ClientToScreen(hwnd, &screenCenter);
        SetCursorPos(screenCenter.x, screenCenter.y);

        mousePos_ = center;
        prevMousePos_ = center;
    }
}

void Input::Update() {
    // 前フレーム保存
    memcpy(prevKeys_, keys_, sizeof(keys_));
    memcpy(prevMouseButtons_, mouseButtons_, sizeof(mouseButtons_));

    HRESULT hr = keyboardDevice_->GetDeviceState(sizeof(keys_), keys_);
    if (FAILED(hr)) {
        keyboardDevice_->Acquire();
        keyboardDevice_->GetDeviceState(sizeof(keys_), keys_);
    }

    // マウスボタン取得
    mouseButtons_[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 0x80 : 0;
    mouseButtons_[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 0x80 : 0;

    UpdateMouseDelta();

    // TABでカメラ操作切り替え
    bool toggleKey = IsKeyPressed(DIK_TAB);
    if (toggleKey && !prevToggleKeyState_) {
        cameraControlEnabled_ = !cameraControlEnabled_;
        justEnteredCameraMode_ = cameraControlEnabled_;
        ShowCursor(!cameraControlEnabled_);
    }
    prevToggleKeyState_ = toggleKey;
}

bool Input::IsKeyTrigger(BYTE keyCode) const {
    return (keys_[keyCode] & 0x80) && !(prevKeys_[keyCode] & 0x80);
}

bool Input::IsKeyPressed(BYTE keyCode) const {
    return (keys_[keyCode] & 0x80) != 0;
}

bool Input::IsKeyReleased(BYTE keyCode) const {
    return !(keys_[keyCode] & 0x80) && (prevKeys_[keyCode] & 0x80);
}

bool Input::IsMousePressed(int button) const {
    if (button < 0 || button >= 2) return false;
    return (mouseButtons_[button] & 0x80) != 0;
}

bool Input::IsMouseTrigger(int button) const {
    if (button < 0 || button >= 2) return false;
    return (mouseButtons_[button] & 0x80) && !(prevMouseButtons_[button] & 0x80);
}

bool Input::IsMouseReleased(int button) const {
    if (button < 0 || button >= 2) return false;
    return !(mouseButtons_[button] & 0x80) && (prevMouseButtons_[button] & 0x80);
}