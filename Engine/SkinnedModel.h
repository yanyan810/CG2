#pragma once
#include <string>
#include <vector>
#include <cstdint>

#include "Vector3.h"
#include "Matrix4x4.h"
#include "DirectXCommon.h"
#include "TextureManager.h"

#include <wrl.h>
#include <assimp/scene.h>

class SkinnedModel {
public:
    // ==== 頂点構造体（ボーン4本まで）====
    struct SkinVertex {
        Vector4 position;
        Vector3 normal;
        Vector2 texcoord;
        uint32_t boneIndex[4];
        float    boneWeight[4];
    };

    // マテリアル（最小限）
    struct MaterialCBData {
        Vector4  color;
        int32_t  enableLighting;
        float    pad[3];
        Matrix4x4 uvTransform;
    };

    // Transform CB
    struct TransformCBData {
        Matrix4x4 worldViewProj;
        Matrix4x4 world;
    };

    struct Bone {
        std::string name;
        int         parentIndex;
        Matrix4x4   offsetMatrix; // inverse bind pose
    };

public:
    void Initialize(DirectXCommon* dx, const std::string& filePath);

    void Draw();

    // ==== Transform setter ====
    void SetScale(const Vector3& s) { scale_ = s; }
    void SetRotate(const Vector3& r) { rotate_ = r; }
    void SetTranslate(const Vector3& t) { translate_ = t; }

    // カメラ制御用
    void SetCameraRotate(const Vector3& r) { cameraRotate_ = r; }
    void SetCameraTranslate(const Vector3& t) { cameraTranslate_ = t; }

    void SetDebugBoneRotate(int index, const Vector3& rot);
    const std::vector<Bone>& GetBones() const { return bones_; }
    void SetDebugBoneTranslate(int index, const Vector3& trans);


private:
    void LoadFbx_(const std::string& filePath);
    void CreateBuffers_();
    void CreatePipelineIfNeeded_();

private:
    DirectXCommon* dx_ = nullptr;

    // 頂点
    std::vector<SkinVertex> vertices_;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbView_{};

    // マテリアル
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    MaterialCBData* materialData_ = nullptr;

    // Transform CB
    Microsoft::WRL::ComPtr<ID3D12Resource> transformResource_;
    TransformCBData* transformData_ = nullptr;

    // Bone 行列 CB
    std::vector<Bone> bones_;
    std::vector<Matrix4x4> boneMatrices_;
    Microsoft::WRL::ComPtr<ID3D12Resource> boneMatrixResource_;
    Matrix4x4* boneMatrixData_ = nullptr;

    //ボーンごとのデバッグ用回転
    std::vector<Vector3>    debugBoneRot_;   // ラジアン
    bool                    debugPoseEnable_ = true;
    std::vector<Vector3>    debugBoneTrans_;

    // テクスチャ
    uint32_t textureIndex_ = 0;

    // Transform 値
    Vector3 scale_{ 1.0f,1.0f,1.0f };
    Vector3 rotate_{ 0.0f,0.0f,0.0f };
    Vector3 translate_{ 0.0f,0.0f,0.0f };

    // カメラ（とりあえず Object3d と同じ値にしておく）
    Vector3 cameraScale_{ 1.0f,1.0f,1.0f };
    Vector3 cameraRotate_{ 0.3f,0.0f,0.0f };
    Vector3 cameraTranslate_{ 0.0f,4.0f,-10.0f };

    // ==== 共有 RootSignature / PSO ====
    static Microsoft::WRL::ComPtr<ID3D12RootSignature> sRootSignature_;
    static Microsoft::WRL::ComPtr<ID3D12PipelineState> sPipelineState_;
};
