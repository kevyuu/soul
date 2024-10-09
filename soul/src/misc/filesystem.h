#include "core/config.h"
#include "core/path.h"
#include "core/string.h"

namespace soul
{
  auto get_file_content(
    const Path& path, memory::Allocator* allocator = soul::get_default_allocator()) -> String;

  void write_to_file(const Path& path, StringView string);

  void delete_file(const Path& path);
} // namespace soul
