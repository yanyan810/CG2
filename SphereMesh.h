#pragma once
#include <d3d12.h>
#include "Matrix4x4.h"
#include "Vector.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <cassert>


struct VertexData {
    Vector4 position;
    Vector2 texcoord;
};

class SphereMesh {
public:
    void Initialize(ID3D12Device* device, uint32_t subdivision);
    void Update(const Transform& camera, float aspect);
    void Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS matAddr);

    D3D12_GPU_VIRTUAL_ADDRESS GetWVPAddress() const { return wvpResource_->GetGPUVirtualAddress(); }
    D3D12_VERTEX_BUFFER_VIEW GetVBV() const { return vbv_; }

private:
    ID3D12Resource* vertexResource_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    ID3D12Resource* wvpResource_ = nullptr;
    Matrix4x4* wvpMap_ = nullptr;
    Transform transform_{ {1,1,1}, {0,0,0}, {0,0,0} };
    uint32_t vertexCount_ = 0;
};