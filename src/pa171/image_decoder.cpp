#include <pa171/image_decoder.hpp>

#include <ranges>

#include <range/v3/functional/arithmetic.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/zip.hpp>

#include <pa171/coding/lzw_decoder.hpp>
#include <pa171/quantization/haar_iwt.hpp>
#include <pa171/transform/wavelet.hpp>

namespace pa171
{

void
image_decoder::set_region_size(std::size_t const region_size)
{
  region_size_ = region_size;
}

void
image_decoder::set_transform_haar_iwt(
  std::optional<std::size_t> const num_iters,
  int const q_factor,
  int const q_alpha,
  int const q_beta)
{
  transform_function_ =
    [=,
     inv_recursive_2d_wt =
       transform::inv_recursive_2d_wavelet_transform<std::int16_t>{},
     inv_quantizer = quantization::inv_haar_iwt<std::uint8_t, std::int16_t>{},
     hr_in_buffer = std::vector<std::int16_t>{},
     hr_out_buffer =
       std::vector<std::int16_t>{}](std::span<std::byte const> const input,
                                    view_2d<std::uint8_t*> const output) mutable
  {
    auto const width = output.width();
    auto const height = output.height();

    hr_in_buffer.resize(width * height);
    hr_out_buffer.resize(width * height);

    // De-quantize inputs
    inv_quantizer(
      std::span{
        reinterpret_cast<std::int8_t const*>(input.data()),
        input.size(),
      },
      hr_in_buffer.begin(),
      width,
      height,
      q_factor,
      q_alpha,
      q_beta,
      num_iters);

    // Apply inverse transform
    inv_recursive_2d_wt(hr_in_buffer,
                        view_2d{ hr_out_buffer.begin(), width, height },
                        transform::inv_haar_iwt<std::int16_t>,
                        num_iters);

    // Clamp-convert output region to uint8
    std::ranges::transform(
      hr_out_buffer,
      (output.rows() | ranges::views::join).begin(),
      [](std::int16_t const value)
      {
        return std::clamp(
          value,
          static_cast<std::int16_t>(std::numeric_limits<std::uint8_t>::min()),
          static_cast<std::int16_t>(std::numeric_limits<std::uint8_t>::max()));
      });
  };
}

void
image_decoder::set_coding_lzw(coding::lzw::code_point_size_t const code_size,
                              coding::lzw::options_t const options)
{
  byte_decoding_function_ =
    [lzw_decoder = coding::lzw::decoder{ code_size, options }](
      std::span<std::byte const> const input,
      std::vector<std::byte>& output) mutable
  { lzw_decoder(input, std::back_inserter(output)); };
}

void
image_decoder::operator()(std::span<std::byte const> const input,
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

        transform_in_regions_.emplace_back(std::span{ decoded_ }.subspan(
          buffer_offset, region_width * region_height));
        transform_out_regions_.push_back(
          output.block(j, i, region_width, region_height));

        buffer_offset += region_width * region_height;
      }
    }
  }
  else
  {
    transform_in_regions_.emplace_back(decoded_);
    transform_out_regions_.push_back(output);
  }

  // For each region, apply the transform
  for (auto const [in_region, out_region] :
       ranges::views::zip(transform_in_regions_, transform_out_regions_))
  {

    if (transform_function_)
    {
      transform_function_(in_region, out_region);
    }
    else
    {
      // No transform - copy decoded bytes directly to output
      std::ranges::copy_n(
        reinterpret_cast<std::uint8_t const*>(in_region.data()),
        in_region.size(),
        (out_region.rows() | ranges::views::join).begin());
    }
  }
}

} // namespace pa171
