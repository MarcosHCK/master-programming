/* Copyright 2026 MarcosHCK
 */
#pragma once
#include <chrono>
#include <tuple>
#include <../clase89ac/matrix.h>

using ticks_t = std::chrono::duration<long, std::ratio<1, 1000000000>>;

std::tuple<matrix<bool>, ticks_t, unsigned> optimize_markov (const matrix<double>& A, const matrix<double>& B);
std::tuple<matrix<bool>, ticks_t, unsigned> optimize_popularity (const matrix<double>& A, const matrix<double>& B);
matrix<double> to_probabilities (const matrix<bool>& A);