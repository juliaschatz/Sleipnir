// Copyright (c) Sleipnir contributors

#include "sleipnir/control/OCPSolver.hpp"

using namespace sleipnir;

FixedStepOCPSolver::FixedStepOCPSolver(int numStates, int numInputs,
                                       std::chrono::duration<double> dt,
                                       int numSteps,
                                       const DynamicsFunction& dynamics,
                                       DynamicsType dynamicsType,
                                       TranscriptionMethod method)
    : m_numStates(numStates),
      m_numInputs(numInputs),
      m_dt(dt),
      m_numSteps(numSteps),
      m_transcriptionMethod(method),
      m_dynamicsType(dynamicsType),
      m_dynamicsFunction(dynamics) {
  // u is numSteps + 1 so that the final constraintFunction evaluation works
  // Not sure what the best way to do this is
  m_U = DecisionVariable(m_numInputs, m_numSteps + 1);

  if (m_transcriptionMethod == TranscriptionMethod::kDirectTranscription) {
    m_X = DecisionVariable(m_numStates, m_numSteps + 1);
    ConstrainDirectTranscription();
  } else if (m_transcriptionMethod == TranscriptionMethod::kDirectCollocation) {
    m_X = DecisionVariable(m_numStates, m_numSteps + 1);
    ConstrainDirectCollocation();
  } else if (m_transcriptionMethod == TranscriptionMethod::kSingleShooting) {
    // In single-shooting the states aren't decision variables, but instead
    // depend on the input and previous states
    m_X = VariableMatrix(m_numStates, m_numSteps + 1);
    ConstrainSingleShooting();
  }
}

void FixedStepOCPSolver::ConstrainDirectCollocation() {
  if (m_dynamicsType != DynamicsType::kExplicitODE) {
    throw std::runtime_error("Direct Collocation requires an explicit ODE");
  }
  for (int i = 0; i < m_numSteps; ++i) {
    auto x_begin = X().Col(i);
    auto x_end = X().Col(i + 1);
    auto u_begin = U().Col(i);
    auto t_begin = m_dt * i;
    auto t_end = m_dt * (i + 1);
    auto t_c = t_begin + m_dt / 2.0;

    // Use u_begin on the end point as well because we are approaching a
    // discontinuity from the left
    auto f_begin = m_dynamicsFunction(t_begin, x_begin, u_begin);
    auto f_end = m_dynamicsFunction(t_end, x_end, u_begin);
    auto x_c =
        (x_begin + x_end) / 2.0 + (m_dt.count() / 8.0) * (f_begin - f_end);
    auto xprime_c = (-3.0 / (2.0 * m_dt.count())) * (x_begin - x_end) -
                    (f_begin + f_end) / 4.0;
    auto f_c = m_dynamicsFunction(t_c, x_c, u_begin);
    SubjectTo(f_c == xprime_c);
  }
}

void FixedStepOCPSolver::ConstrainDirectTranscription() {
  for (int i = 0; i < m_numSteps; ++i) {
    auto x_begin = X().Col(i);
    auto x_end = X().Col(i + 1);
    auto u = U().Col(i);
    auto t = m_dt * i;
    if (m_dynamicsType == DynamicsType::kExplicitODE) {
      SubjectTo(x_end == RK4<const DynamicsFunction&, VariableMatrix,
                             VariableMatrix, std::chrono::duration<double>>(
                             m_dynamicsFunction, x_begin, u, t, m_dt));
    } else if (m_dynamicsType == DynamicsType::kDiscrete) {
      SubjectTo(x_end == m_dynamicsFunction(t, x_begin, u));
    }
  }
}

void FixedStepOCPSolver::ConstrainSingleShooting() {
  for (int i = 0; i < m_numSteps; ++i) {
    auto x_begin = X().Col(i);
    auto x_end = X().Col(i + 1);
    auto u = U().Col(i);
    auto t = m_dt * i;
    if (m_dynamicsType == DynamicsType::kExplicitODE) {
      x_end = RK4<const DynamicsFunction&, VariableMatrix, VariableMatrix,
                  std::chrono::duration<double>>(m_dynamicsFunction, x_begin, u,
                                                 t, m_dt);
    } else if (m_dynamicsType == DynamicsType::kDiscrete) {
      x_end = m_dynamicsFunction(t, x_begin, u);
    }
  }
}

void FixedStepOCPSolver::ConstrainAlways(
    const FixedStepConstraintFunction& constraintFunction) {
  for (int i = 0; i < m_numSteps + 1; ++i) {
    auto x = X().Col(i);
    auto u = U().Col(i);
    constraintFunction(x, u);
  }
}

void FixedStepOCPSolver::ConstrainInitialState(
    const VariableMatrix& initialState) {
  SubjectTo(InitialState() == initialState);
}

void FixedStepOCPSolver::ConstrainFinalState(const VariableMatrix& finalState) {
  SubjectTo(FinalState() == finalState);
}
