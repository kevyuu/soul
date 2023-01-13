#include "transform.hlsl"

struct VSInput {
	[[vk::location(0)]] float2 position: POSITION;
	[[vk::location(1)]] float3 color: COLOR;
};

[[vk::push_constant]]
struct push_constant {
	soulsl::DescriptorID transform_descriptor_id;
	uint offset;
} push_constant;

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 color: COLOR0;
};

[shader("vertex")]
VSOutput vsMain(VSInput input)
{
	VSOutput output;
	Transform transform = get_buffer<Transform>(push_constant.transform_descriptor_id, push_constant.offset);
	output.position = mul(transform.translation, mul(transform.rotation, mul(transform.scale, float4(input.position, 0.0, 1.0))));
	output.color = transform.color + input.color;
    return output;
}

struct PSOutput
{
    [[vk::location(0)]] float4 color: SV_Target;
};

[shader("pixel")]
PSOutput psMain(VSOutput input)
{
    PSOutput output;
    output.color = float4(input.color, 1.0);
    return output;
}