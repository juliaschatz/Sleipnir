// Copyright (c) Sleipnir contributors

#include <cmath>

#include <fmt/core.h>
#include <sleipnir/optimization/OptimizationProblem.hpp>
#include <units/time.h>

#ifndef RUNNING_TESTS
int main() {
  constexpr auto T = 5_s;
  constexpr units::second_t dt = 5_ms;
  constexpr int N = T / dt;

  // Flywheel model:
  // States: [velocity]
  // Inputs: [voltage]
  Eigen::Matrix<double, 1, 1> A{std::exp(-dt.value())};
  Eigen::Matrix<double, 1, 1> B{1.0 - std::exp(-dt.value())};

  sleipnir::OptimizationProblem problem;
  auto X = problem.DecisionVariable(1, N + 1);
  auto U = problem.DecisionVariable(1, N);

  // Dynamics constraint
  for (int k = 0; k < N; ++k) {
    problem.SubjectTo(X.Col(k + 1) == A * X.Col(k) + B * U.Col(k));
  }

  // State and input constraints
  problem.SubjectTo(X.Col(0) == 0.0);
  problem.SubjectTo(-12 <= U);
  problem.SubjectTo(U <= 12);

  // Cost function - minimize error
  Eigen::Matrix<double, 1, 1> r{10.0};
  sleipnir::VariableMatrix J = 0.0;
  for (int k = 0; k < N + 1; ++k) {
    J += (r - X.Col(k)).T() * (r - X.Col(k));
  }
  problem.Minimize(J);

  problem.Solve();

  // The first state
  fmt::print("x₀ = {}\n", X.Value(0, 0));

  // The first input
  fmt::print("u₀ = {}\n", U.Value(0, 0));
}
#endif
