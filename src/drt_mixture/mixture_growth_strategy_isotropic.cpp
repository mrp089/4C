/*----------------------------------------------------------------------*/
/*! \file
\brief Implementation of an isotropic growth strategy for the growth remodel mixture rule

\level 3
*/
/*----------------------------------------------------------------------*/

#include "mixture_growth_strategy_isotropic.H"
#include "mixture_growth_strategy.H"

MIXTURE::PAR::IsotropicGrowthStrategy::IsotropicGrowthStrategy(
    const Teuchos::RCP<MAT::PAR::Material>& matdata)
    : MIXTURE::PAR::MixtureGrowthStrategy(matdata)
{
}

std::unique_ptr<MIXTURE::MixtureGrowthStrategy>
MIXTURE::PAR::IsotropicGrowthStrategy::CreateGrowthStrategy()
{
  std::unique_ptr<MIXTURE::IsotropicGrowthStrategy> strategy(
      new MIXTURE::IsotropicGrowthStrategy());
  return std::move(strategy);
}

void MIXTURE::IsotropicGrowthStrategy::EvaluateInverseGrowthDeformationGradient(
    LINALG::Matrix<3, 3>& iFgM, const MIXTURE::MixtureRule& mixtureRule,
    double currentReferenceGrowthScalar, int gp) const
{
  iFgM.Clear();

  for (int i = 0; i < 3; ++i)
  {
    iFgM(i, i) = std::pow(currentReferenceGrowthScalar, -1.0 / 3.0);
  }
}

void MIXTURE::IsotropicGrowthStrategy::AddGrowthStressCmat(const MIXTURE::MixtureRule& mixtureRule,
    double currentReferenceGrowthScalar, const LINALG::Matrix<3, 3>& F,
    const LINALG::Matrix<6, 1>& E_strain, Teuchos::ParameterList& params,
    LINALG::Matrix<6, 1>& S_stress, LINALG::Matrix<6, 6>& cmat, const int gp,
    const int eleGID) const
{
}