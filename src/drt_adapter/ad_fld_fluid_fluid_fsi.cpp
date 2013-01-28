/*----------------------------------------------------------------------*/
#include "ad_fld_fluid_fluid_fsi.H"

#include "../drt_adapter/ad_fld_fluid.H"
#include "../drt_fluid/xfluidfluid.H"
#include "../drt_fluid/fluid_utils_mapextractor.H"
#include "../linalg/linalg_mapextractor.H"
#include "../linalg/linalg_utils.H"

#include <Teuchos_RCP.hpp>
#include <Epetra_Vector.h>
#include <Epetra_Map.h>
#include <vector>
#include <set>
/*======================================================================*/
/* constructor */
ADAPTER::FluidFluidFSI::FluidFluidFSI(Teuchos::RCP<Fluid> fluid,
    Teuchos::RCP<DRT::Discretization> embfluiddis,
    Teuchos::RCP<DRT::Discretization> bgfluiddis,
    Teuchos::RCP<LINALG::Solver> solver,
    Teuchos::RCP<Teuchos::ParameterList> params,
    bool isale,
    bool dirichletcond,
    bool monolithicfluidfluidfsi)
: FluidWrapper(fluid),
  embfluiddis_(embfluiddis),
  bgfluiddis_(bgfluiddis),
  solver_(solver),
  params_(params),
  monolithicfluidfluidfsi_(monolithicfluidfluidfsi)
{
  // make sure
  if (fluid_ == Teuchos::null)
    dserror("Failed to create the underlying fluid adapter");

  // cast fluid to fluidimplicit
  xfluidfluid_ = Teuchos::rcp_dynamic_cast<FLD::XFluidFluid>(fluid_);
  if (xfluidfluid_ == Teuchos::null)
    dserror("Failed to cast ADAPTER::Fluid to FLD::XFluidFluid.");

  interface_ = Teuchos::rcp(new FLD::UTILS::MapExtractor());
  meshmap_   = Teuchos::rcp(new LINALG::MapExtractor());


  monolithic_approach_= DRT::INPUT::IntegralValue<INPAR::XFEM::Monolithic_xffsi_Approach>
                        (params->sublist("XFLUID DYNAMIC/GENERAL"),"MONOLITHIC_XFFSI_APPROACH");

  interface_->Setup(*embfluiddis);
  xfluidfluid_->SetSurfaceSplitter(&(*interface_));

  // build inner velocity map
  // dofs at the interface are excluded
  // we use only velocity dofs and only those without Dirichlet constraint

  // here we get the dirichletmaps for the both discretizations
  const Teuchos::RCP<const LINALG::MapExtractor> embdbcmaps = xfluidfluid_->EmbeddedDirichMaps();
  const Teuchos::RCP<const LINALG::MapExtractor> bgdbcmaps = xfluidfluid_->BackgroundDirichMaps();

  // first build the inner map of embedded fluid (other map)
  // intersected with the dofs with no dbc
  std::vector<Teuchos::RCP<const Epetra_Map> > maps;
  maps.push_back(interface_->OtherMap());
  maps.push_back(embdbcmaps->OtherMap());
  Teuchos::RCP<Epetra_Map> innervelmap_emb = LINALG::MultiMapExtractor::IntersectMaps(maps);

  // now the not-dbc map of background fluid and merge it with the
  // inner map of embedded fluid
  std::vector<Teuchos::RCP<const Epetra_Map> > bgembmaps;
  bgembmaps.push_back(bgdbcmaps->OtherMap());
  bgembmaps.push_back(innervelmap_emb);
  Teuchos::RCP<Epetra_Map> innermap_bgemb = LINALG::MultiMapExtractor::MergeMaps(bgembmaps);

  //now throw out the pressure dofs
  std::vector<Teuchos::RCP<const Epetra_Map> > finalmaps;
  finalmaps.push_back(innermap_bgemb);
  finalmaps.push_back(VelocityRowMap());
  innervelmap_ = LINALG::MultiMapExtractor::IntersectMaps(finalmaps);

  if (dirichletcond)
  {
    // mark all interface velocities as dirichlet values
    xfluidfluid_->AddDirichCond(interface_->FSICondMap());
  }

  interfaceforcen_ = Teuchos::rcp(new Epetra_Vector(*(interface_->FSICondMap())));
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
double ADAPTER::FluidFluidFSI::TimeScaling() const
{
  if (params_->get<bool>("interface second order"))
    return 2./xfluidfluid_->Dt();
  else
    return 1./xfluidfluid_->Dt();
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::FluidFluidFSI::Update()
{
  Teuchos::RCP<Epetra_Vector> interfaceforcem = interface_->ExtractFSICondVector(xfluidfluid_->TrueResidual());

  interfaceforcen_ = xfluidfluid_->ExtrapolateEndPoint(interfaceforcen_,interfaceforcem);

  xfluidfluid_->TimeUpdate();
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<const Epetra_Map> ADAPTER::FluidFluidFSI::InnerVelocityRowMap()
{
  return innervelmap_;
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> ADAPTER::FluidFluidFSI::ExtractInterfaceForces()
{
//  return interface_->ExtractFSICondVector(xfluidfluid_->TrueResidual());
  Teuchos::RCP<Epetra_Vector> interfaceforcem = interface_->ExtractFSICondVector(xfluidfluid_->TrueResidual());

  return xfluidfluid_->ExtrapolateEndPoint(interfaceforcen_,interfaceforcem);
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> ADAPTER::FluidFluidFSI::ExtractInterfaceVelnp()
{
  return interface_->ExtractFSICondVector(xfluidfluid_->Velnp());
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> ADAPTER::FluidFluidFSI::ExtractInterfaceVeln()
{
  return interface_->ExtractFSICondVector(xfluidfluid_->Veln());
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<FLD::UTILS::FluidXFluidMapExtractor>const& ADAPTER::FluidFluidFSI::XFluidFluidMapExtractor()
{
  return xfluidfluid_->XFluidFluidMapExtractor();
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::FluidFluidFSI::ApplyEmbFixedMeshDisplacement(Teuchos::RCP<const Epetra_Vector> disp)
{
  // xfluidfluid_->ApplyEmbFixedMeshDisplacement(disp);
  meshmap_->InsertCondVector(disp,xfluidfluid_->ViewOfDispoldstate());
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::FluidFluidFSI::ApplyMeshDisplacement(Teuchos::RCP<const Epetra_Vector> fluiddisp)
{
  // meshmap contains the whole ale map. It transfers the
  // displacement we get from Ale-dis to the displacement of the
  // embedded-fluid-dis
  meshmap_->InsertCondVector(fluiddisp,xfluidfluid_->ViewOfDispnp());

  // new grid velocity
  xfluidfluid_->UpdateGridv();

  return;
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> ADAPTER::FluidFluidFSI::RelaxationSolve(Teuchos::RCP<Epetra_Vector> ivel)
{
  dserror("ADAPTER::FluidFluidFSI::RelaxationSolve");
//   const Epetra_Map* dofrowmap = Discretization()->DofRowMap();
//   Teuchos::RCP<Epetra_Vector> relax = LINALG::CreateVector(*dofrowmap,true);
//   interface_->InsertFSICondVector(ivel,relax);
//   fluidimpl_->LinearRelaxationSolve(relax);
//   return ExtractInterfaceForces();
  return  Teuchos::null;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::FluidFluidFSI::ApplyInterfaceVelocities(Teuchos::RCP<Epetra_Vector> ivel)
{
  // apply the interface velocities
  interface_->InsertFSICondVector(ivel,xfluidfluid_->ViewofVelnp());

//   const Teuchos::ParameterList& fsidyn   = DRT::Problem::Instance()->FSIDynamicParams();
//   if (DRT::INPUT::IntegralValue<int>(fsidyn,"DIVPROJECTION"))
//   {
//     // project the velocity field into a divergence free subspace
//     // (might enhance the linear solver, but we are still not sure.)
//     ProjVelToDivZero();
//   }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void ADAPTER::FluidFluidFSI::SetMeshMap(Teuchos::RCP<const Epetra_Map> mm)
{
  meshmap_->Setup(*embfluiddis_->DofRowMap(),mm,LINALG::SplitMap(*embfluiddis_->DofRowMap(),*mm));
  return;
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::FluidFluidFSI::ApplyMeshVelocity(Teuchos::RCP<const Epetra_Vector> gridvel)
{
  meshmap_->InsertCondVector(gridvel,xfluidfluid_->ViewOfGridVel());
  return;
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::FluidFluidFSI::DisplacementToVelocity(Teuchos::RCP<Epetra_Vector> fcx)
{
  // get interface velocity at t(n)
  const Teuchos::RCP<Epetra_Vector> veln = Interface()->ExtractFSICondVector(Veln());
  /// We convert Delta d(n+1,i+1) to Delta u(n+1,i+1) here.
  // Delta d(n+1,i+1) = ( theta Delta u(n+1,i+1) + u(n) ) * dt
  double timescale = TimeScaling();
  fcx->Update(-timescale*xfluidfluid_->Dt(),*veln,timescale);

  return;
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::FluidFluidFSI::VelocityToDisplacement(Teuchos::RCP<Epetra_Vector> fcx)
{
  // get interface velocity at t(n)
  const Teuchos::RCP<Epetra_Vector> veln = Interface()->ExtractFSICondVector(Veln());

  /*
   * Delta d(n+1,i+1) = fac * [Delta u(n+1,i+1) + 2 * u(n)]
   *
   *             / = dt / 2   if interface time integration is second order
   * with fac = |
   *             \ = dt       if interface time integration is first order
   */
  double timescale = 1./TimeScaling();
  fcx->Update(xfluidfluid_->Dt(),*veln,timescale);
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::FluidFluidFSI::VelocityToDisplacement(
    Teuchos::RCP<Epetra_Vector> fcx,
    Teuchos::RCP<Epetra_Vector> ddgpre,
    Teuchos::RCP<Epetra_Vector> dugpre
)
{
#ifdef DEBUG
  // check, whether maps are the same
  if (! fcx->Map().SameAs(ddgpre->Map())) { dserror("Maps do not match, but they have to."); }
  if (! fcx->Map().SameAs(dugpre->Map())) { dserror("Maps do not match, but they have to."); }
#endif

  // get interface velocity at t(n)
  const Teuchos::RCP<Epetra_Vector> veln = Interface()->ExtractFSICondVector(Veln());

  /*
   * Delta d(n+1,i+1) = fac * [ Delta u(n+1,i+1) + Delta u(predicted)]
   *
   *                  + dt * u(n) - Delta d_structure(predicted)
   *
   *             / = dt / 2   if interface time integration is second order
   * with fac = |
   *             \ = dt       if interface time integration is first order
   */

  // NOTE: if we use steady_state predictors (only the old solution) dugpre and
  // ddqpre are zero

  const double ts = 1.0/TimeScaling();
  fcx->Update(xfluidfluid_->Dt(), *veln, ts, *dugpre, ts);
  fcx->Update(-1.0, *ddgpre, 1.0);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void ADAPTER::FluidFluidFSI::UseBlockMatrix(bool splitmatrix)
{
  Teuchos::RCP<std::set<int> > condelements = Interface()->ConditionedElementMap(*Discretization());
  xfluidfluid_->UseBlockMatrix(condelements,*Interface(),*Interface(),splitmatrix);
}
