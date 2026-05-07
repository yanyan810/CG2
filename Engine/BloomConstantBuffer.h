#pragma once
#include "DirectXCommon.h"
#include "Vector3.h"

struct BloomParam
{
	float threshold;
	float intensity;
	float vignetteIntensity;
	float vignetteScale;
	float timer; // 経過時間
	float distortionAmount; // うねうねの強さ
	float chromAbAmount; // 色収差（にじみ）の強さ
	float isGrayscale;
	float isInverted;
	float noiseIntensity; // ノイズの強さ
	float scanlineIntensity; // 走査線の強さ
	float scanlineFrequency; // 走査線の密度
	float curvature; // 画面の膨らみ具合 (0.02 くらいがおすすめ)
	float borderSharp; // 枠の角の鋭さ (20.0 くらい)
	float glitchAmount; // 追加：グリッチの強さ（0.0 ~ 0.1くらい）
	float padding; // paddingを調節して16バイト境界に合わせる
	float dissolveAmount; // 0.0で表示、1.0で消える。0未満で無効
	float dissolveEdgeWidth;
	float dissolveEdgeIntensity;
	float dissolveNoiseScale;
	Vector4 dissolveEdgeColor;
};


class BloomConstantBuffer
{
public:
    void Initialize(DirectXCommon* dxCommon);
    void Update(const BloomParam& param);
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const;

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    BloomParam* mappedData_ = nullptr;
};
