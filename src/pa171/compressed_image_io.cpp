#include <pa171/compressed_image_io.hpp>

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace pa171
{

namespace
{

struct compressed_image_header
{
  static constexpr auto magic = std::string_view{ "PA171_456394" };

  std::array<char, magic.size()> magic_value;
  compression_options options;
  std::uint32_t width;
  std::uint32_t height;
  std::uint64_t payload_length;

  void set_magic() noexcept { std::ranges::copy(magic, magic_value.begin()); }

  [[nodiscard]] auto is_valid() const noexcept -> bool
  {
    return std::string_view{ magic_value.data(), magic_value.size() } == magic;
  }
};

static_assert(std::is_trivially_copyable_v<compressed_image_header>);

} // namespace

[[nodiscard]] auto
header_size() noexcept -> std::size_t
{
  return sizeof(compressed_image_header);
}

void
read_compressed_image(std::filesystem::path const& path,
                      compression_options& options,
                      std::size_t& width,
                      std::size_t& height,
                      std::vector<std::byte>& data)
{
  auto file = std::ifstream{ path, std::ios::binary };

  // Read and validate header
  auto header = compressed_image_header{};

  if (not file.read(
        reinterpret_cast<char*>(&header),
        static_cast<std::streamsize>(sizeof(compressed_image_header))))
  {
    throw std::runtime_error{ "Failed to read image header" };
  }

  if (not header.is_valid())
  {
    throw std::runtime_error{ "Invalid image header" };
  }

  options = header.options;
  width = header.width;
  height = header.height;

  // Read payload
  data.resize(header.payload_length);

  if (not file.read(reinterpret_cast<char*>(data.data()),
                    static_cast<std::streamsize>(data.size())))
  {
    throw std::runtime_error{ "Failed to read image payload" };
  }
}

void
write_compressed_image(std::filesystem::path const& path,
                       compression_options const& options,
                       std::size_t width,
                       std::size_t height,
                       std::span<std::byte const> data)
{
  auto file = std::ofstream{ path, std::ios::binary };

  // Write header
  auto header = compressed_image_header{};
  header.options = options;
  header.width = static_cast<std::uint32_t>(width);
  header.height = static_cast<std::uint32_t>(height);
  header.payload_length = data.size();
  header.set_magic();

  if (not file.write(
        reinterpret_cast<char*>(&header),
        static_cast<std::streamsize>(sizeof(compressed_image_header))))
  {
    throw std::runtime_error{ "Failed to write image header" };
  }

  // Write payload
  if (not file.write(reinterpret_cast<char const*>(data.data()),
                     static_cast<std::streamsize>(data.size())))
  {
    throw std::runtime_error{ "Failed to write image payload" };
  }
}

} // namespace pa171
