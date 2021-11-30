#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <limits>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <absl/container/flat_hash_map.h>

#include <pa171/coding/lzw_base.hpp>
#include <pa171/utils/numeric.hpp>

namespace pa171::coding::lzw
{

class encoder
{
public:
    explicit encoder(code_point_size_t code_size = 12u, options_t options = {})
        : code_size_{ code_size }
        , options_{ options }
    {
    }

    template<std::ranges::forward_range R, std::output_iterator<std::byte> O>
    requires std::same_as<std::ranges::range_value_t<R>, std::byte>
    auto operator()(R&& range, O const result) -> O
    {
        return (*this)(
            std::ranges::begin(range), std::ranges::end(range), result);
    }

    template<std::forward_iterator I,
             std::sentinel_for<I> S,
             std::output_iterator<std::byte> O>
    requires std::same_as<std::iter_value_t<I>, std::byte>
    auto operator()(I begin, S const end, O result) -> O
    {
        init_table();
        block_end_ = 0u;

        auto accumulator = std::string{};
        auto code_point = code_point_type{};

        while (begin != end)
        {
            if ((options_ & flush_full_dict) and not next_code_point_)
            {
                init_table();
            }

            do
            {
                accumulator.push_back(static_cast<char>(*begin));

                if (next_code_point_)
                {
                    if (auto const [match, inserted] =
                            table_.try_emplace(accumulator, *next_code_point_);
                        not inserted)
                    {
                        code_point = match->second;
                        ++begin;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    // Table is full, do not create new code points

                    if (auto const match = table_.find(accumulator);
                        match != table_.end())
                    {
                        code_point = match->second;
                        ++begin;
                    }
                    else
                    {
                        break;
                    }
                }
            } while (begin != end);

            result = write_code_point(result, code_point);

            update_next_code_point();
            accumulator.clear();
        }

        result = write_code_point(result, end_input_code_point);
        result = flush_block(result);

        return result;
    }

private:
    using block_type = std::uint64_t;
    using block_index_type = std::uint32_t;
    using code_point_type = std::uint32_t;
    using table_type = absl::flat_hash_map<std::string, code_point_type>;

    static constexpr auto initial_dynamic_code_size = code_point_size_t{ 9 };
    static constexpr auto end_input_code_point = code_point_type{ 256 };
    static constexpr auto first_dynamic_code_point = code_point_type{ 257 };
    static constexpr auto block_size = std::numeric_limits<block_type>::digits;

    code_point_size_t code_size_;
    options_t options_;
    table_type table_;
    code_point_size_t current_code_size_ = {};
    std::optional<code_point_type> next_code_point_ = {};
    block_type block_ = {};
    block_index_type block_end_ = {};

    void update_next_code_point() noexcept
    {
        if (not next_code_point_)
        {
            return;
        }

        if ((options_ & dynamic_code_size) and
            *next_code_point_ & (1u << current_code_size_))
        {
            ++current_code_size_;
        }

        if (not next_code_point_)
        {
            return;
        }

        const auto max_code_point = static_cast<code_point_type>(
            (code_point_type{ 1 } << code_size_) - 1u);

        if (*next_code_point_ == max_code_point)
        {
            next_code_point_ = std::nullopt;
            return;
        }

        ++*next_code_point_;
    }

    void init_table()
    {
        table_.clear();
        current_code_size_ = (options_ & dynamic_code_size)
                                 ? initial_dynamic_code_size
                                 : code_size_;

        for (auto const base_symbol :
             std::views::iota(code_point_type{ 0 }, code_point_type{ 256 }))
        {
            auto const base_symbol_char = static_cast<char>(base_symbol);
            table_.emplace(std::string_view{ &base_symbol_char, 1u },
                           base_symbol);
        }

        next_code_point_ = first_dynamic_code_point;
    }

    template<std::output_iterator<std::byte> O>
    [[nodiscard]] auto write_code_point(O result,
                                        code_point_type const code_point) -> O
    {
        static_assert(block_size >=
                      std::numeric_limits<code_point_type>::digits);

        block_ |= block_type{ code_point } << block_end_;
        block_end_ += current_code_size_;

        if (block_end_ >= block_size)
        {
            result = std::ranges::copy(std::as_bytes(std::span{ &block_, 1u }),
                                       result)
                         .out;

            block_end_ -= block_size;
            block_ = code_point >> (code_point - block_end_);
        }

        return result;
    }

    template<std::output_iterator<std::byte> O>
    [[nodiscard]] auto flush_block(O result) -> O
    {
        result = std::ranges::copy(std::as_bytes(std::span{ &block_, 1u })
                                       .first(ceil_div(block_end_, 8u)),
                                   result)
                     .out;
        block_end_ = 0u;

        return result;
    }
};

} // namespace pa171::coding::lzw
