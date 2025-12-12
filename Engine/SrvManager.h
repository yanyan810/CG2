//#pragma once
//#include"DirectXCommon.h"
//#include "WinApp.h"
//
////SRV管理
//class SrvManager
//{
//
//public:
//
//	void Initialize(DirectXCommon* dxCommon);
//
//	uint32_t Allocate();
//
//	//げったー
//	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptionHandle(uint32_t index);
//	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptionHandle(uint32_t index);
//
//	//SRV生成（テクスチャ）
//	void CreateSRVTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels);
//	//SRV生成(Structed Buffer用)
//	void CreateSRVforStructedBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElemens, UINT structureByteStride);
//
//	void PreDraw();
//
//	void SetGraphicsDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);
//
//private:
//	DirectXCommon* dxCommon_ = nullptr;
//
//	//最大SRV
//	static const uint32_t kMaxSRVCount;
//	//SRV用のスクリプタサイズ
//	uint32_t descriptorSize;
//	//SRV陽デスクリプタヒープ
//	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
//
//	uint32_t useIndex = 0;
//
//
//
//};

#pragma once
#include "DirectXCommon.h"
#include <wrl.h>

class SrvManager
{
public:
    void Initialize(DirectXCommon* dxCommon);

    uint32_t Allocate();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptionHandle(uint32_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptionHandle(uint32_t index);

    // SRV生成（テクスチャ）
    void CreateSRVTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels);

    void PreDraw();
    void SetGraphicsDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);

    // ★追加：外から heap を取れるように
    ID3D12DescriptorHeap* GetDescriptorHeap() const { return descriptorHeap.Get(); }

private:
    DirectXCommon* dxCommon_ = nullptr;

    static const uint32_t kMaxSRVCount;
    uint32_t descriptorSize = 0;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    uint32_t useIndex = 0;
};
