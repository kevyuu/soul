struct VSInput {
	[[vk::location(0)]] float2 position: POSITION;
	[[vk::location(1)]] float2 tex_coord: TEXCOORD;
};

[[vk::push_constant]]
struct push_constant {
	soulsl::DescriptorID texture_descriptor_id;
	soulsl::DescriptorID sampler_descriptor_id;
	float depth;
} push_constant;

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 tex_coord: TEXCOORD;
};

[shader("vertex")]
VSOutput vsMain(VSInput input)
{
	VSOutput output;
	output.position = float4(input.position, 0.0, 1.0);
	output.tex_coord = input.tex_coord;
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
	Texture3D test_texture = get_texture_3d(push_constant.texture_descriptor_id);
	SamplerState test_sampler = get_sampler(push_constant.sampler_descriptor_id);
	float color = test_texture.Sample(test_sampler, float3(input.tex_coord, push_constant.depth));
	output.color = float4(color, color, color, 1.0);
    return output;
}