cbuffer gMaterial : register(b0)
{
    float4 color;
};

struct PixelSharderOutput
{
    float4 color : SV_TARGET0;
};

PixelSharderOutput main()
{
    PixelSharderOutput output;
    output.color = color; // ← ここはそのままでOK
    return output;
}
