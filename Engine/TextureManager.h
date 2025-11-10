#pragma once
#include "WinApp.h"
#include "DirectXCommon.h"
#include "StringUtility.h"

class TextureManager
{
public:
	//シングルトンインスタンスの取得
	static TextureManager* GetInstance();
	//終了
	void Finalize();

	void Initialize(DirectXCommon* dxCommon);

	void LoadTexture(const std::string& filePath);

	uint32_t GetTextureIndexByFilePath(const std::string& filePath);

	//テクスチャ番号からGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

	//メタデータ取得
	const DirectX::TexMetadata& GetMetaData(uint32_t textureIndex);

private:
	static TextureManager* instance;

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;



private:

	//テクスチャ一枚のデータ
	struct TextureData {

		std::string filePath; //テクスチャファイルのパス
		DirectX::TexMetadata metadata; //テクスチャのメタデータ
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU; //CPU用SRVハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU; //GPU用SRVハンドル


	};

	//テクスチャデータ
	std::vector<TextureData> textureDatas_;

	DirectXCommon* dx_ = nullptr;

	//SRVインデックスの開始番号
	static uint32_t kSRVIndexTop;

};

