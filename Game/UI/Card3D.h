#pragma once
#include <memory>
#include <d3d12.h>

#include "Object3d.h"
#include "Object3dCommon.h"
#include "TextureManager.h"
#include "CardDef.h"

class Camera;
class DirectXCommon;

class Card3D {
public:

    void Initialize(
        Object3dCommon* objCom,
        DirectXCommon* dx,
        Camera* cam,
        const CardDef& def);

    void SetTransform(const Vector3& pos, const Vector3& rot, const Vector3& scale);
    void Update(float dt);
    void Draw();

    Vector3 GetWorldPos() const { return pos_; }

private:

    std::unique_ptr<Object3d> frame_;
    std::unique_ptr<Object3d> art_;

    D3D12_GPU_DESCRIPTOR_HANDLE artSrv_{};

    Vector3 pos_{};
    Vector3 rot_{};
    Vector3 scale_{ 1,1,1 };
};