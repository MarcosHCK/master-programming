/* Copyright 2026 MarcosHCK
 */
#include <../clase567/exception.h>
#include <../clase89ac/geneticopt.h>
#include <../clase89ac/matrix.h>
#include <../clase89ac/matrixfile.h>
#include <../clase89ac/matrixoperations.h>
#include <../clase89ac/operations.stringlist.h>
#include <algorithm>
#include <argparse/argparse.hpp>
#include <chrono>
#include <fstream>
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>
#include <indicators/dynamic_progress.hpp>
#include <iostream>
using namespace argparse;
using namespace matrix_operations;
using namespace utility;

static int work (ArgumentParser& parser);

int main (int argc, char* argv[])
{

  ArgumentParser parser;

  parser.add_argument ("input")
        .help ("Input matrix file")
        .metavar ("example.txt");

  parser.add_argument ("--output")
        .help ("Output matrix file")
        .metavar ("output.txt");

  parser.add_argument ("--output-separator")
        .default_value (" ")
        .help ("Output matrix file separator")
        .metavar ("VALUE");

  try
    { parser.parse_args (argc, argv); }
  catch (const std::exception &excpt)
    { std::cerr << excpt.what () << std::endl;
      std::cerr << parser;
      return 1; }

  try
    { return work (parser); }
  catch (const with_stacktrace &excpt)
    {

      auto& trace = excpt.get_trace ();
      std::cerr << "Exception: " << excpt.what () << std::endl;
      std::cerr << "Stack trace (" << trace.size () << " frames):" << std::endl;

      for (const auto& frame: trace)
        std::cerr << "  " << frame << std::endl;
    }
return 1;
}

template<matrix_type _A_matrix>
static inline size_t biggest_number (_A_matrix&& A) noexcept
{

  auto max = std::ranges::max (std::views::all (A) | std::views::transform ([](auto e) noexcept -> size_t
                                { return std::remove_cvref_t<_A_matrix>::to_string_element (e).length (); }));
return max;
}

template<std::ranges::input_range Range>
  requires std::convertible_to<std::ranges::range_value_t<Range>, std::string>
static inline std::generator<std::string> wrap_lines (Range range) noexcept
{

  size_t bigger = 0;

  for (auto&& line: range)
    {
      bigger = std::max (bigger, (line = "| " + line + " |").length ());
      co_yield line;
    }

  while (true)
    {
      co_yield std::string (bigger, ' ');
    }
}

template<matrix_type _A_matrix,
         matrix_type _B_matrix,
         matrix_type _R_matrix>
static inline void print_operation (_A_matrix&& A, _B_matrix&& B, const std::string& op, _R_matrix&& R) noexcept
{

  auto max = std::max (A.get_rows (), std::max (B.get_rows (), R.get_rows ()));
  auto min = std::min (A.get_rows (), std::min (B.get_rows (), R.get_rows ()));
  auto opp = min >> 1;

  std::string eqh (3, ' ');
  std::string oph (2 + op.length (), ' ');

  auto&& tuples = std::views::zip (std::views::iota ((decltype (max)) 0, max),
                                   wrap_lines (A.to_string_lines (biggest_number (A))),
                                   wrap_lines (B.to_string_lines (biggest_number (B))),
                                   wrap_lines (R.to_string_lines (biggest_number (R))));

  for (const auto [ i, line_a, line_b, line_r ]: std::move (tuples))
    {

      std::cout << line_a << (i != opp ? oph : " " + op + " ")
                << line_b << (i != opp ? eqh : std::string (" = "))
                << line_r << '\n';
    }
}

template<matrix_type _A_matrix>
static inline void write_matrix (ArgumentParser& parser, _A_matrix&& A)
{

  if (! parser.present ("--output"))
    return;

  auto file = parser.get<std::string> ("--output");
  auto sep = parser.get<std::string> ("--output-separator");

  if (auto stream = std::ofstream (file, std::ios::out); ! stream)

    throw wrap_stacktrace<std::invalid_argument> ("cannot open file " + file);
  else
    return matrix_file::save_matrix<_A_matrix> (stream, std::forward<_A_matrix> (A), std::string_view (sep));
}

template<std::random_engine<unsigned long> RandomEngine>
struct combinator<matrix<bool>, RandomEngine>: default_mutator<RandomEngine>, sbx_combine<RandomEngine>
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

template<>
struct fitness_ranker<matrix<bool>>
{

  const matrix<bool>& A;
  unsigned n, v, fact;

  inline fitness_ranker (const matrix<bool>& _A, unsigned _n, unsigned _v) noexcept:
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
struct uniform_distribution<matrix<bool>, RandomEngine>
{

  unsigned n;

  inline uniform_distribution (unsigned _n) noexcept:
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

static inline matrix<bool> popularize (const matrix<bool>& F, unsigned v)
{

  genetic_optimization<matrix<bool>> minimizer ({
      .mutation_rate = 0.2,
      .mutation_sigma = 0.8,
    });

  auto progress_bar = std::make_unique<indicators::BlockProgressBar> (

      indicators::option::BarWidth { 80 },
      indicators::option::ForegroundColor { indicators::Color::white },
      indicators::option::ShowPercentage { true },
      indicators::option::FontStyles { std::vector { indicators::FontStyle::bold } },
      indicators::option::MaxProgress { minimizer.get_config ().generations }
    );

  indicators::DynamicProgress<indicators::BlockProgressBar> dynamic_bar (*progress_bar);
  indicators::show_console_cursor (false);

  dynamic_bar.set_option (indicators::option::HideBarWhenComplete { true });

  auto n = F.get_cols ();
  auto R = minimizer.find_best (fitness_ranker<matrix<bool>> (F, n, v), uniform_distribution<matrix<bool>, std::mt19937> (n),
                                [&bar = dynamic_bar](int g) { bar [0].tick (); });

return (dynamic_bar [0].mark_as_completed (), dynamic_bar.print_progress (), indicators::show_console_cursor (true), R);
}

static int work (ArgumentParser& parser)
{

  auto file = parser.get<std::string> ("input");
  auto stream = std::ifstream (file, std::ios::in);

  if (! stream)
    throw wrap_stacktrace<std::invalid_argument> ("cannot open file " + file);

  auto [ A, B, op ] = matrix_file::load_operation<double> (stream);

# define MEASURE(...) ({ \
  ; \
    auto __start = std::chrono::steady_clock::now (); \
    auto __value = ((__VA_ARGS__)); \
    auto __stop = std::chrono::steady_clock::now (); \
    std::cout << "took " << std::chrono::duration_cast<std::chrono::microseconds> (__stop - __start) << '\n'; \
    write_matrix (parser, __value); \
    __value; \
  })

  if (auto r = matrix_operations::details::id_lookup::lookup (op.c_str (), op.length ()); NULL == r)
    throw wrap_stacktrace<std::invalid_argument> ("invalid operation " + op);

  else switch (auto [ _, id ] = *r; id)
    {

    case matrix_operations::details::operation_id::PLUS: print_operation (A, B, op, MEASURE (A + B));
      break;

    case matrix_operations::details::operation_id::MINUS: print_operation (A, B, op, MEASURE (A - B));
      break;

    case matrix_operations::details::operation_id::MULTIPLY: print_operation (A, B, op, MEASURE (A * B));
      break;

    case matrix_operations::details::operation_id::LEFT_MULTIPLY: print_operation (A, B, op, MEASURE (B * A));
      break;

    case matrix_operations::details::operation_id::DIVIDE: print_operation (A, B, op, MEASURE (A * (B ^ inverse)));
      break;

    case matrix_operations::details::operation_id::LEFT_DIVIDE: print_operation (A, B, op, MEASURE ((B ^ inverse) * A));
      break;

    case matrix_operations::details::operation_id::SCALAR_POWER:
    
        if (B.get_cols () != B.get_rows () || 1 != B.get_cols ())
          throw wrap_stacktrace<std::invalid_argument> ("expected scalar operand");
        print_operation (A, B, op, MEASURE (A ^ B [0, 0]));
      break;

    case matrix_operations::details::operation_id::POPULARIZE: {

        if (A.get_cols () != A.get_rows ())
          throw wrap_stacktrace<std::invalid_argument> ("expected square operand");

        auto n = A.get_cols ();
        auto span = std::span<std::remove_cvref_t<decltype (*A.data ())>> (A.data (), A.get_cols () * A.get_rows ());

        if (std::any_of (span.begin (), span.end (), [](double v) { return 0 != v && 1 != v; }) || A != (A ^ transpose))
          throw wrap_stacktrace<std::invalid_argument> ("expected adjacency operand");

        if (B.get_cols () != B.get_rows () || 1 != B.get_cols ())
          throw wrap_stacktrace<std::invalid_argument> ("expected scalar operand");

        auto v = static_cast<decltype (n)> (B [0, 0]);

        if (0 > v || v > A.get_cols ())
          throw wrap_stacktrace<std::invalid_argument> ("expected node number");

        auto R = MEASURE (popularize (A, v));

        print_operation (A, B, op, matrix<double> (R));

        auto [ _, _, diff ] = (fitness_ranker<matrix<bool>> (A, n, v)).rank (R);
        matrix<unsigned> G (1, 1, std::vector<unsigned> { 2 });

        std::cout << "differences = " << diff << std::endl;
        print_operation (R, G, "^", (matrix<unsigned> (R)) ^ 2);

    } break;

    default:
      throw wrap_stacktrace<std::invalid_argument> ("invalid operation " + op);
    }
#undef MEASURE
return 0;
}