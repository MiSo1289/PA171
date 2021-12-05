#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>
#include <optional>

#include <pa171/coding/lzw_base.hpp>
#include <pa171/utils/view_2d.hpp>

namespace pa171
{

class image_decoder
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

  void operator()(std::span<std::byte const> input,
                  view_2d<std::uint8_t*> output);

private:
  using transform_function_type = void(std::span<std::byte const> input,
                                       view_2d<std::uint8_t*> output);
  using byte_decoding_function_type = void(std::span<std::byte const> input,
                                           std::vector<std::byte>& output);

  // Components
  std::function<transform_function_type> transform_function_;
  std::function<byte_decoding_function_type> byte_decoding_function_;

  // Settings
  std::optional<std::size_t> region_size_;

  // Buffers
  std::vector<std::byte> decoded_;
  std::vector<std::span<std::byte const>> transform_in_regions_;
  std::vector<view_2d<std::uint8_t*>> transform_out_regions_;
};

} // namespace pa171
