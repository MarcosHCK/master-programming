/* Copyright 2026 MarcosHCK
 */
#pragma once
#include <../clase567/exception.h>
#include <../clase89ac/matrix.h>
#include <generator>
#include <optional>
#include <stdexcept>

namespace matrix_file
{

  static inline void ltrim (std::string &s)
    {
      s.erase (s.begin(), std::find_if (s.begin(), s.end(), [](unsigned char ch)
        { return !std::isspace(ch); }));
    }

  static inline void rtrim (std::string &s)
    {
      s.erase (std::find_if (s.rbegin (), s.rend (), [](unsigned char ch)
        { return !std::isspace(ch); }).base(), s.end());
    }

  static inline void trim (std::string &s)
    {
      rtrim (s); ltrim (s);
    }

  static inline std::generator<std::string> file_lines (std::istream& stream, char comment_char = '#')
    {

      for (std::string line (1, 0); std::getline (stream, line); )
        {

          auto begin = line.begin ();
          auto end = line.end ();

          if (auto pos = line.find (comment_char); pos != line.npos)
            end = begin + pos;

          if (auto str = std::string (begin, end); (trim (str), str).length () > 0)
            co_yield std::move (str);
        }
    }

  template<typename T>
  static inline matrix<T> load_matrix (std::istream& stream, std::optional<char> delimiter = std::nullopt, char comment_char = '#')
    {
      return matrix<T>::load (file_lines (stream, comment_char), delimiter);
    }

  template<typename T>
  using operation_t = std::tuple<matrix<T>, matrix<T>, std::string>;

  template<typename T>
  static inline operation_t<T> load_operation (std::istream& stream, std::optional<char> delimiter = std::nullopt, char comment_char = '#')
    {

      std::optional<matrix<T>> A, B;
      std::string operation;

      auto lines = file_lines (stream, comment_char);
      auto iter = lines.begin ();

      auto gen = [&] () noexcept -> std::generator<std::string> { for (; iter != lines.end (); ++iter)
        {

          if ((*iter) [0] == ':')
            {
              operation = (*iter).substr (1);
                          ++iter;
              break;
            }

          co_yield *iter;
        } };

      A.emplace (matrix<T>::load (gen (), delimiter));

      if ("" == operation)
        throw utility::wrap_stacktrace<std::invalid_argument> ("missing operation");

      B.emplace (matrix<T>::load (gen (), delimiter));

    return { std::move (A).value (), std::move (B).value (), std::move (operation) };
    }
};