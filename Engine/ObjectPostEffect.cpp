#include "ObjectPostEffect.h"
#include <cassert>

void ObjectPostEffect::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, RtvManager* rtvManager)
{
    dxCommon_ = dxCommon;
    srvManager_ = srvManager;
    rtvManager_ = rtvManager;

    const std::array<float, 4> transparent = { 0.0f, 0.0f, 0.0f, 0.0f };

    objectRT_ = std::make_unique<RenderTexture>();
    objectRT_->Initialize(dxCommon_, srvManager_, rtvManager_, WinApp::kClientWidth, WinApp::kClientHeight, transparent);

    uint32_t bloomWidth = WinApp::kClientWidth / 4;
    uint32_t bloomHeight = WinApp::kClientHeight / 4;

    bloomRT_A_ = std::make_unique<RenderTexture>();
    bloomRT_A_->Initialize(dxCommon_, srvManager_, rtvManager_, bloomWidth, bloomHeight, transparent);

    bloomRT_B_ = std::make_unique<RenderTexture>();
    bloomRT_B_->Initialize(dxCommon_, srvManager_, rtvManager_, bloomWidth, bloomHeight, transparent);

    bloomRT_Half_ = std::make_unique<RenderTexture>();
    bloomRT_Half_->Initialize(dxCommon_, srvManager_, rtvManager_, WinApp::kClientWidth / 2, WinApp::kClientHeight / 2, transparent);

    for (auto& cb : cbs_) {
        cb = std::make_unique<BloomConstantBuffer>();
        cb->Initialize(dxCommon_);
    }

    postEffect_ = std::make_unique<PostEffect>();
    postEffect_->Initialize(dxCommon_, cbs_[currentCbIndex_].get());

    param_.threshold = 0.0f;
    param_.intensity = 1.0f;
    param_.vignetteIntensity = 0.0f;
    param_.vignetteScale = 0.0f;
    param_.chromAbAmount = 0.0f;
    param_.distortionAmount = 0.0f;
    param_.noiseIntensity = 0.0f;
    param_.scanlineIntensity = 0.0f;
    param_.scanlineFrequency = 100.0f;
    param_.curvature = 0.0f;
    param_.borderSharp = 0.0f;
    param_.glitchAmount = 0.0f;
    param_.radialBlurStrength = 0.0f;
    param_.dissolveAmount = -1.0f;
    param_.dissolveEdgeWidth = 0.08f;
    param_.dissolveEdgeIntensity = 2.0f;
    param_.dissolveNoiseScale = 36.0f;
    param_.dissolveEdgeColor = { 0.15f, 1.0f, 1.0f, 1.0f };

    cbs_[currentCbIndex_]->Update(param_);
}

void ObjectPostEffect::Update()
{
    timer_ += 1.0f / 60.0f;
    param_.timer = timer_;
    cbs_[currentCbIndex_]->Update(param_);
}

void ObjectPostEffect::BeginCapture()
{
    const float transparent[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    Transition(objectRT_->GetResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    dxCommon_->SetRenderTarget(objectRT_->GetRTVHandle());
    dxCommon_->ClearRenderTarget(objectRT_->GetRTVHandle(), transparent);
    dxCommon_->SetViewport(WinApp::kClientWidth, WinApp::kClientHeight);
}

void ObjectPostEffect::EndCapture()
{
    EndCaptureToRenderTarget({ 0 }, WinApp::kClientWidth, WinApp::kClientHeight);
}

void ObjectPostEffect::EndCaptureToRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE outputRTV, int width, int height, int clipHeight)
{
    Transition(objectRT_->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    Transition(bloomRT_Half_->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    dxCommon_->SetRenderTargetNoDepth(bloomRT_Half_->GetRTVHandle());
    dxCommon_->SetViewport(WinApp::kClientWidth / 2, WinApp::kClientHeight / 2);
    const float transparent[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    dxCommon_->ClearRenderTarget(bloomRT_Half_->GetRTVHandle(), transparent);
    postEffect_->Draw(objectRT_->GetGPUHandle(), Bloom_Extract);
    Transition(bloomRT_Half_->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    Transition(bloomRT_A_->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    dxCommon_->SetRenderTargetNoDepth(bloomRT_A_->GetRTVHandle());
    dxCommon_->SetViewport(WinApp::kClientWidth / 4, WinApp::kClientHeight / 4);
    dxCommon_->ClearRenderTarget(bloomRT_A_->GetRTVHandle(), transparent);
    postEffect_->Draw(bloomRT_Half_->GetGPUHandle(), Bloom_Downsample);
    Transition(bloomRT_A_->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    Transition(bloomRT_B_->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    dxCommon_->SetRenderTargetNoDepth(bloomRT_B_->GetRTVHandle());
    dxCommon_->ClearRenderTarget(bloomRT_B_->GetRTVHandle(), transparent);
    postEffect_->Draw(bloomRT_A_->GetGPUHandle(), Bloom_BlurH);
    Transition(bloomRT_B_->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    Transition(bloomRT_A_->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    dxCommon_->SetRenderTargetNoDepth(bloomRT_A_->GetRTVHandle());
    dxCommon_->ClearRenderTarget(bloomRT_A_->GetRTVHandle(), transparent);
    postEffect_->Draw(bloomRT_B_->GetGPUHandle(), Bloom_BlurV);
    Transition(bloomRT_A_->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    if (outputRTV.ptr == 0) {
        dxCommon_->SetBackBuffer();
    } else {
        dxCommon_->SetRenderTargetNoDepth(outputRTV);
    }
    dxCommon_->SetViewport(width, height);
    if (clipHeight > 0) {
        dxCommon_->SetScissorRect(0, 0, width, clipHeight);
    }
    postEffect_->DrawObjectComposite(objectRT_->GetGPUHandle(), bloomRT_A_->GetGPUHandle());
}

void ObjectPostEffect::SetParam(const BloomParam& param)
{
    param_ = param;
    param_.timer = timer_;
    currentCbIndex_ = (currentCbIndex_ + 1) % kParamBufferCount_;
    cbs_[currentCbIndex_]->Update(param_);
    postEffect_->SetBloomConstantBuffer(cbs_[currentCbIndex_].get());
}

void ObjectPostEffect::Transition(ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    assert(res != nullptr);
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = res;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);
}
