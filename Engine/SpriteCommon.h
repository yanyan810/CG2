#pragma once
#include <d3d12.h>
#include <wrl.h>

class DirectXCommon;

class SpriteCommon {
public:
    void Initialize(DirectXCommon* dx);

    ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
    ID3D12PipelineState* GetPipelineState() const { return pso_.Get(); }

    DirectXCommon* GetDxCommon() const { return dx_; }

    //共通描画設定
	void SetGraphicsPipelineState();

private:
    // ルートシグネチャの作成
    void CreateRootSignature();
    // グラフィックスパイプラインの生成
    void CreateGraphicsPipelineState();

private:
    DirectXCommon* dx_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

    // ※ 今の頂点構造(VertexData)に合わせて POSITION/TEXCOORD/NORMAL の3要素
    D3D12_INPUT_ELEMENT_DESC inputElems_[3] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_INPUT_LAYOUT_DESC inputLayout_{ inputElems_, 3 };
};
