#include "app/app.h"

#include "store.h"
#include "view.h"

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*
  {
    return runtime::get_context_allocator();
  }

} // namespace soul

using namespace soul;

namespace khaos
{
  class ArdentgineApp : public soul::app::App
  {
  private:
  public:
    ArdentgineApp() : App("Astel"_str), store_(storage_path_cref()) {};

  private:
    void on_render_frame(NotNull<gpu::RenderGraph*> /*render_graph*/) override
    {
      app::Gui& gui = gui_ref();
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
