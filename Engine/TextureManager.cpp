#include "TextureManager.h"

//Imguiで0バンを使用するので1から
uint32_t TextureManager::kSRVIndexTop = 1;

TextureManager* TextureManager::instance = nullptr;

using namespace StringUtility;

TextureManager* TextureManager::GetInstance() {

	if (instance == nullptr) {
		instance = new TextureManager();
	}
	return instance;
}

void TextureManager::Finalize() {
	//if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	//}
}

void TextureManager::Initialize(DirectXCommon* dxCommon) {

	dx_ = dxCommon;

	textureDatas_.reserve(DirectXCommon::kMaxSRVCount);

}

void TextureManager::LoadTexture(const std::string& filePath) {

	//読み込み済みテクスチャを検索
	auto it = std::find_if(

		textureDatas_.begin(), textureDatas_.end(),
		[&]( TextureData& textureData) {
			return textureData.filePath == filePath;
		});
	//見つかったら何もしない
	if (it != textureDatas_.end()) {
		return;
	}

	//テクスチャ枚数上限チェック
	assert(textureDatas_.size() + kSRVIndexTop < DirectXCommon::kMaxSRVCount);

	//テクスチャファイルを読んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	//ミニマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(),
		image.GetImageCount(),
		image.GetMetadata(),
		DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	//テクスチャデータを追加
	textureDatas_.resize(textureDatas_.size() + 1);
	//追加したテクスチャデータの参照を取得する
	TextureData& textureData = textureDatas_.back();
	textureData.filePath = filePath;
	textureData.metadata = mipImages.GetMetadata();
	textureData.resource = dx_->CreateTextureResource(textureData.metadata);
	
	// ★ 画像データをGPUへアップロード（これが無いと真っ黒）
	dx_->UploadTextureData(textureData.resource, mipImages);

	//テクスチャデータの要素番号をSRVのインデックスとする
	uint32_t srvIndex = static_cast<uint32_t>(textureDatas_.size() - 1)+kSRVIndexTop;
	textureData.srvHandleCPU = dx_->GetSRVCPUDescriptorHandle(srvIndex);
	textureData.srvHandleGPU = dx_->GetSRVGPUDescriptorHandle(srvIndex);

	//srvの設定を行う
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = static_cast<UINT>(textureData.metadata.mipLevels);
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	dx_->GetDevice()->CreateShaderResourceView(
		textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);

}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath) {

	//読み込み済みテクスチャを検索
	auto it = std::find_if(

		textureDatas_.begin(), textureDatas_.end(),
		[&](TextureData& textureData) {
			return textureData.filePath == filePath;
		});

	if (it != textureDatas_.end()) {
		//読み込み済みなら要素番号を渡す
		uint32_t textureIndex = static_cast<uint32_t>(std::distance(textureDatas_.begin(), it));
		return textureIndex;
	}

	assert(0);
	return 0;

}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureIndex) {
	assert(textureIndex < textureDatas_.size());

	TextureData& textureData = textureDatas_[textureIndex];

	return textureData.srvHandleGPU;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(uint32_t textureIndex) {
	assert(textureIndex < textureDatas_.size());
	TextureData& textureData = textureDatas_[textureIndex];
	return textureData.metadata;
}