#include "Object3d.hlsli"
struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
};

struct PixelSharderOutput
{
    float4 color : SV_TARGET0;
};

struct DirectionalLight
{
    
    float4 color;
    float3 direction;
    float intensity;
   
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

PixelSharderOutput main(VertexShaderOutput input)
{
    PixelSharderOutput output;
    output.color = gMaterial.color; // ← ここはそのままでOK
    float4 transformedUV = mul(float4(input.texcoord, 0.0f,1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    if (gMaterial.enableLighting != 0)
    {
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;

      //  output.color = float4(normalize(input.normal) * 0.5 + 0.5, 1.0f);
        
      //  output.color = float4((input.normal * 0.5f + 0.5f), 1.0f); // 可視化
        
        //output.color = gMaterial.color * gDirectionalLight.color * (cos * 0.8f + 0.2f);
        
        //float3 normal = normalize(input.normal);
        //output.color= float4(normal * 0.5f + 0.5f, 1.0f); // 色で法線可視化
        
        //float3 n = normalize(input.normal);
        //output.color= float4(abs(n), 1.0f); // 絶対値をとると全方向が見える
        
        //float3 L = normalize(-gDirectionalLight.direction);
        //output.color= float4(L * 0.5f + 0.5f, 1.0f); // ライトベクトルを色に変換
        
    }
    else
    {
        output.color *= gMaterial.color * textureColor;
       
    }
    
    //float3 normal = normalize(input.normal);
    //output.color = float4(normal * 0.5f + 0.5f, 1.0f); // RGB確認
      // 法線の可視化（R=右, G=上, B=前）-1~1 → 0~1 に補正
    //float3 normal = normalize(input.normal);
    //output.color = float4(normal * 0.5f + 0.5f, 1.0f);

    
        return output;
}



