struct RayTracingPushConstant
{
	soulsl::DescriptorID scene_descriptor_id;
	soulsl::DescriptorID as_descriptor_id;
	soulsl::DescriptorID image_descriptor_id;
	soulsl::DescriptorID sampler_descriptor_id;
};
