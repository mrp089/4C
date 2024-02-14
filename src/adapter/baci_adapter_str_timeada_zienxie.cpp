/*----------------------------------------------------------------------*/
/*! \file

\brief Wrapper for the structural time integration which gives fine grained
       access in the adaptive time marching loop


\level 1

*/
/*----------------------------------------------------------------------*/

#include "baci_adapter_str_timeada_zienxie.hpp"

#include "baci_structure_new_timint_base.hpp"
#include "baci_structure_new_timint_basedataglobalstate.hpp"

BACI_NAMESPACE_OPEN


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::StructureTimeAdaZienXie::IntegrateStepAuxiliar()
{
  const STR::TIMINT::Base& stm = *stm_;
  const STR::TIMINT::BaseDataGlobalState& gstate = stm.DataGlobalState();

  // get state vectors of marching integrator
  Teuchos::RCP<const Epetra_Vector> dis = gstate.GetDisN();    // D_{n}^{A2}
  Teuchos::RCP<const Epetra_Vector> vel = gstate.GetVelN();    // V_{n}^{A2}
  Teuchos::RCP<const Epetra_Vector> acc = gstate.GetAccN();    // A_{n}^{A2}
  Teuchos::RCP<const Epetra_Vector> accn = gstate.GetAccNp();  // A_{n+1}^{A2}

  // build ZX displacements D_{n+1}^{ZX}
  // using the second order (or lower) accurate new accelerations
  locerrdisn_->Update(1.0, *dis, stepsize_, *vel, 0.0);
  locerrdisn_->Update(stepsize_ * stepsize_ / 3.0, *acc, stepsize_ * stepsize_ / 6.0, *accn, 1.0);
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::StructureTimeAdaZienXie::UpdateAuxiliar()
{
  // NOTHING TO UPDATE
}
BACI_NAMESPACE_CLOSE