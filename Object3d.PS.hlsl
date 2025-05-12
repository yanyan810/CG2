#include "Object3d.hlsli"
cbuffer gMaterial : register(b0)
{
    float4 color;
};

struct PixelSharderOutput
{
    float4 color : SV_TARGET0;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

PixelSharderOutput main(VertexShaderOutput input)
{
    PixelSharderOutput output;
    output.color = color; // ← ここはそのままでOK
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color *= color*textureColor; 
    return output;
}



