#include "VideoPlayerMF.h"
#include <cassert>
#include <d3dx12.h>
#include "SrvManager.h"

#include <Windows.h>

static inline void HR(HRESULT hr) { assert(SUCCEEDED(hr)); }


static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, ws.data(), len);
    return ws;
}

bool VideoPlayerMF::Open(const std::string& pathUtf8, bool loop) {
    return Open(Utf8ToWide(pathUtf8), loop);
}


bool VideoPlayerMF::CreateDxResources(ID3D12Device* device, SrvManager* srv)
{
    if (!device || !srv) return false;
    if (width_ == 0 || height_ == 0) return false;

    const DXGI_FORMAT fmt = DXGI_FORMAT_B8G8R8A8_UNORM;

    // ---- Texture (Default heap) ----
    auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(fmt, width_, height_, 1, 1);

    HR(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,   // 最初は Upload できるよう COPY_DEST
        nullptr,
        IID_PPV_ARGS(&videoTex_)
    ));

    texInPSR_ = false;

    // ---- Upload buffer ----
    const UINT64 uploadSize = GetRequiredIntermediateSize(videoTex_.Get(), 0, 1);

    HR(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uploadSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuf_)
    ));

    // ---- SRV ----
    srvIndex_ = srv->Allocate();
    srv->CreateSRVTexture2D(srvIndex_, videoTex_.Get(), fmt, 1);
    srvGpu_ = srv->GetGPUDescriptionHandle(srvIndex_);

    gpuInitialized_ = true;
    return true;
}

void VideoPlayerMF::UploadToGpu(ID3D12GraphicsCommandList* cmd)
{
    if (!gpuInitialized_ || !cmd) return;
    if (!hasNewFrame_) return;
    if (!videoTex_ || !uploadBuf_) return;

    // もし前フレームで PSR のままなら、まず COPY_DEST に戻す
    if (texInPSR_) {
        cmd->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            videoTex_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_COPY_DEST
        ));
        texInPSR_ = false;
    }

    D3D12_SUBRESOURCE_DATA sub{};
    sub.pData = frame_.data();
    sub.RowPitch = stride_;
    sub.SlicePitch = (LONG_PTR)stride_ * (LONG_PTR)height_;

    UpdateSubresources(cmd, videoTex_.Get(), uploadBuf_.Get(), 0, 0, 1, &sub);

    // 描画できるよう PSR へ
    cmd->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        videoTex_.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    ));
    texInPSR_ = true;

    hasNewFrame_ = false;
}

void VideoPlayerMF::EndFrame(ID3D12GraphicsCommandList* cmd)
{
    if (!gpuInitialized_ || !cmd) return;
    if (!videoTex_) return;

    // このフレームで描画したなら、次のUpload用にCOPY_DESTへ戻す
    if (texInPSR_) {
        cmd->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            videoTex_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_COPY_DEST
        ));
        texInPSR_ = false;
    }
}
