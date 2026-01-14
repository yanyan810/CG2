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

    // Alpha 0 は捨てる（※抜けが気になるなら一旦コメントアウトしてOK）
    if (textureColor.a == 0.0f)
        discard;

    // ベース色
    output.color = gMaterial.color * textureColor;

    if (gMaterial.enableLighting != 0)
    {
        float3 N = normalize(input.normal);

        // あなたの設計：direction は「光が進む向き」想定なので - を付けて L を作る
        float3 L = normalize(-gDirectionalLight.direction); // 点→光
        float3 V = normalize(gCamera.worldPosition - input.worldPosition); // 点→カメラ

        float NdotL = dot(N, L);

        // 拡散（Lambert / HalfLambert）
        float lighting = 1.0f;
        if (gMaterial.enableLighting == 1)
        {
            lighting = saturate(NdotL); // Lambert
        }
        else if (gMaterial.enableLighting == 2)
        {
            lighting = pow(NdotL * 0.5f + 0.5f, 2.0f); // Half-Lambert
        }

        float3 diffuse =
            gMaterial.color.rgb *
            textureColor.rgb *
            gDirectionalLight.color.rgb *
            lighting *
            gDirectionalLight.intensity;

        // ---------------------------
        // ★ 鏡面反射：Phong（reflect）版
        // ---------------------------
        float3 R = reflect(-L, N); // 入射（光が当たる向き）は -L
        float phongSpecPow = pow(saturate(dot(R, V)), max(gMaterial.shininess, 1.0f));
        float3 phongSpecular =
            gDirectionalLight.color.rgb *
            gDirectionalLight.intensity *
            phongSpecPow;

        // ---------------------------
        // ★ 鏡面反射：Blinn-Phong（HalfVector）版（画像の計算）
        // halfVector = normalize(L + V)
        // ---------------------------
        float3 H = normalize(L + V);
        float NdotH = dot(N, H);
        float blinnSpecPow = pow(saturate(NdotH), max(gMaterial.shininess, 1.0f));
        float3 blinnSpecular =
            gDirectionalLight.color.rgb *
            gDirectionalLight.intensity *
            blinnSpecPow;

        // ★ 確認用モード
        // 3: Phong鏡面のみ
        if (gMaterial.enableLighting == 3)
        {
            output.color.rgb = phongSpecular;
            output.color.a = 1.0f;
            return output;
        }

        // 4: Blinn鏡面のみ（追加）
        if (gMaterial.enableLighting == 4)
        {
            output.color.rgb = blinnSpecular;
            output.color.a = 1.0f;
            return output;
        }

        // 通常：diffuse + specular
        // ★ どっちを足すかは好み：まずは Blinn の方が安定なので Blinn を採用してみる
        // output.color.rgb = diffuse + phongSpecular;
        output.color.rgb = diffuse + blinnSpecular;

        output.color.a = gMaterial.color.a * textureColor.a;
    }

    return output;
}
