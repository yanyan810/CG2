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

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

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


// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

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

	MSG msg{};

	//ログのディレクトリを用意
	std::filesystem::create_directory("logs");

	//現在時刻を取得(UTI時刻)
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	//ログファイルの名前にコンマ何秒はいらないので削って秒にする
	std::chrono::time_point <std::chrono::system_clock,std::chrono::seconds>
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
	const char* featureLevelStrings[] = {"12.2","12.1","12.0"};
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

	//ウィンドウボタンのxボタンが押されえるまでループ
	while (msg.message != WM_QUIT) {
		//windowにメッセージが来てたら最優先で処理させる
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			//ゲームの更新処理
			//ゲームの描画処理
		}


	}
    return 0;
}