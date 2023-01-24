#include "msaa_type.hlsl"

struct VSInput {
	[[vk::location(0)]] float2 position: POSITION;
};

[[vk::push_constant]]
MSAAPushConstant push_constant;

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 color: COLOR0;
};

[shader("vertex")]
VSOutput vs_main(VSInput input)
{
	VSOutput output;
	output.position = mul(push_constant.transform, float4(input.position, 0, 1));
	output.color = push_constant.color;
	return output;
}

struct PSOutput
{
	[[vk::location(0)]] float4 color: SV_Target;
};

[shader("pixel")]
PSOutput ps_main(VSOutput input)
{
	PSOutput output;
	output.color = float4(input.color, 1.0);
	return output;
}