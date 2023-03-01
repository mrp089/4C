/*----------------------------------------------------------------------*/
/*! \file
\brief Definition of a penalty term like growth strategy

\level 3
*/
/*----------------------------------------------------------------------*/

#include "mixture_growth_strategy_stiffness.H"
#include "mixture_growth_strategy.H"
#include "mat_service.H"
#include "lib_voigt_notation.H"
#include "mat_par_material.H"

MIXTURE::PAR::StiffnessGrowthStrategy::StiffnessGrowthStrategy(
    const Teuchos::RCP<MAT::PAR::Material>& matdata)
    : MIXTURE::PAR::MixtureGrowthStrategy(matdata), kappa_(matdata->GetDouble("KAPPA"))
{
}

std::unique_ptr<MIXTURE::MixtureGrowthStrategy>
MIXTURE::PAR::StiffnessGrowthStrategy::CreateGrowthStrategy()
{
  std::unique_ptr<MIXTURE::StiffnessGrowthStrategy> strategy(
      new MIXTURE::StiffnessGrowthStrategy(this));
  return std::move(strategy);
}

MIXTURE::StiffnessGrowthStrategy::StiffnessGrowthStrategy(
    MIXTURE::PAR::StiffnessGrowthStrategy* params)
    : params_(params)
{
}

void MIXTURE::StiffnessGrowthStrategy::EvaluateInverseGrowthDeformationGradient(
    LINALG::Matrix<3, 3>& iFgM, const MIXTURE::MixtureRule& mixtureRule,
    double currentReferenceGrowthScalar, int gp) const
{
  MAT::IdentityMatrix(iFgM);
}

void MIXTURE::StiffnessGrowthStrategy::EvaluateGrowthStressCmat(
    const MIXTURE::MixtureRule& mixtureRule, double currentReferenceGrowthScalar,
    const LINALG::Matrix<1, 6>& dCurrentReferenceGrowthScalarDC, const LINALG::Matrix<3, 3>& F,
    const LINALG::Matrix<6, 1>& E_strain, Teuchos::ParameterList& params,
    LINALG::Matrix<6, 1>& S_stress, LINALG::Matrix<6, 6>& cmat, const int gp,
    const int eleGID) const
{
  LINALG::Matrix<3, 3> iC(false);
  iC.MultiplyTN(F, F);
  iC.Invert();

  LINALG::Matrix<6, 1> iC_stress(false);
  UTILS::VOIGT::Stresses::MatrixToVector(iC, iC_stress);

  const double kappa = params_->kappa_;
  const double detF = F.Determinant();
  const double I3 = detF * detF;

  const double dPi = 0.5 * kappa * (1.0 - currentReferenceGrowthScalar / detF);
  const double ddPi = 0.25 * kappa * currentReferenceGrowthScalar / std::pow(detF, 3);

  const double ddPiDGrowthScalar = -0.5 * kappa / detF;

  const double gamma2 = 2.0 * I3 * dPi;
  const double dgamma2DGrowthScalar = 4.0 * I3 * ddPiDGrowthScalar;
  const double delta5 = 4. * (I3 * dPi + I3 * I3 * ddPi);
  const double delta6 = -4.0 * I3 * dPi;


  S_stress.Update(gamma2, iC_stress, 0.0);

  // contribution: Cinv \otimes Cinv
  cmat.MultiplyNT(delta5, iC_stress, iC_stress, 0.0);
  // contribution: Cinv \odot Cinv
  MAT::AddtoCmatHolzapfelProduct(cmat, iC_stress, delta6);

  cmat.MultiplyNN(dgamma2DGrowthScalar, iC_stress, dCurrentReferenceGrowthScalarDC, 1.0);
}