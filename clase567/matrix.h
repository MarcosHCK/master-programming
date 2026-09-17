/* Copyright 2026 MarcosHCK
 */
#pragma once
#include <../clase567/exception.h>
#include <../clase567/matrixbase.h>
#include <format>
#include <generator>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

template<typename T = double> class matrix: public matrix_base
{

  std::vector<T> _values;
public:

  using value_type = T;

  inline matrix (unsigned rows, unsigned cols): matrix_base (rows, cols)
    {
      _values.resize (get_cols () * get_rows (), 0.0);
    }

  inline constexpr T* data () noexcept { return _values.data (); }
  inline constexpr const T* data () const noexcept { return _values.data (); }

  inline T& operator[] (unsigned r, unsigned c)
    {
      return _values [c + r * get_cols ()];
    }

  inline T operator[] (unsigned r, unsigned c) const
    {
      return _values [c + r * get_cols ()];
    }

  inline T& at (std::pair<unsigned, unsigned> index)
    {
      auto [ r, c ] = (check_bounds (index), index);
    return operator[] (r, c);
    }

  inline T at (std::pair<unsigned, unsigned> index) const
    {
      auto [ r, c ] = (check_bounds (index), index);
    return operator[] (r, c);
    }

  template<typename R = T>
  inline R determinant () const
    {

      if (get_cols () != get_rows ())
        throw utility::wrap_stacktrace<std::format_error> ("only square matrices");

      switch (get_cols ())
        {

        case 0:
          return 1;

        case 1:
          return _values [0];

        case 2:
          return _values [0] * _values [3] - _values [1] * _values [2];

        default:
          {

            auto span = std::ranges::subrange (data (), &data () [get_cols ()]);
            auto incr = std::views::iota ((decltype (get_cols ())) 0, get_cols ());

            auto sum = std::views::zip (incr, span)
                     | std::views::transform ([&](auto&& p) noexcept -> std::pair<bool, R>
              {
                auto [ c, value ] = p;
                return { 0 == (c & 1), static_cast<R> (value) * minor (0, c).template determinant<R> () };
              });

            return std::accumulate (sum.begin (), sum.end (), (R) 0, [](R acc, auto&& p) noexcept -> R
              {
                auto [ e, value ] = p;
                return e ? acc + value : acc - value;
              });
          }
        }
    }

  template<std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_reference_t<Range>, std::string>
  static inline matrix<T> load (Range&& lines, std::optional<char> delimiter = std::nullopt)
    {

      std::regex regex = get_regex_delimiter (delimiter);
      std::vector<std::vector<T>> rows;

      for (const std::string line: lines)
        {

          int size = 0 == rows.size () ? -1 : rows [0].size ();

          if (auto row = load_line<T> (regex, line, size); size == -1 || row.size () == (size_t) size)

            rows.push_back (std::move (row));
          else
            {
              auto m = std::string ("mismatching number of columns in row ") + std::to_string (1 + rows.size ());
              throw utility::wrap_stacktrace<std::invalid_argument> (std::move (m));
            }
        }

      if (0 == rows.size ())
        return matrix<T> (0, 0);

      matrix<T> matrix (rows.size (), rows [0].size ());
      matrix._values.resize (0);

      for (const auto& row: rows)
        {
          matrix._values.append_range (row);
        }
    return matrix;
    }

  inline matrix<T> minor (unsigned r, unsigned c) const
    {

      check_bounds ({ r, c });
      matrix<T> M (get_cols () - 1, get_rows () - 1);

      for (unsigned ri = 0, ra = 0; ri < get_cols (); ++ri) if (r == ri) continue; else
        {

          for (unsigned ci = 0, ca = 0; ci < get_cols (); ++ci) if (c == ci) continue; else
            {
              M [ra, ca] = operator[] (ri, ci);
              ++ca;
            }

          ++ra;
        }
    return M;
    }

  inline std::string to_string () const
    {

      std::string full;

      for (const auto bit: to_string_lines ())
        {
          full.append_range (bit);
          full.push_back ('\n');
        }
    return (full.resize (full.length () - 1), full);
    }

  static inline std::string to_string_element (T value, unsigned pad = 0)
    {

      auto str = std::to_string (value);

      if (str.length () < pad)
        str.insert_range (str.begin (), std::string (pad - str.length (), ' '));
    return str;
    }

  inline std::generator<std::string> to_string_lines (unsigned pad = 0) const
    {

      std::string line ("| |");

      if (_values.begin () == _values.end ())
        co_yield line;

      for (auto it = _values.begin (); it != _values.end (); it += get_cols ())
        {

          line.assign ("|");

          for (const T value: std::ranges::subrange (it, it + get_cols ()))
            {
              line.push_back (' ');
              line.append_range (to_string_element (value, pad));
            }

          line.append_range (std::string (" |"));
          co_yield line;
        }
    }
};