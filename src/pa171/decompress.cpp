#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <lyra/lyra.hpp>

#include <pa171/compressed_image_io.hpp>
#include <pa171/compression_options.hpp>
#include <pa171/image_decoder.hpp>
#include <pa171/image_io.hpp>

auto
main(int const argc, char const* const* const argv) -> int
{
  try
  {
    // Parse arguments
    auto show_help = false;
    auto in_path = std::filesystem::path{};
    auto out_path = std::filesystem::path{};

    auto const parser =
      lyra::cli_parser{}
        .add_argument(lyra::help(show_help))
        .add_argument(
          lyra::arg(in_path, "in").help("Input compressed image path"))
        .add_argument(lyra::arg(out_path, "out").help("Output BMP image path"));

    if (auto const parse_result = parser.parse(lyra::args(argc, argv));
        not parse_result)
    {
      fmt::print(stderr, "{}\n", parse_result.errorMessage());
      fmt::print("See --help for correct usage");

      return EXIT_FAILURE;
    }

    if (show_help)
    {
      // Display usage and exit
      fmt::print("{}", parser);

      return EXIT_SUCCESS;
    }

    // Read the compressed image
    auto options = pa171::compression_options{};
    auto width = std::size_t{};
    auto height = std::size_t{};
    auto compressed_data = std::vector<std::byte>{};
    pa171::read_compressed_image(
      in_path, options, width, height, compressed_data);

    // Decode the image
    auto decoder = pa171::image_decoder{};
    pa171::apply_options(options, decoder);

    auto decoded_image = std::vector<std::uint8_t>(width * height);
    decoder(compressed_data,
            pa171::view_2d{ decoded_image.data(), width, height });

    // Write the decoded image
    pa171::write_grayscale_image_as_bmp(
      out_path, width, height, decoded_image.data());
  }
  catch (std::exception const& error)
  {
    fmt::print(stderr, "{}\n", error.what());

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
