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
#include <pa171/quantization/haar_iwt.hpp>
#include <pa171/transform/wavelet.hpp>
#include <pa171/utils/view_2d.hpp>

namespace pa171
{

class image_encoder
{
public:
  void set_transform_haar_iwt(
    std::optional<std::size_t> const num_iters = std::nullopt,
    int const q_factor = 2,
    int const q_alpha = 2,
    int const q_beta = 0)
  {
    using recursive_2d_wt_type =
      transform::recursive_2d_wavelet_transform<std::int32_t>;

    transform_function_ = [=,
                           recursive_2d_wt = recursive_2d_wt_type{},
                           quantizer = quantization::haar_iwt{}](
                            view_2d<std::int32_t const*> const input,
                            std::int32_t* const output) mutable
    {
      recursive_2d_wt(
        input, output, transform::haar_iwt<std::int32_t>, num_iters);

      quantizer(output,
                input.width(),
                input.height(),
                q_factor,
                q_alpha,
                q_beta,
                num_iters);
    };
  }

  void set_transform_db4_iwt(
    std::optional<std::size_t> const num_iters = std::nullopt)
  {
    using recursive_2d_wt_type =
      transform::recursive_2d_wavelet_transform<std::int32_t>;

    transform_function_ = [=, recursive_2d_wt = recursive_2d_wt_type{}](
                            view_2d<std::int32_t const*> const input,
                            std::int32_t* const output) mutable
    {
      recursive_2d_wt(
        input, output, transform::db4_iwt<std::int32_t>, num_iters);
    };
  }

  void set_transform_bior_2_2_iwt(
    std::optional<std::size_t> const num_iters = std::nullopt)
  {
    using recursive_2d_wt_type =
      transform::recursive_2d_wavelet_transform<std::int32_t>;

    transform_function_ = [=, recursive_2d_wt = recursive_2d_wt_type{}](
                            view_2d<std::int32_t const*> const input,
                            std::int32_t* const output) mutable
    {
      recursive_2d_wt(
        input, output, transform::bior_2_2_iwt<std::int32_t>, num_iters);
    };
  }

  //  void set_transform_haar_wavelet()
  //  {
  //    using recursive_2d_wt_type =
  //    transform::recursive_2d_wavelet_transform<float>;
  //
  //    transform_function_ = wrap_floating_point_transform(
  //      [recursive_2d_wt = recursive_2d_wt_type{}](view_2d<float const*> const
  //      input,
  //                                       float* const output) mutable
  //      { recursive_2d_wt(input, output, transform::haar_wt<float>); });
  //  }
  //
  //  void set_transform_db4_wavelet()
  //  {
  //    using recursive_2d_wt_type =
  //    transform::recursive_2d_wavelet_transform<float>;
  //
  //    transform_function_ = wrap_floating_point_transform(
  //      [recursive_2d_lwt = recursive_2d_wt_type{}](view_2d<float const*>
  //      const input,
  //                                        float* const output) mutable
  //      { recursive_2d_lwt(input, output, transform::db4_wt<float>); });
  //  }

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

    transform_out_.resize(width * height);

    transform_in_regions_.clear();
    transform_out_regions_.clear();

    // Split transform input and output into regions
    if (region_size_)
    {
      auto buffer_offset = std::size_t{ 0 };

      for (auto i = std::size_t{ 0 }; i < height; i += *region_size_)
      {
        for (auto j = std::size_t{ 0 }; j < width; j += *region_size_)
        {
          auto const region_width = std::min(width - j, *region_size_);
          auto const region_height = std::min(height - i, *region_size_);

          transform_in_regions_.emplace_back(
            input.block(j, i, region_width, region_height));
          transform_out_regions_.emplace_back(
            std::span{ transform_out_ }.subspan(buffer_offset,
                                                region_width * region_height));

          buffer_offset += region_width * region_height;
        }
      }
    }
    else
    {
      transform_in_regions_.push_back(input);
      transform_out_regions_.emplace_back(transform_out_);
    }

    auto min = std::numeric_limits<std::int32_t>::max();
    auto max = std::numeric_limits<std::int32_t>::min();
    // For each region, apply the transform
    for (auto const [in_region, out_region] :
         ranges::views::zip(transform_in_regions_, transform_out_regions_))
    {
      if (transform_function_)
      {
        auto const region_width = in_region.width();
        auto const region_height = in_region.height();
        transform_in_hdr_.resize(region_width * region_height);
        transform_out_hdr_.resize(out_region.size());

        std::ranges::transform(in_region.rows() | ranges::views::join,
                               transform_in_hdr_.begin(),
                               ranges::convert_to<std::int32_t>{});

        transform_function_(
          view_2d{ transform_in_hdr_.data(), region_width, region_height },
          transform_out_hdr_.data());


        for (auto& el : transform_out_hdr_)
        {
          min = std::min(el, min);
          max = std::max(el, max);
        }

        std::ranges::transform(transform_out_hdr_,
                               out_region.begin(),
                               ranges::convert_to<std::int8_t>{});
      }
      else
      {
        // No transform - input will be encoded as is
        std::ranges::transform(in_region.rows() | ranges::views::join,
                               out_region.begin(),
                               ranges::convert_to<std::int8_t>{});
      }
    }

    std::cout << "min=" << min << std::endl;
    std::cout << "max=" << max << std::endl;

    byte_encoding_function_(std::as_bytes(std::span{ transform_out_ }), output);
  }

private:
  using transform_function_type = void(view_2d<std::int32_t const*> input,
                                       std::int32_t* output);
  using byte_encoding_function_type = void(std::span<std::byte const> input,
                                           std::vector<std::byte>& output);

  // Components
  std::function<transform_function_type> transform_function_;
  std::function<byte_encoding_function_type> byte_encoding_function_;

  // Settings
  std::optional<std::size_t> region_size_;

  // Buffers
  std::vector<std::int32_t> transform_in_hdr_;
  std::vector<std::int32_t> transform_out_hdr_;
  std::vector<std::uint8_t> transform_in_;
  std::vector<std::int8_t> transform_out_;
  std::vector<view_2d<std::uint8_t const*>> transform_in_regions_;
  std::vector<std::span<std::int8_t>> transform_out_regions_;

  //  [[nodiscard]] auto wrap_floating_point_transform(auto transform)
  //    -> std::function<transform_function_type>
  //  {
  //    return [transform = std::move(transform),
  //            in_buffer = std::vector<float>{},
  //            out_buffer =
  //              std::vector<float>{}](view_2d<std::uint8_t const*> const
  //              input,
  //                                    std::uint8_t* const output) mutable
  //    {
  //      auto const width = input.width();
  //      auto const height = input.height();
  //
  //      in_buffer.resize(width * height);
  //      out_buffer.resize(width * height);
  //
  //      // Convert to floating point
  //      std::ranges::transform(input.rows() | ranges::views::join,
  //                             in_buffer.begin(),
  //                             ranges::convert_to<float>{});
  //
  //      // Apply transform
  //      transform(view_2d{ in_buffer.data(), width, height },
  //      out_buffer.data());
  //
  //      // Convert to integral
  //      std::ranges::transform(
  //        out_buffer, output, ranges::convert_to<std::uint8_t>{});
  //    };
  //  }
};

} // namespace pa171
