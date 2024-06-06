/*----------------------------------------------------------------------*/
/*! \file
 \brief two-way coupled monolithic algorithm for scalar transport within multiphase porous medium

   \level 3

 *----------------------------------------------------------------------*/



#include "4C_poromultiphase_scatra_monolithic_twoway.hpp"

#include "4C_adapter_art_net.hpp"
#include "4C_adapter_porofluidmultiphase_wrapper.hpp"
#include "4C_adapter_scatra_base_algorithm.hpp"
#include "4C_adapter_str_structure.hpp"
#include "4C_discretization_fem_general_assemblestrategy.hpp"
#include "4C_global_data.hpp"
#include "4C_io_control.hpp"
#include "4C_lib_discret.hpp"
#include "4C_linalg_equilibrate.hpp"
#include "4C_linalg_utils_sparse_algebra_manipulation.hpp"
#include "4C_linear_solver_method.hpp"
#include "4C_linear_solver_method_linalg.hpp"
#include "4C_linear_solver_method_parameters.hpp"
#include "4C_poromultiphase_base.hpp"
#include "4C_scatra_ele_action.hpp"
#include "4C_scatra_timint_implicit.hpp"
#include "4C_scatra_timint_meshtying_strategy_artery.hpp"
#include "4C_utils_parameter_list.hpp"

#include <Teuchos_TimeMonitor.hpp>

FOUR_C_NAMESPACE_OPEN


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::PoroMultiPhaseScaTraMonolithicTwoWay(
    const Epetra_Comm& comm, const Teuchos::ParameterList& globaltimeparams)
    : PoroMultiPhaseScaTraMonolithic(comm, globaltimeparams),
      ittolinc_(0.0),
      ittolres_(0.0),
      itmax_(0),
      itmin_(1),
      itnum_(0),
      blockrowdofmap_(Teuchos::null),
      equilibration_(Teuchos::null),
      equilibration_method_(Core::LinAlg::EquilibrationMethod::none),
      solveradaptolbetter_(0.0),
      solveradapttol_(false),
      solve_structure_(true),
      struct_offset_(1),
      tolinc_(0.0),
      tolfres_(0.0),
      tolinc_struct_(0.0),
      tolfres_struct_(0.0),
      tolinc_fluid_(0.0),
      tolfres_fluid_(0.0),
      tolinc_scatra_(0.0),
      tolfres_scatra_(0.0),
      normrhs_(0.0),
      normrhsfluid_(0.0),
      normincfluid_(0.0),
      normrhsstruct_(0.0),
      normincstruct_(0.0),
      normrhsscatra_(0.0),
      normincscatra_(0.0),
      normrhsart_(0.0),
      normincart_(0.0),
      arterypressnorm_(0.0),
      normrhsartsca_(0.0),
      normincartsca_(0.0),
      arteryscanorm_(0.0),
      maxinc_(0.0),
      maxres_(0.0),
      vectornormfres_(Inpar::PoroMultiPhaseScaTra::norm_undefined),
      vectornorminc_(Inpar::PoroMultiPhaseScaTra::norm_undefined),
      timernewton_("", true),
      dtsolve_(0.0),
      dtele_(0.0),
      fdcheck_(Inpar::PoroMultiPhaseScaTra::FdCheck::fdcheck_none)
{
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::Init(
    const Teuchos::ParameterList& globaltimeparams, const Teuchos::ParameterList& algoparams,
    const Teuchos::ParameterList& poroparams, const Teuchos::ParameterList& structparams,
    const Teuchos::ParameterList& fluidparams, const Teuchos::ParameterList& scatraparams,
    const std::string& struct_disname, const std::string& fluid_disname,
    const std::string& scatra_disname, bool isale, int nds_disp, int nds_vel, int nds_solidpressure,
    int ndsporofluid_scatra, const std::map<int, std::set<int>>* nearbyelepairs)
{
  // call base class
  PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithic::Init(globaltimeparams, algoparams,
      poroparams, structparams, fluidparams, scatraparams, struct_disname, fluid_disname,
      scatra_disname, isale, nds_disp, nds_vel, nds_solidpressure, ndsporofluid_scatra,
      nearbyelepairs);

  // read input variables
  itmax_ = algoparams.get<int>("ITEMAX");
  ittolinc_ = algoparams.sublist("MONOLITHIC").get<double>("TOLINC_GLOBAL");
  ittolres_ = algoparams.sublist("MONOLITHIC").get<double>("TOLRES_GLOBAL");

  blockrowdofmap_ = Teuchos::rcp(new Core::LinAlg::MultiMapExtractor);

  fdcheck_ = Core::UTILS::IntegralValue<Inpar::PoroMultiPhaseScaTra::FdCheck>(
      algoparams.sublist("MONOLITHIC"), "FDCHECK");

  equilibration_method_ = Teuchos::getIntegralValue<Core::LinAlg::EquilibrationMethod>(
      algoparams.sublist("MONOLITHIC"), "EQUILIBRATION");

  solveradaptolbetter_ = algoparams.sublist("MONOLITHIC").get<double>("ADAPTCONV_BETTER");
  solveradapttol_ =
      (Core::UTILS::IntegralValue<int>(algoparams.sublist("MONOLITHIC"), "ADAPTCONV") == 1);

  // do we also solve the structure, this is helpful in case of fluid-scatra coupling without mesh
  // deformation
  solve_structure_ = Core::UTILS::IntegralValue<int>(poroparams, "SOLVE_STRUCTURE");
  if (!solve_structure_) struct_offset_ = 0;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::SetupSystem()
{
  // setup the poro subsystem first
  poro_field()->SetupSystem();

  // -------------------------------------------------------------create combined map
  setup_maps();

  //-----------------------------------build map of global dofs with DBC
  build_combined_dbc_map();
  // -------------------------------------------------------------

  // initialize Poroscatra-systemmatrix_
  systemmatrix_ =
      Teuchos::rcp(new Core::LinAlg::BlockSparseMatrix<Core::LinAlg::DefaultBlockMatrixStrategy>(
          *Extractor(), *Extractor(), 81, false, true));

  //! structure-scatra coupling matrix k_pss_ --> equal to zero so far
  //! fluid-scatra coupling matrix
  k_pfs_ = Teuchos::rcp(new Core::LinAlg::SparseMatrix(*(poro_field()->FluidDofRowMap()),
      //*(fluid_field()->dof_row_map()),
      81, true, true));

  //! scatra-structure coupling matrix
  k_sps_ = Teuchos::rcp(new Core::LinAlg::SparseMatrix(
      *(ScatraAlgo()->ScaTraField()->discretization()->dof_row_map()), 81, true, true));
  //! scatra-fluid coupling matrix
  k_spf_ = Teuchos::rcp(new Core::LinAlg::SparseMatrix(
      *(ScatraAlgo()->ScaTraField()->discretization()->dof_row_map()),
      //*(fluid_field()->dof_row_map()),
      81, true, true));

  // instantiate appropriate equilibration class
  auto equilibration_method =
      std::vector<Core::LinAlg::EquilibrationMethod>(1, equilibration_method_);
  equilibration_ = Core::LinAlg::BuildEquilibration(
      Core::LinAlg::MatrixType::block_field, equilibration_method, fullmap_);

  return;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::setup_maps()
{
  // create combined map
  std::vector<Teuchos::RCP<const Epetra_Map>> vecSpaces;

  if (solve_structure_)
  {
    vecSpaces.push_back(poro_field()->StructDofRowMap());
    vecSpaces.push_back(poro_field()->FluidDofRowMap());
    const Epetra_Map* dofrowmapscatra =
        (ScatraAlgo()->ScaTraField()->discretization())->dof_row_map(0);
    vecSpaces.push_back(Teuchos::rcp(dofrowmapscatra, false));

    if (vecSpaces[0]->NumGlobalElements() == 0) FOUR_C_THROW("No poro structure equation. Panic.");
    if (vecSpaces[1]->NumGlobalElements() == 0) FOUR_C_THROW("No poro fluid equation. Panic.");
    if (vecSpaces[2]->NumGlobalElements() == 0) FOUR_C_THROW("No scatra equation. Panic.");
  }
  else
  {
    vecSpaces.push_back(poro_field()->FluidDofRowMap());
    const Epetra_Map* dofrowmapscatra =
        (ScatraAlgo()->ScaTraField()->discretization())->dof_row_map(0);
    vecSpaces.push_back(Teuchos::rcp(dofrowmapscatra, false));

    if (vecSpaces[0]->NumGlobalElements() == 0) FOUR_C_THROW("No poro fluid equation. Panic.");
    if (vecSpaces[1]->NumGlobalElements() == 0) FOUR_C_THROW("No scatra equation. Panic.");
  }

  // full fluid-structure-scatra-map
  fullmap_ = Core::LinAlg::MultiMapExtractor::MergeMaps(vecSpaces);

  // full Poromultiphase-elasticity-blockmap
  blockrowdofmap_->Setup(*fullmap_, vecSpaces);

  // check global map extractor
  blockrowdofmap_->check_for_valid_map_extractor();

  return;
}

/*-----------------------------------------------------------------------/
/-----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::build_combined_dbc_map()
{
  // Combined DBC map of poromultielast-problem
  const Teuchos::RCP<const Epetra_Map> porocondmap = poro_field()->combined_dbc_map();
  const Teuchos::RCP<const Epetra_Map> scatracondmap =
      ScatraAlgo()->ScaTraField()->DirichMaps()->CondMap();
  combinedDBCMap_ = Core::LinAlg::MergeMap(porocondmap, scatracondmap, false);

  return;
}

/*-----------------------------------------------------------------------------------*
 *-----------------------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::build_block_null_spaces()
{
  // Build block null spaces of structure and fluid-field
  if (solve_structure_) poro_field()->build_block_null_spaces(solver_);
  // only fluid
  else
  {
    // equip smoother for fluid matrix block with empty parameter sublists to trigger null space
    // computation
    Teuchos::ParameterList& blocksmootherparams1 = solver_->Params().sublist("Inverse1");
    blocksmootherparams1.sublist("Belos Parameters");
    blocksmootherparams1.sublist("MueLu Parameters");

    poro_field()->fluid_field()->discretization()->compute_null_space_if_necessary(
        blocksmootherparams1);
  }

  // equip smoother for scatra matrix block with empty parameter sublists to trigger null space
  // computation
  Teuchos::ParameterList& blocksmootherparams =
      solver_->Params().sublist("Inverse" + std::to_string(struct_offset_ + 2));
  blocksmootherparams.sublist("Belos Parameters");
  blocksmootherparams.sublist("MueLu Parameters");

  ScatraAlgo()->ScaTraField()->discretization()->compute_null_space_if_necessary(
      blocksmootherparams);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::SetupSolver()
{
  //  solver
  // create a linear solver
  // get dynamic section of poroelasticity
  const Teuchos::ParameterList& poromultscatradyn =
      Global::Problem::Instance()->poro_multi_phase_scatra_dynamic_params();
  // get the solver number used for linear poroelasticity solver
  const int linsolvernumber = poromultscatradyn.sublist("MONOLITHIC").get<int>("LINEAR_SOLVER");
  // check if the poroelasticity solver has a valid solver number
  if (linsolvernumber == (-1))
    FOUR_C_THROW(
        "no linear solver defined for poromultiphaseflow with scatra coupling.\n"
        " Please set LINEAR_SOLVER in POROMULTIPHASESCATRA DYNAMIC to a valid number!");
  const Teuchos::ParameterList& solverparams =
      Global::Problem::Instance()->SolverParams(linsolvernumber);
  const auto solvertype =
      Teuchos::getIntegralValue<Core::LinearSolver::SolverType>(solverparams, "SOLVER");

  create_linear_solver(solverparams, solvertype);

  vectornormfres_ = Core::UTILS::IntegralValue<Inpar::PoroMultiPhaseScaTra::VectorNorm>(
      poromultscatradyn.sublist("MONOLITHIC"), "VECTORNORM_RESF");
  vectornorminc_ = Core::UTILS::IntegralValue<Inpar::PoroMultiPhaseScaTra::VectorNorm>(
      poromultscatradyn.sublist("MONOLITHIC"), "VECTORNORM_INC");
}

/*-----------------------------------------------------------------------------------*
 *-----------------------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::create_linear_solver(
    const Teuchos::ParameterList& solverparams, const Core::LinearSolver::SolverType solvertype)
{
  solver_ = Teuchos::rcp(new Core::LinAlg::Solver(solverparams, Comm()));
  // no need to do the rest for direct solvers
  if (solvertype == Core::LinearSolver::SolverType::umfpack or
      solvertype == Core::LinearSolver::SolverType::superlu)
    return;

  if (solvertype != Core::LinearSolver::SolverType::belos)
  {
    std::cout << "!!!!!!!!!!!!!!!!!!!!!! ATTENTION !!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::cout << " Note: the BGS2x2 preconditioner now " << std::endl;
    std::cout << " uses the structural solver and fluid solver blocks" << std::endl;
    std::cout << " for building the internal inverses" << std::endl;
    std::cout << " Remove the old BGS PRECONDITIONER BLOCK entries " << std::endl;
    std::cout << " in the dat files!" << std::endl;
    std::cout << "!!!!!!!!!!!!!!!!!!!!!! ATTENTION !!!!!!!!!!!!!!!!!!!!!" << std::endl;
    FOUR_C_THROW("Iterative solver expected");
  }
  const auto azprectype =
      Teuchos::getIntegralValue<Core::LinearSolver::PreconditionerType>(solverparams, "AZPREC");

  // plausibility check
  switch (azprectype)
  {
    case Core::LinearSolver::PreconditionerType::multigrid_nxn:
    {
      // no plausibility checks here
      // if you forget to declare an xml file you will get an error message anyway
    }
    break;
    default:
      FOUR_C_THROW("AMGnxn preconditioner expected");
      break;
  }

  // build the null spaces of the single blocks
  build_block_null_spaces();
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::TimeStep()
{
  // Prepare stuff
  setup_newton();
  print_header();

  // Evaluate
  evaluate(iterinc_);

  // Newton-Loop
  while ((not converged() and itnum_ < itmax_) or (itnum_ < itmin_))
  {
    // increment number of iteration
    itnum_++;

    // Solve
    linear_solve();
    solver_->ResetTolerance();

    // Build Convergence Norms
    build_convergence_norms();

    // Evaluate
    if (not converged())
    {
      evaluate(iterinc_);
      // perform FD Check of monolithic system matrix
      if (fdcheck_ == Inpar::PoroMultiPhaseScaTra::fdcheck_global)
        poro_multi_phase_sca_tra_fd_check();
    }
    else
    {
      // convergence check is based on residual(phi_i) < tol and phi_i+1 - phi_i < tol
      // in this function we update phi_i+1 as phi_i+1 = phi_i + iterinc for all fields
      // even though we have not evaluated the residual of phi_i+1 it will still be more exact than
      // the one at phi_i
      update_fields_after_convergence();
    }

    // print output
    newton_output();
  }

  // Error-Check
  newton_error_check();

  return;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::evaluate(
    Teuchos::RCP<const Epetra_Vector> iterinc)
{
  TEUCHOS_FUNC_TIME_MONITOR("PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::Evaluate");

  // reset timer
  timernewton_.reset();
  // *********** time measurement ***********
  double dtcpu = timernewton_.wallTime();
  // *********** time measurement ***********

  // displacement, fluid variable and scatra variable incremental vector
  Teuchos::RCP<const Epetra_Vector> porostructinc;
  Teuchos::RCP<const Epetra_Vector> porofluidinc;
  Teuchos::RCP<const Epetra_Vector> scatrainc;
  extract_field_vectors(iterinc, porostructinc, porofluidinc, scatrainc);

  // (1) Newton update of the scatra field
  update_scatra(scatrainc);

  // (2) set scatra solution on fluid field
  SetScatraSolution();

  // (3) access poro problem to build poro-poro block
  poro_field()->Evaluate(porostructinc, porofluidinc, itnum_ == 0);

  // (4) set fluid and structure solution on scatra field
  SetPoroSolution();

  // (5) access ScaTra problem to build scatra-scatra block
  evaluate_scatra();

  // (6) Build the monolithic system matrix
  setup_system_matrix();

  // check whether we have a sanely filled tangent matrix
  if (not systemmatrix_->Filled())
  {
    FOUR_C_THROW("Effective tangent matrix must be filled here");
  }

  // (7) Build the monolithic system vector
  setup_rhs();

  // *********** time measurement ***********
  double mydtele = timernewton_.wallTime() - dtcpu;
  Comm().MaxAll(&mydtele, &dtele_, 1);
  // *********** time measurement ***********
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::setup_system_matrix()
{
  // set loma block matrix to zero
  systemmatrix_->Zero();

  //----------------------------------------------------------------------
  // 1st diagonal block (upper left): poro weighting - poro solution
  // has dimensions ((ndim+n_phases)*n_nodes)x((ndim+n_phases)*n_nodes)
  //----------------------------------------------------------------------
  // get matrix block
  Teuchos::RCP<Core::LinAlg::BlockSparseMatrixBase> mat_pp = poro_field()->BlockSystemMatrix();

  // uncomplete matrix block (appears to be required in certain cases (locsys+iterative solver))
  mat_pp->UnComplete();

  // assign matrix block
  if (solve_structure_)
  {
    systemmatrix_->Assign(0, 0, Core::LinAlg::View, mat_pp->Matrix(0, 0));
    systemmatrix_->Assign(0, 1, Core::LinAlg::View, mat_pp->Matrix(0, 1));
    systemmatrix_->Assign(1, 0, Core::LinAlg::View, mat_pp->Matrix(1, 0));
  }
  systemmatrix_->Assign(struct_offset_, struct_offset_, Core::LinAlg::View, mat_pp->Matrix(1, 1));

  //----------------------------------------------------------------------
  // 2nd diagonal block (lower right): scatra weighting - scatra solution
  // has dimensions (n_species*n_nodes)x(n_species*n_nodes)
  //----------------------------------------------------------------------
  // get matrix block
  Teuchos::RCP<Core::LinAlg::SparseMatrix> mat_ss = ScatraAlgo()->ScaTraField()->SystemMatrix();

  // uncomplete matrix block (appears to be required in certain cases)
  mat_ss->UnComplete();

  // assign matrix block
  systemmatrix_->Assign(struct_offset_ + 1, struct_offset_ + 1, Core::LinAlg::View, *mat_ss);

  // complete scatra block matrix
  systemmatrix_->Complete();

  //----------------------------------------------------------------------
  // 1st off-diagonal block k_ps (upper right): poro weighting - scatra solution
  // has dimensions ((ndim+n_phases)*n_nodes)x(n_species*n_nodes)
  // so far no coupling of structure with scatra --> k_pss_ = 0
  // --> dimensions (n_phases*n_nodes)x(n_species*n_nodes)
  //----------------------------------------------------------------------

  // create empty matrix
  Teuchos::RCP<Core::LinAlg::SparseMatrix> k_pfs = poro_fluid_scatra_coupling_matrix();

  // call the porofluid-elements and calculate the off-diagonal scatra matrix block
  apply_poro_fluid_scatra_coupl_matrix(k_pfs);

  // apply DBC's also on off-diagonal fluid-scatra coupling block (main-diagonal blocks have already
  // been set, either in poromultielast_monolithic.cpp or in the respective evalute calls)
  k_pfs->ApplyDirichlet(*poro_field()->fluid_field()->GetDBCMapExtractor()->CondMap(), false);

  // uncomplete matrix block (appears to be required in certain cases)
  // k_pss_->UnComplete();
  k_pfs->UnComplete();

  // assign matrix block
  // systemmatrix_->Assign(0,2,Core::LinAlg::View,*(k_pss_)); --> zero
  systemmatrix_->Assign(struct_offset_, struct_offset_ + 1, Core::LinAlg::View, *(k_pfs));

  //----------------------------------------------------------------------
  // 2nd off-diagonal block k_sp (lower left): scatra weighting - poro solution
  // has dimensions (n_species*n_nodes)x((ndim+n_phases)*n_nodes)
  //----------------------------------------------------------------------

  // create empty matrix
  Teuchos::RCP<Core::LinAlg::SparseMatrix> k_sps = scatra_struct_coupling_matrix();

  // call the scatra-elements and calculate the off-diagonal structure matrix block
  apply_scatra_struct_coupl_matrix(k_sps);

  // apply DBC's also on off-diagonal scatra-structure coupling block (main-diagonal blocks have
  // already been set, either in poromultielast_monolithic.cpp or in the respective evalute calls)
  k_sps->ApplyDirichlet(*ScatraAlgo()->ScaTraField()->DirichMaps()->CondMap(), false);

  // create empty matrix
  Teuchos::RCP<Core::LinAlg::SparseMatrix> k_spf = scatra_poro_fluid_coupling_matrix();

  // call the scatra-elements and calculate the off-diagonal structure matrix block
  apply_scatra_poro_fluid_coupl_matrix(k_spf);

  // apply DBC's also on off-diagonal scatra-fluid coupling block (main-diagonal blocks have already
  // been set, either in poromultielast_monolithic.cpp or in the respective evalute calls)
  k_spf->ApplyDirichlet(*ScatraAlgo()->ScaTraField()->DirichMaps()->CondMap(), false);

  // uncomplete matrix block (appears to be required in certain cases (locsys+iterative solver))
  k_sps->UnComplete();
  k_spf->UnComplete();

  // assign matrix block
  if (solve_structure_) systemmatrix_->Assign(2, 0, Core::LinAlg::View, *(k_sps));
  systemmatrix_->Assign(struct_offset_ + 1, struct_offset_, Core::LinAlg::View, *(k_spf));

  // complete block matrix
  systemmatrix_->Complete();
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::RCP<Core::LinAlg::SparseMatrix>
PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::poro_fluid_scatra_coupling_matrix()
{
  Teuchos::RCP<Core::LinAlg::SparseMatrix> sparse =
      Teuchos::rcp_dynamic_cast<Core::LinAlg::SparseMatrix>(k_pfs_);
  if (sparse == Teuchos::null) FOUR_C_THROW("cast to Core::LinAlg::SparseMatrix failed!");

  return sparse;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::RCP<Core::LinAlg::SparseMatrix>
PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::scatra_struct_coupling_matrix()
{
  Teuchos::RCP<Core::LinAlg::SparseMatrix> sparse =
      Teuchos::rcp_dynamic_cast<Core::LinAlg::SparseMatrix>(k_sps_);
  if (sparse == Teuchos::null) FOUR_C_THROW("cast to Core::LinAlg::SparseMatrix failed!");

  return sparse;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::RCP<Core::LinAlg::SparseMatrix>
PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::scatra_poro_fluid_coupling_matrix()
{
  Teuchos::RCP<Core::LinAlg::SparseMatrix> sparse =
      Teuchos::rcp_dynamic_cast<Core::LinAlg::SparseMatrix>(k_spf_);
  if (sparse == Teuchos::null) FOUR_C_THROW("cast to Core::LinAlg::SparseMatrix failed!");

  return sparse;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::evaluate_scatra()
{
  ScatraAlgo()->ScaTraField()->PrepareLinearSolve();
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::
    apply_poro_fluid_scatra_coupl_matrix(
        Teuchos::RCP<Core::LinAlg::SparseOperator> k_pfs  //!< off-diagonal tangent matrix term
    )
{
  // reset
  k_pfs->Zero();
  // evaluate
  poro_field()->fluid_field()->assemble_fluid_scatra_coupling_mat(k_pfs);
  // complete
  k_pfs->Complete(ScatraAlgo()->ScaTraField()->SystemMatrix()->RangeMap(),
      poro_field()->fluid_field()->SystemMatrix()->RangeMap());
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::apply_scatra_struct_coupl_matrix(
    Teuchos::RCP<Core::LinAlg::SparseOperator> k_sps  //!< off-diagonal tangent matrix term
)
{
  // create the parameters for the discretization
  Teuchos::ParameterList sparams_struct;

  k_sps->Zero();

  if (solve_structure_)
  {
    Core::UTILS::AddEnumClassToParameterList<ScaTra::Action>(
        "action", ScaTra::Action::calc_scatra_mono_odblock_mesh, sparams_struct);
    // other parameters that might be needed by the elements
    sparams_struct.set("delta time", Dt());
    sparams_struct.set("total time", Time());

    // we cannot employ L2-projection for monolithic coupling yet
    sparams_struct.set<bool>("L2-projection", false);

    ScatraAlgo()->ScaTraField()->discretization()->ClearState();
    ScatraAlgo()->ScaTraField()->discretization()->set_state(
        0, "hist", ScatraAlgo()->ScaTraField()->Hist());
    ScatraAlgo()->ScaTraField()->discretization()->set_state(
        0, "phinp", ScatraAlgo()->ScaTraField()->Phinp());

    // build specific assemble strategy for mechanical-fluid system matrix
    // from the point of view of structure_field:
    // structdofset = 0, fluiddofset = 1
    Core::FE::AssembleStrategy scatrastrategy_struct(0,  // scatradofset for row
        1,                                               // structuredofset for column
        k_sps,                                           // scatra-structure coupling matrix
        Teuchos::null, Teuchos::null, Teuchos::null, Teuchos::null);

    ScatraAlgo()->ScaTraField()->discretization()->Evaluate(sparams_struct, scatrastrategy_struct);
  }

  // complete
  k_sps->Complete(poro_field()->structure_field()->SystemMatrix()->RangeMap(),
      ScatraAlgo()->ScaTraField()->SystemMatrix()->RangeMap());

  ScatraAlgo()->ScaTraField()->discretization()->ClearState();
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::
    apply_scatra_poro_fluid_coupl_matrix(
        Teuchos::RCP<Core::LinAlg::SparseOperator> k_spf  //!< off-diagonal tangent matrix term
    )
{
  // create the parameters for the discretization
  Teuchos::ParameterList sparams_fluid;

  k_spf->Zero();

  Core::UTILS::AddEnumClassToParameterList<ScaTra::Action>(
      "action", ScaTra::Action::calc_scatra_mono_odblock_fluid, sparams_fluid);
  // other parameters that might be needed by the elements
  sparams_fluid.set("delta time", Dt());
  sparams_fluid.set("total time", Time());

  // we cannot employ L2-projection for monolithic coupling yet
  sparams_fluid.set<bool>("L2-projection", false);

  ScatraAlgo()->ScaTraField()->discretization()->ClearState();
  ScatraAlgo()->ScaTraField()->discretization()->set_state(
      0, "hist", ScatraAlgo()->ScaTraField()->Hist());
  ScatraAlgo()->ScaTraField()->discretization()->set_state(
      0, "phinp", ScatraAlgo()->ScaTraField()->Phinp());


  // build specific assemble strategy for mechanical-fluid system matrix
  // from the point of view of structure_field:
  // structdofset = 0, fluiddofset = 1
  Core::FE::AssembleStrategy scatrastrategy_fluid(0,  // scatradofset for row
      2,                                              // fluiddofset for column
      k_spf,                                          // scatra-structure coupling matrix
      Teuchos::null, Teuchos::null, Teuchos::null, Teuchos::null);

  ScatraAlgo()->ScaTraField()->discretization()->Evaluate(sparams_fluid, scatrastrategy_fluid);

  // complete
  k_spf->Complete(poro_field()->fluid_field()->SystemMatrix()->RangeMap(),
      ScatraAlgo()->ScaTraField()->SystemMatrix()->RangeMap());

  ScatraAlgo()->ScaTraField()->discretization()->ClearState();
}

/*-----------------------------------------------------------------------------*
 *-----------------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::update_fields_after_convergence()
{
  // displacement, fluid variable and scatra variable incremental vector
  Teuchos::RCP<const Epetra_Vector> porostructinc;
  Teuchos::RCP<const Epetra_Vector> porofluidinc;
  Teuchos::RCP<const Epetra_Vector> scatrainc;
  extract_field_vectors(iterinc_, porostructinc, porofluidinc, scatrainc);

  // update ScaTra field
  update_scatra(scatrainc);

  // update structure and fluid field
  poro_field()->update_fields_after_convergence(porostructinc, porofluidinc);
}
/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::update_scatra(
    Teuchos::RCP<const Epetra_Vector> scatrainc)
{
  ScatraAlgo()->ScaTraField()->UpdateIter(scatrainc);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::setup_rhs()
{
  // create full monolithic rhs vector
  if (rhs_ == Teuchos::null) rhs_ = Teuchos::rcp(new Epetra_Vector(*dof_row_map(), true));

  // note: rhs of fluid-structure system already setup in evaluate call

  // fill the Poroelasticity rhs vector rhs_ with the single field rhss
  setup_vector(*rhs_, poro_field()->RHS(), ScatraAlgo()->ScaTraField()->Residual());
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::setup_vector(
    Epetra_Vector& f, Teuchos::RCP<const Epetra_Vector> pv, Teuchos::RCP<const Epetra_Vector> sv)
{
  // extract dofs of the two fields
  // and put the poro/scatra field vector into the global vector f
  // noticing the block number

  //  Teuchos::RCP<const Epetra_Vector> psx;
  //  Teuchos::RCP<const Epetra_Vector> pfx;

  if (solve_structure_)
    Extractor()->InsertVector(*(poro_field()->Extractor()->ExtractVector(pv, 0)), 0, f);
  Extractor()->InsertVector(*(poro_field()->Extractor()->ExtractVector(pv, 1)), struct_offset_, f);
  Extractor()->InsertVector(*sv, struct_offset_ + 1, f);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::extract_field_vectors(
    Teuchos::RCP<const Epetra_Vector> x, Teuchos::RCP<const Epetra_Vector>& stx,
    Teuchos::RCP<const Epetra_Vector>& flx, Teuchos::RCP<const Epetra_Vector>& scx)
{
  TEUCHOS_FUNC_TIME_MONITOR(
      "PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::extract_field_vectors");

  // process structure unknowns of the first field
  if (solve_structure_)
    stx = Extractor()->ExtractVector(x, 0);
  else
    stx = Teuchos::rcp(new Epetra_Vector(*poro_field()->StructDofRowMap(), true));

  // process fluid unknowns of the second field
  flx = Extractor()->ExtractVector(x, struct_offset_);

  // process scatra unknowns of the third field
  scx = Extractor()->ExtractVector(x, struct_offset_ + 1);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::extract3_d_field_vectors(
    Teuchos::RCP<const Epetra_Vector> x, Teuchos::RCP<const Epetra_Vector>& stx,
    Teuchos::RCP<const Epetra_Vector>& flx, Teuchos::RCP<const Epetra_Vector>& scx)
{
  PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::extract_field_vectors(
      x, stx, flx, scx);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::linear_solve()
{
  // reset timer
  timernewton_.reset();
  // *********** time measurement ***********
  double dtcpu = timernewton_.wallTime();
  // *********** time measurement ***********
  Core::LinAlg::SolverParams solver_params;
  if (solveradapttol_ and (itnum_ > 1))
  {
    solver_params.nonlin_tolerance = ittolres_;
    solver_params.nonlin_residual = std::max(maxinc_, maxres_);
    solver_params.lin_tol_better = solveradaptolbetter_;
  }
  iterinc_->PutScalar(0.0);  // Useful? depends on solver and more

  // equilibrate global system of equations if necessary
  equilibration_->EquilibrateSystem(systemmatrix_, rhs_, blockrowdofmap_);

  // standard solver call
  // system is ready to solve since Dirichlet Boundary conditions have been applied in
  // setup_system_matrix or Evaluate
  solver_params.refactor = true;
  solver_params.reset = itnum_ == 1;
  solver_->Solve(systemmatrix_->EpetraOperator(), iterinc_, rhs_, solver_params);

  equilibration_->unequilibrate_increment(iterinc_);

  // *********** time measurement ***********
  double mydtsolve = timernewton_.wallTime() - dtcpu;
  Comm().MaxAll(&mydtsolve, &dtsolve_, 1);
  // *********** time measurement ***********
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
bool PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::converged()
{
  return (normincfluid_ < ittolinc_ && normincstruct_ < ittolinc_ && normincscatra_ < ittolinc_ &&
          normincart_ < ittolinc_ && normincartsca_ < ittolinc_ && normrhs_ < ittolres_ &&
          normrhsfluid_ < ittolres_ && normrhsstruct_ < ittolres_ && normrhsscatra_ < ittolres_ &&
          normrhsart_ < ittolres_ && normrhsartsca_ < ittolres_);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::build_convergence_norms()
{
  //------------------------------------------------------------ build residual force norms
  normrhs_ = UTILS::calculate_vector_norm(vectornormfres_, rhs_);
  Teuchos::RCP<const Epetra_Vector> rhs_st;
  Teuchos::RCP<const Epetra_Vector> rhs_fl;
  Teuchos::RCP<const Epetra_Vector> rhs_sc;

  // get structure and fluid RHS
  extract3_d_field_vectors(rhs_, rhs_st, rhs_fl, rhs_sc);

  // build also norms for structure, fluid and scatra
  normrhsstruct_ = UTILS::calculate_vector_norm(vectornormfres_, rhs_st);
  normrhsfluid_ = UTILS::calculate_vector_norm(vectornormfres_, rhs_fl);
  normrhsscatra_ = UTILS::calculate_vector_norm(vectornormfres_, rhs_sc);

  //------------------------------------------------------------- build residual increment norms
  // displacement and fluid velocity & pressure incremental vector
  Teuchos::RCP<const Epetra_Vector> iterincst;
  Teuchos::RCP<const Epetra_Vector> iterincfl;
  Teuchos::RCP<const Epetra_Vector> iterincsc;

  // get structure and fluid increment
  extract3_d_field_vectors(iterinc_, iterincst, iterincfl, iterincsc);

  // build also norms for fluid and structure
  normincstruct_ = UTILS::calculate_vector_norm(vectornorminc_, iterincst);
  normincfluid_ = UTILS::calculate_vector_norm(vectornorminc_, iterincfl);
  normincscatra_ = UTILS::calculate_vector_norm(vectornorminc_, iterincsc);

  double dispnorm =
      UTILS::calculate_vector_norm(vectornorminc_, poro_field()->structure_field()->Dispnp());
  double fluidnorm =
      UTILS::calculate_vector_norm(vectornorminc_, poro_field()->fluid_field()->Phinp());
  double scatranorm =
      UTILS::calculate_vector_norm(vectornorminc_, ScatraAlgo()->ScaTraField()->Phinp());

  // take care of very small norms
  if (dispnorm < 1.0e-6) dispnorm = 1.0;
  if (fluidnorm < 1.0e-6) fluidnorm = 1.0;
  if (scatranorm < 1.0e-6) scatranorm = 1.0;
  if (arterypressnorm_ < 1.0e-6) arterypressnorm_ = 1.0;
  if (arteryscanorm_ < 1.0e-6) arteryscanorm_ = 1.0;

  // build relative increment norm
  normincstruct_ /= dispnorm;
  normincfluid_ /= fluidnorm;
  normincscatra_ /= scatranorm;
  normincart_ /= arterypressnorm_;
  normincartsca_ /= arteryscanorm_;

  // build the maximum value of the residuals and increments
  maxinc_ = std::max({normincfluid_, normincstruct_, normincscatra_, normincart_, normincartsca_});
  maxres_ = std::max(
      {normrhs_, normrhsfluid_, normrhsstruct_, normrhsscatra_, normrhsart_, normrhsartsca_});
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::setup_newton()
{
  // initialise equilibrium loop and norms
  itnum_ = 0;
  normrhs_ = 0.0;
  normrhsfluid_ = 0.0;
  normincfluid_ = 0.0;
  normrhsstruct_ = 0.0;
  normincstruct_ = 0.0;
  normrhsscatra_ = 0.0;
  normincscatra_ = 0.0;
  tolinc_ = 0.0;
  tolfres_ = 0.0;
  tolinc_struct_ = 0.0;
  tolfres_struct_ = 0.0;
  tolinc_fluid_ = 0.0;
  tolfres_fluid_ = 0.0;
  tolinc_scatra_ = 0.0;
  tolfres_scatra_ = 0.0;
  normrhsart_ = 0.0;
  normincart_ = 0.0;
  arterypressnorm_ = 0.0;
  normrhsartsca_ = 0.0;
  normincartsca_ = 0.0;
  arteryscanorm_ = 0.0;
  maxinc_ = 0.0;
  maxres_ = 0.0;

  // incremental solution vector with length of all dofs
  if (iterinc_ == Teuchos::null)
    iterinc_ = Core::LinAlg::CreateVector(*dof_row_map(), true);
  else
    iterinc_->PutScalar(0.0);

  // a zero vector of full length
  if (zeros_ == Teuchos::null)
    zeros_ = Core::LinAlg::CreateVector(*dof_row_map(), true);
  else
    zeros_->PutScalar(0.0);

  // AitkenReset();
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::newton_output()
{
  // print the incremental based convergence check to the screen
  if (Comm().MyPID() == 0)
  {
    if (itnum_ == 1)
      printf(
          "+--------------+-------------+-------------+--------------+------------+-----"
          "-------+-----------------+\n");
    printf(
        "|-  step/max  -|- fluid-inc -|- displ-inc -|- scatra-inc -|-  1Dp-inc -|- "
        " 1Ds-inc -|- norm(tot-rhs) -| (ts =%10.3E,",
        dtsolve_);
    printf("\n");
    printf(
        "|   %3d/%3d    | %10.3E  | %10.3E  |  %10.3E  | %10.3E | %10.3E |   %10.3E    |  "
        "te =%10.3E)",
        itnum_, itmax_, normincfluid_, normincstruct_, normincscatra_, normincart_, normincartsca_,
        normrhs_, dtele_);
    printf("\n");
    printf(
        "+--------------+-------------+-------------+--------------+------------+-----"
        "-------+-----------------+\n");
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::newton_error_check()
{
  // print the incremental based convergence check to the screen
  if (converged())  // norminc_ < ittolinc_ && normrhs_ < ittolinc_ && normincfluid_ < ittolinc_ &&
                    // normincstruct_ < ittolinc_
  {
    if (Comm().MyPID() == 0)
    {
      printf(
          "|  Monolithic iteration loop converged after iteration %3d/%3d !                        "
          "              |\n",
          itnum_, itmax_);
      printf(
          "|  Quantity           [norm]:                 TOL                                       "
          "              |\n");
      printf(
          "|  Max. rel. increment [%3s]:  %10.3E  < %10.3E                                        "
          "       |\n",
          VectorNormString(vectornorminc_).c_str(), maxinc_, ittolinc_);
      printf(
          "|  Maximum    residual [%3s]:  %10.3E  < %10.3E                                        "
          "       |\n",
          VectorNormString(vectornormfres_).c_str(), maxres_, ittolres_);
      printf(
          "+--------------+-------------+-------------+--------------+------------+-----"
          "-------+-----------------+\n");
      printf("\n");
    }
  }
  else
  {
    if ((Comm().MyPID() == 0))
    {
      printf(
          "|     >>>>>> not converged in %3d steps!                                                "
          "       |\n",
          itmax_);
      printf(
          "|  Quantity           [norm]:                 TOL                                       "
          "       |\n");
      printf(
          "|  Max. rel. increment [%3s]:  %10.3E    %10.3E                                        "
          "|\n",
          VectorNormString(vectornorminc_).c_str(), maxinc_, ittolinc_);
      printf(
          "|  Maximum    residual [%3s]:  %10.3E    %10.3E                                        "
          "|\n",
          VectorNormString(vectornormfres_).c_str(), maxres_, ittolres_);
      printf(
          "+--------------+-------------+-------------+--------------+------------+-----"
          "-------+-----------------+\n");
      printf("\n");
      printf("\n");
    }
    HandleDivergence();
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::RCP<const Epetra_Map>
PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::dof_row_map()
{
  return blockrowdofmap_->FullMap();
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::print_header()
{
  if (!solve_structure_) print_structure_disabled_info();
  if (Comm().MyPID() == 0)
  {
    std::cout << "+--------------------------------------------------------------------------------"
                 "---------------------+"
              << std::endl;
    std::cout << "| MONOLITHIC POROMULTIPHASE-SCATRA SOLVER                                        "
                 "                     |"
              << std::endl;
    std::cout << "| STEP: " << std::setw(5) << std::setprecision(4) << std::scientific << Step()
              << "/" << std::setw(5) << std::setprecision(4) << std::scientific << n_step()
              << ", Time: " << std::setw(11) << std::setprecision(4) << std::scientific << Time()
              << "/" << std::setw(11) << std::setprecision(4) << std::scientific << max_time()
              << ", Dt: " << std::setw(11) << std::setprecision(4) << std::scientific << Dt()
              << "                                   |" << std::endl;
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::print_structure_disabled_info()
{
  // print out Info
  if (Comm().MyPID() == 0)
  {
    std::cout << "\n";
    std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
                 "++++++++++++++++++++++++++++++++\n";
    std::cout << " INFO:    STRUCTURE FIELD IS NOT SOLVED; MAKE SURE YOU HAVE CONSTRAINED ALL DOFS "
                 "IN YOUR STRUCTURE WITH A DBC\n";
    std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
                 "++++++++++++++++++++++++++++++++\n";
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::poro_multi_phase_sca_tra_fd_check()
{
  std::cout << "\n******************finite difference check***************" << std::endl;

  int dof_struct = (poro_field()->structure_field()->dof_row_map()->NumGlobalElements());
  int dof_fluid = (poro_field()->fluid_field()->dof_row_map()->NumGlobalElements());
  int dof_scatra = (ScatraAlgo()->ScaTraField()->dof_row_map()->NumGlobalElements());

  std::cout << "structure field has " << dof_struct << " DOFs" << std::endl;
  std::cout << "fluid field has " << dof_fluid << " DOFs" << std::endl;
  std::cout << "scatra field has " << dof_scatra << " DOFs" << std::endl;
  if (artery_coupl_)
  {
    int dof_artery = (poro_field()->fluid_field()->ArteryDofRowMap()->NumGlobalElements());
    int dof_artscatra = (scatramsht_->ArtScatraField()->dof_row_map()->NumGlobalElements());
    std::cout << "artery field has " << dof_artery << " DOFs" << std::endl;
    std::cout << "artery-scatra field has " << dof_artscatra << " DOFs" << std::endl;

    std::cout << "\n\n============================================================\n"
                 "WARNING: THIS FD CHECK DOES NOT WORK FOR NODE BASED COUPLING\n"
                 "============================================================\n\n";
  }

  Teuchos::RCP<Epetra_Vector> iterinc = Teuchos::null;
  iterinc = Core::LinAlg::CreateVector(*dof_row_map(), true);

  const int dofs = iterinc->GlobalLength();
  std::cout << "in total " << dofs << " DOFs" << std::endl;
  const double delta = 1e-8;

  iterinc->PutScalar(0.0);

  iterinc->ReplaceGlobalValue(0, 0, delta);

  Teuchos::RCP<Epetra_CrsMatrix> stiff_approx = Teuchos::null;
  stiff_approx = Core::LinAlg::CreateMatrix(*dof_row_map(), 81);

  Teuchos::RCP<Epetra_Vector> rhs_old = Teuchos::rcp(new Epetra_Vector(*dof_row_map(), true));
  rhs_old->Update(1.0, *rhs_, 0.0);
  Teuchos::RCP<Epetra_Vector> rhs_copy = Teuchos::rcp(new Epetra_Vector(*dof_row_map(), true));

  Teuchos::RCP<Core::LinAlg::SparseMatrix> sparse = systemmatrix_->Merge();
  Teuchos::RCP<Core::LinAlg::SparseMatrix> sparse_copy =
      Teuchos::rcp(new Core::LinAlg::SparseMatrix(sparse->EpetraMatrix(), Core::LinAlg::Copy));

  if (false)
  {
    std::cout << "iterinc_" << std::endl << *iterinc_ << std::endl;
    std::cout << "iterinc" << std::endl << *iterinc << std::endl;
    // std::cout << "meshdisp: " << std::endl << *(poro_field()->fluid_field()-> ->Dispnp());
    std::cout << "disp: " << std::endl << *(poro_field()->structure_field()->Dispnp());
    // std::cout << "fluid vel" << std::endl << *(poro_field()->fluid_field()->Velnp());
    // std::cout << "fluid acc" << std::endl << *(poro_field()->fluid_field()->Accnp());
    // std::cout << "gridvel fluid" << std::endl << *(poro_field()->fluid_field()->GridVel());
    std::cout << "gridvel struct" << std::endl << *(poro_field()->structure_field()->Velnp());
  }

  const int zeilennr = -1;
  const int spaltenr = -1;
  for (int i = 0; i < dofs; ++i)
  {
    if (combined_dbc_map()->MyGID(i))
    {
      iterinc->ReplaceGlobalValue(i, 0, 0.0);
    }

    if (i == spaltenr)
      std::cout << "\n******************" << spaltenr + 1 << ". Spalte!!***************"
                << std::endl;

    evaluate(iterinc);
    setup_rhs();

    rhs_copy->Update(1.0, *rhs_, 0.0);

    iterinc_->PutScalar(0.0);  // Useful? depends on solver and more
    Core::LinAlg::apply_dirichlet_to_system(
        *sparse_copy, *iterinc_, *rhs_copy, *zeros_, *combined_dbc_map());


    if (i == spaltenr)
    {
      std::cout << "rhs_: " << (*rhs_copy)[zeilennr] << std::endl;
      std::cout << "rhs_old: " << (*rhs_old)[zeilennr] << std::endl;
    }

    rhs_copy->Update(-1.0, *rhs_old, 1.0);
    rhs_copy->Scale(-1.0 / delta);

    int* index = &i;
    for (int j = 0; j < dofs; ++j)
    {
      double value = (*rhs_copy)[j];
      stiff_approx->InsertGlobalValues(j, 1, &value, index);

      if ((j == zeilennr) and (i == spaltenr))
      {
        std::cout << "\n******************" << zeilennr + 1 << ". Zeile!!***************"
                  << std::endl;
        std::cout << "iterinc_" << std::endl << *iterinc_ << std::endl;
        std::cout << "iterinc" << std::endl << *iterinc << std::endl;
        // std::cout << "meshdisp: " << std::endl << *(poro_field()->fluid_field()->Dispnp());
        // std::cout << "meshdisp scatra: " << std::endl <<
        // *(ScaTraField()->discretization()->GetState(ScaTraField()->NdsDisp(),"dispnp"));
        std::cout << "disp: " << std::endl << *(poro_field()->structure_field()->Dispnp());
        // std::cout << "fluid vel" << std::endl << *(poro_field()->fluid_field()->Velnp());
        // std::cout << "scatra vel" << std::endl <<
        // *(ScaTraField()->discretization()->GetState(ScaTraField()->NdsVel(),"velocity field"));
        // std::cout << "fluid acc" << std::endl << *(poro_field()->fluid_field()->Accnp());
        // std::cout << "gridvel fluid" << std::endl << *(poro_field()->fluid_field()->GridVel());
        std::cout << "gridvel struct" << std::endl << *(poro_field()->structure_field()->Velnp());

        std::cout << "stiff_apprx(" << zeilennr << "," << spaltenr << "): " << (*rhs_copy)[zeilennr]
                  << std::endl;

        std::cout << "value(" << zeilennr << "," << spaltenr << "): " << value << std::endl;
        std::cout << "\n******************" << zeilennr + 1 << ". Zeile Ende!!***************"
                  << std::endl;
      }
    }

    if (not combined_dbc_map()->MyGID(i)) iterinc->ReplaceGlobalValue(i, 0, -delta);

    iterinc->ReplaceGlobalValue(i - 1, 0, 0.0);

    if (i != dofs - 1) iterinc->ReplaceGlobalValue(i + 1, 0, delta);

    if (i == spaltenr)
      std::cout << "\n******************" << spaltenr + 1 << ". Spalte Ende!!***************"
                << std::endl;
  }

  evaluate(iterinc);
  setup_rhs();

  stiff_approx->FillComplete();

  Teuchos::RCP<Core::LinAlg::SparseMatrix> stiff_approx_sparse = Teuchos::null;
  stiff_approx_sparse =
      Teuchos::rcp(new Core::LinAlg::SparseMatrix(stiff_approx, Core::LinAlg::Copy));

  stiff_approx_sparse->Add(*sparse_copy, false, -1.0, 1.0);

  Teuchos::RCP<Epetra_CrsMatrix> sparse_crs = sparse_copy->EpetraMatrix();

  Teuchos::RCP<Epetra_CrsMatrix> error_crs = stiff_approx_sparse->EpetraMatrix();

  error_crs->FillComplete();
  sparse_crs->FillComplete();

  bool success = true;
  double error_max_rel = 0.0;
  double error_max_abs = 0.0;
  for (int i = 0; i < dofs; ++i)
  {
    if (not combined_dbc_map()->MyGID(i))
    {
      for (int j = 0; j < dofs; ++j)
      {
        if (not combined_dbc_map()->MyGID(j))
        {
          double stiff_approx_ij = 0.0;
          double sparse_ij = 0.0;
          double error_ij = 0.0;

          {
            // get error_crs entry ij
            int errornumentries;
            int errorlength = error_crs->NumGlobalEntries(i);
            std::vector<double> errorvalues(errorlength);
            std::vector<int> errorindices(errorlength);
            // int errorextractionstatus =
            error_crs->ExtractGlobalRowCopy(
                i, errorlength, errornumentries, errorvalues.data(), errorindices.data());
            for (int k = 0; k < errorlength; ++k)
            {
              if (errorindices[k] == j)
              {
                error_ij = errorvalues[k];
                break;
              }
              else
                error_ij = 0.0;
            }
          }

          // get sparse_ij entry ij
          {
            int sparsenumentries;
            int sparselength = sparse_crs->NumGlobalEntries(i);
            std::vector<double> sparsevalues(sparselength);
            std::vector<int> sparseindices(sparselength);
            // int sparseextractionstatus =
            sparse_crs->ExtractGlobalRowCopy(
                i, sparselength, sparsenumentries, sparsevalues.data(), sparseindices.data());
            for (int k = 0; k < sparselength; ++k)
            {
              if (sparseindices[k] == j)
              {
                sparse_ij = sparsevalues[k];
                break;
              }
              else
                sparse_ij = 0.0;
            }
          }

          // get stiff_approx entry ij
          {
            int approxnumentries;
            int approxlength = stiff_approx->NumGlobalEntries(i);
            std::vector<double> approxvalues(approxlength);
            std::vector<int> approxindices(approxlength);
            // int approxextractionstatus =
            stiff_approx->ExtractGlobalRowCopy(
                i, approxlength, approxnumentries, approxvalues.data(), approxindices.data());
            for (int k = 0; k < approxlength; ++k)
            {
              if (approxindices[k] == j)
              {
                stiff_approx_ij = approxvalues[k];
                break;
              }
              else
                stiff_approx_ij = 0.0;
            }
          }

          double error = 0.0;
          if (abs(stiff_approx_ij) > 1e-5)
            error = error_ij / (stiff_approx_ij);
          else if (abs(sparse_ij) > 1e-5)
            error = error_ij / (sparse_ij);

          if (abs(error) > abs(error_max_rel)) error_max_rel = abs(error);
          if (abs(error_ij) > abs(error_max_abs)) error_max_abs = abs(error_ij);

          if ((abs(error) > 1e-4))
          {
            if ((abs(error_ij) > 1e-5))
            //  if( (sparse_ij>1e-1) or (stiff_approx_ij>1e-1) )
            {
              std::cout << "finite difference check failed entry (" << i << "," << j
                        << ")! stiff: " << sparse_ij << ", approx: " << stiff_approx_ij
                        << " ,abs. error: " << error_ij << " , rel. error: " << error << std::endl;

              success = false;
            }
          }
        }
      }
    }
  }

  if (success)
  {
    std::cout << "finite difference check successful, max. rel. error: " << error_max_rel
              << " , max. abs. error: " << error_max_abs << std::endl;
    std::cout << "******************finite difference check done***************\n\n" << std::endl;
  }
  else
    FOUR_C_THROW("PoroFDCheck failed");
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::
    PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling(
        const Epetra_Comm& comm, const Teuchos::ParameterList& globaltimeparams)
    : PoroMultiPhaseScaTraMonolithicTwoWay(comm, globaltimeparams)
{
  blockrowdofmap_artscatra_ = Teuchos::rcp(new Core::LinAlg::MultiMapExtractor);
  blockrowdofmap_artporo_ = Teuchos::rcp(new Core::LinAlg::MultiMapExtractor);
  nodal_coupl_inactive_ = false;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::SetupSystem()
{
  PoroMultiPhaseScaTraMonolithicTwoWay::SetupSystem();

  //! arteryscatra-artery coupling matrix, this matrix has the full map of all coupled + uncoupled
  //! DOFs
  k_asa_ = Teuchos::rcp(new Core::LinAlg::SparseMatrix(
      *(scatramsht_->ArtScatraField()->discretization()->dof_row_map()), 81, true, true));

  //! simple check if nodal coupling active or not, if condensed and un-condensed dofrowmaps have
  //! equal size
  nodal_coupl_inactive_ =
      ((poro_field()->ArteryDofRowMap()->NumGlobalElements() == poro_field()
                                                                    ->fluid_field()
                                                                    ->ArtNetTimInt()
                                                                    ->discretization()
                                                                    ->dof_row_map(0)
                                                                    ->NumGlobalElements())) &&
      (scatramsht_->ArtScatraDofRowMap()->NumGlobalElements() ==
          scatramsht_->ArtScatraField()->discretization()->dof_row_map(0)->NumGlobalElements());
}
/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::setup_maps()
{
  // create combined map
  std::vector<Teuchos::RCP<const Epetra_Map>> vecSpaces;

  if (solve_structure_)
  {
    vecSpaces.push_back(poro_field()->StructDofRowMap());
    vecSpaces.push_back(poro_field()->FluidDofRowMap());
    const Epetra_Map* dofrowmapscatra =
        (ScatraAlgo()->ScaTraField()->discretization())->dof_row_map(0);
    vecSpaces.push_back(Teuchos::rcp(dofrowmapscatra, false));
    vecSpaces.push_back(poro_field()->ArteryDofRowMap());
    vecSpaces.push_back(scatramsht_->ArtScatraDofRowMap());
    if (vecSpaces[0]->NumGlobalElements() == 0) FOUR_C_THROW("No poro structure equation. Panic.");
    if (vecSpaces[1]->NumGlobalElements() == 0) FOUR_C_THROW("No poro fluid equation. Panic.");
    if (vecSpaces[2]->NumGlobalElements() == 0) FOUR_C_THROW("No scatra equation. Panic.");
    if (vecSpaces[3]->NumGlobalElements() == 0) FOUR_C_THROW("No artery equation. Panic.");
    if (vecSpaces[4]->NumGlobalElements() == 0) FOUR_C_THROW("No artery scatra equation. Panic.");
  }
  else
  {
    vecSpaces.push_back(poro_field()->FluidDofRowMap());
    const Epetra_Map* dofrowmapscatra =
        (ScatraAlgo()->ScaTraField()->discretization())->dof_row_map(0);
    vecSpaces.push_back(Teuchos::rcp(dofrowmapscatra, false));
    vecSpaces.push_back(poro_field()->ArteryDofRowMap());
    vecSpaces.push_back(scatramsht_->ArtScatraDofRowMap());
    if (vecSpaces[0]->NumGlobalElements() == 0) FOUR_C_THROW("No poro fluid equation. Panic.");
    if (vecSpaces[1]->NumGlobalElements() == 0) FOUR_C_THROW("No scatra equation. Panic.");
    if (vecSpaces[2]->NumGlobalElements() == 0) FOUR_C_THROW("No artery equation. Panic.");
    if (vecSpaces[3]->NumGlobalElements() == 0) FOUR_C_THROW("No artery scatra equation. Panic.");
  }

  // full fluid-structure-scatra-artery-arteryscatra map
  fullmap_ = Core::LinAlg::MultiMapExtractor::MergeMaps(vecSpaces);

  // full Poromultiphasescatra block map coupled with artery network
  blockrowdofmap_->Setup(*fullmap_, vecSpaces);

  // check global map extractor
  blockrowdofmap_->check_for_valid_map_extractor();

  // full porofluid-artery map
  fullmap_artporo_ = Core::LinAlg::MultiMapExtractor::MergeMaps(
      {vecSpaces[struct_offset_], vecSpaces[struct_offset_ + 2]});

  // full porofluid-artery blockmap
  blockrowdofmap_artporo_->Setup(
      *fullmap_artporo_, {vecSpaces[struct_offset_], vecSpaces[struct_offset_ + 2]});

  // full artery-arteryscatra map
  fullmap_artscatra_ = Core::LinAlg::MultiMapExtractor::MergeMaps(
      {vecSpaces[struct_offset_ + 1], vecSpaces[struct_offset_ + 3]});

  // full artery-arteryscatra blockmap
  blockrowdofmap_artscatra_->Setup(
      *fullmap_artscatra_, {vecSpaces[struct_offset_ + 1], vecSpaces[struct_offset_ + 3]});
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::update_scatra(
    Teuchos::RCP<const Epetra_Vector> scatrainc)
{
  ScatraAlgo()->ScaTraField()->UpdateIter(blockrowdofmap_artscatra_->ExtractVector(scatrainc, 0));
  scatramsht_->UpdateArtScatraIter(scatrainc);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::
    extract_field_vectors(Teuchos::RCP<const Epetra_Vector> x,
        Teuchos::RCP<const Epetra_Vector>& stx, Teuchos::RCP<const Epetra_Vector>& flx,
        Teuchos::RCP<const Epetra_Vector>& scx)
{
  TEUCHOS_FUNC_TIME_MONITOR(
      "PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::extract_field_vectors");

  // process structure unknowns of the first field
  if (solve_structure_)
    stx = Extractor()->ExtractVector(x, 0);
  else
    stx = Teuchos::rcp(new Epetra_Vector(*poro_field()->StructDofRowMap(), true));

  // process artery and porofluid unknowns
  Teuchos::RCP<const Epetra_Vector> porofluid = Extractor()->ExtractVector(x, struct_offset_);
  Teuchos::RCP<const Epetra_Vector> artery = Extractor()->ExtractVector(x, struct_offset_ + 2);

  Teuchos::RCP<Epetra_Vector> dummy1 = Teuchos::rcp(new Epetra_Vector(*fullmap_artporo_));

  // build the combined increment of porofluid and artery
  blockrowdofmap_artporo_->InsertVector(porofluid, 0, dummy1);
  blockrowdofmap_artporo_->InsertVector(artery, 1, dummy1);

  flx = dummy1;

  // process scatra and artery scatra unknowns of the third field
  Teuchos::RCP<const Epetra_Vector> scatra = Extractor()->ExtractVector(x, struct_offset_ + 1);
  Teuchos::RCP<const Epetra_Vector> artscatra = Extractor()->ExtractVector(x, struct_offset_ + 3);

  Teuchos::RCP<Epetra_Vector> dummy2 = Teuchos::rcp(new Epetra_Vector(*fullmap_artscatra_));

  // build the combined increment of artery and artery-scatra
  blockrowdofmap_artscatra_->InsertVector(scatra, 0, dummy2);
  blockrowdofmap_artscatra_->InsertVector(artscatra, 1, dummy2);

  scx = dummy2;
}
/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::setup_system_matrix()
{
  PoroMultiPhaseScaTraMonolithicTwoWay::setup_system_matrix();

  // --------------------------------------------------------------------------- artery-porofluid
  // get matrix block
  Teuchos::RCP<Core::LinAlg::BlockSparseMatrixBase> mat_pp = poro_field()->BlockSystemMatrix();

  // artery part
  systemmatrix_->Assign(
      struct_offset_ + 2, struct_offset_ + 2, Core::LinAlg::View, mat_pp->Matrix(2, 2));
  // artery-porofluid part
  systemmatrix_->Assign(
      struct_offset_ + 2, struct_offset_, Core::LinAlg::View, mat_pp->Matrix(2, 1));
  // porofluid-artery part
  systemmatrix_->Assign(
      struct_offset_, struct_offset_ + 2, Core::LinAlg::View, mat_pp->Matrix(1, 2));

  // -------------------------------------------------------------------------arteryscatra-scatra
  // arteryscatra part
  systemmatrix_->Assign(struct_offset_ + 3, struct_offset_ + 3, Core::LinAlg::View,
      scatramsht_->combined_system_matrix()->Matrix(1, 1));
  // scatra-arteryscatra part
  systemmatrix_->Assign(struct_offset_ + 1, struct_offset_ + 3, Core::LinAlg::View,
      scatramsht_->combined_system_matrix()->Matrix(0, 1));
  // arteryscatra-scatra part
  systemmatrix_->Assign(struct_offset_ + 3, struct_offset_ + 1, Core::LinAlg::View,
      scatramsht_->combined_system_matrix()->Matrix(1, 0));

  if (nodal_coupl_inactive_)
  {
    // create empty matrix
    Teuchos::RCP<Core::LinAlg::SparseMatrix> k_asa = artery_scatra_artery_coupling_matrix();

    // call the scatra-elements and calculate the off-diagonal structure matrix block
    apply_artery_scatra_artery_coupl_matrix(k_asa);

    // apply DBC's also on off-diagonal scatra-fluid coupling block (main-diagonal blocks have
    // already been set, either in poromultielast_monolithic.cpp or in the respective evalute calls)
    k_asa->ApplyDirichlet(*scatramsht_->ArtScatraField()->DirichMaps()->CondMap(), false);

    // arteryscatra-scatra part
    systemmatrix_->Assign(struct_offset_ + 3, struct_offset_ + 2, Core::LinAlg::View, *k_asa);
  }

  systemmatrix_->Complete();
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::setup_rhs()
{
  // create full monolithic rhs vector
  if (rhs_ == Teuchos::null) rhs_ = Teuchos::rcp(new Epetra_Vector(*dof_row_map(), true));

  // structure
  if (solve_structure_)
    Extractor()->InsertVector(
        *(poro_field()->Extractor()->ExtractVector(poro_field()->RHS(), 0)), 0, *rhs_);
  // porofluid
  Extractor()->InsertVector(
      *(poro_field()->Extractor()->ExtractVector(poro_field()->RHS(), 1)), struct_offset_, *rhs_);
  // scatra
  Extractor()->InsertVector(
      *(blockrowdofmap_artscatra_->ExtractVector(scatramsht_->CombinedRHS(), 0)),
      struct_offset_ + 1, *rhs_);

  // artery
  Extractor()->InsertVector(*(poro_field()->Extractor()->ExtractVector(poro_field()->RHS(), 2)),
      struct_offset_ + 2, *rhs_);
  // arteryscatra
  Extractor()->InsertVector(
      *(blockrowdofmap_artscatra_->ExtractVector(scatramsht_->CombinedRHS(), 1)),
      struct_offset_ + 3, *rhs_);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::
    build_convergence_norms()
{
  Teuchos::RCP<const Epetra_Vector> arteryrhs =
      Extractor()->ExtractVector(rhs_, struct_offset_ + 2);
  Teuchos::RCP<const Epetra_Vector> arteryinc =
      Extractor()->ExtractVector(iterinc_, struct_offset_ + 2);

  // build also norms for artery
  normrhsart_ = UTILS::calculate_vector_norm(vectornormfres_, arteryrhs);
  normincart_ = UTILS::calculate_vector_norm(vectornorminc_, arteryinc);
  arterypressnorm_ = UTILS::calculate_vector_norm(
      vectornorminc_, (poro_field()->fluid_field()->ArtNetTimInt()->Pressurenp()));

  Teuchos::RCP<const Epetra_Vector> arteryscarhs =
      Extractor()->ExtractVector(rhs_, struct_offset_ + 3);
  Teuchos::RCP<const Epetra_Vector> arteryscainc =
      Extractor()->ExtractVector(iterinc_, struct_offset_ + 3);

  // build also norms for artery
  normrhsartsca_ = UTILS::calculate_vector_norm(vectornormfres_, arteryscarhs);
  normincartsca_ = UTILS::calculate_vector_norm(vectornorminc_, arteryscainc);
  arteryscanorm_ =
      UTILS::calculate_vector_norm(vectornorminc_, (scatramsht_->ArtScatraField()->Phinp()));

  // call base class
  PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWay::build_convergence_norms();
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::evaluate_scatra()
{
  PoroMultiPhaseScaTraMonolithicTwoWay::evaluate_scatra();
  scatramsht_->SetupSystem(
      ScatraAlgo()->ScaTraField()->SystemMatrix(), ScatraAlgo()->ScaTraField()->Residual());
}

/*-----------------------------------------------------------------------/
/-----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::
    build_combined_dbc_map()
{
  PoroMultiPhaseScaTraMonolithicTwoWay::build_combined_dbc_map();

  const Teuchos::RCP<const Epetra_Map> artscatracondmap =
      scatramsht_->ArtScatraField()->DirichMaps()->CondMap();

  combinedDBCMap_ = Core::LinAlg::MergeMap(combinedDBCMap_, artscatracondmap, false);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::RCP<Core::LinAlg::SparseMatrix> PoroMultiPhaseScaTra::
    PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::artery_scatra_artery_coupling_matrix()
{
  Teuchos::RCP<Core::LinAlg::SparseMatrix> sparse =
      Teuchos::rcp_dynamic_cast<Core::LinAlg::SparseMatrix>(k_asa_);
  if (sparse == Teuchos::null) FOUR_C_THROW("cast to Core::LinAlg::SparseMatrix failed!");

  return sparse;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::
    apply_artery_scatra_artery_coupl_matrix(
        Teuchos::RCP<Core::LinAlg::SparseOperator> k_asa  //!< off-diagonal tangent matrix term
    )
{
  // create the parameters for the discretization
  Teuchos::ParameterList sparams_artery;

  k_asa->Zero();

  Core::UTILS::AddEnumClassToParameterList<ScaTra::Action>(
      "action", ScaTra::Action::calc_scatra_mono_odblock_fluid, sparams_artery);
  // other parameters that might be needed by the elements
  sparams_artery.set("delta time", Dt());
  sparams_artery.set("total time", Time());

  scatramsht_->ArtScatraField()->discretization()->ClearState();
  scatramsht_->ArtScatraField()->discretization()->set_state(
      0, "phinp", scatramsht_->ArtScatraField()->Phinp());
  scatramsht_->ArtScatraField()->discretization()->set_state(
      0, "hist", scatramsht_->ArtScatraField()->Hist());
  scatramsht_->ArtScatraField()->discretization()->set_state(
      2, "one_d_artery_pressure", poro_field()->fluid_field()->ArtNetTimInt()->Pressurenp());

  // build specific assemble strategy for mechanical-fluid system matrix
  Core::FE::AssembleStrategy artscatrastrategy_artery(0,  // scatradofset for row
      2,                                                  // arterydofset for column
      k_asa,                                              // scatra-artery coupling matrix
      Teuchos::null, Teuchos::null, Teuchos::null, Teuchos::null);

  scatramsht_->ArtScatraField()->discretization()->Evaluate(
      sparams_artery, artscatrastrategy_artery);

  // complete
  k_asa->Complete(poro_field()->fluid_field()->ArtNetTimInt()->SystemMatrix()->RangeMap(),
      scatramsht_->ArtScatraField()->SystemMatrix()->RangeMap());

  scatramsht_->ArtScatraField()->discretization()->ClearState();
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void PoroMultiPhaseScaTra::PoroMultiPhaseScaTraMonolithicTwoWayArteryCoupling::
    build_block_null_spaces()
{
  // base class -> structure, porofluid, scatra
  PoroMultiPhaseScaTraMonolithicTwoWay::build_block_null_spaces();

  // artery
  poro_field()->build_artery_block_null_space(solver_, struct_offset_ + 3);

  // artery-scatra
  Teuchos::ParameterList& blocksmootherparams5 =
      solver_->Params().sublist("Inverse" + std::to_string(struct_offset_ + 4));
  blocksmootherparams5.sublist("Belos Parameters");
  blocksmootherparams5.sublist("MueLu Parameters");

  // build null space of complete discretization
  scatramsht_->ArtScatraField()->discretization()->compute_null_space_if_necessary(
      blocksmootherparams5);
  // fix the null space if some DOFs are condensed out
  Core::LinearSolver::Parameters::FixNullSpace("ArteryScatra",
      *(scatramsht_->ArtScatraField()->discretization()->dof_row_map(0)),
      *(scatramsht_->ArtScatraDofRowMap()), blocksmootherparams5);
}

FOUR_C_NAMESPACE_CLOSE
