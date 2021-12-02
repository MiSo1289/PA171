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

#include <absl/container/flat_hash_map.h>

#include <pa171/coding/lzw_base.hpp>

namespace pa171::coding::lzw
{

class encoder
{
public:
    explicit encoder(code_point_size_t const code_size = default_code_size,
                     options_t const options = default_options)
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
    auto operator()(I first, S const last, O result) -> O
    {
        init_table();
        block_end_ = 0u;

        auto input_accumulator = std::string{};
        auto next_code_word = std::string{};
        auto code_point = code_point_type{};

        while (first != last)
        {
            if ((options_ & flush_full_dict) and not next_code_point_)
            {
                init_table();
            }

            auto const input_byte = *first++;
            input_accumulator.push_back(static_cast<char>(input_byte));

            if (auto const match = table_.find(input_accumulator);
                match != table_.end())
            {
                code_point = match->second;
                continue;
            }

            result = write_code_point(result, code_point);

            if (next_code_point_)
            {
                if (not next_code_word.empty())
                {
                    table_.emplace(std::move(next_code_word),
                                   *next_code_point_);
                    update_next_code_point();
                }

                next_code_word = std::move(input_accumulator);
            }

            input_accumulator = static_cast<char>(input_byte);

            auto const match = table_.find(input_accumulator);
            assert(match != table_.end());
            code_point = match->second;
        }

        if (not input_accumulator.empty())
        {
            result = write_code_point(result, code_point);
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

        if (block_size - block_end_ < current_code_size_)
        {
            while (block_end_ >= 8u)
            {
                *result++ = static_cast<std::byte>(block_ & 0xFFu);

                block_ >>= 8u;
                block_end_ -= 8u;
            }
        }

        block_ |= block_type{ code_point } << block_end_;
        block_end_ += current_code_size_;

        return result;
    }

    template<std::output_iterator<std::byte> O>
    [[nodiscard]] auto flush_block(O result) -> O
    {
        while (block_end_ > 0u)
        {
            *result++ = static_cast<std::byte>(block_ & 0xFFu);

            auto const shift = std::min(block_index_type{ 8 }, block_end_);
            block_ >>= shift;
            block_end_ -= shift;
        }

        return result;
    }
};

} // namespace pa171::coding::lzw
