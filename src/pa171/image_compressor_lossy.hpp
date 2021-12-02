#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <vector>

#include <range/v3/functional/arithmetic.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/zip.hpp>

#include <pa171/coding/lzw_base.hpp>
#include <pa171/coding/lzw_encoder.hpp>
#include <pa171/transform/wavelet.hpp>
#include <pa171/utils/view_2d.hpp>

namespace pa171
{

class image_compressor_lossy
{
public:
  void set_transform_haar_wavelet()
  {
    using full_2d_wt_type = transform::full_2d_wavelet_transform<float>;

    transform_function_ =
      [full_2d_wt = full_2d_wt_type{}](view_2d<float const*> const input,
                                       float* const output) mutable
    { full_2d_wt(input, output, transform::haar_wt<float>); };
  }

  void set_transform_db4_wavelet()
  {
    using full_2d_wt_type = transform::full_2d_wavelet_transform<float>;

    transform_function_ =
      [full_2d_lwt = full_2d_wt_type{}](view_2d<float const*> const input,
                                        float* const output) mutable
    { full_2d_lwt(input, output, transform::db4_wt<float>); };
  }

  void set_region_size(std::size_t const region_size)
  {
    region_size_ = region_size;
  }

  void set_coding_lzw(
    coding::lzw::code_point_size_t const code_size =
      coding::lzw::default_code_size,
    coding::lzw::options_t const options = coding::lzw::default_options)
  {
    byte_encoding_function_ =
      [lzw_encoder = coding::lzw::encoder{ code_size, options }](
        std::span<std::byte const> const input,
        std::vector<std::byte>& output) mutable
    { lzw_encoder(input, std::back_inserter(output)); };
  }

  void operator()(view_2d<std::uint8_t const*> const input,
                  std::vector<std::byte>& output)
  {
    auto const width = input.width();
    auto const height = input.height();

    if (transform_function_)
    {
      transform_out_floor_.resize(width * height);

      transform_in_regions_.clear();
      transform_out_regions_.clear();

      // Split transform input and output into regions
      if (region_size_)
      {
        for (auto i = std::size_t{ 0 }; i < height; i += *region_size_)
        {
          for (auto j = std::size_t{ 0 }; j < width; j += *region_size_)
          {
            auto const region_width = std::min(width - j, *region_size_);
            auto const region_height = std::min(height - i, *region_size_);

            transform_in_regions_.emplace_back(
              input.block(j, i, region_width, region_height));
            transform_out_regions_.emplace_back(
              std::span{ transform_out_floor_ }.subspan(
                j + (i * width), region_width * region_height));
          }
        }
      }
      else
      {
        transform_in_regions_.push_back(input);
        transform_out_regions_.push_back(std::span{ transform_out_floor_ });
      }

      // For each region, apply the transform (TODO: and quantization)
      for (auto const [in_region, out_region] :
           ranges::views::zip(transform_in_regions_, transform_out_regions_))
      {
        auto const r_width = in_region.width();
        auto const r_height = in_region.height();

        transform_in_.resize(r_width * r_height);
        transform_out_.resize(r_width * r_height);

        // Convert to floating point
        std::ranges::transform(in_region.rows() | ranges::views::join,
                               transform_in_.begin(),
                               ranges::convert_to<float>{});

        // Apply transform
        transform_function_(view_2d{ transform_in_.data(), r_width, r_height },
                            transform_out_.data());

        // Convert to integral
        std::ranges::transform(transform_out_,
                               out_region.begin(),
                               ranges::convert_to<std::uint8_t>{});
      }
    }
    else
    {
      transform_out_floor_.resize(width * height);
      std::ranges::copy(input.rows() | ranges::views::join,
                        transform_out_floor_.begin());
    }

    byte_encoding_function_(std::as_bytes(std::span{ transform_out_floor_ }),
                            output);
  }

private:
  using transform_function_type = void(view_2d<float const*> data,
                                       float* output);
  using byte_encoding_function_type = void(std::span<std::byte const> input,
                                           std::vector<std::byte>& output);

  // Components
  std::function<transform_function_type> transform_function_;
  std::function<byte_encoding_function_type> byte_encoding_function_;

  // Settings
  std::optional<std::size_t> region_size_;

  // Buffers
  std::vector<float> transform_in_;
  std::vector<float> transform_out_;
  std::vector<std::uint8_t> transform_out_floor_;
  std::vector<view_2d<std::uint8_t const*>> transform_in_regions_;
  std::vector<std::span<std::uint8_t>> transform_out_regions_;
};

} // namespace pa171
