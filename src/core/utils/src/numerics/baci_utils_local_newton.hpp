/*----------------------------------------------------------------------*/
/*! \file

\brief Implementation and helper functions for local Newton methods.

\level 2
*/
/*----------------------------------------------------------------------*/
#ifndef FOUR_C_UTILS_LOCAL_NEWTON_HPP
#define FOUR_C_UTILS_LOCAL_NEWTON_HPP

#include "baci_config.hpp"

#include "baci_linalg_fixedsizematrix.hpp"
#include "baci_utils_fad.hpp"

#include <functional>

FOUR_C_NAMESPACE_OPEN

namespace CORE::UTILS
{
  constexpr double LOCAL_NEWTON_DEFAULT_TOLERANCE = 1e-12;
  constexpr unsigned LOCAL_NEWTON_DEFAULT_MAXIMUM_ITERATIONS = 50;

  /// @brief Free functions defining a Newton iterations for different scalar, vector and jacobian
  /// types.
  ///
  /// @note These functions are overloaded for common types. If you need to overload this function
  /// for your own types, you may do so in the same namespace that also contains your type.
  /// @{
  template <typename ScalarType>
  void LocalNewtonIteration(ScalarType& x, const ScalarType residuum, const ScalarType jacobian)
  {
    x -= residuum / jacobian;
  }

  template <unsigned N, typename ScalarType>
  void LocalNewtonIteration(CORE::LINALG::Matrix<N, 1, ScalarType>& x,
      const CORE::LINALG::Matrix<N, 1, ScalarType>& residuum,
      CORE::LINALG::Matrix<N, N, ScalarType>&& jacobian)
  {
    jacobian.Invert();
    x.MultiplyNN(-1, jacobian, residuum, 1.0);
  }
  /// @}

  /// @brief Free functions defining a to compute the L2-norm of the used Vector Type
  ///
  /// @note These functions are overloaded for common types. If you need to overload this function
  /// for your own types, you may do so in the same namespace that also contains your type.
  /// @{
  template <typename DefaultAndFADScalarType>
  DefaultAndFADScalarType L2Norm(const DefaultAndFADScalarType& x)
  {
    return CORE::FADUTILS::Norm(x);
  }

  template <unsigned N, typename ScalarType>
  ScalarType L2Norm(const CORE::LINALG::Matrix<N, 1, ScalarType>& x)
  {
    return CORE::FADUTILS::VectorNorm(x);
  }
  /// @}

  /*!
   * @brief Finds the root of a function (scalar or vector valued) using the Newton-Raphson method
   * starting from the initial guess @p x_0.
   *
   * @note In order that this function works well, you may need to overload the functions
   * `ScalarType L2Norm(const VectorType&)` that computes the L2-norm of the used vector type, and
   * `void LocalNewtonIteration(VectorType& x, const VectorType& residuum, JacobianType&&
   * jacobian_of_residuum)` that does a local Newton step. For often used parameters (double,
   * CORE::LINALG::Matrix and FAD-types), these overloads are already implemented.
   *
   * @note The jacobian at the root is often needed to compute the linearization of the
   * Newton-Raphson method w.r.t the primary variables. @p SolveLocalNewtonAndReturnJacobian returns
   * the derivative of the residuum w.r.t. the unknown parameter x, i.e. \f$\frac{\partial
   * \boldsymbol{R}}{\partial \boldsymbol{x}}\f$. For the linearization of your method, you usually
   * need the derivative of x w.r.t. the primary variable u. This can be computed via
   * \f$\frac{\partial \boldsymbol{x}}{\partial \boldsymbol{u}} = (\frac{\partial
   * \boldsymbol{R}}{\partial \boldsymbol{x}})^{-1} \frac{\partial \boldsymbol{R}}{\partial
   * \boldsymbol{u}}\f$, where \f$\frac{\partial \boldsymbol{R}}{\partial \boldsymbol{u}}\f$ is the
   * partial derivative of the residuum R w.r.t. the primary variable u. If you don't need to
   * linearize your function, you can use @p SolveLocalNewton which does not return the
   * linearization.
   *
   * You can use the function as follows:
   *
   * Example 1 (scalar-valued double function):
   * @code
   * double x_0 = 0.0;
   * auto [x, jacobian] = CORE::UTILS::SolveLocalNewtonAndReturnJacobian(x_0, [](double x) {
   *     return std::make_tuple<double, double>({std::pow(x, 2), 2*x});
   *   }, 1e-9);
   * @endcode
   *
   * Example 2 (vector-valued double function):
   * @code
   * CORE::LINALG::Matrix<2,1> x_0(true);
   * auto [x, jacobian] = CORE::UTILS::SolveLocalNewtonAndReturnJacobian(x_0,
   * [](CORE::LINALG::Matrix<2,1> x) { return std::make_tuple<CORE::LINALG::Matrix<2,1>,
   * CORE::LINALG::Matrix<2,2>>({ CORE::LINALG::Matrix<2,1>{true}, // define your function here
   *       CORE::LINALG::Matrix<2,2>{true} // define your jacobian here
   *     });
   *   }, 1e-9);
   * @endcode
   *
   * Example 3 (Custom type double function):
   * @code
   * namespace {
   *   void LocalNewtonIteration(MyVectorType& x, MyVectorType residuum, MyJacobianType
   * jacobian)
   *   {
   *     // define your Newton update here
   *   }
   *
   *   double L2Norm(const MyVectorType& x)
   *   {
   *      // define norm of vector here
   *   }
   * }
   *
   * double x_0 = MyVectorType{...}; // initial value
   * auto [x, jacobian] = CORE::UTILS::SolveLocalNewtonAndReturnJacobian(x_0, [](MyVectorType x) {
   *     return std::make_tuple<MyVectorType, MyJacobianType>({
   *       MyVectorType{...}, // define your function here
   *       MyJacobianType{...} // define your jacobian here
   *     });
   *   }, 1e-9);
   * @endcode
   *
   * @tparam ScalarType The type of the scalar used within method (type of tolerance or norm of
   * residuum).
   * @tparam VectorType The type of the residuum and the unknowns.
   * @tparam ResiduumAndJacobianEvaluator A class that defines the operator() with the signature
   * std::tuple<VectorType, JacobianType>(VectorType) that evaluates the residuum and it's
   * jacobian at a specific point.
   * @param residuum_and_jacobian_evaluator A function object that evaluates the residuum and it's
   * jacobian at a specific point.
   * @param x_0 Initial guess for the solution
   * @param tolerance The tolerance that is used for a convergence criterion.
   * @param max_iterations Maximum allowed number of newton iterations
   * @return Returns x such that the residuum is smaller than the given tolerance. A pair containing
   * the final result and the jacobian. The jacobian is often needed to compute the linearization of
   * the local Newton method.
   */
  template <typename ScalarType, typename VectorType, typename ResiduumAndJacobianEvaluator>
  auto SolveLocalNewtonAndReturnJacobian(
      ResiduumAndJacobianEvaluator residuum_and_jacobian_evaluator, VectorType x_0,
      const ScalarType tolerance = LOCAL_NEWTON_DEFAULT_TOLERANCE,
      const unsigned max_iterations = LOCAL_NEWTON_DEFAULT_MAXIMUM_ITERATIONS)
      -> std::tuple<VectorType,
          std::tuple_element_t<1, decltype(residuum_and_jacobian_evaluator(x_0))>>
  {
    auto [residuum, jacobian] = residuum_and_jacobian_evaluator(x_0);

    unsigned iteration = 0;
    while (L2Norm(residuum) > tolerance)
    {
      if (iteration > max_iterations)
      {
        dserror(
            "The local Newton method did not converge within %d iterations. Residuum is %.3e > "
            "%.3e.",
            max_iterations, FADUTILS::CastToDouble(L2Norm(residuum)),
            FADUTILS::CastToDouble(tolerance));
      }

      LocalNewtonIteration(x_0, residuum, std::move(jacobian));

      std::tie(residuum, jacobian) = residuum_and_jacobian_evaluator(x_0);

      ++iteration;
    }

    return {x_0, jacobian};
  }

  /*!
   * @brief Finds the root of a function (scalar or vector valued) using the Newton-Raphson method
   * starting from the initial guess @p x_0.
   *
   * @note in contrast to @p SolveLocalNewtonAndReturnJacobian, this function does not return the
   * jacobian at the root of the function. The remaining syntax is identical.
   *
   * @tparam ScalarType The type of the scalar used within method (type of tolerance or norm of
   * residuum).
   * @tparam VectorType The type of the residuum and the unknowns.
   * @tparam ResiduumAndJacobianEvaluator A class that defines the operator() with the signature
   * std::tuple<VectorType, JacobianType>(VectorType) that evaluates the residuum and it's
   * jacobian at a specific point.
   * @param residuum_and_jacobian_evaluator A function object that evaluates the residuum and it's
   * jacobian at a specific point.
   * @param x_0 Initial guess for the solution
   * @param tolerance The tolerance that is used for a convergence criterion.
   * @param max_iterations Maximum allowed number of newton iterations
   * @return Returns x such that the residuum is smaller than the given tolerance.
   */
  template <typename ScalarType, typename VectorType, typename ResiduumAndJacobianEvaluator>
  auto SolveLocalNewton(ResiduumAndJacobianEvaluator residuum_and_jacobian_evaluator,
      VectorType x_0, const ScalarType tolerance = LOCAL_NEWTON_DEFAULT_TOLERANCE,
      const unsigned max_iterations = LOCAL_NEWTON_DEFAULT_MAXIMUM_ITERATIONS) -> VectorType
  {
    return std::get<0>(SolveLocalNewtonAndReturnJacobian(
        residuum_and_jacobian_evaluator, x_0, tolerance, max_iterations));
  }

}  // namespace CORE::UTILS

FOUR_C_NAMESPACE_CLOSE

#endif
