#pragma once
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <cstdint>

#include <d3d12.h>
#include <dxgi1_6.h>

class SrvManager;

class VideoPlayerMF {
public:
    bool Open(const std::string& pathUtf8, bool loop);

    void Close();

    bool ReadNextFrame();

    bool CreateDxResources(ID3D12Device* device, SrvManager* srv);

    // Upload（このフレームで描画する前に呼ぶ）
    void UploadToGpu(ID3D12GraphicsCommandList* cmd);

    // 描画が終わった後に呼ぶ（次のUploadのために戻す）
    void EndFrame(ID3D12GraphicsCommandList* cmd);

    // SRV を使って描画したい時用
    uint32_t SrvIndex() const { return srvIndex_; }
    D3D12_GPU_DESCRIPTOR_HANDLE SrvGpu() const { return srvGpu_; }

    uint32_t Width()  const { return width_; }
    uint32_t Height() const { return height_; }
    int32_t  Stride() const { return stride_; }
    const uint8_t* FrameBGRA() const { return frame_.data(); }

    LONGLONG LastTimestamp100ns() const { return lastTs100ns_; }

    bool IsReady() const { return gpuInitialized_ && (videoTex_ != nullptr) && (srvGpu_.ptr != 0); }

private:
    Microsoft::WRL::ComPtr<IMFSourceReader> reader_;
    uint32_t width_ = 0, height_ = 0;
    int32_t  stride_ = 0;

    bool loop_ = false;
    LONGLONG duration100ns_ = 0;

    std::vector<uint8_t> frame_;
    LONGLONG lastTs100ns_ = 0;
    bool hasNewFrame_ = false;

    // DX12
    Microsoft::WRL::ComPtr<ID3D12Resource> videoTex_;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuf_;
    uint32_t srvIndex_ = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpu_{};

    bool gpuInitialized_ = false;
    bool texInPSR_ = false; // 今 PSR 状態か？（状態管理）
};
