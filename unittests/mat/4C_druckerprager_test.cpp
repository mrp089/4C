/*----------------------------------------------------------------------*/
/*! \file
\brief unit testing functionality for druckerprager material
\level 3
*/
/*----------------------------------------------------------------------*/
#include <gtest/gtest.h>

#include "4C_beam3_kirchhoff.hpp"
#include "4C_beam3_reissner.hpp"
#include "4C_comm_pack_buffer.hpp"
#include "4C_global_data.hpp"
#include "4C_linalg_FADmatrix_utils.hpp"
#include "4C_mat_material_factory.hpp"
#include "4C_mat_par_bundle.hpp"
#include "4C_mat_plasticdruckerprager.hpp"
#include "4C_mat_service.hpp"
#include "4C_material_base.hpp"
#include "4C_material_input_base.hpp"
#include "4C_unittest_utils_assertions_test.hpp"
namespace
{
  using namespace FourC;

  class DruckerPragerTest : public ::testing::Test
  {
   protected:
    void SetUp() override
    {
      const Teuchos::RCP<CORE::MAT::PAR::Material> container = Teuchos::rcp(
          new CORE::MAT::PAR::Material(1, CORE::Materials::MaterialType::m_pldruckprag));
      container->Add("YOUNG", 1.0);
      container->Add("NUE", 0.25);
      container->Add("DENS", 0.0);
      container->Add("ISOHARD", 1.0);
      container->Add("TOL", 1.e-12);
      container->Add("C", 1.);
      container->Add("ETA", 1.);
      container->Add("XI", 1.);
      container->Add("ETABAR", 1.);
      container->Add("MAXITER", 50);
      container->set_parameter(new MAT::PAR::PlasticDruckerPrager(container));
      GLOBAL::Problem& problem = (*GLOBAL::Problem::Instance());
      problem.Materials()->SetReadFromProblem(0);
      problem.Materials()->Insert(1, container);
      problem.Materials().assert_not_null();
      param_druckprag_ = Teuchos::rcp(new MAT::PAR::PlasticDruckerPrager(container));
      druckprag_ = Teuchos::rcp(new MAT::PlasticDruckerPrager(param_druckprag_.get()));
    };
    void TearDown() override
    {
      // We need to make sure the GLOBAL::Problem instance created in SetUp is deleted again. If
      // this is not done, some troubles arise where unit tests influence each other on some
      // configurations. We suspect that missing singleton destruction might be the reason for that.
      GLOBAL::Problem::Done();
    };
    Teuchos::RCP<MAT::PAR::PlasticDruckerPrager> param_druckprag_;
    CORE::COMM::PackBuffer data;
    Teuchos::RCP<MAT::PlasticDruckerPrager> druckprag_;
  };

  //! test member function Pack and unpack
  TEST_F(DruckerPragerTest, TestPackUnpack)
  {
    INPUT::LineDefinition linedef;
    druckprag_->Setup(1, &linedef);
    CORE::LINALG::Matrix<6, 1> input_strain;
    for (int i = 0; i < 6; ++i) input_strain(i) = .1;
    Teuchos::ParameterList paras;
    CORE::LINALG::Matrix<3, 3> defgrad(true);
    CORE::LINALG::Matrix<6, 1> ref_stress(true);
    for (int i = 0; i < 3; ++i)
      ref_stress(i) =
          (1.0 / ((1.0 + 0.25) * (1.0 - (2.0 * 0.25)))) * ((1.0 - 0.25) + 0.25 + 0.25) * .1;
    for (int i = 3; i < 6; ++i)
      ref_stress(i) =
          (1.0 / ((1.0 + 0.25) * (1.0 - (2.0 * 0.25)))) * ((1.0 - (2.0 * 0.25)) / 2.0) * .1;
    CORE::LINALG::Matrix<6, 6> result_cmat(true);
    CORE::LINALG::Matrix<6, 1> result_stress(true);
    data.StartPacking();
    druckprag_->Pack(data);
    std::vector<char> dataSend;
    swap(dataSend, data());
    for (int i = 0; i < 4; i++) dataSend.erase(dataSend.begin());
    auto plastic = Teuchos::rcp(new MAT::PlasticDruckerPrager());
    plastic->Unpack(dataSend);
    plastic->Evaluate(&defgrad, &input_strain, paras, &result_stress, &result_cmat, 0, 0);
    FOUR_C_EXPECT_NEAR(result_stress, ref_stress, 1.0e-12);
  };

  //! test member function Evaluate
  TEST_F(DruckerPragerTest, TestEvaluate)
  {
    INPUT::LineDefinition linedef;
    druckprag_->Setup(1, &linedef);
    CORE::LINALG::Matrix<6, 1> input_strain;
    for (int i = 0; i < 6; ++i) input_strain(i) = .1;
    Teuchos::ParameterList paras;
    CORE::LINALG::Matrix<3, 3> defgrad(true);
    CORE::LINALG::Matrix<6, 1> ref_stress(true);
    for (int i = 0; i < 3; ++i)
      ref_stress(i) =
          (1.0 / ((1.0 + 0.25) * (1.0 - (2.0 * 0.25)))) * ((1.0 - 0.25) + 0.25 + 0.25) * .1;
    for (int i = 3; i < 6; ++i)
      ref_stress(i) =
          (1.0 / ((1.0 + 0.25) * (1.0 - (2.0 * 0.25)))) * ((1.0 - (2.0 * 0.25)) / 2.0) * .1;
    CORE::LINALG::Matrix<6, 6> result_cmat(true);
    CORE::LINALG::Matrix<6, 1> result_stress(true);
    druckprag_->Evaluate(&defgrad, &input_strain, paras, &result_stress, &result_cmat, 0, 0);
    FOUR_C_EXPECT_NEAR(result_stress, ref_stress, 1.0e-12);
  };

  //! test member function Evaluate for Return to Cone
  TEST_F(DruckerPragerTest, TestEvaluateReturnToCone)
  {
    INPUT::LineDefinition linedef;
    druckprag_->Setup(1, &linedef);
    CORE::LINALG::Matrix<6, 1> input_strain;
    for (int i = 0; i < 3; ++i) input_strain(i) = 0.0;
    for (int i = 3; i < 6; ++i) input_strain(i) = 2.2;
    Teuchos::ParameterList paras;
    CORE::LINALG::Matrix<3, 3> defgrad(true);
    double Dgamma = (2.2 * sqrt(3) / 2.5 - 1.0) / 31.0 * 15.0;
    CORE::LINALG::Matrix<6, 1> ref_stress;
    for (int i = 0; i < 3; ++i) ref_stress(i) = -(Dgamma * (1.0 / (3.0 * (1.0 - (2.0 * 0.25)))));
    for (int i = 3; i < 6; ++i)
      ref_stress(i) = (1.0 / (2 * (1.0 + 0.25))) *
                      (1 - ((1.0 / (2 * (1.0 + 0.25))) * Dgamma / (2.2 * sqrt(3) / 2.5))) * 2.2;
    CORE::LINALG::Matrix<6, 6> result_cmat(true);
    CORE::LINALG::Matrix<6, 1> result_stress(true);
    druckprag_->Evaluate(&defgrad, &input_strain, paras, &result_stress, &result_cmat, 0, 0);
    FOUR_C_EXPECT_NEAR(result_stress, ref_stress, 1.0e-12);
  };

  //! test member function Evaluate for Return to Apex
  TEST_F(DruckerPragerTest, TestEvaluateReturnToApex)
  {
    INPUT::LineDefinition linedef;
    druckprag_->Setup(1, &linedef);
    CORE::LINALG::Matrix<6, 1> input_strain;
    for (int i = 0; i < 3; ++i) input_strain(i) = 1.0;
    for (int i = 3; i < 6; ++i) input_strain(i) = 0.0;
    Teuchos::ParameterList paras;
    CORE::LINALG::Matrix<3, 3> defgrad(true);
    CORE::LINALG::Matrix<6, 1> ref_stress(true);
    for (int i = 0; i < 3; ++i) ref_stress(i) = 2.0 - (10. / 15.) * (3. / 5.);
    CORE::LINALG::Matrix<6, 6> result_cmat(true);
    CORE::LINALG::Matrix<6, 1> result_stress(true);
    druckprag_->Evaluate(&defgrad, &input_strain, paras, &result_stress, &result_cmat, 0, 0);
    FOUR_C_EXPECT_NEAR(result_stress, ref_stress, 1.0e-12);
  };

  //! test member function Evaluate for History and elastic unloading
  TEST_F(DruckerPragerTest, TestEvaluateHistory)
  {
    INPUT::LineDefinition linedef;
    druckprag_->Setup(1, &linedef);
    CORE::LINALG::Matrix<6, 1, FAD> input_strain;
    for (int i = 0; i < 3; ++i) input_strain(i) = FAD(6, i, 0.1);
    for (int i = 3; i < 6; ++i) input_strain(i) = FAD(6, i, 0.1);
    Teuchos::ParameterList paras;
    CORE::LINALG::Matrix<3, 3> defgrad(true);
    CORE::LINALG::Matrix<6, 1, FAD> ref_stress(true);
    CORE::LINALG::Matrix<6, 6> result_cmat(true);
    CORE::LINALG::Matrix<6, 1, FAD> result_stress(true);
    druckprag_->Evaluate(&defgrad, &input_strain, paras, &result_stress, &result_cmat, 0, 0);
    CORE::LINALG::Matrix<6, 6> ref_cmat(true);
    for (int i = 0; i < 6; i++)
    {
      for (int j = 0; j < 6; j++)
      {
        ref_cmat(i, j) = result_stress(i).dx(j);
      }
    }
    FOUR_C_EXPECT_NEAR(result_cmat, ref_cmat, 1.0e-12);
    druckprag_->Update();
    for (int i = 0; i < 3; ++i) input_strain(i) = FAD(6, i, 1.0);
    for (int i = 3; i < 6; ++i) input_strain(i) = FAD(6, i, 0.0);
    druckprag_->Evaluate(&defgrad, &input_strain, paras, &result_stress, &result_cmat, 0, 0);
    for (int i = 0; i < 6; i++)
    {
      for (int j = 0; j < 6; j++)
      {
        ref_cmat(i, j) = result_stress(i).dx(j);
      }
    }
    FOUR_C_EXPECT_NEAR(result_cmat, ref_cmat, 1.0e-12);
    druckprag_->Update();
    for (int i = 0; i < 3; ++i) input_strain(i) = FAD(6, i, 0.2);
    for (int i = 3; i < 6; ++i) input_strain(i) = FAD(6, i, 0.0);
    druckprag_->Evaluate(&defgrad, &input_strain, paras, &result_stress, &result_cmat, 0, 0);
    for (int i = 0; i < 6; i++)
    {
      for (int j = 0; j < 6; j++)
      {
        ref_cmat(i, j) = result_stress(i).dx(j);
      }
    }
    FOUR_C_EXPECT_NEAR(CORE::FADUTILS::CastToDouble(result_stress),
        CORE::FADUTILS::CastToDouble(ref_stress), 1.0e-12);
  };

  //! test member function Evaluate for arbitrary values
  TEST_F(DruckerPragerTest, TestEvaluateRandomStrain)
  {
    INPUT::LineDefinition linedef;
    druckprag_->Setup(1, &linedef);
    CORE::LINALG::Matrix<6, 1> input_strain;
    input_strain(0) = 1.1;
    input_strain(1) = 2.0;
    input_strain(2) = 0.1;
    input_strain(3) = 2.5;
    input_strain(4) = 1.4;
    input_strain(5) = 1.0;
    Teuchos::ParameterList paras;
    CORE::LINALG::Matrix<3, 3> defgrad(true);
    CORE::LINALG::Matrix<6, 1> ref_stress(true);
    ref_stress(0) = 1.3231031817668;
    ref_stress(1) = 1.7934880206154;
    ref_stress(2) = 0.8004533608238;
    ref_stress(3) = 0.6533122761787;
    ref_stress(4) = 0.3658548746601;
    ref_stress(5) = 0.2613249104715;
    CORE::LINALG::Matrix<6, 6> result_cmat(true);
    CORE::LINALG::Matrix<6, 1> result_stress(true);
    druckprag_->Evaluate(&defgrad, &input_strain, paras, &result_stress, &result_cmat, 0, 0);
    FOUR_C_EXPECT_NEAR(result_stress, ref_stress, 1.0e-12);
  };

  //! test member function Evaluate
  TEST_F(DruckerPragerTest, TestEvaluateCmat)
  {
    INPUT::LineDefinition linedef;
    druckprag_->Setup(1, &linedef);
    CORE::LINALG::Matrix<6, 1, FAD> input_strain;
    for (int i = 0; i < 6; ++i) input_strain(i) = FAD(6, i, .1 * i);
    Teuchos::ParameterList paras;
    CORE::LINALG::Matrix<3, 3> defgrad(true);
    CORE::LINALG::Matrix<6, 1, FAD> ref_stress(true);
    for (int i = 0; i < 3; ++i)
      ref_stress(i) =
          FAD((1.0 / ((1.0 + 0.25) * (1.0 - (2.0 * 0.25)))) * ((1.0 - 0.25) + 0.25 + 0.25) * .1);
    for (int i = 3; i < 6; ++i)
      ref_stress(i) =
          FAD((1.0 / ((1.0 + 0.25) * (1.0 - (2.0 * 0.25)))) * ((1.0 - (2.0 * 0.25)) / 2.0) * .1);
    CORE::LINALG::Matrix<6, 6> result_cmat(true);
    CORE::LINALG::Matrix<6, 1, FAD> result_stress(true);
    druckprag_->Evaluate(&defgrad, &input_strain, paras, &result_stress, &result_cmat, 0, 0);
    CORE::LINALG::Matrix<6, 6> ref_cmat(true);
    for (int i = 0; i < 6; i++)
    {
      for (int j = 0; j < 6; j++)
      {
        ref_cmat(i, j) = result_stress(i).dx(j);
      }
    }
    FOUR_C_EXPECT_NEAR(result_cmat, ref_cmat, 1.0e-12);
  };

  //! test CMAT matrix for Return to Cone
  TEST_F(DruckerPragerTest, TestEvaluateReturnToConeCmat)
  {
    INPUT::LineDefinition linedef;
    druckprag_->Setup(1, &linedef);
    CORE::LINALG::Matrix<6, 1, FAD> input_strain;
    for (int i = 0; i < 3; ++i) input_strain(i) = FAD(6, i, 0.1 * i);
    for (int i = 3; i < 6; ++i) input_strain(i) = FAD(6, i, 2.2 * i);
    Teuchos::ParameterList paras;
    CORE::LINALG::Matrix<3, 3> defgrad(true);
    CORE::LINALG::Matrix<6, 6> result_cmat(true);
    CORE::LINALG::Matrix<6, 1, FAD> result_stress(true);
    druckprag_->Evaluate(&defgrad, &input_strain, paras, &result_stress, &result_cmat, 0, 0);
    CORE::LINALG::Matrix<6, 6> ref_cmat(true);
    for (int i = 0; i < 6; i++)
    {
      for (int j = 0; j < 6; j++)
      {
        ref_cmat(i, j) = result_stress(i).dx(j);
      }
    }
    FOUR_C_EXPECT_NEAR(result_cmat, ref_cmat, 1.0e-12);
  };
  TEST_F(DruckerPragerTest, TestEvaluateReturnToApexCmat)
  {
    INPUT::LineDefinition linedef;
    druckprag_->Setup(1, &linedef);
    CORE::LINALG::Matrix<6, 1, FAD> input_strain;
    for (int i = 0; i < 3; ++i) input_strain(i) = FAD(6, i, 1.0);
    for (int i = 3; i < 6; ++i) input_strain(i) = FAD(6, i, 0.0);
    Teuchos::ParameterList paras;
    CORE::LINALG::Matrix<3, 3> defgrad(true);
    CORE::LINALG::Matrix<6, 6> result_cmat(true);
    CORE::LINALG::Matrix<6, 1, FAD> result_stress(true);
    druckprag_->Evaluate(&defgrad, &input_strain, paras, &result_stress, &result_cmat, 0, 0);
    CORE::LINALG::Matrix<6, 6> ref_cmat(true);
    for (int i = 0; i < 6; i++)
    {
      for (int j = 0; j < 6; j++)
      {
        ref_cmat(i, j) = result_stress(i).dx(j);
      }
    }
    FOUR_C_EXPECT_NEAR(result_cmat, ref_cmat, 1.0e-12);
  };

  //! test CMAT matrix for Return to Apex
  TEST_F(DruckerPragerTest, TestEvaluateRandomStrainCmat)
  {
    INPUT::LineDefinition linedef;
    druckprag_->Setup(1, &linedef);
    CORE::LINALG::Matrix<6, 1, FAD> input_strain;
    input_strain(0) = FAD(6, 0, 1.1);
    input_strain(1) = FAD(6, 1, 2.0);
    input_strain(2) = FAD(6, 2, 0.1);
    input_strain(3) = FAD(6, 3, 2.5);
    input_strain(4) = FAD(6, 4, 1.4);
    input_strain(5) = FAD(6, 5, 1.0);
    Teuchos::ParameterList paras;
    CORE::LINALG::Matrix<3, 3> defgrad(true);
    CORE::LINALG::Matrix<6, 1, FAD> ref_stress(true);
    ref_stress(0) = FAD(1.4142412329012);
    ref_stress(1) = FAD(1.8571566160540);
    ref_stress(2) = FAD(0.9221130293981);
    ref_stress(3) = FAD(0.6151602543789);
    ref_stress(4) = FAD(0.3444897424522);
    ref_stress(5) = FAD(0.2460641017516);
    CORE::LINALG::Matrix<6, 6> result_cmat(true);
    CORE::LINALG::Matrix<6, 1, FAD> result_stress(true);
    druckprag_->Evaluate(&defgrad, &input_strain, paras, &result_stress, &result_cmat, 0, 0);
    CORE::LINALG::Matrix<6, 6> ref_cmat(true);
    for (int i = 0; i < 6; i++)
    {
      for (int j = 0; j < 6; j++)
      {
        ref_cmat(i, j) = result_stress(i).dx(j);
      }
    }
    FOUR_C_EXPECT_NEAR(result_cmat, ref_cmat, 1.0e-12);
  };
}  // namespace