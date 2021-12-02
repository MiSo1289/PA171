#pragma once

#include <concepts>

namespace pa171
{

template<std::unsigned_integral T>
[[nodiscard]] constexpr auto
ceil_div(T const numerator, T const denominator) noexcept -> T
{
    return static_cast<T>((numerator + denominator - T{ 1 }) / denominator);
}

template<std::integral T>
[[nodiscard]] constexpr auto
wrap_mod(T const number, T const mod) noexcept -> T
{
    return static_cast<T>((number % mod + mod) % mod);
}

} // namespace pa171
