#pragma once

#include <cstdint>

namespace pa171::coding::lzw
{

using code_point_size_t = std::uint32_t;
using options_t = unsigned;

enum : options_t
{
    dynamic_code_size = 1u << 0u,
    flush_full_dict = 1u << 1u,
};

} // namespace pa171::coding::lzw
