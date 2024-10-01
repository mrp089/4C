/*----------------------------------------------------------------------*/
/*! \file

\brief Wrapper for the field time integration

\level 2


*/
/*-----------------------------------------------------------------------*/

#include "4C_adapter_field_wrapper.hpp"

FOUR_C_NAMESPACE_OPEN

/*-----------------------------------------------------------------------/
| start new time step                                                    |
/-----------------------------------------------------------------------*/
void Adapter::FieldWrapper::prepare_time_step()
{
  field_->prepare_time_step();
  if (nox_correction_) reset_stepinc();
}


void Adapter::FieldWrapper::update_state_incrementally(
    Teuchos::RCP<const Core::LinAlg::Vector<double>> disiterinc)
{
  if (nox_correction_) get_iterinc(disiterinc);
  field_->update_state_incrementally(disiterinc);
}

/*-----------------------------------------------------------------------/
| update dofs and evaluate elements                                      |
/-----------------------------------------------------------------------*/
void Adapter::FieldWrapper::evaluate(Teuchos::RCP<const Core::LinAlg::Vector<double>> disiterinc)
{
  if (nox_correction_) get_iterinc(disiterinc);
  field_->evaluate(disiterinc);
}

/*-----------------------------------------------------------------------/
| update dofs and evaluate elements                                      |
/-----------------------------------------------------------------------*/
void Adapter::FieldWrapper::evaluate(
    Teuchos::RCP<const Core::LinAlg::Vector<double>> disiterinc, bool firstiter)
{
  if (nox_correction_) get_iterinc(disiterinc);
  field_->evaluate(disiterinc, firstiter);
}

/*-----------------------------------------------------------------------/
| Reset Step Increment                                                   |
/-----------------------------------------------------------------------*/
void Adapter::FieldWrapper::reset_stepinc()
{
  if (stepinc_ != Teuchos::null) stepinc_->PutScalar(0.);
}

/*-----------------------------------------------------------------------/
| Get Iteration Increment from Step Increment                            |
/-----------------------------------------------------------------------*/
void Adapter::FieldWrapper::get_iterinc(Teuchos::RCP<const Core::LinAlg::Vector<double>>& stepinc)
{
  // The field solver always expects an iteration increment only. And
  // there are Dirichlet conditions that need to be preserved. So take
  // the sum of increments we get from NOX and apply the latest iteration
  // increment only.
  // Naming:
  //
  // x^n+1_i+1 = x^n+1_i + iterinc  (sometimes referred to as residual increment), and
  //
  // x^n+1_i+1 = x^n     + stepinc
  if (stepinc != Teuchos::null)
  {
    // iteration increments
    Teuchos::RCP<Core::LinAlg::Vector<double>> iterinc =
        Teuchos::rcp(new Core::LinAlg::Vector<double>(*stepinc));
    if (stepinc_ != Teuchos::null)
    {
      iterinc->Update(-1.0, *stepinc_, 1.0);

      // update incremental dof member to provided step increments
      // shortly: disinc_^<i> := disp^<i+1>
      stepinc_->Update(1.0, *stepinc, 0.0);
    }
    else
    {
      stepinc_ = Teuchos::rcp(new Core::LinAlg::Vector<double>(*stepinc));
    }
    // output is iterinc!
    stepinc = Teuchos::rcp(new const Core::LinAlg::Vector<double>(*iterinc));
  }
}

FOUR_C_NAMESPACE_CLOSE
