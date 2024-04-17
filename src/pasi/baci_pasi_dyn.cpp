/*---------------------------------------------------------------------------*/
/*! \file
\brief control routine for particle structure interaction problems
\level 3
*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 | headers                                                                   |
 *---------------------------------------------------------------------------*/
#include "baci_pasi_dyn.hpp"

#include "baci_comm_utils.hpp"
#include "baci_global_data.hpp"
#include "baci_inpar_pasi.hpp"
#include "baci_lib_discret.hpp"
#include "baci_pasi_partitioned_onewaycoup.hpp"
#include "baci_pasi_partitioned_twowaycoup.hpp"
#include "baci_pasi_utils.hpp"

FOUR_C_NAMESPACE_OPEN

/*---------------------------------------------------------------------------*
 | definitions                                                               |
 *---------------------------------------------------------------------------*/
void pasi_dyn()
{
  // get pointer to global problem
  GLOBAL::Problem* problem = GLOBAL::Problem::Instance();

  // create a communicator
  const Epetra_Comm& comm = problem->GetDis("structure")->Comm();

  // print pasi logo to screen
  if (comm.MyPID() == 0) PASI::UTILS::Logo();

  // get parameter list
  const Teuchos::ParameterList& params = problem->PASIDynamicParams();

  // modification of time parameters of subproblems
  PASI::UTILS::ChangeTimeParameter(comm, params,
      const_cast<Teuchos::ParameterList&>(problem->ParticleParams()),
      const_cast<Teuchos::ParameterList&>(problem->StructuralDynamicParams()));

  // create particle structure interaction algorithm
  Teuchos::RCP<PASI::PartitionedAlgo> algo = Teuchos::null;

  // get type of partitioned coupling
  int coupling = CORE::UTILS::IntegralValue<int>(params, "COUPLING");

  // query algorithm
  switch (coupling)
  {
    case INPAR::PASI::partitioned_onewaycoup:
    {
      algo = Teuchos::rcp(new PASI::PASI_PartOneWayCoup(comm, params));
      break;
    }
    case INPAR::PASI::partitioned_twowaycoup:
    {
      algo = Teuchos::rcp(new PASI::PASI_PartTwoWayCoup(comm, params));
      break;
    }
    case INPAR::PASI::partitioned_twowaycoup_disprelax:
    {
      algo = Teuchos::rcp(new PASI::PASI_PartTwoWayCoup_DispRelax(comm, params));
      break;
    }
    case INPAR::PASI::partitioned_twowaycoup_disprelaxaitken:
    {
      algo = Teuchos::rcp(new PASI::PASI_PartTwoWayCoup_DispRelaxAitken(comm, params));
      break;
    }
    default:
    {
      dserror("no valid coupling type for particle structure interaction specified!");
      break;
    }
  }

  // init pasi algorithm
  algo->Init();

  // read restart information
  const int restart = problem->Restart();
  if (restart) algo->ReadRestart(restart);

  // setup pasi algorithm
  algo->Setup();

  // solve partitioned particle structure interaction
  algo->Timeloop();

  // perform result tests
  algo->TestResults(comm);

  // print summary statistics for all timers
  Teuchos::RCP<const Teuchos::Comm<int>> TeuchosComm = CORE::COMM::toTeuchosComm<int>(comm);
  Teuchos::TimeMonitor::summarize(TeuchosComm.ptr(), std::cout, false, true, false);
}

FOUR_C_NAMESPACE_CLOSE
