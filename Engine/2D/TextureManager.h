#pragma once
#include "DirectXCommon.h"
#include "StringUtility.h"
#include "SrvManager.h"
#include <unordered_map>
#include <string>
#include <wrl.h>
#include <DirectXTex.h>

class TextureManager
{
public:
    static TextureManager* GetInstance();
    void Finalize();

    void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);
    void LoadTexture(const std::string& filePath);

    // ★スライド通り：SRVインデックス取得
    uint32_t GetSrvIndex(const std::string& filePath) const;

    // GPUハンドル取得（filePath → srvIndex → handle）
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath) const;

    // メタデータ取得
    const DirectX::TexMetadata& GetMetaData(const std::string& filePath) const;

    // SRV DescriptorHeap 取得（描画側が SetDescriptorHeaps したい時用）
    ID3D12DescriptorHeap* GetSrvDescriptorHeap() const;

    void LoadTextureFromMemory(const std::string& key, const uint8_t* data, size_t sizeBytes);
    bool HasTexture(const std::string& key) const;
    
    // TextureManager.h 追加
    void CreateDynamicTexture2D(
        const std::string& key,
        uint32_t width,
        uint32_t height,
        DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM
    );

    // srcRowPitchBytes は「元データの1行バイト数」（MFのstride）
    void UpdateDynamicTexture2D(
        const std::string& key,
        const uint8_t* src,
        uint32_t srcRowPitchBytes
    );


private:
    static TextureManager* instance;

    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(TextureManager&) = delete;
    TextureManager& operator=(TextureManager&) = delete;

    struct TextureData {
        DirectX::TexMetadata metadata{};
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        uint32_t srvIndex = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU{};
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU{};

        // ===== 動的更新用 =====
        bool isDynamic = false;
        DXGI_FORMAT dynamicFormat = DXGI_FORMAT_UNKNOWN;

        Microsoft::WRL::ComPtr<ID3D12Resource> upload; // 常駐UPLOADバッファ
        uint8_t* mappedUpload = nullptr;

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        UINT numRows = 0;
        UINT64 rowSizeInBytes = 0;
        UINT64 uploadSize = 0;

        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST; // 現在state（管理）
    };

    // filePath を key にする（スライド通り）
    std::unordered_map<std::string, TextureData> textureDatas_;

    DirectXCommon* dx_ = nullptr;
    SrvManager* srvManager_ = nullptr;

    // 空文字のときに使う白テクスチャのキー
    static constexpr const char* kWhiteKey = "__white__";

    const TextureData& GetDataByPathOrWhite_(const std::string& filePath) const;
    TextureData& GetDataByPathOrWhite_(const std::string& filePath);
};
