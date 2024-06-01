#include "misc/image_data.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "core/log.h"
#include "core/option.h"
#include "core/vector.h"

namespace soul
{

  ImageData::ImageData(ImageData&& other) noexcept
      : pixels_(std::exchange(other.pixels_, nullptr)),
        width_(std::exchange(other.width_, 0)),
        height_(std::exchange(other.height_, 0)),
        channel_count_(std::exchange(other.channel_count_, 0))
  {
  }

  auto ImageData::operator=(ImageData&& other) noexcept -> ImageData&
  {
    using std::swap;
    swap(pixels_, other.pixels_);
    swap(width_, other.width_);
    swap(height_, other.height_);
    swap(channel_count_, other.channel_count_);
    return *this;
  }

  void ImageData::cleanup()
  {
    SOUL_ASSERT(0, pixels_ != nullptr);
    stbi_image_free(pixels_);
  }

  ImageData::~ImageData()
  {
    if (pixels_ != nullptr)
    {
      cleanup();
    }
  }

  auto ImageData::FromRawBytes(Span<const byte*> bytes, const Option<u32> desired_channel_count)
    -> ImageData
  {
    i32 width         = 0;
    i32 height        = 0;
    i32 channel_count = 0;

    i32 stb_desired_channel_count = cast<i32>(desired_channel_count.unwrap_or(0LLU));
    byte* pixels                  = stbi_load_from_memory(
      bytes.data(),
      cast<i32>(bytes.size_in_bytes()),
      &width,
      &height,
      &channel_count,
      stb_desired_channel_count);

    SOUL_ASSERT(0, pixels != nullptr, "Fail to load pixels");
    if (desired_channel_count.is_some())
    {
      channel_count = cast<i32>(desired_channel_count.some_ref());
    }

    return ImageData(pixels, width, height, channel_count);
  }

  auto ImageData::FromFile(const Path& path, const Option<u32> desired_channel_count) -> ImageData
  {
    i32 width         = 0;
    i32 height        = 0;
    i32 channel_count = 0;

    const auto path_str = path.string();
    const b8 is_hdr     = stbi_is_hdr(path_str.c_str()) != 0;

    const i32 stb_desired_channel_count = i32(desired_channel_count.unwrap_or(0LLU));

    byte* pixels = [&]
    {
      if (is_hdr)
      {
        return cast<byte*>(
          stbi_loadf(path_str.c_str(), &width, &height, &channel_count, stb_desired_channel_count));
      } else
      {
        return stbi_load(
          path_str.c_str(), &width, &height, &channel_count, stb_desired_channel_count);
      }
    }();

    SOUL_ASSERT(0, pixels != nullptr, "Fail to load pixels");

    if (desired_channel_count.is_some())
    {
      channel_count = cast<i32>(desired_channel_count.some_ref());
    }

    return ImageData(pixels, width, height, channel_count, is_hdr);
  }

  ImageData::ImageData(byte* pixels, i32 width, i32 height, i32 channel_count, b8 is_hdr)
      : pixels_(pixels),
        width_(width),
        height_(height),
        channel_count_(channel_count),
        is_hdr_(is_hdr)
  {
  }

} // namespace soul
