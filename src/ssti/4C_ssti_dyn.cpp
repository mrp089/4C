/*----------------------------------------------------------------------*/
/*! \file
 \brief control routine for scalar structure thermo interaction

 \level 2

 *------------------------------------------------------------------------------------------------*/

#include "4C_ssti_dyn.hpp"

#include "4C_fem_discretization.hpp"
#include "4C_global_data.hpp"
#include "4C_inpar_ssti.hpp"
#include "4C_io_control.hpp"
#include "4C_ssti_algorithm.hpp"
#include "4C_ssti_monolithic.hpp"
#include "4C_ssti_utils.hpp"

#include <Teuchos_StandardParameterEntryValidators.hpp>
#include <Teuchos_TimeMonitor.hpp>

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ssti_drt()
{
  Global::Problem* problem = Global::Problem::instance();

  const Epetra_Comm& comm = problem->get_dis("structure")->get_comm();

  auto ssti = SSTI::build_ssti(Teuchos::getIntegralValue<Inpar::SSTI::SolutionScheme>(
                                   problem->ssti_control_params(), "COUPALGO"),
      comm, problem->ssti_control_params());

  ssti->init(comm, problem->ssti_control_params(), problem->scalar_transport_dynamic_params(),
      problem->ssti_control_params().sublist("THERMO"), problem->structural_dynamic_params());

  ssti->setup();

  const int restart = problem->restart();
  if (restart) ssti->read_restart(restart);

  ssti->setup_system();

  ssti->timeloop();

  Teuchos::TimeMonitor::summarize();

  ssti->test_results(comm);
}

FOUR_C_NAMESPACE_CLOSE
