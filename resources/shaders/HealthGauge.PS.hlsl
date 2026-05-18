#include "Sprite2d.hlsli"

struct HealthGaugeParam
{
    float4 hpColor;
    float4 damageColor;
    float4 shieldColor;
    float4 bgColor;
    float4 borderColor;
    float4 shadowColor;
    float hpRatio;
    float damageStartRatio;
    float damageEndRatio;
    float shieldStartRatio;
    float shieldEndRatio;
    float skew;
    float borderWidth;
    float blink;
    float glow;
    float alpha;
    float2 pad;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

ConstantBuffer<HealthGaugeParam> gGauge : register(b0);

float InRange(float x, float start, float end)
{
    return step(start, x) * step(x, end);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;
    float skew = gGauge.skew;
    float leftEdge = lerp(skew, 0.0f, uv.y);
    float rightEdge = lerp(1.0f, 1.0f - skew, uv.y);

    float inside = step(leftEdge, uv.x) * step(uv.x, rightEdge);
    if (inside <= 0.0f)
    {
        discard;
    }

    float width = max(rightEdge - leftEdge, 0.0001f);
    float localX = saturate((uv.x - leftEdge) / width);
    float border = saturate(gGauge.borderWidth);
    float innerY = step(border, uv.y) * step(uv.y, 1.0f - border);
    float innerX = step(border, localX) * step(localX, 1.0f - border);
    float inner = innerX * innerY;

    float fillX = saturate((localX - border) / max(1.0f - border * 2.0f, 0.0001f));
    float verticalShade = 0.90f + 0.10f * smoothstep(0.15f, 0.95f, uv.y);

    float hpMask = inner * step(fillX, saturate(gGauge.hpRatio));
    float damageMask = inner * InRange(fillX, saturate(gGauge.damageStartRatio), saturate(gGauge.damageEndRatio));
    float shieldMask = inner * InRange(fillX, saturate(gGauge.shieldStartRatio), saturate(gGauge.shieldEndRatio));

    float4 damageBlink = lerp(gGauge.hpColor, gGauge.damageColor, gGauge.blink);
    float4 color = gGauge.shadowColor;
    color = lerp(color, gGauge.borderColor, 1.0f);
    color = lerp(color, gGauge.bgColor, inner);
    color = lerp(color, gGauge.hpColor * verticalShade, hpMask);
    color = lerp(color, damageBlink, damageMask);
    color = lerp(color, gGauge.shieldColor, shieldMask);

    float edgeLight = smoothstep(0.0f, 0.08f, uv.y) * (1.0f - smoothstep(0.08f, 0.18f, uv.y));
    color.rgb += gGauge.borderColor.rgb * edgeLight * 0.08f;
    color.rgb += color.rgb * gGauge.glow;
    color.a *= gGauge.alpha;

    output.color = color;
    return output;
}
