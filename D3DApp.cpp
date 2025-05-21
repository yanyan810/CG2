#include "D3DApp.h"
#include <cassert>
#include <format>

using Microsoft::WRL::ComPtr;

void D3DApp::Initialize(HWND hwnd, int width, int height) {
    CreateDeviceAndSwapChain(hwnd);
    CreateRenderTargets();

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_));

    device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
    device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));

    device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void D3DApp::CreateDeviceAndSwapChain(HWND hwnd) {
    ComPtr<IDXGIFactory7> factory;
    CreateDXGIFactory(IID_PPV_ARGS(&factory));

    ComPtr<IDXGIAdapter4> adapter;
    for (UINT i = 0; factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC3 desc;
        adapter->GetDesc3(&desc);
        if (!(desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
            break;
        }
        adapter.Reset();
    }
    assert(adapter);

    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0 };
    for (auto lvl : levels) {
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), lvl, IID_PPV_ARGS(&device_)))) {
            break;
        }
    }
    assert(device_);

    DXGI_SWAP_CHAIN_DESC swapDesc{};
    swapDesc.BufferCount = 2;
    swapDesc.BufferDesc.Width = 1280;
    swapDesc.BufferDesc.Height = 720;
    swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapDesc.OutputWindow = hwnd;
    swapDesc.SampleDesc.Count = 1;
    swapDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain> baseSwap;
    factory->CreateSwapChain(commandQueue_.Get(), &swapDesc, &baseSwap);
    baseSwap.As(&swapChain_);
}

void D3DApp::CreateRenderTargets() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_));

    rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

    for (int i = 0; i < 2; ++i) {
        swapChain_->GetBuffer(i, IID_PPV_ARGS(&renderTargets_[i]));
        device_->CreateRenderTargetView(renderTargets_[i].Get(), nullptr, handle);
        handle.ptr += rtvDescriptorSize_;
    }
}

void D3DApp::BeginFrame() {
    backBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = renderTargets_[backBufferIndex_].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandAllocator_->Reset();
    commandList_->Reset(commandAllocator_.Get(), nullptr);
    commandList_->ResourceBarrier(1, &barrier);
}

void D3DApp::EndFrame() {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = renderTargets_[backBufferIndex_].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList_->ResourceBarrier(1, &barrier);

    commandList_->Close();
    ID3D12CommandList* cmds[] = { commandList_.Get() };
    commandQueue_->ExecuteCommandLists(1, cmds);
    swapChain_->Present(1, 0);

    fenceValue_++;
    commandQueue_->Signal(fence_.Get(), fenceValue_);
    if (fence_->GetCompletedValue() < fenceValue_) {
        fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
        WaitForSingleObject(fenceEvent_, INFINITE);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::GetCurrentRTV() const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += backBufferIndex_ * rtvDescriptorSize_;
    return handle;
}

void D3DApp::WaitGPU() {
    fenceValue_++;
    commandQueue_->Signal(fence_.Get(), fenceValue_);
    fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
    WaitForSingleObject(fenceEvent_, INFINITE);
}

void D3DApp::Finalize() {
    WaitGPU();
    CloseHandle(fenceEvent_);
}
