#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <ranges>
#include <span>
#include <vector>

#include <pa171/coding/lzw_base.hpp>
#include <pa171/coding/lzw_decoder.hpp>
#include <pa171/transform/wavelet.hpp>

namespace pa171
{

class image_decoder
{
public:
    void set_transform_haar_wavelet()
    {
        using full_2d_ilwt_type =
            transform::full_2d_inverse_wavelet_transform<std::uint8_t>;

        inverse_transform_function_ = [full_2d_ilwt = full_2d_ilwt_type{}](
                                          std::uint8_t* const data,
                                          std::size_t const width,
                                          std::size_t const height) mutable
        {
            full_2d_ilwt(data,
                         width,
                         height,
                         transform::inverse_haar_integer_wavelet_transform<
                             std::uint8_t>);
        };
    }

    void set_coding_lzw(
        coding::lzw::code_point_size_t const code_size =
            coding::lzw::default_code_size,
        coding::lzw::options_t const options = coding::lzw::default_options)
    {
        byte_decoding_function_ =
            [lzw_encoder = coding::lzw::decoder{ code_size, options }](
                std::span<std::byte const> const input,
                std::vector<std::byte>& output) mutable
        { lzw_encoder(input, std::back_inserter(output)); };
    }

    void operator()(std::span<std::byte const> input,
                    std::size_t width,
                    std::size_t height,
                    std::uint8_t* output)
    {
        byte_decode_buffer_.clear();
        byte_decoding_function_(input, byte_decode_buffer_);

        if (byte_decode_buffer_.size() != width * height)
        {
            throw std::runtime_error{ "Decoded size mismatch" };
        }

        std::ranges::copy_n(
            reinterpret_cast<std::uint8_t const*>(byte_decode_buffer_.data()),
            static_cast<std::ptrdiff_t>(width * height),
            output);

        if (inverse_transform_function_)
        {
            inverse_transform_function_(output, width, height);
        }
    }

private:
    using inverse_transform_function_type = void(std::uint8_t* data,
                                                 std::size_t width,
                                                 std::size_t height);
    using byte_decoding_function_type = void(std::span<std::byte const> input,
                                             std::vector<std::byte>& output);

    std::function<inverse_transform_function_type> inverse_transform_function_;
    std::function<byte_decoding_function_type> byte_decoding_function_;
    std::vector<std::byte> byte_decode_buffer_;
};

} // namespace pa171
