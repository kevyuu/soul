#pragma once
#include <filesystem>

#include "gpu/render_graph.h"
#include "runtime/data.h"

struct GLFWwindow;
class ImGuiRenderGraphPass;

#include "builtins.h"
#include "camera_manipulator.h"

#include "memory/allocators/page_allocator.h"
#include "runtime/runtime.h"

namespace soul::gpu
{
  class System;
  class GLFWWsi;
} // namespace soul::gpu

struct ScreenDimension {
  i32 width;
  i32 height;
};

struct AppConfig {
  soul::Option<ScreenDimension> screen_dimension = soul::nilopt;
  bool enable_imgui = false;
};

struct WindowData {
  bool resized = false;
};

struct RuntimeInitializer {
  explicit RuntimeInitializer(const soul::runtime::Config& config);
};

class App
{
public:
  explicit App(const AppConfig& app_config);
  App(const App&) = delete;
  auto operator=(const App&) -> App& = delete;
  App(App&& app) = delete;
  auto operator=(App&& app) -> App& = delete;
  virtual ~App();

  auto run() -> void;
  auto get_elapsed_seconds() const -> float;
  auto get_frame_index() const -> usize;

  static auto get_exe_path() -> soul::Path;
  static auto get_media_path() -> soul::Path;

private:
  soul::memory::MallocAllocator malloc_allocator_;
  soul::runtime::DefaultAllocator default_allocator_;
  soul::memory::PageAllocator page_allocator_;
  soul::memory::ProxyAllocator<soul::memory::PageAllocator, soul::memory::ProfileProxy>
    proxy_page_allocator_;
  soul::memory::LinearAllocator linear_allocator_;
  soul::runtime::TempAllocator temp_allocator_;
  RuntimeInitializer runtime_initializer_;

  WindowData window_data_;
  soul::NotNull<GLFWwindow*> window_;
  ImGuiRenderGraphPass* imgui_render_graph_pass_ = nullptr;

  soul::NotNull<soul::gpu::GLFWWsi*> wsi_;

  const AppConfig app_config_;
  usize frame_index_ = 0;

  const std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();

  virtual auto render(soul::gpu::TextureNodeID render_target, soul::gpu::RenderGraph& render_graph)
    -> soul::gpu::TextureNodeID = 0;

  virtual auto handle_input() -> void {}

protected:
  soul::NotNull<soul::gpu::System*> gpu_system_;
  soul::gpu::GPUProperties gpu_properties_ = {};
  CameraManipulator camera_man_;
};
