//#include "TextureManager.h"
//
////Imguiで0バンを使用するので1から
//uint32_t TextureManager::kSRVIndexTop = 1;
//
//TextureManager* TextureManager::instance = nullptr;
//
//using namespace StringUtility;
//
//TextureManager* TextureManager::GetInstance() {
//
//	if (instance == nullptr) {
//		instance = new TextureManager();
//	}
//	return instance;
//}
//
//void TextureManager::Finalize() {
//	//if (instance != nullptr) {
//		delete instance;
//		instance = nullptr;
//	//}
//}
//
//void TextureManager::Initialize(DirectXCommon* dxCommon,SrvManager* srvManager) {
//
//	dx_ = dxCommon;
//
//	srvManager_ = srvManager;
//
//	textureDatas_.reserve(DirectXCommon::kMaxSRVCount);
//
//	// =========================================================
//	// ★ 0番は「白(不透明)テクスチャ」を必ず入れておく
//	//    Model側で textureFilePath が空のとき textureIndex=0 になるため
//	// =========================================================
//	{
//		DirectX::ScratchImage whiteImg{};
//		HRESULT hr = whiteImg.Initialize2D(
//			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, // sRGB運用ならこれ
//			1, 1, 1, 1
//		);
//		assert(SUCCEEDED(hr));
//
//		// 1x1 の RGBA を白(255,255,255,255)で埋める
//		auto* img = whiteImg.GetImage(0, 0, 0);
//		assert(img && img->pixels && img->rowPitch >= 4);
//
//		img->pixels[0] = 255; // R
//		img->pixels[1] = 255; // G
//		img->pixels[2] = 255; // B
//		img->pixels[3] = 255; // A
//
//		textureDatas_.resize(textureDatas_.size() + 1);
//		TextureData& tex = textureDatas_.back();
//
//		tex.filePath = "__white__";
//		tex.metadata = whiteImg.GetMetadata();
//		tex.resource = dx_->CreateTextureResource(tex.metadata);
//
//		// GPUへアップロード（これやらないと黒/不定になる）:contentReference[oaicite:2]{index=2}
//		dx_->UploadTextureData(tex.resource, whiteImg);
//
//		// SRV は ImGui が 0番を使うので 1番を使う（kSRVIndexTop=1）:contentReference[oaicite:3]{index=3}
//		uint32_t srvIndex = static_cast<uint32_t>(textureDatas_.size() - 1) + kSRVIndexTop;
//		tex.srvHandleCPU = dx_->GetSRVCPUDescriptorHandle(srvIndex);
//		tex.srvHandleGPU = dx_->GetSRVGPUDescriptorHandle(srvIndex);
//
//		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
//		srvDesc.Format = tex.metadata.format;
//		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
//		srvDesc.Texture2D.MostDetailedMip = 0;
//		srvDesc.Texture2D.MipLevels = 1;
//		srvDesc.Texture2D.PlaneSlice = 0;
//		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
//
//		dx_->GetDevice()->CreateShaderResourceView(
//			tex.resource.Get(), &srvDesc, tex.srvHandleCPU);
//	}
//}
//
//void TextureManager::LoadTexture(const std::string& filePath) {
//
//	if (filePath.empty()) {
//		return; // 0番(白)を使う想定
//	}
//
//	//読み込み済みテクスチャを検索
//	if (textureDatas_.contains(filePath)) {
//
//
//
//}
//	//テクスチャ枚数上限チェック
//	//assert(srvManager_->Allocate());
//
//	//テクスチャファイルを読んでプログラムで扱えるようにする
//	DirectX::ScratchImage image{};
//	std::wstring filePathW = ConvertString(filePath);
//	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
//	assert(SUCCEEDED(hr));
//
//	//ミップマップの作成
//	DirectX::ScratchImage mipImages{};
//	hr = DirectX::GenerateMipMaps(
//		image.GetImages(),
//		image.GetImageCount(),
//		image.GetMetadata(),
//		DirectX::TEX_FILTER_SRGB, 0, mipImages
//	);
//
//	if (FAILED(hr)) {
//		// ★ ミップ生成に失敗したら、そのまま使う
//		mipImages = std::move(image);
//	}
//
//	//テクスチャデータを追加
//	textureDatas_.resize(textureDatas_.size() + 1);
//	//追加したテクスチャデータの参照を取得する
//	//TextureData& textureData = textureDatas_.back();
//	TextureData& textureData = textureDatas_[filePath];
//	//textureData.filePath = filePath;
//	//textureData.metadata = mipImages.GetMetadata();
//	//textureData.resource = dx_->CreateTextureResource(textureData.metadata);
//	
//	textureData.srvIndex = srvManager_->Allocate();
//	textureData.srvHandleCPU = srvManager_->GetCPUDescriptionHandle(textureData.srvIndex);
//	textureData.srvHandleGPU = srvManager_->GetGPUDescriptionHandle(textureData.srvIndex);
//
//	// ★ 画像データをGPUへアップロード（これが無いと真っ黒）
//	dx_->UploadTextureData(textureData.resource, mipImages);
//
//	//テクスチャデータの要素番号をSRVのインデックスとする
//	uint32_t srvIndex = static_cast<uint32_t>(textureDatas_.size() - 1)+kSRVIndexTop;
//	textureData.srvHandleCPU = dx_->GetSRVCPUDescriptorHandle(srvIndex);
//	textureData.srvHandleGPU = dx_->GetSRVGPUDescriptorHandle(srvIndex);
//
//	//srvの設定を行う
//	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
//	srvDesc.Format = textureData.metadata.format;
//	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
//	srvDesc.Texture2D.MostDetailedMip = 0;
//	srvDesc.Texture2D.MipLevels = static_cast<UINT>(textureData.metadata.mipLevels);
//	srvDesc.Texture2D.PlaneSlice = 0;
//	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
//
//	dx_->GetDevice()->CreateShaderResourceView(
//		textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);
//
//}
//
//uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath) {
//
//	//読み込み済みテクスチャを検索
//	auto it = std::find_if(
//
//		textureDatas_.begin(), textureDatas_.end(),
//		[&](TextureData& textureData) {
//			return textureData.filePath == filePath;
//		});
//
//	if (it != textureDatas_.end()) {
//		//読み込み済みなら要素番号を渡す
//		uint32_t textureIndex = static_cast<uint32_t>(std::distance(textureDatas_.begin(), it));
//		return textureIndex;
//	}
//
//	assert(0);
//	return 0;
//
//}
//
//D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath) {
//	assert(textureIndex < textureDatas_.size());
//
//	TextureData& textureData = textureDatas_[textureIndex];
//
//	return textureData.srvHandleGPU;
//}
//
//const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath) {
//	assert(textureIndex < textureDatas_.size());
//	TextureData& textureData = textureDatas_[textureIndex];
//	return textureData.metadata;
//}
//
//ID3D12DescriptorHeap* TextureManager::GetSrvDescriptorHeap() const {
//	assert(dx_);                    // Initialize 済み前提
//	return dx_->GetSRVDescriptorHeap();
//}

#include "TextureManager.h"
#include <cassert>

TextureManager* TextureManager::instance = nullptr;
using namespace StringUtility;

TextureManager* TextureManager::GetInstance()
{
    if (instance == nullptr) {
        instance = new TextureManager();
    }
    return instance;
}

void TextureManager::Finalize()
{
    delete instance;
    instance = nullptr;
}

void TextureManager::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager)
{
    dx_ = dxCommon;
    srvManager_ = srvManager;

    // ---- 1x1 白テクスチャを必ず作る（filePath="" のとき用）----
    {
        DirectX::ScratchImage whiteImg{};
        HRESULT hr = whiteImg.Initialize2D(
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            1, 1, 1, 1
        );
        assert(SUCCEEDED(hr));

        auto* img = whiteImg.GetImage(0, 0, 0);
        assert(img && img->pixels && img->rowPitch >= 4);

        img->pixels[0] = 255; // R
        img->pixels[1] = 255; // G
        img->pixels[2] = 255; // B
        img->pixels[3] = 255; // A

        TextureData& tex = textureDatas_[kWhiteKey];
        tex.metadata = whiteImg.GetMetadata();
        tex.resource = dx_->CreateTextureResource(tex.metadata);
        dx_->UploadTextureData(tex.resource, whiteImg);

        tex.srvIndex = srvManager_->Allocate();
        tex.srvHandleCPU = srvManager_->GetCPUDescriptionHandle(tex.srvIndex);
        tex.srvHandleGPU = srvManager_->GetGPUDescriptionHandle(tex.srvIndex);

        srvManager_->CreateSRVTexture2D(
            tex.srvIndex,
            tex.resource.Get(),
            tex.metadata.format,
            1
        );
    }
}

void TextureManager::LoadTexture(const std::string& filePath)
{
    if (filePath.empty()) {
        return; // 空は白テクスチャを使う想定
    }

    // ★読込済みチェック（スライド通り）
    if (textureDatas_.contains(filePath)) {
        return;
    }

    // ---- 画像読み込み ----
    DirectX::ScratchImage image{};
    std::wstring filePathW = ConvertString(filePath);
    HRESULT hr = DirectX::LoadFromWICFile(
        filePathW.c_str(),
        DirectX::WIC_FLAGS_FORCE_SRGB,
        nullptr,
        image
    );
    assert(SUCCEEDED(hr));

    // ---- ミップ生成（失敗したら元画像でOK）----
    DirectX::ScratchImage mipImages{};
    hr = DirectX::GenerateMipMaps(
        image.GetImages(),
        image.GetImageCount(),
        image.GetMetadata(),
        DirectX::TEX_FILTER_SRGB,
        0,
        mipImages
    );
    if (FAILED(hr)) {
        mipImages = std::move(image);
    }

    // ---- 登録（unordered_map の key に filePath を使う）----
    TextureData& tex = textureDatas_[filePath];

    tex.metadata = mipImages.GetMetadata();
    tex.resource = dx_->CreateTextureResource(tex.metadata);

    // GPUへアップロード
    dx_->UploadTextureData(tex.resource, mipImages);

    // SRV確保（★チェックは Allocate 内で完結）
    tex.srvIndex = srvManager_->Allocate();
    tex.srvHandleCPU = srvManager_->GetCPUDescriptionHandle(tex.srvIndex);
    tex.srvHandleGPU = srvManager_->GetGPUDescriptionHandle(tex.srvIndex);

    // SRV作成は SrvManager に委譲
    srvManager_->CreateSRVTexture2D(
        tex.srvIndex,
        tex.resource.Get(),
        tex.metadata.format,
        static_cast<UINT>(tex.metadata.mipLevels)
    );
}

const TextureManager::TextureData&
TextureManager::GetDataByPathOrWhite_(const std::string& filePath) const
{
    if (filePath.empty()) {
        return textureDatas_.at(kWhiteKey);
    }
    return textureDatas_.at(filePath);
}

TextureManager::TextureData&
TextureManager::GetDataByPathOrWhite_(const std::string& filePath)
{
    if (filePath.empty()) {
        return textureDatas_.at(kWhiteKey);
    }
    return textureDatas_.at(filePath);
}

uint32_t TextureManager::GetSrvIndex(const std::string& filePath) const
{
    return GetDataByPathOrWhite_(filePath).srvIndex;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath) const
{
    return GetDataByPathOrWhite_(filePath).srvHandleGPU;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath) const
{
    return GetDataByPathOrWhite_(filePath).metadata;
}

ID3D12DescriptorHeap* TextureManager::GetSrvDescriptorHeap() const
{
    assert(srvManager_);
    return srvManager_->GetDescriptorHeap();
}
