/*----------------------------------------------------------------------*/
/*! \file

\brief unit testing functionality for the mirco contact constitutivelaw with pressure-based Green
function

\level 2

*/
/*----------------------------------------------------------------------*/

#include <gtest/gtest.h>

#include "baci_contact_constitutivelaw_mirco_contactconstitutivelaw.hpp"
#include "baci_global_data.hpp"
#include "baci_mat_par_bundle.hpp"

#ifdef BACI_WITH_MIRCO

namespace
{
  using namespace BACI;

  class MircoConstitutiveLawPressureTest : public ::testing::Test
  {
   public:
    MircoConstitutiveLawPressureTest()
    {
      const int problemid(0);
      GLOBAL::Problem& problem = (*GLOBAL::Problem::Instance());
      problem.Materials()->SetReadFromProblem(problemid);
      // set up material to be added to problem instance
      const int matid(1);
      Teuchos::RCP<MAT::PAR::Material> material =
          Teuchos::rcp(new MAT::PAR::Material(matid, INPAR::MAT::m_stvenant, "first_material"));
      material->Add("YOUNG", 1.0);
      material->Add("NUE", 0.3);

      // add material to problem instance
      problem.Materials()->Insert(matid, material);

      // initialize container for material parameters
      const Teuchos::RCP<CONTACT::CONSTITUTIVELAW::Container> container =
          Teuchos::rcp(new CONTACT::CONSTITUTIVELAW::Container(
              1, INPAR::CONTACT::ConstitutiveLawType::colaw_mirco, "Mirco Constitutivelaw"));

      // add parameters to container
      container->Add("FirstMatID", 1);
      container->Add("SecondMatID", 1);
      container->Add("LateralLength", 1000.0);
      container->Add("Resolution", 6);
      container->Add("PressureGreenFunFlag", true);
      container->Add("InitialTopologyStdDeviation", 20.0);
      container->Add("HurstExponent", 0.7);
      container->Add("RandomTopologyFlag", true);
      container->Add("RandomSeedFlag", false);
      container->Add("RandomGeneratorSeed", 95);
      container->Add("Tolerance", 0.01);
      container->Add("MaxIteration", 100);
      container->Add("WarmStartingFlag", true);
      container->Add("Offset", 2.0);
      container->Add("FiniteDifferenceFraction", 0.001);
      container->Add("ActiveGapTolerance", 1e-6);
      container->Add("TopologyFilePath", std::string("sup6.dat"));

      const Teuchos::RCP<CONTACT::CONSTITUTIVELAW::ConstitutiveLaw> mircococonstlaw =
          CONTACT::CONSTITUTIVELAW::ConstitutiveLaw::Factory(container);
      coconstlaw_ = mircococonstlaw;
    }
    Teuchos::RCP<CONTACT::CONSTITUTIVELAW::ConstitutiveLaw> coconstlaw_;
  };

  //! test member function Evaluate
  TEST_F(MircoConstitutiveLawPressureTest, TestEvaluate)
  {
    // gap < 0
    EXPECT_ANY_THROW(coconstlaw_->Evaluate(1.0));
    // 0< gap < offset
    EXPECT_ANY_THROW(coconstlaw_->Evaluate(-0.25));
    // offset < gap
    EXPECT_NEAR(coconstlaw_->Evaluate(-12.0), -0.0005861475487657709, 1.e-10);
  }

  //! test member function EvaluateDeriv
  TEST_F(MircoConstitutiveLawPressureTest, TestEvaluateDeriv)
  {
    EXPECT_NEAR(coconstlaw_->EvaluateDeriv(-12), 1.56329102801896e-04, 1.e-10);
    EXPECT_ANY_THROW(coconstlaw_->EvaluateDeriv(-0.25));
  }
}  // namespace

#endif