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

namespace pa171
{

class image_encoder
{
public:
    void set_coding_lzw(coding::lzw::code_point_size_t code_size = 12u,
                        coding::lzw::options_t options = {})
    {
        byte_encoding_function_ =
            [lzw_encoder = coding::lzw::encoder{ code_size, options }](
                std::span<std::byte const> const input,
                std::vector<std::byte>& output) mutable
        { lzw_encoder(input, std::back_inserter(output)); };
    }

    void operator()(std::size_t width,
                    std::size_t height,
                    std::uint8_t const* data,
                    std::vector<std::byte>& output)
    {
        auto data_bytes =
            std::as_bytes(std::span{ data, data + width * height });
        byte_encoding_function_(data_bytes, output);
    }

private:
    using byte_encoding_function_type = void(std::span<std::byte const> input,
                                      std::vector<std::byte>& output);

    std::function<byte_encoding_function_type> byte_encoding_function_;
};

} // namespace pa171
