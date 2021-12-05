#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

#include <algorithm>
#include <pa171/utils/numeric.hpp>
#include <pa171/utils/view_2d.hpp>

namespace pa171::quantization
{

template<std::integral SrcType, std::signed_integral HRType>
class haar_iwt
{
public:
  using src_type = SrcType;
  using hr_type = HRType;
  using lr_type = std::make_signed_t<src_type>;

  static_assert(sizeof(lr_type) <= sizeof(hr_type));

  template<std::ranges::input_range R, std::output_iterator<lr_type> O>
  requires std::same_as<std::ranges::range_value_t<R>, hr_type>
  auto operator()(R&& range,
                  O const result,
                  std::size_t const width,
                  std::size_t const height,
                  int const factor = 2u,
                  int const alpha = 2u,
                  int const beta = 0u,
                  std::optional<std::size_t> const levels = std::nullopt)
    -> std::pair<std::ranges::iterator_t<R>, O>
  {
    return (*this)(std::ranges::begin(range),
                   std::ranges::end(range),
                   result,
                   width,
                   height,
                   factor,
                   alpha,
                   beta,
                   levels);
  }

  template<std::input_iterator I,
           std::sentinel_for<I> S,
           std::output_iterator<lr_type> O>
  requires std::same_as<std::iter_value_t<I>, hr_type>
  auto operator()(I const first,
                  S const last,
                  O const result,
                  std::size_t const width,
                  std::size_t const height,
                  int const factor = 2u,
                  int const alpha = 2u,
                  int const beta = 0u,
                  std::optional<std::size_t> const levels = std::nullopt)
    -> std::pair<I, O>
  {
    return backtrack(
      first, last, result, width, height, factor, alpha, beta, levels);
  }

private:
  template<std::input_iterator I,
           std::sentinel_for<I> S,
           std::output_iterator<lr_type> O>
  requires std::same_as<std::iter_value_t<I>, hr_type>
  auto backtrack(I first,
                 S const last,
                 O result,
                 std::size_t const width,
                 std::size_t const height,
                 int factor = 2u,
                 int const alpha = 2u,
                 int const beta = 0u,
                 std::optional<std::size_t> levels = std::nullopt)
    -> std::pair<I, O>
  {
    // Factor must be at least 2 to keep the scale of data before transform
    factor = std::max(factor, 2);

    if ((width == 1u and height == 1u) or (levels and *levels == 0u))
    {
      constexpr auto shift =
        std::numeric_limits<lr_type>::min() -
        static_cast<lr_type>(std::numeric_limits<src_type>::min());

      auto const transform_result =
        std::ranges::transform(first,
                               last,
                               result,
                               [](hr_type const value)
                               { return static_cast<lr_type>(value + shift); });
      first = transform_result.in;
      result = transform_result.out;

      return { first, result };
    }

    auto const prev_width = ceil_div(width, std::size_t{ 2 });
    auto const prev_height = ceil_div(height, std::size_t{ 2 });

    for ([[maybe_unused]] auto const i :
         std::views::iota(std::size_t{ 0 }, (width / 2u) * (height / 2u)))
    {
      assert(first != last);

      // Quantize diagonal (second derivative - needs double factor)
      *result++ = static_cast<lr_type>(*first++ / (2 * factor));
    }

    for ([[maybe_unused]] auto const i : std::views::iota(
           std::size_t{ 0 },
           (width / 2u) * prev_height + prev_width * (height / 2u)))
    {
      assert(first != last);

      // Quantize vertical / horizontal
      *result++ = static_cast<lr_type>(*first++ / factor);
    }

    if (levels)
    {
      --*levels;
    }

    return backtrack(first,
                     last,
                     result,
                     prev_width,
                     prev_height,
                     static_cast<int>(ceil_div(static_cast<unsigned>(factor),
                                               static_cast<unsigned>(alpha))) -
                       beta,
                     alpha,
                     beta,
                     levels);
  }
};

template<std::integral SrcType, std::signed_integral HRType>
class inv_haar_iwt
{
public:
  using src_type = SrcType;
  using hr_type = HRType;
  using lr_type = std::make_signed_t<src_type>;

  static_assert(sizeof(lr_type) <= sizeof(hr_type));

  template<std::ranges::input_range R, std::output_iterator<hr_type> O>
  requires std::same_as<std::ranges::range_value_t<R>, lr_type>
  auto operator()(R&& range,
                  O const result,
                  std::size_t const width,
                  std::size_t const height,
                  int const factor = 2u,
                  int const alpha = 2u,
                  int const beta = 0u,
                  std::optional<std::size_t> const levels = std::nullopt)
    -> std::pair<std::ranges::iterator_t<R>, O>
  {
    return (*this)(std::ranges::begin(range),
                   std::ranges::end(range),
                   result,
                   width,
                   height,
                   factor,
                   alpha,
                   beta,
                   levels);
  }

  template<std::input_iterator I,
           std::sentinel_for<I> S,
           std::output_iterator<hr_type> O>
  requires std::same_as<std::iter_value_t<I>, lr_type>
  auto operator()(I const first,
                  S const last,
                  O const result,
                  std::size_t const width,
                  std::size_t const height,
                  int const factor = 2u,
                  int const alpha = 2u,
                  int const beta = 0u,
                  std::optional<std::size_t> const levels = std::nullopt)
    -> std::pair<I, O>
  {
    return backtrack(
      first, last, result, width, height, factor, alpha, beta, levels);
  }

private:
  template<std::input_iterator I,
           std::sentinel_for<I> S,
           std::output_iterator<hr_type> O>
  requires std::same_as<std::iter_value_t<I>, lr_type>
  auto backtrack(I first,
                 S const last,
                 O result,
                 std::size_t const width,
                 std::size_t const height,
                 int factor = 2u,
                 int const alpha = 2u,
                 int const beta = 0u,
                 std::optional<std::size_t> levels = std::nullopt)
    -> std::pair<I, O>
  {
    factor = std::max(factor, 2);

    if ((width == 1u and height == 1u) or (levels and *levels == 0u))
    {
      constexpr auto inv_shift =
        static_cast<lr_type>(std::numeric_limits<src_type>::min()) -
        std::numeric_limits<lr_type>::min();

      auto const transform_result =
        std::ranges::transform(first,
                               last,
                               result,
                               [](lr_type const value) {
                                 return static_cast<hr_type>(value) + inv_shift;
                               });
      first = transform_result.in;
      result = transform_result.out;

      return { first, result };
    }

    auto const prev_width = ceil_div(width, std::size_t{ 2 });
    auto const prev_height = ceil_div(height, std::size_t{ 2 });

    for ([[maybe_unused]] auto const i :
         std::views::iota(std::size_t{ 0 }, (width / 2u) * (height / 2u)))
    {
      assert(first != last);

      *result++ = static_cast<hr_type>(*first++) * 2 * factor;
    }

    for ([[maybe_unused]] auto const i : std::views::iota(
           std::size_t{ 0 },
           (width / 2u) * prev_height + prev_width * (height / 2u)))
    {
      assert(first != last);

      *result++ = static_cast<hr_type>(*first++) * factor;
    }

    if (levels)
    {
      --*levels;
    }

    return backtrack(first,
                     last,
                     result,
                     prev_width,
                     prev_height,
                     static_cast<int>(ceil_div(static_cast<unsigned>(factor),
                                               static_cast<unsigned>(alpha))) -
                       beta,
                     alpha,
                     beta,
                     levels);
  }
};

} // namespace pa171::quantization
