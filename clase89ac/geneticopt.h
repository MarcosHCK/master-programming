/* Copyright 2026 MarcosHCK
 */
#pragma once
#include <algorithm>
#include <execution>
#include <mutex>
#include <random>
#include <ranges>
#include <vector>

namespace std
{

  template<template<typename> typename E,
           typename T>
  concept container = requires ()
    {

      typename E<T>::size_type;
      typename E<T>::value_type;

      requires std::is_same_v<T, typename E<T>::value_type>;

      requires requires (E<T> e, T&& v) { { e.push_back (v) } -> std::same_as<void>; }
            || requires (E<T> e, const T& v) { { e.push_back (v) } -> std::same_as<void>; };

      requires requires (E<T> e) { { e.size () } -> std::same_as<typename E<T>::size_type>; };

      requires requires (E<T> e, typename E<T>::size_type i) { { e [i] } -> std::same_as<T&>; };
    };

  template<template<typename> typename E,
           typename T>
  concept container_reserves = requires ()
    {

      requires container<E, T>;

      requires requires (E<T> e, typename E<T>::size_type s)
        {
          { e.reserve (s) } -> std::same_as<void>;
        };
    };

  template<typename T, typename Ret, typename... Args>
  concept invocable_r_v = requires ()
    {
      requires std::is_invocable_r_v<Ret, T, Args ...>;
    };

  template<typename E,
           typename T = unsigned>
  concept random_engine = requires (E e, uint64_t z)
    {

      typename E::result_type;
      requires std::same_as<T, typename E::result_type>;
      requires std::is_constructible_v<E, typename E::result_type>;
      requires std::is_invocable_r_v<typename E::result_type, E>;
      { e.discard (z) } -> std::same_as<void>;
    };
}

struct genetic_optimization_config
{

  double crossover_rate = 0.9;
  std::size_t elite_size = 2;
  std::size_t generations = 500;
  double mutation_rate = 0.05;
  double mutation_sigma = 0.1;
  std::size_t population_size = 200;
  std::size_t tournament_size = 3;
};

template<typename T, std::random_engine<unsigned long> RandomEngine> struct combinator;
template<typename T> struct fitness_ranker;
template<typename T, std::random_engine<unsigned long> RandomEngine> struct uniform_distribution;

struct progress_reporter { inline void operator() (int g) const noexcept { }; };

template<typename V = double,
         std::random_engine<unsigned long> RandomEngine = std::mt19937>
static inline V uniform01 (RandomEngine& rng)
  noexcept (std::is_nothrow_invocable_v<decltype (std::generate_canonical<V, -1u, RandomEngine>), RandomEngine&>)
{
  return std::generate_canonical<V, -1u> (rng);
}

template<std::random_engine<unsigned long> RandomEngine>
struct default_mutator
{

  using config = genetic_optimization_config;

  template<std::ranges::input_range Range1,
           std::ranges::input_range Range2>
    requires std::same_as<std::ranges::range_value_t<Range1>, double>
          && std::same_as<std::ranges::range_value_t<Range2>, std::pair<double, double>>
  inline auto operator() (RandomEngine& rng, config& config, Range1&& range, Range2&& bounds) const
      noexcept (std::is_nothrow_invocable_v<decltype (uniform01<double, RandomEngine>), RandomEngine&>)
    {

      constexpr bool noexcept_ = std::is_nothrow_invocable_v<decltype (uniform01<double, RandomEngine>), RandomEngine&>;

      auto mr = config.mutation_rate;
      auto ms = config.mutation_sigma;

      return std::views::zip (std::forward<Range1> (range), std::forward<Range2> (bounds))
           | std::views::transform ([&rng = rng, mr = mr, ms = ms](auto&& p) noexcept (noexcept_)
              { return uniform01 (rng) > mr ? std::get<0> (p) : combine (rng, ms, std::get<0> (p), std::get<1> (p)); });
    }

private:

  static inline double combine (RandomEngine& rng, double sigma, double x, std::pair<double, double> bounds)
      noexcept (std::is_nothrow_invocable_v<decltype (uniform01<double, RandomEngine>), RandomEngine&>)
    {

      auto [ l, h ] = bounds;
      auto dist = std::normal_distribution<double> (0.0, sigma * (h - l));
    return std::clamp (x + dist (rng), l, h);
    }
};

template<std::random_engine<unsigned long> RandomEngine>
struct sbx_combine
{

  using config = genetic_optimization_config;

  template<std::ranges::forward_range Range1,
           std::ranges::forward_range Range2>
    requires std::same_as<std::ranges::range_value_t<Range1>, double>
          && std::same_as<std::ranges::range_value_t<Range2>, double>
  inline auto operator() (RandomEngine& rng, config& config, Range1&& range1, Range2&& range2) const
      noexcept (std::is_nothrow_invocable_v<decltype (uniform01<double, RandomEngine>), RandomEngine&>)
    {

      auto cross = uniform01 (rng) > config.crossover_rate;
      auto&& result1 = combine_n<0> (rng, cross, range1, range2);
      auto&& result2 = combine_n<1> (rng, cross, range1, range2);
    return std::make_pair (std::move (result1), std::move (result2));
    }

private:

  static inline std::pair<double, double> combine (RandomEngine& rng, double x, double y)
      noexcept (std::is_nothrow_invocable_v<decltype (uniform01<double, RandomEngine>), RandomEngine&>)
    {

      constexpr double eta = 15.0;

      double u = uniform01 (rng),
             b = (0.5 >= u) ? std::pow (2.0 * u, 1.0 / (1.0 + eta))
                            : std::pow (1.0 / (2.0 * (1.0 - u)), 1.0 / (1.0 + eta));

    return { 0.5 * ((1 + b) * x + (1 - b) * y), 0.5 * ((1 - b) * x + (1 + b) * y) };
    }

  template<int N,
           std::ranges::input_range Range1,
           std::ranges::input_range Range2>
    requires std::same_as<std::ranges::range_reference_t<Range1>, double>
          && std::same_as<std::ranges::range_reference_t<Range2>, double>
  static inline auto combine_n (RandomEngine& rng, bool cross, Range1&& range1, Range2&& range2)
      noexcept (std::is_nothrow_invocable_v<decltype (uniform01<double, RandomEngine>), RandomEngine&>)
    {

      constexpr bool noexcept_ = std::is_nothrow_invocable_v<decltype (uniform01<double, RandomEngine>), RandomEngine&>;

      return std::views::zip (std::forward<Range1> (range1), std::forward<Range2> (range2))
           | std::views::transform ([cross = cross, &rng = rng] (auto p) noexcept (noexcept_)
              { return (cross || uniform01 (rng) > 0.5) ? std::get<N> (p) : std::get<N> (combine (rng, std::get<0> (p), std::get<1> (p))); });
    }
};

template<typename T,
         typename Rank = double,
         template<typename> typename Container = std::vector,
         std::random_engine<unsigned long> RandomEngine = std::mt19937,
         std::invocable_r_v<std::pair<T, T>, RandomEngine&, genetic_optimization_config&, const T&, const T&> auto Combinator = combinator<T, RandomEngine> ()>
  requires std::container<Container, T> && std::is_default_constructible_v<Rank>
class genetic_optimization
{

  genetic_optimization_config _config;

  template<std::invocable_r_v<bool, typename Container<T>::size_type, typename Container<T>::size_type> Comparer>
  static inline void best_order (std::vector<typename Container<T>::size_type>& order, Comparer&& comparer)
      noexcept (std::is_nothrow_invocable_r_v<bool, Comparer, typename Container<T>::size_type, typename Container<T>::size_type>)
    {

      constexpr bool noexcept_ = std::is_nothrow_invocable_r_v<bool, Comparer, typename Container<T>::size_type, typename Container<T>::size_type>;

      std::iota (order.begin (), order.end (), 0);
      std::sort (order.begin (), order.end (), [&, comparer = std::forward<Comparer> (comparer)] (auto i, auto j) noexcept (noexcept_)
        { return comparer (i, j); });
    }

  template<std::invocable_r_v<bool, typename Container<T>::size_type, typename Container<T>::size_type> Comparer,
           typename result_type = typename Container<T>::size_type>
  inline typename Container<T>::size_type tournament (RandomEngine& rng, Comparer&& comparer)
      noexcept (std::is_nothrow_constructible_v<std::uniform_int_distribution<result_type>, result_type, result_type>
             && std::is_nothrow_invocable_r_v<result_type, std::uniform_int_distribution<result_type>, RandomEngine&>)
    {

      auto dist = std::uniform_int_distribution<typename Container<T>::size_type> (0, _config.tournament_size - 1);
      auto best = dist (rng);

      for (decltype (_config.tournament_size) i = 0; i < _config.tournament_size; ++i)
        {

          if (auto trial = dist (rng); true == comparer (trial, best))
            best = trial;
        }
    return best;
    }

public:

  using value_type = T;

  inline constexpr genetic_optimization (genetic_optimization_config config = {}) noexcept: _config (config)
    { }

  inline constexpr auto& get_config () const noexcept { return _config; }

  template<std::invocable_r_v<Rank, const T&> FitnessRanker,
           std::invocable_r_v<T, RandomEngine&> UniformDistribution,
           std::invocable<int> ProgressReporter = progress_reporter>
  inline T find_best (FitnessRanker&& ranker = fitness_ranker<T> (),
                      UniformDistribution&& uniform_generator = uniform_distribution<T, RandomEngine> (),
                      ProgressReporter&& progress_reporter = ::progress_reporter ())
    {

      auto rng_seed = (std::random_device { }) ();

      return find_best (rng_seed, std::forward<FitnessRanker> (ranker), std::forward<UniformDistribution> (uniform_generator),
                                  std::forward<ProgressReporter> (progress_reporter));
    }

  template<std::invocable_r_v<Rank, const T&> FitnessRanker,
           std::invocable_r_v<T, RandomEngine&> UniformDistribution,
           std::invocable<int> ProgressReporter = progress_reporter>
  inline T find_best (typename RandomEngine::result_type rng_seed,
                      FitnessRanker&& ranker = fitness_ranker<T> (),
                      UniformDistribution&& uniform_generator = uniform_distribution<T, RandomEngine> (),
                      ProgressReporter&& progress_reporter = ::progress_reporter ())
    {

      RandomEngine rng (rng_seed);

      return find_best (rng, std::forward<FitnessRanker> (ranker), std::forward<UniformDistribution> (uniform_generator),
                             std::forward<ProgressReporter> (progress_reporter));
    }

  template<std::invocable_r_v<Rank, const T&> FitnessRanker,
           std::invocable_r_v<T, RandomEngine&> UniformDistribution,
           std::invocable<int> ProgressReporter = progress_reporter>
  inline T find_best (RandomEngine& rng,
                      FitnessRanker&& ranker = fitness_ranker<T> (),
                      UniformDistribution&& uniform_generator = uniform_distribution<T, RandomEngine> (),
                      ProgressReporter&& progress_reporter = ::progress_reporter ())
    {

      Container<T> population, swap_population;

      auto elite_size = static_cast<Container<T>::size_type> (_config.elite_size);
      auto population_size = static_cast<Container<T>::size_type> (_config.population_size);

      if constexpr (std::container_reserves<Container, T>)
        {
          population.reserve (population_size);
          swap_population.reserve (population_size);
        }

      for (typename Container<T>::size_type i = 0; i < population_size; ++i)
        {

          population.push_back (uniform_generator (rng));

          if constexpr (std::is_default_constructible_v<T>)

            swap_population.push_back (T ());
          else
            swap_population.push_back (uniform_generator (rng));
        }

      std::vector<typename Container<T>::size_type> order (population_size);

      auto fitness = std::vector<Rank> (std::from_range_t { }, std::views::transform (population, ranker));
      auto swap_fitness = std::vector<Rank> (population_size);
      auto peasant_grounds = std::vector<typename Container<T>::size_type> ((population_size - elite_size + 1) / 2);

      using size_type = typename Container<T>::size_type;

      auto comparer = [&fitness = fitness] (size_type a, size_type b) noexcept
        { return fitness [a] < fitness [b]; };

      for (typename Container<T>::size_type i = elite_size, j = 0; i < population_size; i += 2)
        peasant_grounds [j++] = i;

      std::mutex global_rng_m;

      for (decltype (_config.generations) g = 0; g < _config.generations;
           ++g, fitness.swap (swap_fitness), population.swap (swap_population))
        {

          if constexpr (! std::same_as<::progress_reporter, ProgressReporter>)
            progress_reporter (g);

          best_order (order, comparer);

          for (typename Container<T>::size_type i = 0; i < elite_size; ++i)
            {
              swap_fitness [i] = fitness [order [i]];
              swap_population [i] = population [order [i]];
            }

          std::for_each (std::execution::par, peasant_grounds.begin (), peasant_grounds.end (), [&](auto i)
            {

              constexpr bool noexcept_ = std::is_nothrow_invocable_r_v<typename RandomEngine::result_type, RandomEngine>;

              thread_local RandomEngine local_rng = [&] noexcept (noexcept_)
                {
                  std::lock_guard lock (global_rng_m);
                  return RandomEngine (rng ());
                } ();

              auto& parent1 = population [tournament (local_rng, comparer)];
              auto& parent2 = population [tournament (local_rng, comparer)];
              auto [ child1, child2 ] = Combinator (local_rng, _config, parent1, parent2);

              swap_fitness [i] = ranker (child1);
              swap_population [i] = std::move (child1);

              if (auto j = 1 + i; j < population_size)
                {
                  swap_fitness [j] = ranker (child2);
                  swap_population [j] = std::move (child2);
                }
            });
        }
    return (best_order (order, comparer), std::move (population [order [0]]));
    }
};