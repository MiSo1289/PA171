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
#include <pa171/image_encoder.hpp>
#include <pa171/image_io.hpp>

auto
main(int const argc, char const* const* const argv) -> int
{
  try
  {
    constexpr auto max_loss_level = 64u;
    constexpr auto q_factor_per_loss_level = 2u;
    constexpr auto region_size = 32u;

    // Parse arguments
    auto show_help = false;
    auto show_stats = false;
    auto loss_level = 8u;
    auto in_path = std::filesystem::path{};
    auto out_path = std::filesystem::path{};

    auto const parser =
      lyra::cli_parser{}
        .add_argument(lyra::help(show_help))
        .add_argument(lyra::opt(show_stats)
                        .name("-s")
                        .name("--stats")
                        .help("Display compression stats"))
        .add_argument(
          lyra::opt(loss_level, "level")
            .name("-l")
            .name("--loss-level")
            .help(fmt::format(
              "Compression level. 0 = lossless; default = {}; max = {}",
              loss_level,
              max_loss_level)))
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

    // Build compression options from arguments
    auto options = pa171::compression_options{};

    if (loss_level > 0u)
    {
      auto& haar_iwt_options =
        options.transform
          .emplace<pa171::compression_options::transform_haar_iwt>();
      haar_iwt_options.q_factor = static_cast<int>(
        q_factor_per_loss_level * std::min(loss_level, max_loss_level));

      options.region_size = region_size;
    }
    else
    {
      options.transform = {};
      options.region_size = std::nullopt;
    }

    // Read the input image
    auto width = std::size_t{};
    auto height = std::size_t{};
    auto const image_data = pa171::read_grayscale_image(in_path, width, height);

    // Encode the image
    auto encoder = pa171::image_encoder{};
    pa171::apply_options(options, encoder);

    auto compressed_data = std::vector<std::byte>();
    encoder(pa171::view_2d{ image_data.get(), width, height }, compressed_data);

    if (show_stats)
    {
      auto const original_size = width * height;
      auto const compressed_size = compressed_data.size();
      auto const header_size = pa171::header_size();
      auto const compression_ratio =
        static_cast<float>(compressed_size + header_size) /
        static_cast<float>(original_size);

      fmt::print("Original size: {}B\n"
                 "Compressed size: {}B (+ {}B header)\n"
                 "Compression ratio (including header): {}\n",
                 original_size,
                 compressed_size,
                 header_size,
                 compression_ratio);
      std::fflush(stdout);
    }

    // Write the encoded image
    pa171::write_compressed_image(
      out_path, options, width, height, compressed_data);
  }
  catch (std::exception const& error)
  {
    fmt::print(stderr, "{}\n", error.what());

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
