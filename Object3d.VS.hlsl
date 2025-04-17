float4 main( float4 pos : POSITION ) : SV_POSITION
{
	return pos;
}

struct VertexShaderOutput{
    float4 position : SV_POSITION;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = input.position;
    return output;
}

struct PixelSharderOutput{
    float4 color : SV_TARGET0;
};

PixelSharderOutput main(VertexShaderOutput input)
{
    PixelSharderOutput output;
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f); // Red color
    return output;
}