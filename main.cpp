#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <locale>
#include <codecvt>
#include <strsafe.h>
#include <DbgHelp.h> // Add this include to define MINIDUMP_EXCEPTION_INFORMATION  

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib") // Link with Dbghelp.lib to use MiniDumpWriteDump


//クライアント領域サイズ
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;

//ウィンドウサイズを表す構造体にクライアント領域を入れる
RECT wrc = { 0, 0, kClientWidth, kClientHeight };

//ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対して,アプリの終了を伝える 
		PostQuitMessage(0);
		return 0;

	}

	//標準メッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}

// std::wstring -> std::string の変換
std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return {};
	}

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string result(size_needed - 1, 0); // -1 to exclude null terminator
	WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, &result[0], size_needed, nullptr, nullptr);
	return result;
}

// std::string -> std::wstring の変換
std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return {};
	}

	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	std::wstring result(size_needed - 1, 0); // -1 to exclude null terminator
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size_needed);
	return result;
}

// ダンプファイルを生成する関数
LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	// 時刻を取得して、時刻付き名称にしたファイルを作成。Dumpsディレクトリに出力
	SYSTEMTIME time;
	GetLocalTime(&time);
	TCHAR filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	// processId に GetCurrentProcessId() を渡し、threadId には GetCurrentThreadId() を渡す
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();

	// MiniDumpWriteDump に渡す構造体
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation = { 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;

	// MiniDumpWriteDump 関数を呼び出してダンプを作成
	MiniDumpWriteDump(GetCurrentProcess(),
		processId, dumpFileHandle,
		MiniDumpNormal, &minidumpInformation,
		nullptr, nullptr);

	return EXCEPTION_EXECUTE_HANDLER;


}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {



	// CrashHandlerを登録
	SetUnhandledExceptionFilter(ExportDump);

	WNDCLASS wc{};

	//ウィンドプロシージャ
	wc.lpfnWndProc = WindowProc;

	//ウィンドウクラス名
	wc.lpszClassName = L"CG2WindowClass";

	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);

	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//ウィンドウクラスを登録する
	RegisterClass(&wc);

	//クライアント領域をもとに実際のサイズにwcrを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hwnd = CreateWindow(
		wc.lpszClassName, //ウィンドウクラス名
		L"CG2", //ウィンドウ名
		WS_OVERLAPPEDWINDOW, //ウィンドウスタイル
		CW_USEDEFAULT, //x座標
		CW_USEDEFAULT, //y座標
		wrc.right - wrc.left, //幅
		wrc.bottom - wrc.top, //高さ
		nullptr, //親ウィンドウハンドル
		nullptr, //メニューハンドル
		wc.hInstance, //インスタンスハンドル
		nullptr); //オプション

	//ウィンドウを表示する
	ShowWindow(hwnd, SW_SHOW);

#ifdef _DEBUG

	ID3D12Debug1* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		//デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();
		//さらにGPU側でもチェックを行えるようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}

#endif

	MSG msg{};

	//ログのディレクトリを用意
	std::filesystem::create_directory("logs");

	//現在時刻を取得(UTI時刻)
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	//ログファイルの名前にコンマ何秒はいらないので削って秒にする
	std::chrono::time_point <std::chrono::system_clock, std::chrono::seconds>
		nowSecond = std::chrono::time_point_cast<std::chrono::seconds>(now);

	//日本時間(PCの設定時間)に変換
	std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSecond };

	//formatを使って年月日_時分秒の文字列に変換
	std::string dataStrigng = std::format("{:%Y%m%d_%H%M%S}", localTime);
	//時刻を使ってファイル名を決定
	std::string localFilePath = std::string("logs/") + dataStrigng + ".log";
	//ファイルを作って書き込み準備
	std::ofstream logStream(localFilePath);

	//DXGIファクトリーの生成
	IDXGIFactory7* dxgiFactory = nullptr;

	//HRESULTWindows系のエラーコードであり、
	// 関数が成功したかどうかをSUCCEEDEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	//初期化の根本的なエラーが出た場合はプログラムが間違っているか、どうにもできない場合が多いのでasserにしておく
	assert(SUCCEEDED(hr));

	//使用するアダプタ用の変数,最初にnullptrを入れておく
	IDXGIAdapter4* useAdapter = nullptr;
	//良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
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
	ID3D12Device* device = nullptr;
	//機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); i++) {
		//採用したアダプターでデバイス生成
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
		//生成できたのでログ出力を行ってループを抜ける
		if (SUCCEEDED(hr)) {
			Log(std::format("Use Feature Level: {}\n", featureLevelStrings[i]));
			break;
		}
	}
	//デバイスの精がうまくいかなかった場合
	assert(device != nullptr);
	Log("Complete create D3D12Device!!!\n");//初期化のログを出す

#ifdef _DEBUG

	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(
		IID_PPV_ARGS(&infoQueue)))) {
		//やばいエラーの時止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		//エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		//警告時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		//解放
		infoQueue->Release();

		//抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		//指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);

	}


#endif 


	//コマンドキューを生成する
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc,
		IID_PPV_ARGS(&commandQueue));
	//コマンドキュー生成が失敗した場合
	assert(SUCCEEDED(hr));

	//コマンドアロケータを生成する
	ID3D12CommandAllocator* commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator));
	//コマンドアロケータの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドリストを生成する
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator,
		nullptr,
		IID_PPV_ARGS(&commandList));
	//コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// SwapChainを生成する
	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = kClientWidth;   // 幅
	swapChainDesc.BufferDesc.Height = kClientHeight; // 高さ
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // カラー形式
	swapChainDesc.SampleDesc.Count = 1;              // マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画対象として使う
	swapChainDesc.BufferCount = 2;                   // ダブルバッファ
	swapChainDesc.OutputWindow = hwnd;               // 出力するウィンドウのハンドル
	swapChainDesc.Windowed = true;                   // ウィンドウモードで起動
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // フリップ後は破棄
	//コマンドキュー,ウィンドウハンドル,設定を渡して生成する
	hr = dxgiFactory->CreateSwapChain(
		commandQueue, &swapChainDesc, reinterpret_cast<IDXGISwapChain**>(&swapChain));
	assert(SUCCEEDED(hr));

	//ディスクリプタヒープの生成
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビュー用
	rtvDescriptorHeapDesc.NumDescriptors = 2; // ダブルバッファように2つ。多くても別に損はない
	hr = device->CreateDescriptorHeap(
		&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	//ディスクリプタヒープが作れなかったので起動できない
	assert(SUCCEEDED(hr));

	//SwapChainからResourcesを取得する
	// SwapChainからResourcesを取得する
	ID3D12Resource* swapChainResources[2] = { nullptr };
	for (int i = 0; i < 2; i++) {
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainResources[i]));
		assert(SUCCEEDED(hr));
	}


	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャとして書き込む

	// ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	// 一つ目のRTV
	rtvHandles[0] = rtvStartHandle; // 先頭ハンドルを設定
	device->CreateRenderTargetView(
		swapChainResources[0], // リソース
		&rtvDesc,              // 設定
		rtvHandles[0]);        // ディスクリプタの先頭

	// 二つ目のディスクリプタハンドルを作成
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// 二つ目のRTV
	device->CreateRenderTargetView(
		swapChainResources[1], // リソース
		&rtvDesc,              // 設定
		rtvHandles[1]);        // ディスクリプタの先頭


	ID3D12Fence* fence = nullptr;
	UINT64 fenceValue = 0;
	HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	//ウィンドウボタンのxボタンが押されえるまでループ
	while (msg.message != WM_QUIT) {
		//windowにメッセージが来てたら最優先で処理させる
       
		/*hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
		assert(SUCCEEDED(hr));*/
		hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
		assert(SUCCEEDED(hr));


		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			//ゲームの更新処理

			// バッファのインデックスを取得
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

			// === リソースバリア: Present → RenderTarget ===
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = swapChainResources[backBufferIndex];
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			commandList->ResourceBarrier(1, &barrier);

			// 描画先のRTVを設定
			commandList->OMSetRenderTargets(1,
				&rtvHandles[backBufferIndex],
				FALSE, nullptr);

			// 画面をクリアする（青っぽい色）
			float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
			commandList->ClearRenderTargetView(
				rtvHandles[backBufferIndex],
				clearColor,
				0, nullptr);

			// === リソースバリア: RenderTarget → Present ===
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			commandList->ResourceBarrier(1, &barrier);

			// コマンドリストを閉じる
			hr = commandList->Close();
			assert(SUCCEEDED(hr));

			// GPUにコマンドを送る
			ID3D12CommandList* commandLists[] = { commandList };
			commandQueue->ExecuteCommandLists(1, commandLists);
			//GPUとOSに画面の交換を行うように通知する
			swapChain->Present(1, 0);

			// --- GPUにフェンスを発行して、完了を通知してもらう ---
			fenceValue++;
			hr = commandQueue->Signal(fence, fenceValue);
			assert(SUCCEEDED(hr));

			// --- GPUが終わるのを待機する ---
			if (fence->GetCompletedValue() < fenceValue) {
				hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
				assert(SUCCEEDED(hr));
				WaitForSingleObject(fenceEvent, INFINITE);
			}


			
		
			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));

			hr = commandList->Reset(commandAllocator, nullptr);
			assert(SUCCEEDED(hr));

			//ゲームの描画処理



		}


	}
	return 0;
}

