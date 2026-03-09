#include "TextSprite.h"

#include <Windows.h>
#include <vector>
#include <string>
#include <cstring>

#include "TextureManager.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"

namespace {

    // UTF16LE + BOM の .txt をメモリ上に作る
    std::vector<uint8_t> MakeUtf16TextFileBytes(const std::wstring& text)
    {
        std::vector<uint8_t> bytes;

        // BOM
        bytes.push_back(0xFF);
        bytes.push_back(0xFE);

        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(text.data());
        bytes.insert(bytes.end(), ptr, ptr + text.size() * sizeof(wchar_t));
        return bytes;
    }

}

void TextSprite::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dx)
{
    spriteCommon_ = spriteCommon;
    dx_ = dx;

    RebuildTexture_();

    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(spriteCommon_, dx_, textureKey_);
    sprite_->SetPosition(position_);
    sprite_->SetScale({ 1.0f, 1.0f, 1.0f });
}

void TextSprite::SetText(const std::wstring& text)
{
    if (text_ == text) {
        return;
    }
    text_ = text;
    RebuildTexture_();
}

void TextSprite::Update(const Matrix4x4& view, const Matrix4x4& proj)
{
    if (!sprite_ || text_.empty()) {
        return;
    }

    sprite_->SetPosition(position_);
    sprite_->SetScale({ 1.0f, 1.0f, 1.0f });
    sprite_->Update(view, proj);
}

void TextSprite::Draw()
{
    if (!sprite_ || text_.empty()) {
        return;
    }

    sprite_->Draw();
}

void TextSprite::RebuildTexture_()
{
    if (!spriteCommon_ || !dx_) {
        return;
    }

    // 空文字なら再生成しない
    if (text_.empty()) {
        return;
    }

    const std::wstring& drawText = text_;

    // --- GDI で文字を描画 ---
    const int texW = 1024;
    const int texH = 128;

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = texW;
    bmi.bmiHeader.biHeight = -texH; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pixels = nullptr;
    HDC hdc = CreateCompatibleDC(nullptr);
    HBITMAP hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pixels, nullptr, 0);
    HGDIOBJ oldBmp = SelectObject(hdc, hBmp);

    // 背景を透明相当にするため、まず全部0クリア
    // BGRA = 0,0,0,0
    std::memset(pixels, 0, texW * texH * 4);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));

    HFONT hFont = CreateFontW(
        36, 0, 0, 0,
        FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Yu Gothic UI"
    );
    HGDIOBJ oldFont = SelectObject(hdc, hFont);

    RECT textRc{ 16, 16, texW - 16, texH - 16 };
    DrawTextW(hdc, drawText.c_str(), -1, &textRc,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    BITMAPFILEHEADER bfh{};
    bfh.bfType = 0x4D42; // 'BM'
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + texW * texH * 4;

    std::vector<uint8_t> bmpBytes(bfh.bfSize);
    std::memcpy(bmpBytes.data(), &bfh, sizeof(bfh));
    std::memcpy(bmpBytes.data() + sizeof(bfh), &bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
    std::memcpy(bmpBytes.data() + bfh.bfOffBits, pixels, texW * texH * 4);

    SelectObject(hdc, oldFont);
    DeleteObject(hFont);

    SelectObject(hdc, oldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdc);

    TextureManager::GetInstance()->LoadTextureFromMemory(
        textureKey_,
        bmpBytes.data(),
        bmpBytes.size()
    );

    if (TextureManager::GetInstance()->HasTexture(textureKey_)) {
        OutputDebugStringA(("[TextSprite] register success: " + textureKey_ + "\n").c_str());
    } else {
        OutputDebugStringA(("[TextSprite] register failed: " + textureKey_ + "\n").c_str());
    }

    if (sprite_) {
        sprite_->SetTextureFilePath(textureKey_);
        sprite_->SetScale(size_);
    }
}