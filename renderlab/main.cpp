#include "editor/store.h"
#include "editor/view.h"
#include "scene.h"

#include "app/app.h"
#include "demo/pica_pica_demo.h"
#include "demo/sponza_demo.h"

#include <cstdio>

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*
  {
    return runtime::get_context_allocator();
  }

} // namespace soul

using namespace soul;

namespace renderlab
{
  class RenderlabApp : public soul::app::App
  {
  private:
    Scene scene_;
    EditorStore editor_store_;
    EditorView editor_view_;

  public:
    RenderlabApp()
        : scene_(Scene::Create(&gpu_system_ref())),
          editor_store_(&scene_),
          editor_view_(&editor_store_)
    {
      SponzaDemo demo;
      demo.load_scene(&scene_);
    }

  private:
    void on_render_frame(NotNull<gpu::RenderGraph*> render_graph) override
    {
      scene_.prepare_render_data(render_graph);
      editor_store_.active_render_pipeline_ref().submit_passes(render_graph);
      app::Gui& gui = gui_ref();
      editor_view_.render(&gui);
    }
  };
} // namespace renderlab

auto main() -> int
{
  auto app = renderlab::RenderlabApp();
  app.run();
  return 0;
}
