#pragma once

#include <cstddef>
#include <optional>
#include <variant>

#include <range/v3/functional/overload.hpp>

#include <pa171/coding/lzw_base.hpp>

namespace pa171
{

struct compression_options
{
  struct transform_haar_iwt
  {
    std::optional<std::size_t> num_iters = std::nullopt;
    std::int16_t q_factor = 32;
    std::int16_t q_alpha = 8;
    std::int16_t q_beta = 0;
  };

  struct coding_lzw
  {
    coding::lzw::code_point_size_t code_size = coding::lzw::default_code_size;
    coding::lzw::options_t options = coding::lzw::default_options;
  };

  std::optional<std::uint32_t> region_size = std::nullopt;
  std::variant<std::monostate, transform_haar_iwt> transform = {};
  std::variant<coding_lzw> coding = coding_lzw{};
};

template<typename Configurable>
void
apply_options(compression_options const& options, Configurable& configurable)
{
  if (options.region_size)
  {
    configurable.set_region_size(*options.region_size);
  }

  std::visit(ranges::overload(
               [&](compression_options::transform_haar_iwt const& haar_iwt)
               {
                 configurable.set_transform_haar_iwt(haar_iwt.num_iters,
                                                     haar_iwt.q_factor,
                                                     haar_iwt.q_alpha,
                                                     haar_iwt.q_beta);
               },
               [](std::monostate) {}),
             options.transform);

  std::visit(ranges::overload(
               [&](compression_options::coding_lzw const& lzw)
               { configurable.set_coding_lzw(lzw.code_size, lzw.options); }),
             options.coding);
}

} // namespace pa171
