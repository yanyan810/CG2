#include "PostEffect.h"

void PostEffect::Initialize(DirectXCommon* dxCommon, BloomConstantBuffer* bloomCB) {
	dxCommon_ = dxCommon;
	bloomCB_ = bloomCB;
}

void PostEffect::SetBloomConstantBuffer(BloomConstantBuffer* bloomCB)
{
	bloomCB_ = bloomCB;
}

void PostEffect::Draw(D3D12_GPU_DESCRIPTOR_HANDLE inputSRV, PostEffectType blendMode)
{
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetPSOEffect(blendMode).root_.GetSignature().Get());
	dxCommon_->GetCommandList()->SetPipelineState(dxCommon_->GetPSOEffect(blendMode).graphicsState_.Get());

	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, bloomCB_->GetGPUAddress());
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, inputSRV);

	// 全画面三角形
	dxCommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);

}

void PostEffect::DrawComposite(D3D12_GPU_DESCRIPTOR_HANDLE sceneSRV, D3D12_GPU_DESCRIPTOR_HANDLE bloomSRV) {

	dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetPSOEffect(Bloom_Composite).root_.GetSignature().Get());
	dxCommon_->GetCommandList()->SetPipelineState(dxCommon_->GetPSOEffect(Bloom_Composite).graphicsState_.Get());

	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, bloomCB_->GetGPUAddress());
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, sceneSRV);
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, bloomSRV);

	dxCommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}

void PostEffect::DrawObjectComposite(D3D12_GPU_DESCRIPTOR_HANDLE objectSRV, D3D12_GPU_DESCRIPTOR_HANDLE bloomSRV) {

	dxCommon_->GetCommandList()->SetGraphicsRootSignature(dxCommon_->GetPSOEffect(ObjectPost_Composite).root_.GetSignature().Get());
	dxCommon_->GetCommandList()->SetPipelineState(dxCommon_->GetPSOEffect(ObjectPost_Composite).graphicsState_.Get());

	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, bloomCB_->GetGPUAddress());
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, objectSRV);
	dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, bloomSRV);

	dxCommon_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
}
