/*--------------------------------------------------------------------------*/
/*! \file
\brief calculation classes for evaluation of constitutive relation for lubrication

\level 3

*/
/*--------------------------------------------------------------------------*/

#include "4C_mat_lubrication_law.hpp"

#include "4C_mat_par_bundle.hpp"

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
MAT::PAR::LubricationLaw::LubricationLaw(Teuchos::RCP<CORE::MAT::PAR::Material> matdata)
    : Parameter(matdata)
{
  return;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
MAT::PAR::LubricationLawConstant::LubricationLawConstant(
    Teuchos::RCP<CORE::MAT::PAR::Material> matdata)
    : LubricationLaw(matdata), viscosity_(matdata->Get<double>("VISCOSITY"))
{
  return;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::RCP<CORE::MAT::Material> MAT::PAR::LubricationLawConstant::create_material()
{
  return Teuchos::null;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void MAT::PAR::LubricationLawConstant::ComputeViscosity(const double& press, double& viscosity)
{
  viscosity = viscosity_;
  return;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void MAT::PAR::LubricationLawConstant::constitutive_derivatives(
    const double& press, const double& viscosity, double& dviscosity_dp)
{
  dviscosity_dp = 0.0;

  return;
}

/*---------------------------------------------------------------------*
 *  Method definitions for Barus viscosity
 *---------------------------------------------------------------------*/

// Standard Constructor
MAT::PAR::LubricationLawBarus::LubricationLawBarus(Teuchos::RCP<CORE::MAT::PAR::Material> matdata)
    : LubricationLaw(matdata),
      ABSViscosity_(matdata->Get<double>("ABSViscosity")),
      PreVisCoeff_(matdata->Get<double>("PreVisCoeff"))
{
  return;
}

// Create material instance of matching type with my parameters
Teuchos::RCP<CORE::MAT::Material> MAT::PAR::LubricationLawBarus::create_material()
{
  return Teuchos::null;
}

// Calculate the current viscosity
void MAT::PAR::LubricationLawBarus::ComputeViscosity(const double& press, double& viscosity)
{
  viscosity = ABSViscosity_ * (std::exp(PreVisCoeff_ * press));

  return;
}

// Evaluate constitutive relation for viscosity and compute derivatives
void MAT::PAR::LubricationLawBarus::constitutive_derivatives(
    const double& press, const double& viscosity, double& dviscosity_dp)
{
  dviscosity_dp = viscosity * PreVisCoeff_;

  return;
}

/*---------------------------------------------------------------------*
 *  Method definitions for Roeland viscosity
 *---------------------------------------------------------------------*/

// Standard Constructor
MAT::PAR::LubricationLawRoeland::LubricationLawRoeland(
    Teuchos::RCP<CORE::MAT::PAR::Material> matdata)
    : LubricationLaw(matdata),
      ABSViscosity_(matdata->Get<double>("ABSViscosity")),
      PreVisCoeff_(matdata->Get<double>("PreVisCoeff")),
      RefVisc_(matdata->Get<double>("RefVisc")),
      RefPress_(matdata->Get<double>("RefPress"))
{
  z_ = (PreVisCoeff_ * RefPress_) / (log(ABSViscosity_ / RefVisc_));
  return;
}

// Create material instance of matching type with my parameters
Teuchos::RCP<CORE::MAT::Material> MAT::PAR::LubricationLawRoeland::create_material()
{
  return Teuchos::null;
}

// Calculate the current viscosity
void MAT::PAR::LubricationLawRoeland::ComputeViscosity(const double& press, double& viscosity)
{
  // double z = (PreVisCoeff_ * RefPress_) / (log ( ABSViscosity_ / RefVisc_ ));

  viscosity =
      ABSViscosity_ * exp(log(ABSViscosity_ / RefVisc_) * (pow((1 + press / RefPress_), z_) - 1));

  return;
}

// Evaluate constitutive relation for viscosity and compute derivatives
void MAT::PAR::LubricationLawRoeland::constitutive_derivatives(
    const double& press, const double& viscosity, double& dviscosity_dp)
{
  // double z = (PreVisCoeff_ * RefPress_ ) / (log ( ABSViscosity_ / RefVisc_ ));

  dviscosity_dp = viscosity * log(ABSViscosity_ / RefVisc_) * z_ *
                  pow((1 + press / RefPress_), (z_ - 1)) * (1 / RefPress_);

  return;
}

FOUR_C_NAMESPACE_CLOSE