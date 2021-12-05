#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

namespace pa171
{

struct image_data_deleter
{
  using pointer = std::uint8_t*;

  void operator()(pointer ptr);
};

using image_data_pointer = std::unique_ptr<std::uint8_t[], image_data_deleter>;

[[nodiscard]] auto read_grayscale_image(std::filesystem::path const& path,
                                        std::size_t& out_width,
                                        std::size_t& out_height)
  -> image_data_pointer;

void write_grayscale_image_as_bmp(std::filesystem::path const& path,
                           std::size_t width,
                           std::size_t height,
                           std::uint8_t const* data);

} // namespace pa171
