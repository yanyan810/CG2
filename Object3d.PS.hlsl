struct PixelSharderOutput
{
    float4 color : SV_TARGET0;
};

PixelSharderOutput main()
{
    PixelSharderOutput output;
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f); // Red color
    return output;
}