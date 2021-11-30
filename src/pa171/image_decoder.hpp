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

namespace pa171
{

class image_decoder
{
public:
    void set_coding_lzw(coding::lzw::code_point_size_t code_size = 12u,
                        coding::lzw::options_t options = {})
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

        auto data_pointer =
            reinterpret_cast<std::uint8_t const*>(byte_decode_buffer_.data());
        std::ranges::copy(data_pointer, data_pointer + width * height, output);
    }

private:
    using byte_decoding_function_type = void(std::span<std::byte const> input,
                                             std::vector<std::byte>& output);

    std::function<byte_decoding_function_type> byte_decoding_function_;
    std::vector<std::byte> byte_decode_buffer_;
};

} // namespace pa171
