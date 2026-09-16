/* Copyright 2026 MarcosHCK
 */
#pragma once
#include <../clase567/exception.h>
#include <../clase567/matrixbase.h>
#include <generator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

template<typename T = double> class matrix: public matrix_base
{

  std::vector<T> _values;
public:

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

      for (auto it = matrix._values.begin (); const auto& row: rows)
        {
          matrix._values.append_range (row);
        }
    return matrix;
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

  inline std::generator<std::string> to_string_lines () const
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
              line.append_range (std::to_string (value));
            }

          co_yield (line.append_range (" |"), line);
        }
    }
};