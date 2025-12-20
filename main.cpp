#include "WinApp.h"
#include "DirectXCommon.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "Model.h"
#include "ModelManager.h"
#include "YMath.h"
#include "ParticleCommon.h"
#include "Particle.h"
#include "SkinnedModel.h"
#include "SrvManager.h"
#include "ParticleManager.h"
#include "ImGuiManagaer.h"	

#include <locale>
#include <codecvt>
#include <strsafe.h>
#include <DbgHelp.h>   
#include <dxgidebug.h>

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

#pragma message("### HERE")

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


#ifdef _DEBUG
		//リソースチェック
		Microsoft::WRL::ComPtr <IDXGIDebug1> debug = nullptr;
		//hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);

		}
#endif
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
	dxCommon->Initialize(winApp);

	//srvの初期化
	SrvManager* srvManager = nullptr;
	srvManager = new SrvManager();
	srvManager->Initialize(dxCommon);


	SpriteCommon* spriteCommon = nullptr;

	//スプライト共通部分の初期化
	spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon);

	TextureManager::GetInstance()->Initialize(dxCommon, srvManager);

	// 2) 使うテクスチャをロード（1回でOK）
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("resources/sample.png");

	//const int spriteCount = 5; // 出したい数
	/*std::vector<std::unique_ptr<Sprite>> sprites;
	std::vector<Vector2> basePos;*/

	Camera* camera = new Camera();
	camera->SetRotate({ 0.0f,0.0f,0.0f });
	camera->SetTranslate({ 0.0f,3.0f,-10.0f });

	Sprite* sprite = new Sprite;

	sprite->Initialize(spriteCommon, dxCommon, "resources/uvChecker.png");

	sprite->AdjustTextureSize();

	Vector2 spriteTrans = { 0.0f,0.0f };

	Vector3 spriteScale = { 1.0f,1.0f ,1.0f };

	sprite->SetPosition(spriteTrans);
	sprite->SetScale(spriteScale);

	//// 4) UV設定（画像全体を使うならこれでOK）
	//sprite->SetTextureTopLeft({ 0.0f, 0.0f });
	//sprite->SetTextureCutSize({ 512.0f, 512.0f }); // 画像サイズに合わせる

	// 5) アンカー（左上に固定）
	sprite->SetAnchorPoint({ 0.0f, 0.0f });

	// 6) 反転なし
	sprite->SetFlipX(false);
	sprite->SetFlipY(false);

	//for (int i = 0; i < spriteCount; ++i) {

	//	// 1) テクスチャを交互に選択
	//	std::string texturePath = (i % 2 == 0)
	//		? "resources/uvChecker.png"
	//		: "resources/sample.png";

	//	auto sprite = std::make_unique<Sprite>();
	//	sprite->Initialize(spriteCommon, dxCommon, texturePath);

	//	// 2) 配置（X方向にずらして並べる）
	//	Vector2 p = { 10.0f,10.0f };
	//	sprite->SetPosition(p);
	//	sprite->SetScale({ 1.0f, 0.1f, 0.1f });

	//	// 3) 色を少しずつ変える（任意）
	//	sprite->SetColor({ 1.0f, 0.2f * i, 1.0f - 0.2f * i, 1.0f });

	//	// 4) UV設定（画像全体を使うならこれでOK）
	//	sprite->SetTextureTopLeft({ 0.0f, 0.0f });
	//	sprite->SetTextureCutSize({ 512.0f, 512.0f }); // 画像サイズに合わせる

	//	// 5) アンカー（左上に固定）
	//	sprite->SetAnchorPoint({ 0.0f, 0.0f });

	//	// 6) 反転なし
	//	sprite->SetFlipX(false);
	//	sprite->SetFlipY(false);

	//	// リストへ追加
	//	basePos.push_back(p);
	//	sprites.push_back(std::move(sprite));
	//}

	//3Dモデルマネージャの初期化
	ModelManager::GetInstance()->Initialize(dxCommon);

	//3Dオブジェクト共通部分の初期化
	Object3dCommon* object3dCommon = nullptr;
	//3dオブジェクトの初期化	
	object3dCommon = new Object3dCommon();
	object3dCommon->Initialize(dxCommon);

	// 3Dオブジェクト（1個目）
	Object3d* object3dA = new Object3d();
	object3dA->Initialize(object3dCommon, dxCommon);
	object3dA->SetModel("fence/fence.obj");        // ← 同じ Model を共有
	object3dA->SetTranslate({ 0.0f, 0.0f, 20.0f });   // 位置A

	// 3Dオブジェクト（2個目）
	Object3d* object3dB = new Object3d();
	object3dB->Initialize(object3dCommon, dxCommon);
	object3dB->SetModel("plane.obj");        // ← ここも同じ Model
	object3dB->SetScale({ 1.0f, 1.0f, 1.0f });
	object3dB->SetTranslate({ 0.0f, 0.0f, 20.0f });
	object3dB->SetRotate({ 0.0f, 0.5f, 0.0f });

	// ==== （オブジェクト生成の後あたりで）初期値を UI 側に取り込む ====
	Vector4 uiLightColor = object3dA->GetLightColor();          // RGBA
	Vector3 uiLightDir = object3dA->GetDirection();      // 方向
	float   uiLightIntensity = object3dA->GetIntensity();    // 強度

	auto Normalize3 = [](Vector3 v) {
		float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		if (l < 1e-6f) return Vector3{ 0.0f, -1.0f, 0.0f };
		return Vector3{ v.x / l, v.y / l, v.z / l };
		};

	object3dA->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);
	object3dB->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNone);


	// ========== Particle 用 ==========
	ParticleCommon* particleCommon = new ParticleCommon();
	particleCommon->Initialize(dxCommon);

	Particle* particle = new Particle();
	particle->Initialize(particleCommon, dxCommon, srvManager);

	// 好きなモデルをセット（とりあえず plane.obj とか）
	particle->SetModel("plane.obj");
	// 位置やスケールも設定しておく
	particle->SetTranslate({ -2.0f, 0.0f, 0.0f });
	particle->SetScale({ 1.0f, 1.0f, 1.0f });
	// ブレンド（加算とか）を変えたいとき
	particle->SetBlendMode(ParticleCommon::BlendMode::kBlendModeAdd);
	particle->SetCamera(camera);

	//パテ２
	Particle* particle2 = new Particle();
	particle2->Initialize(particleCommon, dxCommon, srvManager);

	particle2->SetModel("plane.obj");
	// 位置やスケールも設定しておく
	particle2->SetTranslate({ -2.0f, 30.0f, 20.0f });
	particle2->SetScale({ 1.0f, 1.0f, 1.0f });
	// ブレンド（加算とか）を変えたいとき
	particle2->SetBlendMode(ParticleCommon::BlendMode::kBlendModeMultily);
	particle2->SetCamera(camera);

	SkinnedModel* skinnedHuman = nullptr;

	skinnedHuman = new SkinnedModel();
	//skinnedHuman->Initialize(dxCommon, "resources/55-rp_nathan_animated_003_walking_fbx/rp_nathan_animated_003_walking.fbx");
	skinnedHuman->Initialize(dxCommon, "resources/Human.fbx");
	// 表示位置をちょっと調整したければ
	skinnedHuman->SetScale({ 0.02f * 1.0f, 0.02f * 1.0f, 0.02f * 1.0f });
	skinnedHuman->SetTranslate({ 0.0f, -20.0f, 100.0f });
	skinnedHuman->SetRotate({ 0.0f, 0.0f, 0.0f }); // ← Y軸回転で180度回す

	Vector3 scale = { 1.0f,1.0f,1.0f };
	Vector3 tramnslate = { 0.0f,0.0f,0.0f };
	Vector3 rotate = { 0.0f,0.0f,0.0f };


	object3dCommon->SetDefaultCamera(camera);
	object3dCommon->SetDefaultCamera(camera);
	object3dA->SetCamera(camera);
	object3dB->SetCamera(camera);

	auto* pm = ParticleManager::GetInstance();
	pm->Initialize(dxCommon, srvManager, particleCommon);

	// グループ作成（テクスチャは好きなやつ）
	pm->CreateParticleGroup("smoke", "resources/uvChecker.png");
	pm->SetGroupBlendMode("smoke", ParticleCommon::BlendMode::kBlendModeAdd); // 例：加算
	auto* pm2 = ParticleManager::GetInstance();

	pm2->Initialize(dxCommon, srvManager, particleCommon);

	// グループ作成（テクスチャは好きなやつ）
	pm2->CreateParticleGroup("flash", "resources/circle.png");
	pm2->SetGroupBlendMode("flash", ParticleCommon::BlendMode::kBlendModeSubtract); // 例：加算
	MSG msg{};

	ImGuiManagaer* imguiManager = new ImGuiManagaer();
	imguiManager->Initialize(winApp, dxCommon, srvManager);

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

	bool useSample = true;

	SoundData soundData1 = SoundLoadWave("resources/fanfare.wav");

	bool isDrawSphere = true;
	bool isDrawModel = true;
	bool isDrawBunny = false;
	bool isDrawSprite = true;
	bool isDrawTea = false;

	auto rotA = object3dA->GetRotate();
	auto rotB = object3dB->GetRotate();

	bool isEnd = false;

	// 例えば main.cpp のどこか（SkinnedModel カメラのウィンドウの近く）に追加
	static int     uiBoneIndex = 0;
	static Vector3 uiBoneRot{ 0.0f, 0.0f, 0.0f };
	static Vector3 uiBoneTrans{ 0.0f, 0.0f, 0.0f };
	static Vector3 uiBoneScale{ 1.0f, 1.0f, 1.0f };

	struct SpriteTuner {
		bool visible = true;

		// transform
		float pos[2] = { 200.0f, 200.0f };
		float scale[2] = { 1.0f, 1.0f };
		float rotDeg = 0.0f;

		// pivot/anchor (0..1)
		float pivot[2] = { 0.5f, 0.5f };

		// color (0..1)
		float color[4] = { 1,1,1,1 };

		// uv (0..1) 画像の切り抜き
		// uv0 = 左上, uv1 = 右下
		float uv0[2] = { 0.0f, 0.0f };
		float uv1[2] = { 1.0f, 1.0f };

		// option
		int blendMode = 0; // 0:Alpha, 1:Add, 2:Mul, 3:Opaque ... みたいにあなたのPSOに合わせる
	};


	//ウィンドウボタンのxボタンが押されえるまでループ
	while (msg.message != WM_QUIT && !isEnd) {

		if (winApp->ProcessMessage()) {
			//ゲームループを抜ける
			break;
		}

		input.Update();

		debugCamera.Update();

		//ImGui設定







		//ImGuiここまで

		//ImGui::Begin("SkinnedModel Bones");

		//// ボーンが存在するときだけ表示
		//const auto& bones = skinnedHuman->GetBones();
		//if (!bones.empty()) {

		//	// Bone 選択コンボ
		//	const char* currentName = bones[uiBoneIndex].name.c_str();
		//	if (ImGui::BeginCombo("Bone", currentName)) {
		//		for (int i = 0; i < static_cast<int>(bones.size()); ++i) {
		//			bool isSelected = (i == uiBoneIndex);
		//			if (ImGui::Selectable(bones[i].name.c_str(), isSelected)) {
		//				uiBoneIndex = i;
		//				// ボーン変更時は回転＆移動をリセット（お好みで）
		//				uiBoneRot = { 0.0f, 0.0f, 0.0f };
		//				uiBoneTrans = { 0.0f, 0.0f, 0.0f };
		//				uiBoneScale = { 1.0f, 1.0f, 1.0f };
		//			}
		//			if (isSelected) {
		//				ImGui::SetItemDefaultFocus();
		//			}
		//		}
		//		ImGui::EndCombo();
		//	}

		//	// ★ 追加：ローカル位置（ボーン原点からのオフセット）
		//	ImGui::DragFloat3("Bone Pos (local)", &uiBoneTrans.x, 0.01f);

		//	// 回転スライダー（度指定だけど値はラジアンで入る）
		//	ImGui::SliderAngle("Bone Rot X", &uiBoneRot.x);
		//	ImGui::SliderAngle("Bone Rot Y", &uiBoneRot.y);
		//	ImGui::SliderAngle("Bone Rot Z", &uiBoneRot.z);

		//	// スケールスライダー
		//	ImGui::DragFloat3("Bone Scale", &uiBoneScale.x, 0.01f);

		//	// SkinnedModel に反映
		//	skinnedHuman->SetDebugBoneTranslate(uiBoneIndex, uiBoneTrans);
		//	skinnedHuman->SetDebugBoneRotate(uiBoneIndex, uiBoneRot);
		//	//skinnedHuman->SetDebugBoneScale(uiBoneIndex, uiBoneScale);
		//}
		//ImGui::End();





		//ゲームの更新処理


		camera->Update();


		//ImGuiの表示
		static int selectedTexture1 = 0;
		static int selectedTexture2 = 0;
		const char* textureNames[] = { "UV", "Sample", "Axis" };

		////スプライトのUVを変換する行列を計算
		//Matrix4x4 uvMatrix =
		//	Matrix4x4::Scale(uvTransformSprite.scale) *
		//	Matrix4x4::RotateZ(uvTransformSprite.rotate.z) *
		//	Matrix4x4::Translation(uvTransformSprite.translate);
		//for (auto& sp : sprites) {
		//	sp->SetUVTransform(uvMatrix);    // ← 既存インスタンスへ反映
		//}

		// ===== GPUコマンド発行開始 =====
		dxCommon->PreDraw();                  // ← クリア & バリア遷移
		//dxCommon->SetDescriptorHeaps();       // ← SRVヒープをセット

		srvManager->PreDraw(/*dxCommon->GetCommandList()*/); // ★ SrvManager の heap をバインド

#ifdef USE_IMGUI
		imguiManager->Begin();

		// --- ここは「UIを組み立てるだけ」 ---
		ImGui::Begin("Performance");
		ImGui::Text("Engine FPS : %.2f", dxCommon->GetFPS());
		ImGui::End();

		ImGui::Begin("Sprite Control");
		ImGui::DragFloat2("Position", &spriteTrans.x, 1.0f);
		ImGui::DragFloat3("Scale", &spriteScale.x, 0.01f, 0.01f, 10.0f);
		if (ImGui::Button("Reset")) {
			spriteTrans = { 0.0f, 0.0f };
			spriteScale = { 1.0f, 1.0f, 1.0f };
		}
		ImGui::End();

		// ★ 反映はここでOK
		sprite->SetPosition(spriteTrans);
		sprite->SetScale(spriteScale);

		//particle->DebugImGui();

		ImGui::Begin("Camera TRS");

		// ① setter に入っている「現在値」を getter で取る
		Vector3 camT = camera->GetTranslate();
		Vector3 camR = camera->GetRotate();   // ラジアン
		//Vector3 camS = camera->GetScale();

		bool changed = false;

		// ② ImGui で編集
		changed |= ImGui::DragFloat3("Translate", &camT.x, 0.1f);
		changed |= ImGui::DragFloat3("Rotate (rad)", &camR.x, 0.01f);
		//changed |= ImGui::DragFloat3("Scale", &camS.x, 0.01f, 0.01f, 100.0f);

		// リセット
		if (ImGui::Button("Reset")) {
			camT = { 0.0f, 0.0f, 0.0f };
			camR = { 0.0f, 0.0f, 0.0f };
			//camS = { 1.0f, 1.0f, 1.0f };
			changed = true;
		}

		ImGui::End();

		// ③ 変更があったら setter に戻す
		if (changed) {
			camera->SetTranslate(camT);
			camera->SetRotate(camR);
			//camera->SetScale(camS);
		}


#endif // USE_IMGUI

		// 共有設定（PSO/RootSig/トポロジ）

		dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


		//	Objectの描画準備。Objectの描画に共通のグラフィックコマンドを詰む
		object3dCommon->SetGraphicsPipelineState();

		rotA.y += 0.2f;
		rotB.y += 0.2f;

		object3dA->SetRotate(rotA);
		object3dB->SetRotate(rotB);

		object3dA->Update();
		object3dB->Update();

		object3dA->Draw();
		object3dB->Draw();

		// ==== 3D（スキン付き）====
	//	skinnedCommon->SetGraphicsPipelineState(); // ← Skinned 用 PSO
		// Transform / Material / Light / Bone 行列を Root にセット

		//skinnedHuman->UpdateAnimation(1.0f / 60.0f);

		//skinnedHuman->Draw();

		// ==== パーティクル描画 ====
// ここで Particle 用の PSO/RootSignature に切り替え
		particleCommon->SetGraphicsPipelineState();

		particle->SpawnParticle();

		//// 必要なら位置アニメとかここで transform をいじる
		//particle->Update();
		//particle->Draw();

		//particle2->SpawnParticle();

		//// 必要なら位置アニメとかここで transform をいじる
		//particle2->Update();
		//particle2->Draw();



		// 例：毎フレーム 10 個出す
		pm->Emit("smoke", { 0.0f, 7.0f, 20.0f }, 2);
		pm->Update(1.0f / 60.0f, *camera);
		pm->Draw(dxCommon->GetCommandList());

		pm2->Emit("flash", { 0.0f, -3.0f, 20.0f }, 5);
		pm2->Update(1.0f / 60.0f, *camera);
		pm2->Draw(dxCommon->GetCommandList());

		//Spriteの描画準備。Spriteの描画に共通のグラフィックコマンドを詰む
		spriteCommon->SetGraphicsPipelineState();


		const Matrix4x4 view2D = Matrix4x4::MakeIdentity4x4();
		const Matrix4x4 proj2D = Matrix4x4::MakeOrthographicMatrix(
			0.0f, 0.0f,
			float(WinApp::kClientWidth),
			float(WinApp::kClientHeight),
			0.0f, 100.0f
		);

		sprite->Update(view2D, proj2D);
		sprite->Draw();




		// =====================
// ★ 最後に ImGui を描く
// =====================
#ifdef USE_IMGUI
		imguiManager->End(dxCommon->GetCommandList());
#endif


		// ===== フレーム終了 =====
		dxCommon->PostDraw();


		if (input.IsKeyTrigger(DIK_ESCAPE)) {
			//ESCキーが押されたら終了
			isEnd = true;
		}




	}

	imguiManager->Shutdown();


	xAudio2.Reset();
	SoundUnload(&soundData1);

#ifdef _DEBUG
	//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
	//debugController->Release();
#endif



// WindowsAPIの終了処理
	winApp->Finalize();

	TextureManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();

	delete imguiManager;
	imguiManager = nullptr;

	// WindowsApp解放
	delete winApp;
	winApp = nullptr;

	dxCommon->ReportLiveObjects();

	delete dxCommon;
	dxCommon = nullptr;

	delete srvManager;
	srvManager = nullptr;

	// ★ スプライト共通 解放
	delete spriteCommon;
	spriteCommon = nullptr;

	delete sprite;
	sprite = nullptr;

	// ★ パーティクル 解放
	delete particle;
	particle = nullptr;

	delete particle2;
	particle2 = nullptr;


	delete particleCommon;
	particleCommon = nullptr;

	// 3D 共通・オブジェクト解放
	delete object3dCommon;
	object3dCommon = nullptr;

	delete object3dA;
	object3dA = nullptr;

	delete object3dB;
	object3dB = nullptr;

	delete skinnedHuman;
	skinnedHuman = nullptr;

	return 0;
}

