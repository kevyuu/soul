#include "core/type.h"
#include "gpu/gpu.h"
#include "math/math.h"

#include "runtime/scope_allocator.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <app.h>
#include <imgui/imgui.h>

#include <random>

using namespace soul;

// Translation of Ken Perlin's JAVA implementation (http://mrl.nyu.edu/~perlin/noise/)
template <typename T>
class PerlinNoise
{
private:
  uint32_t permutations_[512] = {};

  static auto fade(T t) -> T
  {
    return t * t * t * (t * (t * static_cast<T>(6) - static_cast<T>(15)) + static_cast<T>(10));
  }

  static auto lerp(T t, T a, T b) -> T { return a + t * (b - a); }

  [[nodiscard]]
  auto grad(const int hash, T x, T y, T z) const -> T
  {
    // Convert LO 4 bits of hash code into 12 gradient directions
    const auto h = hash & 15;
    T u = h < 8 ? x : y;
    T v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
  }

public:
  PerlinNoise()
  {
    // Generate random lookup for permutations containing all numbers from 0..255
    std::vector<uint8_t> plookup;
    plookup.resize(256);
    std::iota(plookup.begin(), plookup.end(), 0);
    std::default_random_engine rnd_engine(std::random_device{}());
    std::ranges::shuffle(plookup, rnd_engine);

    for (uint32_t i = 0; i < 256; i++) {
      permutations_[i] = permutations_[256 + i] = plookup[i];
    }
  }

  [[nodiscard]]
  auto noise(T x, T y, T z) const -> T
  {
    // Find unit cube that contains point
    const auto X = static_cast<int32_t>(floor(x)) & 255;
    const auto Y = static_cast<int32_t>(floor(y)) & 255;
    const auto Z = static_cast<int32_t>(floor(z)) & 255;
    // Find relative x,y,z of point in cube
    x -= floor(x);
    y -= floor(y);
    z -= floor(z);

    // Compute fade curves for each of x,y,z
    T u = fade(x);
    T v = fade(y);
    T w = fade(z);

    // Hash coordinates of the 8 cube corners
    const auto a = permutations_[X] + Y;
    const auto aa = permutations_[a] + Z;
    const auto ab = permutations_[a + 1] + Z;
    const auto b = permutations_[X + 1] + Y;
    const auto ba = permutations_[b] + Z;
    const auto bb = permutations_[b + 1] + Z;

    // And add blended results for 8 corners of the cube;
    T res = lerp(
      w,
      lerp(
        v,
        lerp(u, grad(permutations_[aa], x, y, z), grad(permutations_[ba], x - 1, y, z)),
        lerp(u, grad(permutations_[ab], x, y - 1, z), grad(permutations_[bb], x - 1, y - 1, z))),
      lerp(
        v,
        lerp(
          u,
          grad(permutations_[aa + 1], x, y, z - 1),
          grad(permutations_[ba + 1], x - 1, y, z - 1)),
        lerp(
          u,
          grad(permutations_[ab + 1], x, y - 1, z - 1),
          grad(permutations_[bb + 1], x - 1, y - 1, z - 1))));
    return res;
  }
};

// Fractal noise generator based on perlin noise above
template <typename T>
class FractalNoise
{
private:
  PerlinNoise<float> perlin_noise_;
  uint32_t octaves_;
  T frequency_;
  T amplitude_;
  T persistence_;

public:
  explicit FractalNoise(const PerlinNoise<T>& perlin_noise)
  {
    this->perlin_noise_ = perlin_noise;
    octaves_ = 6;
    persistence_ = static_cast<T>(0.5);
  }

  [[nodiscard]]
  auto noise(T x, T y, T z) const -> T
  {
    T sum = 0;
    T frequency = static_cast<T>(1);
    T amplitude = static_cast<T>(1);
    T max = static_cast<T>(0);
    for (uint32_t i = 0; i < octaves_; i++) {
      sum += perlin_noise_.noise(x * frequency, y * frequency, z * frequency) * amplitude;
      max += amplitude;
      amplitude *= persistence_;
      frequency *= static_cast<T>(2);
    }

    sum = sum / max;
    return (sum + static_cast<T>(1.0)) / static_cast<T>(2.0);
  }
};

class Texture3DSampleApp final : public App
{
  static constexpr float CYCLE_DURATION = 30.0f;
  static constexpr vec3ui32 DIMENSION{128, 128, 128};

  struct Vertex {
    vec2f position = {};
    vec2f texture_coords = {};
  };

  static constexpr Vertex VERTICES[4] = {
    {{-1.0f, -1.0f}, {0.0f, 1.0f}}, // top right
    {{1.0f, -1.0f}, {1.0f, 1.0f}},  // top left
    {{1.0f, 1.0f}, {1.0f, 0.0f}},   // bottom left
    {{-1.0f, 1.0f}, {0.0f, 0.0f}}   // bottom right
  };

  using Index = u16;
  static constexpr Index INDICES[] = {0, 1, 2, 2, 3, 0};

  gpu::ProgramID program_id_ = gpu::ProgramID();
  gpu::BufferID vertex_buffer_id_ = gpu::BufferID();
  gpu::BufferID index_buffer_id_ = gpu::BufferID();
  gpu::TextureID test_texture_id_ = gpu::TextureID();
  gpu::SamplerID test_sampler_id_ = gpu::SamplerID();
  float depth_ = 0.0f;

  const std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();

  static auto create_noise_data(vec3ui32 dimension, memory::Allocator& allocator) -> void*
  {
    const auto tex_mem_size = dimension.x * dimension.y * dimension.z;

    auto* data = allocator.allocate_array<u8>(tex_mem_size);
    memset(data, 0, tex_mem_size);

    const PerlinNoise<float> perlin_noise;
    const FractalNoise<float> fractal_noise(perlin_noise);

    std::default_random_engine generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(0, 9);
    auto dice = std::bind(distribution, generator);

    const auto noise_scale = static_cast<float>(dice()) + 4.0f;

    for (u32 z = 0; z < dimension.z; z++) {
      for (u32 y = 0; y < dimension.y; y++) {
        for (u32 x = 0; x < dimension.x; x++) {
          const auto nx = static_cast<float>(x) / static_cast<float>(dimension.x);
          const auto ny = static_cast<float>(y) / static_cast<float>(dimension.y);
          const auto nz = static_cast<float>(z) / static_cast<float>(dimension.z);

          auto n = fractal_noise.noise(nx * noise_scale, ny * noise_scale, nz * noise_scale);
          n = n - floor(n);

          data[x + y * dimension.x + z * dimension.x * dimension.y] =
            static_cast<uint8_t>(floor(n * 255));
        }
      }
    }

    return data;
  }

  auto render(gpu::TextureNodeID render_target, gpu::RenderGraph& render_graph)
    -> gpu::TextureNodeID override
  {
    auto update_noise_texture = false;
    if (ImGui::Begin("Options")) {
      if (ImGui::Button("Generate New Noise Texture")) {
        update_noise_texture = true;
      }
      ImGui::End();
    }

    const gpu::ColorAttachmentDesc color_attachment_desc = {
      .node_id = render_target,
      .clear = true,
    };

    const vec2ui32 viewport = gpu_system_->get_swapchain_extent();

    gpu::TextureNodeID noise_texture_node =
      render_graph.import_texture("Noise Texture", test_texture_id_);
    if (update_noise_texture) {
      struct UpdatePassParameter {
        gpu::TextureNodeID noise_texture;
      };
      const auto update_pass_parameter =
        render_graph
          .add_non_shader_pass<UpdatePassParameter>(
            "Update Noise Texture",
            gpu::QueueType::TRANSFER,
            [noise_texture_node](auto& parameter, auto& builder) {
              parameter.noise_texture =
                builder.add_dst_texture(noise_texture_node, gpu::TransferDataSource::CPU);
            },
            [](const auto& parameter, auto& registry, auto& command_list) {
              runtime::ScopeAllocator scope_allocator("Update Noise Execution");
              const auto* data = create_noise_data(DIMENSION, scope_allocator);
              const gpu::TextureRegionUpdate region_load = {
                .subresource = {.layer_count = 1},
                .extent = DIMENSION,
              };

              using Command = gpu::RenderCommandUpdateTexture;
              const Command command = {
                .dst_texture = registry.get_texture(parameter.noise_texture),
                .data = data,
                .data_size = soul::cast<usize>(DIMENSION.x) * DIMENSION.y * DIMENSION.z,
                .regions = u32cspan(&region_load, 1),
              };
              command_list.template push<Command>(command);
            })
          .get_parameter();

      noise_texture_node = update_pass_parameter.noise_texture;
    }

    struct CopyPassParameter {
      gpu::TextureNodeID src_noise_texture;
      gpu::TextureNodeID dst_noise_texture;
    };

    const auto copy_dst_texture_node = render_graph.create_texture(
      "Copy Dst Texture", gpu::RGTextureDesc::create_d3(gpu::TextureFormat::R8, 1, DIMENSION));
    const auto copy_pass_parameter =
      render_graph
        .add_non_shader_pass<CopyPassParameter>(
          "Copy Pass Parameter",
          gpu::QueueType::TRANSFER,
          [=](auto& parameter, auto& builder) {
            parameter.src_noise_texture = builder.add_src_texture(noise_texture_node);
            parameter.dst_noise_texture = builder.add_dst_texture(copy_dst_texture_node);
          },
          [](const auto& parameter, auto& registry, auto& command_list) {
            const gpu::TextureRegionCopy region_copy = {
              .src_subresource = {0, 0, 1},
              .src_offset = {},
              .dst_subresource = {0, 0, 1},
              .dst_offset = {},
              .extent = DIMENSION,
            };

            command_list.push(gpu::RenderCommandCopyTexture{
              .src_texture = registry.get_texture(parameter.src_noise_texture),
              .dst_texture = registry.get_texture(parameter.dst_noise_texture),
              .regions = u32cspan(&region_copy, 1),
            });
          })
        .get_parameter();
    noise_texture_node = copy_pass_parameter.dst_noise_texture;

    struct RenderPassParameter {
      gpu::TextureNodeID noise_texture;
    };
    const auto& raster_node = render_graph.add_raster_pass<RenderPassParameter>(
      "Render Pass",
      gpu::RGRenderTargetDesc(viewport, color_attachment_desc),
      [noise_texture_node](auto& parameter, auto& builder) {
        parameter.noise_texture = builder.add_shader_texture(
          noise_texture_node,
          {gpu::ShaderStage::VERTEX, gpu::ShaderStage::FRAGMENT},
          gpu::ShaderTextureReadUsage::UNIFORM);
      },
      [viewport, this](const auto& parameter, auto& registry, auto& command_list) {
        const gpu::GraphicPipelineStateDesc pipeline_desc = {
          .program_id = program_id_,
          .input_bindings = {{.stride = sizeof(Vertex)}},
          .input_attributes =
            {
              {
                .binding = 0,
                .offset = offsetof(Vertex, position),
                .type = gpu::VertexElementType::FLOAT2,
              },
              {
                .binding = 0,
                .offset = offsetof(Vertex, texture_coords),
                .type = gpu::VertexElementType::FLOAT3,
              },
            },
          .viewport =
            {.width = static_cast<float>(viewport.x), .height = static_cast<float>(viewport.y)},
          .scissor = {.extent = viewport},
          .color_attachment_count = 1,
        };
        const auto pipeline_state_id = registry.get_pipeline_state(pipeline_desc);

        struct PushConstant {
          gpu::DescriptorID texture_descriptor_id;
          gpu::DescriptorID sampler_descriptor_id;
          float depth = 0.0f;
        };

        depth_ = (fmod(get_elapsed_seconds(), CYCLE_DURATION) / static_cast<float>(DIMENSION.z));

        const PushConstant push_constant = {
          .texture_descriptor_id =
            gpu_system_->get_srv_descriptor_id(registry.get_texture(parameter.noise_texture)),
          .sampler_descriptor_id = gpu_system_->get_sampler_descriptor_id(test_sampler_id_),
          .depth = depth_,
        };

        command_list.push(gpu::RenderCommandDrawIndex{
          .pipeline_state_id = pipeline_state_id,
          .push_constant_data = soul::cast<void*>(&push_constant),
          .push_constant_size = sizeof(PushConstant),
          .vertex_buffer_ids = {vertex_buffer_id_},
          .index_buffer_id = index_buffer_id_,
          .first_index = 0,
          .index_count = std::size(INDICES),
        });
      });

    return raster_node.get_color_attachment_node_id();
  }

public:
  explicit Texture3DSampleApp(const AppConfig& app_config) : App(app_config)
  {
    runtime::ScopeAllocator scope_allocator("Texture 3D Sample App");
    gpu::ShaderSource shader_source = gpu::ShaderFile("texture_3d_sample.hlsl");
    std::filesystem::path search_path = "shaders/";
    const auto entry_points = soul::Array{
      gpu::ShaderEntryPoint{gpu::ShaderStage::VERTEX, "vsMain"},
      gpu::ShaderEntryPoint{gpu::ShaderStage::FRAGMENT, "psMain"},
    };
    const gpu::ProgramDesc program_desc = {
      .search_paths = u32cspan(&search_path, 1),
      .sources = u32cspan(&shader_source, 1),
      .entry_points = entry_points.cspan<u32>(),
    };
    auto result = gpu_system_->create_program(program_desc);
    if (result.is_ok()) {
      SOUL_PANIC("Fail to create program");
    }
    program_id_ = result.ok_ref();

    vertex_buffer_id_ = gpu_system_->create_buffer(
      {
        .size = sizeof(Vertex) * std::size(VERTICES),
        .usage_flags = {gpu::BufferUsage::VERTEX},
        .queue_flags = {gpu::QueueType::GRAPHIC},
        .name = "Vertex buffer",
      },
      VERTICES);
    gpu_system_->flush_buffer(vertex_buffer_id_);

    index_buffer_id_ = gpu_system_->create_buffer(
      {
        .size = sizeof(Index) * std::size(INDICES),
        .usage_flags = {gpu::BufferUsage::INDEX},
        .queue_flags = {gpu::QueueType::GRAPHIC},
        .name = "Index buffer",
      },
      INDICES);
    gpu_system_->flush_buffer(index_buffer_id_);

    {
      const gpu::TextureRegionUpdate region_load = {
        .subresource = {.layer_count = 1}, .extent = DIMENSION};

      const auto* data = create_noise_data(DIMENSION, scope_allocator);
      static constexpr usize CHANNEL_COUNT = 1;
      const gpu::TextureLoadDesc load_desc = {
        .data = data,
        .data_size = soul::cast<usize>(DIMENSION.x) * DIMENSION.y * DIMENSION.z * CHANNEL_COUNT,
        .regions = {&region_load, 1},
      };

      test_texture_id_ = gpu_system_->create_texture(
        gpu::TextureDesc::d3(
          "Test texture",
          gpu::TextureFormat::R8,
          1,
          {gpu::TextureUsage::SAMPLED, gpu::TextureUsage::TRANSFER_SRC},
          {gpu::QueueType::GRAPHIC, gpu::QueueType::TRANSFER},
          {128, 128, 128}),
        load_desc);
      test_sampler_id_ = gpu_system_->request_sampler(gpu::SamplerDesc::same_filter_wrap(
        gpu::TextureFilter::LINEAR, gpu::TextureWrap::CLAMP_TO_EDGE));
    }
  }
};

auto main(int /* argc */, char* /* argv */[]) -> int
{
  stbi_set_flip_vertically_on_load(true);
  Texture3DSampleApp app({.enable_imgui = true});
  app.run();

  return 0;
}
