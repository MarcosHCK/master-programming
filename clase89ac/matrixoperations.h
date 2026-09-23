/* Copyright 2026 MarcosHCK
 */
#pragma once
#include <../clase567/exception.h>
#include <../clase89ac/matrix.h>
#include <algorithm>
#include <cmath>
#include <ranges>

namespace matrix_operations
{

  template<typename T>
  concept matrix_type = requires (std::remove_cvref_t<T> value)
    {
      typename decltype (value)::value_type;
      requires std::same_as<decltype (value), matrix<typename decltype (value)::value_type>>;
    };

  namespace details
    {

      template<typename A, typename B>
      struct higher_type
        {

          using type = std::conditional_t<std::same_as<A, B>, A,
                       std::conditional_t<std::floating_point<A> && ! std::floating_point<B>, A,
                       std::conditional_t<std::floating_point<B> && ! std::floating_point<A>, B,
                       std::conditional_t<(sizeof (A) > sizeof (B)), A, B>>>>;
        };

      template<typename A, typename B = A>
      concept equatable = requires (A a, B b)
        {
          a == b;
          { a == b } -> std::same_as<bool>;
        };

      template<typename A, typename B = A>
      static inline constexpr bool nothrow_equatable_v = noexcept (std::declval<A> () == std::declval<B> ());
    }

  template<matrix_type _A_matrix,
           matrix_type _B_matrix,
           typename R = details::higher_type<typename std::remove_cvref_t<_A_matrix>::value_type,
                                             typename std::remove_cvref_t<_B_matrix>::value_type>::type>
  static inline matrix<R> operator+ (_A_matrix&& A, _B_matrix&& B);

  template<matrix_type _A_matrix,
           matrix_type _B_matrix,
           typename R = details::higher_type<typename std::remove_cvref_t<_A_matrix>::value_type,
                                             typename std::remove_cvref_t<_B_matrix>::value_type>::type>
  static inline matrix<R> operator- (_A_matrix&& A, _B_matrix&& B);

  template<matrix_type _A_matrix,
           typename R = typename std::remove_cvref_t<_A_matrix>::value_type>
  static inline matrix<R> operator- (_A_matrix&& A);

  constexpr auto adjugate = std::integral_constant<char, 'A'> ();
  constexpr auto inverse = std::integral_constant<int, -1> ();
  constexpr auto transpose = std::integral_constant<char, 'T'> ();

  template<matrix_type _A_matrix,
           typename R = typename std::remove_cvref_t<_A_matrix>::value_type>
  static inline matrix<R> operator^ (_A_matrix&& A, decltype (adjugate));

  template<matrix_type _A_matrix,
           typename R = typename std::remove_cvref_t<_A_matrix>::value_type>
  static inline matrix<R> operator^ (_A_matrix&& A, decltype (inverse));

  template<matrix_type _A_matrix,
           typename R = typename std::remove_cvref_t<_A_matrix>::value_type>
  static inline matrix<R> operator^ (_A_matrix&& A, decltype (transpose));

  template<matrix_type _A_matrix,
         matrix_type _B_matrix,
         typename R = details::higher_type<typename std::remove_cvref_t<_A_matrix>::value_type,
                                           typename std::remove_cvref_t<_B_matrix>::value_type>::type>
  static inline matrix<R> operator* (_A_matrix&& A, _B_matrix&& B);

  template<matrix_type _A_matrix,
           typename _B_scalar,
           typename R = typename std::remove_cvref_t<_A_matrix>::value_type>
  static inline matrix<R> operator^ (_A_matrix&& A, _B_scalar&& scalar);

  template<matrix_type _A_matrix,
           matrix_type _B_matrix,
           typename R = details::higher_type<typename std::remove_cvref_t<_A_matrix>::value_type,
                                             typename std::remove_cvref_t<_B_matrix>::value_type>::type>
    requires matrix_operations::details::equatable<typename std::remove_cvref_t<_A_matrix>::value_type,
                                                  typename std::remove_cvref_t<_B_matrix>::value_type>
  static inline bool operator== (_A_matrix&& A, _B_matrix&& B)
    noexcept (matrix_operations::details::nothrow_equatable_v<typename std::remove_cvref_t<_A_matrix>::value_type,
                                                              typename std::remove_cvref_t<_B_matrix>::value_type>);
}

template<matrix_operations::matrix_type _A_matrix,
         matrix_operations::matrix_type _B_matrix,
         typename R>
static inline matrix<R> matrix_operations::operator+ (_A_matrix&& A, _B_matrix&& B)
{

  if (A.get_cols () != B.get_cols ())
    {
      auto m = ("incompatible number of cols: " + std::to_string (A.get_cols ())) + " against " + std::to_string (B.get_cols ());
      throw utility::wrap_stacktrace<std::invalid_argument> (std::move (m));
    }

  if (A.get_rows () != B.get_rows ())
    {
      auto m = ("incompatible number of rows: " + std::to_string (A.get_rows ())) + " against " + std::to_string (B.get_rows ());
      throw utility::wrap_stacktrace<std::invalid_argument> (std::move (m));
    }

  matrix<R> M (A.get_rows (), A.get_cols ());
  
  std::ranges::copy (std::views::zip (A, B) | std::views::transform ([](auto&& pair) -> R
                      { return static_cast<R> (std::get<0> (pair)) + static_cast<R> (std::get<1> (pair)); }),
                     M.begin ());
return M;
}

template<matrix_operations::matrix_type _A_matrix,
         matrix_operations::matrix_type _B_matrix,
         typename R>
static inline matrix<R> matrix_operations::operator- (_A_matrix&& A, _B_matrix&& B)
{

  if (A.get_cols () != B.get_cols ())
    {
      auto m = ("incompatible number of cols: " + std::to_string (A.get_cols ())) + " against " + std::to_string (B.get_cols ());
      throw utility::wrap_stacktrace<std::invalid_argument> (std::move (m));
    }

  if (A.get_rows () != B.get_rows ())
    {
      auto m = ("incompatible number of rows: " + std::to_string (A.get_rows ())) + " against " + std::to_string (B.get_rows ());
      throw utility::wrap_stacktrace<std::invalid_argument> (std::move (m));
    }

  matrix<R> M (A.get_rows (), A.get_cols ());
  
  std::ranges::copy (std::views::zip (A, B) | std::views::transform ([](auto&& pair) -> R
                      { return static_cast<R> (std::get<0> (pair)) - static_cast<R> (std::get<1> (pair)); }),
                     M.begin ());
return M;
}

template<matrix_operations::matrix_type _A_matrix,
         typename R>
static inline matrix<R> matrix_operations::operator- (_A_matrix&& A)
{

  matrix<R> M (A.get_rows (), A.get_cols ());
  
  std::ranges::copy (std::views::all (A) | std::views::transform ([](auto&& e) -> R
                      { return - static_cast<R> (e); }), M.begin ());
return M;
}

template<matrix_operations::matrix_type _A_matrix,
         typename R>
static inline matrix<R> matrix_operations::operator^ (_A_matrix&& A, decltype (matrix_operations::adjugate))
{

  if (A.get_cols () != A.get_rows ())
    throw utility::wrap_stacktrace<std::format_error> ("only square matrices");

  matrix<R> M (A.get_rows (), A.get_cols ());

  for (unsigned c = 0; c < A.get_cols (); ++c)
  for (unsigned r = 0; r < A.get_rows (); ++r)
    {

      auto d = A.minor (r, c).determinant ();
      M [c, r] = 0 == ((c + r) & 1) ? d : -d;
    }
return M;
}

template<matrix_operations::matrix_type _A_matrix,
         typename R>
static inline matrix<R> matrix_operations::operator^ (_A_matrix&& A, decltype (matrix_operations::inverse))
{

  if (A.get_cols () != A.get_rows ())
    throw utility::wrap_stacktrace<std::format_error> ("only square matrices");

  matrix<R> M (A.get_rows (), A.get_cols ());
  matrix<R> C = A ^ std::integral_constant<char, 'A'> ();

  auto det = A.template determinant<R> ();

  if (det == (R) 0)
    throw utility::wrap_stacktrace<std::invalid_argument> ("matrix is not invertible (determinant is zero)");

  std::ranges::copy (std::views::all (A) | std::views::transform ([=] (R element) noexcept -> R
                      { return element / det; }), M.begin ());
return M;
}

template<matrix_operations::matrix_type _A_matrix,
         typename R>
static inline matrix<R> matrix_operations::operator^ (_A_matrix&& A, decltype (matrix_operations::transpose))
{

  matrix<R> M (A.get_cols (), A.get_rows ());

  for (unsigned c = 0; c < A.get_cols (); ++c)
  for (unsigned r = 0; r < A.get_rows (); ++r)
    {
      M [c, r] = A [r, c];
    }
return M;
}

template<matrix_operations::matrix_type _A_matrix,
         matrix_operations::matrix_type _B_matrix,
         typename R>
static inline matrix<R> matrix_operations::operator* (_A_matrix&& A, _B_matrix&& B)
{

  if (A.get_cols () != B.get_rows ())
    {
      auto m = ("incompatible sizes: " + std::to_string (A.get_cols ())) + " columns against " + std::to_string (B.get_rows ()) + " rows";
      throw utility::wrap_stacktrace<std::invalid_argument> (std::move (m));
    }

  matrix<R> M (A.get_rows (), B.get_cols ());

  auto t = B ^ transpose;

  for (unsigned r = 0; r < M.get_rows (); ++r)
    {

      auto a_row_p = A.begin () + r * A.get_cols ();

      for (unsigned c = 0; c < M.get_cols (); ++c)
        {

          auto b_col_p = t.begin () + c * t.get_cols ();

          auto&& a_row_s = std::ranges::subrange (a_row_p, a_row_p + A.get_cols ());
          auto&& b_col_s = std::ranges::subrange (b_col_p, b_col_p + t.get_cols ());

          auto chain = std::views::zip (std::move (a_row_s), std::move (b_col_s))
                     | std::views::transform ([](auto&& p) noexcept -> R
                        { return static_cast<R> (std::get<0> (p)) * static_cast<R> (std::get<1> (p)); });

          M [r, c] = std::accumulate (chain.begin (), chain.end (), (R) 0);
        }
    }
return M;
}

template<matrix_operations::matrix_type _A_matrix,
         typename _B_scalar, typename R>
static inline matrix<R> matrix_operations::operator^ (_A_matrix&& A, _B_scalar&& scalar)
{

  if (A.get_cols () != A.get_rows ())
    throw utility::wrap_stacktrace<std::format_error> ("only square matrices");

  if constexpr (std::floating_point<std::remove_cvref_t<_B_scalar>>
             || std::signed_integral<std::remove_cvref_t<_B_scalar>>)
    {

      if (scalar < 0)
        throw utility::wrap_stacktrace<std::invalid_argument> ("negative exponent");
    }

  if constexpr (std::floating_point<std::remove_cvref_t<_B_scalar>>)
    {

      if (scalar != std::floorl ((long double) scalar))
        throw utility::wrap_stacktrace<std::invalid_argument> ("fractional exponent");
    }

  auto M = matrix<R>::identity (A.get_cols ());
  auto s = (size_t) scalar;

  for (matrix<R> B = A; s > 0; B = B * B, s >>= 1) if (1 == (s & 1))
    M = M * B;

return M;
}

template<matrix_operations::matrix_type _A_matrix,
         matrix_operations::matrix_type _B_matrix,
         typename R>
  requires matrix_operations::details::equatable<typename std::remove_cvref_t<_A_matrix>::value_type,
                                                 typename std::remove_cvref_t<_B_matrix>::value_type>
static inline bool matrix_operations::operator== (_A_matrix&& A, _B_matrix&& B)
  noexcept (matrix_operations::details::nothrow_equatable_v<typename std::remove_cvref_t<_A_matrix>::value_type,
                                                            typename std::remove_cvref_t<_B_matrix>::value_type>)
{

  if (A.get_cols () != B.get_cols () || A.get_rows () != B.get_rows ())
    return false;

  if constexpr (std::same_as<typename std::remove_cvref_t<_A_matrix>::value_type,
                             typename std::remove_cvref_t<_B_matrix>::value_type>)
    return A.operator== (B);

  constexpr auto noexcept_ = matrix_operations::details::nothrow_equatable_v<typename std::remove_cvref_t<_A_matrix>::value_type,
                                                                             typename std::remove_cvref_t<_B_matrix>::value_type>;

  return std::ranges::all_of (std::views::zip (A, B), [](auto&& p) noexcept (noexcept_) -> bool
    { return std::get<0> (p) == std::get<1> (p); });
}