/*-----------------------------------------------------------*/
/*! \file

\brief %NOX::NLN extension of the %::NOX::Epetra::LinearSystem.



\level 3

*/
/*-----------------------------------------------------------*/

#ifndef FOUR_C_SOLVER_NONLIN_NOX_LINEARSYSTEM_HPP
#define FOUR_C_SOLVER_NONLIN_NOX_LINEARSYSTEM_HPP

#include "baci_config.hpp"

#include "baci_solver_nonlin_nox_enum_lists.hpp"
#include "baci_solver_nonlin_nox_forward_decl.hpp"

#include <NOX_Epetra_Interface_Required.H>
#include <NOX_Epetra_LinearSystem.H>
#include <Teuchos_Time.hpp>

FOUR_C_NAMESPACE_OPEN

// Forward declaration
namespace CORE::LINALG
{
  class Solver;
  struct SolverParams;
  class SparseOperator;
  class SparseMatrix;
  class SerialDenseMatrix;
  class SerialDenseVector;
  class BlockSparseMatrixBase;
}  // namespace CORE::LINALG

namespace NOX
{
  namespace NLN
  {
    namespace Solver
    {
      class PseudoTransient;
    }  // namespace Solver
    namespace LinSystem
    {
      class PrePostOperator;
      class Scaling;
    }  // namespace LinSystem
    class LinearSystem : public ::NOX::Epetra::LinearSystem
    {
     public:
      typedef std::map<NOX::NLN::SolutionType, Teuchos::RCP<CORE::LINALG::Solver>> SolverMap;

     protected:
      //! Source of the RowMatrix if using a native preconditioner
      enum PreconditionerMatrixSourceType
      {
        UseJacobian,
        SeparateMatrix
      };

      enum PreconditionerType
      {
        None,
        Ifpack,
        NewIfpack,
        ML,
        UserDefined
      };

     public:
      //! Standard constructor with full functionality.
      LinearSystem(Teuchos::ParameterList& printParams, Teuchos::ParameterList& linearSolverParams,
          const SolverMap& solvers, const Teuchos::RCP<::NOX::Epetra::Interface::Required>& iReq,
          const Teuchos::RCP<::NOX::Epetra::Interface::Jacobian>& iJac,
          const Teuchos::RCP<CORE::LINALG::SparseOperator>& J,
          const Teuchos::RCP<::NOX::Epetra::Interface::Preconditioner>& iPrec,
          const Teuchos::RCP<CORE::LINALG::SparseOperator>& preconditioner,
          const ::NOX::Epetra::Vector& cloneVector,
          const Teuchos::RCP<::NOX::Epetra::Scaling> scalingObject);

      //! Constructor without scaling object
      LinearSystem(Teuchos::ParameterList& printParams, Teuchos::ParameterList& linearSolverParams,
          const SolverMap& solvers, const Teuchos::RCP<::NOX::Epetra::Interface::Required>& iReq,
          const Teuchos::RCP<::NOX::Epetra::Interface::Jacobian>& iJac,
          const Teuchos::RCP<CORE::LINALG::SparseOperator>& J,
          const Teuchos::RCP<::NOX::Epetra::Interface::Preconditioner>& iPrec,
          const Teuchos::RCP<CORE::LINALG::SparseOperator>& preconditioner,
          const ::NOX::Epetra::Vector& cloneVector);

      //! Constructor without preconditioner
      LinearSystem(Teuchos::ParameterList& printParams, Teuchos::ParameterList& linearSolverParams,
          const SolverMap& solvers, const Teuchos::RCP<::NOX::Epetra::Interface::Required>& iReq,
          const Teuchos::RCP<::NOX::Epetra::Interface::Jacobian>& iJac,
          const Teuchos::RCP<CORE::LINALG::SparseOperator>& J,
          const ::NOX::Epetra::Vector& cloneVector,
          const Teuchos::RCP<::NOX::Epetra::Scaling> scalingObject);

      //! Constructor without preconditioner and scaling object
      LinearSystem(Teuchos::ParameterList& printParams, Teuchos::ParameterList& linearSolverParams,
          const SolverMap& solvers, const Teuchos::RCP<::NOX::Epetra::Interface::Required>& iReq,
          const Teuchos::RCP<::NOX::Epetra::Interface::Jacobian>& iJac,
          const Teuchos::RCP<CORE::LINALG::SparseOperator>& J,
          const ::NOX::Epetra::Vector& cloneVector);

      //! reset the linear solver parameters
      void reset(Teuchos::ParameterList& p);

      //! reset PrePostOperator wrapper object
      void resetPrePostOperator(Teuchos::ParameterList& p);

      //! Evaluate the Jacobian
      bool computeJacobian(const ::NOX::Epetra::Vector& x) override;

      //! Evaluate the Jacobian and the right hand side based on the solution vector x at once.
      virtual bool computeFandJacobian(const ::NOX::Epetra::Vector& x, ::NOX::Epetra::Vector& rhs);

      bool computeCorrectionSystem(const enum NOX::NLN::CorrectionType type,
          const ::NOX::Abstract::Group& grp, const ::NOX::Epetra::Vector& x,
          ::NOX::Epetra::Vector& rhs);

      bool applyJacobianBlock(const ::NOX::Epetra::Vector& input,
          Teuchos::RCP<::NOX::Epetra::Vector>& result, unsigned rbid, unsigned cbid) const;

      bool applyJacobian(
          const ::NOX::Epetra::Vector& input, ::NOX::Epetra::Vector& result) const override;

      bool applyJacobianTranspose(
          const ::NOX::Epetra::Vector& input, ::NOX::Epetra::Vector& result) const override;

      bool applyJacobianInverse(Teuchos::ParameterList& linearSolverParams,
          const ::NOX::Epetra::Vector& input, ::NOX::Epetra::Vector& result) override;

      bool applyRightPreconditioning(bool useTranspose, Teuchos::ParameterList& linearSolverParams,
          const ::NOX::Epetra::Vector& input, ::NOX::Epetra::Vector& result) const override;

      bool createPreconditioner(const ::NOX::Epetra::Vector& x,
          Teuchos::ParameterList& linearSolverParams, bool recomputeGraph) const override;

      //! adjust the pseudo time step (using a least squares approximation)
      void adjustPseudoTimeStep(double& delta, const double& stepSize,
          const ::NOX::Epetra::Vector& dir, const ::NOX::Epetra::Vector& rhs,
          const NOX::NLN::Solver::PseudoTransient& ptcsolver);

      //! ::NOX::Epetra::Interface::Required accessor
      Teuchos::RCP<const ::NOX::Epetra::Interface::Required> getRequiredInterface() const;

      //! ::NOX::Epetra::Interface::Jacobian accessor
      Teuchos::RCP<const ::NOX::Epetra::Interface::Jacobian> getJacobianInterface() const;

      //! ::NOX::Epetra::Interface::Preconditioner accessor
      Teuchos::RCP<const ::NOX::Epetra::Interface::Preconditioner> getPrecInterface() const;

      /** \brief return the Jacobian range map
       *
       *  \param rbid  row block id
       *  \param cbid  column block id */
      const Epetra_Map& getJacobianRangeMap(unsigned rbid, unsigned cbid) const;

      /** \brief access the Jacobian block
       *
       *  \param rbid  row block id
       *  \param cbid  column block id */
      const CORE::LINALG::SparseMatrix& getJacobianBlock(unsigned rbid, unsigned cbid) const;

      /** \brief get a copy of the block diagonal
       *
       *  \param diag_bid  diagonal block id */
      Teuchos::RCP<Epetra_Vector> getDiagonalOfJacobian(unsigned diag_bid) const;

      /** \brief replace the diagonal of the diagonal block in the Jacobian
       *
       *  \param diag_bid  diagonal block id */
      void replaceDiagonalOfJacobian(const Epetra_Vector& new_diag, unsigned diag_bid);

      //! Returns Jacobian Epetra_Operator pointer
      Teuchos::RCP<const Epetra_Operator> getJacobianOperator() const override;

      /// return jacobian operator
      Teuchos::RCP<Epetra_Operator> getJacobianOperator() override;

      //! Returns the operator type of the jacobian
      const enum NOX::NLN::LinSystem::OperatorType& getJacobianOperatorType() const;

      //! Set the jacobian operator
      //! Derived function: Check if the input operator is a LINALG_SparseOperator
      void setJacobianOperatorForSolve(
          const Teuchos::RCP<const Epetra_Operator>& solveJacOp) override;

      //! Set the jacobian operator of this class
      void SetJacobianOperatorForSolve(
          const Teuchos::RCP<const CORE::LINALG::SparseOperator>& solveJacOp);

      Teuchos::RCP<::NOX::Epetra::Scaling> getScaling() override;

      void resetScaling(const Teuchos::RCP<::NOX::Epetra::Scaling>& scalingObject) override;

      bool destroyPreconditioner() const override;

      bool recomputePreconditioner(const ::NOX::Epetra::Vector& x,
          Teuchos::ParameterList& linearSolverParams) const override;

      ::NOX::Epetra::LinearSystem::PreconditionerReusePolicyType getPreconditionerPolicy(
          bool advanceReuseCounter) override;

      bool isPreconditionerConstructed() const override;

      bool hasPreconditioner() const override;

      Teuchos::RCP<const Epetra_Operator> getGeneratedPrecOperator() const override;

      Teuchos::RCP<Epetra_Operator> getGeneratedPrecOperator() override;

      void setPrecOperatorForSolve(const Teuchos::RCP<const Epetra_Operator>& solvePrecOp) override;

      //! destroy the jacobian ptr
      bool DestroyJacobian();

      //! compute the eigenvalues of the jacobian operator in serial mode
      /**
       *  \pre Not supported in parallel. The Jacobian matrix should be not too
       *  large since the sparse matrix is transformed to a full matrix.
       *
       *  \note The computation can become quite expensive even for rather
       *  small matrices. The underlying LAPACK routine computes all
       *  eigenvalues of your system matrix. Therefore, if you are only interested
       *  in an estimate for condition number think about the GMRES variant.
       *  Nevertheless, the here computed eigenvalues are the exact ones.
       *
       *  \return the computed condition number.
       *  \author hiermeier \date 04/18 */
      void computeSerialEigenvaluesOfJacobian(CORE::LINALG::SerialDenseVector& reigenvalues,
          CORE::LINALG::SerialDenseVector& ieigenvalues) const;

      /// compute the respective condition number (only possible in serial mode)
      double computeSerialConditionNumberOfJacobian(
          const LinSystem::ConditionNumber condnum_type) const;

     protected:
      /// access the jacobian
      inline CORE::LINALG::SparseOperator& Jacobian() const
      {
        if (jacPtr_.is_null()) throwError("JacPtr", "JacPtr is nullptr!");

        return *jacPtr_;
      }

      /// access the jacobian (read-only)
      inline const Teuchos::RCP<CORE::LINALG::SparseOperator>& JacobianPtr() const
      {
        if (jacPtr_.is_null()) throwError("JacPtr", "JacPtr is nullptr!");

        return jacPtr_;
      }

      //! PURE VIRTUAL FUNCTIONS: These functions have to be defined in the derived
      //! problem specific subclasses.

      //! sets the options of the underlying solver
      virtual CORE::LINALG::SolverParams SetSolverOptions(Teuchos::ParameterList& p,
          Teuchos::RCP<CORE::LINALG::Solver>& solverPtr,
          const NOX::NLN::SolutionType& solverType) = 0;

      //! Returns a pointer to linear solver, which has to be used
      virtual NOX::NLN::SolutionType GetActiveLinSolver(
          const std::map<NOX::NLN::SolutionType, Teuchos::RCP<CORE::LINALG::Solver>>& solvers,
          Teuchos::RCP<CORE::LINALG::Solver>& currSolver) = 0;

      //! Set-up the linear problem object
      virtual void SetLinearProblemForSolve(Epetra_LinearProblem& linear_problem,
          CORE::LINALG::SparseOperator& jac, Epetra_Vector& lhs, Epetra_Vector& rhs) const;

      /*! \brief Complete the solution vector after a linear solver attempt
       *
       *  This method is especially meaningful, when a sub-part of the linear
       *  problem has been solved explicitly.
       *
       *  \param linProblem (in) : Solved linear problem
       *  \param lhs        (out): left-hand-side vector which can be extended
       *
       *  \author hiermeier \date 04/17 */
      virtual void CompleteSolutionAfterSolve(
          const Epetra_LinearProblem& linProblem, Epetra_Vector& lhs) const;

      /// convert jacobian matrix to dense matrix
      void convertJacobianToDenseMatrix(CORE::LINALG::SerialDenseMatrix& dense) const;

      /// convert sparse matrix to dense matrix
      void convertSparseToDenseMatrix(const CORE::LINALG::SparseMatrix& sparse,
          CORE::LINALG::SerialDenseMatrix& dense, const Epetra_Map& full_rangemap,
          const Epetra_Map& full_domainmap) const;

      /// prepare the dense matrix in case of a block sparse matrix
      void prepareBlockDenseMatrix(const CORE::LINALG::BlockSparseMatrixBase& block_sparse,
          CORE::LINALG::SerialDenseMatrix& block_dense) const;

      /// throw an error if there is a row containing only zeros
      void throwIfZeroRow(const CORE::LINALG::SerialDenseMatrix& block_dense) const;

      /// solve the non-symmetric eigenvalue problem
      void solveNonSymmEigenValueProblem(CORE::LINALG::SerialDenseMatrix& mat,
          CORE::LINALG::SerialDenseVector& reigenvalues,
          CORE::LINALG::SerialDenseVector& ieigenvalues) const;

      /// call GEEV from LAPACK
      void callGEEV(CORE::LINALG::SerialDenseMatrix& mat,
          CORE::LINALG::SerialDenseVector& reigenvalues,
          CORE::LINALG::SerialDenseVector& ieigenvalues) const;

      /// call GGEV from LAPACK
      void callGGEV(CORE::LINALG::SerialDenseMatrix& mat,
          CORE::LINALG::SerialDenseVector& reigenvalues,
          CORE::LINALG::SerialDenseVector& ieigenvalues) const;

     private:
      //! throws an error
      void throwError(const std::string& functionName, const std::string& errorMsg) const;

     protected:
      //! Printing Utilities object
      ::NOX::Utils utils_;

      //! Solver pointers
      const std::map<NOX::NLN::SolutionType, Teuchos::RCP<CORE::LINALG::Solver>>& solvers_;

      //! Reference to the user supplied required interface functions
      Teuchos::RCP<::NOX::Epetra::Interface::Required> reqInterfacePtr_;

      //! Reference to the user supplied Jacobian interface functions
      Teuchos::RCP<::NOX::Epetra::Interface::Jacobian> jacInterfacePtr_;

      //! Type of operator for the Jacobian.
      NOX::NLN::LinSystem::OperatorType jacType_;

      //! Reference to the user supplied preconditioner interface functions
      Teuchos::RCP<::NOX::Epetra::Interface::Preconditioner> precInterfacePtr_;

      //! Type of operator for the preconditioner.
      NOX::NLN::LinSystem::OperatorType precType_;

      //! Pointer to the preconditioner operator.
      Teuchos::RCP<Epetra_Operator> precPtr_;

      PreconditionerMatrixSourceType precMatrixSource_;

      //! Scaling object supplied by the user
      Teuchos::RCP<::NOX::Epetra::Scaling> scaling_;

      double conditionNumberEstimate_;

      //! Teuchos::Time object
      Teuchos::Time timer_;

      //! Total time spent in createPreconditioner (sec.).
      double timeCreatePreconditioner_;

      //! Total time spent in applyJacobianInverse (sec.).
      double timeApplyJacbianInverse_;

      //! residual 2-norm
      double resNorm2_;

      //! If set to true, solver information is printed to the "Output" sublist of the "Linear
      //! Solver" list.
      bool outputSolveDetails_;

      //! Zero out the initial guess for linear solves performed through applyJacobianInverse calls
      //! (i.e. zero out the result vector before the linear solve).
      bool zeroInitialGuess_;

      //! Stores the parameter "Compute Scaling Manually".
      bool manualScaling_;

      //! Pointer to an user defined wrapped NOX::NLN::Abstract::PrePostOperator object.
      Teuchos::RCP<NOX::NLN::LinSystem::PrePostOperator> prePostOperatorPtr_;

     private:
      /*! \brief Pointer to the Jacobian operator.
       *
       *  Use the provided accessors to access this member. Direct access is prohibited
       *  due to the pointer management by changing states (e.g. XFEM). */
      Teuchos::RCP<CORE::LINALG::SparseOperator> jacPtr_;
    };
  }  // namespace NLN
}  // namespace NOX

FOUR_C_NAMESPACE_CLOSE

#endif
