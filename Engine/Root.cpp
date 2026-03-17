#include "Root.h"

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

void Root::Create(Microsoft::WRL::ComPtr<ID3D12Device>& device)
{

	// バイナリをもとに生成
	HRESULT hr = device->CreateRootSignature(0,
		signatureBlob_->GetBufferPointer(), signatureBlob_->GetBufferSize(),
		IID_PPV_ARGS(&signature_));
	assert(SUCCEEDED(hr));

}
