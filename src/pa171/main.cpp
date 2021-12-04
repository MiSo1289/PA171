#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>

#include <pa171/image_decoder.hpp>
#include <pa171/image_encoder.hpp>
#include <pa171/image_io.hpp>

// garfield: 0.364629 with LZW-12
//           0.382525 with LZW-16
//           0.890721 with Haar and LZW-12
//           0.934100 with Haar and LZW-16
//           1.034360 with DB4 and LZW-12
//           1.065290 with DB4 and LZW-16
//
// doge: 0.496941 with LZW-16
//       0.533602 with Haar and LZW-16
//       0.557631 with Haar and LZW-12
//       0.576175 with DB4 and LZW-16
//       0.584579 with DB4 and LZW-12
//       0.601369 with LZW-12
//
// forest: 1.06102 with LZW-16
//         1.15015 with Haar and LZW-12
//         1.18188 with Haar and LZW-16
//         1.20876 with DB4 and LZW-16
//         1.21778 with LZW-12
//         1.22264 with DB4 and LZW-12

auto
main(int const argc, char const* const* const argv) -> int
{
  auto [w, h] = std::array<std::size_t, 2u>{};
  auto const image_data = pa171::read_grayscale_image("data/garfield.bmp", w, h);

  auto coder = pa171::image_encoder{};
  coder.set_region_size(32u);
  coder.set_transform_haar_iwt(std::nullopt, 32, 8);
  coder.set_coding_lzw(16u,
                       pa171::coding::lzw::dynamic_code_size |
                         pa171::coding::lzw::flush_full_dict);

  auto result = std::vector<std::byte>{};
  coder(pa171::view_2d{ image_data.get(), w, h }, result);

  std::cout << "Input size: " << w * h << "\n";
  std::cout << "Output size: " << result.size() << "\n";
  std::cout << "Compression ratio: "
            << static_cast<float>(result.size()) / static_cast<float>(w * h)
            << "\n";

  std::fill_n(image_data.get(), w * h, 0u);

  auto decoder = pa171::image_decoder{};
  decoder.set_region_size(32u);
  decoder.set_transform_haar_iwt(std::nullopt, 32, 8);
  decoder.set_coding_lzw(16u,
                         pa171::coding::lzw::dynamic_code_size |
                           pa171::coding::lzw::flush_full_dict);

  decoder(result, pa171::view_2d{ image_data.get(), w, h });
  pa171::write_grayscale_image_as_bmp(
    "data/garfield_decoded.bmp", w, h, image_data.get());

  return EXIT_SUCCESS;
}
