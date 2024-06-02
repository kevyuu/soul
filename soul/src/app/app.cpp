#include "app/app.h"
#include "core/panic.h"
#include "runtime/runtime.h"

namespace soul::app
{

  AppRuntime::AppRuntime()
      : malloc_allocator_("Default Allocator"_str),
        default_allocator_(
          &malloc_allocator_,
          runtime::DefaultAllocatorProxy::Config(
            memory::MutexProxy::Config(),
            memory::ProfileProxy::Config(),
            memory::CounterProxy::Config(),
            memory::ClearValuesProxy::Config{u8{0xFA}, u8{0xFF}},
            memory::BoundGuardProxy::Config())),
        page_allocator_("Page allocator"_str),
        proxy_page_allocator_(&page_allocator_, memory::ProfileProxy::Config()),
        linear_allocator_(
          "Main Thread Temporary Allocator"_str, 10 * ONE_MEGABYTE, &proxy_page_allocator_),
        temp_allocator_(&linear_allocator_, runtime::TempProxy::Config())
  {
    runtime::init({0, 4096, &temp_allocator_, 20 * ONE_MEGABYTE, &default_allocator_});
  }

  AppRuntime::~AppRuntime()
  {
    runtime::shutdown();
  }

  App::App() : window_(Window::Desc{}, this), gpu_system_(runtime::get_context_allocator())
  {
    const gpu::System::Config config = {
      .wsi                 = &window_.wsi_ref(),
      .max_frame_in_flight = 3,
      .thread_count        = runtime::get_thread_count(),
    };
    gpu_system_.init(config);
    gui_ = get_default_allocator()->create<Gui>(&gpu_system_, 1.0f);
  }

  App::~App()
  {
    gpu_system_.shutdown();
  }

  void App::run()
  {
    window_.msg_loop();
  }

  auto App::gui_ref() -> Gui&
  {
    return *gui_;
  }

  auto App::gpu_system_ref() -> gpu::System&
  {
    return gpu_system_;
  }

  void App::handle_window_size_change()
  {
    const vec2u32 window_size = window_.get_client_area_size();
    gui_->on_window_resize(window_size.x, window_size.y);
    gpu_system_.recreate_swapchain();
    on_window_resize(window_size.x, window_size.y);
  }

  void App::handle_render_frame()
  {
    runtime::System::get().begin_frame();
    cpu_timer_.tick();
    gui_->begin_frame();
    gpu::RenderGraph render_graph;

    const auto swapchain_texture_node_id =
      render_graph.import_texture("Swapchain Texture"_str, gpu_system_.get_swapchain_texture());
    on_render_frame(&render_graph);
    gui_->render_frame(&render_graph, swapchain_texture_node_id, cpu_timer_.delta_in_seconds());
    gpu_system_.execute(render_graph);
    gpu_system_.flush_frame();
  }

  void App::handle_keyboard_event(const KeyboardEvent& key_event)
  {
    if (gui_->on_keyboard_event(key_event))
    {
      return;
    }
    on_keyboard_event(key_event);
  }

  void App::handle_mouse_event(const MouseEvent& mouse_event)
  {
    if (gui_->on_mouse_event(mouse_event))
    {
      return;
    }
    on_mouse_event(mouse_event);
  }

  void App::handle_window_focus_event(b8 focused)
  {
    gui_->on_window_focus_event(focused);
    on_window_focus_event(focused);
  }

  void App::handle_dropped_file(const Path& path)
  {
    on_dropped_file(path);
  }
} // namespace soul::app
