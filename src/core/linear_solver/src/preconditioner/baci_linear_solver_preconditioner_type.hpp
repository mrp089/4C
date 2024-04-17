/*----------------------------------------------------------------------*/
/*! \file

\brief Declaration

\level 1

*/
/*----------------------------------------------------------------------*/
#ifndef FOUR_C_LINEAR_SOLVER_PRECONDITIONER_TYPE_HPP
#define FOUR_C_LINEAR_SOLVER_PRECONDITIONER_TYPE_HPP

#include "baci_config.hpp"

#include <Epetra_Comm.h>
#include <Epetra_CrsGraph.h>
#include <Epetra_CrsMatrix.h>
#include <Epetra_LinearProblem.h>
#include <Epetra_Map.h>
#include <Epetra_Operator.h>
#include <Epetra_Vector.h>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>

FOUR_C_NAMESPACE_OPEN

namespace CORE::LINEAR_SOLVER
{
  /// preconditioner base class
  /*!
     A KrylovSolver object needs one (or more) preconditioner objects. There
     are many possible preconditioners. A unified framework simplifies the
     solution process.
  */
  class PreconditionerType
  {
   public:
    /*!
       No setup is done upon construction, only the preconditioner object is
       created.
    */
    PreconditionerType() = default;

    /// virtual destruction
    virtual ~PreconditionerType() = default;

    /// linear problem created (managed) by this preconditioner
    /*!
       This is how the iterative solver sees the linear problem that needs to be solved.
    */
    virtual Epetra_LinearProblem& LinearProblem() { return lp_; }

    /*! \brief Support routine for setup
     *
     * Pass the components of the linear system on to the Epetra_LinearProblem.
     *
     * @param matrix Matrix of the linear problem
     * @param x Solution vector of the linear problem
     * @param b Right-hand side of the linear problem
     */
    void SetupLinearProblem(Epetra_Operator* matrix, Epetra_MultiVector* x, Epetra_MultiVector* b)
    {
      lp_.SetOperator(matrix);
      lp_.SetLHS(x);
      lp_.SetRHS(b);
    }

    /// Setup preconditioner with a given linear system.
    virtual void Setup(
        bool create, Epetra_Operator* matrix, Epetra_MultiVector* x, Epetra_MultiVector* b) = 0;

    /// Finish calculation after linear solve.
    /*!
      This is empty in most cases, however some preconditioners might want to
      scale the solution.
    */
    virtual void Finish(Epetra_Operator* matrix, Epetra_MultiVector* x, Epetra_MultiVector* b) {}

    /// linear operator used for preconditioning
    virtual Teuchos::RCP<Epetra_Operator> PrecOperator() const = 0;

    /// return name of sublist in paramterlist which contains parameters for preconditioner
    virtual std::string getParameterListName() const = 0;

   protected:
    //! a linear problem wrapper class used by Trilinos and for scaling of the system
    Epetra_LinearProblem lp_;
  };
}  // namespace CORE::LINEAR_SOLVER

FOUR_C_NAMESPACE_CLOSE

#endif
