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

namespace __optimize_markov
{

  template<std::random_engine<unsigned long> RandomEngine>
  struct combinator_;

  struct fitness_ranker_;

  template<std::random_engine<unsigned long> RandomEngine>
  struct uniform_distribution_;
}

template<std::random_engine<unsigned long> RandomEngine>
struct __optimize_markov::combinator_: default_mutator<RandomEngine>, sbx_combine<RandomEngine>
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

  inline std::pair<matrix<bool>, matrix<bool>> operator() (RandomEngine& rng, config_t& config, const matrix<bool>& x, const matrix<bool>& y) const
    {

      auto&& xr = std::views::all (x)
                | std::views::transform ([] (auto v) noexcept { return static_cast<double> (v); });

      auto&& yr = std::views::all (y)
                | std::views::transform ([] (auto v) noexcept { return static_cast<double> (v); });

      auto&& [ Xr, Yr ] = sbx_combine<RandomEngine>::operator() (rng, config, std::move (xr), std::move (yr));

      auto&& Xrr = default_mutator<RandomEngine>::operator() (rng, config, std::move (Xr), bounds_range ())
                 | std::views::transform ([](auto v) noexcept { return v > 0.5; });

      auto&& Yrr = default_mutator<RandomEngine>::operator() (rng, config, std::move (Yr), bounds_range ())
                 | std::views::transform ([](auto v) noexcept { return v > 0.5; });

    return { matrix<bool> (x.get_rows (), x.get_cols (), std::move (Xrr)), matrix<bool> (y.get_rows (), y.get_cols (), std::move (Yrr)) };
    }
};

struct __optimize_markov::fitness_ranker_
{

  const matrix<bool>& A;
  const matrix<double>& P;
  unsigned n, v;

  inline fitness_ranker_ (const matrix<bool>& _A, const matrix<double>& _P, unsigned _n, unsigned _v) noexcept:
      A (_A), P (_P), n (_n), v (_v)
    {}

  inline double operator() (const matrix<bool>& C)
    {

      auto [ own, best, diff ] = rank (C);
    return (own > best ? own - best : 2 + (best - own)) + (double) diff;
    }

  inline std::tuple<double, double, unsigned> rank (const matrix<bool>& C) const
    {

      auto M = to_probabilities (C) * P;

      auto best = (double) 0;
      auto diff = (unsigned) 0;

      for (unsigned i = 0; i < n; ++i)
        {

          if (i != v)
            best = std::max (best, M [i, 0]);

          for (unsigned j = 0; j < n; ++j) if (A [i, j] != C [i, j])
            ++diff;
        }

    return { M [v, 0], best, diff };
    }
};

template<std::random_engine<unsigned long> RandomEngine>
struct __optimize_markov::uniform_distribution_
{

  unsigned n;

  inline uniform_distribution_ (unsigned _n) noexcept:
      n (_n)
    {
    }

  inline matrix<bool> operator() (RandomEngine& rng) const
    {

      matrix<bool> M (n, n);

      for (auto it = M.begin (); it != M.end (); ++it)
        (*it) = (bool) (std::uniform_int_distribution (0, 1)) (rng);
    return M;
    }
};

matrix<double> to_probabilities (const matrix<bool>& A)
{

  auto&& seed = std::views::all (A ^ transpose)
              | std::views::chunk (A.get_cols ())
              | std::views::transform ([](auto&& p) noexcept
                  { auto ones = std::count_if (p.begin (), p.end (), [](auto e) noexcept { return e; });
                    auto prob = 1.0 / (double) ones;
                    return std::views::all (p) | std::views::transform ([prob] (auto e) noexcept { return !e ? 0 : prob; }); })
              | std::views::join;

return matrix<double> (A.get_rows (), A.get_cols (), std::move (seed)) ^ transpose;
}

std::tuple<matrix<bool>, ticks_t, unsigned> optimize_markov (const matrix<double>& A, const matrix<double>& B)
{

  if (A.get_cols () != A.get_rows ())
    throw wrap_stacktrace<std::invalid_argument> ("expected square operand");

  if (std::any_of (A.begin (), A.end (), [](double v) { return 0 != v && 1 != v; }))
    throw wrap_stacktrace<std::invalid_argument> ("expected adjacency operand");

  auto n = A.get_cols ();
  auto F = matrix<bool> (A);

  if (A.get_rows () != B.get_rows ())
    throw wrap_stacktrace<std::invalid_argument> ("invalid probability vector");

  if (2 != B.get_cols ())
    throw wrap_stacktrace<std::invalid_argument> ("invalid chain descriptor");

  auto P = matrix<double> (B.get_rows (), 1, B ^ transpose);
  auto v = B [0, 1];

  if (v != std::floor (v) || v < 0 || v >= A.get_cols ())
    throw wrap_stacktrace<std::invalid_argument> ("invalid round trip number");

  constexpr auto Combinator = __optimize_markov::combinator_<std::mt19937> ();
  using fitness_ranker_ = __optimize_markov::fitness_ranker_;
  using uniform_distribution_ = __optimize_markov::uniform_distribution_<std::mt19937>;

  genetic_optimization<matrix<bool>, double, std::vector, std::mt19937, Combinator> minimizer ({
      .mutation_rate = 0.2,
      .mutation_sigma = 0.8,
    });

  auto progress_bar = optimize_bar (minimizer.get_config ().generations);

  auto [ R, ticks ] = minimizer.find_best (fitness_ranker_ (F, P, n, v), uniform_distribution_ (n), progress_bar);
  auto [ _, _, diff ] = fitness_ranker_ (F, P, n, v).rank (R);
return { std::move (R), ticks, diff };
}