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

	SpriteCommon* spriteCommon = nullptr;

	//スプライト共通部分の初期化
	spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon);

	TextureManager::GetInstance()->Initialize(dxCommon);

	//// 2) 使うテクスチャをロード（1回でOK）
	//TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
	//TextureManager::GetInstance()->LoadTexture("resources/sample.png");

	//const int spriteCount = 5; // 出したい数
	//std::vector<std::unique_ptr<Sprite>> sprites;
	//std::vector<Vector2> basePos;

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
	//	sprite->SetScale({ 0.1f, 0.1f, 0.1f });

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

	//// 3Dオブジェクト（1個目）
	//Object3d* object3dA = new Object3d();
	//object3dA->Initialize(object3dCommon, dxCommon);
	//object3dA->SetModel("fence/fence.obj");        // ← 同じ Model を共有
	//object3dA->SetTranslate({ 0.0f, 0.0f, 0.0f });   // 位置A

	//// 3Dオブジェクト（2個目）
	//Object3d* object3dB = new Object3d();
	//object3dB->Initialize(object3dCommon, dxCommon);
	//object3dB->SetModel("Human2.fbx");        // ← ここも同じ Model
	//object3dB->SetScale({ 0.1f, 0.1f, 0.1f });
	//object3dB->SetTranslate({ 0.0f, 0.0f, 0.0f });

	//// ==== （オブジェクト生成の後あたりで）初期値を UI 側に取り込む ====
	//Vector4 uiLightColor = object3dA->GetLightColor();          // RGBA
	//Vector3 uiLightDir = object3dA->GetDirection();      // 方向
	//float   uiLightIntensity = object3dA->GetIntensity();    // 強度

	//auto Normalize3 = [](Vector3 v) {
	//	float l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	//	if (l < 1e-6f) return Vector3{ 0.0f, -1.0f, 0.0f };
	//	return Vector3{ v.x / l, v.y / l, v.z / l };
	//	};

	//object3dA->SetBlendMode(Object3dCommon::BlendMode::kBlendModeAdd);
	//object3dB->SetBlendMode(Object3dCommon::BlendMode::kBlendModeNormal);

	// ========== Particle 用 ==========
	ParticleCommon* particleCommon = new ParticleCommon();
	particleCommon->Initialize(dxCommon);

	Particle* particle = new Particle();
	particle->Initialize(particleCommon, dxCommon);

	// 好きなモデルをセット（とりあえず plane.obj とか）
	particle->SetModel("plane.obj");
	// 位置やスケールも設定しておく
	particle->SetTranslate({ -2.0f, 0.0f, 0.0f });
	particle->SetScale({ 1.0f, 1.0f, 1.0f });
	// ブレンド（加算とか）を変えたいとき
	particle->SetBlendMode(ParticleCommon::BlendMode::kBlendModeAdd);

	SkinnedModel* skinnedHuman = nullptr;

	skinnedHuman = new SkinnedModel();
	skinnedHuman->Initialize(dxCommon, "resources/Human2.fbx");

	// 表示位置をちょっと調整したければ
	skinnedHuman->SetScale({ 0.8f, 0.8f, 0.8f });
	skinnedHuman->SetTranslate({ 0.0f, 0.0f, 0.0f });
	skinnedHuman->SetRotate({ 0.0f, 0.0f, 0.0f }); // ← Y軸回転で180度回す

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

	bool useSample = true;

	SoundData soundData1 = SoundLoadWave("resources/fanfare.wav");

	bool isDrawSphere = true;
	bool isDrawModel = true;
	bool isDrawBunny = false;
	bool isDrawSprite = true;
	bool isDrawTea = false;

	/*auto rotA = object3dA->GetRotate();
	auto rotB = object3dB->GetRotate();*/

	bool isEnd = false;

	// 例えば main.cpp のどこか（SkinnedModel カメラのウィンドウの近く）に追加
	static int     uiBoneIndex = 0;
	static Vector3 uiBoneRot{ 0.0f, 0.0f, 0.0f };
	static Vector3 uiBoneTrans{ 0.0f, 0.0f, 0.0f };

	//ウィンドウボタンのxボタンが押されえるまでループ
	while (msg.message != WM_QUIT && !isEnd) {

		if (winApp->ProcessMessage()) {
			//ゲームループを抜ける
			break;
		}

		input.Update();

		debugCamera.Update();

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		
		ImGui::Begin("SkinnedModel Bones");

		// ボーンが存在するときだけ表示
		const auto& bones = skinnedHuman->GetBones();
		if (!bones.empty()) {

			// Bone 選択コンボ
			const char* currentName = bones[uiBoneIndex].name.c_str();
			if (ImGui::BeginCombo("Bone", currentName)) {
				for (int i = 0; i < static_cast<int>(bones.size()); ++i) {
					bool isSelected = (i == uiBoneIndex);
					if (ImGui::Selectable(bones[i].name.c_str(), isSelected)) {
						uiBoneIndex = i;
						// ボーン変更時は回転＆移動をリセット（お好みで）
						uiBoneRot = { 0.0f, 0.0f, 0.0f };
						uiBoneTrans = { 0.0f, 0.0f, 0.0f };
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			// ★ 追加：ローカル位置（ボーン原点からのオフセット）
			ImGui::DragFloat3("Bone Pos (local)", &uiBoneTrans.x, 0.01f);

			// 回転スライダー（度指定だけど値はラジアンで入る）
			ImGui::SliderAngle("Bone Rot X", &uiBoneRot.x);
			ImGui::SliderAngle("Bone Rot Y", &uiBoneRot.y);
			ImGui::SliderAngle("Bone Rot Z", &uiBoneRot.z);

			// SkinnedModel に反映
			skinnedHuman->SetDebugBoneTranslate(uiBoneIndex, uiBoneTrans);
			skinnedHuman->SetDebugBoneRotate(uiBoneIndex, uiBoneRot);
		}
		ImGui::End();



	
		//ゲームの更新処理

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
		dxCommon->SetDescriptorHeaps();       // ← SRVヒープをセット

		//Spriteの描画準備。Spriteの描画に共通のグラフィックコマンドを詰む
		spriteCommon->SetGraphicsPipelineState();

		// 共有設定（PSO/RootSig/トポロジ）

		dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);



		//// ==== 毎フレーム（ImGui の描画ブロック内）====
		//ImGui::Begin("Lighting");

		//// 色（RGB or RGBA どちらでもOK。アルファは 1.0f 固定でも良い）
		//ImGui::ColorEdit3("Directional Color", &uiLightColor.x);

		//// 方向（-1～+1 の範囲で編集 → 後で正規化）
		//ImGui::DragFloat3("Directional Dir", &uiLightDir.x, 0.01f, -1.0f, 1.0f);

		//// 強度（0～4 くらいが扱いやすい）
		//ImGui::SliderFloat("Intensity", &uiLightIntensity, 0.0f, 1.0f);

		//ImGui::End();

		//ImGui::Begin("Material");

		//static Vector4 uiMatA = object3dA->GetMaterialColor();
		//ImGui::ColorEdit4("Material A", &uiMatA.x);
		//object3dA->SetMaterialColor(uiMatA);

		//static Vector4 uiMatB = object3dB->GetMaterialColor();
		//ImGui::ColorEdit4("Material B", &uiMatB.x);
		//object3dB->SetMaterialColor(uiMatB);

		//ImGui::End();

		//ImGui::Begin("Material");

		//static int uiBlendModeA = (int)Object3dCommon::BlendMode::kBlendModeNormal;
		//static int uiBlendModeB = (int)Object3dCommon::BlendMode::kBlendModeNormal;

		//const char* blendNames[] = {
		//	"None",
		//	"Normal",
		//	"Add",
		//	"Subtract",
		//	"Multiply",
		//	"Screen"
		//};

		//// A のブレンド
		//ImGui::Combo("Blend A", &uiBlendModeA, blendNames, IM_ARRAYSIZE(blendNames));
		//object3dA->SetBlendMode((Object3dCommon::BlendMode)uiBlendModeA);

		//// B のブレンド
		//ImGui::Combo("Blend B", &uiBlendModeB, blendNames, IM_ARRAYSIZE(blendNames));
		//object3dB->SetBlendMode((Object3dCommon::BlendMode)uiBlendModeB);

		//ImGui::End();

		particle->DebugImGui();

		// 反映ボタンが欲しければ：if (ImGui::Button("Apply")) { ... }
		// 即時反映で良ければ毎フレームそのまま Set～ する

		//// ==== UI 値を Object3d の CB に書き戻す（即時反映）====
		//Vector3 dirN = Normalize3(uiLightDir);

		//// A に適用
		//object3dA->SetLightColor({ uiLightColor.x, uiLightColor.y, uiLightColor.z, 1.0f });
		//object3dA->SetDirection(dirN);
		//object3dA->SetIntensity(uiLightIntensity);

		//// B にも同じ設定（片方だけで良ければ外してOK）
		//object3dB->SetLightColor({ uiLightColor.x, uiLightColor.y, uiLightColor.z, 1.0f });
		//object3dB->SetDirection(dirN);
		//object3dB->SetIntensity(uiLightIntensity);


		//// ---- Sprite (Indexed) ----
		//	// === ImGuiで全スプライト共通操作 ===
		//	ImGui::Begin("Sprite Controller");

		//	// 共通で動かすパラメータ
		//	static Transform spriteCtrl = {
		//		{1.0f, 1.0f, 1.0f}, // scale
		//		{0.0f, 0.0f, 0.0f}, // rotate
		//		{0.0f, 0.0f, 0.0f}  // translate
		//	};
		//	static Vector4 color = { 1,1,1,1 };
		//	ImGui::DragFloat2("Translate", &spriteCtrl.translate.x, 1.0f);
		//	ImGui::DragFloat2("Scale", &spriteCtrl.scale.x, 0.01f, 0.01f, 10.0f);
		//	ImGui::SliderAngle("Rotation Z", &spriteCtrl.rotate.z);
		//	ImGui::ColorEdit4("Color", &color.x);
		//	ImGui::End();

		//	// === 全スプライトに適用 & 描画 ===
		//	const Matrix4x4 view2D = Matrix4x4::MakeIdentity4x4();
		//	const Matrix4x4 proj2D = Matrix4x4::MakeOrthographicMatrix(
		//		0.0f, 0.0f,
		//		float(WinApp::kClientWidth),
		//		float(WinApp::kClientHeight),
		//		0.0f, 100.0f
		//	);
		//	for (size_t i = 0; i < sprites.size(); ++i) {
		//		auto& sp = sprites[i];
		//		sp->SetScale(spriteCtrl.scale);
		//		sp->SetRotation(spriteCtrl.rotate);
		//		sp->SetColor(color);

		//		// ★ ここを「上書き」→「加算」に変更
		//		Vector2 pos = { basePos[i].x + spriteCtrl.translate.x,
		//						basePos[i].y + spriteCtrl.translate.y };
		//		sp->SetPosition(pos);

		//		sp->Update(view2D, proj2D);
		//		sp->Draw();
		//	}




	//	rotA.z += 0.02f;
	//	object3dA->SetRotate(rotA);

	//	
	////	rotB.y += 0.02f;
	//	object3dB->SetRotate(rotB);

		//Objectの描画準備。Objectの描画に共通のグラフィックコマンドを詰む
	//	object3dCommon->SetGraphicsPipelineState();

	//	//object3dA->Update();
	//	object3dB->Update();

	//	//object3dA->Draw();
	//	object3dB->Draw();

		// ==== 3D（スキン付き）====
	//	skinnedCommon->SetGraphicsPipelineState(); // ← Skinned 用 PSO
		// Transform / Material / Light / Bone 行列を Root にセット

		skinnedHuman->UpdateAnimation(1.0f / 60.0f);

		skinnedHuman->Draw();

		// ==== パーティクル描画 ====
// ここで Particle 用の PSO/RootSignature に切り替え
	//	particleCommon->SetGraphicsPipelineState();

		//particle->SpawnParticle();

		//// 必要なら位置アニメとかここで transform をいじる
		//particle->Update();
		//particle->Draw();

		// ---- ImGui ----
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());

		// ===== フレーム終了 =====
		dxCommon->PostDraw();


		if (input.IsKeyTrigger(DIK_ESCAPE)) {
			//ESCキーが押されたら終了
			isEnd = true;
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
	//debugController->Release();
#endif



// WindowsAPIの終了処理
	winApp->Finalize();

	TextureManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();

	// WindowsApp解放
	delete winApp;
	winApp = nullptr;

	dxCommon->ReportLiveObjects();

	delete dxCommon;
	dxCommon = nullptr;

	// ★ スプライト共通 解放
	delete spriteCommon;
	spriteCommon = nullptr;

	// ★ パーティクル 解放
	delete particle;
	particle = nullptr;

	delete particleCommon;
	particleCommon = nullptr;

	// 3D 共通・オブジェクト解放
	delete object3dCommon;
	object3dCommon = nullptr;

	//delete object3dA;
	//object3dA = nullptr;

	//delete object3dB;
	//object3dB = nullptr;

	delete skinnedHuman;
	skinnedHuman = nullptr;

	return 0;
}

