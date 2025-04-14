#include <Windows.h>
#include <cstdint>
#include <filesystem>
#include<fstream>//ファイルに書いたり読んだりするライブラリ
#include <chrono>//時間を扱うライブラリ


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

void Log(std::ostream& os, const std::string& message) {
	//ログの出力
	os << message << std::endl;
	OutputDebugStringA(message.c_str());
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
	std::filesystem::create_directories("logs");

	//現在時刻を取得(UTC時刻)
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	//ログファイルの名前にコンマ何秒はいらないので、削って秒にする
	std::chrono::time_point<std::chrono::system_clock,std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);

	//日本時間(PCの設定時間)に変換
	std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
	//formatお使って年月日_時分秒の文字列に変換
	std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
	//時刻を使ってファイル名を決定
	std::string logFilePath = std::string("logs/") + dateString + ".log";
	//ファイルを作って書き込み準備
	std::ofstream logStream(logFilePath);

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