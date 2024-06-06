/*----------------------------------------------------------------------*/
/*! \file

\brief Testcases for the CoupAnisoExpoShear summand with element fibers

\level 2


*/
/*----------------------------------------------------------------------*/

#include <gtest/gtest.h>

#include "4C_linalg_fixedsizematrix_voigt_notation.hpp"
#include "4C_mat_anisotropy.hpp"
#include "4C_matelast_coupanisoexpo.hpp"
#include "4C_matelast_coupanisoexposhear.hpp"
#include "4C_unittest_utils_assertions_test.hpp"

namespace
{
  using namespace FourC;

  void SetupSingleStructuralTensor(const Core::LinAlg::Matrix<3, 1>& fiber1,
      const Core::LinAlg::Matrix<3, 1>& fiber2, Core::LinAlg::Matrix<3, 3>& structuralTensor)
  {
    Core::LinAlg::Matrix<3, 3> fiber1fiber2T(false);

    fiber1fiber2T.MultiplyNT(fiber1, fiber2);

    structuralTensor.Update(0.5, fiber1fiber2T);
    structuralTensor.UpdateT(0.5, fiber1fiber2T, 1.0);
  }

  class CoupAnisoExpoShearElementFibersTest
      : public ::testing::TestWithParam<std::tuple<std::array<int, 2>, int>>
  {
   public:
    CoupAnisoExpoShearElementFibersTest()
        : anisotropy_(),
          eleFibers_(3),
          eleTensors_(false),
          eleTensors_stress_(false),
          eleScalarProducts_(0.0)
    {
      // setup fibers fibers
      eleFibers_[0](0) = 0.858753861115007;
      eleFibers_[0](1) = 0.449823451060242;
      eleFibers_[0](2) = 0.245358246032859;

      eleFibers_[1](0) = 0.103448275862069;
      eleFibers_[1](1) = 0.137931034482759;
      eleFibers_[1](2) = 0.068965517241379;

      eleFibers_[2](0) = 0.872502871778232;
      eleFibers_[2](1) = 0.134231211042805;
      eleFibers_[2](2) = 0.469809238649817;

      // setup structural tensor
      SetupSingleStructuralTensor(
          eleFibers_[GetFiberIds()[0]], eleFibers_[GetFiberIds()[1]], eleTensors_);

      // Setup structural tensors in stress like Voigt notation
      Core::LinAlg::Voigt::Stresses::MatrixToVector(eleTensors_, eleTensors_stress_);

      // setup scalar product
      eleScalarProducts_ = eleFibers_[GetFiberIds()[0]].Dot(eleFibers_[GetFiberIds()[1]]);

      setup_anisotropy_extension(GetFiberIds());
    }

    void setup_anisotropy_extension(std::array<int, 2> fiber_ids)
    {
      anisotropyExtension_ =
          std::make_unique<Mat::Elastic::CoupAnisoExpoShearAnisotropyExtension>(1, fiber_ids);

      anisotropy_.register_anisotropy_extension(*anisotropyExtension_);

      anisotropy_.set_number_of_gauss_points(2);

      // Setup element fibers
      anisotropy_.SetElementFibers(eleFibers_);
    }

    [[nodiscard]] int get_gauss_point() const { return std::get<1>(GetParam()); }

    [[nodiscard]] std::array<int, 2> GetFiberIds() const { return std::get<0>(GetParam()); }

    Mat::Anisotropy anisotropy_;
    std::unique_ptr<Mat::Elastic::CoupAnisoExpoShearAnisotropyExtension> anisotropyExtension_;

    std::vector<Core::LinAlg::Matrix<3, 1>> eleFibers_;
    Core::LinAlg::Matrix<3, 3> eleTensors_;
    Core::LinAlg::Matrix<6, 1> eleTensors_stress_;
    double eleScalarProducts_;
  };

  TEST_P(CoupAnisoExpoShearElementFibersTest, GetScalarProduct)
  {
    EXPECT_NEAR(
        anisotropyExtension_->GetScalarProduct(get_gauss_point()), eleScalarProducts_, 1e-10);
  }

  TEST_P(CoupAnisoExpoShearElementFibersTest, GetStructuralTensor)
  {
    FOUR_C_EXPECT_NEAR(
        anisotropyExtension_->GetStructuralTensor(get_gauss_point()), eleTensors_, 1e-10);
  }

  TEST_P(CoupAnisoExpoShearElementFibersTest, GetStructuralTensorStress)
  {
    FOUR_C_EXPECT_NEAR(anisotropyExtension_->get_structural_tensor_stress(get_gauss_point()),
        eleTensors_stress_, 1e-10);
  }

  INSTANTIATE_TEST_SUITE_P(GaussPoints, CoupAnisoExpoShearElementFibersTest,
      ::testing::Combine(::testing::Values(std::array<int, 2>({0, 1}), std::array<int, 2>({0, 2}),
                             std::array<int, 2>({1, 2})),
          ::testing::Values(0, 1)));
}  // namespace
