#include "core/log.h"

#include "misc/filesystem.h"

#include <cstdio>
#include <filesystem>

namespace soul::fs
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

    err = fclose(file);
    SOUL_ASSERT(0, err == 0);

    return result;
  }

  void write_file(const Path& path, StringView string)
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

  auto copy_file(const Path& from_path, const Path& to_path) -> b8
  {
    return std::filesystem::copy_file(from_path, to_path);
  }

  void delete_file(const Path& path)
  {
    const auto err = remove(path.string().c_str());
    SOUL_ASSERT(0, err == 0);
  }

  auto exists(const Path& path) -> b8
  {
    return std::filesystem::exists(path);
  }

  auto is_directory(const Path& path) -> b8
  {
    return std::filesystem::is_directory(path);
  }

} // namespace soul::fs
