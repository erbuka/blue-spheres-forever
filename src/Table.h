#pragma once

#include <string_view>
#include <array>
#include <tuple>

namespace bsf
{
  template <size_t R, typename... Args>
  struct Table
  {
    using RowType = std::tuple<Args...>;

    const std::array<RowType, R> Rows;

    template <size_t KeyCol, typename K>
    constexpr const RowType &GetRow(const K &keyVal) const
    {
      for (size_t i = 0; i < R; ++i)
        if (std::get<KeyCol>(Rows[i]) == keyVal)
          return Rows[i];

      throw std::range_error("Row not found");
    }

    template <size_t KeyCol, size_t TargetCol, typename K>
    constexpr const auto &Get(const K &keyVal) const
    {
      return std::get<TargetCol>(GetRow<KeyCol>(keyVal));
    }

    template <typename K>
    constexpr const RowType &operator[](const K &key0) const
    {
      return GetRow<0>(key0);
    }
  };
}