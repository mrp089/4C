/*----------------------------------------------------------------------------*/
/*!
\file nln_problem_coarselevel.cpp

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

// Epetra
#include <Epetra_Comm.h>
#include <Epetra_MultiVector.h>
#include <Epetra_Vector.h>

// Teuchos
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>

// baci
#include "fas_hierarchy.H"
#include "nln_operator_fas.H"
#include "nln_problem_coarselevel.H"

#include "../drt_io/io_control.H"
#include "../drt_io/io_pstream.H"
#include "../drt_lib/drt_dserror.H"

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
NLNSOL::NlnProblemCoarseLevel::NlnProblemCoarseLevel()
: hierarchy_(Teuchos::null),
  fhat_(Teuchos::null),
  fbar_(Teuchos::null),
  levelid_(0)
{
  return;
}

/*----------------------------------------------------------------------------*/
void NLNSOL::NlnProblemCoarseLevel::Setup()
{
  if (not IsInit()) { dserror("Init() has not been called, yet."); }

  hierarchy_ = Params().get<Teuchos::RCP<const NLNSOL::FAS::AMGHierarchy> >(
      "AMG Hierarchy");
  levelid_ = Params().get<int>("Level ID");

  // call base class
  NLNSOL::NlnProblem::Setup();

  return;
}

/*----------------------------------------------------------------------------*/
void NLNSOL::NlnProblemCoarseLevel::ComputeF(const Epetra_MultiVector& xc,
    Epetra_MultiVector& fc) const
{
  int err = 0;

  // Make sure that Init() and Setup() have been called
  if (not IsInit()) { dserror("Init() has not been called, yet."); }
  if (not IsSetup()) { dserror("Setup() has not been called, yet."); }

  // prolongate current solution to fine level
  Teuchos::RCP<Epetra_MultiVector> xf =
      Hierarchy().ProlongateToFineLevel(xc, levelid_);

  // call evaluate from the outer nonlinear solver
  Teuchos::RCP<Epetra_MultiVector> ffine =
      Teuchos::rcp(new Epetra_MultiVector(xf->Map(), true));
  NLNSOL::NlnProblem::ComputeF(*xf,*ffine);

  // restrict fine level residual to current coarse level
  Teuchos::RCP<Epetra_MultiVector> fcoarse =
      Hierarchy().RestrictToCoarseLevel(*ffine, levelid_);

  // residual correction on coarse level
  if (fhat_.is_null()) { dserror("Residual correction 'fhat_' not set, yet."); }
  if (fbar_.is_null()) { dserror("Residual correction 'fbar_' not set, yet."); }
  err = fcoarse->Update(-1.0, *fhat_, 1.0, *fbar_, 1.0);
  if (err != 0) { dserror("Update failed."); }

  // copy result to provided output variable
  err = fc.Update(1.0, *fcoarse, 0.0);
  if (err != 0) { dserror("Update failed."); }

  return;
}

/*----------------------------------------------------------------------------*/
const NLNSOL::FAS::AMGHierarchy&
NLNSOL::NlnProblemCoarseLevel::Hierarchy() const
{
  if (hierarchy_.is_null())
    dserror("AMG-FAS hierarchy object 'nlnprecfas_' has not been set, yet.");

  return *hierarchy_;
}

/*----------------------------------------------------------------------------*/
void NLNSOL::NlnProblemCoarseLevel::SetFHatFBar(
    Teuchos::RCP<Epetra_MultiVector> fhat,
    Teuchos::RCP<Epetra_MultiVector> fbar)
{
  fhat_ = fhat;
  fbar_ = fbar;

  return;
}
