#include "SphereMesh.h"

void SphereMesh::Initialize(ID3D12Device* device, uint32_t subdivision) {
    const uint32_t kVertexNum = subdivision * subdivision * 6;
    vertexCount_ = kVertexNum;

    // 頂点バッファ作成
    size_t bufferSize = sizeof(VertexData) * kVertexNum;
    D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = bufferSize;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource_));

    VertexData* vtx = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vtx));

    const float kLonStep = 2.0f * float(M_PI) / subdivision;
    const float kLatStep = float(M_PI) / subdivision;

    for (uint32_t lat = 0; lat < subdivision; ++lat) {
        float lat0 = -0.5f * float(M_PI) + kLatStep * lat;
        float lat1 = lat0 + kLatStep;
        float sinLat0 = sinf(lat0), cosLat0 = cosf(lat0);
        float sinLat1 = sinf(lat1), cosLat1 = cosf(lat1);

        for (uint32_t lon = 0; lon < subdivision; ++lon) {
            float lon0 = lon * kLonStep;
            float lon1 = lon0 + kLonStep;
            float sinLon0 = sinf(lon0), cosLon0 = cosf(lon0);
            float sinLon1 = sinf(lon1), cosLon1 = cosf(lon1);

            uint32_t base = (lat * subdivision + lon) * 6;

            vtx[base + 0].position = { cosLat0 * cosLon0, sinLat0, cosLat0 * sinLon0, 1 }; vtx[base + 0].texcoord = { lon / float(subdivision),     1 - lat / float(subdivision) };
            vtx[base + 1].position = { cosLat1 * cosLon0, sinLat1, cosLat1 * sinLon0, 1 }; vtx[base + 1].texcoord = { lon / float(subdivision),     1 - (lat + 1) / float(subdivision) };
            vtx[base + 2].position = { cosLat0 * cosLon1, sinLat0, cosLat0 * sinLon1, 1 }; vtx[base + 2].texcoord = { (lon + 1) / float(subdivision), 1 - lat / float(subdivision) };

            vtx[base + 3] = vtx[base + 2];
            vtx[base + 4] = vtx[base + 1];
            vtx[base + 5].position = { cosLat1 * cosLon1, sinLat1, cosLat1 * sinLon1, 1 }; vtx[base + 5].texcoord = { (lon + 1) / float(subdivision), 1 - (lat + 1) / float(subdivision) };
        }
    }
    vertexResource_->Unmap(0, nullptr);

    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.StrideInBytes = sizeof(VertexData);
    vbv_.SizeInBytes = static_cast<UINT>(bufferSize);

   // wvpResource_ = CreateBufferResource(device, sizeof(Matrix4x4));
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpMap_));
    *wvpMap_ = Matrix4x4::MakeIdentity4x4();
}

void SphereMesh::Update(const Transform& camera, float aspect) {
    transform_.rotate.y += 0.03f;
    Matrix4x4 world = Matrix4x4::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 camMat = Matrix4x4::MakeAffineMatrix(camera.scale, camera.rotate, camera.translate);
    Matrix4x4 view = Matrix4x4::Inverse(camMat);
    Matrix4x4 proj = Matrix4x4::PerspectiveFov(0.45f, aspect, 0.1f, 100.0f);
    *wvpMap_ = Matrix4x4::Multiply(world, Matrix4x4::Multiply(view, proj));
}

void SphereMesh::Draw(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS matAddr) {
    cmdList->IASetVertexBuffers(0, 1, &vbv_);
    cmdList->SetGraphicsRootConstantBufferView(0, matAddr);
    cmdList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
    cmdList->DrawInstanced(vertexCount_, 1, 0, 0);
}