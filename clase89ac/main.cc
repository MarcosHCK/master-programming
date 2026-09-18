/* Copyright 2026 MarcosHCK
 */
#include <argparse/argparse.hpp>
#include <../clase567/exception.h>
#include <../clase567/matrix.h>
#include <../clase567/matrixfile.h>
#include <../clase567/matrixoperations.h>
#include <chrono>
#include <fstream>
#include <iostream>
using namespace argparse;
using namespace utility;

static int work (ArgumentParser& parser);

int main (int argc, char* argv[])
{

  ArgumentParser parser;

  parser.add_argument ("input")
        .help ("Input matrix file")
        .metavar ("IMAGE.txt");

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

  auto span = std::span<typename std::remove_cvref_t<_A_matrix>::value_type> (A.data (), A.get_rows () * A.get_cols ());

  auto max = std::ranges::max (std::views::all (span) | std::views::transform ([](auto e) noexcept -> size_t
                                { return std::remove_cvref_t<_A_matrix>::to_string_element (e).length (); }));
return max;
}

template<std::ranges::input_range Range>
  requires std::convertible_to<std::ranges::range_reference_t<Range>, std::string>
static inline std::generator<std::string> wrap_lines (Range range) noexcept
{

  size_t bigger = 0;

  for (const auto line: range)
    {
      bigger = std::max (bigger, line.length ());
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

  auto tuples = std::views::zip (std::views::iota ((decltype (max)) 0, max),
                                 wrap_lines (A.to_string_lines (biggest_number (A))),
                                 wrap_lines (B.to_string_lines (biggest_number (B))),
                                 wrap_lines (R.to_string_lines (biggest_number (R))));

  for (const auto [ i, line_a, line_b, line_r ]: tuples)
    {

      std::cout << line_a << (i != opp ? oph : " " + op + " ")
                << line_b << (i != opp ? eqh : std::string (" = "))
                << line_r << '\n';
    }
}

static int work (ArgumentParser& parser)
{

  auto file = parser.get<std::string> ("input");
  auto stream = std::ifstream (file, std::ios::in);

  if (! stream)
    throw wrap_stacktrace<std::invalid_argument> ("cannot open file " + file);

  auto [ A, B, op ] = matrix_file::load_operation<double> (stream);

  if (1 != op.length ())
    throw wrap_stacktrace<std::invalid_argument> ("invalid operation " + op);

# define MEASURE(...) ({ \
  ; \
    auto __start = std::chrono::steady_clock::now (); \
    auto __value = ((__VA_ARGS__)); \
    auto __stop = std::chrono::steady_clock::now (); \
    std::cout << "took " << std::chrono::duration_cast<std::chrono::microseconds> (__stop - __start) << '\n'; \
    __value; \
  })

  switch (op [0])
    {

    case '+': print_operation (A, B, op, MEASURE (A + B));
      break;

    case '-': print_operation (A, B, op, MEASURE (A - B));
      break;

    case '*': print_operation (A, B, op, MEASURE (A * B));
      break;

    case '/': print_operation (A, B, op, MEASURE (A * (B ^ inverse)));
      break;

    case '%': print_operation (A, B, op, MEASURE ((B ^ inverse) * A));
      break;

    case '^': if (B.get_cols () != B.get_rows () || 1 != B.get_cols ())
                throw wrap_stacktrace<std::invalid_argument> ("expected scalar operator");
              print_operation (A, B, op, MEASURE (A ^ B [0, 0]));
      break;

    default:
      throw wrap_stacktrace<std::invalid_argument> ("invalid operation " + op);
    }
#undef MEASURE
return 0;
}