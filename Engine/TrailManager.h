#pragma once
#include <deque>
#include "DirectXCommon.h"
#include "TextureManager.h"
#include "Object3dCommon.h"

// 1点分の剣の状態
struct SwordSection {
    Vector3 tip;  // 先端の座標
    Vector3 base; // 根元の座標
};

struct TrailVertex {
    Vector3 pos;   // POSITION
    Vector4 color; // COLOR (ここを毎フレーム変えてフェードアウトさせる)
    Vector2 uv;    // TEXCOORD
};

class TrailManager {
public:
    static const uint32_t kInterpolationSteps = 4; // 1フレーム間を何分割するか
    // 頂点バッファの最大サイズもこれに合わせて増やしておく必要があります
    static const uint32_t kMaxPoints = 50; // 記録する最大フレーム数
    static const uint32_t kMaxVertices = (kMaxPoints - 1) * kInterpolationSteps * 2;

    void Initialize(DirectXCommon* dxcommon, Object3dCommon* object3dCommon, const std::string& textureFilePath);

    // 更新：新しい先端・根元の位置を渡す
    void Update(const Vector3& tipPos, const Vector3& basePos);

    void Draw(const Matrix4x4& viewProjection);

    Vector3 Transform(const Vector3& v, const Matrix4x4& m);

private:

    Object3dCommon* object3dCommon_;

    DirectXCommon* dxCommon_ = nullptr;

    // 頂点バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    TrailVertex* vertexData_ = nullptr;

    // 定数バッファ（ViewProjection用）
    Microsoft::WRL::ComPtr<ID3D12Resource> constResource_;
    Matrix4x4* constData_ = nullptr;

    // 座標履歴
    std::deque<SwordSection> points_;

    std::string textureFilePath_;

    uint32_t currentVertexCount_;

    Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

};