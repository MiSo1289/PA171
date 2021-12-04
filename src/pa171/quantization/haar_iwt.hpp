#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include <pa171/utils/numeric.hpp>
#include <pa171/utils/view_2d.hpp>

namespace pa171::quantization
{

class haar_iwt
{
public:
  void operator()(std::int32_t* const data,
                  std::size_t const width,
                  std::size_t const height,
                  int const factor = 2u,
                  int const alpha = 2u,
                  int const beta = 0u,
                  std::optional<std::size_t> const levels = std::nullopt)
  {
    backtrack(data, width, height, factor, alpha, beta, levels);
  }

private:
  void backtrack(std::int32_t* const data,
                 std::size_t const width,
                 std::size_t const height,
                 int factor = 2u,
                 int const alpha = 2u,
                 int const beta = 0u,
                 std::optional<std::size_t> levels = std::nullopt)
  {
    // Factor must be at least 2 to keep the scale of data before transform
    factor = std::max(factor, 2);

    if ((width == 1u and height == 1u) or (levels and *levels == 0u))
    {
      for (auto& elem : std::span{data, width * height})
      {
        // Approximations should be 0 to 255, shift them to -128 to 127
        elem -= 128;
      }
      return;
    }

    auto const prev_width = ceil_div(width, std::size_t{ 2 });
    auto const prev_height = ceil_div(height, std::size_t{ 2 });

    if (levels)
    {
      --*levels;
    }

    backtrack(data + (width * height) - prev_width * prev_height,
              prev_width,
              prev_height,
              static_cast<int>(ceil_div(static_cast<unsigned>(factor),
                                        static_cast<unsigned>(alpha))) -
                beta,
              alpha,
              beta,
              levels);

    for (auto& elem : std::span{ data, (width / 2u) * (height / 2u) })
    {
      // Quantize diagonal (second derivative - needs double factor)
      elem /= 2 * factor;
    }

    for (auto& elem : std::span{ data, width * height }.subspan(
           (width / 2u) * (height / 2u),
           (width / 2u) * prev_height + prev_width * (height / 2u)))
    {
      // Quantize vertical / horizontal
      elem /= factor;
    }
  }
};

class inv_haar_iwt
{
public:
  void operator()(std::int32_t* const data,
                  std::size_t const width,
                  std::size_t const height,
                  int const factor = 2u,
                  int const alpha = 2u,
                  int const beta = 0u,
                  std::optional<std::size_t> const levels = std::nullopt)
  {
    backtrack(data, width, height, factor, alpha, beta, levels);
  }

private:
  void backtrack(std::int32_t* const data,
                 std::size_t const width,
                 std::size_t const height,
                 int factor = 2u,
                 int const alpha = 2u,
                 int const beta = 0u,
                 std::optional<std::size_t> levels = std::nullopt)
  {
    // Factor must be at least 2 to keep the scale of data before transform
    factor = std::max(factor, 2);

    if ((width == 1u and height == 1u) or (levels and *levels == 0u))
    {
      for (auto& elem : std::span{data, width * height})
      {
        elem += 128;
      }
      return;
    }

    auto const prev_width = ceil_div(width, std::size_t{ 2 });
    auto const prev_height = ceil_div(height, std::size_t{ 2 });

    if (levels)
    {
      --*levels;
    }

    backtrack(data + (width * height) - prev_width * prev_height,
              prev_width,
              prev_height,
              static_cast<int>(ceil_div(static_cast<unsigned>(factor),
                                        static_cast<unsigned>(alpha))) -
                beta,
              alpha,
              beta,
              levels);

    for (auto& elem : std::span{ data, (width / 2u) * (height / 2u) })
    {
      elem *= 2 * factor;
    }

    for (auto& elem : std::span{ data, width * height }.subspan(
           (width / 2u) * (height / 2u),
           (width / 2u) * prev_height + prev_width * (height / 2u)))
    {
      elem *= factor;
    }
  }
};

} // namespace pa171::quantization
