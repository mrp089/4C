/*----------------------------------------------------------------------*/
/*! \file

\brief  Basis of all monolithic poroelasticity algorithms

\level 2


*/
#include "4C_poroelast_monolithic.hpp"

#include "4C_adapter_fld_base_algorithm.hpp"
#include "4C_adapter_fld_poro.hpp"
#include "4C_adapter_str_fpsiwrapper.hpp"
#include "4C_contact_lagrange_strategy_poro.hpp"
#include "4C_contact_meshtying_contact_bridge.hpp"
#include "4C_contact_meshtying_poro_lagrange_strategy.hpp"
#include "4C_contact_nitsche_strategy_poro.hpp"
#include "4C_discretization_condition_utils.hpp"
#include "4C_discretization_fem_general_assemblestrategy.hpp"
#include "4C_fluid_ele_action.hpp"
#include "4C_fluid_utils_mapextractor.hpp"
#include "4C_global_data.hpp"
#include "4C_inpar_solver.hpp"
#include "4C_io_control.hpp"
#include "4C_lib_elements_paramsminimal.hpp"
#include "4C_linalg_equilibrate.hpp"
#include "4C_linalg_utils_sparse_algebra_assemble.hpp"
#include "4C_linalg_utils_sparse_algebra_create.hpp"
#include "4C_linalg_utils_sparse_algebra_manipulation.hpp"
#include "4C_linear_solver_method.hpp"
#include "4C_linear_solver_method_linalg.hpp"
#include "4C_mortar_manager_base.hpp"
#include "4C_structure_aux.hpp"

#include <Teuchos_Time.hpp>
#include <Teuchos_TimeMonitor.hpp>

#include <sstream>

FOUR_C_NAMESPACE_OPEN



POROELAST::Monolithic::Monolithic(const Epetra_Comm& comm, const Teuchos::ParameterList& timeparams,
    Teuchos::RCP<CORE::LINALG::MapExtractor> porosity_splitter)
    : PoroBase(comm, timeparams, porosity_splitter),
      printscreen_(true),  // ADD INPUT PARAMETER
      printiter_(true),    // ADD INPUT PARAMETER
      zeros_(Teuchos::null),
      blockrowdofmap_(Teuchos::null),
      normtypeinc_(INPAR::POROELAST::convnorm_undefined),
      normtypefres_(INPAR::POROELAST::convnorm_undefined),
      combincfres_(INPAR::POROELAST::bop_undefined),
      vectornormfres_(INPAR::POROELAST::norm_undefined),
      vectornorminc_(INPAR::POROELAST::norm_undefined),
      tolinc_(0.0),
      tolfres_(0.0),
      tolinc_struct_(0.0),
      tolfres_struct_(0.0),
      tolinc_velocity_(0.0),
      tolfres_velocity_(0.0),
      tolinc_pressure_(0.0),
      tolfres_pressure_(0.0),
      tolinc_porosity_(0.0),
      tolfres_porosity_(0.0),
      itermax_(0),
      itermin_(0),
      normrhs_(0.0),
      norminc_(0.0),
      normrhsfluidvel_(0.0),
      normincfluidvel_(0.0),
      normrhsfluidpres_(0.0),
      normincfluidpres_(0.0),
      normrhsfluid_(0.0),
      normincfluid_(0.0),
      normrhsstruct_(0.0),
      normincstruct_(0.0),
      normrhsporo_(0.0),
      normincporo_(0.0),
      timer_(Teuchos::rcp(new Teuchos::Time("", false))),
      iter_(-1),
      iterinc_(Teuchos::null),
      directsolve_(true),
      del_(Teuchos::null),
      delhist_(Teuchos::null),
      mu_(0.0),
      equilibration_(Teuchos::null),
      equilibration_method_(CORE::LINALG::EquilibrationMethod::none)
{
  const Teuchos::ParameterList& sdynparams =
      GLOBAL::Problem::Instance()->structural_dynamic_params();

  // some solver paramaters are red form the structure dynamic list (this is not the best way to do
  // it ...)
  solveradapttol_ = (CORE::UTILS::IntegralValue<int>(sdynparams, "ADAPTCONV") == 1);
  solveradaptolbetter_ = (sdynparams.get<double>("ADAPTCONV_BETTER"));

  const Teuchos::ParameterList& poroparams =
      GLOBAL::Problem::Instance()->poroelast_dynamic_params();
  equilibration_method_ =
      Teuchos::getIntegralValue<CORE::LINALG::EquilibrationMethod>(poroparams, "EQUILIBRATION");

  strmethodname_ = CORE::UTILS::IntegralValue<INPAR::STR::DynamicType>(sdynparams, "DYNAMICTYP");
  no_penetration_ = false;
  nit_contact_ = false;
  // if inpar is set to nopenetration for contact!!! to be done!
  // clean up as soon as old time integration is unused!
  if (oldstructimint_)
  {
    if (structure_field()->meshtying_contact_bridge() != Teuchos::null)
    {
      if (structure_field()->meshtying_contact_bridge()->HaveContact())
      {
        auto* poro_lagrange_strat = dynamic_cast<CONTACT::LagrangeStrategyPoro*>(
            &structure_field()->meshtying_contact_bridge()->ContactManager()->GetStrategy());
        if (poro_lagrange_strat)
          no_penetration_ = poro_lagrange_strat->has_poro_no_penetration();
        else
        {
          auto* co_nitsche_strategy = dynamic_cast<CONTACT::NitscheStrategyPoro*>(
              &structure_field()->meshtying_contact_bridge()->ContactManager()->GetStrategy());
          if (co_nitsche_strategy)
          {
            nit_contact_ = true;
            no_penetration_ = co_nitsche_strategy->has_poro_no_penetration();
          }
        }
      }
    }
  }
  blockrowdofmap_ = Teuchos::rcp(new CORE::LINALG::MultiMapExtractor);

  // contact no penetration constraint not yet works for non-matching structure and fluid
  // discretizations
  if (no_penetration_ and (not matchinggrid_))
  {
    FOUR_C_THROW(
        "The contact no penetration constraint does not yet work for non-matching "
        "discretizations!");
  }
}

void POROELAST::Monolithic::do_time_step()
{
  // counter and print header
  // predict solution of both field (call the adapter)
  prepare_time_step();

  // Newton-Raphson iteration
  Solve();

  // calculate stresses, strains, energies
  constexpr bool force_prepare = false;
  prepare_output(force_prepare);

  // update all single field solvers
  Update();

  // write output to screen and files
  Output();
}

void POROELAST::Monolithic::Solve()
{
  // we do a Newton-Raphson iteration here.
  // the specific time integration has set the following
  // --> On #rhs_ is the positive force residuum
  // --> On #systemmatrix_ is the effective dynamic tangent matrix

  //---------------------------------------- initialise equilibrium loop and norms
  SetupNewton();

  //---------------------------------------------- iteration loop
  // equilibrium iteration loop (loop over k)
  while (((not Converged()) and (iter_ <= itermax_)) or (iter_ <= itermin_))
  {
    timer_->start();
    Teuchos::Time timer("eval", true);
    timer.start();
    // compute residual forces #rhs_ and tangent #tang_
    // whose components are globally oriented
    // build linear system stiffness matrix and rhs/force residual for each
    // field, here e.g. for structure field: field want the iteration increment
    // 1.) Update(iterinc_),
    // 2.) evaluate_force_stiff_residual(),
    // 3.) prepare_system_for_newton_solve()
    Evaluate(iterinc_, iter_ == 1);
    // std::cout << "  time for Evaluate : " <<  timer.totalElapsedTime(true) << "\n";
    // timer.reset();

    // if (iter_>1 and Step()>2 )
    // PoroFDCheck();

    // build norms
    build_convergence_norms();
    if ((not Converged()) or combincfres_ == INPAR::POROELAST::bop_or)
    {
      // (Newton-ready) residual with blanked Dirichlet DOFs (see adapter_timint!)
      // is done in prepare_system_for_newton_solve() within Evaluate(iterinc_)
      linear_solve();
      // std::cout << "  time for Evaluate linear_solve: " << timer.totalElapsedTime(true) << "\n";
      // timer.reset();

      // reset solver tolerance
      solver_->ResetTolerance();

      // rebuild norms
      build_convergence_norms();
    }

    // print stuff
    print_newton_iter();

    // Aitken();

    // Recover Lagrangean Multiplier in Newton Iteration (for contact & contact no penetration!)
    // adjust LM Recovery for the meshtying case
    recover_lagrange_multiplier_after_newton_step(iterinc_);

    // increment equilibrium loop index
    iter_ += 1;
  }  // end equilibrium loop

  //---------------------------------------------- output of number of iterations
  //  {
  //    std::ostringstream s;
  //    s << std::right << std::setw(16) << std::scientific << Time()
  //      << std::right << std::setw(10) << std::scientific << Step()
  //      << std::right << std::setw(10) << std::scientific << iter_-1;
  //
  //    std::ofstream f;
  //    const std::string fname = GLOBAL::Problem::Instance()->OutputControlFile()->FileName()
  //                            + "_numiter.txt";
  //
  //    if (Step() <= 1)
  //      f.open(fname.c_str(),std::fstream::trunc); //f << header.str() << std::endl;
  //    else
  //      f.open(fname.c_str(),std::fstream::ate | std::fstream::app);
  //
  //    f << s.str() << "\n";
  //    f.close();
  //  }

  // correct iteration counter
  iter_ -= 1;

  // test whether max iterations was hit
  if ((Converged()) and (Comm().MyPID() == 0))
  {
    print_newton_conv();
  }
  else if (iter_ >= itermax_)
  {
    FOUR_C_THROW("Newton unconverged in %d iterations", iter_);
  }
}

void POROELAST::Monolithic::update_state_incrementally(Teuchos::RCP<const Epetra_Vector> iterinc)
{
  TEUCHOS_FUNC_TIME_MONITOR("POROELAST::Monolithic::update_state_incrementally");

  // displacement and fluid velocity & pressure incremental vector
  Teuchos::RCP<const Epetra_Vector> s_iterinc = Teuchos::null;
  Teuchos::RCP<const Epetra_Vector> f_iterinc = Teuchos::null;

  // do nothing in the case no increment vector is given
  if (iterinc == Teuchos::null)
  {
    return;
  }
  else
  {
    // extract displacement sx and fluid fx incremental vector of global
    // unknown incremental vector x (different for splits)
    extract_field_vectors(iterinc, s_iterinc, f_iterinc);

    // update poro iterinc
    if (is_part_of_multifield_problem_) UpdatePoroIterinc(iterinc);
  }

  update_state_incrementally(s_iterinc, f_iterinc);
}

void POROELAST::Monolithic::update_state_incrementally(
    Teuchos::RCP<const Epetra_Vector> s_iterinc, Teuchos::RCP<const Epetra_Vector> f_iterinc)
{
  // Newton update of the fluid field
  // update velocities and pressures before passed to the structural field
  //  update_iter_incrementally(fx),
  fluid_field()->UpdateNewton(f_iterinc);

  // call all elements and assemble rhs and matrices
  // structural field

  // structure Evaluate (builds tangent, residual and applies DBC)

  // apply current velocity and pressures to structure
  set_fluid_solution();

  // apply current velocity of fluid to ContactMangager if contact problem
  if (no_penetration_)
    set_poro_contact_states();  // ATM svel is set in structure evaluate as the vel of the structure
                                // is evaluated there ...

  // Monolithic Poroelasticity accesses the linearised structure problem:
  //   UpdaterIterIncrementally(sx),
  structure_field()->update_state_incrementally(s_iterinc);
  // fluid field

  // fluid Evaluate
  // (builds tangent, residual and applies DBC and recent coupling values)

  // set structure displacements onto the fluid
  set_struct_solution();
}

void POROELAST::Monolithic::Evaluate(Teuchos::RCP<const Epetra_Vector> iterinc, bool firstiter)
{
  TEUCHOS_FUNC_TIME_MONITOR("POROELAST::Monolithic::Evaluate");

  // Evaluate Stiffness and RHS of Structural & Fluid field
  EvaluateFields(iterinc);

  // fill off diagonal blocks and fill global system matrix
  setup_system_matrix();

  // check whether we have a sanely filled tangent matrix
  if (not systemmatrix_->Filled())
  {
    FOUR_C_THROW("Effective tangent matrix must be filled here");
  }

  // create full monolithic rhs vector
  setup_rhs(firstiter);

  // Modify System for Contact or Meshtying!
  eval_poro_mortar();
}

void POROELAST::Monolithic::Evaluate(Teuchos::RCP<const Epetra_Vector> s_iterinc,
    Teuchos::RCP<const Epetra_Vector> f_iterinc, bool firstiter)
{
  TEUCHOS_FUNC_TIME_MONITOR("POROELAST::Monolithic::Evaluate");

  // Evaluate Stiffness and RHS of Structural & Fluid field
  EvaluateFields(s_iterinc, f_iterinc);

  // fill off diagonal blocks and fill global system matrix
  setup_system_matrix();

  // check whether we have a sanely filled tangent matrix
  if (not systemmatrix_->Filled())
  {
    FOUR_C_THROW("Effective tangent matrix must be filled here");
  }

  // create full monolithic rhs vector
  setup_rhs(firstiter);

  // Modify System for Contact or Meshtying!
  eval_poro_mortar();
}

void POROELAST::Monolithic::EvaluateFields(
    Teuchos::RCP<const Epetra_Vector> s_iterinc, Teuchos::RCP<const Epetra_Vector> f_iterinc)
{
  // Update State incrementally
  update_state_incrementally(s_iterinc, f_iterinc);

  // Monolithic Poroelasticity accesses the linearised structure problem:
  //   evaluate_force_stiff_residual()
  //   prepare_system_for_newton_solve()
  structure_field()->Evaluate(Teuchos::null);

  // monolithic Poroelasticity accesses the linearised fluid problem
  //   evaluate_rhs_tang_residual() and
  //   prepare_system_for_newton_solve()
  fluid_field()->Evaluate(Teuchos::null);
}

void POROELAST::Monolithic::EvaluateFields(Teuchos::RCP<const Epetra_Vector> iterinc)
{
  // Update State incrementally
  update_state_incrementally(iterinc);

  // Monolithic Poroelasticity accesses the linearised structure problem:
  //   evaluate_force_stiff_residual()
  //   prepare_system_for_newton_solve()
  structure_field()->Evaluate(Teuchos::null);

  // monolithic Poroelasticity accesses the linearised fluid problem
  //   evaluate_rhs_tang_residual() and
  //   prepare_system_for_newton_solve()
  fluid_field()->Evaluate(Teuchos::null);
}

void POROELAST::Monolithic::extract_field_vectors(Teuchos::RCP<const Epetra_Vector> x,
    Teuchos::RCP<const Epetra_Vector>& sx, Teuchos::RCP<const Epetra_Vector>& fx, bool firstcall)
{
  TEUCHOS_FUNC_TIME_MONITOR("POROELAST::Monolithic::extract_field_vectors");

  // process structure unknowns of the first field
  sx = Extractor()->ExtractVector(x, 0);

  // process fluid unknowns of the second field
  fx = Extractor()->ExtractVector(x, 1);
}

void POROELAST::Monolithic::SetupSystem()
{
  {
    // -------------------------------------------------------------create combined map
    std::vector<Teuchos::RCP<const Epetra_Map>> vecSpaces;

    // Note:
    // when using constraints applied via Lagrange-Multipliers there is a
    // difference between structure_field()->dof_row_map() and structure_field()->dof_row_map(0).
    // structure_field()->dof_row_map(0) returns the dof_row_map
    // known to the discretization (without lagrange multipliers)
    // while structure_field()->dof_row_map() returns the dof_row_map known to
    // the constraint manager (with lagrange multipliers)
    // In the constrained case we want the "whole" RowDofMap,
    // otherwise both calls are equivalent

    vecSpaces.push_back(structure_field()->dof_row_map());
    // use its own dof_row_map, that is the 0th map of the discretization
    vecSpaces.push_back(fluid_field()->dof_row_map(0));

    if (vecSpaces[0]->NumGlobalElements() == 0) FOUR_C_THROW("No structure equation. Panic.");
    if (vecSpaces[1]->NumGlobalElements() == 0) FOUR_C_THROW("No fluid equation. Panic.");

    // full Poroelasticity-map
    fullmap_ = CORE::LINALG::MultiMapExtractor::MergeMaps(vecSpaces);
    // full Poroelasticity-blockmap
    blockrowdofmap_->Setup(*fullmap_, vecSpaces);
  }
  // -------------------------------------------------------------

  //-----------------------------------build map of global dofs with DBC
  build_combined_dbc_map();
  // -------------------------------------------------------------

  // initialize Poroelasticity-systemmatrix_
  systemmatrix_ =
      Teuchos::rcp(new CORE::LINALG::BlockSparseMatrix<CORE::LINALG::DefaultBlockMatrixStrategy>(
          *Extractor(), *Extractor(), 81, false, true));

  k_sf_ = Teuchos::rcp(
      new CORE::LINALG::SparseMatrix(*(structure_field()->dof_row_map()), 81, true, true));
  k_fs_ = Teuchos::rcp(
      new CORE::LINALG::SparseMatrix(*(fluid_field()->discretization()->dof_row_map(0)),
          //*(fluid_field()->dof_row_map()),
          81, true, true));

  nopen_handle_->Setup(dof_row_map(), (fluid_field()->discretization()->dof_row_map(0)));

  SetupEquilibration();
}  // SetupSystem()

void POROELAST::Monolithic::SetupEquilibration()
{
  // instantiate appropriate equilibration class
  auto equilibration_method =
      std::vector<CORE::LINALG::EquilibrationMethod>(1, equilibration_method_);
  equilibration_ = CORE::LINALG::BuildEquilibration(
      CORE::LINALG::MatrixType::block_field, equilibration_method, fullmap_);
}

void POROELAST::Monolithic::setup_system_matrix(CORE::LINALG::BlockSparseMatrixBase& mat)
{
  TEUCHOS_FUNC_TIME_MONITOR("POROELAST::Monolithic::setup_system_matrix");

  // pure structural part k_ss (3nx3n)

  // build pure structural block k_ss
  // build block matrix
  // The maps of the block matrix have to match the maps of the blocks we
  // insert here. Extract Jacobian matrices and put them into composite system
  // matrix W
  Teuchos::RCP<CORE::LINALG::SparseMatrix> k_ss = structure_field()->SystemMatrix();

  if (k_ss == Teuchos::null) FOUR_C_THROW("structure system matrix null pointer!");

  /*----------------------------------------------------------------------*/
  // structural part k_sf (3nxn)
  // build mechanical-fluid block

  // create empty matrix
  Teuchos::RCP<CORE::LINALG::SparseMatrix> k_sf = struct_fluid_coupling_matrix();

  // call the element and calculate the matrix block
  apply_str_coupl_matrix(k_sf);

  if (!matchinggrid_)
  {
    k_sf->Complete(*(structure_field()->discretization()->dof_row_map(1)),
        *(structure_field()->discretization()->dof_row_map(0)));

    k_sf = volcoupl_->apply_matrix_mapping12(k_sf);
  }

  /*----------------------------------------------------------------------*/
  // pure fluid part k_ff ( (3n+1)x(3n+1) )

  // build pure fluid block k_ff
  // build block matrix
  // The maps of the block matrix have to match the maps of the blocks we
  // insert here. Extract Jacobian matrices and put them into composite system
  // matrix W
  Teuchos::RCP<CORE::LINALG::SparseMatrix> k_ff = fluid_field()->SystemMatrix();

  if (k_ff == Teuchos::null) FOUR_C_THROW("fuid system matrix null pointer!");

  if (nopen_handle_->HasCond())
  {
    // Evaluate poroelasticity specific conditions
    evaluate_condition(k_ff);
  }

  /*----------------------------------------------------------------------*/
  // fluid part k_fs ( (3n+1)x3n )
  // build fluid-mechanical block

  // create empty matrix
  Teuchos::RCP<CORE::LINALG::SparseMatrix> k_fs = fluid_struct_coupling_matrix();

  // call the element and calculate the matrix block
  apply_fluid_coupl_matrix(k_fs);

  if (!matchinggrid_)
  {
    k_fs->Complete(*(fluid_field()->discretization()->dof_row_map(1)),
        *(fluid_field()->discretization()->dof_row_map(0)));

    k_fs = volcoupl_->apply_matrix_mapping21(k_fs);
  }

  // assign structure part to the Poroelasticity matrix
  mat.Assign(0, 0, CORE::LINALG::View, *k_ss);
  // assign coupling part to the Poroelasticity matrix
  mat.Assign(0, 1, CORE::LINALG::View, *k_sf);
  // assign fluid part to the poroelasticity matrix
  mat.Assign(1, 1, CORE::LINALG::View, *k_ff);
  // assign coupling part to the Poroelasticity matrix
  mat.Assign(1, 0, CORE::LINALG::View, *k_fs);

  /*----------------------------------------------------------------------*/
  // done. make sure all blocks are filled.
  mat.Complete();
}

void POROELAST::Monolithic::setup_rhs(bool firstcall)
{
  TEUCHOS_FUNC_TIME_MONITOR("POROELAST::Monolithic::setup_rhs");

  // create full monolithic rhs vector
  if (rhs_ == Teuchos::null) rhs_ = Teuchos::rcp(new Epetra_Vector(*dof_row_map(), true));

  // fill the Poroelasticity rhs vector rhs_ with the single field rhss
  setup_vector(*rhs_, structure_field()->RHS(), fluid_field()->RHS());

  // add rhs terms due to no penetration condition
  nopen_handle_->ApplyCondRHS(iterinc_, rhs_);
}

void POROELAST::Monolithic::prepare_time_step() { PoroBase::prepare_time_step(); }

void POROELAST::Monolithic::linear_solve()
{
  CORE::LINALG::SolverParams solver_params;
  if (solveradapttol_ and (iter_ > 1))
  {
    solver_params.nonlin_tolerance = tolfres_;
    solver_params.nonlin_residual = normrhs_;
    solver_params.lin_tol_better = solveradaptolbetter_;
  }
  iterinc_->PutScalar(0.0);  // Useful? depends on solver and more

  // equilibrate global system of equations if necessary
  equilibration_->EquilibrateSystem(systemmatrix_, rhs_, blockrowdofmap_);

  if (directsolve_)
  {
    // merge blockmatrix to SparseMatrix
    Teuchos::RCP<CORE::LINALG::SparseMatrix> sparse = systemmatrix_->Merge();

    // apply dirichlet boundary conditions
    CORE::LINALG::apply_dirichlet_to_system(
        *sparse, *iterinc_, *rhs_, *zeros_, *combined_dbc_map());

    // standard solver call

    solver_params.refactor = true;
    solver_params.reset = iter_ == 1;
    solver_->Solve(sparse->EpetraOperator(), iterinc_, rhs_, solver_params);
  }
  else  // use bgs2x2_operator
  {
    // apply dirichlet boundary conditions
    CORE::LINALG::apply_dirichlet_to_system(
        *systemmatrix_, *iterinc_, *rhs_, *zeros_, *combined_dbc_map());

    // standard solver call
    solver_params.refactor = true;
    solver_params.reset = iter_ == 1;
    solver_->Solve(systemmatrix_->EpetraOperator(), iterinc_, rhs_, solver_params);
  }

  equilibration_->unequilibrate_increment(iterinc_);
}

void POROELAST::Monolithic::create_linear_solver()
{
  // get dynamic section
  const Teuchos::ParameterList& porodyn = GLOBAL::Problem::Instance()->poroelast_dynamic_params();

  // get the linear solver number
  const int linsolvernumber = porodyn.get<int>("LINEAR_SOLVER");
  if (linsolvernumber == (-1))
  {
    FOUR_C_THROW(
        "no linear solver defined for monolithic Poroelasticity. Please set LINEAR_SOLVER in "
        "POROELASTICITY DYNAMIC to a valid number!");
  }

  // get parameter list of structural dynamics
  const Teuchos::ParameterList& sdyn = GLOBAL::Problem::Instance()->structural_dynamic_params();
  // use solver blocks for structure
  // get the solver number used for structural solver
  const int slinsolvernumber = sdyn.get<int>("LINEAR_SOLVER");
  // check if the structural solver has a valid solver number
  if (slinsolvernumber == (-1))
  {
    FOUR_C_THROW(
        "no linear solver defined for structural field. Please set LINEAR_SOLVER in STRUCTURAL "
        "DYNAMIC to a valid number!");
  }

  // get parameter list of fluid dynamics
  const Teuchos::ParameterList& fdyn = GLOBAL::Problem::Instance()->FluidDynamicParams();
  // use solver blocks for fluid
  // get the solver number used for fluid solver
  const int flinsolvernumber = fdyn.get<int>("LINEAR_SOLVER");
  // check if the fluid solver has a valid solver number
  if (flinsolvernumber == (-1))
  {
    FOUR_C_THROW(
        "no linear solver defined for fluid field. Please set LINEAR_SOLVER in FLUID DYNAMIC to a "
        "valid number!");
  }

  // get solver parameter list of linear Poroelasticity solver
  const Teuchos::ParameterList& porosolverparams =
      GLOBAL::Problem::Instance()->SolverParams(linsolvernumber);

  const auto solvertype =
      Teuchos::getIntegralValue<CORE::LINEAR_SOLVER::SolverType>(porosolverparams, "SOLVER");

  if (solvertype != CORE::LINEAR_SOLVER::SolverType::belos)
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
  const auto azprectype = Teuchos::getIntegralValue<CORE::LINEAR_SOLVER::PreconditionerType>(
      porosolverparams, "AZPREC");

  // plausibility check
  switch (azprectype)
  {
    case CORE::LINEAR_SOLVER::PreconditionerType::block_gauss_seidel_2x2:
      break;
    case CORE::LINEAR_SOLVER::PreconditionerType::multigrid_nxn:
    {
      // no plausibility checks here
      // if you forget to declare an xml file you will get an error message anyway
    }
    break;
    default:
      FOUR_C_THROW("Block Gauss-Seidel BGS2x2 preconditioner expected");
      break;
  }

  solver_ = Teuchos::rcp(new CORE::LINALG::Solver(porosolverparams, Comm()));

  // use solver blocks for structure and fluid
  const Teuchos::ParameterList& ssolverparams =
      GLOBAL::Problem::Instance()->SolverParams(slinsolvernumber);
  const Teuchos::ParameterList& fsolverparams =
      GLOBAL::Problem::Instance()->SolverParams(flinsolvernumber);

  solver_->put_solver_params_to_sub_params("Inverse1", ssolverparams);
  solver_->put_solver_params_to_sub_params("Inverse2", fsolverparams);

  // prescribe rigid body modes
  structure_field()->discretization()->compute_null_space_if_necessary(
      solver_->Params().sublist("Inverse1"));
  fluid_field()->discretization()->compute_null_space_if_necessary(
      solver_->Params().sublist("Inverse2"));
}

void POROELAST::Monolithic::initial_guess(Teuchos::RCP<Epetra_Vector> ig)
{
  TEUCHOS_FUNC_TIME_MONITOR("POROELAST::Monolithic::initial_guess");

  // InitalGuess() is called of the single fields and results are put in
  // increment vector ig
  setup_vector(*ig,
      // returns residual displacements \f$\Delta D_{n+1}^{<k>}\f$ - disi_
      structure_field()->initial_guess(),
      // returns residual velocities or iterative fluid increment - incvel_
      fluid_field()->initial_guess());
}

void POROELAST::Monolithic::setup_vector(
    Epetra_Vector& f, Teuchos::RCP<const Epetra_Vector> sv, Teuchos::RCP<const Epetra_Vector> fv)
{
  // extract dofs of the two fields
  // and put the structural/fluid field vector into the global vector f
  // noticing the block number

  Extractor()->InsertVector(*sv, 0, f);
  if (not oldstructimint_) f.Scale(-1);
  Extractor()->InsertVector(*fv, 1, f);
}

bool POROELAST::Monolithic::Converged()
{
  // check for single norms
  bool convinc = false;
  bool convfres = false;

  // residual increments
  switch (normtypeinc_)
  {
    case INPAR::POROELAST::convnorm_abs_global:
      convinc = norminc_ < tolinc_;
      break;
    case INPAR::POROELAST::convnorm_abs_singlefields:
      convinc = (normincstruct_ < tolinc_struct_ and normincfluidvel_ < tolinc_velocity_ and
                 normincfluidpres_ < tolinc_pressure_ and normincporo_ < tolinc_porosity_);
      break;
    default:
      FOUR_C_THROW("Cannot check for convergence of residual values!");
      break;
  }

  // residual forces
  switch (normtypefres_)
  {
    case INPAR::POROELAST::convnorm_abs_global:
      convfres = normrhs_ < tolfres_;
      break;
    case INPAR::POROELAST::convnorm_abs_singlefields:
      convfres = (normrhsstruct_ < tolfres_struct_ and normrhsfluidvel_ < tolfres_velocity_ and
                  normrhsfluidpres_ < tolfres_pressure_ and normrhsporo_ < tolfres_porosity_);
      break;
    default:
      FOUR_C_THROW("Cannot check for convergence of residual forces!");
      break;
  }

  // combine increments and forces
  bool conv = false;
  if (combincfres_ == INPAR::POROELAST::bop_and)
    conv = convinc and convfres;
  else if (combincfres_ == INPAR::POROELAST::bop_or)
    conv = convinc or convfres;
  else
    FOUR_C_THROW("Something went terribly wrong with binary operator!");

  if (conv)
  {
    // clean up as soon as old time integration is unused!
    if (oldstructimint_)
    {
      if (structure_field()->meshtying_contact_bridge() != Teuchos::null)
      // assume dual mortar lagmult contact!!! ... for general case store member into algo!
      {
        if (structure_field()->meshtying_contact_bridge()->HaveContact())
        {
          conv = structure_field()
                     ->meshtying_contact_bridge()
                     ->GetStrategy()
                     .active_set_semi_smooth_converged();
        }
      }
    }
  }

  // return things
  return conv;
}

void POROELAST::Monolithic::print_newton_iter()
{
  // print to standard out
  // replace myrank_ here general by Comm().MyPID()
  if ((Comm().MyPID() == 0) and printscreen_ and (Step() % printscreen_ == 0) and printiter_)
  {
    if (iter_ == 1) print_newton_iter_header(stdout);
    print_newton_iter_text(stdout);
  }
}

void POROELAST::Monolithic::print_newton_iter_header(FILE* ofile)
{
  // open outstringstream
  std::ostringstream oss;

  print_newton_iter_header_stream(oss);

  // add solution time
  oss << std::setw(14) << "wct";
  // finish oss
  oss << std::ends;

  // print to screen (could be done differently...)
  if (ofile == nullptr) FOUR_C_THROW("no ofile available");
  fprintf(ofile, "%s\n", oss.str().c_str());

  // print it, now
  fflush(ofile);
}

void POROELAST::Monolithic::print_newton_iter_header_stream(std::ostringstream& oss)
{
  oss << "------------------------------------------------------------" << std::endl;
  oss << "                   Newton-Raphson Scheme                    " << std::endl;
  oss << "                NormRES " << VectorNormString(vectornormfres_);
  oss << "     NormINC " << VectorNormString(vectornorminc_) << "                    " << std::endl;
  oss << "------------------------------------------------------------" << std::endl;

  // enter converged state etc
  oss << "numiter";

  // different style due relative or absolute error checking

  // --------------------------------------------------------global system test
  // residual forces
  switch (normtypefres_)
  {
    case INPAR::POROELAST::convnorm_abs_global:
      oss << std::setw(15) << "abs-res"
          << "(" << std::setw(5) << std::setprecision(2) << tolfres_ << ")";
      break;
    case INPAR::POROELAST::convnorm_abs_singlefields:
      oss << std::setw(15) << "abs-s-res"
          << "(" << std::setw(5) << std::setprecision(2) << tolfres_struct_ << ")";
      if (porosity_dof_)
        oss << std::setw(15) << "abs-poro-res"
            << "(" << std::setw(5) << std::setprecision(2) << tolfres_porosity_ << ")";
      oss << std::setw(15) << "abs-fvel-res"
          << "(" << std::setw(5) << std::setprecision(2) << tolfres_velocity_ << ")";
      oss << std::setw(15) << "abs-fpres-res"
          << "(" << std::setw(5) << std::setprecision(2) << tolfres_pressure_ << ")";
      break;
    default:
      FOUR_C_THROW("Unknown or undefined convergence form for residual.");
      break;
  }

  switch (normtypeinc_)
  {
    case INPAR::POROELAST::convnorm_abs_global:
      oss << std::setw(15) << "abs-inc"
          << "(" << std::setw(5) << std::setprecision(2) << tolinc_ << ")";
      break;
      break;
    case INPAR::POROELAST::convnorm_abs_singlefields:
      oss << std::setw(15) << "abs-s-inc"
          << "(" << std::setw(5) << std::setprecision(2) << tolinc_struct_ << ")";
      if (porosity_dof_)
        oss << std::setw(15) << "abs-poro-inc"
            << "(" << std::setw(5) << std::setprecision(2) << tolinc_porosity_ << ")";
      oss << std::setw(15) << "abs-fvel-inc"
          << "(" << std::setw(5) << std::setprecision(2) << tolinc_velocity_ << ")";
      oss << std::setw(15) << "abs-fpres-inc"
          << "(" << std::setw(5) << std::setprecision(2) << tolinc_pressure_ << ")";
      break;
    default:
      FOUR_C_THROW("Unknown or undefined convergence form for increment.");
      break;
  }
}

void POROELAST::Monolithic::print_newton_iter_text(FILE* ofile)
{
  // open outstringstream
  std::ostringstream oss;

  print_newton_iter_text_stream(oss);

  // add solution time
  oss << std::setw(14) << std::setprecision(2) << std::scientific << timer_->totalElapsedTime(true);
  // finish oss
  oss << std::ends;

  // print to screen (could be done differently...)
  if (ofile == nullptr) FOUR_C_THROW("no ofile available");
  fprintf(ofile, "%s\n", oss.str().c_str());

  // print it, now
  fflush(ofile);
}

void POROELAST::Monolithic::print_newton_iter_text_stream(std::ostringstream& oss)
{
  // enter converged state etc
  oss << std::setw(7) << iter_;

  // different style due relative or absolute error checking

  // --------------------------------------------------------global system test
  // residual forces
  switch (normtypefres_)
  {
    case INPAR::POROELAST::convnorm_abs_global:
      oss << std::setw(22) << std::setprecision(5) << std::scientific << normrhs_;
      break;
    case INPAR::POROELAST::convnorm_abs_singlefields:
      break;
    default:
      FOUR_C_THROW("Unknown or undefined convergence form for global residual.");
      break;
  }
  // increments
  switch (normtypeinc_)
  {
    case INPAR::POROELAST::convnorm_abs_global:
      oss << std::setw(22) << std::setprecision(5) << std::scientific << norminc_;
      break;
    case INPAR::POROELAST::convnorm_abs_singlefields:
      break;
    default:
      FOUR_C_THROW("Unknown or undefined convergence form for global increment.");
      break;
  }

  // --------------------------------------------------------single field test
  switch (normtypefres_)
  {
    case INPAR::POROELAST::convnorm_abs_singlefields:
      oss << std::setw(22) << std::setprecision(5) << std::scientific << normrhsstruct_;
      if (porosity_dof_)
        oss << std::setw(22) << std::setprecision(5) << std::scientific << normrhsporo_;
      oss << std::setw(22) << std::setprecision(5) << std::scientific << normrhsfluidvel_;
      oss << std::setw(22) << std::setprecision(5) << std::scientific << normrhsfluidpres_;
      break;
    case INPAR::POROELAST::convnorm_abs_global:
      break;
    default:
      FOUR_C_THROW("Unknown or undefined convergence form for single field residual.");
      break;
  }

  switch (normtypeinc_)
  {
    case INPAR::POROELAST::convnorm_abs_singlefields:
      oss << std::setw(22) << std::setprecision(5) << std::scientific << normincstruct_;
      if (porosity_dof_)
        oss << std::setw(22) << std::setprecision(5) << std::scientific << normincporo_;
      oss << std::setw(22) << std::setprecision(5) << std::scientific << normincfluidvel_;
      oss << std::setw(22) << std::setprecision(5) << std::scientific << normincfluidpres_;
      break;
    case INPAR::POROELAST::convnorm_abs_global:
      break;
    default:
      FOUR_C_THROW("Unknown or undefined convergence form for single field increment.");
      break;
  }
}

void POROELAST::Monolithic::print_newton_conv() {}

void POROELAST::Monolithic::apply_str_coupl_matrix(Teuchos::RCP<CORE::LINALG::SparseOperator> k_sf)
{
  k_sf->Zero();

  // create the parameters for the discretization
  Teuchos::ParameterList sparams;

  if (oldstructimint_)
  {
    const std::string action = "struct_poro_calc_fluidcoupling";
    sparams.set("action", action);
    // other parameters that might be needed by the elements
    sparams.set("delta time", Dt());
    sparams.set("total time", Time());
  }
  else
  {
    //! pointer to the model evaluator data container
    Teuchos::RCP<DRT::ELEMENTS::ParamsMinimal> params =
        Teuchos::rcp(new DRT::ELEMENTS::ParamsMinimal());

    // set parameters needed for element evalutation
    params->SetActionType(DRT::ELEMENTS::struct_poro_calc_fluidcoupling);
    params->SetTotalTime(Time());
    params->SetDeltaTime(Dt());

    sparams.set<Teuchos::RCP<DRT::ELEMENTS::ParamsInterface>>("interface", params);
  }

  structure_field()->discretization()->ClearState();
  structure_field()->discretization()->set_state(0, "displacement", structure_field()->Dispnp());
  structure_field()->discretization()->set_state(0, "velocity", structure_field()->Velnp());

  // build specific assemble strategy for mechanical-fluid system matrix
  // from the point of view of structure_field:
  // structdofset = 0, fluiddofset = 1
  CORE::FE::AssembleStrategy structuralstrategy(0,  // structdofset for row
      1,                                            // fluiddofset for column
      k_sf,                                         // mechanical-fluid coupling matrix
      Teuchos::null, Teuchos::null, Teuchos::null, Teuchos::null);

  // evaluate the mechanical-fluid system matrix on the structural element
  structure_field()->discretization()->evaluate_condition(
      sparams, structuralstrategy, "PoroCoupling");
  // structure_field()->discretization()->Evaluate( sparams, structuralstrategy);
  structure_field()->discretization()->ClearState();

  // scale with time integration factor
  k_sf->Scale(1.0 - structure_field()->TimIntParam());
}

void POROELAST::Monolithic::apply_fluid_coupl_matrix(
    Teuchos::RCP<CORE::LINALG::SparseOperator> k_fs)
{
  k_fs->Zero();

  // create the parameters for the discretization
  Teuchos::ParameterList fparams;
  // action for elements
  fparams.set<int>("action", FLD::calc_porousflow_fluid_coupling);
  // physical type
  fparams.set<int>("Physical Type", fluid_field()->PhysicalType());
  // other parameters that might be needed by the elements
  fparams.set("delta time", Dt());
  fparams.set("total time", Time());

  fluid_field()->discretization()->ClearState();

  // set general vector values needed by elements
  fluid_field()->discretization()->set_state(0, "dispnp", fluid_field()->Dispnp());
  fluid_field()->discretization()->set_state(0, "dispn", fluid_field()->Dispn());
  fluid_field()->discretization()->set_state(0, "gridv", fluid_field()->GridVel());
  fluid_field()->discretization()->set_state(0, "gridvn", fluid_field()->GridVeln());
  fluid_field()->discretization()->set_state(0, "veln", fluid_field()->Veln());
  fluid_field()->discretization()->set_state(0, "accnp", fluid_field()->Accnp());
  fluid_field()->discretization()->set_state(0, "accam", fluid_field()->Accam());
  fluid_field()->discretization()->set_state(0, "accn", fluid_field()->Accn());

  fluid_field()->discretization()->set_state(0, "scaaf", fluid_field()->Scaaf());

  fluid_field()->discretization()->set_state(0, "hist", fluid_field()->Hist());

  // set scheme-specific element parameters and vector values
  if (fluid_field()->TimIntScheme() == INPAR::FLUID::timeint_npgenalpha or
      fluid_field()->TimIntScheme() == INPAR::FLUID::timeint_npgenalpha)
    fluid_field()->discretization()->set_state(0, "velaf", fluid_field()->Velaf());
  else
    fluid_field()->discretization()->set_state(0, "velaf", fluid_field()->Velnp());

  fluid_field()->discretization()->set_state(0, "velnp", fluid_field()->Velnp());

  // build specific assemble strategy for the fluid-mechanical system matrix
  // from the point of view of fluid_field:
  // fluiddofset = 0, structdofset = 1
  CORE::FE::AssembleStrategy fluidstrategy(0,  // fluiddofset for row
      1,                                       // structdofset for column
      k_fs,                                    // fluid-mechanical matrix
      Teuchos::null,                           // no other matrix or vectors
      Teuchos::null, Teuchos::null, Teuchos::null);

  // evaluate the fluid-mechanical system matrix on the fluid element
  fluid_field()->discretization()->evaluate_condition(fparams, fluidstrategy, "PoroCoupling");
  // fluid_field()->discretization()->Evaluate( fparams, fluidstrategy );

  // evaluate coupling terms from partial integration of continuity equation
  if (part_int_cond_)
  {
    // create the parameters for the discretization
    Teuchos::ParameterList params;
    // action for elements
    params.set<int>("action", FLD::poro_boundary);
    params.set("total time", Time());
    params.set("delta time", Dt());
    params.set<POROELAST::Coupltype>("coupling", POROELAST::fluidstructure);
    params.set("timescale", fluid_field()->residual_scaling());
    params.set<int>("Physical Type", fluid_field()->PhysicalType());

    fluid_field()->discretization()->ClearState();
    fluid_field()->discretization()->set_state(0, "dispnp", fluid_field()->Dispnp());
    fluid_field()->discretization()->set_state(0, "gridv", fluid_field()->GridVel());
    fluid_field()->discretization()->set_state(0, "velnp", fluid_field()->Velnp());
    fluid_field()->discretization()->set_state(0, "scaaf", fluid_field()->Scaaf());

    fluid_field()->discretization()->evaluate_condition(params, fluidstrategy, "PoroPartInt");

    fluid_field()->discretization()->ClearState();
  }
  if (pres_int_cond_)
  {
    // create the parameters for the discretization
    Teuchos::ParameterList params;
    // action for elements
    params.set<int>("action", FLD::poro_prescoupl);
    params.set<POROELAST::Coupltype>("coupling", POROELAST::fluidstructure);
    params.set<int>("Physical Type", fluid_field()->PhysicalType());

    fluid_field()->discretization()->ClearState();
    fluid_field()->discretization()->set_state(0, "dispnp", fluid_field()->Dispnp());
    fluid_field()->discretization()->set_state(0, "velnp", fluid_field()->Velnp());

    fluid_field()->discretization()->evaluate_condition(params, fluidstrategy, "PoroPresInt");

    fluid_field()->discretization()->ClearState();
  }

  // apply normal flux condition on coupling part
  if (nopen_handle_->HasCond())
  {
    k_fs->Complete(
        structure_field()->SystemMatrix()->RangeMap(), fluid_field()->SystemMatrix()->RangeMap());
    evaluate_condition(k_fs, POROELAST::fluidstructure);
  }

  fluid_field()->discretization()->ClearState();
}

[[maybe_unused]] void POROELAST::Monolithic::PoroFDCheck()
{
  std::cout << "\n******************finite difference check***************" << std::endl;

  int dof_struct = (DofRowMapStructure()->NumGlobalElements());
  int dof_fluid = (DofRowMapFluid()->NumGlobalElements());

  std::cout << "structure field has " << dof_struct << " DOFs" << std::endl;
  std::cout << "fluid field has " << dof_fluid << " DOFs" << std::endl;

  Teuchos::RCP<Epetra_Vector> iterinc = Teuchos::null;
  Teuchos::RCP<Epetra_Vector> abs_iterinc = Teuchos::null;
  iterinc = CORE::LINALG::CreateVector(*dof_row_map(), true);
  abs_iterinc = CORE::LINALG::CreateVector(*dof_row_map(), true);

  const int dofs = iterinc->GlobalLength();
  std::cout << "in total " << dofs << " DOFs" << std::endl;
  const double delta = 1e-8;

  iterinc->PutScalar(0.0);

  iterinc->ReplaceGlobalValue(0, 0, delta);

  abs_iterinc->Update(1.0, *iterinc_, 0.0);

  Teuchos::RCP<Epetra_CrsMatrix> stiff_approx = Teuchos::null;
  stiff_approx = CORE::LINALG::CreateMatrix(*dof_row_map(), 81);

  Teuchos::RCP<Epetra_Vector> rhs_old = Teuchos::rcp(new Epetra_Vector(*dof_row_map(), true));
  rhs_old->Update(1.0, *rhs_, 0.0);
  Teuchos::RCP<Epetra_Vector> rhs_copy = Teuchos::rcp(new Epetra_Vector(*dof_row_map(), true));

  Teuchos::RCP<CORE::LINALG::SparseMatrix> sparse = systemmatrix_->Merge();
  Teuchos::RCP<CORE::LINALG::SparseMatrix> sparse_copy =
      Teuchos::rcp(new CORE::LINALG::SparseMatrix(sparse->EpetraMatrix(), CORE::LINALG::Copy));

  bool output = false;
  if (output)
  {
    std::cout << "iterinc_" << std::endl << *iterinc_ << std::endl;
    std::cout << "iterinc" << std::endl << *iterinc << std::endl;
    std::cout << "meshdisp: " << std::endl << *(fluid_field()->Dispnp());
    std::cout << "disp: " << std::endl << *(structure_field()->Dispnp());
    std::cout << "fluid vel" << std::endl << *(fluid_field()->Velnp());
    std::cout << "fluid acc" << std::endl << *(fluid_field()->Accnp());
    std::cout << "gridvel fluid" << std::endl << *(fluid_field()->GridVel());
    std::cout << "gridvel struct" << std::endl << *(structure_field()->Velnp());
  }

  const int row_number = -1;
  const int column_number = -1;
  for (int i = 0; i < dofs; ++i)
  {
    if (combined_dbc_map()->MyGID(i))
    {
      iterinc->ReplaceGlobalValue(i, 0, 0.0);
    }
    abs_iterinc->Update(1.0, *iterinc, 1.0);

    if (i == column_number)
      std::cout << "\n******************" << column_number + 1 << ". Spalte!!***************"
                << std::endl;

    Evaluate(iterinc, iter_ == 1);

    rhs_copy->Update(1.0, *rhs_, 0.0);

    iterinc_->PutScalar(0.0);  // Useful? depends on solver and more
    CORE::LINALG::apply_dirichlet_to_system(
        *sparse_copy, *iterinc_, *rhs_copy, *zeros_, *combined_dbc_map());


    if (i == column_number)
    {
      std::cout << "rhs_: " << (*rhs_copy)[row_number] << std::endl;
      std::cout << "rhs_old: " << (*rhs_old)[row_number] << std::endl;
    }

    rhs_copy->Update(-1.0, *rhs_old, 1.0);
    rhs_copy->Scale(-1.0 / delta);

    int* index = &i;
    for (int j = 0; j < dofs; ++j)
    {
      double value = (*rhs_copy)[j];
      stiff_approx->InsertGlobalValues(j, 1, &value, index);

      if ((j == row_number) and (i == column_number))
      {
        std::cout << "\n******************" << row_number + 1 << ". Row!!***************"
                  << std::endl;
        std::cout << "iterinc_" << std::endl << *iterinc_ << std::endl;
        std::cout << "iterinc" << std::endl << *iterinc << std::endl;
        std::cout << "meshdisp: " << std::endl << *(fluid_field()->Dispnp());
        std::cout << "disp: " << std::endl << *(structure_field()->Dispnp());
        std::cout << "fluid vel" << std::endl << *(fluid_field()->Velnp());
        std::cout << "fluid acc" << std::endl << *(fluid_field()->Accnp());
        std::cout << "gridvel fluid" << std::endl << *(fluid_field()->GridVel());
        std::cout << "gridvel struct" << std::endl << *(structure_field()->Velnp());

        std::cout << "stiff_apprx(" << row_number << "," << column_number
                  << "): " << (*rhs_copy)[row_number] << std::endl;

        std::cout << "value(" << row_number << "," << column_number << "): " << value << std::endl;
        std::cout << "\n******************" << row_number + 1 << ". Row End!!***************"
                  << std::endl;
      }
    }

    if (not combined_dbc_map()->MyGID(i)) iterinc->ReplaceGlobalValue(i, 0, -delta);

    iterinc->ReplaceGlobalValue(i - 1, 0, 0.0);

    if (i != dofs - 1) iterinc->ReplaceGlobalValue(i + 1, 0, delta);

    if (i == column_number)
      std::cout << "\n******************" << column_number + 1 << ". Column End!!***************"
                << std::endl;
  }

  Evaluate(iterinc, iter_ == 1);

  stiff_approx->FillComplete();

  Teuchos::RCP<CORE::LINALG::SparseMatrix> stiff_approx_sparse = Teuchos::null;
  stiff_approx_sparse =
      Teuchos::rcp(new CORE::LINALG::SparseMatrix(stiff_approx, CORE::LINALG::Copy));

  stiff_approx_sparse->Add(*sparse_copy, false, -1.0, 1.0);

  Teuchos::RCP<Epetra_CrsMatrix> sparse_crs = sparse_copy->EpetraMatrix();

  Teuchos::RCP<Epetra_CrsMatrix> error_crs = stiff_approx_sparse->EpetraMatrix();

  error_crs->FillComplete();
  sparse_crs->FillComplete();

  bool success = true;
  double error_max = 0.0;
  double abs_error_max = 0.0;
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

          if (abs(error) > abs(error_max)) error_max = abs(error);
          if (abs(error_ij) > abs(abs_error_max)) abs_error_max = abs(error_ij);

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
    std::cout << "finite difference check successful, max. rel. error: " << error_max
              << "  (max. abs. error: " << abs_error_max << ")" << std::endl;
    std::cout << "******************finite difference check done***************\n\n" << std::endl;
  }
  else
    FOUR_C_THROW("PoroFDCheck failed in step: %d, iter: %d", Step(), iter_);
}

void POROELAST::Monolithic::evaluate_condition(
    Teuchos::RCP<CORE::LINALG::SparseOperator> Sysmat, POROELAST::Coupltype coupltype)
{
  nopen_handle_->Clear(coupltype);
  Teuchos::RCP<CORE::LINALG::SparseMatrix> ConstraintMatrix =
      nopen_handle_->ConstraintMatrix(coupltype);
  Teuchos::RCP<CORE::LINALG::SparseMatrix> struct_vel_constraint_matrix =
      nopen_handle_->struct_vel_constraint_matrix(coupltype);

  // evaluate condition on elements and assemble matrices
  fluid_field()->evaluate_no_penetration_cond(nopen_handle_->RHS(), ConstraintMatrix,
      struct_vel_constraint_matrix, nopen_handle_->CondVector(), nopen_handle_->CondIDs(),
      coupltype);

  if (coupltype == POROELAST::fluidfluid)  // fluid fluid part
  {
    ConstraintMatrix->Complete();
    nopen_handle_->buid_no_penetration_map(
        fluid_field()->discretization()->Comm(), fluid_field()->dof_row_map());
  }
  else  // fluid structure part
  {
    // double timescale = fluid_field()->TimeScaling();
    double timescale = fluid_field()->residual_scaling();
    struct_vel_constraint_matrix->Scale(timescale);
    struct_vel_constraint_matrix->Complete(
        structure_field()->SystemMatrix()->RangeMap(), fluid_field()->SystemMatrix()->RangeMap());
    ConstraintMatrix->Add(*struct_vel_constraint_matrix, false, 1.0, 1.0);
    ConstraintMatrix->Complete(
        structure_field()->SystemMatrix()->RangeMap(), fluid_field()->SystemMatrix()->RangeMap());
  }

  const Teuchos::RCP<const Epetra_Map>& nopenetrationmap = nopen_handle_->Extractor()->Map(1);
  const Teuchos::RCP<const Epetra_Map>& othermap = nopen_handle_->Extractor()->Map(0);
  ConstraintMatrix->ApplyDirichlet(*othermap, false);
  Sysmat->ApplyDirichlet(*nopenetrationmap, false);
  Sysmat->UnComplete();
  Sysmat->Add(*ConstraintMatrix, false, 1.0, 1.0);
}

Teuchos::RCP<CORE::LINALG::SparseMatrix> POROELAST::Monolithic::struct_fluid_coupling_matrix()
{
  Teuchos::RCP<CORE::LINALG::SparseMatrix> sparse =
      Teuchos::rcp_dynamic_cast<CORE::LINALG::SparseMatrix>(k_sf_);
  if (sparse == Teuchos::null) FOUR_C_THROW("cast to CORE::LINALG::SparseMatrix failed!");

  return sparse;
}

Teuchos::RCP<CORE::LINALG::SparseMatrix> POROELAST::Monolithic::fluid_struct_coupling_matrix()
{
  Teuchos::RCP<CORE::LINALG::SparseMatrix> sparse =
      Teuchos::rcp_dynamic_cast<CORE::LINALG::SparseMatrix>(k_fs_);
  if (sparse == Teuchos::null) FOUR_C_THROW("cast to CORE::LINALG::SparseMatrix failed!");

  return sparse;
}

Teuchos::RCP<CORE::LINALG::BlockSparseMatrixBase>
POROELAST::Monolithic::struct_fluid_coupling_block_matrix()
{
  Teuchos::RCP<CORE::LINALG::BlockSparseMatrixBase> blocksparse =
      Teuchos::rcp_dynamic_cast<CORE::LINALG::BlockSparseMatrixBase>(k_sf_);
  if (blocksparse == Teuchos::null)
    FOUR_C_THROW("cast to CORE::LINALG::BlockSparseMatrixBase failed!");

  return blocksparse;
}


Teuchos::RCP<CORE::LINALG::BlockSparseMatrixBase>
POROELAST::Monolithic::fluid_struct_coupling_block_matrix()
{
  Teuchos::RCP<CORE::LINALG::BlockSparseMatrixBase> blocksparse =
      Teuchos::rcp_dynamic_cast<CORE::LINALG::BlockSparseMatrixBase>(k_fs_);
  if (blocksparse == Teuchos::null)
    FOUR_C_THROW("cast to CORE::LINALG::BlockSparseMatrixBase failed!");

  return blocksparse;
}

void POROELAST::Monolithic::SetupNewton()
{
  // initialise equilibrium loop and norms
  iter_ = 1;
  normrhs_ = 0.0;
  norminc_ = 0.0;
  normrhsfluid_ = 0.0;
  normincfluid_ = 0.0;
  normrhsfluidvel_ = 0.0;
  normincfluidvel_ = 0.0;
  normrhsfluidpres_ = 0.0;
  normincfluidpres_ = 0.0;
  normrhsstruct_ = 0.0;
  normincstruct_ = 0.0;
  normrhsporo_ = 0.0;
  normincporo_ = 0.0;

  // incremental solution vector with length of all dofs
  if (iterinc_ == Teuchos::null)
    iterinc_ = CORE::LINALG::CreateVector(*dof_row_map(), true);
  else
    iterinc_->PutScalar(0.0);

  // a zero vector of full length
  if (zeros_ == Teuchos::null)
    zeros_ = CORE::LINALG::CreateVector(*dof_row_map(), true);
  else
    zeros_->PutScalar(0.0);

  // AitkenReset();
}

void POROELAST::Monolithic::build_convergence_norms()
{
  //------------------------------------------------------------ build residual force norms
  normrhs_ = UTILS::calculate_vector_norm(vectornormfres_, rhs_);
  Teuchos::RCP<const Epetra_Vector> rhs_s;
  Teuchos::RCP<const Epetra_Vector> rhs_f;
  Teuchos::RCP<const Epetra_Vector> rhs_fvel;
  Teuchos::RCP<const Epetra_Vector> rhs_fpres;

  // process structure unknowns of the first field
  rhs_s = Extractor()->ExtractVector(rhs_, 0);
  // process fluid unknowns of the second field
  rhs_f = Extractor()->ExtractVector(rhs_, 1);
  rhs_fvel = fluid_field()->ExtractVelocityPart(rhs_f);
  rhs_fpres = fluid_field()->ExtractPressurePart(rhs_f);

  if (porosity_dof_)
  {
    Teuchos::RCP<const Epetra_Vector> rhs_poro = porosity_splitter_->ExtractCondVector(rhs_s);
    Teuchos::RCP<const Epetra_Vector> rhs_sdisp = porosity_splitter_->ExtractOtherVector(rhs_s);

    normrhsstruct_ = UTILS::calculate_vector_norm(vectornormfres_, rhs_sdisp);
    normrhsporo_ = UTILS::calculate_vector_norm(vectornormfres_, rhs_poro);
  }
  else
    normrhsstruct_ = UTILS::calculate_vector_norm(vectornormfres_, rhs_s);

  normrhsfluid_ = UTILS::calculate_vector_norm(vectornormfres_, rhs_f);
  normrhsfluidvel_ = UTILS::calculate_vector_norm(vectornormfres_, rhs_fvel);
  normrhsfluidpres_ = UTILS::calculate_vector_norm(vectornormfres_, rhs_fpres);


  //------------------------------------------------------------- build residual increment norms
  iterinc_->Norm2(&norminc_);

  // displacement and fluid velocity & pressure incremental vector
  Teuchos::RCP<const Epetra_Vector> interincs;
  Teuchos::RCP<const Epetra_Vector> interincf;
  Teuchos::RCP<const Epetra_Vector> interincfvel;
  Teuchos::RCP<const Epetra_Vector> interincfpres;
  // process structure unknowns of the first field
  interincs = Extractor()->ExtractVector(iterinc_, 0);
  // process fluid unknowns of the second field
  interincf = Extractor()->ExtractVector(iterinc_, 1);
  interincfvel = fluid_field()->ExtractVelocityPart(interincf);
  interincfpres = fluid_field()->ExtractPressurePart(interincf);

  if (porosity_dof_)
  {
    Teuchos::RCP<const Epetra_Vector> interincporo =
        porosity_splitter_->ExtractCondVector(interincs);
    Teuchos::RCP<const Epetra_Vector> interincsdisp =
        porosity_splitter_->ExtractOtherVector(interincs);

    normincstruct_ = UTILS::calculate_vector_norm(vectornorminc_, interincsdisp);
    normincporo_ = UTILS::calculate_vector_norm(vectornorminc_, interincporo);
  }
  else
    normincstruct_ = UTILS::calculate_vector_norm(vectornorminc_, interincs);

  normincfluid_ = UTILS::calculate_vector_norm(vectornorminc_, interincf);
  normincfluidvel_ = UTILS::calculate_vector_norm(vectornorminc_, interincfvel);
  normincfluidpres_ = UTILS::calculate_vector_norm(vectornorminc_, interincfpres);
}

bool POROELAST::Monolithic::SetupSolver()
{
  //  solver
  // create a linear solver
  // get dynamic section of poroelasticity
  const Teuchos::ParameterList& poroelastdyn =
      GLOBAL::Problem::Instance()->poroelast_dynamic_params();
  // get the solver number used for linear poroelasticity solver
  const int linsolvernumber = poroelastdyn.get<int>("LINEAR_SOLVER");
  // check if the poroelasticity solver has a valid solver number
  if (linsolvernumber == (-1))
  {
    FOUR_C_THROW(
        "no linear solver defined for poroelasticity. Please set LINEAR_SOLVER in POROELASTICITY "
        "DYNAMIC to a valid number!");
  }
  const Teuchos::ParameterList& solverparams =
      GLOBAL::Problem::Instance()->SolverParams(linsolvernumber);
  const auto solvertype =
      Teuchos::getIntegralValue<CORE::LINEAR_SOLVER::SolverType>(solverparams, "SOLVER");

  directsolve_ = (solvertype == CORE::LINEAR_SOLVER::SolverType::umfpack or
                  solvertype == CORE::LINEAR_SOLVER::SolverType::superlu);

  if (directsolve_)
  {
    solver_ = Teuchos::rcp(new CORE::LINALG::Solver(solverparams, Comm()));
  }
  else
    // create a linear solver
    create_linear_solver();

  // Get the parameters for the Newton iteration
  itermax_ = poroelastdyn.get<int>("ITEMAX");
  itermin_ = poroelastdyn.get<int>("ITEMIN");
  normtypeinc_ = CORE::UTILS::IntegralValue<INPAR::POROELAST::ConvNorm>(poroelastdyn, "NORM_INC");
  normtypefres_ = CORE::UTILS::IntegralValue<INPAR::POROELAST::ConvNorm>(poroelastdyn, "NORM_RESF");
  combincfres_ =
      CORE::UTILS::IntegralValue<INPAR::POROELAST::BinaryOp>(poroelastdyn, "NORMCOMBI_RESFINC");
  vectornormfres_ =
      CORE::UTILS::IntegralValue<INPAR::POROELAST::VectorNorm>(poroelastdyn, "VECTORNORM_RESF");
  vectornorminc_ =
      CORE::UTILS::IntegralValue<INPAR::POROELAST::VectorNorm>(poroelastdyn, "VECTORNORM_INC");

  tolinc_ = poroelastdyn.get<double>("TOLINC_GLOBAL");
  tolfres_ = poroelastdyn.get<double>("TOLRES_GLOBAL");

  tolinc_struct_ = poroelastdyn.get<double>("TOLINC_DISP");
  tolinc_velocity_ = poroelastdyn.get<double>("TOLINC_VEL");
  tolinc_pressure_ = poroelastdyn.get<double>("TOLINC_PRES");
  tolinc_porosity_ = poroelastdyn.get<double>("TOLINC_PORO");

  tolfres_struct_ = poroelastdyn.get<double>("TOLRES_DISP");
  tolfres_velocity_ = poroelastdyn.get<double>("TOLRES_VEL");
  tolfres_pressure_ = poroelastdyn.get<double>("TOLRES_PRES");
  tolfres_porosity_ = poroelastdyn.get<double>("TOLRES_PORO");

  return true;
}

Teuchos::RCP<CORE::LINALG::SparseMatrix> POROELAST::Monolithic::SystemMatrix()
{
  return systemmatrix_->Merge();
}

void POROELAST::Monolithic::IncrementPoroIter() { iter_ += 1; }

void POROELAST::Monolithic::UpdatePoroIterinc(Teuchos::RCP<const Epetra_Vector> poroinc)
{
  iterinc_->PutScalar(0.0);
  iterinc_->Update(1.0, *poroinc, 0.0);
}

void POROELAST::Monolithic::ClearPoroIterinc() { iterinc_->PutScalar(0.0); }

void POROELAST::Monolithic::aitken()
{
  // initialise increment vector with solution of last iteration (i)
  // update del_ with current residual vector
  // difference of last two solutions
  if (del_ == Teuchos::null)  // first iteration, itnum==1
  {
    del_ = CORE::LINALG::CreateVector(*dof_row_map(), true);
    delhist_ = CORE::LINALG::CreateVector(*dof_row_map(), true);
    del_->PutScalar(1.0e20);
    delhist_->PutScalar(0.0);
  }

  // calculate difference of current (i+1) and old (i) residual vector
  // delhist = ( r^{i+1}_{n+1} - r^i_{n+1} )
  // update history vector old increment r^i_{n+1}
  delhist_->Update(1.0, *del_, 0.0);         // r^i_{n+1}
  delhist_->Update(1.0, *iterinc_, (-1.0));  // update r^{i+1}_{n+1}


  // del_ = r^{i+1}_{n+1} = T^{i+1}_{n+1} - T^{i}_{n+1}
  del_->Update(1.0, *iterinc_, 0.0);
  // den = |r^{i+1} - r^{i}|^2
  double den = 0.0;
  delhist_->Norm2(&den);
  // calculate dot product
  // dot = delhist_ . del_ = ( r^{i+1}_{n+1} - r^i_{n+1} )^T . r^{i+1}_{n+1}
  double top = 0.0;
  delhist_->Dot(*del_, &top);

  // mu_: Aikten factor in Mok's version
  // mu_: relaxation parameter in Irons & Tuck
  // mu^{i+1} = mu^i + (mu^i -1) . (r^{i+1} - r^i)^T . (-r^{i+1}) / |r^{i+1} - r^{i}|^2
  // top = ( r^{i+1} - r^i )^T . r^{i+1} --> use -top
  // Uli's implementation: mu_ = mu_ + (mu_ - 1.0) * top / (den*den). with '-' included in top
  mu_ = mu_ + (mu_ - 1) * (-top) / (den * den);

  iterinc_->Scale(1.0 - mu_);
}

[[maybe_unused]] void POROELAST::Monolithic::aitken_reset()
{
  if (del_ == Teuchos::null)  // first iteration, itnum==1
  {
    del_ = CORE::LINALG::CreateVector(*dof_row_map(), true);
    delhist_ = CORE::LINALG::CreateVector(*dof_row_map(), true);
  }
  del_->PutScalar(1.0e20);
  delhist_->PutScalar(0.0);
  mu_ = 0.0;
}

const Epetra_Map& POROELAST::Monolithic::FluidRangeMap()
{
  return fluid_field()->SystemMatrix()->RangeMap();
}

const Epetra_Map& POROELAST::Monolithic::FluidDomainMap()
{
  return fluid_field()->SystemMatrix()->DomainMap();
}

const Epetra_Map& POROELAST::Monolithic::StructureDomainMap()
{
  return structure_field()->DomainMap();
}

Teuchos::RCP<const Epetra_Map> POROELAST::Monolithic::dof_row_map()
{
  return blockrowdofmap_->FullMap();
}

Teuchos::RCP<const Epetra_Map> POROELAST::Monolithic::DofRowMapStructure()
{
  return blockrowdofmap_->Map(0);
}

Teuchos::RCP<const Epetra_Map> POROELAST::Monolithic::DofRowMapFluid()
{
  return blockrowdofmap_->Map(1);
}

void POROELAST::Monolithic::recover_lagrange_multiplier_after_newton_step(
    Teuchos::RCP<const Epetra_Vector> iterinc)
{
  // clean up as soon as old time integration is unused!
  if (oldstructimint_)
  {
    if (structure_field()->meshtying_contact_bridge() != Teuchos::null)
    {
      if (structure_field()->meshtying_contact_bridge()->HaveContact() && !nit_contact_)
      {
        // Recover structural contact lagrange multiplier !!! For Poro & FPSI Problems this is
        // deactivated in the Structure!!!

        CONTACT::LagrangeStrategyPoro& costrategy = static_cast<CONTACT::LagrangeStrategyPoro&>(
            structure_field()->meshtying_contact_bridge()->ContactManager()->GetStrategy());

        // displacement and fluid velocity & pressure incremental vector
        Teuchos::RCP<const Epetra_Vector> s_iterinc;
        Teuchos::RCP<const Epetra_Vector> f_iterinc;
        extract_field_vectors(iterinc, s_iterinc, f_iterinc);

        // RecoverStructuralLM
        Teuchos::RCP<Epetra_Vector> tmpsx =
            Teuchos::rcp<Epetra_Vector>(new Epetra_Vector(*s_iterinc));
        Teuchos::RCP<Epetra_Vector> tmpfx =
            Teuchos::rcp<Epetra_Vector>(new Epetra_Vector(*f_iterinc));

        costrategy.RecoverCoupled(tmpsx, tmpfx);
        if (no_penetration_) costrategy.RecoverPoroNoPen(tmpsx, tmpfx);
      }
      else if (structure_field()->meshtying_contact_bridge()->HaveMeshtying())
      {  // if meshtying  //h.Willmann

        CONTACT::PoroMtLagrangeStrategy& costrategy = static_cast<CONTACT::PoroMtLagrangeStrategy&>(
            structure_field()->meshtying_contact_bridge()->MtManager()->GetStrategy());

        // displacement and fluid velocity & pressure incremental vector
        Teuchos::RCP<const Epetra_Vector> s_iterinc;
        Teuchos::RCP<const Epetra_Vector> f_iterinc;
        extract_field_vectors(iterinc, s_iterinc, f_iterinc);

        Teuchos::RCP<Epetra_Vector> tmpfx =
            Teuchos::rcp<Epetra_Vector>(new Epetra_Vector(*f_iterinc));

        // Recover part of LM stemming from offdiagonal coupling matrix
        costrategy.recover_coupling_matrix_partof_lmp(tmpfx);
      }
    }
  }
}

void POROELAST::Monolithic::set_poro_contact_states()
{
  // clean up as soon as old time integration is unused!
  if (oldstructimint_)
  {
    if (structure_field()->meshtying_contact_bridge() != Teuchos::null)
    {
      if (structure_field()->meshtying_contact_bridge()->HaveContact())
      {
        if (!nit_contact_)  // Lagmult contact
        {
          CONTACT::LagrangeStrategyPoro& costrategy = static_cast<CONTACT::LagrangeStrategyPoro&>(
              structure_field()->meshtying_contact_bridge()->ContactManager()->GetStrategy());
          Teuchos::RCP<Epetra_Vector> fvel = Teuchos::rcp(
              new Epetra_Vector(*fluid_field()->ExtractVelocityPart(fluid_field()->Velnp())));
          fvel = fluid_structure_coupling().SlaveToMaster(fvel);
          costrategy.set_state(MORTAR::state_fvelocity, *fvel);

          // To get pressure dofs into first structural component!!! - any idea for nice
          // implementation?
          Teuchos::RCP<const Epetra_Vector> fpres =
              fluid_field()->ExtractPressurePart(fluid_field()->Velnp());
          Teuchos::RCP<Epetra_Vector> modfpres =
              Teuchos::rcp(new Epetra_Vector(*fluid_field()->VelocityRowMap(), true));

          int* mygids = fpres->Map().MyGlobalElements();
          double* val = fpres->Values();
          const int ndim = GLOBAL::Problem::Instance()->NDim();
          for (int i = 0; i < fpres->MyLength(); ++i)
          {
            int gid = mygids[i] - ndim;
            modfpres->ReplaceGlobalValues(1, &val[i], &gid);
          }

          modfpres = fluid_structure_coupling().SlaveToMaster(modfpres);
          costrategy.set_state(MORTAR::state_fpressure, *modfpres);

          Teuchos::RCP<Epetra_Vector> dis =
              Teuchos::rcp(new Epetra_Vector(*structure_field()->Dispnp()));
          costrategy.SetParentState("displacement", dis,
              structure_field()->discretization());  // add displacements of the parent element!!!
        }
        else
        {
          CONTACT::NitscheStrategyPoro& costrategy = dynamic_cast<CONTACT::NitscheStrategyPoro&>(
              structure_field()->meshtying_contact_bridge()->ContactManager()->GetStrategy());
          costrategy.SetParentState(MORTAR::state_fvelocity, *fluid_field()->Velnp());
        }
      }
    }
  }
}

void POROELAST::Monolithic::eval_poro_mortar()
{
  // clean up as soon as old time integration is unused!
  if (oldstructimint_)
  {
    if (structure_field()->meshtying_contact_bridge() != Teuchos::null)
    {
      if (structure_field()->meshtying_contact_bridge()->HaveContact())
      {
        if (!nit_contact_)  // Lagmult contact
        {
          CONTACT::LagrangeStrategyPoro& costrategy = static_cast<CONTACT::LagrangeStrategyPoro&>(
              structure_field()->meshtying_contact_bridge()->ContactManager()->GetStrategy());
          //---Modifiy coupling matrix k_sf

          // Get matrix block!
          Teuchos::RCP<CORE::LINALG::SparseOperator> k_ss =
              Teuchos::rcp<CORE::LINALG::SparseMatrix>(
                  new CORE::LINALG::SparseMatrix(systemmatrix_->Matrix(0, 0)));
          Teuchos::RCP<CORE::LINALG::SparseOperator> k_sf =
              Teuchos::rcp<CORE::LINALG::SparseMatrix>(
                  new CORE::LINALG::SparseMatrix(systemmatrix_->Matrix(0, 1)));
          Teuchos::RCP<Epetra_Vector> rhs_s = Extractor()->ExtractVector(rhs_, 0);

          // Evaluate Poro Contact Condensation for K_ss, K_sf
          costrategy.apply_force_stiff_cmt_coupled(
              structure_field()->WriteAccessDispnp(), k_ss, k_sf, rhs_s, Step(), iter_, false);

          // Assign modified matrixes & vectors
          systemmatrix_->Assign(0, 0, CORE::LINALG::Copy,
              *Teuchos::rcp_dynamic_cast<CORE::LINALG::SparseMatrix>(k_ss));
          systemmatrix_->Assign(0, 1, CORE::LINALG::Copy,
              *Teuchos::rcp_dynamic_cast<CORE::LINALG::SparseMatrix>(k_sf));
          Extractor()->InsertVector(rhs_s, 0, rhs_);

          //---Modify fluid matrix, coupling matrix k_fs and rhs of fluid
          if (no_penetration_)
          {
            //---Initialize Poro Contact
            costrategy.PoroInitialize(fluid_structure_coupling(),
                fluid_field()->dof_row_map());  // true stands for the no_penetration condition !!!
            // Get matrix blocks & rhs vector!
            Teuchos::RCP<CORE::LINALG::SparseMatrix> f = Teuchos::rcp<CORE::LINALG::SparseMatrix>(
                new CORE::LINALG::SparseMatrix(systemmatrix_->Matrix(1, 1)));
            Teuchos::RCP<CORE::LINALG::SparseMatrix> k_fs =
                Teuchos::rcp<CORE::LINALG::SparseMatrix>(
                    new CORE::LINALG::SparseMatrix(systemmatrix_->Matrix(1, 0)));

            Teuchos::RCP<Epetra_Vector> frhs = Extractor()->ExtractVector(rhs_, 1);

            // Evaluate Poro No Penetration Contact Condensation
            costrategy.evaluate_poro_no_pen_contact(k_fs, f, frhs);

            // Assign modified matrixes & vectors
            systemmatrix_->Assign(1, 1, CORE::LINALG::Copy, *f);
            systemmatrix_->Assign(1, 0, CORE::LINALG::Copy, *k_fs);

            Extractor()->InsertVector(*frhs, 1, *rhs_);
          }
        }
        else  // add nitsche contributions to sysmat and rhs
        {
          CONTACT::NitscheStrategyPoro* pstrat = dynamic_cast<CONTACT::NitscheStrategyPoro*>(
              &structure_field()->meshtying_contact_bridge()->ContactManager()->GetStrategy());

          systemmatrix_->UnComplete();
          systemmatrix_->Matrix(0, 1).Add(
              *pstrat->GetMatrixBlockPtr(CONTACT::MatBlockType::displ_porofluid), false, 1.0, 1.0);
          systemmatrix_->Matrix(1, 1).Add(
              *pstrat->GetMatrixBlockPtr(CONTACT::MatBlockType::porofluid_porofluid), false, 1.0,
              1.0);
          systemmatrix_->Matrix(1, 0).Add(
              *pstrat->GetMatrixBlockPtr(CONTACT::MatBlockType::porofluid_displ), false, 1.0, 1.0);
          systemmatrix_->Complete();

          Extractor()->AddVector(
              *pstrat->GetRhsBlockPtr(CONTACT::VecBlockType::porofluid), 1, *rhs_);
        }
      }
      else if (structure_field()->meshtying_contact_bridge()->HaveMeshtying())
      {  // if meshtying  //h.Willmann

        CONTACT::PoroMtLagrangeStrategy& costrategy = static_cast<CONTACT::PoroMtLagrangeStrategy&>(
            structure_field()->meshtying_contact_bridge()->MtManager()->GetStrategy());


        //---Modifiy coupling matrix k_sf

        // Get matrix block!
        Teuchos::RCP<CORE::LINALG::SparseMatrix> k_sf = Teuchos::rcp<CORE::LINALG::SparseMatrix>(
            new CORE::LINALG::SparseMatrix(systemmatrix_->Matrix(0, 1)));

        // initialize poro meshtying
        costrategy.InitializePoroMt(k_sf);

        // Evaluate Poro Meshtying Condensation for K_sf
        costrategy.evaluate_meshtying_poro_off_diag(k_sf);

        // Assign modified matrix
        systemmatrix_->Assign(0, 1, CORE::LINALG::Copy, *k_sf);
      }
    }
  }
}

void POROELAST::Monolithic::build_combined_dbc_map()
{
  const Teuchos::RCP<const Epetra_Map> scondmap =
      structure_field()->GetDBCMapExtractor()->CondMap();
  const Teuchos::RCP<const Epetra_Map> fcondmap = fluid_field()->GetDBCMapExtractor()->CondMap();
  combinedDBCMap_ = CORE::LINALG::MergeMap(scondmap, fcondmap, false);
}

void POROELAST::Monolithic::read_restart(const int step)
{
  // call base class
  POROELAST::PoroBase::read_restart(step);

  // set states for porous contact
  // apply current velocity of fluid to ContactMangager if contact problem
  if (no_penetration_) set_poro_contact_states();
}

FOUR_C_NAMESPACE_CLOSE