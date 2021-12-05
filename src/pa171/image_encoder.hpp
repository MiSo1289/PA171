#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

#include <pa171/coding/lzw_base.hpp>
#include <pa171/utils/view_2d.hpp>

namespace pa171
{

class image_encoder
{
public:
  void set_region_size(std::size_t region_size);

  void set_transform_haar_iwt(
    std::optional<std::size_t> num_iters = std::nullopt,
    int q_factor = 32,
    int q_alpha = 8,
    int q_beta = 0);

  void set_coding_lzw(
    coding::lzw::code_point_size_t code_size = coding::lzw::default_code_size,
    coding::lzw::options_t options = coding::lzw::default_options);

  void operator()(view_2d<std::uint8_t const*> input,
                  std::vector<std::byte>& output);

private:
  using transform_function_type = void(view_2d<std::uint8_t const*> input,
                                       std::byte* output);
  using byte_encoding_function_type = void(std::span<std::byte const> input,
                                           std::vector<std::byte>& output);

  // Components
  std::function<transform_function_type> transform_function_;
  std::function<byte_encoding_function_type> byte_encoding_function_;

  // Settings
  std::optional<std::size_t> region_size_;

  // Buffers
  std::vector<std::byte> transform_out_;
  std::vector<view_2d<std::uint8_t const*>> transform_in_regions_;
  std::vector<std::span<std::byte>> transform_out_regions_;
};

} // namespace pa171
