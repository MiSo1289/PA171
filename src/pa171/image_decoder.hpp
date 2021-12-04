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
#include <pa171/coding/lzw_decoder.hpp>
#include <pa171/quantization/haar_iwt.hpp>
#include <pa171/transform/wavelet.hpp>
#include <pa171/utils/view_2d.hpp>

namespace pa171
{

class image_decoder
{
public:
  void set_transform_haar_iwt(
    std::optional<std::size_t> const num_iters = std::nullopt,
    int const q_factor = 2,
    int const q_alpha = 2,
    int const q_beta = 0)
  {
    using inv_recursive_2d_wt_type =
      transform::inv_recursive_2d_wavelet_transform<std::int32_t>;

    transform_function_ = [=,
                           inv_recursive_2d_wt = inv_recursive_2d_wt_type{},
                           inv_quantizer = quantization::inv_haar_iwt{}](
                            std::span<std::int32_t> const input,
                            view_2d<std::int32_t*> const output) mutable
    {
      inv_quantizer(input.data(),
                    output.width(),
                    output.height(),
                    q_factor,
                    q_alpha,
                    q_beta,
                    num_iters);

      inv_recursive_2d_wt(
        input, output, transform::inv_haar_iwt<std::int32_t>, num_iters);
    };
  }

  void set_transform_db4_iwt(
    std::optional<std::size_t> const num_iters = std::nullopt)
  {
    using inv_recursive_2d_wt_type =
      transform::inv_recursive_2d_wavelet_transform<std::int32_t>;

    transform_function_ = [=, inv_recursive_2d_wt = inv_recursive_2d_wt_type{}](
                            std::span<std::int32_t const> const input,
                            view_2d<std::int32_t*> const output) mutable
    {
      inv_recursive_2d_wt(
        input, output, transform::inv_db4_iwt<std::int32_t>, num_iters);
    };
  }

  void set_transform_bior_2_2_iwt(
    std::optional<std::size_t> const num_iters = std::nullopt)
  {
    using inv_recursive_2d_wt_type =
      transform::inv_recursive_2d_wavelet_transform<std::int32_t>;

    transform_function_ = [=, inv_recursive_2d_wt = inv_recursive_2d_wt_type{}](
                            std::span<std::int32_t const> const input,
                            view_2d<std::int32_t*> const output) mutable
    {
      inv_recursive_2d_wt(
        input, output, transform::inv_bior_2_2_iwt<std::int32_t>, num_iters);
    };
  }

  //  void set_transform_haar_wavelet()
  //  {
  //    using inv_recursive_2d_wt_type =
  //    transform::inv_recursive_2d_wavelet_transform<float>;
  //
  //    transform_function_ = wrap_floating_point_transform(
  //      [inv_recursive_2d_wt =
  //         inv_recursive_2d_wt_type{}](std::span<float const> const input,
  //                                view_2d<float*> const output) mutable
  //      { inv_recursive_2d_wt(input, output, transform::inv_haar_wt<float>);
  //      });
  //  }
  //
  //  void set_transform_db4_wavelet()
  //  {
  //    using inv_recursive_2d_wt_type =
  //    transform::inv_recursive_2d_wavelet_transform<float>;
  //
  //    transform_function_ = wrap_floating_point_transform(
  //      [inv_recursive_2d_lwt =
  //         inv_recursive_2d_wt_type{}](std::span<float const> const input,
  //                                view_2d<float*> const output) mutable
  //      { inv_recursive_2d_lwt(input, output, transform::inv_db4_wt<float>);
  //      });
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
    byte_decoding_function_ =
      [lzw_decoder = coding::lzw::decoder{ code_size, options }](
        std::span<std::byte const> const input,
        std::vector<std::byte>& output) mutable
    { lzw_decoder(input, std::back_inserter(output)); };
  }

  void operator()(std::span<std::byte const> const input,
                  view_2d<std::uint8_t*> const output)
  {
    auto const width = output.width();
    auto const height = output.height();

    decoded_.clear();
    byte_decoding_function_(input, decoded_);

    if (decoded_.size() != width * height)
    {
      throw std::runtime_error{ "Decoded output length does not match" };
    }

    auto const transform_in = std::span<std::int8_t const>{
      reinterpret_cast<std::int8_t const*>(decoded_.data()),
      decoded_.size(),
    };

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
            transform_in.subspan(buffer_offset, region_width * region_height));
          transform_out_regions_.push_back(
            output.block(j, i, region_width, region_height));

          buffer_offset += region_width * region_height;
        }
      }
    }
    else
    {
      //      transform_in_regions_.push_back(transform_in);
      //      transform_out_regions_.push_back(output);
    }

    // For each region, apply the transform
    for (auto const [in_region, out_region] :
         ranges::views::zip(transform_in_regions_, transform_out_regions_))
    {

      if (transform_function_)
      {
        auto const region_width = out_region.width();
        auto const region_height = out_region.height();
        transform_in_hdr_.resize(in_region.size());
        transform_out_hdr_.resize(region_width * region_height);

        std::ranges::transform(in_region,
                               transform_in_hdr_.begin(),
                               ranges::convert_to<std::int32_t>{});

        transform_function_(
          std::span{ transform_in_hdr_ },
          view_2d{ transform_out_hdr_.data(), region_width, region_height });

        for (auto& el : transform_out_hdr_)
        {
          el = std::clamp(
            el,
            static_cast<std::int32_t>(std::numeric_limits<std::uint8_t>::min()),
            static_cast<std::int32_t>(
              std::numeric_limits<std::uint8_t>::max()));
        }

        std::ranges::transform(
          transform_out_hdr_,
          (out_region.rows() | ranges::views::join).begin(),
          ranges::convert_to<std::uint8_t>{});
      }
      else
      {
        // No transform - copy decoded bytes directly to output
        std::ranges::copy(in_region,
                          (out_region.rows() | ranges::views::join).begin());
      }
    }
  }

private:
  using transform_function_type = void(std::span<std::int32_t> input,
                                       view_2d<std::int32_t*> output);
  using byte_decoding_function_type = void(std::span<std::byte const> input,
                                           std::vector<std::byte>& output);

  // Components
  std::function<transform_function_type> transform_function_;
  std::function<byte_decoding_function_type> byte_decoding_function_;

  // Settings
  std::optional<std::size_t> region_size_;

  // Buffers
  std::vector<std::byte> decoded_;
  std::vector<std::int32_t> transform_in_hdr_;
  std::vector<std::int32_t> transform_out_hdr_;
  std::vector<std::span<std::int8_t const>> transform_in_regions_;
  std::vector<view_2d<std::uint8_t*>> transform_out_regions_;

  //  [[nodiscard]] auto wrap_floating_point_transform(auto transform)
  //    -> std::function<transform_function_type>
  //  {
  //    return
  //      [transform = std::move(transform),
  //       in_buffer = std::vector<float>{},
  //       out_buffer = std::vector<float>{}](std::span<std::uint8_t const>
  //       input,
  //                                          view_2d<std::uint8_t*> output)
  //                                          mutable
  //    {
  //      auto const width = output.width();
  //      auto const height = output.height();
  //
  //      in_buffer.resize(width * height);
  //      out_buffer.resize(width * height);
  //
  //      // Convert to floating point
  //      std::ranges::transform(
  //        input, in_buffer.begin(), ranges::convert_to<float>{});
  //
  //      // Apply transform
  //      transform(std::span{ in_buffer },
  //                view_2d{ out_buffer.data(), width, height });
  //
  //      // Convert to integral
  //      std::ranges::transform(out_buffer,
  //                             (output.rows() | ranges::views::join).begin(),
  //                             ranges::convert_to<std::uint8_t>{});
  //    };
  //  }
};

} // namespace pa171
