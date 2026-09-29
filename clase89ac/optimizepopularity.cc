/* Copyright 2026 MarcosHCK
 */
#include <../clase567/exception.h>
#include <../clase89ac/geneticopt.h>
#include <../clase89ac/matrix.h>
#include <../clase89ac/matrixoperations.h>
#include <../clase89ac/optimizations.h>
#include <../clase89ac/optimizebar.h>
using namespace matrix_operations;
using namespace utility;

namespace __optimize_popularity
{

  template<std::random_engine<unsigned long> RandomEngine>
  struct combinator_;

  struct fitness_ranker_;

  template<std::random_engine<unsigned long> RandomEngine>
  struct uniform_distribution_;
}

template<std::random_engine<unsigned long> RandomEngine>
struct __optimize_popularity::combinator_: default_mutator<RandomEngine>, sbx_combine<RandomEngine>
{

  using config_t = genetic_optimization_config;

  struct bounds_range: std::ranges::view_base
    {

      struct iterator
        {

          using iterator_concept = std::input_iterator_tag;
          using iterator_category = std::input_iterator_tag;
          using difference_type = std::ptrdiff_t;
          using value_type = std::pair<double, double>;

          inline value_type operator*() const noexcept { return bounds_range::bounds; }
          inline iterator& operator++() noexcept { return *this; }
          inline void operator++(int) noexcept { }
          inline bool operator==(std::default_sentinel_t) const noexcept { return false; }

          friend inline bool operator==(std::default_sentinel_t, const iterator&) noexcept
            { return false; } 
        };

      static constexpr iterator::value_type bounds = { 0.0, 1.0 };

      inline iterator begin () const noexcept { return iterator (); };
      inline std::default_sentinel_t end () const noexcept { return { }; }
    };

  class triangle_range: public std::ranges::view_interface<triangle_range>
    {

      unsigned _n;

      struct iterator
        {

          using iterator_concept = std::forward_iterator_tag;
          using iterator_category = std::forward_iterator_tag;
          using difference_type = std::ptrdiff_t;
          using value_type = std::pair<unsigned, unsigned>;

          unsigned i = 0;
          unsigned j = 0;
          unsigned n = 0;

          inline value_type operator*() const noexcept { return { i, j }; }

          inline iterator& operator++() noexcept
            {
              if (++j >= i) { ++i; j = 0; }
            return *this;
            }

          inline iterator operator++(int) noexcept
            {
              auto t = *this;
            return (++*this, t);
            }

          inline bool operator==(const iterator&) const = default;
        };

    public:

      explicit inline triangle_range (unsigned n) noexcept: _n (n)
        { }

      inline iterator begin () const noexcept { return { 0, 0, _n }; }
      inline iterator end () const noexcept { return { _n, 0, _n }; }
    };

  inline std::pair<matrix<bool>, matrix<bool>> operator() (RandomEngine& rng, config_t& config, const matrix<bool>& x, const matrix<bool>& y) const
    {

      auto&& xr = triangle_range (x.get_cols ())
                | std::views::transform ([&x = x] (auto p) noexcept { return static_cast<double> (x [std::get<0> (p), std::get<1> (p)]); });

      auto&& yr = triangle_range (y.get_cols ())
                | std::views::transform ([&y = y] (auto p) noexcept { return static_cast<double> (y [std::get<0> (p), std::get<1> (p)]); });

      auto&& [ Xr, Yr ] = sbx_combine<RandomEngine>::operator() (rng, config, std::move (xr), std::move (yr));

      auto&& Xrr = default_mutator<RandomEngine>::operator() (rng, config, std::move (Xr), bounds_range ());
      auto&& Yrr = default_mutator<RandomEngine>::operator() (rng, config, std::move (Yr), bounds_range ());

      matrix<bool> X (x.get_rows (), x.get_cols ());
      matrix<bool> Y (y.get_rows (), y.get_cols ());

      for (auto [ v, p ]: std::views::zip (std::move (Xrr) | std::views::transform ([](auto v) noexcept { return v > 0.5; }), triangle_range (x.get_cols ())))
        X [std::get<0> (p), std::get<1> (p)] = X [std::get<1> (p), std::get<0> (p)] = v;

      for (auto [ v, p ]: std::views::zip (std::move (Yrr) | std::views::transform ([](auto v) noexcept { return v > 0.5; }), triangle_range (x.get_cols ())))
        Y [std::get<0> (p), std::get<1> (p)] = Y [std::get<1> (p), std::get<0> (p)] = v;

    return { std::move (X), std::move (Y) };
    }
};

struct __optimize_popularity::fitness_ranker_
{

  const matrix<bool>& A;
  unsigned n, v, fact;

  inline fitness_ranker_ (const matrix<bool>& _A, unsigned _n, unsigned _v) noexcept:
      A (_A), n (_n), v (_v)
    {
      fact = n * n;
    }

  inline double operator() (const matrix<bool>& C)
    {

      auto [ own, best, diff ] = rank (C);
    return (own > best ? own - best : 2 + (best - own)) * fact + diff;
    }

  inline std::tuple<unsigned, unsigned, unsigned> rank (const matrix<bool>& C) const
    {

      auto M = ((matrix<unsigned>) C) ^ 2;

      auto best = (unsigned) 0;
      auto diff = (unsigned) 0;

      for (unsigned i = 0; i < n; ++i)
        {

          if (i != v)
            best = std::max (best, M [i, i]);

          for (unsigned j = 0; j < i; ++j) if (A [i, j] != C [i, j])
            ++diff;
        }

    return { M [v, v], best, diff };
    }
};

template<std::random_engine<unsigned long> RandomEngine>
struct __optimize_popularity::uniform_distribution_
{

  unsigned n;

  inline uniform_distribution_ (unsigned _n) noexcept:
      n (_n)
    {
    }

  inline matrix<bool> operator() (RandomEngine& rng) const
    {

      matrix<bool> M (n, n);

      for (unsigned i = 0; i < n; ++i)
      for (unsigned j = 0; j < i; ++j)
        M [i, j] = M [j, i] = (bool) (std::uniform_int_distribution (0, 1)) (rng);
    return M;
    }
};

std::tuple<matrix<bool>, ticks_t, unsigned> optimize_popularity (const matrix<double>& A, const matrix<double>& B)
{

  if (A.get_cols () != A.get_rows ())
    throw wrap_stacktrace<std::invalid_argument> ("expected square operand");

  if (std::any_of (A.begin (), A.end (), [](double v) { return 0 != v && 1 != v; }) || A != (A ^ transpose))
    throw wrap_stacktrace<std::invalid_argument> ("expected adjacency operand");

  auto n = A.get_cols ();
  auto F = matrix<bool> (A);

  if (B.get_cols () != B.get_rows () || 1 != B.get_cols ())
    throw wrap_stacktrace<std::invalid_argument> ("expected scalar operand");

  auto v = static_cast<decltype (n)> (B [0, 0]);

  if (0 > v || v > A.get_cols ())
    throw wrap_stacktrace<std::invalid_argument> ("expected node number");

  constexpr auto Combinator = __optimize_popularity::combinator_<std::mt19937> ();
  using fitness_ranker_ = __optimize_popularity::fitness_ranker_;
  using uniform_distribution_ = __optimize_popularity::uniform_distribution_<std::mt19937>;

  genetic_optimization<matrix<bool>, double, std::vector, std::mt19937, Combinator> minimizer ({
      .mutation_rate = 0.2,
      .mutation_sigma = 0.8,
    });

  auto progress_bar = optimize_bar (minimizer.get_config ().generations);

  auto [ R, ticks ] = minimizer.find_best (fitness_ranker_ (F, n, v), uniform_distribution_ (n), progress_bar);
  auto [ _, _, diff ] = fitness_ranker_ (F, n, v).rank (R);
return { std::move (R), ticks, diff };
}