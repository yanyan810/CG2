#include "Input.h"
#include <cassert>
#include <cstring>

void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {
    HRESULT hr;

    // DirectInputの初期化
    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput_, nullptr);
    assert(SUCCEEDED(hr));

    // キーボードデバイスの作成
    hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboardDevice_, nullptr);
    assert(SUCCEEDED(hr));

    // データフォーマットを設定（標準のキーボードフォーマット）
    hr = keyboardDevice_->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(hr));

    // 協調レベルの設定
    hr = keyboardDevice_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(hr));

    // デバイスの取得開始
    keyboardDevice_->Acquire();
}

void Input::Update() {
    // 前フレームの状態を保存
    memcpy(prevKeys_, keys_, sizeof(keys_));

    // 現在のキー状態を取得
    HRESULT hr = keyboardDevice_->GetDeviceState(sizeof(keys_), keys_);
    if (FAILED(hr)) {
        // フォーカスが外れたなどで取得に失敗したら再取得
        keyboardDevice_->Acquire();
        keyboardDevice_->GetDeviceState(sizeof(keys_), keys_);
    }
}

bool Input::IsKeyTrigger(BYTE keyCode) const {
    return keys_[keyCode] && !prevKeys_[keyCode];
}

bool Input::IsKeyPressed(BYTE keyCode) const {
    return keys_[keyCode];
}

bool Input::IsKeyReleased(BYTE keyCode) const {
    return !keys_[keyCode] && prevKeys_[keyCode];
}
