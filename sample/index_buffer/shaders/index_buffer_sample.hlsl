struct VSInput {
	[[vk::location(0)]] float2 position: POSITION;
	[[vk::location(1)]] float3 color: COLOR;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 color: COLOR0;
};

[shader("vertex")]
VSOutput vsMain(VSInput input)
{
	VSOutput output;
	output.position = float4(input.position, 0.0, 1.0);
	output.color = input.color;
    return output;
}

struct PSOutput
{
    [[vk::location(0)]] float4 color : SV_TARGET;
};

[shader("pixel")]
PSOutput fsMain(VSOutput input)
{
    PSOutput output;
    output.color = float4(input.color, 1.0);
    return output;
}