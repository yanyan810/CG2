#include "WinApp.h"
#include "DirectXCommon.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "TextureManager.h"

#include <format>

#include <vector>


#include <filesystem>
#include <fstream>

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

struct Matrix3x3 {
	float m[3][3]; // 3x3行列を表す
};

struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};


struct Material {
	Vector4 color; // 色
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct DirectionalLight {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

Vector3 Normalize(const Vector3& v) {
	float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	if (length == 0.0f) return { 0.0f, 0.0f, 0.0f };
	return { v.x / length, v.y / length, v.z / length };
}




struct MaterialData {
	std::string textureFilePath; // テクスチャファイルのパス
};

struct ModelData {
	std::vector<VertexData> vertices; // 頂点データ
	MaterialData material;
};


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








MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	MaterialData materialData;//構築するMaterialData
	std::string line;// ファイルから読んだ1行を格納するもの
	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
	assert(file.is_open()); // ファイルが開けなかったらエラー

	while (std::getline(file, line)) {
		std::string identifer;
		std::istringstream s(line);
		s >> identifer; // 先頭の識別子を読む
		if (identifer == "map_Kd") {
			std::string textureFilePath;
			s >> textureFilePath; // テクスチャファイルのパスを読み込む
			materialData.textureFilePath = directoryPath + "/" + textureFilePath; // ディレクトリパスと結合
		}
	}

	return materialData;
}


ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;
	std::string line;

	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::istringstream s(line);
		std::string identifier;
		s >> identifier;

		if (identifier == "v") {
			Vector4 pos{};
			s >> pos.x >> pos.y >> pos.z;
			pos.w = 1.0f;
			pos.x *= -1.0f; // X反転
			positions.push_back(pos);

		} else if (identifier == "vt") {
			Vector2 texcoord{};
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);

		} else if (identifier == "vn") {
			Vector3 normal{};
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f; // X反転
			normals.push_back(normal);

		} else if (identifier == "f") {
			std::vector<VertexData> polygonVertices;
			std::string vertexDefinition;

			while (s >> vertexDefinition) {
				std::istringstream v(vertexDefinition);
				uint32_t indices[3]{};
				for (int i = 0; i < 3; ++i) {
					std::string index;
					std::getline(v, index, '/');
					indices[i] = std::stoi(index);
				}

				Vector4 pos = positions[indices[0] - 1];
				Vector2 texcoord = texcoords[indices[1] - 1];
				Vector3 normal = normals[indices[2] - 1];
				polygonVertices.push_back({ pos, texcoord, normal });
			}

			// 三角形ファンに変換（左手系巻き順：CCW）
			for (size_t i = 1; i + 1 < polygonVertices.size(); ++i) {
				modelData.vertices.push_back(polygonVertices[i + 1]);
				modelData.vertices.push_back(polygonVertices[i]);
				modelData.vertices.push_back(polygonVertices[0]);
			}

		} else if (identifier == "mtllib") {
			std::string mtlFile;
			s >> mtlFile;
			modelData.material = LoadMaterialTemplateFile(directoryPath, mtlFile);
		}
	}

	return modelData;
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



#pragma region 基礎システムの初期化

	SpriteCommon* spriteCommon = nullptr;

	//スプライト共通部分の初期化
	spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon);

#pragma endregion 基礎システムの初期化

	TextureManager::GetInstance()->Initialize(dxCommon);

	// 2) 使うテクスチャをロード（1回でOK）
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("resources/sample.png");

	std::vector<std::unique_ptr<Sprite>> sprites;
	std::vector<Vector2> basePos;
	const int spriteCount = 5;

	for (int i = 0; i < spriteCount; ++i) {
		// 偶数→uvChecker, 奇数→sample
		std::string texturePath = (i % 2 == 0)
			? "resources/uvChecker.png"
			: "resources/sample.png";

		auto sprite = std::make_unique<Sprite>();
		sprite->Initialize(spriteCommon, dxCommon, texturePath);

		Vector2 p = { 50.0f + i * 150.0f, 100.0f };
		sprite->SetPosition(p);
		sprite->SetScale({ 1.0f, 1.0f, 1.0f });
		sprite->SetColor({ 1.0f, 0.2f * i, 1.0f - 0.2f * i, 1.0f });

		// アンカー左上（0,0）…スライドと同じ
		sprite->SetAnchorPoint({ 0.5f, 0.5f });


		basePos.push_back(p);
		sprites.push_back(std::move(sprite));
	}



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

	// ディスクリプタの先頭を取得する
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	//===========================
	//DescriptorRange
	//===========================
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;//0からはじまる
	descriptorRange[0].NumDescriptors = 1;//数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;//オフセットを自動計算

	//============================

	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	//RootParameter作成。複数設定できるので配列。今回は結果一つなので長さ1つの配列
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//VBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0;//レジスタ番号0とバインド
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//VBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;//PixelShaderで使う
	rootParameters[1].Descriptor.ShaderRegister = 0;//レジスタ番号0とバインド
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//DescriptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;//Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);//Tableで使用する数
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CBVを使う
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
	rootParameters[3].Descriptor.ShaderRegister = 1;//レジスタ番号1とバインド
	descriptionRootSignature.pParameters = rootParameters;//ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);//配列の長さ

	//====================
	//Samplerの作成
	//====================
	D3D12_STATIC_SAMPLER_DESC staticSamplaers[1] = {};
	staticSamplaers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//バイリニアフィルタ
	staticSamplaers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//0~1の範囲外をリピート
	staticSamplaers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplaers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplaers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//比較しない
	staticSamplaers[0].MaxLOD = D3D12_FLOAT32_MAX;//ありったけのMipmapを使う
	staticSamplaers[0].ShaderRegister = 0;//レジスタ番号0を使う
	staticSamplaers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
	descriptionRootSignature.pStaticSamplers = staticSamplaers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplaers);


	//シリアライズしてバイナリする
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

	if (FAILED(hr)) {
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	//バイナリをもとに生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	//InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].InputSlot = 0;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElementDescs[0].InstanceDataStepRate = 0;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].InputSlot = 0;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElementDescs[1].InstanceDataStepRate = 0;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].InputSlot = 0;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElementDescs[2].InstanceDataStepRate = 0;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	//BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	//全ての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;
	//RasiterzerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	//裏面を表示しない
	//rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//裏面を表示する
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	//shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon->CompilesSharder(L"resources/shaders/Object3D.VS.hlsl",
		L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon->CompilesSharder(L"resources/shaders/Object3D.PS.hlsl",
		L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	//書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//比較関数はLessEqual。つまり近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//PSOを生成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();//RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;//InputLayout
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	vertexShaderBlob->GetBufferSize() };//VertexShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	pixelShaderBlob->GetBufferSize() };//PixelShader
	graphicsPipelineStateDesc.BlendState = blendDesc;//BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;//RasterizerState
	//書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//利用するトロポジ(形状)のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//どのように画面に色を打ち込むかの設定(気にしなくていい)
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//実際に生成
	Microsoft::WRL::ComPtr < ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	if (FAILED(hr)) {
		char errorMsg[256];
		sprintf_s(errorMsg, "PSO作成に失敗しました: 0x%08X\n", hr);
		OutputDebugStringA(errorMsg);
		assert(false);  // 強制停止（そのままでもOK）
	}
	//モデル読み込み
	ModelData modelData = LoadObjFile("resources", "plane.obj");
	//頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceModel =  dxCommon->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	//頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewModel{};
	//リソースの先頭アドレスから使う
	vertexBufferViewModel.BufferLocation = vertexResourceModel->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点の数分
	vertexBufferViewModel.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	//1頂点当たりのサイズ
	vertexBufferViewModel.StrideInBytes = sizeof(VertexData);
	//頂点リソースにデータを書き込む
	VertexData* vertexDataModel = nullptr;
	//書き込むためのアドレスを取得
	vertexResourceModel->Map(0, nullptr,
		reinterpret_cast<void**>(&vertexDataModel));
	//頂点データをコピー
	std::memcpy(vertexDataModel, modelData.vertices.data(),
		sizeof(VertexData) * modelData.vertices.size());

	//モデル用のTransformationMatrix用のリソースを作る。Matrix4x4 一つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceModel =  dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	//データを書き込む
	TransformationMatrix* transformationMatrixDataModel = nullptr;
	//書き込むためのアドレスを取得
	transformationMatrixResourceModel->Map(0, nullptr,
		reinterpret_cast<void**>(&transformationMatrixDataModel));
	//単位行列を書き込んでおく
	transformationMatrixDataModel->WVP = Matrix4x4::MakeIdentity4x4();
	transformationMatrixDataModel->World = Matrix4x4::MakeIdentity4x4();
	Transform transformModel{ {1.0f,1.0f,1.0f},{0.0f,2.3f,0.0f}, { 0.0f,0.0f,0.0f } };

	
	////CPUで動かす用のTransformを作る
	Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f}, { 0.0f,0.0f,0.0f } };

	//マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource =  dxCommon->CreateBufferResource( sizeof(Material));
	//マテリアルにデータを書き込む
	Material* materialData = nullptr;
	//書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//今回は赤
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = 2; // ライティングを有効化
	// WVP用の定数バッファを作成
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource =  dxCommon->CreateBufferResource( sizeof(Matrix4x4));
	Matrix4x4* wvpData = nullptr;
	//書き込むためのアドレスを取得
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	// 単位行列を代入（とりあえず変形しない）
	*wvpData = Matrix4x4::MakeIdentity4x4();

	materialData->uvTransform = Matrix4x4::MakeIdentity4x4(); // UV変換行列も単位行列

	Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f },{0.0f,0.0f,0.0f} };


	// --- スプライト用パラメータ（ImGui用） ---
	Transform uvTransformSprite{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};

	//=================
	//   球
	//=================
	//頂点バッファビューを作成する
	const uint32_t kSubdivision = 16; // 分割数

	Microsoft::WRL::ComPtr < ID3D12Resource> vertexResourceSphere =  dxCommon->CreateBufferResource( sizeof(VertexData) * kSubdivision * kSubdivision * 6);

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere{};
	// リソースの先頭アドレスを使う
	vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * kSubdivision * kSubdivision * 6;

	// 1頂点あたりのサイズ
	vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	// 頂点リソースにデータを書き込む
	VertexData* vertexDataSphere = nullptr;
	// 書き込むためのアドレスを取得
	vertexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));

	//======================
	//球の計算
	//======================

	const float kLonEvery = 2.0f * float(M_PI) / float(kSubdivision);
	const float kLatEvery = float(M_PI) / float(kSubdivision);

	// 頂点データの書き込み
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		// 各バンドの南端緯度と北端緯度
		float lat = -0.5f * float(M_PI) + kLatEvery * float(latIndex);
		float latN = lat + kLatEvery;
		// sin/cos を一度だけ計算
		float cosLat = cosf(lat);
		float sinLat = sinf(lat);
		float cosLatN = cosf(latN);
		float sinLatN = sinf(latN);

		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			float lon = kLonEvery * float(lonIndex);
			float lonN = lon + kLonEvery;
			float cosLon = cosf(lon);
			float sinLon = sinf(lon);
			float cosLonN = cosf(lonN);
			float sinLonN = sinf(lonN);

			// テクスチャ座標
			float u = float(lonIndex) / float(kSubdivision);
			float uN = float(lonIndex + 1) / float(kSubdivision);
			float v = 1.0f - float(latIndex) / float(kSubdivision);
			float vN = 1.0f - float(latIndex + 1) / float(kSubdivision);

			// 6頂点分のベースオフセット
			uint32_t base = (latIndex * kSubdivision + lonIndex) * 6;

			// 頂点位置を構築
			// BL (Bottom-Left)
			vertexDataSphere[base].position = { cosLat * cosLon,  sinLat,  cosLat * sinLon, 1.0f };
			vertexDataSphere[base].texcoord = { u,  v };
			// TL (Top-Left)
			vertexDataSphere[base + 1].position = { cosLatN * cosLon,  sinLatN, cosLatN * sinLon, 1.0f };
			vertexDataSphere[base + 1].texcoord = { u,  vN };
			// BR (Bottom-Right)
			vertexDataSphere[base + 2].position = { cosLat * cosLonN, sinLat,  cosLat * sinLonN, 1.0f };
			vertexDataSphere[base + 2].texcoord = { uN, v };
			// TR (Top-Right)
			vertexDataSphere[base + 3].position = { cosLatN * cosLonN, sinLatN, cosLatN * sinLonN, 1.0f };
			vertexDataSphere[base + 3].texcoord = { uN, vN };

			// 法線：ポジションの XYZ を正規化（外向きの放射ベクトル）
			for (int i = 0; i < 4; ++i) {
				Vector3 pos = {
					vertexDataSphere[base + i].position.x,
					vertexDataSphere[base + i].position.y,
					vertexDataSphere[base + i].position.z,
				};
				vertexDataSphere[base + i].normal = Normalize(pos);
			}

			// 2枚目の三角形（法線もきちんと引き継ぐ）
			vertexDataSphere[base + 4] = vertexDataSphere[base + 2]; // BR
			vertexDataSphere[base + 5] = vertexDataSphere[base + 1]; // TL

		}

	}

	//球用のTransformationMatrix用のリソースを作る。Matrix4x4 一つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSphere =  dxCommon->CreateBufferResource( sizeof(TransformationMatrix));
	//データを書き込む
	TransformationMatrix* transformationMatrixDataSphere = nullptr;
	//書き込むためのアドレスを取得
	transformationMatrixResourceSphere->Map(0, nullptr,
		reinterpret_cast<void**>(&transformationMatrixDataSphere));
	//単位行列を書き込んでおく
	transformationMatrixDataSphere->WVP = Matrix4x4::MakeIdentity4x4();
	transformationMatrixDataSphere->World = Matrix4x4::MakeIdentity4x4();


	static Transform transformSphere{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f },{0.0f,0.0f,0.0f} };


	//レンダリングパイプ用のカメラ
	Transform cameraTransform({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,0.0f,-10.0f });


	//ライトのリソース作成
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource =  dxCommon->CreateBufferResource( sizeof(DirectionalLight));
	DirectionalLight* directionalLightData = nullptr;
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//初期化
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // ライトの色
	directionalLightData->direction = Normalize({ 0.0f, -1.0f, 0.0f });//ライトの向き
	directionalLightData->intensity = 1.0f; // ライトの強度

//	//画像を読み込む
//	//Textureを読んで転送する
//	DirectX::ScratchImage mipImages = dxCommon->LoadTexture("resources/uvChecker.png");
//	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
//	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = dxCommon->CreateTextureResource( metadata);
//	/*Microsoft::WRL::ComPtr<ID3D12Resource> val = */dxCommon->UploadTextureData(textureResource, mipImages);
//	//2枚目のTextureを読み込む
//	DirectX::ScratchImage mipImages2 = dxCommon->LoadTexture(modelData.material.textureFilePath);
//	//DirectX::ScratchImage mipImages2 = LoadTexture("resources/axis.jpg");
//	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
//	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = dxCommon->CreateTextureResource( metadata2);
///*	Microsoft::WRL::ComPtr<ID3D12Resource> val2 =*/ dxCommon->UploadTextureData(textureResource2, mipImages2);

	////metaDataをもとにSRVの設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	//srvDesc.Format = metadata.format;
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	//srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);//MipMapの数
	////2つ目
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	//srvDesc2.Format = metadata2.format;
	//srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	//srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);//MipMapの数

	//SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = dxCommon->GetSRVCPUDescriptorHandle(1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = dxCommon->GetSRVGPUDescriptorHandle(1);
	//2個目
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = dxCommon->GetSRVCPUDescriptorHandle(2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = dxCommon->GetSRVGPUDescriptorHandle( 2);


	////SRVの生成
	// dxCommon->GetDevice()->CreateShaderResourceView(
	//	textureResource.Get(), // SRVを作成するリソース
	//	&srvDesc,        // SRVの設定
	//	textureSrvHandleCPU); // 作成するSRVのディスクリプタハンドル

	//if (!textureResource2) {
	//	MessageBoxA(nullptr, "textureResource2 が null です。画像の読み込みや GPU リソース作成に失敗しています。", "エラー", MB_OK);
	//}


	////2個目
	// dxCommon->GetDevice()->CreateShaderResourceView(
	//	textureResource2.Get(), // SRVを作成するリソース
	//	&srvDesc2,        // SRVの設定
	//	textureSrvHandleCPU2); // 作成するSRVのディスクリプタハンドル
	//

	bool useSample = true;


	//ウサギ用の描画
	ModelData modelDataBunny = LoadObjFile("resources", "bunny.obj");
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceModelBunny =
		 dxCommon->CreateBufferResource( sizeof(VertexData) * modelDataBunny.vertices.size());
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewModelBunny{};
	vertexBufferViewModelBunny.BufferLocation = vertexResourceModelBunny->GetGPUVirtualAddress();
	vertexBufferViewModelBunny.SizeInBytes = UINT(sizeof(VertexData) * modelDataBunny.vertices.size());
	vertexBufferViewModelBunny.StrideInBytes = sizeof(VertexData);
	VertexData* vertexDataModelBuuny = nullptr;
	vertexResourceModelBunny->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataModelBuuny));
	std::memcpy(vertexDataModelBuuny, modelDataBunny.vertices.data(),
		sizeof(VertexData) * modelDataBunny.vertices.size());
	vertexResourceModelBunny->Unmap(0, nullptr);

	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceModelBunny =
		 dxCommon->CreateBufferResource( sizeof(TransformationMatrix));
	TransformationMatrix* transformationMatrixDataModelBunny = nullptr;
	transformationMatrixResourceModelBunny->Map(0, nullptr,
		reinterpret_cast<void**>(&transformationMatrixDataModelBunny));
	transformationMatrixDataModelBunny->WVP = Matrix4x4::MakeIdentity4x4();
	transformationMatrixDataModelBunny->World = Matrix4x4::MakeIdentity4x4();

	Transform transformModelBunny{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {2.0f,0.0f,0.0f} };

	////ティーポット用の描画

	//ModelData modelDataTea = LoadObjFile("resources", "teapot.obj");
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceModelTea =
	//	 dxCommon->CreateBufferResource( sizeof(VertexData) * modelDataTea.vertices.size());
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferViewModelTea{};
	//vertexBufferViewModelTea.BufferLocation = vertexResourceModelTea->GetGPUVirtualAddress();
	//vertexBufferViewModelTea.SizeInBytes = UINT(sizeof(VertexData) * modelDataTea.vertices.size());
	//vertexBufferViewModelTea.StrideInBytes = sizeof(VertexData);
	//VertexData* vertexDataModelTea = nullptr;
	//vertexResourceModelTea->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataModelTea));
	//std::memcpy(vertexDataModelTea, modelDataTea.vertices.data(),
	//	sizeof(VertexData) * modelDataTea.vertices.size());
	//vertexResourceModelTea->Unmap(0, nullptr);

	//DirectX::ScratchImage mipImagesTea = dxCommon->LoadTexture(modelDataTea.material.textureFilePath);
	////DirectX::ScratchImage mipImagesTea = LoadTexture("resources/axis.jpg");
	//const DirectX::TexMetadata& metadataTea = mipImagesTea.GetMetadata();
	//Microsoft::WRL::ComPtr<ID3D12Resource> textureResourceTea = dxCommon->CreateTextureResource( metadataTea);
	///*Microsoft::WRL::ComPtr<ID3D12Resource> valTea = */dxCommon->UploadTextureData(textureResourceTea, mipImagesTea);

	//Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceModelTea =
	//	 dxCommon->CreateBufferResource( sizeof(TransformationMatrix));
	//TransformationMatrix* transformationMatrixDataModelTea = nullptr;
	//transformationMatrixResourceModelTea->Map(0, nullptr,
	//	reinterpret_cast<void**>(&transformationMatrixDataModelTea));
	//transformationMatrixDataModelTea->WVP = Matrix4x4::MakeIdentity4x4();
	//transformationMatrixDataModelTea->World = Matrix4x4::MakeIdentity4x4();

	//Transform transformModelTea{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {2.0f,0.0f,0.0f} };


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

		//三角形の表示させる処理
		transform.rotate.y += 0.03f;
		Matrix4x4 worldMatrix = Matrix4x4::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		Matrix4x4 cameraMatrix = Matrix4x4::MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
		Matrix4x4 viewMatrix = Matrix4x4::Inverse(cameraMatrix);
		Matrix4x4 projectionMatrix = Matrix4x4::PerspectiveFov(0.45f, float(winApp->kClientWidth) / float(winApp->kClientHeight), 0.1f, 100.0f);
		Matrix4x4 worldViewProjectionMatrix = Matrix4x4::Multiply(worldMatrix, Matrix4x4::Multiply(viewMatrix, projectionMatrix));
		*wvpData = worldViewProjectionMatrix;

			ImGui::Checkbox("useSample", &useSample);

			ImGui::DragFloat3("rotateSpehre", &transformSphere.rotate.x, 0.1f, -10.0f, 10.0f);
			ImGui::Begin("WorldMatrix");
			ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
			ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);
			ImGui::DragFloat3("rotateModel", &transformModel.rotate.x, 0.1f);



		ImGui::Begin("Draw Options");
		ImGui::Checkbox("Draw Sphere", &isDrawSphere);
		ImGui::Checkbox("Draw Model", &isDrawModel);
		ImGui::Checkbox("Draw Sprite", &isDrawSprite);
		ImGui::Checkbox("Draw Bunny", &isDrawBunny);
		ImGui::Checkbox("Draw Tea", &isDrawTea);

		// 0: Unlit, 1: Lambert, 2: Half-Lambert
		const char* lightingModes[] = { "Not", "Lambert", "Half-Lambert" };
		ImGui::Combo("Lighting Mode", &materialData->enableLighting, lightingModes, IM_ARRAYSIZE(lightingModes));



		ImGui::Begin("Editor");

		if (isDrawModel) {

			// モデル
			ImGui::Text("Model Transform");
			ImGui::DragFloat3("Model Position", &transformModel.translate.x, 0.1f);
			ImGui::DragFloat3("Model Rotation", &transformModel.rotate.x, 0.1f);
			ImGui::DragFloat3("Model Scale", &transformModel.scale.x, 0.1f);
		}

		if (isDrawBunny) {
			ImGui::Text("Bunny Transform");
			ImGui::DragFloat3("Bunny Position", &transformModelBunny.translate.x, 0.1f);
			ImGui::DragFloat3("Bunny Rotation", &transformModelBunny.rotate.x, 0.1f);
			ImGui::DragFloat3("Bunny Scale", &transformModelBunny.scale.x, 0.1f);
		}

		/*if (isDrawTea) {

			ImGui::Text("Tea Transform");
			ImGui::DragFloat3("Tea Position", &transformModelTea.translate.x, 0.1f);
			ImGui::DragFloat3("Tea Rotation", &transformModelTea.rotate.x, 0.1f);
			ImGui::DragFloat3("Tea Scale", &transformModelTea.scale.x, 0.1f);

		}*/

		// スフィア
		ImGui::Separator();
		ImGui::Text("Sphere Transform");
		ImGui::DragFloat3("Sphere Position", &transformSphere.translate.x, 0.1f);
		ImGui::DragFloat3("Sphere Rotation", &transformSphere.rotate.x, 0.1f);
		ImGui::DragFloat3("Sphere Scale", &transformSphere.scale.x, 0.1f);

		// スプライト
		ImGui::Separator();
		ImGui::Text("Sprite UV Transform");
		ImGui::DragFloat2("UV Translate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
		ImGui::DragFloat2("UV Scale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
		ImGui::SliderAngle("UV Rotate", &uvTransformSprite.rotate.z);

		// ==== ライト ====
		ImGui::Separator();
		ImGui::Text("Light Settings");
		ImGui::ColorEdit3("Light Color", &directionalLightData->color.x);
		ImGui::DragFloat("Light Intensity", &directionalLightData->intensity, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat3("Light Direction", &directionalLightData->direction.x, 0.01f, -1.0f, 1.0f);


		//音
		ImGui::Separator();
		ImGui::Text("Sound");
		if (ImGui::Button("soundWav")) {
			SoundPlayerWave(xAudio2.Get(), soundData1);
		}

		ImGui::Separator();
		ImGui::Text("Model UV Transform");
		static Transform uvTransformModel{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
		ImGui::DragFloat2("Model UV Translate", &uvTransformModel.translate.x, 0.01f, -10.0f, 10.0f);
		ImGui::DragFloat2("Model UV Scale", &uvTransformModel.scale.x, 0.01f, -10.0f, 10.0f);
		ImGui::SliderAngle("Model UV Rotate", &uvTransformModel.rotate.z);

		// UV変換マトリクスを組み立て
		Matrix4x4 uvTransformMatrixModel = Matrix4x4::Scale(uvTransformModel.scale);
		uvTransformMatrixModel = Matrix4x4::Multiply(
			uvTransformMatrixModel, Matrix4x4::MakeRotateZMatrix(uvTransformModel.rotate.z));
		uvTransformMatrixModel = Matrix4x4::Multiply(
			uvTransformMatrixModel, Matrix4x4::Translation(uvTransformModel.translate));

		// マテリアルへ転送
		materialData->uvTransform = uvTransformMatrixModel;


		ImGui::End();

		ImGui::Begin("Performance");
		ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
		ImGui::End();

	

		//スプライトのUVを変換する行列を計算
		Matrix4x4 uvMatrix =
			Matrix4x4::Scale(uvTransformSprite.scale) *
			Matrix4x4::RotateZ(uvTransformSprite.rotate.z) *
			Matrix4x4::Translation(uvTransformSprite.translate);
		for (auto& sp : sprites) {
			sp->SetUVTransform(uvMatrix);    // ← 既存インスタンスへ反映
		}

		//=================
		//球に関する処理
		//=================
		// 回転
		//transformSphere.rotate.y += 0.03f;

		// ワールド行列
		Matrix4x4 worldMatrixSphere = Matrix4x4::MakeAffineMatrix(
			transformSphere.scale, transformSphere.rotate, transformSphere.translate);

		// カメラ
		Matrix4x4 cameraMatrixSphere = Matrix4x4::MakeAffineMatrix(
			cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
		Matrix4x4 viewMatrixSphere = Matrix4x4::Inverse(cameraMatrixSphere);

		// 射影行列
		Matrix4x4 projectionMatrixSphere = Matrix4x4::PerspectiveFov(
			0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 360.0f);

		// WVP = World * View * Projection
		Matrix4x4 wvpSphere = Matrix4x4::Multiply(
			worldMatrixSphere,
			Matrix4x4::Multiply(viewMatrixSphere, projectionMatrixSphere));

		// TransformationMatrix へ代入
		transformationMatrixDataSphere->WVP = wvpSphere;
		transformationMatrixDataSphere->World = worldMatrixSphere;

		directionalLightData->direction = Normalize(directionalLightData->direction);

		//===========
		//モデルの計算
		//===========
		Matrix4x4 worldMatrixModel = Matrix4x4::MakeAffineMatrix(
			transformModel.scale,
			transformModel.rotate,
			transformModel.translate
		);

		/*	Matrix4x4 cameraMatrixModel = Matrix4x4::MakeAffineMatrix(
				cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrixModel = Matrix4x4::Inverse(cameraMatrixModel);*/

		Matrix4x4 viewMatrixModel = debugCamera.GetViewMatrix();


		Matrix4x4 projectionMatrixModel = Matrix4x4::PerspectiveFov(
			0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);

		Matrix4x4 wvpModel = Matrix4x4::Multiply(
			worldMatrixModel,
			Matrix4x4::Multiply(viewMatrixModel, projectionMatrixModel)
		);

		// 毎フレームこれを実行して transformationMatrixDataModel に書き込む必要あり！
		transformationMatrixDataModel->WVP = wvpModel;
		transformationMatrixDataModel->World = worldMatrixModel;

		//ウサギ用の計算
		Matrix4x4 worldMatrixModelBunny = Matrix4x4::MakeAffineMatrix(
			transformModelBunny.scale,
			transformModelBunny.rotate,
			transformModelBunny.translate
		);
		Matrix4x4 wvpModelBunny = Matrix4x4::Multiply(
			worldMatrixModelBunny,
			Matrix4x4::Multiply(viewMatrixModel, projectionMatrixModel)
		);
		transformationMatrixDataModelBunny->WVP = wvpModelBunny;
		transformationMatrixDataModelBunny->World = worldMatrixModelBunny;

		////ティーポット用の計算

		//Matrix4x4 worldMatrixModelTea = Matrix4x4::MakeAffineMatrix(
		//	transformModelTea.scale,
		//	transformModelTea.rotate,
		//	transformModelTea.translate
		//);
		//Matrix4x4 wvpModelTea = Matrix4x4::Multiply(
		//	worldMatrixModelTea,
		//	Matrix4x4::Multiply(viewMatrixModel, projectionMatrixModel)
		//);
		//transformationMatrixDataModelTea->WVP = wvpModelTea;
		//transformationMatrixDataModelTea->World = worldMatrixModelTea;


		ImGui::End();

		// ===== GPUコマンド発行開始 =====
		dxCommon->PreDraw();                  // ← クリア & バリア遷移
		dxCommon->SetDescriptorHeaps();       // ← SRVヒープをセット

		//Spriteの描画準備。Spriteの描画に共通のグラフィックコマンドを詰む
		spriteCommon->SetGraphicsPipelineState();

		// 共有設定（PSO/RootSig/トポロジ）
		dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
		dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
		dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// ---- Sphere ----
		if (isDrawSphere) {
			dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());                // Pixel用 Material
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSphere->GetGPUVirtualAddress()); // Vertex用 WVP/World
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());        // Pixel用 Light
			dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, useSample ? textureSrvHandleGPU2 : textureSrvHandleGPU);     // SRV
			dxCommon->GetCommandList()->DrawInstanced(kSubdivision * kSubdivision * 6, 1, 0, 0);
		}

		// ---- Model (plane.obj) ----
		if (isDrawModel) {
			dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewModel);
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceModel->GetGPUVirtualAddress());
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
			dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU2);
			dxCommon->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
		}

		// ---- Bunny ----
		if (isDrawBunny) {
			dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewModelBunny);
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceModelBunny->GetGPUVirtualAddress());
			dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
			dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU2);
			dxCommon->GetCommandList()->DrawInstanced(UINT(modelDataBunny.vertices.size()), 1, 0, 0);
		}


		//// ---- Teapot ----
		//if (isDrawTea) {
		//	dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewModelTea);
		//	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
		//	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceModelTea->GetGPUVirtualAddress());
		//	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
		//	dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU2);
		//	dxCommon->GetCommandList()->DrawInstanced(UINT(modelDataTea.vertices.size()), 1, 0, 0);
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

	return 0;
}

