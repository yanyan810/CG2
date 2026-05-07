Texture2D sourceTex : register(t0);
SamplerState samp : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    uint width, height;
    sourceTex.GetDimensions(width, height);
    float2 texel = 1.0f / float2(width, height);
    float2 uv = input.uv;

    // 中心
    float4 c = sourceTex.Sample(samp, uv);

    // 周囲の4点（ボックス配置）
    float4 a = sourceTex.Sample(samp, uv + float2(-2,  2) * texel);
    float4 b = sourceTex.Sample(samp, uv + float2( 2,  2) * texel);
    float4 d = sourceTex.Sample(samp, uv + float2(-2, -2) * texel);
    float4 e = sourceTex.Sample(samp, uv + float2( 2, -2) * texel);

    // 少し内側の4点（ひし形配置）
    float4 f = sourceTex.Sample(samp, uv + float2( 0,  1) * texel);
    float4 g = sourceTex.Sample(samp, uv + float2(-1,  0) * texel);
    float4 h = sourceTex.Sample(samp, uv + float2( 1,  0) * texel);
    float4 i = sourceTex.Sample(samp, uv + float2( 0, -1) * texel);

    // 以下の重み付けで合成（合計 0.125 * 4 + 0.0625 * 4 + 0.03125 * 4 = 1.0）
    // 重なる部分を考慮してブレンドする有名な係数セットです
    float4 color = 0.0f;
    color += (f + g + h + i) * 0.125f;
    color += (a + b + d + e) * 0.03125f;
    color += (f + g + h + i) * 0.0625f; // 重複させてウェイトを調整
    color += c * 0.125f;
    
    return color;
}
