/*----------------------------------------------------------------------*/
/*! \file

\brief volume element


\level 2
*/
/*----------------------------------------------------------------------*/

#include "4C_bele_vele3.hpp"
#include "4C_lib_discret.hpp"
#include "4C_linalg_utils_sparse_algebra_math.hpp"

FOUR_C_NAMESPACE_OPEN



/*----------------------------------------------------------------------*
 |  evaluate the element (public)                            gammi 04/07|
 *----------------------------------------------------------------------*/
int DRT::ELEMENTS::Vele3::Evaluate(Teuchos::ParameterList& params,
    DRT::Discretization& discretization, std::vector<int>& lm,
    CORE::LINALG::SerialDenseMatrix& elemat1, CORE::LINALG::SerialDenseMatrix& elemat2,
    CORE::LINALG::SerialDenseVector& elevec1, CORE::LINALG::SerialDenseVector& elevec2,
    CORE::LINALG::SerialDenseVector& elevec3)
{
  return 0;
}


/*----------------------------------------------------------------------*
 |  do nothing (public)                                      u.may 05/09|
 |                                                                      |
 |  The function is just a dummy.                                       |
 *----------------------------------------------------------------------*/
int DRT::ELEMENTS::Vele3::evaluate_neumann(Teuchos::ParameterList& params,
    DRT::Discretization& discretization, CORE::Conditions::Condition& condition,
    std::vector<int>& lm, CORE::LINALG::SerialDenseVector& elevec1,
    CORE::LINALG::SerialDenseMatrix* elemat1)
{
  return 0;
}

FOUR_C_NAMESPACE_CLOSE