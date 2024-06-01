#pragma once

#include "core/builtins.h"
#include "core/integer.h"
#include "core/path.h"

namespace soul
{
  class ImageData
  {
  private:
    byte* pixels_      = nullptr;
    i32 width_         = 0;
    i32 height_        = 0;
    i32 channel_count_ = 0;
    b8 is_hdr_         = false;

  public:
    ImageData(const ImageData&) = delete;

    ImageData(ImageData&& other) noexcept;

    auto operator=(const ImageData&) -> ImageData& = delete;

    auto operator=(ImageData&&) noexcept -> ImageData&;

    ~ImageData();

    [[nodiscard]]
    static auto FromRawBytes(Span<const byte*> bytes, Option<u32> desired_channel_count = nilopt)
      -> ImageData;

    [[nodiscard]]
    static auto FromFile(const Path& path, Option<u32> desired_channel_count = nilopt) -> ImageData;

    void cleanup();

    [[nodiscard]]
    auto dimension() const -> vec2u32
    {
      return {width_, height_};
    }

    [[nodiscard]]
    auto channel_count() const -> u32
    {
      return channel_count_;
    }

    [[nodiscard]]
    auto cdata() const -> const byte*
    {
      return pixels_;
    }

    [[nodiscard]]
    auto cspan() const -> Span<const byte*>
    {
      const u8 byte_per_channel = is_hdr_ ? 4 : 1;
      return {pixels_, cast<usize>(width_ * height_ * channel_count_ * byte_per_channel)};
    }

  private:
    explicit ImageData(byte* pixels, i32 width, i32 height, i32 channel_count, b8 is_hdr = false);
  };

} // namespace soul
