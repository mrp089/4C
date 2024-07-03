/*-----------------------------------------------------------*/
/*! \file

\brief Model evaluator for structure part of partitioned fsi


\level 3

*/
/*-----------------------------------------------------------*/


#include "4C_fsi_str_model_evaluator_partitioned.hpp"

#include "4C_linalg_utils_sparse_algebra_assemble.hpp"
#include "4C_solver_nonlin_nox_aux.hpp"
#include "4C_solver_nonlin_nox_group.hpp"
#include "4C_structure_new_dbc.hpp"
#include "4C_structure_new_impl_generic.hpp"
#include "4C_structure_new_model_evaluator.hpp"
#include "4C_structure_new_nln_solver_generic.hpp"
#include "4C_structure_new_timint_basedataglobalstate.hpp"
#include "4C_structure_new_timint_implicit.hpp"

#include <Epetra_Comm.h>

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Solid::MODELEVALUATOR::PartitionedFSI::PartitionedFSI()
    : interface_force_np_ptr_(Teuchos::null), is_relaxationsolve_(false)
{
  // empty
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void Solid::MODELEVALUATOR::PartitionedFSI::setup()
{
  // fsi interface force at t_{n+1}
  interface_force_np_ptr_ = Teuchos::rcp(new Epetra_Vector(*global_state().dof_row_map(), true));

  // set flag
  issetup_ = true;

  return;
}
/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void Solid::MODELEVALUATOR::PartitionedFSI::setup_multi_map_extractor()
{
  integrator().model_eval().setup_multi_map_extractor();
}
/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::RCP<const Epetra_Map> Solid::MODELEVALUATOR::PartitionedFSI::get_block_dof_row_map_ptr()
    const
{
  check_init_setup();
  return global_state().dof_row_map();
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::RCP<const Epetra_Vector> Solid::MODELEVALUATOR::PartitionedFSI::get_current_solution_ptr()
    const
{
  check_init();
  return global_state().get_dis_np();
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::RCP<const Epetra_Vector>
Solid::MODELEVALUATOR::PartitionedFSI::get_last_time_step_solution_ptr() const
{
  check_init();
  return global_state().get_dis_n();
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
bool Solid::MODELEVALUATOR::PartitionedFSI::assemble_force(
    Epetra_Vector& f, const double& timefac_np) const
{
  Core::LinAlg::AssembleMyVector(1.0, f, -timefac_np, *interface_force_np_ptr_);
  return true;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void Solid::MODELEVALUATOR::PartitionedFSI::update_step_state(const double& timefac_n)
{
  if (not is_relaxationsolve_)  // standard case
  {
    // add the old time factor scaled contributions to the residual
    Teuchos::RCP<Epetra_Vector>& fstructold_ptr = global_state().get_fstructure_old();
    fstructold_ptr->Update(-timefac_n, *interface_force_np_ptr_, 1.0);
  }
  else
  {
    // do nothing
  }
  return;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::RCP<const Epetra_Vector> Solid::MODELEVALUATOR::PartitionedFSI::solve_relaxation_linear(
    Teuchos::RCP<Adapter::Structure> structure)
{
  // print to screen
  if (global_state().dof_row_map()->Comm().MyPID() == 0)
    std::cout << "\n DO SRUCTURAL RELAXATION SOLVE ..." << std::endl;

  // cast adapter structure to implicit time integrator
  Teuchos::RCP<Solid::TimeInt::Implicit> ti_impl =
      Teuchos::rcp_dynamic_cast<Solid::TimeInt::Implicit>(structure, true);

  // get the nonlinear solver pointer
  Solid::Nln::SOLVER::Generic& nlnsolver =
      const_cast<Solid::Nln::SOLVER::Generic&>(*(ti_impl->get_nln_solver_ptr()));

  // get the solution group
  ::NOX::Abstract::Group& grp = nlnsolver.SolutionGroup();
  NOX::Nln::Group* grp_ptr = dynamic_cast<NOX::Nln::Group*>(&grp);
  if (grp_ptr == nullptr) FOUR_C_THROW("Dynamic cast failed!");

  // get nox parameter
  Teuchos::ParameterList& noxparams = ti_impl->data_sdyn().get_nox_params();

  // create new state vector
  Teuchos::RCP<::NOX::Epetra::Vector> x_ptr =
      global_state().create_global_vector(TimeInt::BaseDataGlobalState::VecInitType::last_time_step,
          ti_impl->impl_int_ptr()->model_eval_ptr());
  // Set the solution vector in the nox group. This will reset all isValid
  // flags.
  grp.setX(*x_ptr);

  // ---------------------------------------------------------------------------
  // Compute F and jacobian
  // ---------------------------------------------------------------------------
  grp_ptr->computeJacobian();

  // overwrite F with boundary force
  interface_force_np_ptr_->Scale(-(ti_impl->TimIntParam()));
  ti_impl->dbc_ptr()->ApplyDirichletToRhs(interface_force_np_ptr_);
  Teuchos::RCP<::NOX::Epetra::Vector> nox_force =
      Teuchos::rcp(new ::NOX::Epetra::Vector(interface_force_np_ptr_));
  grp_ptr->setF(nox_force);

  // ---------------------------------------------------------------------------
  // Check if we are using a Newton direction
  // ---------------------------------------------------------------------------
  const std::string dir_str(NOX::Nln::Aux::GetDirectionMethodListName(noxparams));
  if (dir_str != "Newton")
    FOUR_C_THROW(
        "The RelaxationSolve is currently only working for the direction-"
        "method \"Newton\".");

  // ---------------------------------------------------------------------------
  // (re)set the linear solver parameters
  // ---------------------------------------------------------------------------
  Teuchos::ParameterList& p =
      noxparams.sublist("Direction").sublist("Newton").sublist("Linear Solver");
  p.set<int>("Number of Nonlinear Iterations", 0);
  p.set<int>("Current Time Step", global_state().get_step_np());
  p.set<double>("Wanted Tolerance", 1.0e-6);  //!< dummy

  // ---------------------------------------------------------------------------
  // solve the linear system of equations and update the current state
  // ---------------------------------------------------------------------------
  // compute the Newton direction
  grp_ptr->computeNewton(p);

  // get the increment from the previous solution step
  const ::NOX::Epetra::Vector& increment =
      dynamic_cast<const ::NOX::Epetra::Vector&>(grp_ptr->getNewton());

  // return the increment
  return Teuchos::rcpFromRef(increment.getEpetraVector());
}
/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
const Solid::TimeInt::BaseDataIO& Solid::MODELEVALUATOR::PartitionedFSI::GetInOutput() const
{
  check_init();
  return global_in_output();
}

FOUR_C_NAMESPACE_CLOSE
