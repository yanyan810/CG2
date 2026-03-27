#include "Root.h"

void Root::InitalizeForModelParticle()
{
	descriptionSignature_.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// RootParameterの設定 (ModelParticleシェーダーに合わせる)
	// index 0: Material (b0, Pixel)
	Parameters_[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	Parameters_[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Parameters_[0].Descriptor.ShaderRegister = 0;

	// index 1: DirectionalLight (b1, Pixel)
	Parameters_[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	Parameters_[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Parameters_[1].Descriptor.ShaderRegister = 1;

	// index 2: Camera (b2, Pixel)
	Parameters_[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	Parameters_[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Parameters_[2].Descriptor.ShaderRegister = 2;

	// index 3: StructuredBuffer (t1, Vertex) -> DescriptorTableとして定義
	descriptorRange_[0].BaseShaderRegister = 1; // t1
	descriptorRange_[0].NumDescriptors = 1;
	descriptorRange_[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange_[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	Parameters_[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Parameters_[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // 頂点シェーダーで使用
	Parameters_[3].DescriptorTable.pDescriptorRanges = &descriptorRange_[0];
	Parameters_[3].DescriptorTable.NumDescriptorRanges = 1;

	// index 4: Texture (t0, Pixel) -> DescriptorTable
	descriptorRange_[1].BaseShaderRegister = 0; // t0
	descriptorRange_[1].NumDescriptors = 1;
	descriptorRange_[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange_[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	Parameters_[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Parameters_[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Parameters_[4].DescriptorTable.pDescriptorRanges = &descriptorRange_[1];
	Parameters_[4].DescriptorTable.NumDescriptorRanges = 1;

	descriptionSignature_.pParameters = Parameters_;
	descriptionSignature_.NumParameters = 5;

	// サンプラー設定 (既存のものを利用)
	staticSamplers_[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers_[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers_[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers_[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers_[0].ShaderRegister = 0;
	staticSamplers_[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionSignature_.pStaticSamplers = staticSamplers_;
	descriptionSignature_.NumStaticSamplers = 1;

	// シリアライズと生成
	HRESULT hr = D3D12SerializeRootSignature(&descriptionSignature_, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob_, &errorBlob_);
	if (FAILED(hr)) {
		assert(false);
	}
}
void Root::InitializeForPostEffect()
{
	descriptionSignature_.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// -------- RootParameter 0 : CBV (BloomParam)
	Parameters_[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	Parameters_[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Parameters_[0].Descriptor.ShaderRegister = 0; // b0
	Parameters_[0].Descriptor.RegisterSpace = 0;

	// -------- RootParameter 1 : SRV DescriptorTable (SceneRT)
	descriptorRange_[0].BaseShaderRegister = 0; // t0
	descriptorRange_[0].NumDescriptors = 2;     // ★ 修正
	descriptorRange_[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange_[0].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	Parameters_[1].ParameterType =
		D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Parameters_[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Parameters_[1].DescriptorTable.pDescriptorRanges = descriptorRange_;
	Parameters_[1].DescriptorTable.NumDescriptorRanges = 1;

	descriptionSignature_.pParameters = Parameters_;
	descriptionSignature_.NumParameters = 2;

	// -------- Static Sampler (s0)
	staticSamplers_[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers_[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers_[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers_[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers_[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers_[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers_[0].ShaderRegister = 0; // s0
	staticSamplers_[0].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionSignature_.pStaticSamplers = staticSamplers_;
	descriptionSignature_.NumStaticSamplers = 1;

	// -------- Serialize
	HRESULT hr = D3D12SerializeRootSignature(
		&descriptionSignature_,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signatureBlob_,
		&errorBlob_);

	if (FAILED(hr)) {
		assert(false);
	}
}

void Root::InitalizeForShadow() {
	descriptionSignature_.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// パラメータは WVP (b0) だけでOK
	Parameters_[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	Parameters_[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	Parameters_[0].Descriptor.ShaderRegister = 0;

	descriptionSignature_.pParameters = Parameters_;
	descriptionSignature_.NumParameters = 1; // 1つだけ

	// サンプラーは不要
	descriptionSignature_.pStaticSamplers = nullptr;
	descriptionSignature_.NumStaticSamplers = 0;

	HRESULT hr = D3D12SerializeRootSignature(
		&descriptionSignature_,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signatureBlob_,
		&errorBlob_
	);
}

void Root::InitalizeForTrail()
{
	descriptionSignature_.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// [0] Material (Pixel b0) : 色情報
	Parameters_[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	Parameters_[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Parameters_[0].Descriptor.ShaderRegister = 0;

	// [1] ViewProjectionMatrix (Vertex b0) : カメラ行列
	// 軌跡の頂点は既にワールド座標で計算されることが多いため、World行列を含まないVP行列のみでもOK
	Parameters_[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	Parameters_[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	Parameters_[1].Descriptor.ShaderRegister = 0;

	// [2] DescriptorTable (Texture t0)
	descriptorRange_[0].BaseShaderRegister = 0;
	descriptorRange_[0].NumDescriptors = 1;
	descriptorRange_[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange_[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	Parameters_[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Parameters_[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Parameters_[2].DescriptorTable.pDescriptorRanges = &descriptorRange_[0];
	Parameters_[2].DescriptorTable.NumDescriptorRanges = 1;

	descriptionSignature_.pParameters = Parameters_;
	descriptionSignature_.NumParameters = 3;

	// サンプラー設定 (既存の staticSamplers_[0] と同様)
	staticSamplers_[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers_[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers_[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers_[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers_[0].ShaderRegister = 0;
	staticSamplers_[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionSignature_.pStaticSamplers = staticSamplers_;
	descriptionSignature_.NumStaticSamplers = 1;

	HRESULT hr = D3D12SerializeRootSignature(
		&descriptionSignature_,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signatureBlob_,
		&errorBlob_
	);
}

void Root::Create(Microsoft::WRL::ComPtr<ID3D12Device>& device)
{

	// バイナリをもとに生成
	HRESULT hr = device->CreateRootSignature(0,
		signatureBlob_->GetBufferPointer(), signatureBlob_->GetBufferSize(),
		IID_PPV_ARGS(&signature_));
	assert(SUCCEEDED(hr));

}
