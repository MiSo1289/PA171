#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <limits>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <pa171/coding/lzw_base.hpp>
#include <pa171/utils/numeric.hpp>

namespace pa171::coding::lzw
{

class decode_error : public std::runtime_error
{
public:
    using runtime_error::runtime_error;
};

class decoder
{
public:
    explicit decoder(code_point_size_t code_size = 12u, options_t options = {})
        : code_size_{ code_size }
        , options_{ options }
    {
    }

    template<std::ranges::input_range R, std::output_iterator<std::byte> O>
    requires std::same_as<std::ranges::range_value_t<R>, std::byte>
    auto operator()(R&& range, O const result)
        -> std::pair<std::ranges::iterator_t<R>, O>
    {
        return (*this)(
            std::ranges::begin(range), std::ranges::end(range), result);
    }

    template<std::input_iterator I,
             std::sentinel_for<I> S,
             std::output_iterator<std::byte> O>
    requires std::same_as<std::iter_value_t<I>, std::byte>
    auto operator()(I begin, S const end, O result) -> std::pair<I, O>
    {
        init_table();
        block_end_ = block_size;

        auto accumulator = std::string{};
        auto end_of_input = false;

        do
        {
            if ((options_ & flush_full_dict) and not next_code_point_)
            {
                init_table();
            }

            auto const code_point = read_code_point(begin, end);

            if (not code_point)
            {
                throw decode_error{ "Unexpected end of input" };
            }
            else if (*code_point == end_input_code_point)
            {
                end_of_input = true;
            }
            else if (*code_point >= table_.size())
            {
                throw decode_error{ "Unknown code point found" };
            }
            else
            {
                auto const acc_was_empty = not accumulator.empty();
                auto decoded = table_[*code_point];

                result = write_sequence(result, decoded);

                if (not next_code_point_)
                {
                    // Table is full, do not create new code points
                    continue;
                }

                accumulator.append(decoded);

                if (not acc_was_empty)
                {
                    if (decoded.size() != 1u)
                    {
                        throw decode_error{
                            "Unexpected length of decoded sequence"
                        };
                    }

                    table_.push_back(std::move(accumulator));
                    update_next_code_point();

                    accumulator = std::move(decoded);
                }
            }
        } while (not end_of_input);

        return std::pair{ begin, result };
    }

private:
    using block_type = std::uint64_t;
    using block_index_type = std::uint32_t;
    using code_point_type = std::uint32_t;
    using table_type = std::vector<std::string>;

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

        if (options_ & dynamic_code_size and
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
            table_.emplace_back(std::string_view{ &base_symbol_char, 1u });
        }

        table_.emplace_back(); // Empty slot for the end of input code point

        next_code_point_ = first_dynamic_code_point;
    }

    template<std::input_iterator I, std::sentinel_for<I> S>
    requires std::same_as<std::iter_value_t<I>, std::byte>
    [[nodiscard]] auto read_code_point(I& begin, S const end)
        -> std::optional<code_point_type>
    {
        static_assert(block_size >=
                      std::numeric_limits<code_point_type>::digits);

        if (block_size - block_end_ < current_code_size_)
        {
            while (block_end_ >= 8u and begin != end)
            {
                block_ >>= 8u;
                block_ |= std::to_integer<block_type>(*begin++)
                          << (block_size - 8u);
                block_end_ -= 8u;
            }
        }

        if (block_size - block_end_ < current_code_size_)
        {
            // Not enough bits left in the input range
            return std::nullopt;
        }

        auto const code_point_mask = (1u << current_code_size_) - 1u;
        auto const code_point = static_cast<code_point_type>(
            (block_ >> block_end_) & code_point_mask);
        block_end_ += current_code_size_;

        return code_point;
    }

    template<std::output_iterator<std::byte> O>
    [[nodiscard]] auto write_sequence(O const result,
                                      std::string_view const sequence) -> O
    {
        return std::ranges::copy(std::as_bytes(std::span{ sequence }), result)
            .out;
    }
};

} // namespace pa171::coding::lzw
