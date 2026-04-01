#pragma once
#include <deque>
#include "DirectXCommon.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>

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

struct TrailConfig {
    Vector4 startColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 出現時の色
    Vector4 endColor = { 1.0f, 1.0f, 1.0f, 0.0f };   // 消える時の色
    uint32_t interpolationSteps = 4;                 // 補間分割数
    uint32_t maxPoints = 50;                         // 軌跡の長さ

    // JSON変換 (ModelParticleManagerと同様に)
    nlohmann::json ToJson() const {
        return nlohmann::json{
            {"startColor", {startColor.x, startColor.y, startColor.z, startColor.w}},
            {"endColor", {endColor.x, endColor.y, endColor.z, endColor.w}},
            {"interpolationSteps", interpolationSteps},
            {"maxPoints", maxPoints}
        };
    }

    void FromJson(const nlohmann::json& j) {
        if (j.contains("startColor")) {
            startColor = { j["startColor"][0], j["startColor"][1], j["startColor"][2], j["startColor"][3] };
        }
        if (j.contains("endColor")) {
            endColor = { j["endColor"][0], j["endColor"][1], j["endColor"][2], j["endColor"][3] };
        }
        interpolationSteps = j.value("interpolationSteps", interpolationSteps);
        maxPoints = j.value("maxPoints", maxPoints);
    }
};

class TrailManager {
public:
    static const uint32_t kInterpolationSteps = 4; // 1フレーム間を何分割するか
    // 頂点バッファの最大サイズもこれに合わせて増やしておく必要があります
    static const uint32_t kMaxPoints = 50; // 記録する最大フレーム数
    static const uint32_t kMaxVertices = (kMaxPoints - 1) * kInterpolationSteps * 2;

    void Initialize(DirectXCommon* dxcommon, Object3dCommon* object3dCommon, const std::string& textureFilePath);

    // 更新：新しい先端・根元の位置を渡す
    void Update(const Vector3& tipPos, const Vector3& basePos, const TrailConfig& config);

    void Draw(const Matrix4x4& viewProjection);

    Vector3 Transform(const Vector3& v, const Matrix4x4& m);

    void UpdateImGui(TrailConfig& trailConfig);
    void SaveToJson(const std::string& path, const TrailConfig& config);
    void LoadFromJson(const std::string& path, TrailConfig& config);
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
    Vector4 Lerp(const Vector4& start, const Vector4& end, float t);
    
    // TrailManager.h に追加
    float debugTimer_ = 0.0f;
    bool isPreviewActive_ = false;
};