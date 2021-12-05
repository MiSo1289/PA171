#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <concepts>
#include <iostream>
#include <iterator>
#include <numbers>
#include <optional>
#include <ranges>

#include <range/v3/view/concat.hpp>
#include <range/v3/view/counted.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/stride.hpp>
#include <range/v3/view/subrange.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/unbounded.hpp>

#include <pa171/utils/numeric.hpp>
#include <pa171/utils/view_2d.hpp>

namespace pa171::transform
{

template<typename Step, typename T, std::size_t order>
concept lifting_step = std::is_arithmetic_v<T> and
  requires(Step const& step, T const value)
{
  ([]<std::size_t... indices>(Step const& step,
                              T const value,
                              std::index_sequence<indices...>) requires requires
   {
     {
       step((indices, value)...)
       } -> std::same_as<T>;
   } {}(step, value, std::make_index_sequence<order>{}));
};

template<typename T, std::size_t order, lifting_step<T, order>... Steps>
class inverse_lifting_wavelet_transform
{
public:
  constexpr inverse_lifting_wavelet_transform(
    std::optional<T> const approx_norm,
    std::optional<T> const detail_norm,
    Steps const&... steps)
    : steps_{ steps... }
    , approx_norm_{ approx_norm }
    , detail_norm_{ detail_norm }
  {
  }

  template<std::ranges::input_range RA,
           std::ranges::input_range RD,
           std::output_iterator<T> O>
  requires std::forward_iterator<O>
  void operator()(RA&& approximation_range,
                  RD&& detail_range,
                  O const result) const
  {
    (*this)(std::ranges::begin(approximation_range),
            std::ranges::end(approximation_range),
            std::ranges::begin(detail_range),
            std::ranges::end(detail_range),
            result);
  }

  template<std::input_iterator IA,
           std::sentinel_for<IA> SA,
           std::input_iterator ID,
           std::sentinel_for<ID> SD,
           std::output_iterator<T> O>
  requires std::forward_iterator<O>
  void operator()(IA approximation_first,
                  SA const approximation_last,
                  ID detail_first,
                  SD const detail_last,
                  O const result) const
  {
    auto even_output =
      ranges::views::unbounded(result) | ranges::views::stride(2);
    auto odd_output =
      ranges::views::unbounded(std::next(result)) | ranges::views::stride(2);

    (*this)(approximation_first,
            approximation_last,
            detail_first,
            detail_last,
            std::ranges::begin(even_output),
            std::ranges::begin(odd_output));
  }

  template<std::ranges::input_range RA,
           std::ranges::input_range RD,
           std::output_iterator<T> O1,
           std::output_iterator<T> O2>
  auto operator()(RA&& approximation_range,
                  RD&& detail_range,
                  O1 const even_result,
                  O2 const odd_result) const -> std::pair<O1, O2>
  {
    return (*this)(std::ranges::begin(approximation_range),
                   std::ranges::end(approximation_range),
                   std::ranges::begin(detail_range),
                   std::ranges::end(detail_range),
                   even_result,
                   odd_result);
  }

  template<std::input_iterator IA,
           std::sentinel_for<IA> SA,
           std::input_iterator ID,
           std::sentinel_for<ID> SD,
           std::output_iterator<T> O1,
           std::output_iterator<T> O2>
  auto operator()(IA approximation_first,
                  SA const approximation_last,
                  ID detail_first,
                  SD const detail_last,
                  O1 even_result,
                  O2 odd_result) const -> std::pair<O1, O2>
  {
    auto const even_last =
      std::ranges::copy(approximation_first, approximation_last, even_result)
        .out;
    auto const odd_last =
      std::ranges::copy(detail_first, detail_last, odd_result).out;

    auto const n_even = std::ptrdiff_t{
      std::distance(even_result, even_last),
    };
    auto const n_odd = std::ptrdiff_t{
      std::distance(odd_result, odd_last),
    };

    if (approx_norm_)
    {
      for (auto const i : std::views::iota(std::ptrdiff_t{ 0 }, n_even))
      {
        even_result[i] *= *approx_norm_;
      }
    }

    if (detail_norm_)
    {
      for (auto const i : std::views::iota(std::ptrdiff_t{ 0 }, n_odd))
      {
        odd_result[i] *= *detail_norm_;
      }
    }

    ([&]<std::size_t... step_indices>(std::index_sequence<step_indices...>)
     {
       (
         [&]<std::size_t step_index>(
           std::integral_constant<std::size_t, step_index>)
         {
           constexpr auto reversed_step_index =
             sizeof...(Steps) - step_index - 1u;
           if constexpr (reversed_step_index % 2 == 0)
           {
             auto const& predict = std::get<reversed_step_index>(steps_);

             for (auto const i : std::views::iota(std::ptrdiff_t{ 0 }, n_odd))
             {
               ([&]<std::size_t... offsets>(std::index_sequence<offsets...>) {
                 odd_result[i] += predict(even_result[wrap_mod(
                   i - static_cast<std::ptrdiff_t>(offsets), n_even)]...);
               }(std::make_index_sequence<order>{}));
             }
           }
           else
           {
             auto const& update = std::get<reversed_step_index>(steps_);

             for (auto const i : std::views::iota(std::ptrdiff_t{ 0 }, n_even))
             {
               ([&]<std::size_t... offsets>(std::index_sequence<offsets...>) {
                 even_result[i] -= update(odd_result[wrap_mod(
                   i + static_cast<std::ptrdiff_t>(offsets), n_odd)]...);
               }(std::make_index_sequence<order>{}));
             }
           }
         }(std::integral_constant<std::size_t, step_indices>{}),
         ...);
     }(std::make_index_sequence<sizeof...(Steps)>{}));

    return { even_last, odd_last };
  }

private:
  std::tuple<Steps...> steps_;
  std::optional<T> approx_norm_ = std::nullopt;
  std::optional<T> detail_norm_ = std::nullopt;
};

template<typename T, std::size_t order, lifting_step<T, order>... Steps>
class lifting_wavelet_transform
{
public:
  constexpr lifting_wavelet_transform() = default;

  constexpr explicit lifting_wavelet_transform(Steps const&... steps) requires(
    sizeof...(Steps) > 0u)
    : steps_{ steps... }
  {
  }

  template<lifting_step<T, order> P>
  [[nodiscard]] constexpr auto predict_step(P const& predict) const
    -> lifting_wavelet_transform<T, order, Steps..., P>
  requires(sizeof...(Steps) % 2 == 0)
  {
    return std::apply(
      [&](Steps const&... steps)
      {
        return lifting_wavelet_transform<T, order, Steps..., P>{
          steps...,
          predict,
        };
      },
      steps_);
  }

  template<lifting_step<T, order> P>
  [[nodiscard]] constexpr auto predict_step(P const& predict) const
    requires(sizeof...(Steps) % 2 == 1)
  {
    return update_step([](auto const&...) { return T{ 0 }; })
      .predict_step(predict);
  }

  template<lifting_step<T, order> U>
  [[nodiscard]] constexpr auto update_step(U const& update) const
    -> lifting_wavelet_transform<T, order, Steps..., U>
  requires(sizeof...(Steps) % 2 == 1)
  {
    return std::apply(
      [&](Steps const&... steps)
      {
        return lifting_wavelet_transform<T, order, Steps..., U>{
          steps...,
          update,
        };
      },
      steps_);
  }

  template<lifting_step<T, order> U>
  [[nodiscard]] constexpr auto update_step(U const& update) const
    requires(sizeof...(Steps) % 2 == 0)
  {
    return predict_step([](auto const&...) { return T{ 0 }; })
      .update_step(update);
  }

  [[nodiscard]] constexpr auto normalize_approx(T const normalizer)
    -> lifting_wavelet_transform&
  {
    approx_norm_ = normalizer;
    return *this;
  }

  [[nodiscard]] constexpr auto normalize_detail(T const normalizer)
    -> lifting_wavelet_transform&
  {
    detail_norm_ = normalizer;
    return *this;
  }

  [[nodiscard]] constexpr auto inverse() const
    -> inverse_lifting_wavelet_transform<T, order, Steps...>
  {
    return std::apply(
      [&](Steps const&... steps)
      {
        return inverse_lifting_wavelet_transform<T, order, Steps...>{
          approx_norm_,
          detail_norm_,
          steps...,
        };
      },
      steps_);
  }

  template<std::ranges::forward_range R,
           std::output_iterator<T> OA,
           std::output_iterator<T> OD>
  requires std::random_access_iterator<OA> and std::random_access_iterator<OD>
  auto operator()(R&& range,
                  OA const approximation_result,
                  OD const detail_result) const -> std::pair<OA, OD>
  {
    return (*this)(std::ranges::begin(range),
                   std::ranges::end(range),
                   approximation_result,
                   detail_result);
  }

  template<std::forward_iterator I,
           std::sentinel_for<I> S,
           std::output_iterator<T> OA,
           std::output_iterator<T> OD>
  requires std::random_access_iterator<OA> and std::random_access_iterator<OD>
  auto operator()(I const first,
                  S const last,
                  OA const approximation_result,
                  OD const detail_result) const -> std::pair<OA, OD>
  {
    return (*this)(ranges::subrange{ first, last } | ranges::views::stride(2),
                   ranges::subrange{ std::next(first), last } |
                     ranges::views::stride(2),
                   approximation_result,
                   detail_result);
  }

  template<std::ranges::input_range R1,
           std::ranges::input_range R2,
           std::output_iterator<T> OA,
           std::output_iterator<T> OD>
  requires std::random_access_iterator<OA> and std::random_access_iterator<OD>
  auto operator()(R1&& even_range,
                  R2&& odd_range,
                  OA const approximation_result,
                  OD const detail_result) const -> std::pair<OA, OD>
  {
    return (*this)(std::ranges::begin(even_range),
                   std::ranges::end(even_range),
                   std::ranges::begin(odd_range),
                   std::ranges::end(odd_range),
                   approximation_result,
                   detail_result);
  }

  template<std::input_iterator I1,
           std::sentinel_for<I1> S1,
           std::input_iterator I2,
           std::sentinel_for<I2> S2,
           std::output_iterator<T> OA,
           std::output_iterator<T> OD>
  requires std::random_access_iterator<OA> and std::random_access_iterator<OD>
  auto operator()(I1 even_first,
                  S1 const even_last,
                  I2 odd_first,
                  S2 const odd_last,
                  OA approximation_result,
                  OD detail_result) const -> std::pair<OA, OD>
  {
    auto const approximation_last =
      std::ranges::copy(even_first, even_last, approximation_result).out;
    auto const detail_last =
      std::ranges::copy(odd_first, odd_last, detail_result).out;

    auto const n_approx = std::ptrdiff_t{
      std::distance(approximation_result, approximation_last),
    };
    auto const n_detail = std::ptrdiff_t{
      std::distance(detail_result, detail_last),
    };

    ([&]<std::size_t... step_indices>(std::index_sequence<step_indices...>)
     {
       (
         [&]<std::size_t step_index>(
           std::integral_constant<std::size_t, step_index>)
         {
           if constexpr (step_index % 2 == 0)
           {
             auto const& predict = std::get<step_index>(steps_);

             for (auto const i :
                  std::views::iota(std::ptrdiff_t{ 0 }, n_detail))
             {
               ([&]<std::size_t... offsets>(std::index_sequence<offsets...>) {
                 detail_result[i] -= predict(approximation_result[wrap_mod(
                   i - static_cast<std::ptrdiff_t>(offsets), n_approx)]...);
               }(std::make_index_sequence<order>{}));
             }
           }
           else
           {
             auto const& update = std::get<step_index>(steps_);

             for (auto const i :
                  std::views::iota(std::ptrdiff_t{ 0 }, n_approx))
             {
               ([&]<std::size_t... offsets>(std::index_sequence<offsets...>) {
                 approximation_result[i] += update(detail_result[wrap_mod(
                   i + static_cast<std::ptrdiff_t>(offsets), n_detail)]...);
               }(std::make_index_sequence<order>{}));
             }
           }
         }(std::integral_constant<std::size_t, step_indices>{}),
         ...);
     }(std::make_index_sequence<sizeof...(Steps)>{}));

    if (approx_norm_)
    {
      for (auto const i : std::views::iota(std::ptrdiff_t{ 0 }, n_approx))
      {
        approximation_result[i] /= *approx_norm_;
      }
    }

    if (detail_norm_)
    {
      for (auto const i : std::views::iota(std::ptrdiff_t{ 0 }, n_detail))
      {
        detail_result[i] /= *detail_norm_;
      }
    }

    return { approximation_last, detail_last };
  }

private:
  std::tuple<Steps...> steps_;
  std::optional<T> approx_norm_ = std::nullopt;
  std::optional<T> detail_norm_ = std::nullopt;
};

template<std::integral I>
constexpr auto haar_iwt = lifting_wavelet_transform<I, 1u>{}
                            .predict_step([](I const a) { return a; })
                            .update_step([](I const d)
                                         { return static_cast<I>(d / 2); });

template<std::integral I>
constexpr auto db4_iwt = []
{
  using std::numbers::sqrt3;

  return lifting_wavelet_transform<I, 2u>{}
    .update_step([](I const d0, auto...) { return static_cast<I>(d0 * sqrt3); })
    .predict_step(
      [](I const a0, I const a1)
      {
        constexpr auto c0 = sqrt3 / 4;
        constexpr auto c1 = (sqrt3 - 2) / 4;

        return static_cast<I>(a0 * c0 + a1 * c1);
      })
    .update_step([](I const /*d0*/, I const d1)
                 { return static_cast<I>(-d1); });
}();

template<std::integral I>
constexpr auto bior_2_2_iwt = []
{
  return lifting_wavelet_transform<I, 2u>{}
    .update_step([](I const d0, I const d1)
                 { return static_cast<I>(d0 / I{ 2 } + d1 / I{ 2 }); })
    .predict_step([](I const a0, I const a1)
                  { return static_cast<I>(a0 / I{ 4 } + a1 / I{ 4 }); });
}();

template<std::floating_point F>
constexpr auto haar_wt = []
{
  using std::numbers::sqrt2_v;

  return lifting_wavelet_transform<F, 1u>{}
    .predict_step([](F const a) { return a; })
    .update_step([](F const d) { return static_cast<F>(d / 2); })
    .normalize_approx(sqrt2_v<F>)
    .normalize_detail(sqrt2_v<F> / 2);
}();

template<std::floating_point F>
constexpr auto db4_wt = []
{
  using std::numbers::sqrt3_v;
  using std::numbers::sqrt2_v;

  return lifting_wavelet_transform<F, 2u>{}
    .update_step([](F const d0, auto...) { return d0 * sqrt3_v<F>; })
    .predict_step(
      [](F const a0, F const a1)
      {
        constexpr auto c0 = sqrt3_v<F> / 4;
        constexpr auto c1 = (sqrt3_v<F> - 2) / 4;

        return a0 * c0 + a1 * c1;
      })
    .update_step([](F const /*d0*/, F const d1) { return -d1; })
    .normalize_approx((sqrt3_v<F> + 1) / sqrt2_v<F>)
    .normalize_detail((sqrt3_v<F> - 1) / sqrt2_v<F>);
}();

template<std::integral I>
constexpr auto inv_haar_iwt = haar_iwt<I>.inverse();

template<std::integral I>
constexpr auto inv_db4_iwt = db4_iwt<I>.inverse();

template<std::integral I>
constexpr auto inv_bior_2_2_iwt = bior_2_2_iwt<I>.inverse();

template<std::floating_point F>
constexpr auto inv_haar_wt = haar_wt<F>.inverse();

template<std::floating_point F>
constexpr auto inv_db4_wt = db4_wt<F>.inverse();

template<typename T>
class recursive_2d_wavelet_transform
{
public:
  template<std::random_access_iterator I,
           std::output_iterator<T> O,
           std::size_t order,
           lifting_step<T, order>... Steps>
  auto operator()(view_2d<I> input,
                  O result,
                  lifting_wavelet_transform<T, order, Steps...> const& wavelet,
                  std::optional<std::size_t> num_iters = std::nullopt) -> O
  {
    auto width = input.width();
    auto height = input.height();

    image_.resize(width * height);
    auto const matrix = view_2d{ image_.begin(), width, height };

    std::ranges::copy(input.rows() | ranges::views::join, image_.begin());

    approx_1d_.resize(ceil_div(std::max(width, height), std::size_t{ 2 }));
    detail_1d_.resize(std::max(width, height) / 2u);

    auto const row_stride = width;

    while ((width > 1u or height > 1u) and (not num_iters or *num_iters > 0u))
    {
      auto const prev_approx = matrix.block(0u, 0u, width, height);

      if (height > 1u)
      {
        // Compute the wavelet transform vertically (for each column)
        for (auto const j : std::views::iota(std::size_t{ 0 }, width))
        {
          auto const [approx_end, detail_end] =
            wavelet(prev_approx.col(j), approx_1d_.begin(), detail_1d_.begin());

          std::ranges::copy(
            ranges::views::concat(
              ranges::subrange{ approx_1d_.begin(), approx_end },
              ranges::subrange{ detail_1d_.begin(), detail_end }),
            prev_approx.col(j).begin());
        }
      }

      if (width > 1u)
      {
        approx_1d_.resize(ceil_div(width, std::size_t{ 2 }));
        detail_1d_.resize(width / 2u);

        // Compute the wavelet transform horizontally (for each row)
        for (auto const i : std::views::iota(std::size_t{ 0 }, height))
        {
          auto const [approx_end, detail_end] =
            wavelet(prev_approx.row(i), approx_1d_.begin(), detail_1d_.begin());

          std::ranges::copy(
            ranges::views::concat(
              ranges::subrange{ approx_1d_.begin(), approx_end },
              ranges::subrange{ detail_1d_.begin(), detail_end }),
            prev_approx.row(i).begin());
        }
      }

      // Copy the detail components to output
      auto const detail_diag =
        prev_approx.block(ceil_div(width, std::size_t{ 2 }),
                          ceil_div(height, std::size_t{ 2 }),
                          width / 2u,
                          height / 2u);
      auto const detail_hor =
        prev_approx.block(ceil_div(width, std::size_t{ 2 }),
                          0u,
                          width / 2u,
                          ceil_div(height, std::size_t{ 2 }));
      auto const detail_vert =
        prev_approx.block(0u,
                          ceil_div(height, std::size_t{ 2 }),
                          ceil_div(width, std::size_t{ 2 }),
                          height / 2u);

      for (auto const detail_part : { detail_diag, detail_hor, detail_vert })
      {
        result =
          std::ranges::copy(detail_part.rows() | ranges::views::join, result)
            .out;
      }

      height = ceil_div(height, std::size_t{ 2 });
      width = ceil_div(width, std::size_t{ 2 });
      if (num_iters)
      {
        --*num_iters;
      }
    }

    // Copy the remaining approximation to output
    auto const final_approx = matrix.block(0u, 0u, width, height);
    result =
      std::ranges::copy(final_approx.rows() | ranges::views::join, result).out;

    return result;
  }

private:
  std::vector<T> image_;
  std::vector<T> approx_1d_;
  std::vector<T> detail_1d_;
};

template<typename T>
class inv_recursive_2d_wavelet_transform
{
public:
  template<std::ranges::input_range R,
           std::random_access_iterator O,
           std::size_t order,
           lifting_step<T, order>... Steps>
  requires std::output_iterator<O, T>
  auto operator()(R&& range,
                  view_2d<O> const result,
                  inverse_lifting_wavelet_transform<T, order, Steps...> const&
                    inverse_wavelet,
                  std::optional<std::size_t> const num_iters = std::nullopt)
    -> std::ranges::iterator_t<R>
  {
    return (*this)(std::ranges::begin(range),
                   std::ranges::end(range),
                   result,
                   inverse_wavelet,
                   num_iters);
  }

  template<std::input_iterator I,
           std::sentinel_for<I> S,
           std::random_access_iterator O,
           std::size_t order,
           lifting_step<T, order>... Steps>
  requires std::output_iterator<O, T>
  auto operator()(I const first,
                  S const last,
                  view_2d<O> const result,
                  inverse_lifting_wavelet_transform<T, order, Steps...> const&
                    inverse_wavelet,
                  std::optional<std::size_t> const num_iters = std::nullopt)
    -> I
  {
    buffer_1d_.resize(std::max(result.width(), result.height()));
    return backtrack(first, last, result, inverse_wavelet, num_iters);
  }

private:
  std::vector<T> buffer_1d_;

  template<std::input_iterator I,
           std::sentinel_for<I> S,
           std::random_access_iterator O,
           std::size_t order,
           lifting_step<T, order>... Steps>
  requires std::output_iterator<O, T>
  auto backtrack(I first,
                 S const last,
                 view_2d<O> const image,
                 inverse_lifting_wavelet_transform<T, order, Steps...> const&
                   inverse_wavelet,
                 std::optional<std::size_t> num_iters) -> I
  {
    auto const width = image.width();
    auto const height = image.height();

    if ((width <= 1u and height <= 1u) or (num_iters and *num_iters == 0u))
    {
      // Copy the remaining approximation from input
      first = std::ranges::copy_n(first,
                                  image.width() * image.height(),
                                  (image.rows() | ranges::views::join).begin())
                .in;
      return first;
    }

    // Copy the detail components from input
    auto const detail_diag = image.block(ceil_div(width, std::size_t{ 2 }),
                                         ceil_div(height, std::size_t{ 2 }),
                                         width / 2u,
                                         height / 2u);
    auto const detail_hor = image.block(ceil_div(width, std::size_t{ 2 }),
                                        0u,
                                        width / 2u,
                                        ceil_div(height, std::size_t{ 2 }));
    auto const detail_vert = image.block(0u,
                                         ceil_div(height, std::size_t{ 2 }),
                                         ceil_div(width, std::size_t{ 2 }),
                                         height / 2u);

    for (auto const detail_part : { detail_diag, detail_hor, detail_vert })
    {
      first =
        std::ranges::copy_n(first,
                            detail_part.width() * detail_part.height(),
                            (detail_part.rows() | ranges::views::join).begin())
          .in;
    }

    auto const prev_width = ceil_div(width, std::size_t{ 2 });
    auto const prev_height = ceil_div(height, std::size_t{ 2 });

    // Recurse
    if (num_iters)
    {
      --*num_iters;
    }
    first = backtrack(first,
                      last,
                      image.block(0u, 0u, prev_width, prev_height),
                      inverse_wavelet,
                      num_iters);

    if (width > 1u)
    {
      // Compute the inverse wavelet transform horizontally
      for (auto const i : std::views::iota(std::size_t{ 0 }, height))
      {
        inverse_wavelet(image.row(i) | ranges::views::take(prev_width),
                        image.row(i) | ranges::views::drop(prev_width),
                        buffer_1d_.begin());
        std::ranges::copy_n(buffer_1d_.begin(), width, image.row(i).begin());
      }
    }

    if (height > 1u)
    {
      // Compute the inverse wavelet transform vertically
      for (auto const j : std::views::iota(std::size_t{ 0 }, width))
      {
        inverse_wavelet(image.col(j) | ranges::views::take(prev_height),
                        image.col(j) | ranges::views::drop(prev_height),
                        buffer_1d_.begin());
        std::ranges::copy_n(buffer_1d_.begin(), height, image.col(j).begin());
      }
    }

    return first;
  }
};

} // namespace pa171::transform
