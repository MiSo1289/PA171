#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include <catch2/catch.hpp>

#include <pa171/coding/lzw_decoder.hpp>
#include <pa171/coding/lzw_encoder.hpp>

using namespace std::literals;

namespace lzw = pa171::coding::lzw;

TEST_CASE("LZW encode / decode")
{

    constexpr auto input = "TOBEORNOTTOBEORTOBEORNOT"sv;

    auto encoder = lzw::encoder{
        12u,
        lzw::dynamic_code_size | lzw::flush_full_dict,
    };

    auto encoded = std::vector<std::byte>{};
    encoder(std::as_bytes(std::span{input}), std::back_inserter(encoded));

    auto decoder = lzw::decoder{
        12u,
        lzw::dynamic_code_size | lzw::flush_full_dict,
    };

    auto decoded = std::vector<std::byte>{};
    decoder(encoded, std::back_inserter(decoded));

}
