#include "Object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;

    // ★ int の後に padding（16byte揃え）
    float3 _pad0;

    float4x4 uvTransform;

    // ★ 鏡面反射用
    float shininess;

    // ★ 16byte揃え
    float3 _pad1;
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

struct Camera
{
    float3 worldPosition;
    float _pad;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);

PixelSharderOutput main(VertexShaderOutput input)
{
    PixelSharderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    if (textureColor.a == 0.0f)
        discard;

    // ベース色
    output.color = gMaterial.color * textureColor;

    if (gMaterial.enableLighting != 0)
    {
        float3 N = normalize(input.normal);
        float3 L = normalize(-gDirectionalLight.direction); // 光が当たる方向
        float3 V = normalize(gCamera.worldPosition - input.worldPosition); // ★toEye
        float3 R = reflect(-L, N); // ★反射

        float NdotL = dot(N, L);

        float lighting = 1.0f;
        if (gMaterial.enableLighting == 1)
        {
            lighting = saturate(NdotL); // Lambert
        }
        else if (gMaterial.enableLighting == 2)
        {
            lighting = pow(NdotL * 0.5f + 0.5f, 2.0f); // Half-Lambert
        }

        // diffuse
        float3 diffuse =
            gMaterial.color.rgb *
            textureColor.rgb *
            gDirectionalLight.color.rgb *
            lighting *
            gDirectionalLight.intensity;

        // ★ specular（ここで作る！）
        float specPow = pow(saturate(dot(R, V)), max(gMaterial.shininess, 1.0f));
        float3 specular =
            gDirectionalLight.color.rgb *
            gDirectionalLight.intensity *
            specPow;

        // ★ SpecOnly モード（確認用）
        if (gMaterial.enableLighting == 3)
        {
            output.color.rgb = specular;
            output.color.a = 1.0f;
            return output;
        }

        // 通常：diffuse + specular
        output.color.rgb = diffuse + specular;
        output.color.a = gMaterial.color.a * textureColor.a;
    }

    return output;
}
