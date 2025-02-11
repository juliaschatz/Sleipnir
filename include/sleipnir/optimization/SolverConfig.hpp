// Copyright (c) Sleipnir contributors

#pragma once

#include <chrono>
#include <limits>

namespace sleipnir {

/**
 * Solver configuration.
 */
struct SolverConfig {
  /// The solver will stop once the error is below this tolerance.
  double tolerance = 1e-6;

  /// The maximum number of solver iterations before returning a solution.
  int maxIterations = 500;

  /// The maximum elapsed wall clock time before returning a solution.
  std::chrono::duration<double> timeout{
      std::numeric_limits<double>::infinity()};

  /// Enables diagnostic prints.
  bool diagnostics = false;
};

}  // namespace sleipnir
