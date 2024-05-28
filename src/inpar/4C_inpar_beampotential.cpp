/*----------------------------------------------------------------------*/
/*! \file

\brief input parameter definitions for beam potential-based interactions

\level 3

*/
/*----------------------------------------------------------------------*/

#include "4C_inpar_beampotential.hpp"

#include "4C_inpar_beamcontact.hpp"
#include "4C_inpar_structure.hpp"
#include "4C_inpar_tsi.hpp"
#include "4C_io_condition_definition.hpp"
#include "4C_utils_parameter_list.hpp"

FOUR_C_NAMESPACE_OPEN



void INPAR::BEAMPOTENTIAL::SetValidParameters(Teuchos::RCP<Teuchos::ParameterList> list)
{
  using namespace INPUT;
  using Teuchos::setStringToIntegralParameter;
  using Teuchos::tuple;

  /* parameters for potential-based beam interaction */
  Teuchos::ParameterList& beampotential = list->sublist("BEAM POTENTIAL", false, "");

  setNumericStringParameter("POT_LAW_EXPONENT", "1.0",
      "negative(!) exponent(s)  \f$m_i\f$ of potential law "
      "\f$\\Phi(r) = \\sum_i (k_i * r^{-m_i}).\f$",
      &beampotential);
  setNumericStringParameter("POT_LAW_PREFACTOR", "0.0",
      "prefactor(s) \f$k_i\f$ of potential law \f$\\Phi(r) = \\sum_i (k_i * r^{-m_i})\f$.",
      &beampotential);

  setStringToIntegralParameter<int>("BEAMPOTENTIAL_TYPE", "Surface",
      "Type of potential interaction: surface (default) or volume potential",
      tuple<std::string>("Surface", "surface", "Volume", "volume"),
      tuple<int>(beampot_surf, beampot_surf, beampot_vol, beampot_vol), &beampotential);

  setStringToIntegralParameter<int>("STRATEGY", "DoubleLengthSpecific_LargeSepApprox",
      "strategy to evaluate interaction potential: double/single length specific, "
      "small/large separation approximation, ...",
      tuple<std::string>("DoubleLengthSpecific_LargeSepApprox",
          "DoubleLengthSpecific_SmallSepApprox", "SingleLengthSpecific_SmallSepApprox",
          "SingleLengthSpecific_SmallSepApprox_Simple"),
      tuple<int>(strategy_doublelengthspec_largesepapprox, strategy_doublelengthspec_smallsepapprox,
          strategy_singlelengthspec_smallsepapprox,
          strategy_singlelengthspec_smallsepapprox_simple),
      &beampotential);

  CORE::UTILS::DoubleParameter("CUTOFF_RADIUS", -1.0,
      "Neglect all potential contributions at separation larger"
      "than this cutoff radius",
      &beampotential);

  setStringToIntegralParameter<int>("REGULARIZATION_TYPE", "none",
      "Type of regularization applied to the force law",
      tuple<std::string>("linear_extrapolation", "constant_extrapolation", "None", "none"),
      tuple<int>(
          regularization_linear, regularization_constant, regularization_none, regularization_none),
      &beampotential);

  CORE::UTILS::DoubleParameter("REGULARIZATION_SEPARATION", -1.0,
      "Use regularization of force law at separations "
      "smaller than this separation",
      &beampotential);

  CORE::UTILS::IntParameter("NUM_INTEGRATION_SEGMENTS", 1,
      "Number of integration segments used per beam element", &beampotential);

  CORE::UTILS::IntParameter(
      "NUM_GAUSSPOINTS", 10, "Number of Gauss points used per integration segment", &beampotential);

  CORE::UTILS::BoolParameter("AUTOMATIC_DIFFERENTIATION", "No",
      "apply automatic differentiation via FAD?", &beampotential);

  setStringToIntegralParameter<MasterSlaveChoice>("CHOICE_MASTER_SLAVE", "smaller_eleGID_is_slave",
      "According to which rule shall the role of master and slave be assigned to beam elements?",
      tuple<std::string>("smaller_eleGID_is_slave", "higher_eleGID_is_slave"),
      tuple<MasterSlaveChoice>(
          MasterSlaveChoice::smaller_eleGID_is_slave, MasterSlaveChoice::higher_eleGID_is_slave),
      &beampotential);

  CORE::UTILS::BoolParameter("BEAMPOT_BTSOL", "No",
      "decide, whether potential-based interaction between beams and solids is considered",
      &beampotential);

  CORE::UTILS::BoolParameter("BEAMPOT_BTSPH", "No",
      "decide, whether potential-based interaction between beams and spheres is considered",
      &beampotential);

  // enable octree search and determine type of bounding box (aabb = axis aligned, spbb = spherical)
  setStringToIntegralParameter<int>("BEAMPOT_OCTREE", "None",
      "octree and bounding box type for octree search routine",
      tuple<std::string>(
          "None", "none", "octree_axisaligned", "octree_cylorient", "octree_spherical"),
      tuple<int>(INPAR::BEAMCONTACT::boct_none, INPAR::BEAMCONTACT::boct_none,
          INPAR::BEAMCONTACT::boct_aabb, INPAR::BEAMCONTACT::boct_cobb,
          INPAR::BEAMCONTACT::boct_spbb),
      &beampotential);

  CORE::UTILS::IntParameter(
      "BEAMPOT_TREEDEPTH", 6, "max, tree depth of the octree", &beampotential);
  CORE::UTILS::IntParameter(
      "BEAMPOT_BOXESINOCT", 8, "max number of bounding boxes in any leaf octant", &beampotential);

  /*------------------------------------------------------------------------*/
  /* parameters for visualization of potential-based beam interactions via output at runtime */

  Teuchos::ParameterList& beampotential_output_sublist =
      beampotential.sublist("RUNTIME VTK OUTPUT", false, "");


  // whether to write visualization output for beam contact
  CORE::UTILS::BoolParameter("VTK_OUTPUT_BEAM_POTENTIAL", "No",
      "write visualization output for potential-based beam interactions",
      &beampotential_output_sublist);

  // output interval regarding steps: write output every INTERVAL_STEPS steps
  CORE::UTILS::IntParameter("INTERVAL_STEPS", -1,
      "write output at runtime every INTERVAL_STEPS steps", &beampotential_output_sublist);

  // whether to write output in every iteration of the nonlinear solver
  CORE::UTILS::BoolParameter("EVERY_ITERATION", "No",
      "write output in every iteration of the nonlinear solver", &beampotential_output_sublist);

  // whether to write visualization output for forces
  CORE::UTILS::BoolParameter(
      "FORCES", "No", "write visualization output for forces", &beampotential_output_sublist);

  // whether to write visualization output for moments
  CORE::UTILS::BoolParameter(
      "MOMENTS", "No", "write visualization output for moments", &beampotential_output_sublist);

  // whether to write visualization output for forces/moments separately for each element pair
  CORE::UTILS::BoolParameter("WRITE_FORCE_MOMENT_PER_ELEMENTPAIR", "No",
      "write visualization output for forces/moments separately for each element pair",
      &beampotential_output_sublist);
}

void INPAR::BEAMPOTENTIAL::SetValidConditions(
    std::vector<Teuchos::RCP<INPUT::ConditionDefinition>>& condlist)
{
  using namespace INPUT;

  /*-------------------------------------------------------------------*/
  // beam potential interaction: atom/charge density per unit length on LINE
  Teuchos::RCP<ConditionDefinition> rigidsphere_potential_charge =
      Teuchos::rcp(new ConditionDefinition("DESIGN POINT RIGIDSPHERE POTENTIAL CHARGE CONDITIONS",
          "RigidspherePotentialPointCharge", "Rigidsphere_Potential_Point_Charge",
          CORE::Conditions::RigidspherePotential_PointCharge, false,
          CORE::Conditions::geometry_type_point));

  Teuchos::RCP<ConditionDefinition> beam_potential_line_charge =
      Teuchos::rcp(new ConditionDefinition("DESIGN LINE BEAM POTENTIAL CHARGE CONDITIONS",
          "BeamPotentialLineCharge", "Beam_Potential_Line_Charge_Density",
          CORE::Conditions::BeamPotential_LineChargeDensity, false,
          CORE::Conditions::geometry_type_line));

  rigidsphere_potential_charge->AddComponent(Teuchos::rcp(new INPUT::SeparatorComponent("POTLAW")));
  rigidsphere_potential_charge->AddComponent(Teuchos::rcp(new INPUT::IntComponent("potlaw")));
  rigidsphere_potential_charge->AddComponent(Teuchos::rcp(new INPUT::SeparatorComponent("VAL")));
  rigidsphere_potential_charge->AddComponent(Teuchos::rcp(new INPUT::RealComponent("val")));
  rigidsphere_potential_charge->AddComponent(Teuchos::rcp(new INPUT::SeparatorComponent("FUNCT")));
  rigidsphere_potential_charge->AddComponent(
      Teuchos::rcp(new INPUT::IntComponent("funct", {0, false, true, true})));

  beam_potential_line_charge->AddComponent(Teuchos::rcp(new INPUT::SeparatorComponent("POTLAW")));
  beam_potential_line_charge->AddComponent(Teuchos::rcp(new INPUT::IntComponent("potlaw")));
  beam_potential_line_charge->AddComponent(Teuchos::rcp(new INPUT::SeparatorComponent("VAL")));
  beam_potential_line_charge->AddComponent(Teuchos::rcp(new INPUT::RealComponent("val")));
  beam_potential_line_charge->AddComponent(Teuchos::rcp(new INPUT::SeparatorComponent("FUNCT")));
  beam_potential_line_charge->AddComponent(
      Teuchos::rcp(new INPUT::IntComponent("funct", {0, false, true, true})));

  condlist.push_back(rigidsphere_potential_charge);
  condlist.push_back(beam_potential_line_charge);
}

FOUR_C_NAMESPACE_CLOSE