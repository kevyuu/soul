#include "new_project_popup.h"
#include "store.h"

#include "app/gui.h"

#include <filesystem>

namespace khaos
{
  void NewProjectPopup::on_gui_render(NotNull<app::Gui*> gui, Store* store)
  {
    if (gui->begin_popup_modal(get_id()))
    {
      gui->text("Name"_str);
      gui->input_text("###Name"_str, &name_);
      if (show_empty_name_error_)
      {
        gui->text_colored("Cannot create project with empty name"_str, vec4f32(1, 0.2, 0.2, 1.0));
      } else
      {
        gui->text(""_str);
      }

      gui->text("Path"_str);
      gui->input_text("###Path"_str, &path_);
      gui->same_line();
      if (gui->button("..."_str))
      {
        auto result = gui->open_folder_dialog("Project Location"_str);
        if (result.is_some())
        {
          const auto std_string = std::move(result).unwrap().string();
          path_                 = String::From(StringView(std_string.data(), std_string.size()));
        } else
        {
          path_ = ""_str;
        }
      }
      if (show_directory_not_exists_)
      {
        gui->text_colored("Folder does not exists"_str, vec4f32(1, 0.2, 0.2, 1.0));
      } else if (show_project_already_exists_)
      {
        gui->text_colored(
          "Project with the same name already exists in this folder"_str,
          vec4f32(1, 0.2, 0.2, 1.0));
      } else
      {
        gui->text(""_str);
      }

      if (gui->button("Create"_str))
      {
        show_empty_name_error_       = false;
        show_directory_not_exists_   = false;
        show_project_already_exists_ = false;
        const auto path              = Path::From(StringView(path_.c_str()));
        if (name_.size() == 0)
        {
          show_empty_name_error_ = true;
        } else if (!std::filesystem::is_directory(path))
        {
          show_directory_not_exists_ = true;
        } else if (std::filesystem::is_directory(path / name_.cspan()))
        {
          show_project_already_exists_ = true;
        } else
        {
          store->create_new_project(name_.cspan(), Path::From(StringView(path_.c_str())));
        }
      }
      gui->same_line();
      if (gui->button("Cancel"_str))
      {
        gui->close_current_popup();
      }

      gui->end_popup();
    }
  }

  void NewProjectPopup::open(NotNull<app::Gui*> gui)
  {
    gui->open_popup(get_id());
  }
} // namespace khaos
