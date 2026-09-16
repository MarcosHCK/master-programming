/* Copyright 2026 MarcosHCK
 */
#include "matrixfile.h"
#include <argparse/argparse.hpp>
#include <../clase567/exception.h>
#include <../clase567/matrix.h>
#include <../clase567/matrixfile.h>
#include <fstream>
#include <iostream>
#include <string>
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
      std::cerr << parser; }

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

static int work (ArgumentParser& parser)
{

  auto file = parser.get<std::string> ("input");
  auto stream = std::ifstream (file, std::ios::in);
  auto [ A, B, op ] = matrix_file::load_operation<double> (stream);
  std::cout << A.to_string () << '\n';
  std::cout << op << '\n';
  std::cout << B.to_string () << '\n';
return 0;
}