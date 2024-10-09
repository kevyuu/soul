#include "core/log.h"

#include "misc/filesystem.h"

#include <cstdio>

namespace soul
{
  auto get_file_content(const Path& path, memory::Allocator* allocator) -> String
  {
    FILE* file = nullptr;
    auto err   = fopen_s(&file, path.string().data(), "rb");
    SOUL_ASSERT(0, err == 0);
    err = fseek(file, 0, SEEK_END); // seek to end of file
    SOUL_ASSERT(0, err == 0);
    const auto size = ftell(file); // get current file pointer
    SOUL_LOG_INFO("Size : {}", size);
    err = fseek(file, 0, SEEK_SET); // seek back to beginning of file
    SOUL_ASSERT(0, err == 0);

    auto result = String::WithSize(size, allocator);

    const auto read_count = fread(result.data(), 1, size, file);

    SOUL_LOG_INFO("File Content : {}", result);
    err = fclose(file);
    SOUL_ASSERT(0, err == 0);

    return result;
  }

  void write_to_file(const Path& path, StringView string)
  {
    SOUL_ASSERT(0, string.is_null_terminated());

    FILE* file = nullptr; // Create a file
    auto err   = fopen_s(&file, path.string().data(), "w");
    SOUL_ASSERT(0, err == 0);

    err = fputs(string.data(), file);
    SOUL_ASSERT(0, err == 0);

    err = fclose(file);
    SOUL_ASSERT(0, err == 0);
  }

  void delete_file(const Path& path)
  {
    const auto err = remove(path.string().c_str());
    SOUL_ASSERT(0, err == 0);
  }

} // namespace soul
