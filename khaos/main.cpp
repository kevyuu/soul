#include "app/app.h"

#include "memory/allocators/malloc_allocator.h"
#include "store.h"
#include "view.h"

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*
  {
    static memory::MallocAllocator s_malloc_allocator("Non-Worker Malloc Allocator"_str);

    if (runtime::is_worker_thread())
    {
      return runtime::get_context_allocator();
    }
    return &s_malloc_allocator;
  }

} // namespace soul

using namespace soul;

namespace khaos
{
  class ArdentgineApp : public soul::app::App
  {
  private:
  public:
    ArdentgineApp() : App("Khaos"_str), store_(storage_path_cref(), &gpu_system_ref()) {};

  private:
    void on_render_frame(NotNull<gpu::RenderGraph*> /*render_graph*/) override
    {
      app::Gui& gui = gui_ref();
      store_.on_new_frame();
      view_.render(&gui, &store_);
    }

    View view_;
    Store store_;
  };

} // namespace khaos

auto main() -> int
{
  auto app = khaos::ArdentgineApp();
  app.run();
  return 0;
}
