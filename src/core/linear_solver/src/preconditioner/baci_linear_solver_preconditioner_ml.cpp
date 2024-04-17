/*----------------------------------------------------------------------*/
/*! \file

\brief Interface class to ML preconditoner

\brief Declaration

\level 0

*/
/*----------------------------------------------------------------------*/

#include "baci_linear_solver_preconditioner_ml.hpp"

#include "baci_linalg_utils_sparse_algebra_math.hpp"
#include "baci_utils_exceptions.hpp"

#include <ml_common.h>
#include <ml_epetra.h>
#include <ml_epetra_operator.h>
#include <ml_epetra_utils.h>
#include <ml_include.h>
#include <ml_MultiLevelPreconditioner.h>

FOUR_C_NAMESPACE_OPEN

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
CORE::LINEAR_SOLVER::MLPreconditioner::MLPreconditioner(Teuchos::ParameterList& mllist)
    : mllist_(mllist)
{
}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
void CORE::LINEAR_SOLVER::MLPreconditioner::Setup(
    bool create, Epetra_Operator* matrix, Epetra_MultiVector* x, Epetra_MultiVector* b)
{
  SetupLinearProblem(matrix, x, b);

  if (create)
  {
    Epetra_CrsMatrix* A = dynamic_cast<Epetra_CrsMatrix*>(matrix);
    if (A == nullptr) dserror("CrsMatrix expected");

    // free old matrix first
    P_ = Teuchos::null;
    Pmatrix_ = Teuchos::null;

    // create a copy of the scaled matrix
    // so we can reuse the preconditioner
    Pmatrix_ = Teuchos::rcp(new Epetra_CrsMatrix(*A));

    mllist_.remove("init smoother", false);

    P_ = Teuchos::rcp(new ML_Epetra::MultiLevelPreconditioner(*Pmatrix_, mllist_, true));
  }
}

FOUR_C_NAMESPACE_CLOSE
