#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

#include <pa171/compression_options.hpp>

namespace pa171
{

[[nodiscard]] auto header_size() noexcept -> std::size_t;

void read_compressed_image(std::filesystem::path const& path,
                           compression_options& options,
                           std::size_t& width,
                           std::size_t& height,
                           std::vector<std::byte>& data);

void write_compressed_image(std::filesystem::path const& path,
                            compression_options const& options,
                            std::size_t width,
                            std::size_t height,
                            std::span<std::byte const> data);

} // namespace pa171
