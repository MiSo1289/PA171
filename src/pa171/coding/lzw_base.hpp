#pragma once

#include <cstdint>

namespace pa171::coding::lzw
{

using code_point_size_t = std::uint32_t;

static constexpr auto default_code_size = code_point_size_t{12};

using options_t = unsigned;

enum : options_t
{
    dynamic_code_size = 1u << 0u,
    flush_full_dict = 1u << 1u,

    default_options = dynamic_code_size | flush_full_dict,
};

} // namespace pa171::coding::lzw
