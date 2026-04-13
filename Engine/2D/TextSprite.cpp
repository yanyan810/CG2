#include "TextSprite.h"

#include <Windows.h>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include "DirectXTex.h"

#include "TextureManager.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"

int TextSprite::s_nextId_ = 0;


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

    textureKey_ = "__ui_text_" + std::to_string(s_nextId_++) + "__";

    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(spriteCommon_, dx_, textureKey_);
    sprite_->SetPosition(position_);
    sprite_->SetScale({ 1.0f, 1.0f, 1.0f });

    RebuildTexture_();
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
    sprite_->SetScale(size_);
    sprite_->Update(view, proj);
}

void TextSprite::Draw()
{
    if (!sprite_ || text_.empty()) {
        return;
    }

    sprite_->SetColor({ 1.0f, 1.0f, 1.0f, alpha_ });

    sprite_->Draw();
}

void TextSprite::RebuildTexture_()
{
    if (!spriteCommon_ || !dx_) {
        return;
    }

    if (text_.empty()) {
        return;
    }

    const std::wstring& drawText = text_;

    // -----------------------------
    // 1) まずフォントを作る
    // -----------------------------
    HFONT hFont = CreateFontW(
        28, 0, 0, 0,
        FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Yu Gothic UI"
    );
    if (!hFont) {
        return;
    }

    // -----------------------------
    // 2) 計測用DCで必要サイズを求める
    // -----------------------------
    const int margin = 16;
    const int minTexW = 1024;
    const int maxTexW = 1400;
    const int minTexH = 256;
    const int maxTexH = 2048;

    int texW = 1200; // とりあえず見やすい幅
    texW = (std::max)(minTexW, (std::min)(texW, maxTexW));

    HDC measureDC = CreateCompatibleDC(nullptr);
    if (!measureDC) {
        DeleteObject(hFont);
        return;
    }

    HGDIOBJ oldMeasureFont = SelectObject(measureDC, hFont);

    RECT calcRc{ margin, margin, texW - margin, 0 };
    DrawTextW(
        measureDC,
        drawText.c_str(),
        -1,
        &calcRc,
        DT_LEFT | DT_TOP | DT_WORDBREAK | DT_CALCRECT
    );

    SelectObject(measureDC, oldMeasureFont);
    DeleteDC(measureDC);

    int neededTextH = calcRc.bottom - calcRc.top;
    int texH = neededTextH + margin * 2;
    texH = (std::max)(minTexH, (std::min)(texH, maxTexH));

    // -----------------------------
    // 3) 実描画用ビットマップ作成
    // -----------------------------
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = texW;
    bmi.bmiHeader.biHeight = -texH; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pixels = nullptr;
    HDC hdc = CreateCompatibleDC(nullptr);
    if (!hdc) {
        DeleteObject(hFont);
        return;
    }

    HBITMAP hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (!hBmp || !pixels) {
        DeleteDC(hdc);
        DeleteObject(hFont);
        return;
    }

    HGDIOBJ oldBmp = SelectObject(hdc, hBmp);
    HGDIOBJ oldFont = SelectObject(hdc, hFont);

    std::memset(pixels, 0, texW * texH * 4);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));

    RECT textRc{ margin, margin, texW - margin, texH - margin };
    DrawTextW(
        hdc,
        drawText.c_str(),
        -1,
        &textRc,
        DT_LEFT | DT_TOP | DT_WORDBREAK
    );

    // -----------------------------
    // 4) 白文字 + alpha 化
    // -----------------------------
    uint8_t* p = reinterpret_cast<uint8_t*>(pixels);
    for (int i = 0; i < texW * texH; ++i) {
        uint8_t& b = p[i * 4 + 0];
        uint8_t& g = p[i * 4 + 1];
        uint8_t& r = p[i * 4 + 2];
        uint8_t& a = p[i * 4 + 3];

        uint8_t alpha = (std::max)({ r, g, b });

        r = 255;
        g = 255;
        b = 255;
        a = alpha;
    }

    std::vector<uint8_t> pixelCopy(texW * texH * 4);
    std::memcpy(pixelCopy.data(), pixels, pixelCopy.size());

    // GDI解放
    SelectObject(hdc, oldFont);
    SelectObject(hdc, oldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdc);
    DeleteObject(hFont);

    // -----------------------------
    // 5) PNGメモリ化してテクスチャ登録
    // -----------------------------
    DirectX::Image image{};
    image.width = texW;
    image.height = texH;
    image.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    image.rowPitch = texW * 4;
    image.slicePitch = image.rowPitch * texH;
    image.pixels = pixelCopy.data();

    DirectX::ScratchImage scratch;
    HRESULT hr = scratch.InitializeFromImage(image);
    if (FAILED(hr)) {
        OutputDebugStringA("[TextSprite] ScratchImage initialize failed\n");
        return;
    }

    DirectX::Blob pngBlob;
    hr = DirectX::SaveToWICMemory(
        *scratch.GetImage(0, 0, 0),
        DirectX::WIC_FLAGS_NONE,
        DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
        pngBlob
    );
    if (FAILED(hr)) {
        OutputDebugStringA("[TextSprite] SaveToWICMemory PNG failed\n");
        return;
    }

    TextureManager::GetInstance()->LoadTextureFromMemory(
        textureKey_,
        static_cast<const uint8_t*>(pngBlob.GetBufferPointer()),
        pngBlob.GetBufferSize()
    );

    if (sprite_) {
        sprite_->SetTextureFilePath(textureKey_);
        sprite_->SetScale(size_);
    }
}