#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include"Logger.h"
#include "StringUtility.h"
#include "WinApp.h"
#include <array>
#include <dxcapi.h>
#include <chrono>
#include <thread>

#include "Root.h"

enum ShaderType {
	kModelParticle,
	kComputeParticleUpdate, // 追加：パーティクル更新用
	kPostEffect,
	kShadow,
	kTrail,
};

enum PostEffectType {
	kNone,

	Bloom_Extract,
	Bloom_Downsample,
	Bloom_BlurH,
	Bloom_BlurV,
	Bloom_Composite,
};

class DirectXCommon
{

public:

	struct PSO {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsDesc_{};
		Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsState_ = nullptr;
		Root root_;
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_ = nullptr;
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_ = nullptr;
		ShaderType shaderType_;
		std::wstring vsFilePath_;
		std::wstring psFilePath_;
		PostEffectType postEffectType_;
		D3D12_INPUT_ELEMENT_DESC elementDescs_[3] = {};
		D3D12_INPUT_LAYOUT_DESC layout_{};

		// --- Compute Shader 用に追加 ---
		D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc_{};
		Microsoft::WRL::ComPtr<ID3D12PipelineState> computeState_ = nullptr;
		Microsoft::WRL::ComPtr<IDxcBlob> computeShaderBlob_ = nullptr;
		std::wstring csFilePath_;
	};

	PSO& GetPSOComputeParticle() { return computeParticlePSO; }
	PSO& GetPSOModelParticle() { return psoModelParticle_; }
	PSO& GetPSOTrail() { return trailPSO; }

	PSO& GetPSOEffect(PostEffectType effect) {
		switch (effect)
		{
		case Bloom_Extract:
			return bloomPSO;
			break;
		case Bloom_Downsample:
			return downsamplePSO;
			break;
		case Bloom_BlurH:
			return blurHPSO;
			break;
		case Bloom_BlurV:
			return blurVPSO;
			break;
		case Bloom_Composite:
			return conpositePSO;
			break;
		}
		return bloomPSO;
	}


	void	Initialize(WinApp* winApp);

	//描画前処理
	void PreDraw();
	//描画後処理
	void PostDraw();

	~DirectXCommon(); // デストラクタを宣言

	/// <summary>
	/// 指定番号のCPUでスクリプタハンドルを取得する
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);

	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUDescriptorHandle(uint32_t index);

	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUDescriptorHandle(uint32_t index);

	/// <summary>
	/// 指定番号のGPUでスクリプタハンドルを取得する
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

	D3D12_GPU_DESCRIPTOR_HANDLE GetRTVGPUDescriptorHandle(uint32_t index);

	D3D12_GPU_DESCRIPTOR_HANDLE GetDSVGPUDescriptorHandle(uint32_t index);

	/// <summary>
/// SRVディスクリプタヒープをコマンドリストにセットする
/// </summary>
	void SetDescriptorHeaps(ID3D12DescriptorHeap* srvHeap);

	//getter
	ID3D12Device* GetDevice() const { return device_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }

	void CreateShaderCommon(PSO& pso);
	void CreateShader();

	void ReportLiveObjects();

	Microsoft::WRL::ComPtr<IDxcBlob> CompilesSharder(
		//CompilerするSharderファイルへのパス
		const std::wstring& filePath,
		//Compilerにする使用するProfile
		const wchar_t* profile);

	Microsoft::WRL::ComPtr<ID3D12Resource>CreateBufferResource(size_t sizeInBytes);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVBufferResource(size_t sizeInBytes, D3D12_RESOURCE_FLAGS flags);

	Microsoft::WRL::ComPtr<ID3D12Resource>CreateTextureResource(const DirectX::TexMetadata& metadata);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResourceRenderTexture(uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, const D3D12_CLEAR_VALUE* clearValue);

	void UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages);

	// 現在のFPSを取得
	float GetFPS() const { return fps_; }

	//最大SRV数(最大テクスチャ枚数)
	static const uint32_t kMaxSRVCount;

	// ====== PSO 用 Getter ======
	D3D12_BLEND_DESC GetBlendDesc() const;
	D3D12_RASTERIZER_DESC GetRasterizerDesc() const;
	D3D12_DEPTH_STENCIL_DESC GetDepthStencilDesc() const;

	DXGI_FORMAT GetRTVFormat() const { return rtvDesc.Format; }
	DXGI_FORMAT GetDSVFormat() const { return dsvDesc.Format; }

	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		UINT numDescripters, bool shaderVisible);

	Microsoft::WRL::ComPtr<ID3D12Resource>
		CreateDepthStencilResource(int32_t width, int32_t height);

	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
		const std::wstring& filePath,
		const wchar_t* profile
	);

	/// <summary>
/// 指定番号のCPUでスクリプタハンドルを取得する
/// </summary>
	static	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr < ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriporSize, uint32_t index);

	/// <summary>
	/// 指定番号のGPUでスクリプタハンドルを取得する
	/// </summary>
	static	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr < ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriporSize, uint32_t index);


	D3D12_RENDER_TARGET_VIEW_DESC GetRtvDesc() { return rtvDesc; }
	DXGI_SWAP_CHAIN_DESC1 GetSwapChainDesc() { return swapChainDesc; }

	void ExecuteCommandListAndWait();

	ID3D12CommandQueue* GetCommandQueue() const { return commandQueue.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator() const { return commandAllocator.Get(); }

private:

	void DeviceInitialize();
	void CommandInitialize();
	void SwapChainSpawn();
	void DepthBufferSpawn();
	void DethCriptorHeapSpawn();
	void RenderTargetViewInitialize();
	void DepthStencilViewInitialize();
	void FanceInitialize();
	void ViewPortInitialize();
	void SizeringInitialize();
	void DXCCompilerSpawn();   // ★正しい綴り（リンクエラー側）
	void DXCCompilierSpawn();  // ★今あるやつ（残す）
	//void ImGuiInitialize();

	//FPS固定初期化
	void InitializeFixFPS();
	//FPS固定更新
	void UpdateFixFPS();

private:

	//WindowsApi
	WinApp* winApp_ = nullptr;

	//DXGIファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;

	//コマンド関連の変数
	Microsoft::WRL::ComPtr < ID3D12CommandQueue> commandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	Microsoft::WRL::ComPtr < ID3D12CommandAllocator> commandAllocator = nullptr;

	//スワップチェーン
	Microsoft::WRL::ComPtr <IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	//スワップチェーンリソース
	std::array<Microsoft::WRL::ComPtr < ID3D12Resource>, 2 > swapChainResources;

	//各種デスクリプタヒープの生成
	uint32_t descriptorSizeRTV;
	uint32_t descriptorSizeDSV;

	//アインドバッファの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;

	//RTV用のヒープでディスクリプタの数は2。RTVはshader内で触るものではないので、ShaderVisibleはfalse
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	//SRV用のヒープでディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue

	//DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

	//設定するRTV
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 2> rtvHandles;

	//フェンスの設定
	Microsoft::WRL::ComPtr < ID3D12Fence> fence = nullptr;
	UINT64 fenceValue = 0;
	HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	//ビューポート
	D3D12_VIEWPORT viewport{};

	//シザー
	D3D12_RECT scissorRect{};

	//DXCの初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	IDxcIncludeHandler* includeHandler = nullptr;

	//記録時間
	std::chrono::steady_clock::time_point reference_;

	// 現在のFPS
	float fps_ = 0.0f;

	PSO psoModelParticle_;
	PSO bloomPSO;
	PSO downsamplePSO;
	PSO blurHPSO;
	PSO blurVPSO;
	PSO conpositePSO;
	PSO trailPSO;
	PSO computeParticlePSO;
	ShaderType shaderType_;

	void CommandListExecuteAndReset();

	public:

		void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
		void SetRenderTargetNoDepth(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);

		void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);

		void ClearDepthBuffer();

		void SetBackBuffer();

		void SetViewport(uint32_t width, uint32_t height);

		void Release();

};

