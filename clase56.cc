/* Copyright 2026 MarcosHCK
 */
#include <argparse/argparse.hpp>
#include <chrono>
#include <exception.h>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>
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

static inline std::pair<std::string, double> split_b (std::string& line)
{

  std::string b_i;

  if (auto pos = line.find_first_of ('|'); pos != line.npos)

    { b_i = line.substr (1 + pos); line.erase (pos); }
  else
    throw wrap_stacktrace<std::runtime_error> ("badly formatted row line");

return { (trim (line), line), std::stod ((trim (b_i), b_i)) };
}

const auto space_rgx = std::regex (" ", std::regex_constants::optimize);

static inline std::pair<std::vector<double>, std::vector<double>> load_max (std::istream& stream)
{

  std::vector<double> row, rows;
  std::vector<double> b;
  decltype (row)::size_type last = 0;

  for (std::string line (1, 0); std::getline (stream, line);)
    {

      if (line.starts_with ("#") || line.size () == 0)
        continue;

      auto [ line_, b_i ] = split_b (line);

      b.push_back (b_i);

      std::sregex_token_iterator end;
      std::sregex_token_iterator iter (line_.begin (), line_.end (), space_rgx, -1);

      for (; iter != end; ++iter)
        {

          auto str = iter->str ();

          ltrim (line_);
          rtrim (line_);

          if (str.length () > 0)
            row.push_back (std::stod (str));
        }

      if (last == 0)
        rows.reserve (last = row.size ());

      else if (last != row.size ())
        throw wrap_stacktrace<std::runtime_error> ("incompatible amount of columns in a row");

      rows.insert (rows.end (), row.begin (), row.end ());
      row.clear ();
    }

  if (last == 0)
    throw wrap_stacktrace<std::runtime_error> ("unexpected empty matrix");

  if (last != rows.size () / last)
    throw wrap_stacktrace<std::runtime_error> ("unexpected rectangular matrix");

return { rows, b };
}

static inline std::vector<double> solve (std::vector<double> A, std::vector<double>& b)
{

  std::vector<double> p (b.size ());
  std::vector<double> y (b.size ());
  std::vector<double> x (b.size ());

  for (decltype (b.size ()) n = b.size (), i = 0; i < n; i++)
    p [i] = i;

  for (decltype (b.size ()) n = b.size (), k = 0; k < n; ++k)
    {

      auto maxRow = (decltype (b.size ())) k;
      auto maxVal = std::abs (A [k + n * p [k]]);

      for (decltype (n) i = k + 1; i < n; ++i)

        if (auto next = std::abs (A [k + n * p [i]]); maxVal < next)
          {
            maxVal = next;
            maxRow = i;
          }

      if (maxVal < 1e-15)
        {
          auto m = "Matrix is singular (maxVal " + std::to_string (maxVal) + ")";
          throw wrap_stacktrace<std::runtime_error> (m);
        }

      std::swap (p [k], p [maxRow]);

      for (decltype (n) i = k + 1; i < n; ++i)
        {

          auto factor = A [k + n * p [i]] / A [k + n * p[k]];

          A [k + n * p [i]] = factor;
          
          for (decltype (n) j = k + 1; j < n; ++j)
            A [j + n * p [i]] -= factor * A [j + n * p [k]];
        }
    }

  for (decltype (b.size ()) n = b.size (), i = 0; i < n; ++i)
    {

      double sum = 0.;

      for (size_t j = 0; j < i; j++)
        sum += A [j + n * p[i]] * y [j];

      y [i] = b [p[i]] - sum;
    }
  
  for (ssize_t n = b.size (), i = n - 1; i >= 0; --i)
    {

      double sum = 0.;

      for (decltype (n) j = i + 1; j < n; j++)
        sum += A [j + n * p[i]] * x [j];

      x [i] = (y [i] - sum) / A [i + n * p[i]];
    }
return x;
}

static int work (ArgumentParser& parser)
{

  auto file = parser.get<std::string> ("input");
  auto stream = std::ifstream (file, std::ios::in);

  auto [ A, b ] = load_max (stream);
  auto start = std::chrono::steady_clock::now ();

  auto x = solve (A, b);
  auto stop = std::chrono::steady_clock::now ();

  std::cout << "x = [ ";

  for (decltype (x.size ()) i = 0; i < x.size (); ++i)
    (i == 0 ? std::cout : std::cout << ", ") << x [i];

  std::cout << " ], took " << std::chrono::duration_cast<std::chrono::microseconds> (stop - start) << std::endl;
return 0;
}
