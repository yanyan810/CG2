#pragma once
#include "WinApp.h"
#include "DirectXCommon.h"


class TextureManager
{
public:
	//シングルトンインスタンスの取得
	static TextureManager* GetInstance();
	//終了
	void Finalize();

	void Initialize();


private:
	static TextureManager* instance;

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;

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

};

