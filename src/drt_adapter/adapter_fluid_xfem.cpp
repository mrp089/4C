
#ifdef CCADISCRET

#include "../drt_lib/drt_globalproblem.H"
#include "../drt_lib/drt_validparameters.H"

#include "adapter_fluid_xfem.H"


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
ADAPTER::FluidXFEMAdapter::FluidXFEMAdapter(const Teuchos::ParameterList& prbdyn,
                                          std::string condname)
  : fluid_(prbdyn,true)
{
//  icoupfa_.SetupConditionCoupling(*FluidField().Discretization(),
//                                   FluidField().Interface(),
//                                  *AleField().Discretization(),
//                                   AleField().Interface(),
//                                   condname);

  //FSI::Coupling& coupfa = FluidAleFieldCoupling();

  // the fluid-ale coupling always matches
  //const Epetra_Map* fluidnodemap = FluidField().Discretization()->NodeRowMap();
  //const Epetra_Map* alenodemap   = AleField().Discretization()->NodeRowMap();

//  coupfa_.SetupCoupling(*FluidField().Discretization(),
//                        *AleField().Discretization(),
//                        *fluidnodemap,
//                        *alenodemap);

  //FluidField().SetMeshMap(coupfa_.MasterDofMap());

  // the ale matrix is build just once
  //AleField().BuildSystemMatrix();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<DRT::Discretization> ADAPTER::FluidXFEMAdapter::Discretization()
{
  return FluidField().Discretization();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
const LINALG::MapExtractor& ADAPTER::FluidXFEMAdapter::Interface() const
{
  return FluidField().Interface();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::FluidXFEMAdapter::PrepareTimeStep()
{
  FluidField().PrepareTimeStep();
  //AleField().PrepareTimeStep();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::FluidXFEMAdapter::Update()
{
  FluidField().Update();
  //AleField().Update();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::FluidXFEMAdapter::Output()
{
  FluidField().Output();
  //AleField().Output();

  FluidField().LiftDrag();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
double ADAPTER::FluidXFEMAdapter::ReadRestart(int step)
{
  FluidField().ReadRestart(step);
  //AleField().ReadRestart(step);
  return FluidField().Time();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::FluidXFEMAdapter::NonlinearSolve(Teuchos::RCP<Epetra_Vector> idisp,
                                          Teuchos::RCP<Epetra_Vector> ivel)
{
  if (idisp!=Teuchos::null)
  {
    // if we have values at the interface we need to apply them
    //AleField().ApplyInterfaceDisplacements(FluidToAle(idisp));
    FluidField().ApplyInterfaceVelocities(ivel);
  }

  // Note: We do not look for moving ale boundaries (outside the coupling
  // interface) on the fluid side. Thus if you prescribe time variable ale
  // Dirichlet conditions the according fluid Dirichlet conditions will not
  // notice.

  //AleField().Solve();
  //Teuchos::RCP<Epetra_Vector> fluiddisp = AleToFluidField(AleField().ExtractDisplacement());
  //FluidField().ApplyMeshDisplacement(fluiddisp);
  FluidField().NonlinearSolve();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> ADAPTER::FluidXFEMAdapter::RelaxationSolve(Teuchos::RCP<Epetra_Vector> idisp,
                                                                  double dt)
{
  // Here we have a mesh position independent of the
  // given trial vector, but still the grid velocity depends on the
  // trial vector only.

  // grid velocity
  //AleField().ApplyInterfaceDisplacements(FluidToAle(idisp));

  //AleField().Solve();
  //Teuchos::RCP<Epetra_Vector> fluiddisp = AleToFluidField(AleField().ExtractDisplacement());
  //fluiddisp->Scale(1./dt);

  //FluidField().ApplyMeshVelocity(fluiddisp);

  // grid position is done inside RelaxationSolve

  // the displacement -> velocity conversion at the interface
  idisp->Scale(1./dt);

  return FluidField().RelaxationSolve(idisp);
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> ADAPTER::FluidXFEMAdapter::ExtractInterfaceForces()
{
  return FluidField().ExtractInterfaceForces();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> ADAPTER::FluidXFEMAdapter::IntegrateInterfaceShape()
{
  return FluidField().IntegrateInterfaceShape();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<DRT::ResultTest> ADAPTER::FluidXFEMAdapter::CreateFieldTest()
{
  return FluidField().CreateFieldTest();
}


///*----------------------------------------------------------------------*/
///*----------------------------------------------------------------------*/
//Teuchos::RCP<Epetra_Vector> ADAPTER::FluidXFEMAdapter::AleToFluidField(Teuchos::RCP<Epetra_Vector> iv) const
//{
//  return coupfa_.SlaveToMaster(iv);
//}
//
//
///*----------------------------------------------------------------------*/
///*----------------------------------------------------------------------*/
//Teuchos::RCP<Epetra_Vector> ADAPTER::FluidXFEMAdapter::AleToFluidField(Teuchos::RCP<const Epetra_Vector> iv) const
//{
//  return coupfa_.SlaveToMaster(iv);
//}
//
//
///*----------------------------------------------------------------------*/
///*----------------------------------------------------------------------*/
//Teuchos::RCP<Epetra_Vector> ADAPTER::FluidXFEMAdapter::FluidToAle(Teuchos::RCP<Epetra_Vector> iv) const
//{
//  return icoupfa_.MasterToSlave(iv);
//}
//
//
///*----------------------------------------------------------------------*/
///*----------------------------------------------------------------------*/
//Teuchos::RCP<Epetra_Vector> ADAPTER::FluidXFEMAdapter::FluidToAle(Teuchos::RCP<const Epetra_Vector> iv) const
//{
//  return icoupfa_.MasterToSlave(iv);
//}


#endif
