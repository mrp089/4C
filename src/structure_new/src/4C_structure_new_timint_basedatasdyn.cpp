/*-----------------------------------------------------------*/
/*! \file

\brief Structural dynamics data container for the structural (time)
       integration


\level 3

*/
/*-----------------------------------------------------------*/


#include "4C_structure_new_timint_basedatasdyn.hpp"

#include "4C_beaminteraction_periodic_boundingbox.hpp"
#include "4C_global_data.hpp"
#include "4C_lib_discret.hpp"
#include "4C_linear_solver_method_linalg.hpp"
#include "4C_structure_new_utils.hpp"

#include <Teuchos_Time.hpp>

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
STR::TIMINT::BaseDataSDyn::BaseDataSDyn()
    : isinit_(false),
      issetup_(false),
      timemax_(-1.0),
      stepmax_(-1),
      timer_(Teuchos::null),
      damptype_(INPAR::STR::damp_none),
      dampk_(-1.0),
      dampm_(-1.0),
      masslintype_(INPAR::STR::ml_none),
      lumpmass_(false),
      neglectinertia_(false),
      modeltypes_(Teuchos::null),
      eletechs_(Teuchos::null),
      coupling_model_ptr_(Teuchos::null),
      dyntype_(INPAR::STR::dyna_statics),
      stcscale_(INPAR::STR::stc_none),
      stclayer_(-1),
      itermin_(-1),
      itermax_(-1),
      loadlin_(false),
      prestresstype_(INPAR::STR::PreStress::none),
      predtype_(INPAR::STR::pred_vague),
      nlnsolvertype_(INPAR::STR::soltech_vague),
      divergenceaction_(INPAR::STR::divcont_stop),
      mid_time_energy_type_(INPAR::STR::midavg_vague),
      maxdivconrefinementlevel_(-1),
      noxparams_(Teuchos::null),
      ptc_delta_init_(0.0),
      linsolvers_(Teuchos::null),
      normtype_(INPAR::STR::norm_vague),
      nox_normtype_(::NOX::Abstract::Vector::TwoNorm),
      tol_disp_incr_(-1.0),
      toltype_disp_incr_(INPAR::STR::convnorm_abs),
      tol_fres_(-1.0),
      toltype_fres_(INPAR::STR::convnorm_abs),
      tol_pres_(-1.0),
      toltype_pres_(INPAR::STR::convnorm_abs),
      tol_inco_(-1.0),
      toltype_inco_(INPAR::STR::convnorm_abs),
      tol_plast_res_(-1.0),
      toltype_plast_res_(INPAR::STR::convnorm_abs),
      tol_plast_incr_(-1.0),
      toltype_plast_incr_(INPAR::STR::convnorm_abs),
      tol_eas_res_(-1.0),
      toltype_eas_res_(INPAR::STR::convnorm_abs),
      tol_eas_incr_(-1.0),
      toltype_eas_incr_(INPAR::STR::convnorm_abs),
      normcombo_disp_pres_(INPAR::STR::bop_and),
      normcombo_fres_inco_(INPAR::STR::bop_and),
      normcombo_fres_eas_res_(INPAR::STR::bop_and),
      normcombo_disp_eas_incr_(INPAR::STR::bop_and),
      normcombo_fres_plast_res_(INPAR::STR::bop_and),
      normcombo_disp_plast_incr_(INPAR::STR::bop_and),
      normcombo_fres_disp_(INPAR::STR::bop_and),
      toltype_cardvasc0d_res_(INPAR::STR::convnorm_abs),
      tol_cardvasc0d_res_(-1.0),
      toltype_cardvasc0d_incr_(INPAR::STR::convnorm_abs),
      tol_cardvasc0d_incr_(-1.0),
      toltype_constr_res_(INPAR::STR::convnorm_abs),
      tol_constr_res_(-1.0),
      toltype_constr_incr_(INPAR::STR::convnorm_abs),
      tol_constr_incr_(-1.0),
      toltype_contact_res_(INPAR::STR::convnorm_abs),
      tol_contact_res_(-1.0),
      toltype_contact_lm_incr_(INPAR::STR::convnorm_abs),
      tol_contact_lm_incr_(-1.0),
      normcombo_fres_contact_res_(INPAR::STR::bop_and),
      normcombo_disp_contact_lm_incr_(INPAR::STR::bop_and),
      normcombo_fres_cardvasc0d_res_(INPAR::STR::bop_and),
      normcombo_disp_cardvasc0d_incr_(INPAR::STR::bop_and),
      normcombo_fres_constr_res_(INPAR::STR::bop_and),
      normcombo_disp_constr_incr_(INPAR::STR::bop_and),
      rand_tsfac_(1.0),
      divconrefinementlevel_(0),
      divconnumfinestep_(0),
      sdynparams_ptr_(Teuchos::null)
{
  // empty constructor
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void STR::TIMINT::BaseDataSDyn::Init(const Teuchos::RCP<DRT::Discretization> discret,
    const Teuchos::ParameterList& sdynparams, const Teuchos::ParameterList& xparams,
    const Teuchos::RCP<std::set<enum INPAR::STR::ModelType>> modeltypes,
    const Teuchos::RCP<std::set<enum INPAR::STR::EleTech>> eletechs,
    const Teuchos::RCP<std::map<enum INPAR::STR::ModelType, Teuchos::RCP<CORE::LINALG::Solver>>>
        linsolvers)
{
  // We have to call Setup() after Init()
  issetup_ = false;

  // ---------------------------------------------------------------------------
  // initialize general variables
  // ---------------------------------------------------------------------------
  {
    timemax_ = sdynparams.get<double>("MAXTIME");
    stepmax_ = sdynparams.get<int>("NUMSTEP");

    timer_ = Teuchos::rcp(new Teuchos::Time("", true));

    dyntype_ = CORE::UTILS::IntegralValue<INPAR::STR::DynamicType>(sdynparams, "DYNAMICTYP");

    stcscale_ = CORE::UTILS::IntegralValue<INPAR::STR::StcScale>(sdynparams, "STC_SCALING");

    stclayer_ = sdynparams.get<int>("STC_LAYER");

    isrestarting_initial_state_ =
        (CORE::UTILS::IntegralValue<int>(sdynparams, "CALC_ACC_ON_RESTART") == 1);
  }
  // ---------------------------------------------------------------------------
  // initialize the damping control parameters
  // ---------------------------------------------------------------------------
  {
    damptype_ = CORE::UTILS::IntegralValue<INPAR::STR::DampKind>(sdynparams, "DAMPING");
    dampk_ = sdynparams.get<double>("K_DAMP");
    dampm_ = sdynparams.get<double>("M_DAMP");
  }
  // ---------------------------------------------------------------------------
  // initialize the mass and inertia control parameters
  // ---------------------------------------------------------------------------
  {
    masslintype_ = CORE::UTILS::IntegralValue<INPAR::STR::MassLin>(sdynparams, "MASSLIN");
    lumpmass_ = (CORE::UTILS::IntegralValue<int>(sdynparams, "LUMPMASS") == 1);
    neglectinertia_ = (CORE::UTILS::IntegralValue<int>(sdynparams, "NEGLECTINERTIA") == 1);
  }
  // ---------------------------------------------------------------------------
  // initialize model evaluator control parameters
  // ---------------------------------------------------------------------------
  {
    modeltypes_ = modeltypes;
    eletechs_ = eletechs;
    if (modeltypes_->find(INPAR::STR::model_partitioned_coupling) != modeltypes->end())
    {
      if (modeltypes_->find(INPAR::STR::model_monolithic_coupling) != modeltypes->end())
        FOUR_C_THROW("Cannot have both monolithic and partitioned coupling at the same time!");
      coupling_model_ptr_ =
          sdynparams.get<Teuchos::RCP<STR::MODELEVALUATOR::Generic>>("Partitioned Coupling Model");
    }
    else if (modeltypes_->find(INPAR::STR::model_monolithic_coupling) != modeltypes->end())
    {
      coupling_model_ptr_ =
          sdynparams.get<Teuchos::RCP<STR::MODELEVALUATOR::Generic>>("Monolithic Coupling Model");
    }
    else if (modeltypes_->find(INPAR::STR::model_basic_coupling) != modeltypes->end())
    {
      coupling_model_ptr_ =
          sdynparams.get<Teuchos::RCP<STR::MODELEVALUATOR::Generic>>("Basic Coupling Model");
    }
  }
  // ---------------------------------------------------------------------------
  // initialize implicit variables
  // ---------------------------------------------------------------------------
  {
    itermin_ = sdynparams.get<int>("MINITER");
    itermax_ = sdynparams.get<int>("MAXITER");
    loadlin_ = (CORE::UTILS::IntegralValue<int>(sdynparams, "LOADLIN") == 1);
    prestresstime_ =
        GLOBAL::Problem::Instance()->structural_dynamic_params().get<double>("PRESTRESSTIME");
    prestresstype_ = Teuchos::getIntegralValue<INPAR::STR::PreStress>(
        GLOBAL::Problem::Instance()->structural_dynamic_params(), "PRESTRESS");
    prestress_displacement_tolerance_ = sdynparams.get<double>("PRESTRESSTOLDISP");
    prestress_min_number_of_load_steps_ = sdynparams.get<int>("PRESTRESSMINLOADSTEPS");
    predtype_ = CORE::UTILS::IntegralValue<INPAR::STR::PredEnum>(sdynparams, "PREDICT");
    nlnsolvertype_ = CORE::UTILS::IntegralValue<INPAR::STR::NonlinSolTech>(sdynparams, "NLNSOL");
    divergenceaction_ = CORE::UTILS::IntegralValue<INPAR::STR::DivContAct>(sdynparams, "DIVERCONT");
    mid_time_energy_type_ =
        CORE::UTILS::IntegralValue<INPAR::STR::MidAverageEnum>(sdynparams, "MIDTIME_ENERGY_TYPE");
    maxdivconrefinementlevel_ = sdynparams.get<int>("MAXDIVCONREFINEMENTLEVEL");
    noxparams_ = Teuchos::rcp(new Teuchos::ParameterList(xparams.sublist("NOX")));
    ptc_delta_init_ = sdynparams.get<double>("PTCDT");
  }
  // ---------------------------------------------------------------------------
  // initialize linear solver variables
  // ---------------------------------------------------------------------------
  {
    linsolvers_ = linsolvers;
  }
  // ---------------------------------------------------------------------------
  // initialize the status test control parameters
  // ---------------------------------------------------------------------------
  {
    normtype_ = CORE::UTILS::IntegralValue<INPAR::STR::VectorNorm>(sdynparams, "ITERNORM");
    nox_normtype_ = STR::NLN::Convert2NoxNormType(normtype_);

    // -------------------------------------------------------------------------
    // primary variables
    // -------------------------------------------------------------------------
    tol_disp_incr_ = sdynparams.get<double>("TOLDISP");
    toltype_disp_incr_ = CORE::UTILS::IntegralValue<INPAR::STR::ConvNorm>(sdynparams, "NORM_DISP");

    tol_fres_ = sdynparams.get<double>("TOLRES");
    toltype_fres_ = CORE::UTILS::IntegralValue<INPAR::STR::ConvNorm>(sdynparams, "NORM_RESF");

    tol_pres_ = sdynparams.get<double>("TOLPRE");
    toltype_pres_ = INPAR::STR::convnorm_abs;

    tol_inco_ = sdynparams.get<double>("TOLINCO");
    toltype_inco_ = INPAR::STR::convnorm_abs;

    tol_plast_res_ =
        GLOBAL::Problem::Instance()->semi_smooth_plast_params().get<double>("TOLPLASTCONSTR");
    toltype_plast_res_ = INPAR::STR::convnorm_abs;

    tol_plast_incr_ =
        GLOBAL::Problem::Instance()->semi_smooth_plast_params().get<double>("TOLDELTALP");
    toltype_plast_incr_ = INPAR::STR::convnorm_abs;

    tol_eas_res_ = GLOBAL::Problem::Instance()->semi_smooth_plast_params().get<double>("TOLEASRES");
    toltype_eas_res_ = INPAR::STR::convnorm_abs;

    tol_eas_incr_ =
        GLOBAL::Problem::Instance()->semi_smooth_plast_params().get<double>("TOLEASINCR");
    toltype_eas_incr_ = INPAR::STR::convnorm_abs;

    normcombo_disp_pres_ =
        CORE::UTILS::IntegralValue<INPAR::STR::BinaryOp>(sdynparams, "NORMCOMBI_DISPPRES");
    normcombo_fres_inco_ =
        CORE::UTILS::IntegralValue<INPAR::STR::BinaryOp>(sdynparams, "NORMCOMBI_RESFINCO");
    normcombo_fres_plast_res_ = CORE::UTILS::IntegralValue<INPAR::STR::BinaryOp>(
        GLOBAL::Problem::Instance()->semi_smooth_plast_params(), "NORMCOMBI_RESFPLASTCONSTR");
    normcombo_disp_plast_incr_ = CORE::UTILS::IntegralValue<INPAR::STR::BinaryOp>(
        GLOBAL::Problem::Instance()->semi_smooth_plast_params(), "NORMCOMBI_DISPPLASTINCR");
    normcombo_fres_eas_res_ = CORE::UTILS::IntegralValue<INPAR::STR::BinaryOp>(
        GLOBAL::Problem::Instance()->semi_smooth_plast_params(), "NORMCOMBI_EASRES");
    normcombo_disp_eas_incr_ = CORE::UTILS::IntegralValue<INPAR::STR::BinaryOp>(
        GLOBAL::Problem::Instance()->semi_smooth_plast_params(), "NORMCOMBI_EASINCR");
    normcombo_fres_disp_ =
        CORE::UTILS::IntegralValue<INPAR::STR::BinaryOp>(sdynparams, "NORMCOMBI_RESFDISP");

    // -------------------------------------------------------------------------
    // constraint variables
    // -------------------------------------------------------------------------
    tol_constr_res_ = sdynparams.get<double>("TOLCONSTR");
    toltype_constr_res_ = INPAR::STR::convnorm_abs;

    tol_constr_incr_ = sdynparams.get<double>("TOLCONSTRINCR");
    toltype_constr_incr_ = INPAR::STR::convnorm_abs;

    tol_cardvasc0d_res_ =
        GLOBAL::Problem::Instance()->cardiovascular0_d_structural_params().get<double>(
            "TOL_CARDVASC0D_RES");
    toltype_cardvasc0d_res_ = INPAR::STR::convnorm_abs;

    tol_cardvasc0d_incr_ =
        GLOBAL::Problem::Instance()->cardiovascular0_d_structural_params().get<double>(
            "TOL_CARDVASC0D_DOFINCR");
    toltype_cardvasc0d_incr_ = INPAR::STR::convnorm_abs;

    tol_contact_res_ =
        GLOBAL::Problem::Instance()->contact_dynamic_params().get<double>("TOLCONTCONSTR");
    toltype_contact_res_ = INPAR::STR::convnorm_abs;

    tol_contact_lm_incr_ =
        GLOBAL::Problem::Instance()->contact_dynamic_params().get<double>("TOLLAGR");
    toltype_contact_lm_incr_ = INPAR::STR::convnorm_abs;

    normcombo_fres_contact_res_ = CORE::UTILS::IntegralValue<INPAR::STR::BinaryOp>(
        GLOBAL::Problem::Instance()->contact_dynamic_params(), "NORMCOMBI_RESFCONTCONSTR");
    normcombo_disp_contact_lm_incr_ = CORE::UTILS::IntegralValue<INPAR::STR::BinaryOp>(
        GLOBAL::Problem::Instance()->contact_dynamic_params(), "NORMCOMBI_DISPLAGR");
  }

  {
    // store the structural dynamics parameter list for derived Setup routines
    sdynparams_ptr_ = Teuchos::rcpFromRef(sdynparams);
  }

  // -------------------------------------------------------------------------
  // initial displacement variables
  // -------------------------------------------------------------------------
  {
    initial_disp_ = CORE::UTILS::IntegralValue<INPAR::STR::InitialDisp>(sdynparams, "INITIALDISP");
    start_func_no_ = sdynparams.get<int>("STARTFUNCNO");
  }

  isinit_ = true;
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void STR::TIMINT::BaseDataSDyn::Setup()
{
  check_init();

  std::set<enum INPAR::STR::ModelType>::const_iterator it;
  // setup model type specific data containers
  for (it = (*modeltypes_).begin(); it != (*modeltypes_).end(); ++it)
  {
    switch (*it)
    {
      case INPAR::STR::model_beaminteraction:
      case INPAR::STR::model_beam_interaction_old:
      case INPAR::STR::model_browniandyn:
      {
        periodic_boundingbox_ = Teuchos::rcp(new CORE::GEO::MESHFREE::BoundingBox());
        periodic_boundingbox_->Init();
        periodic_boundingbox_->Setup();
        break;
      }
      default:
      {
        // nothing to do
        break;
      }
    }
  }

  issetup_ = true;
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
double STR::TIMINT::BaseDataSDyn::GetResTolerance(
    const enum NOX::NLN::StatusTest::QuantityType& qtype) const
{
  check_init_setup();
  switch (qtype)
  {
    case NOX::NLN::StatusTest::quantity_structure:
      return tol_fres_;
      break;
    case NOX::NLN::StatusTest::quantity_contact_normal:
    case NOX::NLN::StatusTest::quantity_contact_friction:
    case NOX::NLN::StatusTest::quantity_meshtying:
      return tol_contact_res_;
      break;
    case NOX::NLN::StatusTest::quantity_cardiovascular0d:
      return tol_cardvasc0d_res_;
      break;
    case NOX::NLN::StatusTest::quantity_lag_pen_constraint:
      return tol_constr_res_;
      break;
    case NOX::NLN::StatusTest::quantity_plasticity:
      return tol_plast_res_;
      break;
    case NOX::NLN::StatusTest::quantity_pressure:
      return tol_inco_;
      break;
    case NOX::NLN::StatusTest::quantity_eas:
      return tol_eas_res_;
      break;
    default:
      FOUR_C_THROW(
          "There is no residual tolerance for the given quantity type! "
          "(quantity: %s)",
          NOX::NLN::StatusTest::QuantityType2String(qtype).c_str());
      break;
  }

  return -1.0;
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
double STR::TIMINT::BaseDataSDyn::GetIncrTolerance(
    const enum NOX::NLN::StatusTest::QuantityType& qtype) const
{
  check_init_setup();
  switch (qtype)
  {
    case NOX::NLN::StatusTest::quantity_structure:
      return tol_disp_incr_;
      break;
    case NOX::NLN::StatusTest::quantity_contact_normal:
    case NOX::NLN::StatusTest::quantity_contact_friction:
    case NOX::NLN::StatusTest::quantity_meshtying:
      return tol_contact_lm_incr_;
      break;
    case NOX::NLN::StatusTest::quantity_cardiovascular0d:
      return tol_cardvasc0d_incr_;
      break;
    case NOX::NLN::StatusTest::quantity_lag_pen_constraint:
      return tol_constr_incr_;
      break;
    case NOX::NLN::StatusTest::quantity_plasticity:
      return tol_plast_incr_;
      break;
    case NOX::NLN::StatusTest::quantity_pressure:
      return tol_pres_;
      break;
    case NOX::NLN::StatusTest::quantity_eas:
      return tol_eas_incr_;
      break;
    default:
      FOUR_C_THROW(
          "There is no increment tolerance for the given quantity type! "
          "(quantity: %s)",
          NOX::NLN::StatusTest::QuantityType2String(qtype).c_str());
      break;
  }

  return -1.0;
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
enum INPAR::STR::ConvNorm STR::TIMINT::BaseDataSDyn::GetResToleranceType(
    const enum NOX::NLN::StatusTest::QuantityType& qtype) const
{
  check_init_setup();
  switch (qtype)
  {
    case NOX::NLN::StatusTest::quantity_structure:
      return toltype_fres_;
      break;
    case NOX::NLN::StatusTest::quantity_contact_normal:
    case NOX::NLN::StatusTest::quantity_contact_friction:
    case NOX::NLN::StatusTest::quantity_meshtying:
      return toltype_contact_res_;
      break;
    case NOX::NLN::StatusTest::quantity_cardiovascular0d:
      return toltype_cardvasc0d_res_;
      break;
    case NOX::NLN::StatusTest::quantity_lag_pen_constraint:
      return toltype_constr_res_;
      break;
    case NOX::NLN::StatusTest::quantity_plasticity:
      return toltype_plast_res_;
      break;
    case NOX::NLN::StatusTest::quantity_pressure:
      return toltype_inco_;
      break;
    case NOX::NLN::StatusTest::quantity_eas:
      return toltype_eas_res_;
      break;
    default:
      FOUR_C_THROW(
          "There is no residual tolerance type for the given quantity type! "
          "(quantity: %s)",
          NOX::NLN::StatusTest::QuantityType2String(qtype).c_str());
      break;
  }

  return INPAR::STR::convnorm_abs;
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
enum INPAR::STR::ConvNorm STR::TIMINT::BaseDataSDyn::get_incr_tolerance_type(
    const enum NOX::NLN::StatusTest::QuantityType& qtype) const
{
  check_init_setup();
  switch (qtype)
  {
    case NOX::NLN::StatusTest::quantity_structure:
      return toltype_disp_incr_;
      break;
    case NOX::NLN::StatusTest::quantity_contact_normal:
    case NOX::NLN::StatusTest::quantity_contact_friction:
    case NOX::NLN::StatusTest::quantity_meshtying:
      return toltype_contact_lm_incr_;
      break;
    case NOX::NLN::StatusTest::quantity_cardiovascular0d:
      return toltype_cardvasc0d_incr_;
      break;
    case NOX::NLN::StatusTest::quantity_lag_pen_constraint:
      return toltype_constr_incr_;
      break;
    case NOX::NLN::StatusTest::quantity_plasticity:
      return toltype_plast_incr_;
      break;
    case NOX::NLN::StatusTest::quantity_pressure:
      return toltype_pres_;
      break;
    case NOX::NLN::StatusTest::quantity_eas:
      return toltype_eas_incr_;
      break;
    default:
      FOUR_C_THROW(
          "There is no increment tolerance type for the given quantity type! "
          "(quantity: %s)",
          NOX::NLN::StatusTest::QuantityType2String(qtype).c_str());
      break;
  }

  return INPAR::STR::convnorm_abs;
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
enum INPAR::STR::BinaryOp STR::TIMINT::BaseDataSDyn::GetResComboType(
    const enum NOX::NLN::StatusTest::QuantityType& qtype) const
{
  return GetResComboType(NOX::NLN::StatusTest::quantity_structure, qtype);
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
enum INPAR::STR::BinaryOp STR::TIMINT::BaseDataSDyn::GetResComboType(
    const enum NOX::NLN::StatusTest::QuantityType& qtype_1,
    const enum NOX::NLN::StatusTest::QuantityType& qtype_2) const
{
  check_init_setup();
  // combination: STRUCTURE <--> PRESSURE
  if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
          qtype_2 == NOX::NLN::StatusTest::quantity_pressure) or
      (qtype_1 == NOX::NLN::StatusTest::quantity_pressure and
          qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_fres_inco_;
  // combination: STRUCTURE <--> EAS
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_eas) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_eas and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_fres_eas_res_;
  // combination: STRUCTURE <--> PLASTICITY
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_plasticity) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_plasticity and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_fres_plast_res_;
  // combination: STRUCTURE <--> CONTACT
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_contact_normal) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_contact_normal and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_fres_contact_res_;
  // combination: STRUCTURE <--> frictional CONTACT
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_contact_friction) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_contact_friction and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_fres_contact_res_;
  // combination: STRUCTURE <--> mesh tying
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_meshtying) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_meshtying and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_fres_contact_res_;
  // combination: STRUCTURE <--> CARDIOVASCULAR0D
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_cardiovascular0d) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_cardiovascular0d and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_fres_cardvasc0d_res_;
  // combination: STRUCTURE <--> LAG-PEN-CONSTRAINT
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_lag_pen_constraint) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_lag_pen_constraint and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_fres_constr_res_;
  // no combination was found
  else
    FOUR_C_THROW(
        "There is no combination type for the given quantity types! "
        "(quantity_1: %s, quantity_2: %s)",
        NOX::NLN::StatusTest::QuantityType2String(qtype_1).c_str(),
        NOX::NLN::StatusTest::QuantityType2String(qtype_2).c_str());

  return INPAR::STR::bop_and;
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
enum INPAR::STR::BinaryOp STR::TIMINT::BaseDataSDyn::GetIncrComboType(
    const enum NOX::NLN::StatusTest::QuantityType& qtype) const
{
  return GetIncrComboType(NOX::NLN::StatusTest::quantity_structure, qtype);
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
enum INPAR::STR::BinaryOp STR::TIMINT::BaseDataSDyn::GetIncrComboType(
    const enum NOX::NLN::StatusTest::QuantityType& qtype_1,
    const enum NOX::NLN::StatusTest::QuantityType& qtype_2) const
{
  check_init_setup();
  // combination: STRUCTURE <--> PRESSURE
  if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
          qtype_2 == NOX::NLN::StatusTest::quantity_pressure) or
      (qtype_1 == NOX::NLN::StatusTest::quantity_pressure and
          qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_disp_pres_;
  // combination: STRUCTURE <--> EAS
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_eas) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_eas and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_disp_eas_incr_;
  // combination: STRUCTURE <--> PLASTICITY
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_plasticity) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_plasticity and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_disp_plast_incr_;
  // combination: STRUCTURE <--> CONTACT
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_contact_normal) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_contact_normal and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_disp_contact_lm_incr_;
  // combination: STRUCTURE <--> frictional CONTACT
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_contact_friction) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_contact_friction and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_disp_contact_lm_incr_;
  // combination: STRUCTURE <--> mesh tying
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_meshtying) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_meshtying and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_disp_contact_lm_incr_;
  // combination: STRUCTURE <--> CARDIOVASCULAR0D
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_cardiovascular0d) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_cardiovascular0d and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_disp_cardvasc0d_incr_;
  // combination: STRUCTURE <--> LAG-PEN-CONSTRAINT
  else if ((qtype_1 == NOX::NLN::StatusTest::quantity_structure and
               qtype_2 == NOX::NLN::StatusTest::quantity_lag_pen_constraint) or
           (qtype_1 == NOX::NLN::StatusTest::quantity_lag_pen_constraint and
               qtype_2 == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_disp_constr_incr_;
  // no combination was found
  else
    FOUR_C_THROW(
        "There is no combination type for the given quantity types! "
        "(quantity_1: %s, quantity_2: %s)",
        NOX::NLN::StatusTest::QuantityType2String(qtype_1).c_str(),
        NOX::NLN::StatusTest::QuantityType2String(qtype_2).c_str());

  return INPAR::STR::bop_and;
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
enum INPAR::STR::BinaryOp STR::TIMINT::BaseDataSDyn::GetResIncrComboType(
    const enum NOX::NLN::StatusTest::QuantityType& qtype_res,
    const enum NOX::NLN::StatusTest::QuantityType& qtype_incr) const
{
  check_init_setup();
  // combination: STRUCTURE (force/res) <--> STRUCTURE (displ/incr)
  if ((qtype_res == NOX::NLN::StatusTest::quantity_structure and
          qtype_incr == NOX::NLN::StatusTest::quantity_structure))
    return normcombo_fres_disp_;
  // no combination was found
  else
    FOUR_C_THROW(
        "There is no res-incr-combination type for the given quantity types! "
        "(quantity_res: %s, quantity_incr: %s)",
        NOX::NLN::StatusTest::QuantityType2String(qtype_res).c_str(),
        NOX::NLN::StatusTest::QuantityType2String(qtype_incr).c_str());

  return INPAR::STR::bop_and;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
bool STR::TIMINT::BaseDataSDyn::HaveModelType(const INPAR::STR::ModelType& modeltype) const
{
  check_init_setup();
  return (GetModelTypes().find(modeltype) != GetModelTypes().end());
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
bool STR::TIMINT::BaseDataSDyn::HaveEleTech(const INPAR::STR::EleTech& eletech) const
{
  check_init_setup();
  return (get_element_technologies().find(eletech) != get_element_technologies().end());
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
STR::TIMINT::GenAlphaDataSDyn::GenAlphaDataSDyn()
    : midavg_(INPAR::STR::midavg_vague),
      beta_(-1.0),
      gamma_(-1.0),
      alphaf_(-1.0),
      alpham_(-1.0),
      rhoinf_(-1.0)
{
  // empty constructor
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void STR::TIMINT::GenAlphaDataSDyn::Setup()
{
  check_init();

  // call base class setup
  STR::TIMINT::BaseDataSDyn::Setup();

  midavg_ = CORE::UTILS::IntegralValue<INPAR::STR::MidAverageEnum>(
      get_s_dyn_params().sublist("GENALPHA"), "GENAVG");
  beta_ = get_s_dyn_params().sublist("GENALPHA").get<double>("BETA");
  gamma_ = get_s_dyn_params().sublist("GENALPHA").get<double>("GAMMA");
  alphaf_ = get_s_dyn_params().sublist("GENALPHA").get<double>("ALPHA_F");
  alpham_ = get_s_dyn_params().sublist("GENALPHA").get<double>("ALPHA_M");
  rhoinf_ = get_s_dyn_params().sublist("GENALPHA").get<double>("RHO_INF");

  issetup_ = true;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
STR::TIMINT::OneStepThetaDataSDyn::OneStepThetaDataSDyn() : theta_(-1.0)
{
  // empty constructor
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void STR::TIMINT::OneStepThetaDataSDyn::Setup()
{
  check_init();

  // call base class setup
  STR::TIMINT::BaseDataSDyn::Setup();

  theta_ = get_s_dyn_params().sublist("ONESTEPTHETA").get<double>("THETA");

  issetup_ = true;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
STR::TIMINT::ExplEulerDataSDyn::ExplEulerDataSDyn() : modexpleuler_(true)
{
  // empty constructor
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void STR::TIMINT::ExplEulerDataSDyn::Setup()
{
  check_init();

  // call base class setup
  STR::TIMINT::BaseDataSDyn::Setup();

  modexpleuler_ =
      (CORE::UTILS::IntegralValue<int>(
           GLOBAL::Problem::Instance()->structural_dynamic_params(), "MODIFIEDEXPLEULER") == 1);

  issetup_ = true;
}

FOUR_C_NAMESPACE_CLOSE