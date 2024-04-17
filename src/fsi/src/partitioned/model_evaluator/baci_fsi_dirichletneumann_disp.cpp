/*----------------------------------------------------------------------*/
/*! \file

\brief Solve FSI problems using a Dirichlet-Neumann partitioned approach
based on the interface displacements


\level 1
*/
/*----------------------------------------------------------------------*/

#include "baci_fsi_dirichletneumann_disp.hpp"

#include "baci_adapter_str_fsiwrapper.hpp"
#include "baci_fsi_debugwriter.hpp"
#include "baci_global_data.hpp"
#include "baci_inpar_fsi.hpp"

#include <Teuchos_StandardParameterEntryValidators.hpp>

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
FSI::DirichletNeumannDisp::DirichletNeumannDisp(const Epetra_Comm& comm) : DirichletNeumann(comm)
{
  // empty constructor
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::DirichletNeumannDisp::Setup()
{
  // call setup of base class
  FSI::DirichletNeumann::Setup();
  const Teuchos::ParameterList& fsidyn = GLOBAL::Problem::Instance()->FSIDynamicParams();
  const Teuchos::ParameterList& fsipart = fsidyn.sublist("PARTITIONED SOLVER");
  SetKinematicCoupling(
      CORE::UTILS::IntegralValue<int>(fsipart, "COUPVARIABLE") == INPAR::FSI::CoupVarPart::disp);
}
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> FSI::DirichletNeumannDisp::FluidOp(
    Teuchos::RCP<Epetra_Vector> idisp, const FillType fillFlag)
{
  FSI::Partitioned::FluidOp(idisp, fillFlag);

  if (fillFlag == User)
  {
    // SD relaxation calculation
    return FluidToStruct(MBFluidField()->RelaxationSolve(StructToFluid(idisp), Dt()));
  }
  else
  {
    // normal fluid solve

    // the displacement -> velocity conversion at the interface
    const Teuchos::RCP<Epetra_Vector> ivel = InterfaceVelocity(idisp);

    // A rather simple hack. We need something better!
    const int itemax = MBFluidField()->Itemax();
    if (fillFlag == MF_Res and mfresitemax_ > 0) MBFluidField()->SetItemax(mfresitemax_ + 1);

    MBFluidField()->NonlinearSolve(StructToFluid(idisp), StructToFluid(ivel));

    MBFluidField()->SetItemax(itemax);

    return FluidToStruct(MBFluidField()->ExtractInterfaceForces());
  }
}
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> FSI::DirichletNeumannDisp::StructOp(
    Teuchos::RCP<Epetra_Vector> iforce, const FillType fillFlag)
{
  FSI::Partitioned::StructOp(iforce, fillFlag);

  if (fillFlag == User)
  {
    // SD relaxation calculation
    return StructureField()->RelaxationSolve(iforce);
  }
  else
  {
    // normal structure solve
    if (not use_old_structure_)
      StructureField()->ApplyInterfaceForces(iforce);
    else
      StructureField()->ApplyInterfaceForcesTemporaryDeprecated(
          iforce);  // todo remove this line as soon as possible!
    StructureField()->Solve();
    StructureField()->WriteGmshStrucOutputStep();
    return StructureField()->ExtractInterfaceDispnp();
  }
}
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> FSI::DirichletNeumannDisp::InitialGuess()
{
  if (GetKinematicCoupling())
  {
    // predict displacement
    return StructureField()->PredictInterfaceDispnp();
  }
  else
  {
    const Teuchos::ParameterList& fsidyn = GLOBAL::Problem::Instance()->FSIDynamicParams();
    const Teuchos::ParameterList& fsipart = fsidyn.sublist("PARTITIONED SOLVER");
    if (CORE::UTILS::IntegralValue<int>(fsipart, "PREDICTOR") != 1)
    {
      dserror(
          "unknown interface force predictor '%s'", fsipart.get<std::string>("PREDICTOR").c_str());
    }
    return InterfaceForce();
  }
}

FOUR_C_NAMESPACE_CLOSE
