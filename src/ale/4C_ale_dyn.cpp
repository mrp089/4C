/*----------------------------------------------------------------------------*/
/*! \file

\brief Entry routine for pure ALE problems

\level 1

*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
#include "4C_ale_dyn.hpp"

#include "4C_adapter_ale.hpp"
#include "4C_ale_resulttest.hpp"
#include "4C_global_data.hpp"
#include "4C_lib_discret.hpp"

#include <Teuchos_RCP.hpp>

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void dyn_ale_drt()
{
  // -------------------------------------------------------------------
  // access the discretization
  // -------------------------------------------------------------------
  Teuchos::RCP<Discret::Discretization> actdis = Global::Problem::Instance()->GetDis("ale");

  // -------------------------------------------------------------------
  // ask ALE::AleBaseAlgorithm for the ale time integrator
  // -------------------------------------------------------------------
  Teuchos::RCP<Adapter::AleBaseAlgorithm> ale = Teuchos::rcp(
      new Adapter::AleBaseAlgorithm(Global::Problem::Instance()->AleDynamicParams(), actdis));
  Teuchos::RCP<Adapter::Ale> aletimint = ale->ale_field();

  // -------------------------------------------------------------------
  // read the restart information, set vectors and variables if necessary
  // -------------------------------------------------------------------
  const int restart = Global::Problem::Instance()->Restart();
  if (restart) aletimint->read_restart(restart);

  // -------------------------------------------------------------------
  // call time loop
  // -------------------------------------------------------------------
  aletimint->create_system_matrix();
  aletimint->Integrate();

  // -------------------------------------------------------------------
  // do the result test
  // -------------------------------------------------------------------
  // test results
  Global::Problem::Instance()->AddFieldTest(aletimint->CreateFieldTest());
  Global::Problem::Instance()->TestAll(actdis->Comm());

  return;
}

FOUR_C_NAMESPACE_CLOSE
