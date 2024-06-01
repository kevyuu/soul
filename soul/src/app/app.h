#pragma once

#include "app/cpu_timer.h"
#include "app/gui.h"
#include "app/window.h"

#include "gpu/render_graph.h"
#include "memory/allocators/malloc_allocator.h"
#include "memory/allocators/page_allocator.h"
#include "memory/allocators/proxy_allocator.h"

#include "runtime/runtime.h"

#include "gpu/gpu.h"

namespace soul::app
{
  class AppRuntime
  {
  private:
    memory::MallocAllocator malloc_allocator_;
    runtime::DefaultAllocator default_allocator_;
    memory::PageAllocator page_allocator_;
    memory::ProxyAllocator<memory::PageAllocator, memory::ProfileProxy> proxy_page_allocator_;
    memory::LinearAllocator linear_allocator_;
    runtime::TempAllocator temp_allocator_;

  public:
    AppRuntime();

    AppRuntime(const AppRuntime&) = delete;

    AppRuntime(AppRuntime&&) = delete;

    auto operator=(const AppRuntime&) -> AppRuntime& = delete;

    auto operator=(AppRuntime&&) -> AppRuntime& = delete;

    ~AppRuntime();
  };

  class App : Window::Callbacks
  {
  private:
    AppRuntime app_runtime_;
    Window window_;
    gpu::System gpu_system_;
    Gui* gui_;
    CpuTimer cpu_timer_;

  public:
    App();

    App(const App&) = delete;

    App(App&&) = delete;

    auto operator=(const App&) -> App& = delete;

    auto operator=(App&&) -> App& = delete;

    ~App() override;

    void run();

    virtual void on_window_resize(u32 width, u32 height) {}

    virtual void on_render_frame(NotNull<gpu::RenderGraph*> render_graph) {}

    virtual void on_keyboard_event(const KeyboardEvent& key_event) {}

    virtual void on_mouse_event(const MouseEvent& mouse_event) {}

    virtual void on_window_focus_event(b8 focused) {}

    virtual void on_dropped_file(const Path& path) {}

    auto gui_ref() -> Gui&;

    auto gpu_system_ref() -> gpu::System&;

  private:
    void handle_window_size_change() override;

    void handle_render_frame() override;

    void handle_keyboard_event(const KeyboardEvent& key_event) override;

    void handle_mouse_event(const MouseEvent& mouse_event) override;

    void handle_window_focus_event(b8 focused) override;

    void handle_dropped_file(const Path& path) override;
  };
} // namespace soul::app
