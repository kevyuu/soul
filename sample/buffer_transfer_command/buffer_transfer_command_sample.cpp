#include "core/type.h"
#include "math/math.h"
#include "gpu/gpu.h"

#include "shaders/transform.hlsl"

#include <app.h>

using namespace soul;


class BufferTransferCommandSample final : public App
{

	static constexpr soul_size ROW_COUNT = 2;
	static constexpr soul_size COL_COUNT = 2;
	static constexpr soul_size TRANSFORM_COUNT = ROW_COUNT * COL_COUNT;

	struct Vertex
	{
		vec2f position = {};
		vec3f color = {};
	};

	static constexpr Vertex VERTICES[4] = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	using Index = uint16;
	static constexpr Index INDICES[] = {
		0, 1, 2, 2, 3, 0
	};

	gpu::ProgramID program_id_ = gpu::ProgramID();
	gpu::BufferID vertex_buffer_id_ = gpu::BufferID();
	gpu::BufferID index_buffer_id_ = gpu::BufferID();
	gpu::BufferID transform_q1_buffer_id_ = gpu::BufferID();
	gpu::BufferID transform_q2_buffer_id_ = gpu::BufferID();

	soul::Vector<Transform> transforms_q1_;
	soul::Vector<Transform> transforms_q2_;
	soul::Vector<Transform> transient_transforms_;

	static void fill_transform_vector(soul::Vector<Transform>& transforms_vector, 
		float x_start, float y_start, float x_end, float y_end, uint32 row_count, uint32 col_count)
	{
		for (uint32 col_idx = 0; col_idx < col_count; col_idx++)
		{
		    for (uint32 row_idx = 0; row_idx < row_count; row_idx++)
		    {
				const auto x_offset = x_start + ((x_end - x_start) / static_cast<float>(col_count)) * (static_cast<float>(col_idx) + 0.5f);
				const auto y_offset = y_start + ((y_end - y_start) / static_cast<float>(row_count)) * (static_cast<float>(row_idx) + 0.5f);
				Transform transform = {
					.color = {1.0f, 0.0f, 0.0f},
					.scale = math::scale(mat4f::identity(), vec3f(0.25f, 0.25, 1.0f)),
					.translation = math::translate(mat4f::identity(), vec3f(x_offset, y_offset, 0.0f)),
					.rotation = math::rotate(mat4f::identity(), math::radians(45.0f), vec3f(0.0f, 0.0f, 1.0f))
				};
		        transforms_vector.push_back(transform);
		    }
		}
	}

	static mat4f get_rotation(const float elapsed_seconds)
	{
	    return math::rotate(mat4f::identity(), math::radians(elapsed_seconds * 10.0f), vec3f(0.0f, 0.0f, 1.0f));
	}

	void render(gpu::RenderGraph& render_graph) override
	{
		const gpu::TextureID swapchain_texture_id = gpu_system_->get_swapchain_texture();
		const gpu::TextureNodeID render_target = render_graph.import_texture("Color Output", swapchain_texture_id);
		const gpu::ColorAttachmentDesc color_attachment_desc = {
			.node_id = render_target,
			.clear = true
		};

		const vec2ui32 viewport = gpu_system_->get_swapchain_extent();
		
		const auto elapsed_seconds_float = get_elapsed_seconds();

		const auto transform_buffer_q1 = render_graph.import_buffer("Transform Buffer Q1", transform_q1_buffer_id_);
		const auto transform_buffer_q2 = render_graph.import_buffer("Transform Buffer Q2", transform_q2_buffer_id_);


		const auto transient_transform_buffer = render_graph.create_buffer("Transient Transform Buffer", {
			.size = transient_transforms_.size() * sizeof(Transform)
		});

		struct UpdatePassParameter
		{
			gpu::BufferNodeID transform_buffer_q1;
			gpu::BufferNodeID transform_buffer_q2;
			gpu::BufferNodeID transient_transform_buffer;
		};
		UpdatePassParameter update_pass_parameter = render_graph.add_transfer_pass<UpdatePassParameter>("Update Transform Pass",
			[=](gpu::RGTransferPassDependencyBuilder& builder, UpdatePassParameter& parameter)
			{
				parameter.transform_buffer_q1 = builder.add_dst_buffer(transform_buffer_q1, gpu::TransferDataSource::CPU);
				parameter.transform_buffer_q2 = builder.add_dst_buffer(transform_buffer_q2, gpu::TransferDataSource::CPU);
				parameter.transient_transform_buffer = builder.add_dst_buffer(transient_transform_buffer, gpu::TransferDataSource::CPU);
			},
			[this, elapsed_seconds_float](const UpdatePassParameter& parameter, const gpu::RenderGraphRegistry& registry, gpu::TransferCommandList& command_list)
			{
				{
					Transform transform = transforms_q1_.back();
					transform.rotation = get_rotation(elapsed_seconds_float);
					const gpu::BufferRegionCopy region_copy = {
						.dst_offset = (transforms_q1_.size() - 1) * sizeof(Transform),
						.size = sizeof(Transform)
					};

					const gpu::RenderCommandUpdateBuffer command = {
						.dst_buffer = registry.get_buffer(parameter.transform_buffer_q1),
						.data = &transform,
						.region_count = 1,
						.regions = &region_copy
					};
					command_list.push(command);
				}

				{
					Transform transform = transforms_q2_.back();
					transform.rotation = get_rotation(elapsed_seconds_float);
					const gpu::BufferRegionCopy region_copy = {
						.dst_offset = (transforms_q2_.size() - 1) * sizeof(Transform),
						.size = sizeof(Transform)
					};

					const gpu::RenderCommandUpdateBuffer command = {
						.dst_buffer = registry.get_buffer(parameter.transform_buffer_q2),
						.data = &transform,
						.region_count = 1,
						.regions = &region_copy
					};
					command_list.push(command);
				}

				{
					Transform& transform = transient_transforms_.back();
					transform.rotation = get_rotation(elapsed_seconds_float);
					const gpu::BufferRegionCopy region_copy = {
						.size = transient_transforms_.size() * sizeof(Transform)
					};

					const gpu::RenderCommandUpdateBuffer command = {
						.dst_buffer = registry.get_buffer(parameter.transient_transform_buffer),
						.data = transient_transforms_.data(),
						.region_count = 1,
						.regions = &region_copy
					};
					command_list.push(command);
				}
				
		}).get_parameter();

		struct CopyPassParameter
		{
			gpu::BufferNodeID transform_buffer_q1;
			gpu::BufferNodeID transform_buffer_q2;
			gpu::BufferNodeID transient_transform_buffer;
			gpu::BufferNodeID copy_dst_transform_buffer;
		};
		const auto copy_transform_buffer = render_graph.create_buffer("Copy Transform Buffer", {
			.size = (transforms_q1_.size() + transforms_q2_.size() + transient_transforms_.size()) * sizeof(Transform)
		});
		CopyPassParameter copy_pass_parameter = render_graph.add_transfer_pass<CopyPassParameter>("Copy Transform Buffer",
			[=](gpu::RGTransferPassDependencyBuilder& dependency_builder, CopyPassParameter& parameter)
			{
				parameter.transform_buffer_q1 = dependency_builder.add_src_buffer(update_pass_parameter.transform_buffer_q1);
				parameter.transform_buffer_q2 = dependency_builder.add_src_buffer(update_pass_parameter.transform_buffer_q2);
				parameter.transient_transform_buffer = dependency_builder.add_src_buffer(update_pass_parameter.transient_transform_buffer);
				parameter.copy_dst_transform_buffer = dependency_builder.add_dst_buffer(copy_transform_buffer, gpu::TransferDataSource::GPU);
			},
			[this](const CopyPassParameter& parameter, const gpu::RenderGraphRegistry& registry, gpu::TransferCommandList& command_list)
			{
				using Command = gpu::RenderCommandCopyBuffer;
				const gpu::BufferRegionCopy region_copy_q1 = {
					.size = transforms_q1_.size() * sizeof(Transform)
				};
				command_list.push<Command>({
					.src_buffer = registry.get_buffer(parameter.transform_buffer_q1),
					.dst_buffer = registry.get_buffer(parameter.copy_dst_transform_buffer),
					.region_count = 1,
					.regions = &region_copy_q1
				});
				const gpu::BufferRegionCopy region_copy_q2 = {
					.dst_offset = transforms_q1_.size() * sizeof(Transform),
					.size = transforms_q2_.size() * sizeof(Transform)
				};
				command_list.push<Command>({
					.src_buffer = registry.get_buffer(parameter.transform_buffer_q2),
					.dst_buffer = registry.get_buffer(parameter.copy_dst_transform_buffer),
					.region_count = 1,
					.regions = &region_copy_q2
				});
				const gpu::BufferRegionCopy region_copy_transient = {
					.dst_offset = (transforms_q1_.size() + transforms_q2_.size()) * sizeof(Transform),
					.size = transient_transforms_.size() * sizeof(Transform)
				};
				command_list.push<Command>({
					.src_buffer = registry.get_buffer(parameter.transient_transform_buffer),
					.dst_buffer = registry.get_buffer(parameter.copy_dst_transform_buffer),
					.region_count = 1,
					.regions = &region_copy_transient
				});
			}
		).get_parameter();
		

		struct RenderPassParameter
		{
			gpu::BufferNodeID transform_buffer;
		};
		render_graph.add_graphic_pass<RenderPassParameter>("Render Pass",
			gpu::RGRenderTargetDesc(
				viewport,
				color_attachment_desc
			)
			, [copy_pass_parameter](gpu::RGShaderPassDependencyBuilder& builder, RenderPassParameter& parameter)
			{
				parameter.transform_buffer = builder.add_shader_buffer(copy_pass_parameter.copy_dst_transform_buffer,
					{ gpu::ShaderStage::VERTEX }, gpu::ShaderBufferReadUsage::STORAGE);
			}, [viewport, this](const RenderPassParameter& parameter, gpu::RenderGraphRegistry& registry, gpu::GraphicCommandList& command_list)
			{

				const gpu::GraphicPipelineStateDesc pipeline_desc = {
					.program_id = program_id_,
					.input_bindings = {
						{.stride = sizeof(Vertex)}
					},
					.input_attributes = {
						{.binding = 0, .offset = offsetof(Vertex, position), .type = gpu::VertexElementType::FLOAT2},
						{.binding = 0, .offset = offsetof(Vertex, color), .type = gpu::VertexElementType::FLOAT3}
					},
					.viewport = {
						.width = static_cast<float>(viewport.x),
						.height = static_cast<float>(viewport.y)
					},
					.scissor = {
						.extent = viewport
					},
					.color_attachment_count = 1
				};

				struct PushConstant
				{
					gpu::DescriptorID transform_descriptor_id = gpu::DescriptorID::null();
					uint32 offset = 0;
				};

				using Command = gpu::RenderCommandDrawIndex;

				auto pipeline_state_id = registry.get_pipeline_state(pipeline_desc);
				soul::Vector<PushConstant> push_constants(transforms_q1_.size() + transforms_q2_.size() + transient_transforms_.size());
				const auto transform_buffer = registry.get_buffer(parameter.transform_buffer);
				const auto transform_buffer_descriptor_id = gpu_system_->get_ssbo_descriptor_id(transform_buffer);
				
			    for (soul_size push_constant_idx = 0; push_constant_idx < push_constants.size(); push_constant_idx++)
				{
					push_constants[push_constant_idx] = {
						.transform_descriptor_id = transform_buffer_descriptor_id,
						.offset = soul::cast<uint32>(push_constant_idx * sizeof(Transform))
					};
				}

				command_list.push<Command>(push_constants.size(), [=, this, &push_constants](const soul_size index) -> Command
				{
					return {
						.pipeline_state_id = pipeline_state_id,
						.push_constant_data = &push_constants[index],
						.push_constant_size = sizeof(PushConstant),
						.vertex_buffer_ids = {
							vertex_buffer_id_
						},
						.index_buffer_id = index_buffer_id_,
						.first_index = 0,
						.index_count = std::size(INDICES)
					};
				});

			});
	}

public:
	explicit BufferTransferCommandSample(const AppConfig& app_config) : App(app_config)
	{
		gpu::ShaderSource shader_source = gpu::ShaderFile("buffer_transfer_command_sample.hlsl");
		std::filesystem::path search_path = "shaders/";
		const gpu::ProgramDesc program_desc = {
			.search_paths = &search_path,
			.search_path_count = 1,
			.sources = &shader_source,
			.source_count = 1,
			.entry_point_names = gpu::EntryPoints({
				{gpu::ShaderStage::VERTEX, "vsMain"},
				{gpu::ShaderStage::FRAGMENT, "psMain"}
			})
		};
		auto result = gpu_system_->create_program_dxc(program_desc);
		if (!result)
		{
			SOUL_PANIC("Fail to create program");
		}
		program_id_ = result.value();

		vertex_buffer_id_ = gpu_system_->create_buffer({
			.size = sizeof(Vertex) * std::size(VERTICES),
			.usage_flags = { gpu::BufferUsage::VERTEX },
			.queue_flags = { gpu::QueueType::GRAPHIC }
			}, VERTICES);
		gpu_system_->flush_buffer(vertex_buffer_id_);

		index_buffer_id_ = gpu_system_->create_buffer({
			.size = sizeof(Index) * std::size(INDICES),
			.usage_flags = { gpu::BufferUsage::INDEX },
			.queue_flags = { gpu::QueueType::GRAPHIC }
			}, INDICES);
		gpu_system_->flush_buffer(index_buffer_id_);
		
		fill_transform_vector(transforms_q1_, -1.0f, -1.0f, 0.0f, 0.0f, ROW_COUNT, COL_COUNT);
		transform_q1_buffer_id_ = gpu_system_->create_buffer({
			.size = TRANSFORM_COUNT * sizeof(Transform),
			.usage_flags = {gpu::BufferUsage::STORAGE, gpu::BufferUsage::TRANSFER_SRC},
			.queue_flags = {gpu::QueueType::GRAPHIC, gpu::QueueType::TRANSFER}
		}, transforms_q1_.data());


		fill_transform_vector(transforms_q2_, 0.0f, -1.0f, 1.0f, 0.0f, ROW_COUNT, COL_COUNT);
		transform_q2_buffer_id_ = gpu_system_->create_buffer({
			.size = TRANSFORM_COUNT * sizeof(Transform),
			.usage_flags = {gpu::BufferUsage::STORAGE, gpu::BufferUsage::TRANSFER_SRC},
			.queue_flags = {gpu::QueueType::GRAPHIC, gpu::QueueType::TRANSFER},
			.memory_option = gpu::MemoryOption {
				.required = { gpu::MemoryProperty::HOST_COHERENT },
				.preferred = {gpu::MemoryProperty::DEVICE_LOCAL }
			}
		}, transforms_q2_.data());

		fill_transform_vector(transient_transforms_, -1.0f, 0.0f, 1.0f, 1.0f, ROW_COUNT, COL_COUNT * 2);
	}
};

int main(int argc, char* argv[])
{
	const ScreenDimension screen_dimension = { .width = 800, .height = 600 };
	BufferTransferCommandSample app({ .screen_dimension = screen_dimension });
	app.run();

	return 0;
}