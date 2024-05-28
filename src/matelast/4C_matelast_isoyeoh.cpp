/*----------------------------------------------------------------------*/
/*! \file
\brief Implementation of the isochoric contribution of a Yeoh-type material

\level 1
*/
/*----------------------------------------------------------------------*/

#include "4C_matelast_isoyeoh.hpp"

#include "4C_material_input_base.hpp"

FOUR_C_NAMESPACE_OPEN


MAT::ELASTIC::PAR::IsoYeoh::IsoYeoh(const Teuchos::RCP<CORE::MAT::PAR::Material>& matdata)
    : Parameter(matdata),
      c1_(matdata->Get<double>("C1")),
      c2_(matdata->Get<double>("C2")),
      c3_(matdata->Get<double>("C3"))
{
}

MAT::ELASTIC::IsoYeoh::IsoYeoh(MAT::ELASTIC::PAR::IsoYeoh* params) : params_(params) {}

void MAT::ELASTIC::IsoYeoh::AddStrainEnergy(double& psi, const CORE::LINALG::Matrix<3, 1>& prinv,
    const CORE::LINALG::Matrix<3, 1>& modinv, const CORE::LINALG::Matrix<6, 1>& glstrain,
    const int gp, const int eleGID)
{
  const double c1 = params_->c1_;
  const double c2 = params_->c2_;
  const double c3 = params_->c3_;

  // strain energy: Psi = C1 (\overline{I}_{\boldsymbol{C}}-3) + C2
  // (\overline{I}_{\boldsymbol{C}}-3)^2 + C3 (\overline{I}_{\boldsymbol{C}}-3)^3. add to overall
  // strain energy
  psi += c1 * (modinv(0) - 3.) + c2 * (modinv(0) - 3.) * (modinv(0) - 3.) +
         c3 * (modinv(0) - 3.) * (modinv(0) - 3.) * (modinv(0) - 3.);
}

void MAT::ELASTIC::IsoYeoh::add_derivatives_modified(CORE::LINALG::Matrix<3, 1>& dPmodI,
    CORE::LINALG::Matrix<6, 1>& ddPmodII, const CORE::LINALG::Matrix<3, 1>& modinv, const int gp,
    const int eleGID)
{
  const double c1 = params_->c1_;
  const double c2 = params_->c2_;
  const double c3 = params_->c3_;

  dPmodI(0) += c1 + 2. * c2 * (modinv(0) - 3.) + 3. * c3 * (modinv(0) - 3.) * (modinv(0) - 3.);
  ddPmodII(0) += 2. * c2 + 6. * c3 * (modinv(0) - 3.);
}
FOUR_C_NAMESPACE_CLOSE