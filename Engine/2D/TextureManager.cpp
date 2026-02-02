#include "TextureManager.h"
#include <cassert>
#include <filesystem>
#include <Windows.h>

TextureManager* TextureManager::instance = nullptr;
using namespace StringUtility;

static void DebugPrintA(const std::string& s) {
    OutputDebugStringA((s + "\n").c_str());
}

TextureManager* TextureManager::GetInstance()
{
    if (instance == nullptr) {
        instance = new TextureManager();
    }
    return instance;
}

void TextureManager::Finalize()
{
    delete instance;
    instance = nullptr;
}

void TextureManager::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager)
{
    dx_ = dxCommon;
    srvManager_ = srvManager;

    // ---- 1x1 白テクスチャを必ず作る（filePath="" のとき用）----
    {
        DirectX::ScratchImage whiteImg{};
        HRESULT hr = whiteImg.Initialize2D(
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            1, 1, 1, 1
        );
        assert(SUCCEEDED(hr));

        auto* img = whiteImg.GetImage(0, 0, 0);
        assert(img && img->pixels && img->rowPitch >= 4);

        img->pixels[0] = 255; // R
        img->pixels[1] = 255; // G
        img->pixels[2] = 255; // B
        img->pixels[3] = 255; // A

        TextureData& tex = textureDatas_[kWhiteKey];
        tex.metadata = whiteImg.GetMetadata();
        tex.resource = dx_->CreateTextureResource(tex.metadata);
        dx_->UploadTextureData(tex.resource, whiteImg);

        tex.srvIndex = srvManager_->Allocate();
        tex.srvHandleCPU = srvManager_->GetCPUDescriptionHandle(tex.srvIndex);
        tex.srvHandleGPU = srvManager_->GetGPUDescriptionHandle(tex.srvIndex);

        srvManager_->CreateSRVTexture2D(
            tex.srvIndex,
            tex.resource.Get(),
            tex.metadata.format,
            1
        );
    }
}

void TextureManager::LoadTexture(const std::string& filePath)
{
    if (filePath.empty()) {
        return; // 空は白テクスチャを使う想定
    }

    // ★これを入れる
    if (textureDatas_.contains(filePath)) {
        return; // 既にロード済み
    }

    if (!std::filesystem::exists(filePath)) {
        DebugPrintA("[Texture] file not found: " + filePath);
        return;
    }

    // ---- 画像読み込み ----
    DirectX::ScratchImage image{};
    std::wstring filePathW = ConvertString(filePath);
    HRESULT hr = DirectX::LoadFromWICFile(
        filePathW.c_str(),
        DirectX::WIC_FLAGS_FORCE_SRGB,
        nullptr,
        image
    );
    assert(SUCCEEDED(hr));

    // ---- ミップ生成（失敗したら元画像でOK）----
    DirectX::ScratchImage mipImages{};
    hr = DirectX::GenerateMipMaps(
        image.GetImages(),
        image.GetImageCount(),
        image.GetMetadata(),
        DirectX::TEX_FILTER_SRGB,
        0,
        mipImages
    );
    if (FAILED(hr)) {
        mipImages = std::move(image);
    }

    // ---- 登録（unordered_map の key に filePath を使う）----
    TextureData& tex = textureDatas_[filePath];

    tex.metadata = mipImages.GetMetadata();
    tex.resource = dx_->CreateTextureResource(tex.metadata);

    // GPUへアップロード
    dx_->UploadTextureData(tex.resource, mipImages);

    // SRV確保（★チェックは Allocate 内で完結）
    tex.srvIndex = srvManager_->Allocate();
    tex.srvHandleCPU = srvManager_->GetCPUDescriptionHandle(tex.srvIndex);
    tex.srvHandleGPU = srvManager_->GetGPUDescriptionHandle(tex.srvIndex);

    // SRV作成は SrvManager に委譲
    srvManager_->CreateSRVTexture2D(
        tex.srvIndex,
        tex.resource.Get(),
        tex.metadata.format,
        static_cast<UINT>(tex.metadata.mipLevels)
    );
}


// TextureManager.cpp
bool TextureManager::HasTexture(const std::string& key) const {
    return textureDatas_.contains(key);
}

void TextureManager::LoadTextureFromMemory(const std::string& key, const uint8_t* data, size_t sizeBytes)
{
    if (key.empty()) return;
    if (textureDatas_.contains(key)) return;

    DirectX::ScratchImage image{};
    HRESULT hr = DirectX::LoadFromWICMemory(
        data, sizeBytes,
        DirectX::WIC_FLAGS_FORCE_SRGB,
        nullptr,
        image
    );
    if (FAILED(hr)) {
        OutputDebugStringA(("[Texture] LoadFromWICMemory failed: " + key + "\n").c_str());
        return;
    }

    DirectX::ScratchImage mipImages{};
    hr = DirectX::GenerateMipMaps(
        image.GetImages(), image.GetImageCount(), image.GetMetadata(),
        DirectX::TEX_FILTER_SRGB, 0, mipImages
    );
    if (FAILED(hr)) mipImages = std::move(image);

    TextureData& tex = textureDatas_[key];
    tex.metadata = mipImages.GetMetadata();
    tex.resource = dx_->CreateTextureResource(tex.metadata);
    dx_->UploadTextureData(tex.resource, mipImages);

    tex.srvIndex = srvManager_->Allocate();
    tex.srvHandleCPU = srvManager_->GetCPUDescriptionHandle(tex.srvIndex);
    tex.srvHandleGPU = srvManager_->GetGPUDescriptionHandle(tex.srvIndex);

    srvManager_->CreateSRVTexture2D(
        tex.srvIndex,
        tex.resource.Get(),
        tex.metadata.format,
        (UINT)tex.metadata.mipLevels
    );
}

void TextureManager::CreateDynamicTexture2D(
    const std::string& key, uint32_t width, uint32_t height, DXGI_FORMAT format)
{
    if (key.empty()) return;
    if (textureDatas_.contains(key)) return;

    TextureData tex{};
    tex.isDynamic = true;
    tex.dynamicFormat = format;

    // TexMetadata を最小限埋める（CreateTextureResource が dimension/format 等参照するので）
    tex.metadata.width = width;
    tex.metadata.height = height;
    tex.metadata.depth = 1;
    tex.metadata.arraySize = 1;
    tex.metadata.mipLevels = 1;
    tex.metadata.format = format;
    tex.metadata.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;

    // DEFAULTテクスチャ（最初はCOPY_DESTでOK。Updateで最後にGENERIC_READへ遷移する）
    tex.resource = dx_->CreateTextureResource(tex.metadata);
    tex.state = D3D12_RESOURCE_STATE_COPY_DEST;

    // SRV確保＆作成（mip=1）
    tex.srvIndex = srvManager_->Allocate();
    tex.srvHandleCPU = srvManager_->GetCPUDescriptionHandle(tex.srvIndex);
    tex.srvHandleGPU = srvManager_->GetGPUDescriptionHandle(tex.srvIndex);

    srvManager_->CreateSRVTexture2D(
        tex.srvIndex,
        tex.resource.Get(),
        format,
        1
    );

    // Uploadバッファのサイズ計算（footprint）
    D3D12_RESOURCE_DESC desc = tex.resource->GetDesc();

    dx_->GetDevice()->GetCopyableFootprints(
        &desc,
        0, 1, 0,
        &tex.footprint,
        &tex.numRows,
        &tex.rowSizeInBytes,
        &tex.uploadSize
    );

    // 常駐UPLOADバッファ（Buffer resource）
    tex.upload = dx_->CreateBufferResource((size_t)tex.uploadSize);
    tex.upload->SetName(L"DynamicTextureUpload");

    // mapして保持（毎フレ memcpy する）
    HRESULT hr = tex.upload->Map(0, nullptr, reinterpret_cast<void**>(&tex.mappedUpload));
    assert(SUCCEEDED(hr));

    textureDatas_.emplace(key, std::move(tex));
}

void TextureManager::UpdateDynamicTexture2D(
    const std::string& key,
    const uint8_t* src,
    uint32_t srcRowPitchBytes)
{
    auto it = textureDatas_.find(key);
    if (it == textureDatas_.end()) return;

    TextureData& tex = it->second;
    if (!tex.isDynamic || !tex.resource || !tex.upload || !tex.mappedUpload) return;
    if (!src) return;

    // 1) CPU側：uploadへ row by row でコピー
    //   upload側のRowPitchは footprint.Footprint.RowPitch（256byte aligned）
    const uint32_t dstRowPitch = tex.footprint.Footprint.RowPitch;
    const uint32_t copyBytesPerRow = (uint32_t)tex.rowSizeInBytes; // だいたい width*4 のはず

    for (UINT y = 0; y < tex.numRows; ++y) {
        uint8_t* dstRow = tex.mappedUpload + y * dstRowPitch;
        const uint8_t* srcRow = src + (size_t)y * srcRowPitchBytes;
        std::memcpy(dstRow, srcRow, copyBytesPerRow);
    }

    // 2) GPU側：CopyTextureRegion を commandList に積む（Closeしない！）
    ID3D12GraphicsCommandList* cmd = dx_->GetCommandList();

    // (A) テクスチャをCOPY_DESTへ（必要なら）
    if (tex.state != D3D12_RESOURCE_STATE_COPY_DEST) {
        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = tex.resource.Get();
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore = tex.state;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        cmd->ResourceBarrier(1, &b);
        tex.state = D3D12_RESOURCE_STATE_COPY_DEST;
    }

    // (B) CopyTextureRegion
    D3D12_TEXTURE_COPY_LOCATION dst{};
    dst.pResource = tex.resource.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION srcLoc{};
    srcLoc.pResource = tex.upload.Get();
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.PlacedFootprint = tex.footprint;

    cmd->CopyTextureRegion(&dst, 0, 0, 0, &srcLoc, nullptr);

    // (C) 描画で読めるように GENERIC_READ へ戻す
    {
        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = tex.resource.Get();
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        b.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
        cmd->ResourceBarrier(1, &b);
        tex.state = D3D12_RESOURCE_STATE_GENERIC_READ;
    }
}



const TextureManager::TextureData&
TextureManager::GetDataByPathOrWhite_(const std::string& filePath) const
{
    if (filePath.empty()) {
        return textureDatas_.at(kWhiteKey);
    }
    return textureDatas_.at(filePath);
}

TextureManager::TextureData&
TextureManager::GetDataByPathOrWhite_(const std::string& filePath)
{
    if (filePath.empty()) {
        return textureDatas_.at(kWhiteKey);
    }
    return textureDatas_.at(filePath);
}

uint32_t TextureManager::GetSrvIndex(const std::string& filePath) const
{
    return GetDataByPathOrWhite_(filePath).srvIndex;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath) const
{
    return GetDataByPathOrWhite_(filePath).srvHandleGPU;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath) const
{
    return GetDataByPathOrWhite_(filePath).metadata;
}

ID3D12DescriptorHeap* TextureManager::GetSrvDescriptorHeap() const
{
    assert(srvManager_);
    return srvManager_->GetDescriptorHeap();
}
