#pragma once

#include <functional>
#include <iterator>
#include <ranges>

#include <range/v3/view/concat.hpp>
#include <range/v3/view/counted.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/stride.hpp>
#include <range/v3/view/subrange.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/unbounded.hpp>

namespace pa171
{

template<std::random_access_iterator I>
class view_2d
{
public:
  template<std::convertible_to<I> I2>
  view_2d(view_2d<I2> const& other)
    : view_2d{ other.base(), other.width(), other.height(), other.row_stride() }
  {
  }

  view_2d(I const base, std::size_t const width, std::size_t const height)
    : view_2d{ base, width, height, width }
  {
  }

  view_2d(I const base,
          std::size_t const width,
          std::size_t const height,
          std::size_t const row_stride)
    : base_{ base }
    , width_{ width }
    , height_{ height }
    , row_stride_{ row_stride }
  {
  }

  [[nodiscard]] auto row(std::size_t const i) const
    -> std::ranges::random_access_range auto
  {
    return ranges::views::counted(base_ + i * row_stride_, width_);
  }

  [[nodiscard]] auto col(std::size_t const j) const
    -> std::ranges::random_access_range auto
  {
    return ranges::views::unbounded(base_ + j) |
           ranges::views::stride(row_stride_) | ranges::views::take(height_);
  };

  [[nodiscard]] auto block(std::size_t const x,
                           std::size_t const y,
                           std::size_t const width,
                           std::size_t const height) const -> view_2d
  {
    return view_2d{
      base_ + y * row_stride_ + x,
      width,
      height,
      row_stride_,
    };
  }

  [[nodiscard]] auto rows() const
  {
    return ranges::views::iota(std::size_t{ 0 }, height_) |
           ranges::views::transform(std::bind_front(&view_2d::row, this));
  }

  [[nodiscard]] auto cols() const
  {
    return ranges::views::iota(std::size_t{ 0 }, width_) |
           ranges::views::transform(std::bind_front(&view_2d::col, this));
  }

  [[nodiscard]] auto base() const noexcept -> I { return base_; }

  [[nodiscard]] auto width() const noexcept -> std::size_t { return width_; }

  [[nodiscard]] auto height() const noexcept -> std::size_t { return height_; }

  [[nodiscard]] auto row_stride() const noexcept -> std::size_t
  {
    return row_stride_;
  }

private:
  I base_;
  std::size_t width_;
  std::size_t height_;
  std::size_t row_stride_;
};

} // namespace pa171
