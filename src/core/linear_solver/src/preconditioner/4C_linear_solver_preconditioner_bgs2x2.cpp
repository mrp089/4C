/*----------------------------------------------------------------------*/
/*! \file

\brief Declaration

\level 1

*/
/*----------------------------------------------------------------------*/

#include "4C_linear_solver_preconditioner_bgs2x2.hpp"

#include "4C_linalg_utils_sparse_algebra_math.hpp"
#include "4C_linear_solver_method_linalg.hpp"

#include <ml_MultiLevelPreconditioner.h>

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Core::LinAlg::BgS2x2Operator::BgS2x2Operator(Teuchos::RCP<Epetra_Operator> A,
    const Teuchos::ParameterList& list1, const Teuchos::ParameterList& list2, int global_iter,
    double global_omega, int block1_iter, double block1_omega, int block2_iter, double block2_omega,
    bool fliporder)
    : list1_(list1),
      list2_(list2),
      global_iter_(global_iter),
      global_omega_(global_omega),
      block1_iter_(block1_iter),
      block1_omega_(block1_omega),
      block2_iter_(block2_iter),
      block2_omega_(block2_omega)
{
  if (!fliporder)
  {
    firstind_ = 0;
    secind_ = 1;
  }
  else
  {
    firstind_ = 1;
    secind_ = 0;

    // switch parameter lists according to fliporder
    list2_ = list1;
    list1_ = list2;
  }

  a_ = Teuchos::rcp_dynamic_cast<BlockSparseMatrixBase>(A);
  if (a_ != Teuchos::null)
  {
    // Make a shallow copy of the block matrix as the preconditioners on the
    // blocks will be reused and the next assembly will replace the block
    // matrices.
    a_ = a_->Clone(View);
    mmex_ = a_->RangeExtractor();
  }
  else
  {
    FOUR_C_THROW("BGS2x2: provided operator is not a BlockSparseMatrix!");
  }

  setup_block_preconditioners();

  return;
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void Core::LinAlg::BgS2x2Operator::setup_block_preconditioners()
{
  Teuchos::RCP<Core::LinAlg::Solver> s1 = Teuchos::rcp(new Core::LinAlg::Solver(
      list1_, a_->Comm(), nullptr, Core::IO::Verbositylevel::standard, false));
  solver1_ = Teuchos::rcp(new Core::LinAlg::Preconditioner(s1));
  const Core::LinAlg::SparseMatrix& Op11 = a_->Matrix(firstind_, firstind_);
  solver1_->setup(Op11.EpetraMatrix());

  Teuchos::RCP<Core::LinAlg::Solver> s2 = Teuchos::rcp(new Core::LinAlg::Solver(
      list2_, a_->Comm(), nullptr, Core::IO::Verbositylevel::standard, false));
  solver2_ = Teuchos::rcp(new Core::LinAlg::Preconditioner(s2));
  const Core::LinAlg::SparseMatrix& Op22 = a_->Matrix(secind_, secind_);
  solver2_->setup(Op22.EpetraMatrix());

  return;
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
int Core::LinAlg::BgS2x2Operator::ApplyInverse(
    const Epetra_MultiVector& X, Epetra_MultiVector& Y) const
{
  Teuchos::RCP<Epetra_MultiVector> y1 = mmex_.extract_vector(Y, firstind_);
  Teuchos::RCP<Epetra_MultiVector> y2 = mmex_.extract_vector(Y, secind_);

  Teuchos::RCP<Epetra_MultiVector> z1 =
      Teuchos::rcp(new Epetra_MultiVector(y1->Map(), y1->NumVectors()));
  Teuchos::RCP<Epetra_MultiVector> z2 =
      Teuchos::rcp(new Epetra_MultiVector(y2->Map(), y2->NumVectors()));

  Teuchos::RCP<Epetra_MultiVector> tmpx1 =
      Teuchos::rcp(new Epetra_MultiVector(a_->DomainMap(firstind_), y1->NumVectors()));
  Teuchos::RCP<Epetra_MultiVector> tmpx2 =
      Teuchos::rcp(new Epetra_MultiVector(a_->DomainMap(secind_), y2->NumVectors()));

  const Core::LinAlg::SparseMatrix& Op11 = a_->Matrix(firstind_, firstind_);
  const Core::LinAlg::SparseMatrix& Op22 = a_->Matrix(secind_, secind_);
  const Core::LinAlg::SparseMatrix& Op12 = a_->Matrix(firstind_, secind_);
  const Core::LinAlg::SparseMatrix& Op21 = a_->Matrix(secind_, firstind_);

  // outer Richardson loop
  for (int run = 0; run < global_iter_; ++run)
  {
    Teuchos::RCP<Epetra_MultiVector> x1 = a_->DomainExtractor().extract_vector(X, firstind_);
    Teuchos::RCP<Epetra_MultiVector> x2 = a_->DomainExtractor().extract_vector(X, secind_);

    // ----------------------------------------------------------------
    // first block

    if (run > 0)
    {
      Op11.Multiply(false, *y1, *tmpx1);
      x1->Update(-1.0, *tmpx1, 1.0);
      Op12.Multiply(false, *y2, *tmpx1);
      x1->Update(-1.0, *tmpx1, 1.0);
    }

    solver1_->Solve(Op11.EpetraMatrix(), z1, x1, true);

    local_block_richardson(solver1_, Op11, x1, z1, tmpx1, block1_iter_, block1_omega_);

    if (run > 0)
    {
      y1->Update(global_omega_, *z1, 1.0);
    }
    else
    {
      y1->Update(global_omega_, *z1, 0.0);
    }

    // ----------------------------------------------------------------
    // second block

    if (run > 0)
    {
      Op22.Multiply(false, *y2, *tmpx2);
      x2->Update(-1.0, *tmpx2, 1.0);
    }

    Op21.Multiply(false, *y1, *tmpx2);
    x2->Update(-1.0, *tmpx2, 1.0);

    solver2_->Solve(Op22.EpetraMatrix(), z2, x2, true);

    local_block_richardson(solver2_, Op22, x2, z2, tmpx2, block2_iter_, block2_omega_);

    if (run > 0)
    {
      y2->Update(global_omega_, *z2, 1.0);
    }
    else
    {
      y2->Update(global_omega_, *z2, 0.0);
    }
  }

  mmex_.insert_vector(*y1, firstind_, Y);
  mmex_.insert_vector(*y2, secind_, Y);

  return 0;
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void Core::LinAlg::BgS2x2Operator::local_block_richardson(
    Teuchos::RCP<Core::LinAlg::Preconditioner> solver, const Core::LinAlg::SparseMatrix& Op,
    Teuchos::RCP<Epetra_MultiVector> x, Teuchos::RCP<Epetra_MultiVector> y,
    Teuchos::RCP<Epetra_MultiVector> tmpx, int iter, double omega) const
{
  if (iter > 0)
  {
    y->Scale(omega);
    Teuchos::RCP<Epetra_MultiVector> tmpy =
        Teuchos::rcp(new Epetra_MultiVector(y->Map(), y->NumVectors()));

    for (int i = 0; i < iter; ++i)
    {
      Op.EpetraMatrix()->Multiply(false, *y, *tmpx);
      tmpx->Update(1.0, *x, -1.0);

      solver->Solve(Op.EpetraMatrix(), tmpy, tmpx, false);
      y->Update(omega, *tmpy, 1.0);
    }
  }
}

FOUR_C_NAMESPACE_CLOSE
