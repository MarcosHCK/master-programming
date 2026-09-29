/* Copyright 2026 MarcosHCK
 */
#include <../clase567/exception.h>
#include <../clase89ac/geneticopt.h>
#include <../clase89ac/matrix.h>
#include <../clase89ac/matrixfile.h>
#include <../clase89ac/matrixoperations.h>
#include <../clase89ac/operations.stringlist.h>
#include <../clase89ac/optimizations.h>
#include <algorithm>
#include <argparse/argparse.hpp>
#include <chrono>
#include <fstream>
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

static int work (ArgumentParser& parser)
{

  auto file = parser.get<std::string> ("input");
  auto stream = std::ifstream (file, std::ios::in);

  if (! stream)
    throw wrap_stacktrace<std::invalid_argument> ("cannot open file " + file);

  auto [ A, B, op ] = matrix_file::load_operation<double> (stream);

# define MEASURED(took,value) ({ \
  ; \
    std::cout << "took " << std::chrono::duration_cast<std::chrono::microseconds> ((took)) << '\n'; \
    write_matrix (parser, ((value))); \
  })
# define MEASURE(...) ({ \
  ; \
    auto __start = std::chrono::steady_clock::now (); \
    auto __value = ((__VA_ARGS__)); \
    auto __stop = std::chrono::steady_clock::now (); \
    MEASURED (__stop - __start, __value); \
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

      auto [ R, ticks, diff ] = optimize_popularity (A, B);
      MEASURED (ticks, R);

      print_operation (A, B, op, R);

      std::cout << "differences = " << diff << std::endl;
      print_operation (R, matrix<unsigned> (1, 1, std::vector<unsigned> { 2 }), "^", (matrix<unsigned> (R)) ^ 2);
    } break;

    case matrix_operations::details::operation_id::MARKOV_CHAIN: {

      auto [ R, ticks, diff ] = optimize_markov (A, B);
      MEASURED (ticks, R);

      auto C = to_probabilities (R);
      auto P = matrix<double> (B.get_rows (), 1, B ^ transpose);

      print_operation (A, B, op, R);

      std::cout << "differences = " << diff << std::endl;
      print_operation (R, P, "*", C * P);
    } break;

    default:
      throw wrap_stacktrace<std::invalid_argument> ("invalid operation " + op);
    }
#undef MEASURE
return 0;
}