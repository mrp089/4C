/*----------------------------------------------------------------------*/
/*! \file

\brief Deprecated structure field adapter
       (see ad_str_structure_new.H/.cpp for the new version)

\level 1

*/
/*----------------------------------------------------------------------*/

#include "baci_adapter_str_structure.H"

#include "baci_adapter_str_constr_merged.H"
#include "baci_adapter_str_fpsiwrapper.H"
#include "baci_adapter_str_fsi_timint_adaptive.H"
#include "baci_adapter_str_fsiwrapper_immersed.H"
#include "baci_adapter_str_lung.H"
#include "baci_adapter_str_redairway.H"
#include "baci_adapter_str_ssiwrapper.H"
#include "baci_adapter_str_timeloop.H"
#include "baci_adapter_str_timint_adaptive.H"
#include "baci_adapter_str_wrapper.H"
#include "baci_comm_utils.H"
#include "baci_global_data.H"
#include "baci_inpar_contact.H"
#include "baci_inpar_fsi.H"
#include "baci_inpar_poroelast.H"
#include "baci_inpar_structure.H"
#include "baci_inpar_validparameters.H"
#include "baci_io.H"
#include "baci_io_control.H"
#include "baci_io_pstream.H"
#include "baci_linalg_multiply.H"
#include "baci_linalg_utils_sparse_algebra_create.H"
#include "baci_linalg_utils_sparse_algebra_manipulation.H"
#include "baci_linear_solver_method_linalg.H"
#include "baci_mat_par_bundle.H"
#include "baci_structure_timada_create.H"
#include "baci_structure_timint_create.H"
#include "baci_structure_timint_impl.H"

#include <Teuchos_StandardParameterEntryValidators.hpp>
#include <Teuchos_Time.hpp>
#include <Teuchos_TimeMonitor.hpp>

BACI_NAMESPACE_OPEN

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
ADAPTER::StructureBaseAlgorithm::StructureBaseAlgorithm(const Teuchos::ParameterList& prbdyn,
    const Teuchos::ParameterList& sdyn, Teuchos::RCP<DRT::Discretization> actdis)
{
  CreateStructure(prbdyn, sdyn, actdis);
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::StructureBaseAlgorithm::CreateStructure(const Teuchos::ParameterList& prbdyn,
    const Teuchos::ParameterList& sdyn, Teuchos::RCP<DRT::Discretization> actdis)
{
  // major switch to different time integrators
  switch (INPUT::IntegralValue<INPAR::STR::DynamicType>(sdyn, "DYNAMICTYP"))
  {
    case INPAR::STR::dyna_statics:
    case INPAR::STR::dyna_genalpha:
    case INPAR::STR::dyna_onesteptheta:
    case INPAR::STR::dyna_gemm:
    case INPAR::STR::dyna_expleuler:
    case INPAR::STR::dyna_centrdiff:
    case INPAR::STR::dyna_ab2:
    case INPAR::STR::dyna_euma:
    case INPAR::STR::dyna_euimsto:
      CreateTimInt(prbdyn, sdyn, actdis);  // <-- here is the show
      break;
    default:
      dserror("unknown time integration scheme '%s'", sdyn.get<std::string>("DYNAMICTYP").c_str());
      break;
  }
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::StructureBaseAlgorithm::CreateTimInt(const Teuchos::ParameterList& prbdyn,
    const Teuchos::ParameterList& sdyn, Teuchos::RCP<DRT::Discretization> actdis)
{
  // this is not exactly a one hundred meter race, but we need timing
  Teuchos::RCP<Teuchos::Time> t =
      Teuchos::TimeMonitor::getNewTimer("ADAPTER::StructureTimIntBaseAlgorithm::SetupStructure");
  Teuchos::TimeMonitor monitor(*t);

  // get the problem instance
  GLOBAL::Problem* problem = GLOBAL::Problem::Instance();
  // what's the current problem type?
  GLOBAL::ProblemType probtype = problem->GetProblemType();

  // get mortar information
  std::vector<DRT::Condition*> mtcond(0);
  std::vector<DRT::Condition*> ccond(0);
  actdis->GetCondition("Mortar", mtcond);
  actdis->GetCondition("Contact", ccond);
  bool onlymeshtying = false;
  bool onlycontact = false;
  bool meshtyingandcontact = false;
  if (mtcond.size() != 0 and ccond.size() != 0) meshtyingandcontact = true;
  if (mtcond.size() != 0 and ccond.size() == 0) onlymeshtying = true;
  if (mtcond.size() == 0 and ccond.size() != 0) onlycontact = true;

  // Problem-types involving changing mesh or redistribution of mesh
  // for load balancing (like contact) during the simulation needs an additional step.
  // This is because the discretization read from the input file
  // do not match with the discr. at the current time step.
  // Here we read the discretization at the current time step from restart files
  if (not actdis->Filled() || not actdis->HaveDofs()) actdis->FillComplete();

  // get input parameter lists and copy them, because a few parameters are overwritten
  // const Teuchos::ParameterList& probtype
  //  = problem->ProblemTypeParams();
  Teuchos::RCP<Teuchos::ParameterList> ioflags =
      Teuchos::rcp(new Teuchos::ParameterList(problem->IOParams()));
  Teuchos::RCP<Teuchos::ParameterList> tap =
      Teuchos::rcp(new Teuchos::ParameterList(sdyn.sublist("TIMEADAPTIVITY")));
  Teuchos::RCP<Teuchos::ParameterList> snox =
      Teuchos::rcp(new Teuchos::ParameterList(problem->StructuralNoxParams()));

  // show default parameters
  if ((actdis->Comm()).MyPID() == 0) INPUT::PrintDefaultParameters(IO::cout, sdyn);

  // add extra parameters (a kind of work-around)
  Teuchos::RCP<Teuchos::ParameterList> xparams = Teuchos::rcp(new Teuchos::ParameterList());
  Teuchos::ParameterList& nox = xparams->sublist("NOX");
  nox = *snox;

  // Check if for chosen Rayleigh damping the regarding parameters are given explicitly in the .dat
  // file
  if (INPUT::IntegralValue<INPAR::STR::DampKind>(sdyn, "DAMPING") == INPAR::STR::damp_rayleigh)
  {
    if (sdyn.get<double>("K_DAMP") < 0.0)
    {
      dserror("Rayleigh damping parameter K_DAMP not explicitly given.");
    }
    if (sdyn.get<double>("M_DAMP") < 0.0)
    {
      dserror("Rayleigh damping parameter M_DAMP not explicitly given.");
    }
  }

  // create a solver
  Teuchos::RCP<CORE::LINALG::Solver> solver = CreateLinearSolver(actdis, sdyn);

  // create contact/meshtying solver only if contact/meshtying problem.
  Teuchos::RCP<CORE::LINALG::Solver> contactsolver = Teuchos::null;

  if (onlymeshtying or onlycontact or meshtyingandcontact)
    contactsolver = CreateContactMeshtyingSolver(actdis, sdyn);

  if (solver != Teuchos::null && (solver->Params().isSublist("Belos Parameters")) &&
      solver->Params().isSublist("ML Parameters")  // TODO what about MueLu?
      && INPUT::IntegralValue<INPAR::STR::STC_Scale>(sdyn, "STC_SCALING") != INPAR::STR::stc_none)
  {
    Teuchos::ParameterList& mllist = solver->Params().sublist("ML Parameters");
    Teuchos::RCP<std::vector<double>> ns =
        mllist.get<Teuchos::RCP<std::vector<double>>>("nullspace");

    const int size = actdis->DofRowMap()->NumMyElements();

    // extract the six nullspace vectors corresponding to the modes
    // trans x, trans y, trans z, rot x, rot y, rot z
    // Note: We assume 3d here!

    Teuchos::RCP<Epetra_Vector> nsv1 =
        Teuchos::rcp(new Epetra_Vector(View, *(actdis->DofRowMap()), ns->data()));
    Teuchos::RCP<Epetra_Vector> nsv2 =
        Teuchos::rcp(new Epetra_Vector(View, *(actdis->DofRowMap()), &ns->at(size)));
    Teuchos::RCP<Epetra_Vector> nsv3 =
        Teuchos::rcp(new Epetra_Vector(View, *(actdis->DofRowMap()), &ns->at(2 * size)));
    Teuchos::RCP<Epetra_Vector> nsv4 =
        Teuchos::rcp(new Epetra_Vector(View, *(actdis->DofRowMap()), &ns->at(3 * size)));
    Teuchos::RCP<Epetra_Vector> nsv5 =
        Teuchos::rcp(new Epetra_Vector(View, *(actdis->DofRowMap()), &ns->at(4 * size)));
    Teuchos::RCP<Epetra_Vector> nsv6 =
        Teuchos::rcp(new Epetra_Vector(View, *(actdis->DofRowMap()), &ns->at(5 * size)));


    // prepare matrix for scaled thickness business of thin shell structures
    Teuchos::RCP<CORE::LINALG::SparseMatrix> stcinv =
        Teuchos::rcp(new CORE::LINALG::SparseMatrix(*actdis->DofRowMap(), 81, true, true));

    stcinv->Zero();
    // create the parameters for the discretization
    Teuchos::ParameterList p;
    // action for elements
    const std::string action = "calc_stc_matrix_inverse";
    p.set("action", action);
    p.set<int>("stc_scaling", INPUT::IntegralValue<INPAR::STR::STC_Scale>(sdyn, "STC_SCALING"));
    p.set("stc_layer", 1);

    actdis->Evaluate(p, stcinv, Teuchos::null, Teuchos::null, Teuchos::null, Teuchos::null);

    stcinv->Complete();

    for (int lay = 2; lay <= sdyn.get<int>("STC_LAYER"); ++lay)
    {
      Teuchos::ParameterList pe;

      p.set("stc_layer", lay);

      Teuchos::RCP<CORE::LINALG::SparseMatrix> tmpstcmat =
          Teuchos::rcp(new CORE::LINALG::SparseMatrix(*actdis->DofRowMap(), 81, true, true));
      tmpstcmat->Zero();

      actdis->Evaluate(p, tmpstcmat, Teuchos::null, Teuchos::null, Teuchos::null, Teuchos::null);
      tmpstcmat->Complete();

      stcinv = MLMultiply(*stcinv, *tmpstcmat, false, false, true);
    }

    Teuchos::RCP<Epetra_Vector> temp = CORE::LINALG::CreateVector(*(actdis->DofRowMap()), false);

    stcinv->Multiply(false, *nsv1, *temp);
    nsv1->Update(1.0, *temp, 0.0);
    stcinv->Multiply(false, *nsv2, *temp);
    nsv2->Update(1.0, *temp, 0.0);
    stcinv->Multiply(false, *nsv3, *temp);
    nsv3->Update(1.0, *temp, 0.0);
    stcinv->Multiply(false, *nsv4, *temp);
    nsv4->Update(1.0, *temp, 0.0);
    stcinv->Multiply(false, *nsv5, *temp);
    nsv5->Update(1.0, *temp, 0.0);
    stcinv->Multiply(false, *nsv6, *temp);
    nsv6->Update(1.0, *temp, 0.0);
  }

  // Checks in case of multi-scale simulations
  {
    // make sure we IMR-like generalised-alpha requested for multi-scale
    // simulations
    Teuchos::RCP<MAT::PAR::Bundle> materials = problem->Materials();
    for (std::map<int, Teuchos::RCP<MAT::PAR::Material>>::const_iterator i =
             materials->Map()->begin();
         i != materials->Map()->end(); ++i)
    {
      Teuchos::RCP<MAT::PAR::Material> mat = i->second;
      if (mat->Type() == INPAR::MAT::m_struct_multiscale)
      {
        if (INPUT::IntegralValue<INPAR::STR::DynamicType>(sdyn, "DYNAMICTYP") !=
            INPAR::STR::dyna_genalpha)
          dserror("In multi-scale simulations, you have to use DYNAMICTYP=GenAlpha");
        else if (INPUT::IntegralValue<INPAR::STR::MidAverageEnum>(
                     sdyn.sublist("GENALPHA"), "GENAVG") != INPAR::STR::midavg_trlike)
          dserror(
              "In multi-scale simulations, you have to use DYNAMICTYP=GenAlpha with GENAVG=TrLike");
        break;
      }
    }
  }

  // context for output and restart
  Teuchos::RCP<IO::DiscretizationWriter> output = actdis->Writer();
  if (INPUT::IntegralValue<int>(*ioflags, "OUTPUT_BIN"))
  {
    output->WriteMesh(0, 0.0);
  }

  // create marching time integrator
  Teuchos::RCP<STR::TimInt> tmpstr =
      STR::TimIntCreate(prbdyn, *ioflags, sdyn, *xparams, actdis, solver, contactsolver, output);
  // initialize the time integrator
  tmpstr->Init(prbdyn, sdyn, *xparams, actdis, solver);

  // ---------------------------------------------------------------------------
  // ---------------------------------------------------------------------------
  /* Overwrite certain parameters in STRUCTURAL DYNAMIC/TIMADAPTIVITY by those from
   * FSI DYNAMIC/TIMEADAPTIVITY
   *
   * In case, that the structure field is part of an FSI simulation with time step
   * size adaptivity based on structure field error estimation, we have to provide
   * the following algorithmic control parameters:
   *
   * - ADAPTSTEPMAX
   * - STEPSIZEMAX
   * - STEPSIZEMIN
   * - SIZERATIOMAX
   * - SIZERATIOMIN
   * - SIZERATIOSCALE
   *
   * They are specified by the FSI algorithm since they have to be the same for
   * the structure and fluid field. Hence, we overwrite the corresponding
   * parameters in the structural parameter list in order to avoid redundant
   * parameter specification in the input file.
   *
   * Note: This is really ugly, but currently the only way to avoid that the user
   * has to specify these parameters twice in the input file.
   *
   * ToDO: Find something nicer here!
   *
   * \author mayr.mt \date 12/2013
   */
  // ---------------------------------------------------------------------------
  if (probtype == GLOBAL::ProblemType::fsi or probtype == GLOBAL::ProblemType::fsi_redmodels)
  {
    const Teuchos::ParameterList& fsidyn = problem->FSIDynamicParams();
    const Teuchos::ParameterList& fsiada = fsidyn.sublist("TIMEADAPTIVITY");
    if (INPUT::IntegralValue<bool>(fsiada, "TIMEADAPTON"))
    {
      // overrule time step size adaptivity control parameters
      if (tap->get<std::string>("KIND") != "NONE")
      {
        tap->set<int>("ADAPTSTEPMAX", fsiada.get<int>("ADAPTSTEPMAX"));
        tap->set<double>("STEPSIZEMAX", fsiada.get<double>("DTMAX"));
        tap->set<double>("STEPSIZEMIN", fsiada.get<double>("DTMIN"));
        tap->set<double>("SIZERATIOMAX", fsiada.get<double>("SIZERATIOMAX"));
        tap->set<double>("SIZERATIOMIN", fsiada.get<double>("SIZERATIOMIN"));
        tap->set<double>("SIZERATIOSCALE", fsiada.get<double>("SAFETYFACTOR"));

        if (actdis->Comm().MyPID() == 0)
        {
          IO::cout
              << "*** Due to FSI time step size adaptivity with structure based error estimation,\n"
                 "algorithmic control parameters in STRUCTURAL DYNAMIC/TIMEADAPTIVITY have been\n"
                 "overwritten by those from FSI DYNAMIC/TIMEADAPTIVITY."
              << IO::endl
              << IO::endl;
        }
      }
    }
  }
  // ---------------------------------------------------------------------------
  // ---------------------------------------------------------------------------

  // create auxiliary time integrator, can be seen as a wrapper for tmpstr
  Teuchos::RCP<STR::TimAda> sta = STR::TimAdaCreate(*ioflags, prbdyn, sdyn, *xparams, *tap, tmpstr);

  if (sta != Teuchos::null and tmpstr != Teuchos::null)
  {
    switch (probtype)
    {
      case GLOBAL::ProblemType::structure:  // pure structural time adaptivity
      {
        structure_ = Teuchos::rcp(new StructureTimIntAda(sta, tmpstr));
        break;
      }
      case GLOBAL::ProblemType::fsi:  // structure based time adaptivity within an FSI simulation
      case GLOBAL::ProblemType::fsi_redmodels:
      {
        if ((actdis->Comm()).MyPID() == 0)
          IO::cout << "Using StructureNOXCorrectionWrapper()..." << IO::endl;

        Teuchos::RCP<FSIStructureWrapper> fsiwrapperwithadaptivity =
            Teuchos::rcp(new StructureFSITimIntAda(
                sta, Teuchos::rcp(new StructureNOXCorrectionWrapper(tmpstr))));
        // strTeuchos::rcp_dynamic_cast<StructureFSITimIntAda>(fsiwrapperwithadaptivity)->GetStrTimIntPtr();
        structure_ = fsiwrapperwithadaptivity;
        // structure_->GetStrTimIntPtr()-(prbdyn,sdyn,*xparams,actdis,solver);
        break;
      }
      default:
      {
        dserror(
            "Adaptive time integration for the structure not implemented for desired problem "
            "type.");
        break;
      }
    }
  }
  else if (sta == Teuchos::null and tmpstr != Teuchos::null)
  {
    switch (probtype)
    {
      case GLOBAL::ProblemType::fsi:
      case GLOBAL::ProblemType::fsi_redmodels:
      case GLOBAL::ProblemType::fsi_lung:
      case GLOBAL::ProblemType::gas_fsi:
      case GLOBAL::ProblemType::ac_fsi:
      case GLOBAL::ProblemType::biofilm_fsi:
      case GLOBAL::ProblemType::thermo_fsi:
      {
        const Teuchos::ParameterList& fsidyn = problem->FSIDynamicParams();
        const int coupling = INPUT::IntegralValue<int>(fsidyn, "COUPALGO");

        if ((actdis->Comm()).MyPID() == 0)
          IO::cout << "Using StructureNOXCorrectionWrapper()..." << IO::endl;

        if (tmpstr->HaveConstraint())
        {
          if (coupling == fsi_iter_constr_monolithicstructuresplit or
              coupling == fsi_iter_constr_monolithicfluidsplit)
            structure_ = Teuchos::rcp(
                new FSIStructureWrapper(Teuchos::rcp(new StructureNOXCorrectionWrapper(tmpstr))));
          else
            structure_ = Teuchos::rcp(
                new StructureConstrMerged(Teuchos::rcp(new StructureNOXCorrectionWrapper(tmpstr))));
        }
        else
        {
          if (coupling == fsi_iter_lung_monolithicstructuresplit or
              coupling == fsi_iter_lung_monolithicfluidsplit)
            structure_ = Teuchos::rcp(
                new StructureLung(Teuchos::rcp(new StructureNOXCorrectionWrapper(tmpstr))));
          else
            structure_ = Teuchos::rcp(
                new FSIStructureWrapper(Teuchos::rcp(new StructureNOXCorrectionWrapper(tmpstr))));
        }
      }
      break;
      case GLOBAL::ProblemType::immersed_fsi:
      {
        structure_ = Teuchos::rcp(new FSIStructureWrapperImmersed(tmpstr));
      }
      break;
      case GLOBAL::ProblemType::ssi:
      case GLOBAL::ProblemType::ssti:
      {
        structure_ = Teuchos::rcp(new SSIStructureWrapper(tmpstr));
      }
      break;
      case GLOBAL::ProblemType::redairways_tissue:
      {
        structure_ = Teuchos::rcp(new StructureRedAirway(tmpstr));
      }
      break;
      case GLOBAL::ProblemType::poroelast:
      case GLOBAL::ProblemType::poroscatra:
      case GLOBAL::ProblemType::fpsi:
      case GLOBAL::ProblemType::fps3i:
      case GLOBAL::ProblemType::fpsi_xfem:
      case GLOBAL::ProblemType::fsi_xfem:
      {
        const Teuchos::ParameterList& porodyn = problem->PoroelastDynamicParams();
        const INPAR::POROELAST::SolutionSchemeOverFields coupling =
            INPUT::IntegralValue<INPAR::POROELAST::SolutionSchemeOverFields>(porodyn, "COUPALGO");
        if (tmpstr->HaveConstraint())
        {
          if (coupling == INPAR::POROELAST::Monolithic_structuresplit or
              coupling == INPAR::POROELAST::Monolithic_fluidsplit or
              coupling == INPAR::POROELAST::Monolithic_nopenetrationsplit)
            structure_ = Teuchos::rcp(new FPSIStructureWrapper(tmpstr));
          else
            structure_ = Teuchos::rcp(new StructureConstrMerged(tmpstr));
        }
        else
        {
          structure_ = Teuchos::rcp(new FPSIStructureWrapper(tmpstr));
        }
      }
      break;
      case GLOBAL::ProblemType::struct_ale:
      {
        structure_ = Teuchos::rcp(new FSIStructureWrapper(tmpstr));
      }
      break;
      default:
      {
        /// wrap time loop for pure structure problems
        structure_ = (Teuchos::rcp(new StructureTimeLoop(tmpstr)));
      }
      break;
    }
  }
  else
  {
    dserror("no proper time integration found");
  }
  // see you
  return;
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<CORE::LINALG::Solver> ADAPTER::StructureBaseAlgorithm::CreateLinearSolver(
    Teuchos::RCP<DRT::Discretization>& actdis, const Teuchos::ParameterList& sdyn)
{
  Teuchos::RCP<CORE::LINALG::Solver> solver = Teuchos::null;

  // get the solver number used for structural problems
  const int linsolvernumber = sdyn.get<int>("LINEAR_SOLVER");
  // check if the structural solver has a valid solver number
  if (linsolvernumber == (-1))
    dserror(
        "no linear solver defined for structural field. Please set LINEAR_SOLVER in STRUCTURAL "
        "DYNAMIC to a valid number!");

  solver = Teuchos::rcp(new CORE::LINALG::Solver(
      GLOBAL::Problem::Instance()->SolverParams(linsolvernumber), actdis->Comm()));

  actdis->ComputeNullSpaceIfNecessary(solver->Params());

  return solver;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<CORE::LINALG::Solver> ADAPTER::StructureBaseAlgorithm::CreateContactMeshtyingSolver(
    Teuchos::RCP<DRT::Discretization>& actdis, const Teuchos::ParameterList& sdyn)
{
  Teuchos::RCP<CORE::LINALG::Solver> solver = Teuchos::null;

  // Get mortar information: contact or meshtying or both?
  bool onlymeshtying = false;
  bool onlycontact = false;
  bool meshtyingandcontact = false;
  {
    std::vector<DRT::Condition*> mtcond(0);
    std::vector<DRT::Condition*> ccond(0);
    actdis->GetCondition("Mortar", mtcond);
    actdis->GetCondition("Contact", ccond);
    if (mtcond.size() != 0 and ccond.size() != 0) meshtyingandcontact = true;
    if (mtcond.size() != 0 and ccond.size() == 0) onlymeshtying = true;
    if (mtcond.size() == 0 and ccond.size() != 0) onlycontact = true;
  }
  const Teuchos::ParameterList& mcparams = GLOBAL::Problem::Instance()->ContactDynamicParams();

  // Get the solver number used for meshtying/contact problems
  const int linsolvernumber = mcparams.get<int>("LINEAR_SOLVER");
  // check if the meshtying/contact solver has a valid solver number
  if (linsolvernumber == (-1))
    dserror(
        "No linear solver defined for meshtying/contact problem. Please set LINEAR_SOLVER in "
        "CONTACT DYNAMIC to a valid number!");

  // Distinguish the system type, i.e. condensed vs. saddle-point
  switch (INPUT::IntegralValue<int>(mcparams, "SYSTEM"))
  {
    case INPAR::CONTACT::system_saddlepoint:
    {
      /* Plausibility check
       *
       * Solver can be either a direct solver (UMFPACK, Superlu) or an iterative solver (Belos).
       */
      const auto sol = Teuchos::getIntegralValue<INPAR::SOLVER::SolverType>(
          GLOBAL::Problem::Instance()->SolverParams(linsolvernumber), "SOLVER");
      const auto prec = Teuchos::getIntegralValue<INPAR::SOLVER::PreconditionerType>(
          GLOBAL::Problem::Instance()->SolverParams(linsolvernumber), "AZPREC");
      if (sol != INPAR::SOLVER::SolverType::umfpack && sol != INPAR::SOLVER::SolverType::superlu)
      {
        // if an iterative solver is chosen we need a block preconditioner
        if (prec != INPAR::SOLVER::PreconditionerType::cheap_simple &&
            prec != INPAR::SOLVER::PreconditionerType::multigrid_muelu_contactsp)
          dserror(
              "You have chosen an iterative linear solver. For mortar meshtying/contact problems "
              "in saddle-point formulation, a block preconditioner is required. Choose an "
              "appropriate block preconditioner such as CheapSIMPLE or MueLu_contactSP "
              "(if MueLu is available) in the SOLVER %i block in your input file.",
              linsolvernumber);
      }

      // build meshtying/contact solver
      solver = Teuchos::rcp(new CORE::LINALG::Solver(
          GLOBAL::Problem::Instance()->SolverParams(linsolvernumber), actdis->Comm()));

      actdis->ComputeNullSpaceIfNecessary(solver->Params());

      // feed the solver object with additional information
      if (onlycontact or meshtyingandcontact)
        solver->Params().set<bool>("CONTACT", true);
      else if (onlymeshtying)
        solver->Params().set<bool>("MESHTYING", true);
      else
        dserror(
            "Saddle-point formulations are only supported for solid CONTACT or MESHTYING problems. "
            "Problems like beamcontact or pure structure problem w/o contact do not support a "
            "saddle-point formulation.");

      INPAR::CONTACT::SolvingStrategy soltype =
          INPUT::IntegralValue<INPAR::CONTACT::SolvingStrategy>(mcparams, "STRATEGY");
      if (soltype == INPAR::CONTACT::solution_lagmult)
      {
        // get the solver number used for structural problems
        const int linsolvernumber = sdyn.get<int>("LINEAR_SOLVER");
        // check if the structural solver has a valid solver number
        if (linsolvernumber == (-1))
          dserror(
              "No linear solver defined for structural field. Please set LINEAR_SOLVER in "
              "STRUCTURAL DYNAMIC to a valid number!");

        // provide null space information
        if (prec == INPAR::SOLVER::PreconditionerType::cheap_simple)
        {
          actdis->ComputeNullSpaceIfNecessary(
              solver->Params()
                  .sublist("CheapSIMPLE Parameters")
                  .sublist("Inverse1"));  // Inverse2 is created within blockpreconditioners.cpp
        }
        else if (prec == INPAR::SOLVER::PreconditionerType::multigrid_muelu_contactsp)
        { /* do nothing here */
        }
      }
    }
    break;
    default:
    {
      // build meshtying solver
      solver = Teuchos::rcp(new CORE::LINALG::Solver(
          GLOBAL::Problem::Instance()->SolverParams(linsolvernumber), actdis->Comm()));
      actdis->ComputeNullSpaceIfNecessary(solver->Params());
    }
    break;
  }

  return solver;
}

BACI_NAMESPACE_CLOSE
