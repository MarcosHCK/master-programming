/* Copyright 2026 MarcosHCK
 */
#pragma once
#include <../clase567/exception.h>
#include <../clase567/parsen.h>
#include <ranges>
#include <regex>
#include <utility>

class matrix_base
{

  unsigned _cols;
  unsigned _rows;
public:

  inline constexpr matrix_base (unsigned rows, unsigned cols): _cols (cols), _rows (rows)
    { }

  inline constexpr unsigned get_cols () const noexcept { return _cols; }
  inline constexpr unsigned get_rows () const noexcept { return _rows; }

protected:

  inline void check_bounds (std::pair<unsigned, unsigned> index) const
    {

      auto [ r, c ] = index;

      if (r >= _rows)
        {
          auto m = "bad index [ (row " + std::to_string (r) + "), (col " + std::to_string (c) + ") ]: row out of bounds";
          throw utility::wrap_stacktrace<std::out_of_range> (std::move (m));
        }

      if (c >= _cols)
        {
          auto m = "bad index [ (row " + std::to_string (r) + "), (col " + std::to_string (c) + ") ]: col out of bounds";
          throw utility::wrap_stacktrace<std::out_of_range> (std::move (m));
        }
    }

  static inline std::regex get_regex_delimiter (std::optional<char> delimiter)
    {

      constexpr auto flags = std::regex_constants::ECMAScript
                           | std::regex_constants::optimize;

      if (! delimiter.has_value ())

        return std::regex ("[\\s\\t]+", flags);
      else
        return std::regex (std::string ("[\\") + delimiter.value () + "]+", flags);
    }

  template<typename T>
  static inline std::vector<T> load_line (const std::regex& regex, const std::string& str, int size = -1)
    {

      std::sregex_token_iterator end;
      std::sregex_token_iterator iter (str.begin (), str.end (), regex, -1);
      std::vector<T> row;

      if (size > 0)
        row.reserve (size);

      constexpr auto no_errc = std::errc { };

      for (const auto match: std::ranges::subrange (iter, end))
        {

          auto token = std::string_view (&*match.first, match.length ());
          T value = (T) 0;

          if (auto result = parse_n::parse_n<T> (token, value); result.ec == no_errc)
            row.push_back (value);

          else switch (result.ec)
            {

            case std::errc::invalid_argument:
              throw utility::wrap_stacktrace<std::invalid_argument> (result.ptr);

            case std::errc::result_out_of_range:
              throw utility::wrap_stacktrace<std::out_of_range> (result.ptr);

            default:
              throw utility::wrap_stacktrace<std::runtime_error> (result.ptr);
        } }
    return row;
    }
};