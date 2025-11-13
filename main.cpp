#include "WinApp.h"
#include "DirectXCommon.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "Object3d.h"

#include <locale>
#include <codecvt>
#include <strsafe.h>
#include <DbgHelp.h>   
#include <dxgidebug.h>

#include "math/Matrix4x4.h"
#include "math/Vector3.h"



#define _USE_MATH_DEFINES
#include <math.h>
#include <xaudio2.h>
#define DIRECTINPUT_VERSION 0x0800//DIrectInputバージョンの指定
#include <dinput.h>
#include "Input.h"
#include "DebugCamera.h"

#pragma comment(lib, "Dbghelp.lib") 
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "dinput8.lib")

void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
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

class ResourceObject {
public:
	ResourceObject(ID3D12Resource* resource)
		:resource_(resource)
	{
	}

	//オブジェクトの寿命が尽きたら呼ばれる
	~ResourceObject() {
		if (resource_) {
			resource_->Release();
		}
	}
	ID3D12Resource* Get() { return resource_; }
private:
	ID3D12Resource* resource_;

};

struct D3DResourceLeakChecker {

	~D3DResourceLeakChecker() {

		//リソースチェック
		Microsoft::WRL::ComPtr <IDXGIDebug1> debug = nullptr;
		//hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);

		}

	}

};

//チャンクヘッダー
struct ChunkHeader {
	char id[4]; // チャンクID
	uint32_t size; // チャンクのサイズ（ヘッダーを除く）
};

struct RiffHeader {
	ChunkHeader chunk;//"RIFF"
	char type[4]; // "WAVE"

};

struct  FormatChunk
{
	ChunkHeader chunk;//"fmt"
	WAVEFORMATEX fmt;//波形フォーマット
};

struct SoundData {
	//波形フォーマット
	WAVEFORMATEX wfex;
	//バッファの先頭アドレス
	BYTE* pBuffer;
	//バッファのサイズ
	unsigned int bufferSize;


};

SoundData SoundLoadWave(const char* filename) {
	//HRESULT result;
	//ファイル入力ストリームのインスタンス
	std::ifstream file;
	//.waveファイルをバイナリモードで開く
	file.open(filename, std::ios_base::binary);
	//ファイルオープン失敗を検出する
	assert(file.is_open());

	//RIFFヘッダーの読み込み
	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));
	//ファイル化RIFFかチェック
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}

	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	//Formatチャンクの読み込み
	FormatChunk format = {};

	file.read((char*)&format, sizeof(ChunkHeader));
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		assert(0);
	}

	//チャンク本体の読み込み
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);

	//Dataチャンクの読み込み
	ChunkHeader data;
	file.read((char*)&data, sizeof(data));
	//JUNKチャンクを検出した場合
	if (strncmp(data.id, "JUNK", 4) == 0) {
		//JUNKチャンクのサイズを読み飛ばす
		file.seekg(data.size, std::ios_base::cur);
		//Dataチャンクの再読み込み
		file.read((char*)&data, sizeof(data));
	}

	if (strncmp(data.id, "data", 4) != 0) {
		//Dataチャンクがない場合はエラー
		assert(0);
	}
	//Dataチャンクのデータ部（波形データ）の読み込み
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);
	//waveファイルを閉じる
	file.close();

	//returnする場合の音声データ
	SoundData soundData = {};
	soundData.wfex = format.fmt; // 波形フォーマットを設定
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer); // バッファの先頭アドレスを設定
	soundData.bufferSize = data.size; // バッファのサイズを設定

	return soundData;

}

void SoundUnload(SoundData* soundData) {
	delete[] soundData->pBuffer; // バッファの解放
	soundData->pBuffer = 0; // ポインタをnullptrに設定
	soundData->bufferSize = 0; // サイズを0に設定
	soundData->wfex = {}; // 波形フォーマットを初期化
}

//音声再生
void SoundPlayerWave(IXAudio2* xAudio2, const SoundData& soundData) {
	HRESULT result;
	//波形フォーマットをもとにSourceVoiceの生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	//再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	//波形データの再生
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start();

}



// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	D3DResourceLeakChecker leakCheck;
	

	// CrashHandlerを登録
	SetUnhandledExceptionFilter(ExportDump);

	WinApp* winApp = nullptr;

	//WindowsAPIの初期化
	winApp = new WinApp();
	winApp->Initialize();

	DirectXCommon* dxCommon = nullptr;

	//DirectXの初期化
	dxCommon = new DirectXCommon();
	dxCommon  ->Initialize(winApp);

	SpriteCommon* spriteCommon = nullptr;

	//スプライト共通部分の初期化
	spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon);

	TextureManager::GetInstance()->Initialize(dxCommon);

	// 2) 使うテクスチャをロード（1回でOK）
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("resources/sample.png");

	const int spriteCount = 5; // 出したい数
	std::vector<std::unique_ptr<Sprite>> sprites;
	std::vector<Vector2> basePos;

	for (int i = 0; i < spriteCount; ++i) {

		// 1) テクスチャを交互に選択
		std::string texturePath = (i % 2 == 0)
			? "resources/uvChecker.png"
			: "resources/sample.png";

		auto sprite = std::make_unique<Sprite>();
		sprite->Initialize(spriteCommon, dxCommon, texturePath);

		// 2) 配置（X方向にずらして並べる）
		Vector2 p = { 10.0f,10.0f };
		sprite->SetPosition(p);
		sprite->SetScale({ 0.1f, 0.1f, 0.1f });

		// 3) 色を少しずつ変える（任意）
		sprite->SetColor({ 1.0f, 0.2f * i, 1.0f - 0.2f * i, 1.0f });

		// 4) UV設定（画像全体を使うならこれでOK）
		sprite->SetTextureTopLeft({ 0.0f, 0.0f });
		sprite->SetTextureCutSize({ 512.0f, 512.0f }); // 画像サイズに合わせる

		// 5) アンカー（左上に固定）
		sprite->SetAnchorPoint({ 0.0f, 0.0f });

		// 6) 反転なし
		sprite->SetFlipX(false);
		sprite->SetFlipY(false);

		// リストへ追加
		basePos.push_back(p);
		sprites.push_back(std::move(sprite));
	}

	//3Dオブジェクト共通部分の初期化
	Object3dCommon* object3dCommon = nullptr;
	//3dオブジェクトの初期化	
	object3dCommon = new Object3dCommon();	
	object3dCommon->Initialize(dxCommon);

	Object3d* object3d = new Object3d();
	object3d->Initialize(object3dCommon,dxCommon);

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

	HRESULT hr;

	Input input;
	input.Initialize(winApp);


	DebugCamera debugCamera;
	debugCamera.Initialize();
	debugCamera.SetInput(&input);


	//=========================
	//音を入れる
	//=========================
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice = nullptr;

	//Xaudioエンジンのインスタンスの生成
	hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);

	//マスターボイスを実装
	hr = xAudio2->CreateMasteringVoice(&masterVoice);

	// --- スプライト用パラメータ（ImGui用） ---
	Transform uvTransformSprite{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};

	////=================
	////   球
	////=================
	////頂点バッファビューを作成する
	//const uint32_t kSubdivision = 16; // 分割数

	//Microsoft::WRL::ComPtr < ID3D12Resource> vertexResourceSphere =  dxCommon->CreateBufferResource( sizeof(VertexData) * kSubdivision * kSubdivision * 6);

	//// 頂点バッファビューを作成する
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere{};
	//// リソースの先頭アドレスを使う
	//vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();
	//// 使用するリソースのサイズは頂点3つ分のサイズ
	//vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * kSubdivision * kSubdivision * 6;

	//// 1頂点あたりのサイズ
	//vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	//// 頂点リソースにデータを書き込む
	//VertexData* vertexDataSphere = nullptr;
	//// 書き込むためのアドレスを取得
	//vertexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));

	////======================
	////球の計算
	////======================

	//const float kLonEvery = 2.0f * float(M_PI) / float(kSubdivision);
	//const float kLatEvery = float(M_PI) / float(kSubdivision);

	//// 頂点データの書き込み
	//for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
	//	// 各バンドの南端緯度と北端緯度
	//	float lat = -0.5f * float(M_PI) + kLatEvery * float(latIndex);
	//	float latN = lat + kLatEvery;
	//	// sin/cos を一度だけ計算
	//	float cosLat = cosf(lat);
	//	float sinLat = sinf(lat);
	//	float cosLatN = cosf(latN);
	//	float sinLatN = sinf(latN);

	//	for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
	//		float lon = kLonEvery * float(lonIndex);
	//		float lonN = lon + kLonEvery;
	//		float cosLon = cosf(lon);
	//		float sinLon = sinf(lon);
	//		float cosLonN = cosf(lonN);
	//		float sinLonN = sinf(lonN);

	//		// テクスチャ座標
	//		float u = float(lonIndex) / float(kSubdivision);
	//		float uN = float(lonIndex + 1) / float(kSubdivision);
	//		float v = 1.0f - float(latIndex) / float(kSubdivision);
	//		float vN = 1.0f - float(latIndex + 1) / float(kSubdivision);

	//		// 6頂点分のベースオフセット
	//		uint32_t base = (latIndex * kSubdivision + lonIndex) * 6;

	//		// 頂点位置を構築
	//		// BL (Bottom-Left)
	//		vertexDataSphere[base].position = { cosLat * cosLon,  sinLat,  cosLat * sinLon, 1.0f };
	//		vertexDataSphere[base].texcoord = { u,  v };
	//		// TL (Top-Left)
	//		vertexDataSphere[base + 1].position = { cosLatN * cosLon,  sinLatN, cosLatN * sinLon, 1.0f };
	//		vertexDataSphere[base + 1].texcoord = { u,  vN };
	//		// BR (Bottom-Right)
	//		vertexDataSphere[base + 2].position = { cosLat * cosLonN, sinLat,  cosLat * sinLonN, 1.0f };
	//		vertexDataSphere[base + 2].texcoord = { uN, v };
	//		// TR (Top-Right)
	//		vertexDataSphere[base + 3].position = { cosLatN * cosLonN, sinLatN, cosLatN * sinLonN, 1.0f };
	//		vertexDataSphere[base + 3].texcoord = { uN, vN };

	//		// 法線：ポジションの XYZ を正規化（外向きの放射ベクトル）
	//		for (int i = 0; i < 4; ++i) {
	//			Vector3 pos = {
	//				vertexDataSphere[base + i].position.x,
	//				vertexDataSphere[base + i].position.y,
	//				vertexDataSphere[base + i].position.z,
	//			};
	//			vertexDataSphere[base + i].normal = Normalize(pos);
	//		}

	//		// 2枚目の三角形（法線もきちんと引き継ぐ）
	//		vertexDataSphere[base + 4] = vertexDataSphere[base + 2]; // BR
	//		vertexDataSphere[base + 5] = vertexDataSphere[base + 1]; // TL

	//	}

	//}

	////球用のTransformationMatrix用のリソースを作る。Matrix4x4 一つ分のサイズを用意する
	//Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSphere =  dxCommon->CreateBufferResource( sizeof(TransformationMatrix));
	////データを書き込む
	//TransformationMatrix* transformationMatrixDataSphere = nullptr;
	////書き込むためのアドレスを取得
	//transformationMatrixResourceSphere->Map(0, nullptr,
	//	reinterpret_cast<void**>(&transformationMatrixDataSphere));
	////単位行列を書き込んでおく
	//transformationMatrixDataSphere->WVP = Matrix4x4::MakeIdentity4x4();
	//transformationMatrixDataSphere->World = Matrix4x4::MakeIdentity4x4();


	//static Transform transformSphere{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f },{0.0f,0.0f,0.0f} };


	////レンダリングパイプ用のカメラ
	//Transform cameraTransform({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,0.0f,-10.0f });

	bool useSample = true;




	SoundData soundData1 = SoundLoadWave("resources/fanfare.wav");

	bool isDrawSphere = true;
	bool isDrawModel = true;
	bool isDrawBunny = false;
	bool isDrawSprite = true;
	bool isDrawTea = false;

	//ウィンドウボタンのxボタンが押されえるまでループ
	while (msg.message != WM_QUIT) {

		if (winApp->ProcessMessage()) {
			//ゲームループを抜ける
			break;
		}

		input.Update();

		debugCamera.Update();

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// ImGuiウィンドウ表示
		ImGui::ShowDemoWindow();

		//ゲームの更新処理

		//ImGuiの表示
		static int selectedTexture1 = 0;
		static int selectedTexture2 = 0;
		const char* textureNames[] = { "UV", "Sample", "Axis" };

		//スプライトのUVを変換する行列を計算
		Matrix4x4 uvMatrix =
			Matrix4x4::Scale(uvTransformSprite.scale) *
			Matrix4x4::RotateZ(uvTransformSprite.rotate.z) *
			Matrix4x4::Translation(uvTransformSprite.translate);
		for (auto& sp : sprites) {
			sp->SetUVTransform(uvMatrix);    // ← 既存インスタンスへ反映
		}

		//=================
		////球に関する処理
		////=================
		//// 回転
		////transformSphere.rotate.y += 0.03f;

		//// ワールド行列
		//Matrix4x4 worldMatrixSphere = Matrix4x4::MakeAffineMatrix(
		//	transformSphere.scale, transformSphere.rotate, transformSphere.translate);

		//// カメラ
		//Matrix4x4 cameraMatrixSphere = Matrix4x4::MakeAffineMatrix(
		//	cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
		//Matrix4x4 viewMatrixSphere = Matrix4x4::Inverse(cameraMatrixSphere);

		//// 射影行列
		//Matrix4x4 projectionMatrixSphere = Matrix4x4::PerspectiveFov(
		//	0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 360.0f);

		//// WVP = World * View * Projection
		//Matrix4x4 wvpSphere = Matrix4x4::Multiply(
		//	worldMatrixSphere,
		//	Matrix4x4::Multiply(viewMatrixSphere, projectionMatrixSphere));

		//// TransformationMatrix へ代入
		//transformationMatrixDataSphere->WVP = wvpSphere;
		//transformationMatrixDataSphere->World = worldMatrixSphere;

		//directionalLightData->direction = Normalize(directionalLightData->direction)

		ImGui::End();

		// ===== GPUコマンド発行開始 =====
		dxCommon->PreDraw();                  // ← クリア & バリア遷移
		dxCommon->SetDescriptorHeaps();       // ← SRVヒープをセット

		//Spriteの描画準備。Spriteの描画に共通のグラフィックコマンドを詰む
		spriteCommon->SetGraphicsPipelineState();

		// 共有設定（PSO/RootSig/トポロジ）
		//dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
		//dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
		dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//// ---- Sphere ----
		//if (isDrawSphere) {
		//	dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);
		//	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());                // Pixel用 Material
		//	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSphere->GetGPUVirtualAddress()); // Vertex用 WVP/World
		//	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());        // Pixel用 Light
		//	dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, useSample ? textureSrvHandleGPU2 : textureSrvHandleGPU);     // SRV
		//	dxCommon->GetCommandList()->DrawInstanced(kSubdivision * kSubdivision * 6, 1, 0, 0);
		//}


		// ---- Sprite (Indexed) ----
		if (isDrawSprite) {
			// === ImGuiで全スプライト共通操作 ===
			ImGui::Begin("Sprite Controller");

			// 共通で動かすパラメータ
			static Transform spriteCtrl = {
				{1.0f, 1.0f, 1.0f}, // scale
				{0.0f, 0.0f, 0.0f}, // rotate
				{0.0f, 0.0f, 0.0f}  // translate
			};
			static Vector4 color = { 1,1,1,1 };
			ImGui::DragFloat2("Translate", &spriteCtrl.translate.x, 1.0f);
			ImGui::DragFloat2("Scale", &spriteCtrl.scale.x, 0.01f, 0.01f, 10.0f);
			ImGui::SliderAngle("Rotation Z", &spriteCtrl.rotate.z);
			ImGui::ColorEdit4("Color", &color.x);
			ImGui::End();

			// === 全スプライトに適用 & 描画 ===
			const Matrix4x4 view2D = Matrix4x4::MakeIdentity4x4();
			const Matrix4x4 proj2D = Matrix4x4::MakeOrthographicMatrix(
				0.0f, 0.0f,
				float(WinApp::kClientWidth),
				float(WinApp::kClientHeight),
				0.0f, 100.0f
			);
			for (size_t i = 0; i < sprites.size(); ++i) {
				auto& sp = sprites[i];
				sp->SetScale(spriteCtrl.scale);
				sp->SetRotation(spriteCtrl.rotate);
				sp->SetColor(color);

				// ★ ここを「上書き」→「加算」に変更
				Vector2 pos = { basePos[i].x + spriteCtrl.translate.x,
								basePos[i].y + spriteCtrl.translate.y };
				sp->SetPosition(pos);

				sp->Update(view2D, proj2D);
				sp->Draw();
			}

		}


		//Objectの描画準備。Objectの描画に共通のグラフィックコマンドを詰む
		object3dCommon->SetGraphicsPipelineState();

		object3d->Update();

		object3d->Draw();


		// ---- ImGui ----
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());

		// ===== フレーム終了 =====
		dxCommon->PostDraw();


		if (input.IsKeyTrigger(DIK_ESCAPE)) {
			//ESCキーが押されたら終了
			return 0;
		}




	}


	//ImGuiの終了処理。
	//初期化とは逆順に行う
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	xAudio2.Reset();
	SoundUnload(&soundData1);

#ifdef _DEBUG
	//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
	debugController->Release();
#endif



	// WindowsAPIの終了処理
	winApp->Finalize();

	TextureManager::GetInstance()->Finalize();

	// WindowsApp解放
	delete winApp;
	winApp = nullptr;

	delete dxCommon;
	dxCommon = nullptr;

	delete spriteCommon;
	spriteCommon = nullptr;

	delete object3dCommon;
	object3dCommon = nullptr;

	delete object3d;
	object3d = nullptr;


	return 0;
}

