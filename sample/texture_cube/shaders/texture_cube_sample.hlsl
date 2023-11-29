struct VSInput {
	[[vk::location(0)]] float3 position: POSITION;
};


[[vk::push_constant]]
struct push_constant {
	soulsl::float4x4 projection;
	soulsl::float4x4 view;
	soulsl::DescriptorID texture_descriptor_id;
	soulsl::DescriptorID sampler_descriptor_id;
	float alignment1, alignment2;
} push_constant;

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 tex_coord: TEXCOORD;
};

[shader("vertex")]
VSOutput vs_main(VSInput input)
{
	VSOutput output;
	output.tex_coord = input.position;
	// Convert cubemap coordinates into Vulkan coordinate space
	output.tex_coord.xy *= -1.0;
	output.position = mul(push_constant.projection, mul(push_constant.view, float4(input.position.xyz, 1.0)));
	return output;
}

[shader("pixel")]
float4 ps_main(VSOutput output) : SV_TARGET
{
	TextureCube cube_tex = get_texture_cube(push_constant.texture_descriptor_id);
    SamplerState cube_sampler = get_sampler(push_constant.sampler_descriptor_id);
    return cube_tex.Sample(cube_sampler, output.tex_coord);
}
