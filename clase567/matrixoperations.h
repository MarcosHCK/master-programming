/* Copyright 2026 MarcosHCK
 */
#pragma once
#include <../clase567/exception.h>
#include <../clase567/matrix.h>
#include <numeric>
#include <ranges>

template<typename T>
concept matrix_type = requires (std::remove_cvref_t<T> value) {
  typename decltype (value)::value_type;
  std::same_as<decltype (value), matrix<typename decltype (value)::value_type>>;
};

template<typename A, typename B>
struct higher_type
{

  using type = std::conditional_t<std::same_as<A, B>, A,
                std::conditional_t<std::floating_point<A> && ! std::floating_point<B>, A,
                std::conditional_t<std::floating_point<B> && ! std::floating_point<A>, B,
                std::conditional_t<(sizeof (A) > sizeof (B)), A, B>>>>;
};

template<matrix_type _A_matrix,
         matrix_type _B_matrix,
         typename R = higher_type<typename std::remove_cvref_t<_A_matrix>::value_type,
                                  typename std::remove_cvref_t<_B_matrix>::value_type>::type>
static inline matrix<R> operator+ (_A_matrix&& A, _B_matrix&& B)
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

  auto span1 = std::span<typename std::remove_cvref_t<_A_matrix>::value_type> (A.data (), A.get_cols () * A.get_rows ());
  auto span2 = std::span<typename std::remove_cvref_t<_A_matrix>::value_type> (B.data (), B.get_cols () * B.get_rows ());
  auto span0 = std::span<typename std::remove_cvref_t<_A_matrix>::value_type> (M.data (), M.get_cols () * M.get_rows ());
  
  std::ranges::copy (std::views::zip (span1, span2) | std::views::transform ([](auto&& pair) -> R
                      { return static_cast<R> (std::get<0> (pair)) + static_cast<R> (std::get<1> (pair)); }),
                     span0.begin ());
return M;
}

template<matrix_type _A_matrix,
         typename R = typename std::remove_cvref_t<_A_matrix>::value_type>
static inline matrix<R> operator- (_A_matrix&& A)
{

  matrix<R> M (A.get_rows (), A.get_cols ());

  auto span1 = std::span<typename std::remove_cvref_t<_A_matrix>::value_type> (A.data (), A.get_cols () * A.get_rows ());
  auto span0 = std::span<typename std::remove_cvref_t<_A_matrix>::value_type> (M.data (), M.get_cols () * M.get_rows ());
  
  std::ranges::copy (std::views::all (span1) | std::views::transform ([](auto&& e) -> R
                      { return - static_cast<R> (e); }),
                     span0.begin ());
return M;
}

template<matrix_type _A_matrix,
         typename R = typename std::remove_cvref_t<_A_matrix>::value_type>
static inline matrix<R> operator~ (_A_matrix&& A)
{

  matrix<R> M (A.get_cols (), A.get_rows ());

  for (unsigned c = 0; c < A.get_cols (); ++c)
  for (unsigned r = 0; r < A.get_rows (); ++r)
    {
      M [c, r] = A [r, c];
    }
return M;
}

template<matrix_type _A_matrix,
         matrix_type _B_matrix,
         typename R = higher_type<typename std::remove_cvref_t<_A_matrix>::value_type,
                                  typename std::remove_cvref_t<_B_matrix>::value_type>::type>
static inline matrix<R> operator- (_A_matrix&& A, _B_matrix&& B)
{
  return A + (-B);
}

template<matrix_type _A_matrix,
         matrix_type _B_matrix,
         typename R = higher_type<typename std::remove_cvref_t<_A_matrix>::value_type,
                                  typename std::remove_cvref_t<_B_matrix>::value_type>::type>
static inline matrix<R> operator* (_A_matrix&& A, _B_matrix&& B)
{

  if (A.get_cols () != B.get_rows ())
    {
      auto m = ("incompatible sizes: " + std::to_string (A.get_cols ())) + " columns against " + std::to_string (B.get_rows ()) + " rows";
      throw utility::wrap_stacktrace<std::invalid_argument> (std::move (m));
    }

  matrix<R> M (A.get_rows (), B.get_cols ());

  auto t = ~B;
  auto span1 = std::span<typename std::remove_cvref_t<_A_matrix>::value_type> (A.data (), A.get_cols () * A.get_rows ());
  auto span2 = std::span<typename std::remove_cvref_t<_A_matrix>::value_type> (t.data (), t.get_cols () * t.get_rows ());
  auto span0 = std::span<typename std::remove_cvref_t<_A_matrix>::value_type> (M.data (), M.get_cols () * M.get_rows ());

  for (unsigned r = 0; r < M.get_rows (); ++r)
    {

      auto a_row_p = & span1 [r * A.get_cols ()];
      auto a_row_s = std::span<typename decltype (span1)::value_type> (a_row_p, A.get_cols ());

      for (unsigned c = 0; c < M.get_cols (); ++c)
        {

          auto b_col_p = & span2 [c * t.get_cols ()];
          auto b_col_s = std::span<typename decltype (span1)::value_type> (b_col_p, t.get_cols ());

          auto chain = std::views::zip (a_row_s, b_col_s)
                     | std::views::transform ([](auto&& p) noexcept -> R
                        { return static_cast<R> (std::get<0> (p)) * static_cast<R> (std::get<1> (p)); });

          span0 [c + r * M.get_cols ()] = std::accumulate (chain.begin (), chain.end (), (R) 0);
        }
    }
return M;
}

template<matrix_type _A_matrix,
         typename R = typename std::remove_cvref_t<_A_matrix>::value_type>
static inline matrix<R> operator^ (_A_matrix&& A, std::integral_constant<char, 'A'>)
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

template<matrix_type _A_matrix,
         typename R = typename std::remove_cvref_t<_A_matrix>::value_type>
static inline matrix<R> operator^ (_A_matrix&& A, std::integral_constant<char, 'T'>)
{
  return ~A;
}

constexpr auto adjugate = std::integral_constant<char, 'A'> ();
constexpr auto inverse = std::integral_constant<int, -1> ();
constexpr auto transpose = std::integral_constant<char, 'T'> ();

template<matrix_type _A_matrix,
         typename R = typename std::remove_cvref_t<_A_matrix>::value_type>
static inline matrix<R> operator^ (_A_matrix&& A, std::integral_constant<int, -1>)
{

  if (A.get_cols () != A.get_rows ())
    throw utility::wrap_stacktrace<std::format_error> ("only square matrices");

  matrix<R> M (A.get_rows (), A.get_cols ());
  matrix<R> C = A ^ std::integral_constant<char, 'A'> ();

  auto span1 = std::span<typename std::remove_cvref_t<_A_matrix>::value_type> (C.data (), C.get_cols () * C.get_rows ());
  auto span0 = std::span<typename std::remove_cvref_t<_A_matrix>::value_type> (M.data (), M.get_cols () * M.get_rows ());
  auto det = A.template determinant<R> ();

  if (det == (R) 0)
    throw utility::wrap_stacktrace<std::invalid_argument> ("matrix is not invertible (determinant is zero)");

  std::ranges::copy (std::views::all (span1) | std::views::transform ([=] (R element) noexcept -> R
                      { return element / det; }), span0.begin ());
return M;
}