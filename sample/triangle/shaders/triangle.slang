
static const float2 positions[3] = {
    float2(0.0, -0.5),
    float2(0.5, 0.5),
    float2(-0.5, 0.5)
};

static const float3 colors[3] = {
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
};


struct VSOutput
{
	float4 position : SV_POSITION;
	float3 color: COLOR0;
};

[shader("vertex")]
VSOutput vsMain(uint vertex_id : SV_VERTEXID)
{
	VSOutput output;
	output.position = float4(positions[vertex_id], 0.0, 1.0);
	output.color = colors[vertex_id];
    return output;
}

struct PSOutput
{
    [[vk::location(0)]] float4 color;
};

[shader("fragment")]
PSOutput fsMain(VSOutput input)
{
    PSOutput output;
    output.color = float4(input.color, 1.0);
    return output;
}