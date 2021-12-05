#include <pa171/image_encoder.hpp>

#include <ranges>

#include <range/v3/functional/arithmetic.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/zip.hpp>

#include <pa171/coding/lzw_encoder.hpp>
#include <pa171/quantization/haar_iwt.hpp>
#include <pa171/transform/wavelet.hpp>

namespace pa171
{

void
image_encoder::set_region_size(std::size_t const region_size)
{
  region_size_ = region_size;
}

void
image_encoder::set_transform_haar_iwt(
  std::optional<std::size_t> const num_iters,
  int const q_factor,
  int const q_alpha,
  int const q_beta)
{
  transform_function_ =
    [=,
     recursive_2d_wt =
       transform::recursive_2d_wavelet_transform<std::int16_t>{},
     quantizer = quantization::haar_iwt<std::uint8_t, std::int16_t>{},
     hr_in_buffer = std::vector<std::int16_t>{},
     hr_out_buffer = std::vector<std::int16_t>{}](
      view_2d<std::uint8_t const*> const input, std::byte* const output) mutable
  {
    auto const width = input.width();
    auto const height = input.height();

    hr_in_buffer.resize(width * height);
    hr_out_buffer.resize(width * height);

    // Convert input region to int16
    std::ranges::copy(input.rows() | ranges::views::join, hr_in_buffer.begin());

    // Apply transform
    recursive_2d_wt(view_2d{ hr_in_buffer.begin(), width, height },
                    hr_out_buffer.begin(),
                    transform::haar_iwt<std::int16_t>,
                    num_iters);

    // Quantize results
    quantizer(hr_out_buffer,
              reinterpret_cast<std::int8_t*>(output),
              width,
              height,
              q_factor,
              q_alpha,
              q_beta,
              num_iters);
  };
}

void
image_encoder::set_coding_lzw(coding::lzw::code_point_size_t const code_size,
                              coding::lzw::options_t const options)
{
  byte_encoding_function_ =
    [lzw_encoder = coding::lzw::encoder{ code_size, options }](
      std::span<std::byte const> const input,
      std::vector<std::byte>& output) mutable
  { lzw_encoder(input, std::back_inserter(output)); };
}

void
image_encoder::operator()(view_2d<std::uint8_t const*> const input,
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
        transform_out_regions_.emplace_back(std::span{ transform_out_ }.subspan(
          buffer_offset, region_width * region_height));

        buffer_offset += region_width * region_height;
      }
    }
  }
  else
  {
    transform_in_regions_.push_back(input);
    transform_out_regions_.emplace_back(transform_out_);
  }

  // For each region, apply the transform
  for (auto const [in_region, out_region] :
       ranges::views::zip(transform_in_regions_, transform_out_regions_))
  {
    if (transform_function_)
    {
      transform_function_(in_region, out_region.data());
    }
    else
    {
      // No transform - input will be encoded as is
      std::ranges::copy(in_region.rows() | ranges::views::join,
                        reinterpret_cast<std::uint8_t*>(out_region.data()));
    }
  }

  byte_encoding_function_(std::as_bytes(std::span{ transform_out_ }), output);
}

} // namespace pa171
