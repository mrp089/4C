
#ifdef CCADISCRET

#include "fsi_algorithm.H"

#include "../drt_lib/drt_globalproblem.H"
#include "../drt_lib/drt_validparameters.H"

#include <Teuchos_StandardParameterEntryValidators.hpp>


/*----------------------------------------------------------------------*/
// Note: The order of calling the three BaseAlgorithm-constructors is
// important here! In here control file entries are written. And these entries
// define the order in which the filters handle the Discretizations, which in
// turn defines the dof number ordering of the Discretizations.
/*----------------------------------------------------------------------*/
FSI::Algorithm::Algorithm(Epetra_Comm& comm)
  : StructureBaseAlgorithm(DRT::Problem::Instance()->FSIDynamicParams()),
    FluidMovingBoundaryBaseAlgorithm(DRT::Problem::Instance()->FSIDynamicParams(),"FSICoupling"),
    comm_(comm)
{
  const Teuchos::ParameterList& fsidyn   = DRT::Problem::Instance()->FSIDynamicParams();

  if (comm_.MyPID()==0)
    DRT::INPUT::PrintDefaultParameters(std::cout, fsidyn);

  step_ = 0;
  time_ = 0.;
  dt_ = fsidyn.get<double>("TIMESTEP");
  nstep_ = fsidyn.get<int>("NUMSTEP");
  maxtime_ = fsidyn.get<double>("MAXTIME");
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
FSI::Algorithm::~Algorithm()
{
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::Algorithm::ReadRestart(int step)
{
  StructureField().ReadRestart(step);
  time_ = FluidField().ReadRestart(step);
  step_ = step;
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::Algorithm::PrepareTimeStep()
{
  step_ += 1;
  time_ += dt_;

  if (Comm().MyPID()==0)
    std::cout << "\n"
              << method_ << "\n"
              << "TIME:  "    << std::scientific << time_ << "/" << std::scientific << maxtime_
              << "     DT = " << std::scientific << dt_
              << "     STEP = " YELLOW_LIGHT << setw(4) << step_ << END_COLOR "/" << setw(4) << nstep_
              << "\n"
              << NOX::Utils::fill(82)
              << "\n\n";

  StructureField().PrepareTimeStep();
  FluidField().    PrepareTimeStep();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::Algorithm::Update()
{
  StructureField().Update();
  FluidField().    Update();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::Algorithm::Output()
{
  // Note: The order is important here! In here control file entries are
  // written. And these entries define the order in which the filters handle
  // the Discretizations, which in turn defines the dof number ordering of the
  // Discretizations.
  StructureField().Output();
  FluidField().    Output();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
// Teuchos::RCP<Epetra_Vector> FSI::Algorithm::StructToAle(Teuchos::RCP<Epetra_Vector> iv) const
// {
//   return coupsa_.MasterToSlave(iv);
// }


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
// Teuchos::RCP<Epetra_Vector> FSI::Algorithm::AleToStruct(Teuchos::RCP<Epetra_Vector> iv) const
// {
//   return coupsa_.SlaveToMaster(iv);
// }


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> FSI::Algorithm::StructToFluid(Teuchos::RCP<Epetra_Vector> iv) const
{
  return coupsf_.MasterToSlave(iv);
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> FSI::Algorithm::FluidToStruct(Teuchos::RCP<Epetra_Vector> iv) const
{
  return coupsf_.SlaveToMaster(iv);
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
// Teuchos::RCP<Epetra_Vector> FSI::Algorithm::StructToAle(Teuchos::RCP<const Epetra_Vector> iv) const
// {
//   return coupsa_.MasterToSlave(iv);
// }


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
// Teuchos::RCP<Epetra_Vector> FSI::Algorithm::AleToStruct(Teuchos::RCP<const Epetra_Vector> iv) const
// {
//   return coupsa_.SlaveToMaster(iv);
// }


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> FSI::Algorithm::StructToFluid(Teuchos::RCP<const Epetra_Vector> iv) const
{
  return coupsf_.MasterToSlave(iv);
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> FSI::Algorithm::FluidToStruct(Teuchos::RCP<const Epetra_Vector> iv) const
{
  return coupsf_.SlaveToMaster(iv);
}


#endif
