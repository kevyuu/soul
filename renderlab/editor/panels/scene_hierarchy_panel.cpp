#include "scene_hierarchy_panel.h"

#include "app/gui.h"

#include "editor/store.h"
#include "type.h"

namespace renderlab
{
  namespace impl
  {
    void render_entity_tree_node(
      EntityId entity_id, NotNull<EditorStore*> store, NotNull<app::Gui*> gui)
    {
      if (entity_id.is_null())
      {
        return;
      }
      auto flags = app::Gui::TreeNodeFlags{
        app::Gui::TreeNodeFlag::OPEN_ON_ARROW,
        app::Gui::TreeNodeFlag::OPEN_ON_DOUBLE_CLICK,
        app::Gui::TreeNodeFlag::SPAN_AVAIL_WIDTH,
      };
      if (store->get_selected_entity() == entity_id)
      {
        flags.set(app::Gui::TreeNodeFlag::SELECTED);
      }
      const EntityId first_child = store->scene_ref().get_entity_first_child(entity_id);
      const b8 has_any_child     = !first_child.is_null();
      if (!has_any_child)
      {
        flags |= {
          app::Gui::TreeNodeFlag::LEAF,
          app::Gui::TreeNodeFlag::NO_TREE_PUSH_ON_OPEN,
        };
      }
      const StringView entity_name = store->scene_ref().get_entity_name(entity_id);
      const b8 is_node_open        = gui->tree_node(entity_id.to_underlying(), flags, entity_name);
      if (gui->is_item_clicked())
      {
        store->select_entity(entity_id);
      }
      if (is_node_open && has_any_child)
      {
        render_entity_tree_node(first_child, store, gui);
        gui->tree_pop();
      }
      render_entity_tree_node(store->scene_ref().get_entity_next_sibling(entity_id), store, gui);
    }
  } // namespace impl

  SceneHierarchyPanel::SceneHierarchyPanel(NotNull<EditorStore*> store) : store_(store) {}

  void SceneHierarchyPanel::on_gui_render(NotNull<app::Gui*> gui)
  {
    if (gui->begin_window(
          LABEL,
          vec2f32(1400, 1040),
          vec2f32(20, 40),
          {
            app::Gui::WindowFlag::SHOW_TITLE_BAR,
            app::Gui::WindowFlag::ALLOW_MOVE,
          }))
    {
      impl::render_entity_tree_node(store_->scene_ref().get_root_entity_id(), store_, gui);
    }
    gui->end_window();
  }
} // namespace renderlab
