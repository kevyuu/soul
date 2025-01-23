#include "core/config.h"
#include "core/path.h"
#include "core/string.h"

namespace soul::fs
{
  auto get_file_content(
    const Path& path, memory::Allocator* allocator = soul::get_default_allocator()) -> String;

  void write_file(const Path& path, StringView string);

  auto copy_file(const Path& from_path, const Path& to_path) -> b8;

  void delete_file(const Path& path);

  auto exists(const Path& path) -> b8;

  auto is_directory(const Path& path) -> b8;
} // namespace soul::fs
