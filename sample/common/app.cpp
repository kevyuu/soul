#include "app.h"
#include "imgui_impl_glfw.h"

#include "core/dev_util.h"
#include "core/type.h"

#include "gpu/glfw_wsi.h"
#include "gpu/gpu.h"
#include "memory/allocators/page_allocator.h"
#include "runtime/runtime.h"

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>

#include "imgui_pass.h"
#include "memory/allocators/proxy_allocator.h"

#include <windows.h>

using namespace soul;

auto glfw_print_error_callback(const int code, const char* message) -> void
{
  SOUL_LOG_INFO("GLFW Error. Error code : %d. Message = %s", code, message);
}

auto glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height) -> void
{
  SOUL_LOG_INFO("GLFW Framebuffer size callback. Size = (%d, %d).", width, height);
  auto window_data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
  window_data->resized = true;
}

App::App(const AppConfig& app_config)
    : malloc_allocator_("Default Allocator"),
      default_allocator_(
        &malloc_allocator_,
        runtime::DefaultAllocatorProxy::Config(
          memory::MutexProxy::Config(),
          memory::ProfileProxy::Config(),
          memory::CounterProxy::Config(),
          memory::ClearValuesProxy::Config{uint8{0xFA}, uint8{0xFF}},
          memory::BoundGuardProxy::Config())),
      page_allocator_("Page allocator"),
      proxy_page_allocator_(&page_allocator_, memory::ProfileProxy::Config()),
      linear_allocator_(
        "Main Thread Temporary Allocator", 10 * ONE_MEGABYTE, &proxy_page_allocator_),
      temp_allocator_(&linear_allocator_, runtime::TempProxy::Config()),
      app_config_(app_config),
      camera_man_(
        {0.1f, 0.01f, vec3f(0.0f, 1.0f, 0.0f)}, vec3f(0, 0, 1.0f), vec3f(0, 0, 0), vec3f(0, 1, 0))
{
  SOUL_PROFILE_THREAD_SET_NAME("Main Thread");

  runtime::init({0, 4096, &temp_allocator_, 20 * ONE_MEGABYTE, &default_allocator_});

  glfwSetErrorCallback(glfw_print_error_callback);
  const bool glfw_init_success = glfwInit();
  SOUL_ASSERT(0, glfw_init_success, "GLFW initialization failed!");
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  SOUL_LOG_INFO("GLFW Initialization sucessful");

  SOUL_ASSERT(0, glfwVulkanSupported(), "Vulkan is not supported by glfw");

  if (app_config.screen_dimension) {
    const ScreenDimension screen_dimension = *app_config.screen_dimension;
    window_ =
      glfwCreateWindow(screen_dimension.width, screen_dimension.height, "Vulkan", nullptr, nullptr);
  } else {
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    SOUL_ASSERT(0, mode != nullptr, "Mode cannot be nullpointer");
    window_ = glfwCreateWindow(mode->width, mode->height, "Vulkan", nullptr, nullptr);
    glfwMaximizeWindow(window_);
  }
  SOUL_LOG_INFO("GLFW window creation sucessful");
  wsi_ = default_allocator_.create<gpu::GLFWWsi>(window_);

  glfwSetWindowUserPointer(window_, &window_data_);
  glfwSetFramebufferSizeCallback(window_, glfw_framebuffer_size_callback);

  gpu_system_ = default_allocator_.create<gpu::System>(runtime::get_context_allocator());
  const gpu::System::Config config = {
    .wsi = wsi_, .max_frame_in_flight = 3, .thread_count = runtime::get_thread_count()};

  gpu_system_->init(config);
  gpu_properties_ = gpu_system_->get_gpu_properties();

  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGui_ImplGlfw_InitForVulkan(window_, true);
  int width, height;
  unsigned char* font_pixels;
  io.Fonts->GetTexDataAsRGBA32(&font_pixels, &width, &height);

  if (app_config.enable_imgui) {
    imgui_render_graph_pass_ =
      runtime::get_context_allocator()->create<ImGuiRenderGraphPass>(gpu_system_);
  }
}

App::~App()
{
  if (app_config_.enable_imgui) {
    runtime::get_context_allocator()->destroy(imgui_render_graph_pass_);
  }
  default_allocator_.destroy(gpu_system_);
  default_allocator_.destroy(wsi_);
  glfwDestroyWindow(window_);
  glfwTerminate();
}

auto App::run() -> void
{
  while (!glfwWindowShouldClose(window_)) {
    SOUL_PROFILE_FRAME();
    runtime::System::get().begin_frame();

    vec2f glfw_window_scale;
    glfwGetWindowContentScale(window_, &glfw_window_scale.x, &glfw_window_scale.y);
    SOUL_ASSERT(0, glfw_window_scale.x - glfw_window_scale.y == 0.0f, "");

    {
      SOUL_PROFILE_ZONE_WITH_NAME("GLFW Poll Events");
      glfwPollEvents();
      if (window_data_.resized) {
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        if (width == 0 || height == 0) {
          glfwWaitEvents();
          continue;
        }
        gpu_system_->recreate_swapchain();
        window_data_.resized = false;
      }
    }
    gpu::RenderGraph render_graph;

    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = glfw_window_scale.x;

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
      // orbit camera
      const auto mouse_delta = ImGui::GetIO().MouseDelta;
      if (ImGui::GetIO().KeyShift) {
        camera_man_.pan(mouse_delta.x, mouse_delta.y);
      } else {
        camera_man_.orbit(mouse_delta.x, mouse_delta.y);
      }
    }

    if (io.MouseWheel != 0.0f && io.KeyShift) {
      camera_man_.zoom(io.MouseWheel);
    }

    const auto swapchain_texture_node_id =
      render_graph.import_texture("Swapchain Texture", gpu_system_->get_swapchain_texture());
    const auto render_target_node_id = render(swapchain_texture_node_id, render_graph);

    ImGui::Render();
    if (app_config_.enable_imgui) {
      imgui_render_graph_pass_->add_pass(render_target_node_id, render_graph);
    }

    gpu_system_->execute(render_graph);

    gpu_system_->flush_frame();
    frame_index_++;
  }
}

auto App::get_elapsed_seconds() const -> float
{
  const auto end = std::chrono::steady_clock::now();
  const std::chrono::duration<double> elapsed_seconds = end - start_;
  const auto elapsed_seconds_float = static_cast<float>(elapsed_seconds.count());
  return elapsed_seconds_float;
}

auto App::get_frame_index() const -> soul_size { return frame_index_; }

auto App::get_exe_path() -> std::filesystem::path
{
  static std::string s_exe_path;
  static auto s_exe_path_init = false;
  if (!s_exe_path_init) {
    char module_path[MAX_PATH];
    const size_t module_path_length = GetModuleFileNameA(nullptr, module_path, MAX_PATH);
    s_exe_path = std::string(module_path, module_path_length);
    s_exe_path_init = true;
  }

  return s_exe_path;
}

auto App::get_media_path() -> std::filesystem::path
{
  return get_exe_path().parent_path().parent_path() / "media";
}
