/*----------------------------------------------------------------------*/
/*! \file
 \brief input parameters for porous multiphase fluid problem

   \level 3

 *----------------------------------------------------------------------*/



#include "4C_inpar_porofluidmultiphase.hpp"

#include "4C_inpar_bio.hpp"
#include "4C_utils_parameter_list.hpp"

FOUR_C_NAMESPACE_OPEN

void Inpar::POROFLUIDMULTIPHASE::SetValidParameters(Teuchos::RCP<Teuchos::ParameterList> list)
{
  using namespace Input;
  using Teuchos::setStringToIntegralParameter;
  using Teuchos::tuple;

  Teuchos::ParameterList& porofluidmultiphasedyn = list->sublist("POROFLUIDMULTIPHASE DYNAMIC",
      false, "control parameters for porofluidmultiphase problems\n");

  Core::UTILS::DoubleParameter("MAXTIME", 1000.0, "Total simulation time", &porofluidmultiphasedyn);
  Core::UTILS::IntParameter("NUMSTEP", 20, "Total number of time steps", &porofluidmultiphasedyn);
  Core::UTILS::DoubleParameter("TIMESTEP", 0.1, "Time increment dt", &porofluidmultiphasedyn);
  Core::UTILS::IntParameter(
      "RESULTSEVRY", 1, "Increment for writing solution", &porofluidmultiphasedyn);
  Core::UTILS::IntParameter(
      "RESTARTEVRY", 1, "Increment for writing restart", &porofluidmultiphasedyn);

  Core::UTILS::DoubleParameter(
      "THETA", 0.5, "One-step-theta time integration factor", &porofluidmultiphasedyn);
  //  Core::UTILS::DoubleParameter("ALPHA_M",0.5,"Generalized-alpha time integration
  //  factor",&porofluidmultiphasedyn);
  //  Core::UTILS::DoubleParameter("ALPHA_F",0.5,"Generalized-alpha time integration
  //  factor",&porofluidmultiphasedyn); Core::UTILS::DoubleParameter("GAMMA",0.5,"Generalized-alpha
  //  time integration factor",&porofluidmultiphasedyn);

  setStringToIntegralParameter<int>("TIMEINTEGR", "One_Step_Theta", "Time Integration Scheme",
      tuple<std::string>("One_Step_Theta"), tuple<int>(timeint_one_step_theta),
      &porofluidmultiphasedyn);

  setStringToIntegralParameter<int>("CALCERROR", "No",
      "compute error compared to analytical solution",
      tuple<std::string>("No", "error_by_function"), tuple<int>(calcerror_no, calcerror_byfunction),
      &porofluidmultiphasedyn);

  Core::UTILS::IntParameter("CALCERRORNO", -1,
      "function number for porofluidmultiphase error computation", &porofluidmultiphasedyn);

  // linear solver id used for porofluidmultiphase problems
  Core::UTILS::IntParameter("LINEAR_SOLVER", -1,
      "number of linear solver used for the porofluidmultiphase problem", &porofluidmultiphasedyn);

  Core::UTILS::IntParameter(
      "ITEMAX", 10, "max. number of nonlin. iterations", &porofluidmultiphasedyn);
  Core::UTILS::DoubleParameter("ABSTOLRES", 1e-14,
      "Absolute tolerance for deciding if residual of nonlinear problem is already zero",
      &porofluidmultiphasedyn);

  // convergence criteria adaptivity
  Core::UTILS::BoolParameter("ADAPTCONV", "No",
      "Switch on adaptive control of linear solver tolerance for nonlinear solution",
      &porofluidmultiphasedyn);
  Core::UTILS::DoubleParameter("ADAPTCONV_BETTER", 0.1,
      "The linear solver shall be this much better than the current nonlinear residual in the "
      "nonlinear convergence limit",
      &porofluidmultiphasedyn);

  // parameters for finite difference check
  setStringToIntegralParameter<int>("FDCHECK", "none",
      "flag for finite difference check: none, local, or global",
      tuple<std::string>("none",
          "global"),  // perform finite difference check on time integrator level
      tuple<int>(fdcheck_none, fdcheck_global), &porofluidmultiphasedyn);
  Core::UTILS::DoubleParameter("FDCHECKEPS", 1.e-6,
      "dof perturbation magnitude for finite difference check (1.e-6 seems to work very well, "
      "whereas smaller values don't)",
      &porofluidmultiphasedyn);
  Core::UTILS::DoubleParameter("FDCHECKTOL", 1.e-6,
      "relative tolerance for finite difference check", &porofluidmultiphasedyn);
  Core::UTILS::BoolParameter("SKIPINITDER", "yes",
      "Flag to skip computation of initial time derivative", &porofluidmultiphasedyn);
  Core::UTILS::BoolParameter("OUTPUT_SATANDPRESS", "yes",
      "Flag if output of saturations and pressures should be calculated", &porofluidmultiphasedyn);
  Core::UTILS::BoolParameter("OUTPUT_SOLIDPRESS", "yes",
      "Flag if output of solid pressure should be calculated", &porofluidmultiphasedyn);
  Core::UTILS::BoolParameter("OUTPUT_POROSITY", "yes",
      "Flag if output of porosity should be calculated", &porofluidmultiphasedyn);
  Core::UTILS::BoolParameter("OUTPUT_PHASE_VELOCITIES", "yes",
      "Flag if output of phase velocities should be calculated", &porofluidmultiphasedyn);

  // Biot stabilization
  Core::UTILS::BoolParameter(
      "STAB_BIOT", "No", "Flag to (de)activate BIOT stabilization.", &porofluidmultiphasedyn);
  Core::UTILS::DoubleParameter("STAB_BIOT_SCALING", 1.0,
      "Scaling factor for stabilization parameter for biot stabilization of porous flow.",
      &porofluidmultiphasedyn);

  setStringToIntegralParameter<int>("VECTORNORM_RESF", "L2",
      "type of norm to be applied to residuals",
      tuple<std::string>("L1", "L1_Scaled", "L2", "Rms", "Inf"),
      tuple<int>(Inpar::POROFLUIDMULTIPHASE::norm_l1, Inpar::POROFLUIDMULTIPHASE::norm_l1_scaled,
          Inpar::POROFLUIDMULTIPHASE::norm_l2, Inpar::POROFLUIDMULTIPHASE::norm_rms,
          Inpar::POROFLUIDMULTIPHASE::norm_inf),
      &porofluidmultiphasedyn);

  setStringToIntegralParameter<int>("VECTORNORM_INC", "L2",
      "type of norm to be applied to residuals",
      tuple<std::string>("L1", "L1_Scaled", "L2", "Rms", "Inf"),
      tuple<int>(Inpar::POROFLUIDMULTIPHASE::norm_l1, Inpar::POROFLUIDMULTIPHASE::norm_l1_scaled,
          Inpar::POROFLUIDMULTIPHASE::norm_l2, Inpar::POROFLUIDMULTIPHASE::norm_rms,
          Inpar::POROFLUIDMULTIPHASE::norm_inf),
      &porofluidmultiphasedyn);

  // Iterationparameters
  Core::UTILS::DoubleParameter("TOLRES", 1e-6,
      "tolerance in the residual norm for the Newton iteration", &porofluidmultiphasedyn);
  Core::UTILS::DoubleParameter("TOLINC", 1e-6,
      "tolerance in the increment norm for the Newton iteration", &porofluidmultiphasedyn);

  setStringToIntegralParameter<int>("INITIALFIELD", "zero_field",
      "Initial Field for transport problem",
      tuple<std::string>("zero_field", "field_by_function", "field_by_condition"),
      tuple<int>(initfield_zero_field, initfield_field_by_function, initfield_field_by_condition),
      &porofluidmultiphasedyn);

  Core::UTILS::IntParameter("INITFUNCNO", -1, "function number for scalar transport initial field",
      &porofluidmultiphasedyn);

  setStringToIntegralParameter<int>("DIVERCONT", "stop",
      "What to do with time integration when Newton-Raphson iteration failed",
      tuple<std::string>("stop", "continue"), tuple<int>(divcont_stop, divcont_continue),
      &porofluidmultiphasedyn);

  Core::UTILS::IntParameter("FLUX_PROJ_SOLVER", -1,
      "Number of linear solver used for L2 projection", &porofluidmultiphasedyn);

  setStringToIntegralParameter<int>("FLUX_PROJ_METHOD", "none",
      "Flag to (de)activate flux reconstruction.",
      tuple<std::string>("none",
          //  "superconvergent_patch_recovery",
          "L2_projection"),
      tuple<std::string>("no gradient reconstruction",
          // "gradient reconstruction via superconvergent patch recovery",
          "gracient reconstruction via l2-projection"),
      tuple<int>(gradreco_none,  // no convective streamline edge-based stabilization
                                 //   gradreco_spr,    // convective streamline edge-based
                                 //   stabilization on the entire domain
          gradreco_l2  // pressure edge-based stabilization as ghost penalty around cut elements
          ),
      &porofluidmultiphasedyn);

  // functions used for domain integrals
  setNumericStringParameter(
      "DOMAININT_FUNCT", "-1.0", "functions used for domain integrals", &porofluidmultiphasedyn);

  // coupling with 1D artery network active
  Core::UTILS::BoolParameter(
      "ARTERY_COUPLING", "No", "Coupling with 1D blood vessels.", &porofluidmultiphasedyn);

  Core::UTILS::DoubleParameter("STARTING_DBC_TIME_END", -1.0,
      "End time for the starting Dirichlet BC.", &porofluidmultiphasedyn);

  setNumericStringParameter("STARTING_DBC_ONOFF", "0",
      "Switching the starting Dirichlet BC on or off.", &porofluidmultiphasedyn);

  setNumericStringParameter("STARTING_DBC_FUNCT", "0",
      "Function prescribing the starting Dirichlet BC.", &porofluidmultiphasedyn);

  // ----------------------------------------------------------------------
  // artery mesh tying
  Teuchos::ParameterList& porofluidmultiphasemshtdyn =
      porofluidmultiphasedyn.sublist("ARTERY COUPLING", false, "Parameters for artery mesh tying");

  // maximum number of segments per artery element for 1D-3D artery coupling
  Core::UTILS::IntParameter("MAXNUMSEGPERARTELE", 5,
      "maximum number of segments per artery element for 1D-3D artery coupling",
      &porofluidmultiphasemshtdyn);

  // penalty parameter
  Core::UTILS::DoubleParameter(
      "PENALTY", 1000.0, "Penalty parameter for line-based coupling", &porofluidmultiphasemshtdyn);

  setStringToIntegralParameter<int>("ARTERY_COUPLING_METHOD", "None",
      "Coupling method for artery coupling.",
      tuple<std::string>("None", "Nodal", "GPTS", "MP", "NTP"),
      tuple<std::string>("none", "Nodal Coupling", "Gauss-Point-To-Segment Approach",
          "Mortar Penalty Approach", "1D node-to-point in 2D/3D Approach"),
      tuple<int>(Inpar::ArteryNetwork::ArteryPoroMultiphaseScatraCouplingMethod::none,  // none
          Inpar::ArteryNetwork::ArteryPoroMultiphaseScatraCouplingMethod::nodal,  // Nodal Coupling
          Inpar::ArteryNetwork::ArteryPoroMultiphaseScatraCouplingMethod::
              gpts,  // Gauss-Point-To-Segment
                     // Approach
          Inpar::ArteryNetwork::ArteryPoroMultiphaseScatraCouplingMethod::mp,  // Mortar Penalty
                                                                               // Approach
          Inpar::ArteryNetwork::ArteryPoroMultiphaseScatraCouplingMethod::ntp  // 1Dnode-to-point in
                                                                               // 2D/3D
          ),
      &porofluidmultiphasemshtdyn);

  // coupled artery dofs for mesh tying
  setNumericStringParameter(
      "COUPLEDDOFS_ART", "-1.0", "coupled artery dofs for mesh tying", &porofluidmultiphasemshtdyn);

  // coupled porofluid dofs for mesh tying
  setNumericStringParameter("COUPLEDDOFS_PORO", "-1.0", "coupled porofluid dofs for mesh tying",
      &porofluidmultiphasemshtdyn);

  // functions for coupling (artery part)
  setNumericStringParameter(
      "REACFUNCT_ART", "-1", "functions for coupling (artery part)", &porofluidmultiphasemshtdyn);

  // scale for coupling (artery part)
  setNumericStringParameter(
      "SCALEREAC_ART", "0", "scale for coupling (artery part)", &porofluidmultiphasemshtdyn);

  // functions for coupling (porofluid part)
  setNumericStringParameter("REACFUNCT_CONT", "-1", "functions for coupling (porofluid part)",
      &porofluidmultiphasemshtdyn);

  // scale for coupling (porofluid part)
  setNumericStringParameter(
      "SCALEREAC_CONT", "0", "scale for coupling (porofluid part)", &porofluidmultiphasemshtdyn);

  // Flag if artery elements are evaluated in reference or current configuration
  Core::UTILS::BoolParameter("EVALUATE_IN_REF_CONFIG", "yes",
      "Flag if artery elements are evaluated in reference or current configuration",
      &porofluidmultiphasemshtdyn);

  // Flag if 1D-3D coupling should be evaluated on lateral (cylinder) surface of embedded artery
  // elements
  Core::UTILS::BoolParameter("LATERAL_SURFACE_COUPLING", "no",
      "Flag if 1D-3D coupling should be evaluated on lateral (cylinder) surface of embedded artery "
      "elements",
      &porofluidmultiphasemshtdyn);

  // Number of integration patches per 1D element in axial direction for lateral surface coupling
  Core::UTILS::IntParameter("NUMPATCH_AXI", 1,
      "Number of integration patches per 1D element in axial direction for lateral surface "
      "coupling",
      &porofluidmultiphasemshtdyn);

  // Number of integration patches per 1D element in radial direction for lateral surface coupling
  Core::UTILS::IntParameter("NUMPATCH_RAD", 1,
      "Number of integration patches per 1D element in radial direction for lateral surface "
      "coupling",
      &porofluidmultiphasemshtdyn);

  // Flag if blood vessel volume fraction should be output
  Core::UTILS::BoolParameter("OUTPUT_BLOODVESSELVOLFRAC", "no",
      "Flag if output of blood vessel volume fraction should be calculated",
      &porofluidmultiphasemshtdyn);

  // Flag if summary of coupling-pairs should be printed
  Core::UTILS::BoolParameter("PRINT_OUT_SUMMARY_PAIRS", "no",
      "Flag if summary of coupling-pairs should be printed", &porofluidmultiphasemshtdyn);

  // Flag if free-hanging elements (after blood vessel collapse) should be deleted
  Core::UTILS::BoolParameter("DELETE_FREE_HANGING_ELES", "no",
      "Flag if free-hanging elements (after blood vessel collapse) should be deleted",
      &porofluidmultiphasemshtdyn);

  // components whose size is smaller than this fraction of the total network size are also deleted
  Core::UTILS::DoubleParameter("DELETE_SMALL_FREE_HANGING_COMPS", -1.0,
      "Small connected components whose size is smaller than this fraction of the overall network "
      "size are additionally deleted (a valid choice of this parameter should lie between 0 and 1)",
      &porofluidmultiphasemshtdyn);
}

FOUR_C_NAMESPACE_CLOSE
