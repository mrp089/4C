#include "fsi_fluidfluidmonolithic_structuresplit_nonox.H"
#include "../drt_adapter/adapter_coupling.H"
#include "fsi_matrixtransform.H"

//#include "fsi_overlapprec_fsiamg.H"
#include "fsi_debugwriter.H"
#include "fsi_statustest.H"
#include "fsi_monolithic_linearsystem.H"

#include "../drt_lib/drt_globalproblem.H"
#include "../drt_inpar/inpar_fsi.H"
#include "../drt_fluid/fluid_utils_mapextractor.H"
#include "../drt_structure/stru_aux.H"
#include "../drt_inpar/inpar_xfem.H"

#include "../drt_io/io_control.H"


/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | general problem data                                                 |
 | global variable GENPROB genprob is defined in global_control.c       |
 *----------------------------------------------------------------------*/
extern struct _GENPROB     genprob;

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
FSI::FluidFluidMonolithicStructureSplitNoNOX::FluidFluidMonolithicStructureSplitNoNOX(const Epetra_Comm& comm,
                                                                            const Teuchos::ParameterList& timeparams)
  : MonolithicNoNOX(comm,timeparams)
{
  icoupfa_ = Teuchos::rcp(new ADAPTER::Coupling());
  fscoupfa_ = Teuchos::rcp(new ADAPTER::Coupling());

  sggtransform_ = Teuchos::rcp(new UTILS::MatrixRowColTransform);
  sgitransform_ = Teuchos::rcp(new UTILS::MatrixRowTransform);
  sigtransform_ = Teuchos::rcp(new UTILS::MatrixColTransform);
  aigtransform_ = Teuchos::rcp(new UTILS::MatrixColTransform);
  fmiitransform_ = Teuchos::rcp(new UTILS::MatrixColTransform);
  fmgitransform_ = Teuchos::rcp(new UTILS::MatrixColTransform);
  fsaigtransform_ = Teuchos::rcp(new UTILS::MatrixColTransform);
  fsmgitransform_ = Teuchos::rcp(new UTILS::MatrixColTransform);

  const Teuchos::ParameterList& xdyn = DRT::Problem::Instance()->XFEMGeneralParams();
  monolithic_approach_  = DRT::INPUT::IntegralValue<INPAR::XFEM::Monolithic_xffsi_Approach>
                          (xdyn,"MONOLITHIC_XFFSI_APPROACH");

  currentstep_ = 0;
  relaxing_ale_ = xdyn.get<int>("RELAXING_ALE");

  // Recovering of Lagrange multiplier happens on structure field
  lambda_ = Teuchos::rcp(new Epetra_Vector(*StructureField().Interface()->FSICondMap()));
  ddiinc_ = Teuchos::null;
  solipre_ = Teuchos::null;
  ddginc_ = Teuchos::null;
  solgpre_ = Teuchos::null;
//  fgpre_ = Teuchos::null;
//   sgipre_ = Teuchos::null;
//   sggpre_ = Teuchos::null;

  return;
}
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::SetupSystem()
{
  const Teuchos::ParameterList& fsidyn   = DRT::Problem::Instance()->FSIDynamicParams();
  linearsolverstrategy_ = DRT::INPUT::IntegralValue<INPAR::FSI::LinearBlockSolver>(fsidyn,"LINEARBLOCKSOLVER");

  // right now we use matching meshes at the interface

  ADAPTER::Coupling& coupsf = StructureFluidCoupling();
  ADAPTER::Coupling& coupsa = StructureAleCoupling();
  ADAPTER::Coupling& coupfa = FluidAleCoupling();

  // structure to fluid
  coupsf.SetupConditionCoupling(*StructureField().Discretization(),
                                 StructureField().Interface()->FSICondMap(),
                                *FluidField().Discretization(),
                                 FluidField().Interface()->FSICondMap(),
                                "FSICoupling",
                                 genprob.ndim);

  // structure to ale
  coupsa.SetupConditionCoupling(*StructureField().Discretization(),
                                 StructureField().Interface()->FSICondMap(),
                                *AleField().Discretization(),
                                 AleField().Interface().FSICondMap(),
                                "FSICoupling",
                                 genprob.ndim);

  // fluid to ale at the interface
  icoupfa_->SetupConditionCoupling(*FluidField().Discretization(),
                                    FluidField().Interface()->FSICondMap(),
                                   *AleField().Discretization(),
                                    AleField().Interface().FSICondMap(),
                                   "FSICoupling",
                                    genprob.ndim);

  // In the following we assume that both couplings find the same dof
  // map at the structural side. This enables us to use just one
  // interface dof map for all fields and have just one transfer
  // operator from the interface map to the full field map.
  if (not coupsf.MasterDofMap()->SameAs(*coupsa.MasterDofMap()))
    dserror("structure interface dof maps do not match");

  if (coupsf.MasterDofMap()->NumGlobalElements()==0)
    dserror("No nodes in matching FSI interface. Empty FSI coupling condition?");

  // the fluid-ale coupling always matches
  const Epetra_Map* embfluidnodemap = FluidField().Discretization()->NodeRowMap();
  const Epetra_Map* alenodemap   = AleField().Discretization()->NodeRowMap();

  coupfa.SetupCoupling(*FluidField().Discretization(),
                       *AleField().Discretization(),
                       *embfluidnodemap,
                       *alenodemap,
                       genprob.ndim);

  FluidField().SetMeshMap(coupfa.MasterDofMap());

  // create combined map
  std::vector<Teuchos::RCP<const Epetra_Map> > vecSpaces;
  vecSpaces.push_back(StructureField().Interface()->OtherMap());
  vecSpaces.push_back(FluidField()    .DofRowMap());
  vecSpaces.push_back(AleField()      .Interface().OtherMap());

  if (vecSpaces[0]->NumGlobalElements()==0)
    dserror("No inner structural equations. Splitting not possible. Panic.");

  SetDofRowMaps(vecSpaces);

  // Use normal matrix for fluid equations but build (splitted) mesh movement
  // linearization (if requested in the input file)
  FluidField().UseBlockMatrix(false);

  // Use splitted structure matrix
  StructureField().UseBlockMatrix();

  // build ale system matrix in splitted system
  AleField().BuildSystemMatrix(false);

  aleresidual_ = Teuchos::rcp(new Epetra_Vector(*AleField().Interface().OtherMap()));

  /*----------------------------------------------------------------------*/
  // initialize systemmatrix_
  systemmatrix_
    = rcp(
        new LINALG::BlockSparseMatrix<LINALG::DefaultBlockMatrixStrategy>(
              Extractor(),
              Extractor(),
              81,
              false,
              true
              )
      );

}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::SetupRHS(Epetra_Vector& f, bool firstcall)
{
  TEUCHOS_FUNC_TIME_MONITOR("FSI::MonolithicStructureSplit::SetupRHS");

  // get time integration parameters of structure an fluid time integrators
  // to enable consistent time integration among the fields
  double stiparam = StructureField().TimIntParam();
  double ftiparam = FluidField().TimIntParam();

  SetupVector(f,
              StructureField().RHS(),
              FluidField().RHS(),
              AleField().RHS(),
              FluidField().ResidualScaling());


  // add additional ale residual
  Extractor().AddVector(*aleresidual_,2,f);

  if (firstcall)
  {
    // additional rhs term for ALE equations
    // -dt Aig u(n)
    //
    //    1/dt Delta d(n+1) = theta Delta u(n+1) + u(n)
    //
    // And we are concerned with the u(n) part here.

    Teuchos::RCP<LINALG::BlockSparseMatrixBase> a = AleField().BlockSystemMatrix();
    if (a==Teuchos::null)
      dserror("expect ale block matrix");

    LINALG::SparseMatrix& aig = a->Matrix(0,1);

    Teuchos::RCP<Epetra_Vector> fveln = FluidField().ExtractInterfaceVeln();
    Teuchos::RCP<Epetra_Vector> sveln = FluidToStruct(fveln);
    Teuchos::RCP<Epetra_Vector> aveln = StructToAle(sveln);
    Teuchos::RCP<Epetra_Vector> rhs = Teuchos::rcp(new Epetra_Vector(aig.RowMap()));
    aig.Apply(*aveln,*rhs);
    rhs->Scale(-1.*Dt());

    Extractor().AddVector(*rhs,2,f); // add ALE contributions to 'f'
    // structure
    Teuchos::RCP<Epetra_Vector> veln = StructureField().Interface()->InsertFSICondVector(sveln);
    rhs = Teuchos::rcp(new Epetra_Vector(veln->Map()));

    Teuchos::RCP<LINALG::BlockSparseMatrixBase> s = StructureField().BlockSystemMatrix();
    s->Apply(*veln,*rhs);

    rhs->Scale(-1.*Dt());

    veln = StructureField().Interface()->ExtractOtherVector(rhs); // only inner DOFs
    Extractor().AddVector(*veln,0,f); // add inner structure contributions to 'f'

    veln = StructureField().Interface()->ExtractFSICondVector(rhs); // only DOFs on interface
    veln = FluidField().Interface()->InsertFSICondVector(StructToFluid(veln)); // convert to fluid map

    double scale     = FluidField().ResidualScaling();

    veln->Scale(((1.0-ftiparam)/(1.0-stiparam))*(1./scale));

    // we need a temporary vector with the whole fluid dofs where we
    // can insert veln which has the embedded dofrowmap into it
    Teuchos::RCP<Epetra_Vector> fluidfluidtmp = LINALG::CreateVector(*FluidField().DofRowMap(),true);
    xfluidfluidsplitter_->InsertFluidVector(veln,fluidfluidtmp);

    Extractor().AddVector(*fluidfluidtmp,1,f);


    // shape derivatives
    Teuchos::RCP<LINALG::BlockSparseMatrixBase> mmm = FluidField().ShapeDerivatives();
    if (mmm!=Teuchos::null)
    {
      LINALG::SparseMatrix& fmig = mmm->Matrix(0,1);
      LINALG::SparseMatrix& fmgg = mmm->Matrix(1,1);

      rhs = Teuchos::rcp(new Epetra_Vector(fmig.RowMap()));
      fmig.Apply(*fveln,*rhs);
      veln = FluidField().Interface()->InsertOtherVector(rhs);

      rhs = Teuchos::rcp(new Epetra_Vector(fmgg.RowMap()));
      fmgg.Apply(*fveln,*rhs);
      FluidField().Interface()->InsertFSICondVector(rhs,veln);

      veln->Scale(-1.*Dt());

      // add veln (embedded mesh dofs) to a fluidfluidtmp which contains the
      // whole fluidfluid dofs

      fluidfluidtmp->PutScalar(0.0);
      xfluidfluidsplitter_->InsertFluidVector(veln,fluidfluidtmp);
      Extractor().AddVector(*fluidfluidtmp,1,f);

    }
  }
  // store interface force onto the structure to know it in the next time step as previous force
  // in order to recover the Lagrange multiplier
  // fgpre_ = fgcur_;
  fgcur_ = StructureField().Interface()->ExtractFSICondVector(StructureField().RHS());
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::SetupSystemMatrix()
{
  TEUCHOS_FUNC_TIME_MONITOR("FSI::MonolithicStructureSplit::SetupSystemMatrix");


  // extract Jacobian matrices and put them into composite system
  // matrix W

  const ADAPTER::Coupling& coupsf = StructureFluidCoupling();
  //const ADAPTER::Coupling& coupsa = StructureAleCoupling();

  Teuchos::RCP<LINALG::BlockSparseMatrixBase> s = StructureField().BlockSystemMatrix();
  if (s==Teuchos::null)
    dserror("expect structure block matrix");
  Teuchos::RCP<LINALG::SparseMatrix> f = FluidField().SystemMatrix();
  if (f==Teuchos::null)
    dserror("expect fluid matrix");
  Teuchos::RCP<LINALG::BlockSparseMatrixBase> a = AleField().BlockSystemMatrix();
  if (a==Teuchos::null)
    dserror("expect ale block matrix");

  LINALG::SparseMatrix& aii = a->Matrix(0,0);
  LINALG::SparseMatrix& aig = a->Matrix(0,1);


  sgicur_ = rcp(new LINALG::SparseMatrix(s->Matrix(1,0)));
  sggcur_ = rcp(new LINALG::SparseMatrix(s->Matrix(1,1)));

  /*----------------------------------------------------------------------*/

  double scale     = FluidField().ResidualScaling();
  double timescale = FluidField().TimeScaling();


  // build block matrix
  // The maps of the block matrix have to match the maps of the blocks we
  // insert here.

  // get time integration parameters of structure an fluid time integrators
  // to enable consistent time integration among the fields
  double stiparam = StructureField().TimIntParam();
  double ftiparam = FluidField().TimIntParam();

  // Uncomplete fluid matrix to be able to deal with slightly defective
  // interface meshes.
  f->UnComplete();

  systemmatrix_->Assign(0,0,View,s->Matrix(0,0));

  (*sigtransform_)(s->FullRowMap(),
                   s->FullColMap(),
                   s->Matrix(0,1),
                   1./timescale,
                   ADAPTER::CouplingMasterConverter(coupsf),
                   systemmatrix_->Matrix(0,1));
  (*sggtransform_)(s->Matrix(1,1),
                   ((1.0-ftiparam)/(1.0-stiparam))*1./(scale*timescale),
                   ADAPTER::CouplingMasterConverter(coupsf),
                   ADAPTER::CouplingMasterConverter(coupsf),
                   *f,
                   true,
                   true);
  (*sgitransform_)(s->Matrix(1,0),
                   ((1.0-ftiparam)/(1.0-stiparam))*(1./scale),
                   ADAPTER::CouplingMasterConverter(coupsf),
                   systemmatrix_->Matrix(1,0));

  systemmatrix_->Assign(1,1,View,*f);

  (*aigtransform_)(a->FullRowMap(),
                   a->FullColMap(),
                   aig,
                   1./timescale,
                   ADAPTER::CouplingSlaveConverter(*icoupfa_),
                   systemmatrix_->Matrix(2,1));
  systemmatrix_->Assign(2,2,View,aii);

  /*----------------------------------------------------------------------*/
  // add optional fluid linearization with respect to mesh motion block

  Teuchos::RCP<LINALG::BlockSparseMatrixBase> mmm = FluidField().ShapeDerivatives();
  if (mmm!=Teuchos::null)
  {
    LINALG::SparseMatrix& fmii = mmm->Matrix(0,0);
    LINALG::SparseMatrix& fmig = mmm->Matrix(0,1);
    LINALG::SparseMatrix& fmgi = mmm->Matrix(1,0);
    LINALG::SparseMatrix& fmgg = mmm->Matrix(1,1);

    systemmatrix_->Matrix(1,1).Add(fmgg,false,1./timescale,1.0);
    systemmatrix_->Matrix(1,1).Add(fmig,false,1./timescale,1.0);

    const ADAPTER::Coupling& coupfa = FluidAleCoupling();

    (*fmgitransform_)(mmm->FullRowMap(),
                      mmm->FullColMap(),
                      fmgi,
                      1.,
                      ADAPTER::CouplingMasterConverter(coupfa),
                      systemmatrix_->Matrix(1,2),
                      false,
                      false);

    (*fmiitransform_)(mmm->FullRowMap(),
                      mmm->FullColMap(),
                      fmii,
                      1.,
                      ADAPTER::CouplingMasterConverter(coupfa),
                      systemmatrix_->Matrix(1,2),
                      false,
                      true);
  }

  // done. make sure all blocks are filled.
  systemmatrix_->Complete();
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::InitialGuess(Teuchos::RCP<Epetra_Vector> ig)
{
  TEUCHOS_FUNC_TIME_MONITOR("FSI::MonolithicStructureSplit::InitialGuess");

  SetupVector(*ig,
              StructureField().InitialGuess(),
              FluidField().InitialGuess(),
              AleField().InitialGuess(),
              0.0);
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::ScaleSystem(LINALG::BlockSparseMatrixBase& mat, Epetra_Vector& b)
{
  //should we scale the system?
  const Teuchos::ParameterList& fsidyn   = DRT::Problem::Instance()->FSIDynamicParams();
  const bool scaling_infnorm = (bool)DRT::INPUT::IntegralValue<int>(fsidyn,"INFNORMSCALING");

  if (scaling_infnorm)
  {
    // The matrices are modified here. Do we have to change them back later on?

    Teuchos::RCP<Epetra_CrsMatrix> A = mat.Matrix(0,0).EpetraMatrix();
    srowsum_ = rcp(new Epetra_Vector(A->RowMap(),false));
    scolsum_ = rcp(new Epetra_Vector(A->RowMap(),false));
    A->InvRowSums(*srowsum_);
    A->InvColSums(*scolsum_);
    if (A->LeftScale(*srowsum_) or
        A->RightScale(*scolsum_) or
        mat.Matrix(0,1).EpetraMatrix()->LeftScale(*srowsum_) or
        mat.Matrix(0,2).EpetraMatrix()->LeftScale(*srowsum_) or
        mat.Matrix(1,0).EpetraMatrix()->RightScale(*scolsum_) or
        mat.Matrix(2,0).EpetraMatrix()->RightScale(*scolsum_))
      dserror("structure scaling failed");

    A = mat.Matrix(2,2).EpetraMatrix();
    arowsum_ = rcp(new Epetra_Vector(A->RowMap(),false));
    acolsum_ = rcp(new Epetra_Vector(A->RowMap(),false));
    A->InvRowSums(*arowsum_);
    A->InvColSums(*acolsum_);
    if (A->LeftScale(*arowsum_) or
        A->RightScale(*acolsum_) or
        mat.Matrix(2,0).EpetraMatrix()->LeftScale(*arowsum_) or
        mat.Matrix(2,1).EpetraMatrix()->LeftScale(*arowsum_) or
        mat.Matrix(0,2).EpetraMatrix()->RightScale(*acolsum_) or
        mat.Matrix(1,2).EpetraMatrix()->RightScale(*acolsum_))
      dserror("ale scaling failed");

    Teuchos::RCP<Epetra_Vector> sx = Extractor().ExtractVector(b,0);
    Teuchos::RCP<Epetra_Vector> ax = Extractor().ExtractVector(b,2);

    if (sx->Multiply(1.0, *srowsum_, *sx, 0.0))
      dserror("structure scaling failed");
    if (ax->Multiply(1.0, *arowsum_, *ax, 0.0))
      dserror("ale scaling failed");

    Extractor().InsertVector(*sx,0,b);
    Extractor().InsertVector(*ax,2,b);
  }
}

/*----------------------------------------------------------------------*
 |  map containing the dofs with Dirichlet BC
 *----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Map> FSI::FluidFluidMonolithicStructureSplitNoNOX::CombinedDBCMap()
{
  const Teuchos::RCP<const Epetra_Map > scondmap = StructureField().GetDBCMapExtractor()->CondMap();
  const Teuchos::RCP<const Epetra_Map> ffcondmap = FluidField().FluidDirichMaps();
  const Teuchos::RCP<const Epetra_Map > acondmap = AleField().GetDBCMapExtractor()->CondMap();

  // this is a structure split so we leave the fluid map unchanged. It
  // means that the dirichlet dofs could also contain fsi dofs. So if
  // you want to prescribe any dirichlet values at the fsi-interface it
  // is the fluid field which decides.

  std::vector<Teuchos::RCP<const Epetra_Map> > vectoroverallfsimaps;
  vectoroverallfsimaps.push_back(scondmap);
  vectoroverallfsimaps.push_back(ffcondmap);
  vectoroverallfsimaps.push_back(acondmap);
  Teuchos::RCP<Epetra_Map> overallfsidbcmaps = LINALG::MultiMapExtractor::MergeMaps(vectoroverallfsimaps);

  //structure and ale maps should not have any fsiCondDofs, so we
  //throw them away from overallfsidbcmaps

  vector<int> otherdbcmapvector; //vector of dbc
  const int mylength = overallfsidbcmaps->NumMyElements(); //on each prossesor (lids)
  const int* mygids = overallfsidbcmaps->MyGlobalElements();
  for (int i=0; i<mylength; ++i)
  {
    int gid = mygids[i];
    int fullmaplid = fullmap_->LID(gid);
    // if it is not a fsi dof
    if (fullmaplid >= 0)
      otherdbcmapvector.push_back(gid);
  }

  Teuchos::RCP<Epetra_Map> otherdbcmap = rcp(new Epetra_Map(-1, otherdbcmapvector.size(), &otherdbcmapvector[0], 0, Comm()));

  return otherdbcmap;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::UnscaleSolution(LINALG::BlockSparseMatrixBase& mat, Epetra_Vector& x, Epetra_Vector& b)
{
  const Teuchos::ParameterList& fsidyn   = DRT::Problem::Instance()->FSIDynamicParams();
  const bool scaling_infnorm = (bool)DRT::INPUT::IntegralValue<int>(fsidyn,"INFNORMSCALING");

  if (scaling_infnorm)
  {
    Teuchos::RCP<Epetra_Vector> sy = Extractor().ExtractVector(x,0);
    Teuchos::RCP<Epetra_Vector> ay = Extractor().ExtractVector(x,2);

    if (sy->Multiply(1.0, *scolsum_, *sy, 0.0))
      dserror("structure scaling failed");
    if (ay->Multiply(1.0, *acolsum_, *ay, 0.0))
      dserror("ale scaling failed");

    Extractor().InsertVector(*sy,0,x);
    Extractor().InsertVector(*ay,2,x);

    Teuchos::RCP<Epetra_Vector> sx = Extractor().ExtractVector(b,0);
    Teuchos::RCP<Epetra_Vector> ax = Extractor().ExtractVector(b,2);

    if (sx->ReciprocalMultiply(1.0, *srowsum_, *sx, 0.0))
      dserror("structure scaling failed");
    if (ax->ReciprocalMultiply(1.0, *arowsum_, *ax, 0.0))
      dserror("ale scaling failed");

    Extractor().InsertVector(*sx,0,b);
    Extractor().InsertVector(*ax,2,b);

    Teuchos::RCP<Epetra_CrsMatrix> A = mat.Matrix(0,0).EpetraMatrix();
    srowsum_->Reciprocal(*srowsum_);
    scolsum_->Reciprocal(*scolsum_);
    if (A->LeftScale(*srowsum_) or
        A->RightScale(*scolsum_) or
        mat.Matrix(0,1).EpetraMatrix()->LeftScale(*srowsum_) or
        mat.Matrix(0,2).EpetraMatrix()->LeftScale(*srowsum_) or
        mat.Matrix(1,0).EpetraMatrix()->RightScale(*scolsum_) or
        mat.Matrix(2,0).EpetraMatrix()->RightScale(*scolsum_))
      dserror("structure scaling failed");

    A = mat.Matrix(2,2).EpetraMatrix();
    arowsum_->Reciprocal(*arowsum_);
    acolsum_->Reciprocal(*acolsum_);
    if (A->LeftScale(*arowsum_) or
        A->RightScale(*acolsum_) or
        mat.Matrix(2,0).EpetraMatrix()->LeftScale(*arowsum_) or
        mat.Matrix(2,1).EpetraMatrix()->LeftScale(*arowsum_) or
        mat.Matrix(0,2).EpetraMatrix()->RightScale(*acolsum_) or
        mat.Matrix(1,2).EpetraMatrix()->RightScale(*acolsum_))
      dserror("ale scaling failed");

  }
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::SetupVector(Epetra_Vector &f,
                                                          Teuchos::RCP<const Epetra_Vector> sv,
                                                          Teuchos::RCP<const Epetra_Vector> fv,
                                                          Teuchos::RCP<const Epetra_Vector> av,
                                                          double fluidscale)
{
  // get time integration parameters of structure an fluid time integrators
  // to enable consistent time integration among the fields
  double stiparam = StructureField().TimIntParam();
  double ftiparam = FluidField().TimIntParam();

  // structure inner
  Teuchos::RCP<Epetra_Vector> sov = StructureField().Interface()->ExtractOtherVector(sv);

  // ale inner
  Teuchos::RCP<Epetra_Vector> aov = AleField()      .Interface().ExtractOtherVector(av);

  if (fluidscale!=0)
  {
    // add fluid interface values to structure vector
    // scv: structure fsi dofs
    Teuchos::RCP<Epetra_Vector> scv = StructureField().Interface()->ExtractFSICondVector(sv);

    // modfv: whole embedded fluid map but entries at fsi dofs
    Teuchos::RCP<Epetra_Vector> modfv = FluidField().Interface()->InsertFSICondVector(StructToFluid(scv));

    // modfv = modfv * 1/fluidscale * (1.0-ftiparam)/(1.0-stiparam)
    modfv->Scale( 1./fluidscale*(1.0-ftiparam)/(1.0-stiparam));

    // add contribution of Lagrange multiplier from previous time step
    if (lambda_ != Teuchos::null)
      modfv->Update(-ftiparam+stiparam*(1.0-ftiparam)/(1.0-stiparam), *StructToFluid(lambda_), 1.0);

    // we need a temporary vector with the whole fluid dofs where we
    // can insert veln which has the embedded dofrowmap into it
    Teuchos::RCP<Epetra_Vector> fluidfluidtmp = LINALG::CreateVector(*FluidField().DofRowMap(),true);
    xfluidfluidsplitter_->InsertFluidVector(modfv,fluidfluidtmp);

    // all fluid dofs
    Teuchos::RCP<Epetra_Vector> fvfluidfluid = Teuchos::rcp_const_cast< Epetra_Vector >(fv);

    // adding modfv to fvfluidfluid
    fvfluidfluid->Update(1.0,*fluidfluidtmp,1.0);

    Extractor().InsertVector(*fvfluidfluid,1,f);
  }
  else
  {
    Extractor().InsertVector(*fv,1,f);
  }

  Extractor().InsertVector(*sov,0,f);
  Extractor().InsertVector(*aov,2,f);
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::ExtractFieldVectors(Teuchos::RCP<const Epetra_Vector> x,
                                                                       Teuchos::RCP<const Epetra_Vector>& sx,
                                                                       Teuchos::RCP<const Epetra_Vector>& fx,
                                                                       Teuchos::RCP<const Epetra_Vector>& ax)
{
  TEUCHOS_FUNC_TIME_MONITOR("FSI::MonolithicStructureSplit::ExtractFieldVectors");

  // process fluid unknowns
  fx = Extractor().ExtractVector(x,1);

  // extract embedded fluid vector
  Teuchos::RCP<Epetra_Vector> fx_emb = xfluidfluidsplitter_->ExtractFluidVector(fx);

  // process structure unknowns
  Teuchos::RCP<Epetra_Vector> fcx = FluidField().Interface()->ExtractFSICondVector(fx_emb);

  FluidField().VelocityToDisplacement(fcx);
  Teuchos::RCP<const Epetra_Vector> sox = Extractor().ExtractVector(x,0);
  Teuchos::RCP<Epetra_Vector> scx = FluidToStruct(fcx);

  Teuchos::RCP<Epetra_Vector> s = StructureField().Interface()->InsertOtherVector(sox);
  StructureField().Interface()->InsertFSICondVector(scx, s);
  sx = s;

  // process ale unknowns
  Teuchos::RCP<const Epetra_Vector> aox = Extractor().ExtractVector(x,2);
  Teuchos::RCP<Epetra_Vector> acx = StructToAle(scx);

  Teuchos::RCP<Epetra_Vector> a = AleField().Interface().InsertOtherVector(aox);
  AleField().Interface().InsertFSICondVector(acx, a);

  ax = a;

  // Store field vectors to know them later on as previous quantities
  if (solipre_ != Teuchos::null)
    ddiinc_->Update(1.0, *sox, -1.0, *solipre_, 0.0);  // compute current iteration increment
  else
    ddiinc_ = rcp(new Epetra_Vector(*sox));           // first iteration increment

  solipre_ = sox;                                      // store current step increment

  if (solgpre_ != Teuchos::null)
    ddginc_->Update(1.0, *scx, -1.0, *solgpre_, 0.0);  // compute current iteration increment
  else
    ddginc_ = rcp(new Epetra_Vector(*scx));           // first iteration increment

  solgpre_ = scx;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::PrepareTimeStep()
{
  IncrementTimeAndStep();

  PrintHeader();

  StructureField().PrepareTimeStep();
  FluidField().    PrepareTimeStep();
  AleField().      PrepareTimeStep();

  if (monolithic_approach_!=INPAR::XFEM::XFFSI_Full_Newton)
    SetupNewSystem();

  //xfluidfluid splitter
  xfluidfluidsplitter_ = FluidField().XFluidFluidMapExtractor();
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::Update()
{
  currentstep_ ++;

//  cout <<"currentstep_" <<  currentstep_ <<" " << currentstep_%relaxing_ale_<< endl;
  bool aleupdate = false;
  if (currentstep_%relaxing_ale_==0) aleupdate = true;

  if (monolithic_approach_!= INPAR::XFEM::XFFSI_Full_Newton and aleupdate)
  {
    AleField().SolveAleXFluidFluidFSI();
    FluidField().ApplyMeshDisplacement(AleToFluid(AleField().ExtractDisplacement()));
  }

  StructureField().Update();
  FluidField().    Update();
  AleField().      Update();


  if (monolithic_approach_!= INPAR::XFEM::XFFSI_Full_Newton and aleupdate)
  {
    // build ale system matrix for the next time step. Here first we
    // update the vectors then we set the fluid-fluid dirichlet values
    // in buildsystemmatrix
    AleField().BuildSystemMatrix(false);
    aleresidual_ = Teuchos::rcp(new Epetra_Vector(*AleField().Interface().OtherMap()));
  }
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::SetupNewSystem()
{

  // create combined map
  std::vector<Teuchos::RCP<const Epetra_Map> > vecSpaces;
  vecSpaces.push_back(StructureField().Interface()->OtherMap());
  vecSpaces.push_back(FluidField()    .DofRowMap());
  vecSpaces.push_back(AleField()      .Interface().OtherMap());

  if (vecSpaces[0]->NumGlobalElements()==0)
    dserror("No inner structural equations. Splitting not possible. Panic.");

  SetDofRowMaps(vecSpaces);

  /*----------------------------------------------------------------------*/
  // initialize systemmatrix_
  systemmatrix_
    = rcp(
      new LINALG::BlockSparseMatrix<LINALG::DefaultBlockMatrixStrategy>(
        Extractor(),
        Extractor(),
        81,
        false,
        true
        )
      );
}
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::Newton()
{
  // initialise equilibrium loop
  iter_ = 1;

  x_sum_ = LINALG::CreateVector(*DofRowMap(),true);
  x_sum_->PutScalar(0.0);

  // incremental solution vector with length of all FSI dofs
  iterinc_ = LINALG::CreateVector(*DofRowMap(), true);
  iterinc_->PutScalar(0.0);

  zeros_ = LINALG::CreateVector(*DofRowMap(), true);
  zeros_->PutScalar(0.0);

  // residual vector with length of all FSI dofs
  rhs_ = LINALG::CreateVector(*DofRowMap(), true);
  rhs_->PutScalar(0.0);

  firstcall_ = true;

  // equilibrium iteration loop (loop over k)
  while ( ((not Converged()) and (iter_ <= itermax_)) or (iter_ ==  1) )
  {
    // compute residual forces #rhs_ and tangent #tang_
    // build linear system stiffness matrix and rhs/force
    // residual for each field

    Evaluate(iterinc_);

    if (not FluidField().DofRowMap()->SameAs(Extractor().ExtractVector(iterinc_,1)->Map()))
    {
      cout << GREEN_LIGHT << " New Map!! " <<  END_COLOR <<  endl;
      // save the old x_sum
      Teuchos::RCP<Epetra_Vector> x_sum_n =  LINALG::CreateVector(*DofRowMap(), true);
      *x_sum_n = *x_sum_;
      Teuchos::RCP<const Epetra_Vector> sx_n;
      Teuchos::RCP<const Epetra_Vector> ax_n;
      sx_n = Extractor().ExtractVector(x_sum_n,0);
      ax_n = Extractor().ExtractVector(x_sum_n,2);

      SetupNewSystem();
      xfluidfluidsplitter_ = FluidField().XFluidFluidMapExtractor();
      rhs_ = LINALG::CreateVector(*DofRowMap(), true);
      iterinc_ = LINALG::CreateVector(*DofRowMap(), true);
      zeros_ = LINALG::CreateVector(*DofRowMap(), true);
      x_sum_ = LINALG::CreateVector(*DofRowMap(),true);

      // build the new iter_sum
      Extractor().InsertVector(sx_n,0,x_sum_);
      Extractor().InsertVector(FluidField().Stepinc(),1,x_sum_);
      Extractor().InsertVector(ax_n,2,x_sum_);
      nf_ = (*(FluidField().RHS())).GlobalLength();
    }

    // create the linear system
    // J(x_i) \Delta x_i = - R(x_i)
    // create the systemmatrix
    SetupSystemMatrix();

    // check whether we have a sanely filled tangent matrix
    if (not systemmatrix_->Filled())
    {
      dserror("Effective tangent matrix must be filled here");
    }

    SetupRHS(*rhs_,firstcall_);

    LinearSolve();

    // reset solver tolerance
    solver_->ResetTolerance();

    // build residual and incremental norms
    // for now use for simplicity only L2/Euclidian norm
    BuildCovergenceNorms();

    // print stuff
    PrintNewtonIter();

    // increment equilibrium loop index
    iter_ += 1;

    firstcall_ = false;

  }// end while loop

  // correct iteration counter
  iter_ -= 1;

  // test whether max iterations was hit
  if ( (Converged()) and (Comm().MyPID()==0) )
  {
    cout << endl;
    cout << endl;
    cout << BLUE_LIGHT << "  Newton Converged! " <<  END_COLOR<<  endl;

  }
  else if (iter_ >= itermax_)
  {
    cout << endl;
    cout << endl;
    cout << RED_LIGHT << " Newton unconverged in "<< iter_ << " iterations " << END_COLOR<<  endl;

  }
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::FluidFluidMonolithicStructureSplitNoNOX::BuildCovergenceNorms()
{
  /////////////////////////////
  // build residual norms
  rhs_->Norm2(&normrhs_);

  // structural Dofs
  StructureField().RHS()->Norm2(&normstrrhs_);

  // interface

  // extract embedded fluid vector
  Teuchos::RCP<Epetra_Vector> rhs_emb = xfluidfluidsplitter_->ExtractFluidVector(FluidField().RHS());
  FluidField().Interface()->ExtractFSICondVector(rhs_emb)->Norm2(&norminterfacerhs_);

  // inner fluid velocity Dofs
  std::vector<Teuchos::RCP<const Epetra_Map> > innerfluidvel;
  innerfluidvel.push_back(FluidField().InnerVelocityRowMap());
  innerfluidvel.push_back(Teuchos::null);
  LINALG::MultiMapExtractor fluidvelextract(*(FluidField().DofRowMap()),innerfluidvel);
  fluidvelextract.ExtractVector(FluidField().RHS(),0)->Norm2(&normflvelrhs_);

  // fluid pressure Dofs
  std::vector<Teuchos::RCP<const Epetra_Map> > fluidpres;
  fluidpres.push_back(FluidField().PressureRowMap());
  fluidpres.push_back(Teuchos::null);
  LINALG::MultiMapExtractor fluidpresextract(*(FluidField().DofRowMap()),fluidpres);
  fluidpresextract.ExtractVector(FluidField().RHS(),0)->Norm2(&normflpresrhs_);

  // fluid
  FluidField().RHS()->Norm2(&normflrhs_);

  // ale
  AleField().RHS()->Norm2(&normalerhs_);

  ///////////////////////////////////
  // build solution increment norms

  // build increment norm
  iterinc_->Norm2(&norminc_);

  // structural Dofs
  Extractor().ExtractVector(iterinc_,0)->Norm2(&normstrinc_);

  // interface
  // extract embedded fluid vector
  Teuchos::RCP<Epetra_Vector> inc_emb = xfluidfluidsplitter_->ExtractFluidVector(Extractor().ExtractVector(iterinc_,1));
  FluidField().Interface()->ExtractFSICondVector(inc_emb)->Norm2(&norminterfaceinc_);

  // inner fluid velocity Dofs
  fluidvelextract.ExtractVector(Extractor().ExtractVector(iterinc_,1),0)->Norm2(&normflvelinc_);

  // fluid pressure Dofs
  fluidpresextract.ExtractVector(Extractor().ExtractVector(iterinc_,1),0)->Norm2(&normflpresinc_);

  // ale
  Extractor().ExtractVector(iterinc_,2)->Norm2(&normaleinc_);

  //get length of the structural, fluid and ale vector
  ns_ = (*(StructureField().RHS())).GlobalLength(); //structure
  ni_ = (*(FluidField().Interface()->ExtractFSICondVector(rhs_emb))).GlobalLength(); //fluid fsi
  nf_ = (*(FluidField().RHS())).GlobalLength(); //fluid inner
  nfv_ = (*(fluidvelextract.ExtractVector(FluidField().RHS(),0))).GlobalLength(); //fluid velocity
  nfp_ = (*(fluidpresextract.ExtractVector(FluidField().RHS(),0))).GlobalLength();//fluid pressure
  na_ = (*(AleField().RHS())).GlobalLength(); //ale
  nall_ = (*rhs_).GlobalLength(); //all

}

/*----------------------------------------------------------------------*/
/* Recover the Lagrange multiplier at the interface                     */
/*----------------------------------------------------------------------*/
 void FSI::FluidFluidMonolithicStructureSplitNoNOX::RecoverLagrangeMultiplier()
 {
   // get time integration parameters of structural time integrator
   // to enable consistent time integration among the fields
   double stiparam = StructureField().TimIntParam();

   // compute the product S_{\Gamma I} \Delta d_I
   Teuchos::RCP<Epetra_Vector> sgiddi = LINALG::CreateVector(*StructureField().Interface()->OtherMap(),true); // store the prodcut 'S_{\GammaI} \Delta d_I^{n+1}' in here
   (sgicur_->EpetraMatrix())->Multiply(false, *ddiinc_, *sgiddi);

   // compute the product S_{\Gamma\Gamma} \Delta d_\Gamma
   Teuchos::RCP<Epetra_Vector> sggddg = LINALG::CreateVector(*StructureField().Interface()->OtherMap(),true); // store the prodcut '\Delta t / 2 * S_{\Gamma\Gamma} \Delta u_\Gamma^{n+1}' in here
   (sggcur_->EpetraMatrix())->Multiply(false, *ddginc_, *sggddg);

   // Update the Lagrange multiplier:
   /* \lambda^{n+1} =  - a/b*\lambda^n - f_\Gamma^S
    *                  - S_{\Gamma I} \Delta d_I - S_{\Gamma\Gamma} \Delta d_\Gamma
    */
   lambda_->Update(1.0, *fgcur_, -stiparam/(1.0-stiparam));
   lambda_->Update(-1.0, *sgiddi, -1.0, *sggddg, 1.0);
   lambda_->Scale(1/(1.0-stiparam)); // entire Lagrange multiplier ist divided by (1.-strtimintparam)

   return;
}
