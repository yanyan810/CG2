#include "Object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;

    float3 _pad0;

    float4x4 uvTransform;

    float shininess;
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

struct PointLight
{
    
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
    float2 _pad;
    
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);

PixelSharderOutput main(VertexShaderOutput input)
{
    PixelSharderOutput output;

    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // Alpha 0 は捨てる（※抜けが気になるなら一旦コメントアウトしてOK）
    if (textureColor.a == 0.0f)
        discard;

    // ベース色
    output.color = gMaterial.color * textureColor;

    if (gMaterial.enableLighting != 0)
    {
        float3 N = normalize(input.normal);

    // ===== View =====
        float3 V = normalize(gCamera.worldPosition - input.worldPosition); // 点→カメラ

    // ===== Directional Light =====
        float3 Ld = normalize(-gDirectionalLight.direction); // 点→光（Directional）
        float NdotLd = dot(N, Ld);

        float lightingD = 1.0f;
        if (gMaterial.enableLighting == 1)
        {
            lightingD = saturate(NdotLd);
        }
        else if (gMaterial.enableLighting == 2)
        {
            lightingD = pow(NdotLd * 0.5f + 0.5f, 2.0f);
        }
        else
        {
            lightingD = saturate(NdotLd);
        }

        float3 diffuseD =
        gMaterial.color.rgb *
        textureColor.rgb *
        gDirectionalLight.color.rgb *
        lightingD *
        gDirectionalLight.intensity;

        float3 Hd = normalize(Ld + V);
        float specD = pow(saturate(dot(N, Hd)), max(gMaterial.shininess, 1.0f));
        float3 specularD =
        gDirectionalLight.color.rgb *
        gDirectionalLight.intensity *
        specD;

  // ===== Point Light =====
        float3 toLight = gPointLight.position - input.worldPosition; // 点→光源ベクトル
        float dist = length(toLight);

// 0除算防止（dist=0 を避ける）
        dist = max(dist, 0.001f);

// ★改良版減衰（半径radius以内で 0..1、外は0。decayで落ち方を調整）
        float t = saturate(1.0f - dist / max(gPointLight.radius, 0.001f));
        float factor = pow(t, gPointLight.decay);

// L は「点→光」なので normalize(toLight)
        float3 Lp = toLight / dist; // normalize(toLight) と同じ（dist計算済みなので割り算でOK）
        float NdotLp = dot(N, Lp);

        float lightingP = 1.0f;
        if (gMaterial.enableLighting == 1)
        {
            lightingP = saturate(NdotLp);
        }
        else if (gMaterial.enableLighting == 2)
        {
            lightingP = pow(NdotLp * 0.5f + 0.5f, 2.0f);
        }
        else
        {
            lightingP = saturate(NdotLp);
        }

// ★ここがポイント：点光源の色（強さ）に factor を掛ける
        float3 pointColor = gPointLight.color.rgb * gPointLight.intensity * factor;

        float3 diffuseP =
    gMaterial.color.rgb *
    textureColor.rgb *
    pointColor *
    lightingP;

        float3 Hp = normalize(Lp + V); // Blinn-Phong
        float specP = pow(saturate(dot(N, Hp)), max(gMaterial.shininess, 1.0f));
        float3 specularP = pointColor * specP;


        // ===== 合成前に確認モード =====
        if (gMaterial.enableLighting == 12)
        {
            output.color.rgb = diffuseP + specularP; // Pointだけ
            output.color.a = 1.0f;
            return output;
        }
        if (gMaterial.enableLighting == 11)
        {
            output.color.rgb = diffuseD + specularD; // Directionalだけ
            output.color.a = 1.0f;
            return output;
        }


    // ===== 全部足す（スライドの式）=====
        output.color.rgb = diffuseD + specularD + diffuseP + specularP;
        output.color.a = gMaterial.color.a * textureColor.a;
    }


    return output;
}
