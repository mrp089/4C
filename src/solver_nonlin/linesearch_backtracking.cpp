/*----------------------------------------------------------------------------*/
/*!
\file linesearch_backtracking.cpp

<pre>
Maintainer: Matthias Mayr
            mayr@mhpc.mw.tum.de
            089 - 289-10362
</pre>
*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* headers */

// standard
#include <iostream>

// Epetra
#include <Epetra_Comm.h>
#include <Epetra_MultiVector.h>

// NOX
#include <NOX_Abstract_Group.H>
#include <NOX_Epetra_Vector.H>
#include <NOX_Epetra_MultiVector.H>

// Teuchos
#include <Teuchos_FancyOStream.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_TimeMonitor.hpp>

// baci
#include "linesearch_backtracking.H"

#include "../drt_io/io_pstream.H"
#include "../drt_lib/drt_dserror.H"

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
NLNSOL::LineSearchBacktracking::LineSearchBacktracking()
 : NLNSOL::LineSearchBase()
{
  return;
}

/*----------------------------------------------------------------------------*/
void NLNSOL::LineSearchBacktracking::Setup()
{
  // make sure that Init() has been called
  if (not IsInit()) { dserror("Init() has not been called, yet."); }

  // fill member variables
  itermax_ = Params().sublist("Backtracking").get<int>("max number of backtracking steps");

  // SetupLineSearch() has been called
  SetIsSetup();

  return;
}

/*----------------------------------------------------------------------------*/
const double NLNSOL::LineSearchBacktracking::ComputeLSParam() const
{
  // time measurements
  Teuchos::RCP<Teuchos::Time> time = Teuchos::TimeMonitor::getNewCounter(
      "NLNSOL::LineSearchBacktracking::ComputeLSParam");
  Teuchos::TimeMonitor monitor(*time);

  // create formatted output stream
  Teuchos::RCP<Teuchos::FancyOStream> out =
      Teuchos::getFancyOStream(Teuchos::rcpFromRef(std::cout));
  out->setOutputToRootOnly(0);

  // make sure that Init() and Setup() has been called
  if (not IsInit()) { dserror("Init() has not been called, yet."); }
  if (not IsSetup()) { dserror("Setup() has not been called, yet."); }

  // the line search parameter
  double lsparam = 1.0;

  // take a full step
  Teuchos::RCP<Epetra_MultiVector> xnew =
      Teuchos::rcp(new Epetra_MultiVector(GetXOld().Map(), true));
  xnew->Update(1.0, GetXOld(), lsparam, GetXInc(), 0.0);

  Teuchos::RCP<Epetra_MultiVector> fnew =
      Teuchos::rcp(new Epetra_MultiVector(xnew->Map(), true));
  ComputeF(*xnew, *fnew);

  double fnorm2 = 1.0e+12;
  bool converged = ConvergenceCheck(*fnew, fnorm2);

  int iter = 0; // iteration index for multiple backtracking steps

  while (not converged and not IsSufficientDecrease(fnorm2, lsparam)
      and iter < itermax_)
  {
    ++iter;

    // reduce trial line search parameter
    lsparam /= 2.0;

    *out << "lsparam = " << lsparam;// << std::endl;

    // take a reduced step
    xnew->Update(1.0, GetXOld(), lsparam, GetXInc(), 0.0);
    ComputeF(*xnew, *fnew);

    // evaluate and compute L2-norm of residual
    converged = ConvergenceCheck(*fnew, fnorm2);

    *out << "\tfnorm2 = " << fnorm2
         << "\tinitnorm = " << GetFNormOld()
         << std::endl;
  }

  // check if the sufficient decrease condition could be satisfied at all
  if (not converged
      and (not IsSufficientDecrease(fnorm2, lsparam) or iter > itermax_))
  {
    dserror("Sufficient decrease condition could not be satisfied withing %d "
        "iterations.", itermax_);

    lsparam = 0.0;
  }

  return lsparam;
}



