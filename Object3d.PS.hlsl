#include "Object3d.hlsli"


struct PixelSharderOutput
{
    float4 color : SV_TARGET0;
};


struct Material
{
    float4 color;
    int enableLighting;
};

//平行光源
struct DirectionalLight
{
    float3 direction; // 光の方向
    float4 color; // 光の色
    float intensity; // 光の強度
};


ConstantBuffer<Material> gMaterial : register(b0);
Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

PixelSharderOutput main(VertexShaderOutput input)
{
    PixelSharderOutput output;
    output.color = gMaterial.color; // ← ここはそのままでOK
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    if (gMaterial.enableLighting != 0)
    {
        float cos = saturate(dot(normalize(input.normal), -gDirectionalLight.direction));
        output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;

    }
    else
    {
    
        output.color *= gMaterial.color * textureColor;
    
    }
        return output;
}



