//#pragma once
//#include "WinApp.h"
//#include "DirectXCommon.h"
//#include "StringUtility.h"
//#include "SrvManager.h"
//#include <unordered_map>
//
//
//class TextureManager
//{
//public:
//	//シングルトンインスタンスの取得
//	static TextureManager* GetInstance();
//	//終了
//	void Finalize();
//
//	void Initialize(DirectXCommon* dxCommon,SrvManager* srvManager);
//
//	void LoadTexture(const std::string& filePath);
//
//	uint32_t GetTextureIndexByFilePath(const std::string& filePath);
//
//	//テクスチャ番号からGPUハンドルを取得
//	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);
//
//	//メタデータ取得
//	const DirectX::TexMetadata& GetMetaData(const std::string& filePath);
//
//	ID3D12DescriptorHeap* GetSrvDescriptorHeap() const;
//
//private:
//	static TextureManager* instance;
//
//	TextureManager() = default;
//	~TextureManager() = default;
//	TextureManager(TextureManager&) = delete;
//	TextureManager& operator=(TextureManager&) = delete;
//
//
//
//private:
//
//	//テクスチャ一枚のデータ
//	struct TextureData {
//
//	//	std::string filePath; //テクスチャファイルのパス
//		DirectX::TexMetadata metadata; //テクスチャのメタデータ
//		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
//		uint32_t srvIndex;
//		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU; //CPU用SRVハンドル
//		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU; //GPU用SRVハンドル
//
//
//	};
//
//	//テクスチャデータ
//	std::unordered_map<std::string, TextureData> textureDatas_;
//
//	DirectXCommon* dx_ = nullptr;
//
//	SrvManager* srvManager_ = nullptr; 
//
//	//SRVインデックスの開始番号
//	static uint32_t kSRVIndexTop;
//
//
//};
//

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
