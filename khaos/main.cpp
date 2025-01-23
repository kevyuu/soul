#include "app/app.h"

#include "core/vector.h"
#include "core/path.h"
#include "memory/allocators/malloc_allocator.h"
#include "gpu/render_graph.h"
#include "runtime/runtime.h"

#include "store/store.h"

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
  class KhaosApp : public soul::app::App
  {
  private:
  public:
    explicit KhaosApp(const Option<Path>& project_path)
        : App("Khaos"_str), store_(storage_path_cref(), &gpu_system_ref())
    {
      if (project_path.is_some())
      {
        store_.load_project(project_path.some_ref());
      }
    }

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

auto main(i32 argc, char** argv) -> int
{
  Option<Path> path = nilopt;
  if (argc > 1)
  {
    path = Path::From(StringView(argv[1]));
  }

  auto app = khaos::KhaosApp(Path::From("C:/Users/kevin/Dev/irresistible"_str));
  app.run();

  return 0;
}
