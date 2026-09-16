/* Copyright 2026 MarcosHCK
 */
#pragma once
#include <cerrno>
#include <charconv>
#include <concepts>
#include <ranges>
#include <system_error>

namespace parse_n
{

  template<typename T>
  struct converter;

  template<> struct converter<float> { static inline std::pair<float, int> convert (const char* nptr, char** end) noexcept
    {
      errno = 0;
      return { std::strtof (nptr, end), errno };
    } };

  template<> struct converter<double> { static inline std::pair<double, int> convert (const char* nptr, char** end) noexcept
    {
      errno = 0;
      return { std::strtod (nptr, end), errno };
    } };

  template<> struct converter<long double> { static inline std::pair<long double, int> convert (const char* nptr, char** end) noexcept
    {
      errno = 0;
      return { std::strtold (nptr, end), errno };
    } };

  template<typename T>
  concept fallback_available = requires (const char* nptr, char** end)
    {

      converter<T>::convert (nptr, end);
      { converter<T>::convert (nptr, end) } -> std::same_as<std::pair<T, int>>;
    };

  template<typename T>
  concept from_chars_available = requires (const char* begin, const char* end, T& value)
    {
      std::from_chars<T> (begin, end, value);
    };

  template<typename T,
           std::ranges::input_range Range>
    requires fallback_available<T>
          && std::convertible_to<std::ranges::range_reference_t<Range>, char>
  static inline std::from_chars_result fallback (Range&& range, T& value)
    {

      std::string str (range.begin (), range.end ());

      auto end = (char*) nullptr;
      auto [ result, err ] = converter<T>::convert (str.c_str (), &end);

      if (end == str.c_str ())
        return { range.begin (), std::errc::invalid_argument };

      auto consumed = static_cast<size_t> (end - str.c_str ());

      if (err == ERANGE)
        return { range.begin () + consumed, std::errc::result_out_of_range };

    return (value = result, std::from_chars_result { range.begin () + consumed, {} });
    }

  template<typename T,
           std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_reference_t<Range>, char>
  static inline std::from_chars_result parse_n (Range&& string, T& value)
    {

      if constexpr (! from_chars_available<T>)

        return fallback<T> (string, value);
      else
        return std::from_chars<T> (string.begin (), string.end (), value, 10);
    }
}