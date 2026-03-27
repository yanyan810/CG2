#include "DirectXCommon.h"
#include <cassert>
#include <dxgidebug.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")


using namespace Logger;
using namespace StringUtility;

const uint32_t DirectXCommon::kMaxSRVCount = 512;

/// <summary>
/// デスクリプタヒープの生成
/// </summary>
//DescriptorHeapの作成関数
Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>   DirectXCommon::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType,
	UINT numDescripters, bool shaderVisible) {

	//ディスクリプタヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType; // レンダーターゲットビュー用
	descriptorHeapDesc.NumDescriptors = numDescripters; // ダブルバッファように2つ。多くても別に損はない
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device_->CreateDescriptorHeap(
		&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	//ディスクリプタヒープが作れなかったので起動できない
	assert(SUCCEEDED(hr));

	return descriptorHeap;
}

/// <summary>
///  深度バッファリソースの設定
/// </summary>
Microsoft::WRL::ComPtr<ID3D12Resource>
DirectXCommon::CreateDepthStencilResource(int32_t width, int32_t height) {	//生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;//Textureの幅
	resourceDesc.Height = height;//Textureの高さ
	resourceDesc.MipLevels = 1;//mipmapの数
	resourceDesc.DepthOrArraySize = 1;//奥行きor配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//DepthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1;//サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2次元

	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//DepthStencilとして使う通知

	//理想するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;//VRAM上に作る
	//深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;//1.0f(最大値)でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//DepthStencilとして利用可能なフォーマット
	//Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = device_->CreateCommittedResource(
		&heapProperties,//Heapの設定
		D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定。特になし
		&resourceDesc,//Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE,//深度値を書き込む状態にしておく
		&depthClearValue,//Clear最適値
		IID_PPV_ARGS(&resource));//作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));

	resource->SetName(L"DepthStencil");

	return resource;
}


D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr < ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriporSize, uint32_t index) {
	//CPU側のディスクリプタハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriporSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr < ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriporSize, uint32_t index) {
	//CPU側のディスクリプタハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriporSize * index);
	return handleGPU;
}

// ---- CPUハンドル取得 ----


D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetRTVCPUDescriptorHandle(uint32_t index) {
	return GetCPUDescriptorHandle(rtvDescriptorHeap, descriptorSizeRTV, index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetDSVCPUDescriptorHandle(uint32_t index) {
	return GetCPUDescriptorHandle(dsvDescriptorHeap_, descriptorSizeDSV, index);
}

// ---- GPUハンドル取得 ----

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetRTVGPUDescriptorHandle(uint32_t index) {
	return GetGPUDescriptorHandle(rtvDescriptorHeap.Get(), descriptorSizeRTV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetDSVGPUDescriptorHandle(uint32_t index) {
	return GetGPUDescriptorHandle(dsvDescriptorHeap_.Get(), descriptorSizeDSV, index);
}

//CompileShader関数
Microsoft::WRL::ComPtr<IDxcBlob> DirectXCommon::CompilesSharder(
	//CompilerするSharderファイルへのパス
	const std::wstring& filePath,
	//Compilerにする使用するProfile
	const wchar_t* profile) {
	//これからシェーダーをコンパイルする旨をログにだす
	Log(ConvertString(std::format(L"Compile Shader: {}\n", filePath)));

	//hlslファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);

	//読めなかったら止める
	assert(SUCCEEDED(hr));

	//読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;//UTF8の文字コードであることを通知
	LPCWSTR arguments[] = {
	filePath.c_str(),///コンパイル対象のhlslファイル名
	L"-E", L"main",//エントリーポイントの指定。基本的にmain以外にはしない
	L"-T", profile,//ShaderProfileの設定
	L"-Zi",L"-Qembed_debug",//デバッグ用の情報を埋め込む
	L"-Od",//最適化を外しておく
	L"-Zpr",//メモリレイアウトは行優先
	};

	//実際にshederをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,//読み込んだファイル
		arguments,//コンパイルオプション
		_countof(arguments),//コンパイルオプションの数
		includeHandler,//includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult));//コンパイル結果
	//コンパイルエラーではなくdxcが起動できないなどの致命的な状況
	assert(SUCCEEDED(hr));

	//警告・エラーが出てたらログに出す
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());
		assert(false); // ← 本当にエラーがある時だけ止まる
	}

	//コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	//成功したログを出す
	Log(ConvertString(std::format(L"Compile Succeded,path:{},profile\n", filePath, profile)));
	//もう使わないリソースを解放
	shaderSource->Release();
	//実行用のバイナリを返却
	return shaderBlob;

}

Microsoft::WRL::ComPtr<ID3D12Resource>DirectXCommon::CreateBufferResource( size_t sizeInBytes) {
	//sizeInBytes = (sizeInBytes + 0xff) & ~0xff;

	// ヒープの設定
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	// リソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Microsoft::WRL::ComPtr<ID3D12Resource> buffer = nullptr;
	HRESULT hr = device_->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer));
	assert(SUCCEEDED(hr));

	// ここで名前を付ける（必要なら引数で名前渡す）
	buffer->SetName(L"GenericUploadBuffer");

	return buffer;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateUAVBufferResource(size_t sizeInBytes, D3D12_RESOURCE_FLAGS flags) {
	// 1. ヒープの設定 (GPU専用のDefault Heap)
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 2. リソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = flags; // 引数で渡されたフラグをセット

	// 3. リソースの作成
	// UAV用途の場合、初期状態は COMMON にしておき、使用直前に ResourceBarrier で遷移させるのが安全です
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer = nullptr;
	HRESULT hr = device_->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&buffer));

	assert(SUCCEEDED(hr));

	// デバッグ用に名前を付けておくと便利
	buffer->SetName(L"UAVBufferResource");

	return buffer;
}

Microsoft::WRL::ComPtr<ID3D12Resource>DirectXCommon::CreateTextureResource(const DirectX::TexMetadata& metadata) {

	//metadataをもとにResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);//Textureの幅
	resourceDesc.Height = UINT(metadata.height);;//高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);//mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);;//奥行きor配列Textureの配列数
	resourceDesc.Format = metadata.format;//TextureのFormat
	resourceDesc.SampleDesc.Count = 1;//サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);//Textureの次元数。普段使っているのは2次元

	//利用するHeapの設定。非常に特殊な運用。02_04exで一般的なケースがある
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;//細かい設定を行う
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;//writeBackポリシーでCPUアクセス可能
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;//プロセッサの近くに配置

	//Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device_->CreateCommittedResource(
		&heapProperties,//Heapの設定
		D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定。特になし
		&resourceDesc,//Resourceの設定
		D3D12_RESOURCE_STATE_COPY_DEST,//初回のResourceState。Textureは基本読むだけ
		nullptr,//Clear最適地。使わないのでnullptr
		IID_PPV_ARGS(&resource));//作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));

	resource->SetName(L"TextureResource");

	return resource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateTextureResourceRenderTexture(uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, const D3D12_CLEAR_VALUE* clearValue)
{
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = flags;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = device_->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		clearValue,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));

	return resource;
}

// DirectXCommon.cpp
void DirectXCommon::UploadTextureData(
	const Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
	const DirectX::ScratchImage& mipImages)
{
	// 1) subresource 配列を用意
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device_.Get(),
		mipImages.GetImages(),
		mipImages.GetImageCount(),
		mipImages.GetMetadata(),
		subresources);

	// 2) 中間バッファを作成（※この lifetime が超重要）
	const UINT numSubresources = UINT(subresources.size());
	const UINT64 intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, numSubresources);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediate = CreateBufferResource(intermediateSize);

	// 3) UpdateSubresources（リソースは COPY_DEST で作ってある想定）
	UpdateSubresources(commandList.Get(),
		texture.Get(),
		intermediate.Get(),
		0, 0,
		numSubresources,
		subresources.data());

	// 4) GENERIC_READ へ遷移
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);

	// 5) ★ここが肝：実行してフェンス待ちするまで intermediate を解放しない
	HRESULT hr = commandList->Close();                         assert(SUCCEEDED(hr));
	ID3D12CommandList* lists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, lists);

	fenceValue++;
	hr = commandQueue->Signal(fence.Get(), fenceValue);        assert(SUCCEEDED(hr));
	if (fence->GetCompletedValue() < fenceValue) {
		hr = fence->SetEventOnCompletion(fenceValue, fenceEvent); assert(SUCCEEDED(hr));
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// 6) 実行完了後にようやく中間バッファが自動解放されても OK
	hr = commandAllocator->Reset();                            assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator.Get(), nullptr);  assert(SUCCEEDED(hr));
}

// DirectXCommon 側に SrvManager* を持たせる or 引数で渡す
void DirectXCommon::SetDescriptorHeaps(ID3D12DescriptorHeap* srvHeap)
{
	ID3D12DescriptorHeap* heaps[] = { srvHeap };
	commandList->SetDescriptorHeaps(1, heaps);
}


void DirectXCommon::Initialize(WinApp* winApp) {

	//NULL検出
	assert(winApp);

	//メンバ変数に記録
	this->winApp_ = winApp;
	//FPS固定初期化
	InitializeFixFPS();

	DeviceInitialize();
	CommandInitialize();
	SwapChainSpawn();
	DepthBufferSpawn();
	DethCriptorHeapSpawn();
	RenderTargetViewInitialize();
	DepthStencilViewInitialize();
	FanceInitialize();
	//ViewPortInitialize();
	//SizeringInitialize();
	DXCCompilierSpawn();
//	ImGuiInitialize();

	HRESULT hr = commandList->Close();                                // いったん閉じる（開いていてもOK）
	hr = commandAllocator->Reset();                                    // アロケータをリセット
	hr = commandList->Reset(commandAllocator.Get(), nullptr);          // 開き直す（←重要）

	CreateShader();

}

DirectXCommon::~DirectXCommon() {
	if (fenceEvent) {
		CloseHandle(fenceEvent);
		fenceEvent = nullptr;
	}
}


void DirectXCommon::DeviceInitialize() {

	HRESULT hr;


#ifdef _DEBUG

	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		//デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();
		//さらにGPU側でもチェックを行えるようにする
	//	debugController->SetEnableGPUBasedValidation(TRUE);
	}

#endif

	//HRESULTWindows系のエラーコードであり、
	// 関数が成功したかどうかをSUCCEEDEDマクロで判定できる
	hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
	//初期化の根本的なエラーが出た場合はプログラムが間違っているか、どうにもできない場合が多いのでasserにしておく
	assert(SUCCEEDED(hr));

	//使用するアダプタ用の変数,最初にnullptrを入れておく
	Microsoft::WRL::ComPtr < IDXGIAdapter4> useAdapter = nullptr;
	//良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; i++) {
		//アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));//取得できないのは一大事
		//ソフトウェアのアダプタでなければ採用
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			//採用したアダプタの情報をログに出力。wstringの方なので注意
			Log(ConvertString(std::format(L"Use Adapter: {}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;//ソフトウェアアダプタの場合は見なかったことにする
	}
	//適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);

	//D3D12Deviceの生成

	//機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); i++) {
		//採用したアダプターでデバイス生成
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		//生成できたのでログ出力を行ってループを抜ける
		if (SUCCEEDED(hr)) {
			Log(std::format("Use Feature Level: {}\n", featureLevelStrings[i]));
			break;
		}
	}
	//デバイスの生成がうまくいかなかった場合
	assert(device_ != nullptr);
	Log("Complete create D3D12Device!!!\n");//初期化のログを出す

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		// 警告で止めるかどうかは必要に応じて
		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		infoQueue->PushStorageFilter(&filter);
	}
#endif

}

void DirectXCommon::CommandInitialize() {

	HRESULT hr;

	//コマンドアロケータを生成する

	hr = device_->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator));
	//コマンドアロケータの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドリストを生成する

	hr = device_->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&commandList));
	//コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドキューを生成する

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device_->CreateCommandQueue(&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue));
	//コマンドキュー生成が失敗した場合
	assert(SUCCEEDED(hr));
}

void DirectXCommon::SwapChainSpawn() {

	HRESULT hr;

	// SwapChainを生成する
	swapChainDesc.Width = WinApp::kClientWidth;   // 幅
	swapChainDesc.Height = WinApp::kClientHeight; // 高さ
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // カラー形式
	swapChainDesc.SampleDesc.Count = 1;              // マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画対象として使う
	swapChainDesc.BufferCount = 2;                   // ダブルバッファ            // ウィンドウモードで起動
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // フリップ後は破棄
	//コマンドキュー,ウィンドウハンドル,設定を渡して生成する
	Microsoft::WRL::ComPtr<IDXGISwapChain1> tempSwapChain;
	hr = dxgiFactory_->CreateSwapChainForHwnd(
		commandQueue.Get(), winApp_->GetHwnd(), &swapChainDesc, nullptr, nullptr, &tempSwapChain);
	assert(SUCCEEDED(hr));

	// IDXGISwapChain4 へアップキャスト
	hr = tempSwapChain.As(&swapChain);
	assert(SUCCEEDED(hr));


}


void DirectXCommon::DepthBufferSpawn() {

	// 深度ステンシルリソースを生成（メンバに代入）
	depthStencilResource_ = CreateDepthStencilResource(
		WinApp::kClientWidth, WinApp::kClientHeight);

	// DSV用のヒープを作成（メンバに代入）
	dsvDescriptorHeap_ = CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	// DSVビューの作成
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	device_->CreateDepthStencilView(
		depthStencilResource_.Get(),
		&dsvDesc,
		dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());

	Log("Depth buffer and DSV created successfully.\n");
}


void DirectXCommon::DethCriptorHeapSpawn() {

	// Descriptor サイズ取得

	descriptorSizeRTV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// RTVヒープ作成（2個）
	rtvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	
}

void DirectXCommon::RenderTargetViewInitialize() {
	HRESULT hr;

	// バックバッファ取得（2枚分）
	for (UINT i = 0; i < 2; i++) {
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainResources[i]));
		assert(SUCCEEDED(hr));
	}

	// ★ RTVフォーマットを「スワップチェインと揃える」
	//    → DXGI_FORMAT_R8G8B8A8_UNORM が安全
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// RTV作成
	for (UINT i = 0; i < 2; ++i) {
		rtvHandles[i] = GetCPUDescriptorHandle(rtvDescriptorHeap, descriptorSizeRTV, i);
		device_->CreateRenderTargetView(
			swapChainResources[i].Get(),
			&rtvDesc,
			rtvHandles[i]
		);
	}

	Log("RenderTargetViewInitialize: created RTV for both buffers.\n");
}


void DirectXCommon::DepthStencilViewInitialize() {

	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	device_->CreateDepthStencilView(
		depthStencilResource_.Get(),
		&dsvDesc,
		dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());

	// RTVとDSVをパイプラインに設定
	dsvHandle = GetCPUDescriptorHandle(dsvDescriptorHeap_, descriptorSizeDSV, 0);

	//commandList->OMSetRenderTargets(1, &rtvHandles[0], FALSE, &dsvHandle);

	Log("Depth Stencil View initialized successfully.\n");
}

void DirectXCommon::FanceInitialize() {

	HRESULT hr;

	fenceValue = 0;
	hr = device_->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(fenceEvent != nullptr);

}

void DirectXCommon::ViewPortInitialize() {

	//ビューポート
	//クラアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = WinApp::kClientWidth;
	viewport.Height = WinApp::kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

}

void DirectXCommon::SizeringInitialize() {

	//シザー矩形
	//基本的にビューポートと同じ矩形が構成される
	scissorRect.left = 0;
	scissorRect.right = WinApp::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WinApp::kClientHeight;

}

void DirectXCommon::DXCCompilierSpawn() {

	HRESULT hr;

	//dxCompireを初期化
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	//現時点でincludeしないが、includeに対応するために設定しておく	
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));


}

void DirectXCommon::DXCCompilerSpawn() {
	// ★既存のミス綴り実装へ転送
	DXCCompilierSpawn();
}
//
//void DirectXCommon::ImGuiInitialize() {
//
//
//	//ImGuiの初期化。
//	//こういうもの
//	IMGUI_CHECKVERSION();
//	ImGui::CreateContext();
//	ImGui::StyleColorsDark();
//	ImGui_ImplWin32_Init(winApp_->GetHwnd());
//	ImGui_ImplDX12_Init(device_.Get(),
//		swapChainDesc.BufferCount,
//		rtvDesc.Format,
//		srvDescriptorHeap.Get(),
//		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
//		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
//
//}

void DirectXCommon::InitializeFixFPS() {

	reference_ = std::chrono::steady_clock::now();
	fps_ = 0.0f;

}

void DirectXCommon::UpdateFixFPS() {

	using namespace std::chrono;

	//1/60秒ぴったりの時間
	const microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
	//1/60秒よりわずかに短い時間
	const microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

	// 現在時間と前フレームからの経過時間
	auto now = steady_clock::now();
	auto elapsed = duration_cast<microseconds>(now - reference_);

	// まだほとんど時間が経っていなければスリープして60fpsに近づける
	if (elapsed < kMinCheckTime) {
		while (steady_clock::now() - reference_ < kMinTime) {
			std::this_thread::sleep_for(microseconds(1));
		}
		// スリープ後の正確な経過時間を測り直す
		now = steady_clock::now();
		elapsed = duration_cast<microseconds>(now - reference_);
	}

	// 次フレームの基準時間を更新
	reference_ = now;

	// FPS 計算（経過秒の逆数）
	if (elapsed.count() > 0) {
		float elapsedSec = static_cast<float>(elapsed.count()) / 1'000'000.0f;
		fps_ = 1.0f / elapsedSec;
	}
}


void DirectXCommon::PreDraw() {

	// これから書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	// TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	// 今回のバリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	// Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	// バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	// 遷移前(現在)のResourceState
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	// 遷移後のResourceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	// TransitionBarrierを張る
	commandList->ResourceBarrier(1, &barrier);
	// 描画先のRTVとDSVを設定する
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
	// 指定した色で画面全体をクリアする
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	//float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
	// 指定して深度で画面全体をクリアする
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	SetViewport(WinApp::kClientWidth, WinApp::kClientHeight);
}

void DirectXCommon::PostDraw() {

	// これから書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
	// TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	// 今回のバリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	// Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	// バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	// 遷移前(現在)のResourceState
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	// 遷移後のResourceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	// TranssitionBarrierを張る
	commandList->ResourceBarrier(1, &barrier);

	CommandListExecuteAndReset();

}


void DirectXCommon::ReportLiveObjects()
{
#if _DEBUG
	Microsoft::WRL::ComPtr<ID3D12DebugDevice> debugDevice;
	if (SUCCEEDED(device_.As(&debugDevice))) {
		// 詳細レポート（名前も出る）
		debugDevice->ReportLiveDeviceObjects(
			D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL
		);
	}
#endif
}

// === BlendDesc ===
D3D12_BLEND_DESC DirectXCommon::GetBlendDesc() const {
	D3D12_BLEND_DESC desc{};
	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = FALSE;

	const D3D12_RENDER_TARGET_BLEND_DESC defaultBlend = {
		TRUE, FALSE,
		D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL
	};

	for (int i = 0; i < 8; ++i) {
		desc.RenderTarget[i] = defaultBlend;
	}
	return desc;
}

// === Rasterizer ===
D3D12_RASTERIZER_DESC DirectXCommon::GetRasterizerDesc() const {
	D3D12_RASTERIZER_DESC desc{};
	desc.FillMode = D3D12_FILL_MODE_SOLID;
	desc.CullMode = D3D12_CULL_MODE_BACK;   // CullMode::None でも OK
	desc.FrontCounterClockwise = FALSE;
	desc.DepthClipEnable = TRUE;
	return desc;
}

// === DepthStencil ===
D3D12_DEPTH_STENCIL_DESC DirectXCommon::GetDepthStencilDesc() const {
	D3D12_DEPTH_STENCIL_DESC desc{};
	desc.DepthEnable = TRUE;
	desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	desc.StencilEnable = FALSE;
	return desc;
}

Microsoft::WRL::ComPtr<IDxcBlob> DirectXCommon::CompileShader(
	const std::wstring& filePath,
	const wchar_t* profile
) {
	return CompilesSharder(filePath, profile);
}

void DirectXCommon::CommandListExecuteAndReset()
{

	// コマンドリストの内容を確定させるすべてのコマンドを積んでからCloseすること
	HRESULT hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// GPUにコマンドリストの実行を行わせる
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, commandLists);
	//GPUとOSに画面の交換を行うよう通知する
	swapChain->Present(1, 0);
	// fenceの値を更新
	fenceValue++;
	// GPUがここまでたどり着いたときに、Fenceの値を指定して値に代入するようにSignalを送る
	commandQueue->Signal(fence.Get(), fenceValue);
	// Fenceの値が指定したSignal値にたどり着いているか確認する
	// GetCompletedValueの初期値はFence作成時に渡した初期値
	if (fence->GetCompletedValue() < fenceValue) {
		// 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		// イベント待つ
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// FPS固定
	UpdateFixFPS();

	// 次のフレーム用のコマンドリストを準備
	hr = commandAllocator->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

void DirectXCommon::SetRenderTarget(
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle
	//D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle
) {

	commandList->OMSetRenderTargets(
		1,
		&rtvHandle,
		false,
		&dsvHandle
	);
}

void DirectXCommon::SetRenderTargetNoDepth(
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle
) {
	commandList->OMSetRenderTargets(
		1,
		&rtvHandle,
		false,
		nullptr
	);
}

void DirectXCommon::ClearRenderTarget(
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle
) {
	float clearColor[4] = { 0, 0, 0, 1 };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void DirectXCommon::ClearDepthBuffer() {
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void DirectXCommon::SetBackBuffer() {

	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHandles[backBufferIndex];
	D3D12_CPU_DESCRIPTOR_HANDLE dsv =
		dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	commandList->OMSetRenderTargets(
		1,
		&rtv,
		false,
		&dsv
	);
}

void DirectXCommon::SetViewport(uint32_t width, uint32_t height)
{
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = FLOAT(width);
	viewport.Height = FLOAT(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = LONG(width);
	scissorRect.bottom = LONG(height);

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
}

void DirectXCommon::CreateShaderCommon(PSO& pso)
{
	// 1. 各タイプごとのシェーダーパスとルートシグネチャ初期化
	switch (pso.shaderType_)
	{
	case kModelParticle:
		pso.root_.InitalizeForModelParticle();
		pso.vsFilePath_ = L"resources/shaders/ModelParticle.VS.hlsl";
		pso.psFilePath_ = L"resources/shaders/ModelParticle.PS.hlsl";
		break;
	case kComputeParticleUpdate:
		pso.root_.InitializeForComputeParticle(); // UAV(u0)などを含むルートシグネチャ
		pso.csFilePath_ = L"resources/shaders/ParticleUpdate.CS.hlsl";
		break;
	case kShadow:
		pso.root_.InitalizeForShadow();
		pso.vsFilePath_ = L"resources/shaders/Shadow.VS.hlsl";
		pso.psFilePath_ = L"";
		break;
	case kPostEffect:
		pso.root_.InitializeForPostEffect();
		pso.vsFilePath_ = L"resources/shaders/FullScreen.VS.hlsl";
		switch (pso.postEffectType_) {
		case Bloom_Extract:   pso.psFilePath_ = L"resources/shaders/BloomExtract.PS.hlsl"; break;
		case Bloom_Downsample:pso.psFilePath_ = L"resources/shaders/BloomDownsample.PS.hlsl"; break;
		case Bloom_BlurH:      pso.psFilePath_ = L"resources/shaders/BloomBlurH.PS.hlsl"; break;
		case Bloom_BlurV:      pso.psFilePath_ = L"resources/shaders/BloomBlurV.PS.hlsl"; break;
		case Bloom_Composite:  pso.psFilePath_ = L"resources/shaders/Composite.PS.hlsl"; break;
		}
		break;
	case kTrail:
		pso.root_.InitalizeForTrail(); // 上で作った関数
		pso.vsFilePath_ = L"resources/shaders/Trail.VS.hlsl";
		pso.psFilePath_ = L"resources/shaders/Trail.PS.hlsl";
		break;
	default: assert(false); break;
	}

	// 2. ルートシグネチャ生成
	pso.root_.Create(device_);
	// 3. コンパイルとPSO生成の分岐
	if (pso.shaderType_ == kComputeParticleUpdate) {
		// --- Compute Pipeline の生成 ---
		pso.computeShaderBlob_ = CompileShader(pso.csFilePath_, L"cs_6_0");
		assert(pso.computeShaderBlob_ != nullptr);

		pso.computeDesc_.pRootSignature = pso.root_.GetSignature().Get();
		pso.computeDesc_.CS = {
			pso.computeShaderBlob_->GetBufferPointer(),
			pso.computeShaderBlob_->GetBufferSize()
		};
		pso.computeDesc_.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		HRESULT hr = device_->CreateComputePipelineState(&pso.computeDesc_, IID_PPV_ARGS(&pso.computeState_));
		assert(SUCCEEDED(hr));
	} else {
		pso.vertexShaderBlob_ = CompileShader(pso.vsFilePath_, L"vs_6_0");
		assert(pso.vertexShaderBlob_ != nullptr);

		pso.pixelShaderBlob_ = nullptr;
		if (pso.shaderType_ != kShadow && !pso.psFilePath_.empty()) {
			pso.pixelShaderBlob_ = CompileShader(pso.psFilePath_, L"ps_6_0");
			assert(pso.pixelShaderBlob_ != nullptr);
		}

		// 4. グラフィックスパイプライン記述子の初期化
		ZeroMemory(&pso.graphicsDesc_, sizeof(pso.graphicsDesc_));

		// --- 旧 State クラスの処理をここに統合 ---

		// [RasterizerState] の設定
		pso.graphicsDesc_.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		pso.graphicsDesc_.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

		// [DepthStencilState] のデフォルト設定
		pso.graphicsDesc_.DepthStencilState.DepthEnable = TRUE;
		if (pso.shaderType_ == kModelParticle) {
			// 深度テストは行うが、深度バッファへの書き込みは行わない
			pso.graphicsDesc_.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		} else {
			// 通常のモデルは書き込む
			pso.graphicsDesc_.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		}
		pso.graphicsDesc_.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		// [BlendState] の設定
		pso.graphicsDesc_.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		if (pso.postEffectType_ == Bloom_Composite || pso.shaderType_ == kModelParticle || pso.shaderType_ == kTrail) {
			pso.graphicsDesc_.BlendState.RenderTarget[0].BlendEnable = TRUE;
			pso.graphicsDesc_.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			pso.graphicsDesc_.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			pso.graphicsDesc_.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			pso.graphicsDesc_.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
			pso.graphicsDesc_.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			pso.graphicsDesc_.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		} else {
			pso.graphicsDesc_.BlendState.RenderTarget[0].BlendEnable = FALSE;
		}

		// --- 統合ここまで ---

		// 5. 個別設定の上書き (Shadow / PostEffect / Normal)
		pso.graphicsDesc_.pRootSignature = pso.root_.GetSignature().Get();
		pso.graphicsDesc_.VS = { pso.vertexShaderBlob_->GetBufferPointer(), pso.vertexShaderBlob_->GetBufferSize() };
		if (pso.pixelShaderBlob_) {
			pso.graphicsDesc_.PS = { pso.pixelShaderBlob_->GetBufferPointer(), pso.pixelShaderBlob_->GetBufferSize() };
		}

		if (pso.shaderType_ == kShadow) {
			pso.graphicsDesc_.NumRenderTargets = 0;
			pso.graphicsDesc_.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			// Shadow用に比較関数を調整（必要に応じて）
			pso.graphicsDesc_.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			//pso.inputDesc_.Initialize();
			//pso.graphicsDesc_.InputLayout = pso.inputDesc_.GetLayout();
		} else if (pso.shaderType_ == kPostEffect) {
			pso.graphicsDesc_.NumRenderTargets = 1;
			pso.graphicsDesc_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			pso.graphicsDesc_.DSVFormat = DXGI_FORMAT_UNKNOWN;
			pso.graphicsDesc_.DepthStencilState.DepthEnable = FALSE;
			pso.graphicsDesc_.InputLayout = { nullptr, 0 };
		} else if (pso.shaderType_ == kTrail) {
			pso.graphicsDesc_.NumRenderTargets = 1;
			pso.graphicsDesc_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			pso.graphicsDesc_.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

			// ★ 軌跡用の特殊設定
			pso.graphicsDesc_.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 両面描画
			pso.graphicsDesc_.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 深度は塗らない

			// 0: 座標 (POSITION)
			pso.elementDescs_[0].SemanticName = "POSITION";
			pso.elementDescs_[0].SemanticIndex = 0;
			pso.elementDescs_[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // Vector3
			pso.elementDescs_[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

			// 1: 頂点カラー (COLOR) ★これが重要！
			pso.elementDescs_[1].SemanticName = "COLOR";
			pso.elementDescs_[1].SemanticIndex = 0;
			pso.elementDescs_[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // Vector4 (RGBA)
			pso.elementDescs_[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

			// 2: UV座標 (TEXCOORD)
			pso.elementDescs_[2].SemanticName = "TEXCOORD";
			pso.elementDescs_[2].SemanticIndex = 0;
			pso.elementDescs_[2].Format = DXGI_FORMAT_R32G32_FLOAT; // Vector2
			pso.elementDescs_[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

			pso.layout_.pInputElementDescs = pso.elementDescs_;
			pso.layout_.NumElements = 3; // 座標、色、UV の 3つ

			pso.graphicsDesc_.InputLayout = pso.layout_;
		} else {
			pso.graphicsDesc_.NumRenderTargets = 1;
			pso.graphicsDesc_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			pso.graphicsDesc_.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

			pso.elementDescs_[0].SemanticName = "POSITION";
			pso.elementDescs_[0].SemanticIndex = 0;
			pso.elementDescs_[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			pso.elementDescs_[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			pso.elementDescs_[1].SemanticName = "TEXCOORD";
			pso.elementDescs_[1].SemanticIndex = 0;
			pso.elementDescs_[1].Format = DXGI_FORMAT_R32G32_FLOAT;
			pso.elementDescs_[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			pso.elementDescs_[2].SemanticName = "NORMAL";
			pso.elementDescs_[2].SemanticIndex = 0;
			pso.elementDescs_[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			pso.elementDescs_[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

			pso.layout_.pInputElementDescs = pso.elementDescs_;
			pso.layout_.NumElements = _countof(pso.elementDescs_);

			pso.graphicsDesc_.InputLayout = pso.layout_;
		}

		// 6. 残りの共通設定
		pso.graphicsDesc_.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pso.graphicsDesc_.SampleDesc.Count = 1;
		pso.graphicsDesc_.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		// 7. PSO生成
		HRESULT hr = device_->CreateGraphicsPipelineState(&pso.graphicsDesc_, IID_PPV_ARGS(&pso.graphicsState_));
		assert(SUCCEEDED(hr));
	}
}

void DirectXCommon::CreateShader()
{

	bloomPSO.shaderType_ = kPostEffect;
	bloomPSO.postEffectType_ = Bloom_Extract;
	blurHPSO.shaderType_ = kPostEffect;
	blurHPSO.postEffectType_ = Bloom_BlurH;
	blurVPSO.shaderType_ = kPostEffect;
	blurVPSO.postEffectType_ = Bloom_BlurV;
	conpositePSO.shaderType_ = kPostEffect;
	conpositePSO.postEffectType_ = Bloom_Composite;
	downsamplePSO.shaderType_ = kPostEffect;
	downsamplePSO.postEffectType_ = Bloom_Downsample;

	psoModelParticle_.shaderType_ = kModelParticle;
	trailPSO.shaderType_ = kTrail;

	computeParticlePSO.shaderType_ = kComputeParticleUpdate;

	CreateShaderCommon(bloomPSO);
	CreateShaderCommon(blurHPSO);
	CreateShaderCommon(blurVPSO);
	CreateShaderCommon(conpositePSO);
	CreateShaderCommon(downsamplePSO);
	CreateShaderCommon(psoModelParticle_);
	CreateShaderCommon(trailPSO);
	CreateShaderCommon(computeParticlePSO);
}

void DirectXCommon::ExecuteCommandListAndWait()
{
	// Close
	commandList->Close();

	// 実行
	ID3D12CommandList* lists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, lists);

	// Fence
	fenceValue++;
	commandQueue->Signal(fence.Get(), fenceValue);
	if (fence->GetCompletedValue() < fenceValue) {
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// Reset
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);
}

void DirectXCommon::Release() {

	bloomPSO.root_.GetSignatureBlob()->Release();
	if (bloomPSO.root_.GetErrorBlob()) {
		bloomPSO.root_.GetErrorBlob()->Release();
	}
	bloomPSO.pixelShaderBlob_->Release();
	bloomPSO.vertexShaderBlob_->Release();

	downsamplePSO.root_.GetSignatureBlob()->Release();
	if (downsamplePSO.root_.GetErrorBlob()) {
		downsamplePSO.root_.GetErrorBlob()->Release();
	}
	downsamplePSO.pixelShaderBlob_->Release();
	downsamplePSO.vertexShaderBlob_->Release();

	blurHPSO.root_.GetSignatureBlob()->Release();
	if (blurHPSO.root_.GetErrorBlob()) {
		blurHPSO.root_.GetErrorBlob()->Release();
	}
	blurHPSO.pixelShaderBlob_->Release();
	blurHPSO.vertexShaderBlob_->Release();

	blurVPSO.root_.GetSignatureBlob()->Release();
	if (blurVPSO.root_.GetErrorBlob()) {
		blurVPSO.root_.GetErrorBlob()->Release();
	}
	blurVPSO.pixelShaderBlob_->Release();
	blurVPSO.vertexShaderBlob_->Release();

	conpositePSO.root_.GetSignatureBlob()->Release();
	if (conpositePSO.root_.GetErrorBlob()) {
		conpositePSO.root_.GetErrorBlob()->Release();
	}
	conpositePSO.pixelShaderBlob_->Release();
	conpositePSO.vertexShaderBlob_->Release();

	psoModelParticle_.root_.GetSignatureBlob()->Release();
	if (psoModelParticle_.root_.GetErrorBlob()) {
		psoModelParticle_.root_.GetErrorBlob()->Release();
	}
	psoModelParticle_.pixelShaderBlob_->Release();
	psoModelParticle_.vertexShaderBlob_->Release();

	computeParticlePSO.root_.GetSignatureBlob()->Release();
	if (computeParticlePSO.root_.GetErrorBlob()) {
		computeParticlePSO.root_.GetErrorBlob()->Release();
	}
}