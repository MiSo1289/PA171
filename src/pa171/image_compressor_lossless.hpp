#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <vector>

#include <pa171/coding/lzw_base.hpp>
#include <pa171/coding/lzw_encoder.hpp>
#include <pa171/transform/wavelet.hpp>

namespace pa171
{

class image_compressor_lossless
{
  public:
  void set_transform_haar_wavelet()
  {
    using full_2d_lwt_type = transform::full_2d_wavelet_transform<std::uint8_t>;

    transform_function_ = [full_2d_lwt = full_2d_lwt_type{}](
                            std::uint8_t const* const data,
                            std::size_t const width,
                            std::size_t const height,
                            std::uint8_t* output) mutable
    {
      full_2d_lwt(data,
                  width,
                  height,
                  output,
                  transform::haar_iwt<std::uint8_t>);
    };
  }

  void set_transform_db4_wavelet()
  {
    using full_2d_lwt_type = transform::full_2d_wavelet_transform<std::uint8_t>;

    transform_function_ = [full_2d_lwt = full_2d_lwt_type{}](
                            std::uint8_t const* const data,
                            std::size_t const width,
                            std::size_t const height,
                            std::uint8_t* output) mutable
    {
      full_2d_lwt(data,
                  width,
                  height,
                  output,
                  transform::db4_iwt<std::uint8_t>);
    };
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

  void operator()(std::uint8_t const* const data,
                  std::size_t const width,
                  std::size_t const height,
                  std::vector<std::byte>& output)
  {
    auto to_encode = std::span<std::byte const>{};

    if (transform_function_)
    {
      transform_result_.resize(width * height);
      transform_function_(data, width, height, transform_result_.data());

      to_encode = std::as_bytes(std::span{ transform_result_ });
    }
    else
    {
      to_encode = std::as_bytes(std::span{ data, data + width * height });
    }

    byte_encoding_function_(to_encode, output);
  }

  private:
  using transform_function_type = void(std::uint8_t const* data,
                                       std::size_t width,
                                       std::size_t height,
                                       std::uint8_t* output);
  using byte_encoding_function_type = void(std::span<std::byte const> input,
                                           std::vector<std::byte>& output);

  std::function<transform_function_type> transform_function_;
  std::function<byte_encoding_function_type> byte_encoding_function_;
  std::vector<std::uint8_t> transform_result_;
};

} // namespace pa171
