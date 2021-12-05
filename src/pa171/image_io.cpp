#include <pa171/image_io.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

namespace pa171
{

void
image_data_deleter::operator()(pointer const ptr)
{
  ::stbi_image_free(ptr);
}

auto
read_grayscale_image(std::filesystem::path const& path,
                     size_t& out_width,
                     size_t& out_height) -> image_data_pointer
{
  auto width = int{};
  auto height = int{};
  auto channels = int{};

  auto ptr = image_data_pointer{
    reinterpret_cast<std::uint8_t*>(
      ::stbi_load(path.c_str(), &width, &height, &channels, ::STBI_grey)),
  };

  out_width = static_cast<std::size_t>(width);
  out_height = static_cast<std::size_t>(height);

  return ptr;
}

void
write_grayscale_image_as_bmp(std::filesystem::path const& path,
                             std::size_t const width,
                             std::size_t const height,
                             std::uint8_t const* const data)
{
  ::stbi_write_bmp(path.c_str(),
                   static_cast<int>(width),
                   static_cast<int>(height),
                   ::STBI_grey,
                   data);
}

} // namespace pa171
