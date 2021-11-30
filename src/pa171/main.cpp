#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>

#include <pa171/image_decoder.hpp>
#include <pa171/image_encoder.hpp>
#include <pa171/image_io.hpp>

auto
main(int const argc, char const* const* const argv) -> int
{
    auto [w, h] = std::array<std::size_t, 2u>{};
    auto const image_data =
        pa171::read_grayscale_image("data/image_gray.bmp", w, h);

    auto coder = pa171::image_encoder{};
    coder.set_coding_lzw(12u,
                         pa171::coding::lzw::dynamic_code_size |
                             pa171::coding::lzw::flush_full_dict);

    auto result = std::vector<std::byte>{};
    coder(w, h, image_data.get(), result);

    std::cout << "Input size: " << w * h << "\n";
    std::cout << "Output size: " << result.size() << "\n";
    std::cout << "Compression ratio: "
              << static_cast<float>(result.size()) / static_cast<float>(w * h)
              << "\n";

    auto decoder = pa171::image_decoder{};
    decoder.set_coding_lzw(12u,
                           pa171::coding::lzw::dynamic_code_size |
                               pa171::coding::lzw::flush_full_dict);

    decoder(result, w, h, image_data.get());
    pa171::write_grayscale_image_as_bmp(
        "data/image_gray_decoded.bmp", w, h, image_data.get());

    return EXIT_SUCCESS;
}
