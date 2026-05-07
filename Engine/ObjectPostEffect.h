#pragma once
#include "BloomConstantBuffer.h"
#include "PostEffect.h"
#include "RenderTexture.h"
#include <memory>

class ObjectPostEffect {
public:
    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, RtvManager* rtvManager);
    void Update();

    void BeginCapture();
    void EndCapture();

    BloomParam& GetParam() { return param_; }
    const BloomParam& GetParam() const { return param_; }
    void SetParam(const BloomParam& param);

private:
    void Transition(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

private:
    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;
    RtvManager* rtvManager_ = nullptr;

    std::unique_ptr<RenderTexture> objectRT_;
    std::unique_ptr<RenderTexture> bloomRT_A_;
    std::unique_ptr<RenderTexture> bloomRT_B_;
    std::unique_ptr<RenderTexture> bloomRT_Half_;

    std::unique_ptr<PostEffect> postEffect_;
    std::unique_ptr<BloomConstantBuffer> cb_;
    BloomParam param_{};
    float timer_ = 0.0f;
};
