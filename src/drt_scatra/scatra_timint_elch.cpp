/*!-----------------------------------------------------------------------------------------------*
\file scatra_timint_elch.cpp

  \brief scatra time integration for elch

<pre>
Maintainer: Andreas Ehrl
            ehrl@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15252
</pre>
 *------------------------------------------------------------------------------------------------*/
#include "../drt_fluid/fluid_utils.H" // for splitter

#include "../drt_io/io_control.H"

#include "../drt_lib/drt_globalproblem.H"

#include "../drt_mat/ion.H"
#include "../drt_mat/matlist.H"

#include "../drt_nurbs_discret/drt_nurbs_discret.H"

#include "../drt_scatra/scatra_timint_meshtying_strategy_fluid_elch.H"
#include "../drt_scatra/scatra_timint_meshtying_strategy_s2i_elch.H"
#include "../drt_scatra/scatra_timint_meshtying_strategy_std_elch.H"

#include "../drt_scatra_ele/scatra_ele_action.H"

#include "../linalg/linalg_krylov_projector.H"
#include "../linalg/linalg_solver.H"
#include "../linalg/linalg_utils.H"

#include "scatra_timint_elch.H"

/*----------------------------------------------------------------------*
 | constructor                                              ehrl  01/14 |
 *----------------------------------------------------------------------*/
SCATRA::ScaTraTimIntElch::ScaTraTimIntElch(
        Teuchos::RCP<DRT::Discretization>        dis,
        Teuchos::RCP<LINALG::Solver>             solver,
        Teuchos::RCP<Teuchos::ParameterList>     params,
        Teuchos::RCP<Teuchos::ParameterList>     sctratimintparams,
        Teuchos::RCP<Teuchos::ParameterList>     extraparams,
        Teuchos::RCP<IO::DiscretizationWriter>   output)
  : ScaTraTimIntImpl(dis,solver,sctratimintparams,extraparams,output),
    elchparams_     (params),
    equpot_         (DRT::INPUT::IntegralValue<INPAR::ELCH::EquPot>(*elchparams_,"EQUPOT")),
    frt_            (0.),
    gstatnumite_    (0),
    gstatincrement_ (0.),
    sigma_          (Teuchos::null),
    dlcapexists_    (false),
    ektoggle_       (Teuchos::null),
    dctoggle_       (Teuchos::null),
    electrodesoc_   (Teuchos::null),
    electrodeconc_  (Teuchos::null),
    electrodeeta_   (Teuchos::null),
    electrodecurr_  (Teuchos::null),
    cellvoltage_    (0.)
{
  return;
}


/*----------------------------------------------------------------------*
 | initialize algorithm                                 rasthofer 12/13 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::Init()
{
  // The diffusion-conduction formulation does not support all options of the Nernst-Planck formulation
  // Let's check for valid options
  if(DRT::INPUT::IntegralValue<int>(*elchparams_,"DIFFCOND_FORMULATION"))
    ValidParameterDiffCond();

  // set up the concentration-el.potential splitter
  splitter_ = Teuchos::rcp(new LINALG::MapExtractor);
  FLD::UTILS::SetupFluidSplit(*discret_,numscal_,*splitter_);

  // initialize time-dependent electrode kinetics variables (galvanostatic mode or double layer contribution)
  ComputeTimeDerivPot0(true);

  // initialize dirichlet toggle:
  // for certain ELCH problem formulations we have to provide
  // additional flux terms / currents across Dirichlet boundaries for the standard element call
  Teuchos::RCP<Epetra_Vector> dirichones = LINALG::CreateVector(*(dbcmaps_->CondMap()),false);
  dirichones->PutScalar(1.0);
  dctoggle_ = LINALG::CreateVector(*(discret_->DofRowMap()),true);
  dbcmaps_->InsertCondVector(dirichones, dctoggle_);

  // screen output (has to come after SetInitialField)
  // a safety check for the solver type
  if ((numscal_ > 1) && (solvtype_!=INPAR::SCATRA::solvertype_nonlinear))
    dserror("Solver type has to be set to >>nonlinear<< for ion transport.");

  // check validity of material and element formulation
  Teuchos::ParameterList eleparams;
  eleparams.set<int>("action",SCATRA::check_scatra_element_parameter);
  if(isale_)
    discret_->AddMultiVectorToParameterList(eleparams,"dispnp",dispnp_);
  discret_->Evaluate(eleparams,Teuchos::null,Teuchos::null,Teuchos::null,Teuchos::null,Teuchos::null);

  frt_ = INPAR::ELCH::faraday_const/(INPAR::ELCH::gas_const * elchparams_->get<double>("TEMPERATURE"));

  if (myrank_==0)
  {
    std::cout<<"\nSetup of splitter: numscal = "<<numscal_<<std::endl;
    std::cout<<"Temperature value T (Kelvin)     = "<<elchparams_->get<double>("TEMPERATURE")<<std::endl;
    std::cout<<"Constant F/RT                    = "<<frt_<<std::endl;
  }

  sigma_= Teuchos::rcp(new Epetra_SerialDenseVector(numdofpernode_));
  // conductivity must be stored for the galvanostatic condition in a global variable
  ComputeConductivity(); // every processor has to do this call
  if (myrank_==0)
  {
    for (int k=0;k < numscal_;k++)
    {
      std::cout<<"Electrolyte conductivity (species "<<k+1<<")    = "<<(*sigma_)[k]<<std::endl;
    }
    if (equpot_==INPAR::ELCH::equpot_enc_pde_elim)
    {
      double diff = (*sigma_)[0];
      for (int k=1;k < numscal_;k++)
      {
        diff += (*sigma_)[k];
      }
      std::cout<<"Electrolyte conductivity (species elim) = "<<(*sigma_)[numscal_]-diff<<std::endl;
    }
    std::cout<<"Electrolyte conductivity (all species)  = "<<(*sigma_)[numscal_]<<std::endl<<std::endl;
  }

  // initialize vector for states of charge of resolved electrodes
  std::vector<DRT::Condition*> electrodesocconditions;
  discret_->GetCondition("ElectrodeSOC",electrodesocconditions);
  if(electrodesocconditions.size() > 0)
  {
    electrodesoc_ = Teuchos::rcp(new std::vector<double>);
    electrodesoc_->resize(electrodesocconditions.size(),-1.);
  }

  // initialize vectors for mean reactant concentrations, mean electric overpotentials, and total electric currents at electrode boundaries
  std::vector<DRT::Condition*> electrodeboundaryconditions;
  discret_->GetCondition("ElchBoundaryKinetics",electrodeboundaryconditions);
  if(electrodeboundaryconditions.size() > 0)
  {
    electrodeconc_ = Teuchos::rcp(new std::vector<double>);
    electrodeconc_->resize(electrodeboundaryconditions.size(),-1.);
    electrodeeta_ = Teuchos::rcp(new std::vector<double>);
    electrodeeta_->resize(electrodeboundaryconditions.size(),-1.);
    electrodecurr_ = Teuchos::rcp(new std::vector<double>);
    electrodecurr_->resize(electrodeboundaryconditions.size(),-1.);
  }

  return;
}


/*----------------------------------------------------------------------*
 | set elch-specific element parameters                      fang 11/14 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::SetElementSpecificScaTraParameters(Teuchos::ParameterList& eleparams)
{
  // overwrite action type
  if(DRT::INPUT::IntegralValue<int>(*elchparams_,"DIFFCOND_FORMULATION"))
  {
    eleparams.set<int>("action",SCATRA::set_diffcond_scatra_parameter);

    // parameters for diffusion-conduction formulation
    eleparams.sublist("DIFFCOND") = elchparams_->sublist("DIFFCOND");
  }
  else
    eleparams.set<int>("action",SCATRA::set_elch_scatra_parameter);

  // general elch parameters
  eleparams.set<double>("frt",INPAR::ELCH::faraday_const/(INPAR::ELCH::gas_const*(elchparams_->get<double>("TEMPERATURE"))));
  eleparams.set<int>("equpot",equpot_);

  return;
}


/*----------------------------------------------------------------------*
 | add parameters depending on the problem              rasthofer 12/13 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::AddProblemSpecificParametersAndVectors(
  Teuchos::ParameterList& params //!< parameter list
)
{
  discret_->SetState("dctoggle",dctoggle_);

  return;
}


/*----------------------------------------------------------------------*
 | contains the elch-specific nonlinear iteration loop       ehrl 01/14 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::NonlinearSolve()
{
  bool stopgalvanostat(false);
  gstatnumite_=1;

  // galvanostatic control (ELCH)
  while (!stopgalvanostat)
  {
    ScaTraTimIntImpl::NonlinearSolve();

    stopgalvanostat = ApplyGalvanostaticControl();
  }  // end galvanostatic control

  return;
}


/*----------------------------------------------------------------------*
 | Calculate problem specific norm                            ehrl 01/14|
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::CalcProblemSpecificNorm(
    double& conresnorm,
    double& incconnorm_L2,
    double& connorm_L2,
    double& incpotnorm_L2,
    double& potnorm_L2,
    double& potresnorm,
    double& conresnorminf)
{
  Teuchos::RCP<Epetra_Vector> onlycon = splitter_->ExtractOtherVector(residual_);
  onlycon->Norm2(&conresnorm);
  onlycon->NormInf(&conresnorminf);

  splitter_->ExtractOtherVector(increment_,onlycon);
  onlycon->Norm2(&incconnorm_L2);

  splitter_->ExtractOtherVector(phinp_,onlycon);
  onlycon->Norm2(&connorm_L2);

  Teuchos::RCP<Epetra_Vector> onlypot = splitter_->ExtractCondVector(residual_);
  onlypot->Norm2(&potresnorm);

  splitter_->ExtractCondVector(increment_,onlypot);
  onlypot->Norm2(&incpotnorm_L2);

  splitter_->ExtractCondVector(phinp_,onlypot);
  onlypot->Norm2(&potnorm_L2);

  return;
}


/*----------------------------------------------------------------------*
 |  calculate error compared to analytical solution            gjb 10/08|
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::EvaluateErrorComparedToAnalyticalSol()
{
  const INPAR::SCATRA::CalcError calcerr
    = DRT::INPUT::IntegralValue<INPAR::SCATRA::CalcError>(*params_,"CALCERROR");

  switch (calcerr)
  {
  case INPAR::SCATRA::calcerror_no: // do nothing (the usual case)
    break;
  case INPAR::SCATRA::calcerror_Kwok_Wu:
  {
    //   References:

    //   Kwok, Yue-Kuen and Wu, Charles C. K.
    //   "Fractional step algorithm for solving a multi-dimensional
    //   diffusion-migration equation"
    //   Numerical Methods for Partial Differential Equations
    //   1995, Vol 11, 389-397

    //   G. Bauer, V. Gravemeier, W.A. Wall, A 3D finite element approach for the coupled
    //   numerical simulation of electrochemical systems and fluid flow,
    //   International Journal for Numerical Methods in Engineering, 86
    //   (2011) 1339–1359. DOI: 10.1002/nme.3107

    // create the parameters for the error calculation
    Teuchos::ParameterList eleparams;
    eleparams.set<int>("action",SCATRA::calc_error);
    eleparams.set("total time",time_);
    eleparams.set<int>("calcerrorflag",calcerr);
    //provide displacement field in case of ALE
    if (isale_)
      discret_->AddMultiVectorToParameterList(eleparams,"dispnp",dispnp_);

    // set vector values needed by elements
    discret_->ClearState();
    discret_->SetState("phinp",phinp_);

    // get (squared) error values
    Teuchos::RCP<Epetra_SerialDenseVector> errors
      = Teuchos::rcp(new Epetra_SerialDenseVector(3));
    discret_->EvaluateScalars(eleparams, errors);
    discret_->ClearState();

    double conerr1 = 0.0;
    double conerr2 = 0.0;
    // for the L2 norm, we need the square root
    if(numscal_==2)
    {
      conerr1 = sqrt((*errors)[0]);
      conerr2 = sqrt((*errors)[1]);
    }
    else if(numscal_==1)
    {
      conerr1 = sqrt((*errors)[0]);
      conerr2 = 0.0;
    }
    else
      dserror("The analytical solution of Kwok and Wu is only defined for two species");

    double poterr  = sqrt((*errors)[2]);

    if (myrank_ == 0)
    {
      printf("\nL2_err for Kwok and Wu (time = %f):\n", time_);
      printf(" concentration1 %15.8e\n concentration2 %15.8e\n potential      %15.8e\n\n",
             conerr1,conerr2,poterr);
    }
#if 0
    if (myrank_ == 0)
    {
      // append error of the last time step to the error file
      if ((step_==stepmax_) or (time_==maxtime_))// write results to file
      {
        ostd::stringstream temp;
        const std::string simulation = DRT::Problem::Instance()->OutputControlFile()->FileName();
        //const std::string fname = simulation+".relerror";
        const std::string fname = "XXX_kwok_xele.relerror";

        double elelength=0.0;
        if(simulation.find("5x5")!=std::string::npos)
          elelength=0.2;
        else if(simulation.find("10x10")!=std::string::npos)
          elelength=0.1;
        else if(simulation.find("20x20")!=std::string::npos)
          elelength=0.05;
        else if(simulation.find("40x40")!=std::string::npos)
          elelength=0.025;
        else if(simulation.find("50x50")!=std::string::npos)
          elelength=0.02;
        else if(simulation.find("80x80")!=std::string::npos)
          elelength=0.0125;
        else std::cout << "Warning: file name did not allow a evaluation of the element size!!!" << std::endl;

        std::ofstream f;
        f.precision(12);
        f.open(fname.c_str(),std::fstream::ate | std::fstream::app);
        f << "#| " << simulation << "\n";
        //f << "#| Step | Time | rel. L2-error velocity mag |  rel. L2-error pressure  |\n";
        f << "#| Step | Time | c1 abs. error L2 | c2 abs. error L2 | phi abs. error L2 |  element length  |\n";
        //f << step_ << " " << time_ << " " << velerr/velint << " " << preerr/pint << " "<<"\n";
        f << step_ << " " << time_ << " " << conerr1 << " " << conerr2 << " " <<  poterr << " "<< elelength << "" << "\n";
        f.flush();
        f.close();
      }
    }
#endif
  }
  break;
  case INPAR::SCATRA::calcerror_cylinder:
  {
    //   Reference:
    //   G. Bauer, V. Gravemeier, W.A. Wall, A 3D finite element approach for the coupled
    //   numerical simulation of electrochemical systems and fluid flow,
    //   International Journal for Numerical Methods in Engineering, 2011

    // create the parameters for the error calculation
    Teuchos::ParameterList eleparams;
    eleparams.set<int>("action",SCATRA::calc_error);
    eleparams.set("total time",time_);
    eleparams.set<int>("calcerrorflag",calcerr);
    //provide displacement field in case of ALE
    if (isale_)
      discret_->AddMultiVectorToParameterList(eleparams,"dispnp",dispnp_);

    // set vector values needed by elements
    discret_->ClearState();
    discret_->SetState("phinp",phinp_);

    // get (squared) error values
    Teuchos::RCP<Epetra_SerialDenseVector> errors
      = Teuchos::rcp(new Epetra_SerialDenseVector(3));
    discret_->EvaluateScalars(eleparams, errors);
    discret_->ClearState();

    // for the L2 norm, we need the square root
    double conerr1 = sqrt((*errors)[0]);
    double conerr2 = sqrt((*errors)[1]);
    double poterr  = sqrt((*errors)[2]);

    if (myrank_ == 0)
    {
      printf("\nL2_err for concentric cylinders (time = %f):\n", time_);
      printf(" concentration1 %15.8e\n concentration2 %15.8e\n potential      %15.8e\n\n",
             conerr1,conerr2,poterr);
    }
  }
  break;
  case INPAR::SCATRA::calcerror_electroneutrality:
  {
    // compute L2 norm of electroneutrality condition

    // create the parameters for the error calculation
    Teuchos::ParameterList eleparams;
    eleparams.set<int>("action",SCATRA::calc_error);
    eleparams.set("total time",time_);
    eleparams.set<int>("calcerrorflag",calcerr);
    //provide displacement field in case of ALE
    if (isale_)
      discret_->AddMultiVectorToParameterList(eleparams,"dispnp",dispnp_);

    // set vector values needed by elements
    discret_->ClearState();
    discret_->SetState("phinp",phinp_);

    // get (squared) error values
    Teuchos::RCP<Epetra_SerialDenseVector> errors
      = Teuchos::rcp(new Epetra_SerialDenseVector(1));
    discret_->EvaluateScalars(eleparams, errors);
    discret_->ClearState();

    // for the L2 norm, we need the square root
    double err = sqrt((*errors)[0]);

    if (myrank_ == 0)
    {
      printf("\nL2_err for electroneutrality (time = %f):\n", time_);
      printf(" Deviation from ENC: %15.8e\n\n",err);
    }
  }
  break;
  default:
    dserror("Cannot calculate error. Unknown type of analytical test problem"); break;
  }
  return;
} // SCATRA::ScaTraTimIntImpl::EvaluateErrorComparedToAnalyticalSol


/*----------------------------------------------------------------------*
 | current solution becomes most recent solution of next timestep       |
 |                                                            gjb 08/08 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::Update(const int num)
{
  // perform update of time-dependent electrode variables
  ElectrodeKineticsTimeUpdate();

  return;
}


/*----------------------------------------------------------------------*
 | problem-specific outputs                                  fang 01/15 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::OutputProblemSpecific()
{
  // print electrode boundary status information to screen and files
  OutputElectrodeInfoBoundary();

  // print electrode interior status information to screen and files
  OutputElectrodeInfoInterior();

  // print cell voltage to screen
  OutputCellVoltage();

  return;
} // SCATRA::ScaTraTimIntElch::OutputProblemSpecific()


/*-------------------------------------------------------------------------*
 | output electrode boundary status information to screen       gjb  01/09 |
 *-------------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::OutputElectrodeInfoBoundary()
{
  // evaluate the following type of boundary conditions:
  std::string condname("ElchBoundaryKinetics");
  std::vector<DRT::Condition*> cond;
  discret_->GetCondition(condname,cond);

  // leave method if there's nothing to do!
  if(!cond.size())
    return;

  double sum(0.0);

  if(myrank_ == 0)
  {
    std::cout<<"Status of '"<<condname<<"':\n"
    <<"++----+---------------------+------------------+----------------------+--------------------+----------------+----------------+"<<std::endl;
    printf("|| ID |    Total current    | Area of boundary | Mean current density | Mean overpotential | Electrode pot. | Mean Concentr. |\n");
  }

  // evaluate the conditions and separate via ConditionID
  for (int condid = 0; condid < (int) cond.size(); condid++)
  {
    double currtangent(0.0); // this value remains unused here!
    double currresidual(0.0); // this value remains unused here!
    double electrodesurface(0.0); // this value remains unused here!
    double electrodepot(0.0); // this value remains unused here!
    double meanoverpot(0.0); // this value remains unused here!

    OutputSingleElectrodeInfoBoundary(
        cond[condid],
        condid,
        true,
        sum,
        currtangent,
        currresidual,
        electrodesurface,
        electrodepot,
        meanoverpot);
  } // loop over condid

  if(myrank_ == 0)
  {
    std::cout << "++----+---------------------+------------------+----------------------+--------------------+----------------+----------------+" << std::endl << std::endl;
    // print out the net total current for all indicated boundaries
    printf("Net total current over boundary: %10.3E\n\n",sum);
  }

  // clean up
  discret_->ClearState();

  return;
} // SCATRA::ScaTraTimIntElch::OutputElectrodeInfoBoundary()


/*----------------------------------------------------------------------*
 |  get electrode status for single boundary condition       gjb  11/09 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::OutputSingleElectrodeInfoBoundary(
    DRT::Condition* condition,
    const int condid,
    const bool print,
    double& currentsum,
    double& currtangent,
    double& currresidual,
    double& electrodesurface,
    double& electrodepot,
    double& meanoverpot)
{
  // set vector values needed by elements
  discret_->ClearState();
  discret_->SetState("phinp",phinp_);
  // needed for double-layer capacity!
  discret_->SetState("phidtnp",phidtnp_);

  // set action for elements
  Teuchos::ParameterList eleparams;
  eleparams.set<int>("action",SCATRA::bd_calc_elch_boundary_kinetics);
  eleparams.set("calc_status",true); // just want to have a status output!

  // parameters for Elch/DiffCond formulation
  eleparams.sublist("DIFFCOND") = elchparams_->sublist("DIFFCOND");

  //provide displacement field in case of ALE
  if (isale_)
    discret_->AddMultiVectorToParameterList(eleparams,"dispnp",dispnp_);

  // Since we just want to have the status output for t_{n+1},
  // we have to take care for Gen.Alpha!
  // AddTimeIntegrationSpecificVectors cannot be used since we do not want
  // an evaluation for t_{n+\alpha_f} !!!

  // Warning:
  // Specific time integration parameter are set in the following function.
  // In the case of a genalpha-time integration scheme the solution vector phiaf_ at time n+af
  // is passed to the element evaluation routine. Therefore, the electrode status is evaluate at a
  // different time (n+af) than our output routine (n+1), resulting in slightly different values at the electrode.
  // A different approach is not possible (without major hacks) since the time-integration scheme is
  // necessary to perform galvanostatic simulations, for instance.
  // Think about: double layer effects for genalpha time-integration scheme

  // add element parameters according to time-integration scheme
  AddTimeIntegrationSpecificVectors();

  // initialize result vector
  // physical meaning of vector components is described below
  Teuchos::RCP<Epetra_SerialDenseVector> scalars = Teuchos::rcp(new Epetra_SerialDenseVector(10));

  // evaluate relevant boundary integrals
  discret_->EvaluateScalars(eleparams,scalars,"ElchBoundaryKinetics",condid);

  // get total integral of current
  double currentintegral = (*scalars)(0);
  // get total integral of double layer current
  double currentdlintegral = (*scalars)(1);
  // get total boundary area
  double boundaryint = (*scalars)(2);
  // get total integral of electric potential
  double electpotentialint = (*scalars)(3);
  // get total integral of electric overpotential
  double overpotentialint = (*scalars)(4);
  // get total integral of electric potential difference
  double epdint = (*scalars)(5);
  // get total integral of open circuit electric potential
  double ocpint = (*scalars)(6);
  // get total integral of reactant concentration
  double cint = (*scalars)(7);
  // get derivative of integrated current with respect to electrode potential
  double currderiv = (*scalars)(8);
  // get negative current residual (right-hand side of galvanostatic balance equation)
  double currentresidual = (*scalars)(9);

  // specify some return values
  currentsum += currentintegral; // sum of currents
  currtangent = currderiv;      // tangent w.r.t. electrode potential on metal side
  currresidual = currentresidual;
  electrodesurface = boundaryint;
  electrodepot = electpotentialint/boundaryint;
  meanoverpot = overpotentialint/boundaryint;

  // clean up
  discret_->ClearState();

  // print out results to screen/file if desired
  if(myrank_ == 0 and print)
  {
    // print out results to screen
    printf("|| %2d |     %10.3E      |    %10.3E    |      %10.3E      |     %10.3E     |   %10.3E   |   %10.3E   |\n",
        condid,currentintegral+currentdlintegral,boundaryint,currentintegral/boundaryint+currentdlintegral/boundaryint,overpotentialint/boundaryint, electrodepot, cint/boundaryint);

    // write results to file
    std::ostringstream temp;
    temp << condid;
    const std::string fname
    = DRT::Problem::Instance()->OutputControlFile()->FileName()+".electrode_status_"+temp.str()+".txt";

    std::ofstream f;
    if (Step() == 0)
    {
      f.open(fname.c_str(),std::fstream::trunc);
      f << "#ID,Step,Time,Total_current,Area_of_boundary,Mean_current_density_electrode_kinetics,Mean_current_density_dl,Mean_overpotential,Mean_electrode_pot_diff,Mean_opencircuit_pot,Electrode_pot,Mean_concentration\n";
    }
    else
      f.open(fname.c_str(),std::fstream::ate | std::fstream::app);

    f << condid << "," << Step() << "," << Time() << "," << currentintegral+currentdlintegral  << "," << boundaryint
    << "," << currentintegral/boundaryint << "," << currentdlintegral/boundaryint
    << "," << overpotentialint/boundaryint << "," <<
    epdint/boundaryint << "," << ocpint/boundaryint << "," << electrodepot << ","
    << cint/boundaryint << "\n";
    f.flush();
    f.close();
  } // if (myrank_ == 0)

  // galvanostatic simulations:
  // add the double layer current to the Butler-Volmer current
  currentsum += currentdlintegral;

  // update vectors
  (*electrodeconc_)[condid] = cint/boundaryint;
  (*electrodeeta_)[condid] = overpotentialint/boundaryint;
  (*electrodecurr_)[condid] = currentsum;

  return;
} // SCATRA::ScaTraTimIntElch::OutputSingleElectrodeInfoBoundary


/*-------------------------------------------------------------------------------*
 | output electrode interior status information to screen and files   fang 01/15 |
 *-------------------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::OutputElectrodeInfoInterior()
{
  // extract conditions for electrode state of charge
  std::vector<DRT::Condition*> conditions;
  discret_->GetCondition("ElectrodeSOC",conditions);

  // perform all following operations only if there is at least one condition for electrode state of charge
  if(conditions.size() > 0)
  {
    // print header to screen
    if(myrank_ == 0)
    {
      std::cout << "Electrode state of charge and related:" << std::endl;
      std::cout << "+----+-----------------+----------------+----------------+" << std::endl;
      std::cout << "| ID | state of charge |     C rate     | operation mode |" << std::endl;
    }

    // loop over conditions for electrode state of charge
    for(unsigned condid=0; condid<conditions.size(); ++condid)
    {
      // add state vector to discretization
      discret_->ClearState();
      discret_->SetState("phinp",phinp_);

      // create parameter list
      Teuchos::ParameterList condparams;

      // action for elements
      condparams.set<int>("action",SCATRA::calc_elch_electrode_soc);

      // initialize result vector
      // first component = concentration integral, second component = domain integral
      Teuchos::RCP<Epetra_SerialDenseVector> scalars = Teuchos::rcp(new Epetra_SerialDenseVector(2));

      // evaluate current condition for electrode state of charge
      discret_->EvaluateScalars(condparams,scalars,"ElectrodeSOC",condid);
      discret_->ClearState();

      // extract concentration and domain integrals
      double intconcentration = (*scalars)(0);
      double intdomain = (*scalars)(1);

      // extract reference concentrations at 0% and 100% state of charge
      const double c_0 = conditions[condid]->GetDouble("c_0%");
      const double c_100 = conditions[condid]->GetDouble("c_100%");

      // compute state of charge for current electrode
      const double soc = (intconcentration/intdomain-c_0)/(c_100-c_0);

      // compute C rate for current electrode
      double c_rate(0.);
      if((*electrodesoc_)[condid] != -1.)
        c_rate = (soc-(*electrodesoc_)[condid])/dta_*3600.;

      // determine operation mode
      std::string mode;
      if(c_rate < 0.)
        mode.assign("discharge");
      else if(c_rate == 0.)
        mode.assign(" at rest ");
      else
        mode.assign(" charge  ");

      // update state of charge for current electrode
      (*electrodesoc_)[condid] = soc;

      // print results to screen and files
      if(myrank_ == 0)
      {
        // print results to screen
        std::cout << "| " << std::setw(2) << condid << " |   " << std::setw(7) << std::setprecision(2) << std::fixed << soc*100. << " %     |     " << std::setw(5) << std::abs(c_rate) << "      |   " << mode.c_str() << "    |" << std::endl;

        // set file name
        std::ostringstream id;
        id << condid;
        const std::string filename(DRT::Problem::Instance()->OutputControlFile()->FileName()+".electrode_soc_"+id.str()+".txt");

        // open file in appropriate mode and write header at beginning
        std::ofstream file;
        if(Step() == 0)
        {
          file.open(filename.c_str(),std::fstream::trunc);
          file << "Step,Time,SOC,CRate" << std::endl;
        }
        else
          file.open(filename.c_str(),std::fstream::app);

        // write results for current electrode to file
        file << Step() << "," << Time() << "," << std::setprecision(16) << std::fixed << soc << "," << c_rate << std::endl;

        // close file
        file.close();
      } // if(myrank_ == 0)
    } // loop over conditions

    // print finish line to screen
    if(myrank_ == 0)
      std::cout << "+----+-----------------+----------------+----------------+" << std::endl << std::endl;
  }

  return;
} // SCATRA::ScaTraTimIntElch::OutputElectrodeInfoInterior


/*----------------------------------------------------------------------*
 | output cell voltage to screen                             fang 01/15 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::OutputCellVoltage()
{
  // extract conditions for cell voltage
  std::vector<DRT::Condition*> conditions;
  discret_->GetCondition("CellVoltage",conditions);

  // perform all following operations only if there is at least one condition for cell voltage
  if(conditions.size() > 0)
  {
    // safety check
    if(conditions.size() != 2)
      dserror("Must have exactly two boundary conditions for cell voltage, one per electrode!");

    // print header
    if(myrank_ == 0)
    {
      std::cout << "Electrode potentials and cell voltage:" << std::endl;
      std::cout << "+----+-------------------------+" << std::endl;
      std::cout << "| ID | mean electric potential |" << std::endl;
    }

    // initialize vector for mean electric potentials of electrodes
    std::vector<double> potentials(2,0.);

    // loop over both conditions for cell voltage
    for(unsigned condid=0; condid<conditions.size(); ++condid)
    {
      // add state vector to discretization
      discret_->ClearState();
      discret_->SetState("phinp",phinp_);

      // create parameter list
      Teuchos::ParameterList condparams;

      // action for elements
      condparams.set<int>("action",SCATRA::bd_calc_elch_cell_voltage);

      // initialize result vector
      // first component = electric potential integral, second component = domain integral
      Teuchos::RCP<Epetra_SerialDenseVector> scalars = Teuchos::rcp(new Epetra_SerialDenseVector(2));

      // evaluate current condition for electrode state of charge
      discret_->EvaluateScalars(condparams,scalars,"CellVoltage",condid);
      discret_->ClearState();

      // extract concentration and domain integrals
      double intpotential = (*scalars)(0);
      double intdomain = (*scalars)(1);

      // compute mean electric potential of current electrode
      potentials[condid] = intpotential/intdomain;

      // print mean electric potential of current electrode to screen
      if(myrank_ == 0)
        std::cout << "| " << std::setw(2) << condid << " |         " << std::setw(6) << std::setprecision(3) << std::fixed << potentials[condid] << "          |" << std::endl;
    } // loop over conditions

    // compute cell voltage
    cellvoltage_ = std::abs(potentials[0]-potentials[1]);

    // print cell voltage to screen and file
    if(myrank_ == 0)
    {
      // print cell voltage to screen
      std::cout << "+----+-------------------------+" << std::endl;
      std::cout << "| cell voltage: " << std::setw(6) << cellvoltage_ << "         |" << std::endl;
      std::cout << "+----+-------------------------+" << std::endl << std::endl;

      // set file name
      const std::string filename(DRT::Problem::Instance()->OutputControlFile()->FileName()+".cell_voltage.txt");

      // open file in appropriate mode and write header at beginning
      std::ofstream file;
      if(Step() == 0)
      {
        file.open(filename.c_str(),std::fstream::trunc);
        file << "Step,Time,CellVoltage" << std::endl;
      }
      else
        file.open(filename.c_str(),std::fstream::app);

      // write results for current electrode to file
      file << Step() << "," << Time() << "," << std::setprecision(16) << std::fixed << cellvoltage_ << std::endl;

      // close file
      file.close();
    } // if(myrank_ == 0)
  }

  return;
} // SCATRA::ScaTraTimIntElch::OutputCellVoltage


/*----------------------------------------------------------------------*
 | perform setup of natural convection                       fang 08/14 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::SetupNatConv()
{
  // calculate the initial mean concentration value
  if (numscal_ < 1) dserror("Error since numscal = %d. Not allowed since < 1",numscal_);
  c0_.resize(numscal_);

  discret_->ClearState();
  discret_->SetState("phinp",phinp_);

  // set action for elements
  Teuchos::ParameterList eleparams;
  eleparams.set<int>("action",SCATRA::calc_mean_scalars);
  eleparams.set("inverting",false);

  // provide displacement field in case of ALE
  if (isale_)
    discret_->AddMultiVectorToParameterList(eleparams,"dispnp",dispnp_);

  // evaluate integrals of concentrations and domain
  Teuchos::RCP<Epetra_SerialDenseVector> scalars
  = Teuchos::rcp(new Epetra_SerialDenseVector(numscal_+1));
  discret_->EvaluateScalars(eleparams, scalars);
  discret_->ClearState();   // clean up

  // calculate mean concentration
  const double domint = (*scalars)[numscal_];

  if (std::abs(domint) < EPS15)
    dserror("Division by zero!");

  for(int k=0;k<numscal_;k++)
    c0_[k] = (*scalars)[k]/domint;

  // initialization of the densification coefficient vector
  densific_.resize(numscal_);
  DRT::Element*   element = discret_->lRowElement(0);
  Teuchos::RCP<MAT::Material>  mat = element->Material();

  if (mat->MaterialType() == INPAR::MAT::m_matlist)
  {
    Teuchos::RCP<const MAT::MatList> actmat = Teuchos::rcp_static_cast<const MAT::MatList>(mat);

    for (int k = 0;k<numscal_;++k)
    {
      const int matid = actmat->MatID(k);
      Teuchos::RCP<const MAT::Material> singlemat = actmat->MaterialById(matid);

      if (singlemat->MaterialType() == INPAR::MAT::m_ion)
      {
        Teuchos::RCP<const MAT::Ion> actsinglemat = Teuchos::rcp_static_cast<const MAT::Ion>(singlemat);

        densific_[k] = actsinglemat->Densification();

        if (densific_[k] < 0.0) dserror("received negative densification value");
      }
      else
        dserror("Material type is not allowed!");
    }
  }

  // for a single species calculation
  else if (mat->MaterialType() == INPAR::MAT::m_ion)
  {
    Teuchos::RCP<const MAT::Ion> actmat = Teuchos::rcp_static_cast<const MAT::Ion>(mat);

    densific_[0] = actmat->Densification();

    if (densific_[0] < 0.0) dserror("received negative densification value");
    if (numscal_ > 1) dserror("Single species calculation but numscal = %d > 1",numscal_);
  }
  else
    dserror("Material type is not allowed!");

  return;
} // ScaTraTimIntElch::SetupNatConv()


/*-------------------------------------------------------------------------*
 | valid parameters for the diffusion-conduction formulation    ehrl 12/12 |
 *-------------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::ValidParameterDiffCond()
{
  if(myrank_==0)
  {
    if(DRT::INPUT::IntegralValue<INPAR::ELCH::ElchMovingBoundary>(*elchparams_,"MOVINGBOUNDARY")!=INPAR::ELCH::elch_mov_bndry_no)
      dserror("Moving boundaries are not supported in the ELCH diffusion-conduction framework!!");

    if(DRT::INPUT::IntegralValue<int>(*params_,"NATURAL_CONVECTION"))
      dserror("Natural convection is not supported in the ELCH diffusion-conduction framework!!");

    if((DRT::INPUT::IntegralValue<INPAR::SCATRA::SolverType>(*params_,"SOLVERTYPE"))!= INPAR::SCATRA::solvertype_nonlinear)
      dserror("The only solvertype supported by the ELCH diffusion-conduction framework is the non-linear solver!!");

    if((DRT::INPUT::IntegralValue<INPAR::SCATRA::ConvForm>(*params_,"CONVFORM"))!= INPAR::SCATRA::convform_convective)
      dserror("Only the convective formulation is supported so far!!");

    if((DRT::INPUT::IntegralValue<int>(*params_,"NEUMANNINFLOW"))== true)
      dserror("Neuman inflow BC's are not supported by the ELCH diffusion-conduction framework!!");

    if((DRT::INPUT::IntegralValue<int>(*params_,"CONV_HEAT_TRANS"))== true)
      dserror("Convective heat transfer BC's are not supported by the ELCH diffusion-conduction framework!!");

    if((DRT::INPUT::IntegralValue<INPAR::SCATRA::FSSUGRDIFF>(*params_,"FSSUGRDIFF"))!= INPAR::SCATRA::fssugrdiff_no)
      dserror("Subgrid diffusivity is not supported by the ELCH diffusion-conduction framework!!");

    if((DRT::INPUT::IntegralValue<int>(*elchparams_,"BLOCKPRECOND"))== true)
          dserror("Block preconditioner is not supported so far!!");

    // Parameters defined in "SCALAR TRANSPORT DYNAMIC"
    Teuchos::ParameterList& scatrastabparams = params_->sublist("STABILIZATION");

    if((DRT::INPUT::IntegralValue<INPAR::SCATRA::StabType>(scatrastabparams,"STABTYPE"))!= INPAR::SCATRA::stabtype_no_stabilization)
      dserror("No stabilization is necessary for solving the ELCH diffusion-conduction framework!!");

    if((DRT::INPUT::IntegralValue<INPAR::SCATRA::TauType>(scatrastabparams,"DEFINITION_TAU"))!= INPAR::SCATRA::tau_zero)
      dserror("No stabilization is necessary for solving the ELCH diffusion-conduction framework!!");

    if((DRT::INPUT::IntegralValue<INPAR::SCATRA::EvalTau>(scatrastabparams,"EVALUATION_TAU"))!= INPAR::SCATRA::evaltau_integration_point)
      dserror("Evaluation of stabilization parameter only at Gauss points!!");

    if((DRT::INPUT::IntegralValue<INPAR::SCATRA::EvalMat>(scatrastabparams,"EVALUATION_MAT"))!= INPAR::SCATRA::evalmat_integration_point)
      dserror("Evaluation of material only at Gauss points!!");

    if((DRT::INPUT::IntegralValue<INPAR::SCATRA::Consistency>(scatrastabparams,"CONSISTENCY"))!= INPAR::SCATRA::consistency_no)
          dserror("Consistence formulation is not in the ELCH diffusion-conduction framework!!");

    if((DRT::INPUT::IntegralValue<int>(scatrastabparams,"SUGRVEL"))== true)
          dserror("Subgrid velocity is not incoperated in the ELCH diffusion-conduction framework!!");

    if((DRT::INPUT::IntegralValue<int>(scatrastabparams,"ASSUGRDIFF"))== true)
          dserror("Subgrid diffusivity is not incoperated in the ELCH diffusion-conduction framework!!");
  }

  return;
}


/*----------------------------------------------------------------------*
 | Initialize Nernst-BC                                      ehrl 09/13 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::InitNernstBC()
{
  // access electrode kinetics condition
  std::vector<DRT::Condition*> Elchcond;
  discret_->GetCondition("ElchBoundaryKinetics",Elchcond);
  int numcond = Elchcond.size();

  for(int icond = 0; icond < numcond; icond++)
  {
    // check if Nernst-BC is defined on electrode kinetics condition
    if (Elchcond[icond]->GetInt("kinetic model")==INPAR::SCATRA::nernst)
    {
      if(DRT::INPUT::IntegralValue<int>(*elchparams_,"DIFFCOND_FORMULATION"))
      {
        if(icond==0)
          ektoggle_ = LINALG::CreateVector(*(discret_->DofRowMap()),true);

        // 1.0 for electrode-kinetics toggle
        const double one = 1.0;

        // global node id's which are part of the Nernst-BC
        const std::vector<int>* nodegids = Elchcond[icond]->Nodes();

        // loop over all global nodes part of the Nernst-BC
        for (int ii = 0; ii< (int (nodegids->size())); ++ii)
        {
          if(discret_->NodeRowMap()->MyGID((*nodegids)[ii]))
          {
            // get node with global node id (*nodegids)[ii]
            DRT::Node* node=discret_->gNode((*nodegids)[ii]);

            // get global dof ids of all dof's with global node id (*nodegids)[ii]
            std::vector<int> nodedofs=discret_->Dof(node);

            // define electrode kinetics toggle
            // later on this toggle is used to blanck the sysmat and rhs
            ektoggle_->ReplaceGlobalValues(1,&one,&nodedofs[numscal_]);
          }
        }
      }
      else
        dserror("Nernst BC is only available for diffusion-conduction formulation!");
    }
  }

  // At element level the Nernst condition has to be handled like a DC
  if(ektoggle_!=Teuchos::null)
    dctoggle_->Update(1.0,*ektoggle_,1.0);

  return;
} //SCATRA::ScaTraTimIntImpl::InitNernstBC


/*----------------------------------------------------------------------------------------*
 | initialize meshtying strategy (including standard case without meshtying)   fang 12/14 |
 *----------------------------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::CreateMeshtyingStrategy()
{
  // fluid meshtying
  if(msht_ != INPAR::FLUID::no_meshtying)
    strategy_ = Teuchos::rcp(new MeshtyingStrategyFluidElch(this));

  // scatra-scatra interface coupling
  else if(s2icoupling_)
    strategy_ = Teuchos::rcp(new MeshtyingStrategyS2IElch(this,DRT::Problem::Instance()->S2IDynamicParams()));

  // standard case without meshtying
  else
    strategy_ = Teuchos::rcp(new MeshtyingStrategyStdElch(this));

  return;
} // ScaTraTimIntImpl::UpdateKrylovSpaceProjection


/*----------------------------------------------------------------------*
 | adapt number of transported scalars                       fang 12/14 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::AdaptNumScal()
{
  if (numscal_ > 1) // we have at least two ion species + el. potential
  {
    // number of concentrations transported is numdof-1
    numscal_ -= 1;

    // current is a solution variable
    if(DRT::INPUT::IntegralValue<int>(elchparams_->sublist("DIFFCOND"),"CURRENT_SOLUTION_VAR"))
    {
      // shape of local row element(0) -> number of space dimensions
      // int dim = DRT::Problem::Instance()->NDim();
      int dim = DRT::UTILS::getDimension(discret_->lRowElement(0)->Shape());
      // number of concentrations transported is numdof-1-nsd
      numscal_ -= dim;
    }
  }

  return;
}


/*----------------------------------------------------------------------*
 | calculate initial electric potential field                fang 03/15 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::CalcInitialPotentialField()
{
  if(DRT::INPUT::IntegralValue<int>(*elchparams_,"INITPOTCALC"))
  {
    // time measurement
    TEUCHOS_FUNC_TIME_MONITOR("SCATRA:       + calc initial potential field");

    // safety checks
    dsassert(step_ == 0, "Step counter is not zero!");
    switch(equpot_)
    {
    case INPAR::ELCH::equpot_divi:
    case INPAR::ELCH::equpot_enc_pde:
    case INPAR::ELCH::equpot_enc_pde_elim:
    {
      // These stationary closing equations for the electric potential are OK, since they explicitly contain the
      // electric potential as variable and therefore can be solved for the initial electric potential.
      break;
    }
    default:
    {
      // If the stationary closing equation for the electric potential does not explicitly contain the electric
      // potential as variable, we obtain a zero block associated with the electric potential on the main diagonal
      // of the global system matrix used below. This zero block makes the entire global system matrix singular!
      // In this case, it would be possible to temporarily change the type of closing equation used, e.g., from
      // INPAR::ELCH::equpot_enc to INPAR::ELCH::equpot_enc_pde. This should work, but has not been implemented yet.
      dserror("Initial potential field cannot be computed for chosen closing equation for electric potential!");
      break;
    }
    }

    // screen output
    if (myrank_ == 0)
    {
      std::cout << "SCATRA: calculating initial field for electric potential" << std::endl;
      PrintTimeStepInfo();
      std::cout << "+------------+-------------------+--------------+--------------+" << std::endl;
      std::cout << "|- step/max -|- tol      [norm] -|-- pot-res ---|-- pot-inc ---|" << std::endl;
    }

    // prepare Newton-Raphson iteration
    iternum_ = 0;
    const int    itermax = params_->sublist("NONLINEAR").get<int>("ITEMAX");
    const double itertol = params_->sublist("NONLINEAR").get<double>("CONVTOL");
    const double restol  = params_->sublist("NONLINEAR").get<double>("ABSTOLRES");

    // start Newton-Raphson iteration
    while(true)
    {
      // update iteration counter
      iternum_++;

      // check for non-positive concentration values
      CheckConcentrationValues(phinp_);

      // assemble global system matrix and residual vector
      AssembleMatAndRHS();

      // project residual, such that only part orthogonal to nullspace is considered
      if (projector_!=Teuchos::null)
        projector_->ApplyPT(*residual_);

      // apply actual Dirichlet boundary conditions to system of equations
      LINALG::ApplyDirichlettoSystem(sysmat_,increment_,residual_,zeros_,*(dbcmaps_->CondMap()));

      // apply artificial Dirichlet boundary conditions to system of equations
      // to hold initial concentrations constant when solving for initial potential field
      LINALG::ApplyDirichlettoSystem(sysmat_,increment_,residual_,zeros_,*(splitter_->OtherMap()));

      // calculate vector norms
      // vector norms associated with concentration are not used, but still computed to avoid code redundancy
      double dummy(0.);
      double incpotnorm_L2(0.);
      double potnorm_L2(0.);
      double potresnorm(0.);
      CalcProblemSpecificNorm(dummy,dummy,dummy,incpotnorm_L2,potnorm_L2,potresnorm,dummy);

      // care for the case that nothing really happens in the potential field
      if (potnorm_L2 < 1e-5)
        potnorm_L2 = 1.0;

      // first iteration step: solution increment is not yet available
      if (iternum_ == 1)
      {
        // print first line of convergence table to screen
        if (myrank_ == 0)
          std::cout << "|  " << std::setw(3) << iternum_ << "/" << std::setw(3) << itermax << "   | "
                    << std::setw(10) << std::setprecision(3) << std::scientific << itertol << "[L_2 ]  | "
                    << std::setw(10) << std::setprecision(3) << std::scientific << potresnorm << "   |      --      | (      --     ,te="
                    << std::setw(10) << std::setprecision(3) << std::scientific << dtele_ << ")" << std::endl;

        // absolute tolerance for deciding if residual is already zero
        // prevents additional solver calls that will not improve the residual anymore
        if(potresnorm < restol)
        {
          // print finish line of convergence table to screen
          if (myrank_ == 0)
            std::cout << "+------------+-------------------+--------------+--------------+" << std::endl << std::endl;

          // abort Newton-Raphson iteration
          break;
        }
      }

      // later iteration steps: solution increment can be printed
      else
      {
        // print current line of convergence table to screen
        if (myrank_ == 0)
          std::cout << "|  " << std::setw(3) << iternum_ << "/" << std::setw(3) << itermax << "   | "
                    << std::setw(10) << std::setprecision(3) << std::scientific << itertol << "[L_2 ]  | "
                    << std::setw(10) << std::setprecision(3) << std::scientific << potresnorm << "   | "
                    << std::setw(10) << std::setprecision(3) << std::scientific << incpotnorm_L2/potnorm_L2 << "   | (ts="
                    << std::setw(10) << std::setprecision(3) << std::scientific << dtsolve_ << ",te="
                    << std::setw(10) << std::setprecision(3) << std::scientific << dtele_ << ")" << std::endl;

        // convergence check
        if((potresnorm <= itertol and incpotnorm_L2/potnorm_L2 <= itertol) or potresnorm < restol)
        {
          // print finish line of convergence table to screen
          if (myrank_ == 0)
            std::cout << "+------------+-------------------+--------------+--------------+" << std::endl << std::endl;

          // abort Newton-Raphson iteration
          break;
        }
      }

      // warn if maximum number of iterations is reached without convergence
      if(iternum_ == itermax)
      {
        if(myrank_ == 0)
        {
          std::cout << "+---------------------------------------------------------------+" << std::endl;
          std::cout << "|            >>>>>> not converged!                              |" << std::endl;
          std::cout << "+---------------------------------------------------------------+" << std::endl << std::endl;
        }

        // abort Newton-Raphson iteration
        break;
      }

      // safety checks
      if(std::isnan(incpotnorm_L2) or std::isnan(potnorm_L2) or std::isnan(potresnorm))
        dserror("calculated vector norm is NaN.");
      if(std::isinf(incpotnorm_L2) or std::isinf(potnorm_L2) or std::isinf(potresnorm))
        dserror("calculated vector norm is INF.");

      // zero out increment vector
      increment_->PutScalar(0.);

      // store time before solving global system of equations
      const double time = Teuchos::Time::wallTime();

      // reprepare Krylov projection if required
      if (updateprojection_)
        UpdateKrylovSpaceProjection();

      // solve final system of equations incrementally
      strategy_->Solve(solver_,sysmat_,increment_,residual_,phinp_,1,projector_);

      // determine time needed for solving global system of equations
      dtsolve_ = Teuchos::Time::wallTime()-time;

      // update electric potential degrees of freedom in initial state vector
      splitter_->AddCondVector(splitter_->ExtractCondVector(increment_),phinp_);

      // copy initial state vector
      phin_->Update(1.,*phinp_,0.);

      // update state vectors for intermediate time steps (only for generalized alpha)
      ComputeIntermediateValues();
    } // Newton-Raphson iteration

    // reset global system matrix and its graph, since we solved a very special problem with a special sparsity pattern
    sysmat_->Reset();
  } // if(DRT::INPUT::IntegralValue<int>(*elchparams_,"INITPOTCALC"))

  return;
} // SCATRA::ScaTraTimIntElch::CalcInitialPotentialField()


/*----------------------------------------------------------------------*
 |  calculate conductivity of electrolyte solution             gjb 07/09|
 *----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_SerialDenseVector> SCATRA::ScaTraTimIntElch::ComputeConductivity()
{
  // we perform the calculation on element level hiding the material access!
  // the initial concentration distribution has to be uniform to do so!!

  // create the parameters for the elements
  Teuchos::ParameterList eleparams;
  eleparams.set<int>("action",SCATRA::calc_elch_conductivity);

  //provide displacement field in case of ALE
  if (isale_)
    discret_->AddMultiVectorToParameterList(eleparams,"dispnp",dispnp_);

  // set vector values needed by elements
  discret_->ClearState();
  discret_->SetState("phinp",phinp_);

  // pointer to current element
  DRT::Element* actele = discret_->lRowElement(0);

  // get element location vector, dirichlet flags and ownerships
  DRT::Element::LocationArray la(1);

  actele->LocationVector(*discret_,la,false);

  // define element matrices and vectors
  // -- which are empty and unused, just to satisfy element Evaluate()
  Epetra_SerialDenseMatrix elematrix1;
  Epetra_SerialDenseMatrix elematrix2;
  Epetra_SerialDenseVector elevector2;
  Epetra_SerialDenseVector elevector3;

  // call the element evaluate method of the first row element
  int err = actele->Evaluate(eleparams,*discret_,la,elematrix1,elematrix2,*sigma_,elevector2,elevector3);
  if (err) dserror("error while computing conductivity");
  discret_->ClearState();

  return sigma_;
} // ScaTraTimIntImpl::ComputeConductivity


/*----------------------------------------------------------------------*
 | apply galvanostatic control                                gjb 11/09 |
 *----------------------------------------------------------------------*/
bool SCATRA::ScaTraTimIntElch::ApplyGalvanostaticControl()
{
  // for galvanostatic ELCH applications we have to adjust the
  // applied cell voltage and continue Newton-Raphson iterations until
  // we reach the desired value for the electric current.

  if (DRT::INPUT::IntegralValue<int>(*elchparams_,"GALVANOSTATIC"))
  {
    // set time derivate parameters of applied voltage for a double layer capacitance current density,
    if(dlcapexists_)
      ComputeTimeDerivPot0(false);

    std::vector<DRT::Condition*> cond;
    discret_->GetCondition("ElchBoundaryKinetics",cond);
    if (!cond.empty())
    {
      const unsigned condid_cathode = elchparams_->get<int>("GSTATCONDID_CATHODE");
      const unsigned condid_anode = elchparams_->get<int>("GSTATCONDID_ANODE");
      int gstatitemax = (elchparams_->get<int>("GSTATITEMAX"));
      double gstatcurrenttol = (elchparams_->get<double>("GSTATCURTOL"));
      const int curvenum = elchparams_->get<int>("GSTATCURVENO");
      const double tol = elchparams_->get<double>("GSTATCONVTOL");
      const double effective_length = elchparams_->get<double>("GSTAT_LENGTH_CURRENTPATH");
      if(effective_length<0.0)
        dserror("A negative effective length is not possible!");
      const INPAR::ELCH::ApproxElectResist approxelctresist = DRT::INPUT::IntegralValue<INPAR::ELCH::ApproxElectResist>(*elchparams_,"GSTAT_APPROX_ELECT_RESIST");

      // There are maximal two electrode conditions by definition
      // current flow i at electrodes
      Teuchos::RCP<std::vector<double> > actualcurrent = Teuchos::rcp(new std::vector<double>(2,0.0));
      // residual at electrodes = i*timefac
      Teuchos::RCP<std::vector<double> > currresidual= Teuchos::rcp(new std::vector<double>(2,0.0));
      Teuchos::RCP<std::vector<double> > currtangent = Teuchos::rcp(new std::vector<double>(2,0.0));
      Teuchos::RCP<std::vector<double> > electrodesurface = Teuchos::rcp(new std::vector<double>(2,0.0));
      Teuchos::RCP<std::vector<double> > electrodepot = Teuchos::rcp(new std::vector<double>(2,0.0));
      Teuchos::RCP<std::vector<double> > meanoverpot = Teuchos::rcp(new std::vector<double>(2,0.0));
      //Teuchos::RCP<std::vector<double> > meanconc = Teuchos::rcp(new std::vector<double>(2,0.0));
      double meanelectrodesurface(0.0);
      //Assumption: Residual at BV1 is the negative of the value at BV2, therefore only the first residual is calculated
      double residual(0.0);

      // for all time integration schemes, compute the current value for phidtnp
      // this is needed for evaluating charging currents due to double-layer capacity
      // This may only be called here and not inside OutputSingleElectrodeInfoBoundary!!!!
      // Otherwise you modify your output to file called during Output()
      ComputeTimeDerivative();

      double targetcurrent = DRT::Problem::Instance()->Curve(curvenum-1).f(time_);
      double timefacrhs = 1.0/ResidualScaling();

      double currtangent_anode(0.0);
      double currtangent_cathode(0.0);
      double potinc_ohm(0.0);
      double potdiffbulk(0.0);
      double resistance = 0.0;

      if(cond.size()>2)
        dserror("The framework may not work for geometric setups containing more than two electrodes! \n"
                "If you need it, check the framework exactly!!");

      // loop over all BV
      // degenerated to a loop over 2 (user-specified) BV conditions
      for (unsigned int icond = 0; icond < cond.size(); icond++)
      {
        // note: only the potential at the boundary with id condid_cathode will be adjusted!
        OutputSingleElectrodeInfoBoundary(
            cond[icond],
            icond,
            false,
            (*actualcurrent)[icond],
            (*currtangent)[icond],
            (*currresidual)[icond],
            (*electrodesurface)[icond],
            (*electrodepot)[icond],
            (*meanoverpot)[icond]//,
            //(*meanconc)[icond]
            );

        if(cond.size()==2)
        {
          // In the case the actual current is zero, we assume that the first electrode is the cathode
          if((*actualcurrent)[icond]<0.0 and condid_cathode != icond)
            dserror("The defined GSTATCONDID_CATHODE does not match the actual current flow situation!!");
          else if((*actualcurrent)[icond]>0.0 and condid_anode != icond)
            dserror("The defined GSTATCONDID_ANODE does not match the actual current flow situation!!");
        }
      } // end loop over electrode kinetics

      if(cond.size()==1)
      {
        if(condid_cathode != 0 or condid_anode!=1)
          dserror("The defined GSTATCONDID_CATHODE and GSTATCONDID_ANODE is wrong for a setup with only one electrode!!\n"
                  "Choose: GSTATCONDID_CATHODE=0 and GSTATCONDID_ANODE=1");
      }

      // get the applied electrode potential of the cathode
      const double potold = cond[condid_cathode]->GetDouble("pot");
      double potnew = potold;

      // bulk voltage loss
      // U =  =  eta_A + delta phi_ohm - eta_C
      // -> delta phi_ohm  = V_A - V_C - eta_A + eta_C = V_A - eta_A - (V_C  - eta_C)
      potdiffbulk = ((*electrodepot)[condid_anode]-(*meanoverpot)[condid_anode])-((*electrodepot)[condid_cathode]-(*meanoverpot)[condid_cathode]);
      // cell voltage loss = V_A - V_C
      //potdiffcell=(*electrodepot)[condid_anode]-(*electrodepot)[condid_cathode];
      // tanget at anode and cathode
      currtangent_anode=(*currtangent)[condid_anode];
      currtangent_cathode=(*currtangent)[condid_cathode];

      if(cond.size()==2)
      {
        // mean electrode surface of the cathode abd anode
        meanelectrodesurface=((*electrodesurface)[0]+(*electrodesurface)[1])/2;
      }
      else
        meanelectrodesurface=(*electrodesurface)[condid_cathode];

      // The linearization of potential increment is always based on the cathode side!!

      // Assumption: Residual at BV1 is the negative of the value at BV2, therefore only the first residual is calculated
      // residual := (I - timefacrhs *I_target)
      // I_target is alway negative, since the reference electrode is the cathode
      residual = (*currresidual)[condid_cathode] - (timefacrhs*targetcurrent); // residual is stored only from cathode!

      // convergence test
      {
        if (myrank_==0)
        {
          // format output
          std::cout<<"\n  GALVANOSTATIC MODE:\n";
          std::cout<<"  +--------------------------------------------------------------------------" <<std::endl;
          std::cout<<"  | Convergence check: " <<std::endl;
          std::cout<<"  +--------------------------------------------------------------------------" <<std::endl;
          std::cout<<"  | iteration:                          "<<std::setw(7)<<std::right<<gstatnumite_<<" / "<<gstatitemax<<std::endl;
          std::cout<<"  | actual reaction current at cathode: "<<std::scientific<<std::setw(12)<<std::right<<(*actualcurrent)[condid_cathode]<<std::endl;
          std::cout<<"  | required total current at cathode:  "<<std::setw(12)<<std::right<<targetcurrent<<std::endl;
          std::cout<<"  | negative residual (rhs):            "<<std::setw(12)<<std::right<<residual<<std::endl;
          std::cout<<"  +--------------------------------------------------------------------------" <<std::endl;
        }

        if (gstatnumite_ > gstatitemax)
        {
          if (myrank_==0)
          {
            std::cout<<"  | --> maximum number iterations reached. Not yet converged!"<<std::endl;
            std::cout<<"  +--------------------------------------------------------------------------" <<std::endl <<std::endl;
          }
          return true; // we proceed to next time step
        }
        else if (abs(residual)< gstatcurrenttol)
        {
          if (myrank_==0)
          {
            std::cout<<"  | --> Newton-RHS-Residual is smaller than " << gstatcurrenttol<< "!" << std::endl;
            std::cout<<"  +--------------------------------------------------------------------------" <<std::endl <<std::endl;
          }
          return true; // we proceed to next time step
        }
        // electric potential increment of the last iteration
        else if ((gstatnumite_ > 1) and (abs(gstatincrement_)< (1+abs(potold))*tol)) // < ATOL + |pot|* RTOL
        {
          if (myrank_==0)
          {
            std::cout<<"  | --> converged: |"<<gstatincrement_<<"| < "<<(1+abs(potold))*tol<<std::endl;
            std::cout<<"  +--------------------------------------------------------------------------" <<std::endl <<std::endl;
          }
          return true; // galvanostatic control has converged
        }

        // safety check
        if (abs((*currtangent)[condid_cathode])<EPS13)
          dserror("Tangent in galvanostatic control is near zero: %lf",(*currtangent)[condid_cathode]);
      }

      // calculate the cell potential increment due to ohmic resistance
      if(approxelctresist==INPAR::ELCH::approxelctresist_effleninitcond)
      {
        // update applied electric potential
        // potential drop ButlerVolmer conditions (surface ovepotential) and in the electrolyte (ohmic overpotential) are conected in parallel:

        // 2 different versions:
        // I_0 = I_BV1 = I_ohmic = I_BV2
        // R(I_target, I) = R_BV1(I_target, I) = R_ohmic(I_target, I) = -R_BV2(I_target, I)
        // delta E_0 = delta U_BV1 + delta U_ohmic - (delta U_BV2)
        // => delta E_0 = (R_BV1(I_target, I)/J) + (R_ohmic(I_target, I)/J) - (-R_BV2(I_target, I)/J)
        resistance = effective_length/((*sigma_)[numscal_]*meanelectrodesurface);
        // potinc_ohm=(-1.0*effective_length)/((*sigma_)[numscal_]*(*electrodesurface)[condid_cathode])*residual/timefacrhs;
      }
      else if(approxelctresist==INPAR::ELCH::approxelctresist_relpotcur and cond.size()==2)
      {
        // actual potential difference is used to calculate the current path length
        // -> it is possible to compute the new ohmic potential step
        //    without the input parameter GSTAT_LENGTH_CURRENTPATH
        // actual current < 0,  since the reference electrode is the cathode
        // potdiffbulk > 0,     always positive (see definition)
        // -1.0,                resistance has to be positive
        resistance = -1.0*(potdiffbulk/(*actualcurrent)[condid_cathode]);
        // potinc_ohm = (potdiffbulk/(*actualcurrent)[condid_cathode])*residual/timefacrhs;
      }
//      else if(approxelctresist==INPAR::ELCH::approxelctresist_efflenintegcond and cond.size()==2)
//      {
//        double specificresistance = 0.0;
//        double eps = 0.0;
//        double tort = 0.0;
//        {
//          DRT::Element* actele = discret_->gElement(0);
//          Teuchos::RCP<MAT::Material> mat = actele->Material();
//
//          if (mat->MaterialType() == INPAR::MAT::m_elchmat)
//          {
//            const MAT::ElchMat* actmat = static_cast<const MAT::ElchMat*>(mat.get());
//
//            for (int iphase=0; iphase < actmat->NumPhase();++iphase)
//            {
//              const int phaseid = actmat->PhaseID(iphase);
//              Teuchos::RCP<const MAT::Material> singlephase = actmat->PhaseById(phaseid);
//
//              // dynmic cast: get access to mat_phase
//              const Teuchos::RCP<const MAT::ElchPhase>& actphase = Teuchos::rcp_dynamic_cast<const MAT::ElchPhase>(singlephase);
//
//              if(actphase->MaterialType() == INPAR::MAT::m_elchphase)
//              {
//                const MAT::ElchPhase* actsinglephase = static_cast<const MAT::ElchPhase*>(singlephase.get());
//
//                eps = actsinglephase->Epsilon();
//                tort = actsinglephase->Tortuosity();
//              }
//
//              // 2) loop over materials of the single phase
//              for (int imat=0; imat < actphase->NumMat();++imat)
//              {
//                const int matid = actphase->MatID(imat);
//                Teuchos::RCP<const MAT::Material> singlemat = actphase->MatById(matid);
//
//                if(singlemat->MaterialType() == INPAR::MAT::m_newman)
//                {
//                  const MAT::Newman* actsinglemat = static_cast<const MAT::Newman*>(singlemat.get());
//
//                  specificresistance = actsinglemat->ComputeApproxResistance((*meanconc)[condid_cathode],(*meanconc)[condid_anode]);
//                }
//              }
//            }
//          }
//          resistance = specificresistance*effective_length/meanelectrodesurface/(eps*tort);
//          //potinc_ohm = -specificresistance*effective_length/(*electrodesurface)[condid_cathode]/(eps*tort)*residual/timefacrhs;
//        }
//      }
      else
        dserror("The combination of the parameter GSTAT_APPROX_ELECT_RESIST %i and the number of electrodes %i\n"
                "is not valid!",approxelctresist,cond.size());

      // calculate increment due to ohmic resistance
      potinc_ohm=-1.0*resistance*residual/timefacrhs;

      // Do not update the cell potential for small currents
      if(abs((*actualcurrent)[condid_cathode]) < EPS10)
        potinc_ohm = 0.0;

      // the current flow at both electrodes has to be the same within the solution tolerances
      if(abs((*actualcurrent)[condid_cathode]+(*actualcurrent)[condid_anode])>EPS8)
      {
        if (myrank_==0)
        {
          std::cout << "Warning!!!" << std::endl;
          std::cout << "The difference of the current flow at anode and cathode is " << abs((*actualcurrent)[condid_cathode]+(*actualcurrent)[condid_anode])
                    << " larger than " << EPS8 << std::endl;
        }
      }

      // Newton step:  Jacobian * \Delta pot = - Residual
      const double potinc_cathode = residual/((-1)*currtangent_cathode);
      double potinc_anode = 0.0;
      if (abs(currtangent_anode)>EPS13) // anode surface overpotential is optional
      {
        potinc_anode = residual/((-1)*currtangent_anode);
      }
      gstatincrement_ = (potinc_cathode+potinc_anode+potinc_ohm);
      // update electric potential
      potnew += gstatincrement_;

      if(myrank_==0)
      {
        std::cout<<"  | ohmic potential increment is calculated based on" <<std::endl;
        if(approxelctresist==INPAR::ELCH::approxelctresist_effleninitcond)
          std::cout<<"  | the ohmic resistance is calculated based on GSTAT_LENGTH_CURRENTPATH and the initial conductivity!" <<std::endl;
        else if (approxelctresist==INPAR::ELCH::approxelctresist_relpotcur)
          std::cout<<"  | the ohmic resistance calculated from applied potential and current flow!" <<std::endl;
        else
          std::cout<<"  | the ohmic resistance is calculated based on GSTAT_LENGTH_CURRENTPATH and the integrated conductivity" <<std::endl;
        std::cout<<"  +--------------------------------------------------------------------------" <<std::endl;
        std::cout<<"  | Defined GSTAT_LENGTH_CURRENTPATH:               "<<std::setw(6)<<std::right<< effective_length << std::endl;

        if((*actualcurrent)[condid_cathode]!=0.0)
          std::cout<<"  | Resistance based on the initial conductivity:    "<<std::setw(6)<<std::right<< effective_length/((*sigma_)[numscal_]*meanelectrodesurface) <<std::endl;
        std::cout<<"  | Resistance based on .(see GSTAT_APPROX_ELECT_RESIST): "<<std::setw(6)<<std::right<< resistance <<std::endl;
        std::cout<<"  | New guess for:                                  "<<std::endl;
        std::cout<<"  | - ohmic potential increment:                    "<<std::setw(12)<<std::right<< potinc_ohm <<std::endl;
        std::cout<<"  | - overpotential increment cathode (condid " << condid_cathode <<"):   " <<std::setw(12)<<std::right<< potinc_cathode << std::endl;
        std::cout<<"  | - overpotential increment anode (condid " << condid_anode <<"):     " <<std::setw(12)<<std::right<< potinc_anode << std::endl;
        std::cout<<"  | -> total increment for potential:               " <<std::setw(12)<<std::right<< gstatincrement_ << std::endl;
        std::cout<<"  +--------------------------------------------------------------------------" <<std::endl;
        std::cout<<"  | old potential at the cathode (condid "<<condid_cathode <<"):     "<<std::setw(12)<<std::right<<potold<<std::endl;
        std::cout<<"  | new potential at the cathode (condid "<<condid_cathode <<"):     "<<std::setw(12)<<std::right<<potnew<<std::endl;
        std::cout<<"  +--------------------------------------------------------------------------" <<std::endl<<std::endl;
      }

//      // print additional information
//      if (myrank_==0)
//      {
//        std::cout<< "  actualcurrent - targetcurrent = " << ((*actualcurrent)[condid_cathode]-targetcurrent) << std::endl;
//        std::cout<< "  conductivity                  = " << (*sigma_)[numscal_] << std::endl<< std::endl;
//        std::cout<< "  currtangent_cathode:  " << currtangent_cathode << std::endl;
//        std::cout<< "  currtangent_anode:    " << currtangent_anode << std::endl;
//        std::cout<< "  actualcurrent cathode:    " << (*actualcurrent)[condid_cathode] << std::endl;
//        std::cout<< "  actualcurrent anode:  " << (*actualcurrent)[condid_anode] << std::endl;
//      }

      // replace potential value of the boundary condition (on all processors)
      cond[condid_cathode]->Add("pot",potnew);
      gstatnumite_++;
      return false; // not yet converged -> continue Newton iteration with updated potential
    }
  }
  return true; //default

} // end ApplyGalvanostaticControl()


/*----------------------------------------------------------------------*
 | evaluate contribution of electrode kinetics to eq. system  gjb 02/09 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::EvaluateElectrodeBoundaryConditions(
    Teuchos::RCP<LINALG::SparseOperator> matrix,
    Teuchos::RCP<Epetra_Vector>          rhs
)
{
  // time measurement: evaluate condition 'ElchBoundaryKinetics'
  TEUCHOS_FUNC_TIME_MONITOR("SCATRA:       + evaluate condition 'ElchBoundaryKinetics'");

  discret_->ClearState();

  // create parameter list
  Teuchos::ParameterList condparams;

  // action for elements
  condparams.set<int>("action",SCATRA::bd_calc_elch_boundary_kinetics);

  // parameters for Elch/DiffCond formulation
  condparams.sublist("DIFFCOND") = elchparams_->sublist("DIFFCOND");

  if (isale_)   // provide displacement field in case of ALE
    discret_->AddMultiVectorToParameterList(condparams,"dispnp",dispnp_);

  // add element parameters and set state vectors according to time-integration scheme
  AddTimeIntegrationSpecificVectors();

  // evaluate ElchBoundaryKinetics conditions at time t_{n+1} or t_{n+alpha_F}
  discret_->EvaluateCondition(condparams,matrix,Teuchos::null,rhs,Teuchos::null,Teuchos::null,"ElchBoundaryKinetics");
  discret_->ClearState();

  // Add linearization of NernstCondition to system matrix
  if(ektoggle_!=Teuchos::null)
    LinearizationNernstCondition();

  return;
} // ScaTraTimIntElch::EvaluateElectrodeBoundaryConditions


/*----------------------------------------------------------------------*
 | Add Linearization for Nernst-BC                           ehrl 09/13 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::LinearizationNernstCondition()
{
  // Blank rows with Nernst-BC (inclusive diagonal entry)
  // Nernst-BC is a additional constraint coupled to the original system of equation
  if(!sysmat_->Filled())
    sysmat_->Complete();
  sysmat_->ApplyDirichlet(ektoggle_,false);
  LINALG::ApplyDirichlettoSystem(increment_,residual_,zeros_,ektoggle_);

  discret_->ClearState();

  // create an parameter list
  Teuchos::ParameterList condparams;
  //update total time for time curve actions
  AddTimeIntegrationSpecificVectors();
  // action for elements
  condparams.set<int>("action",SCATRA::bd_calc_elch_linearize_nernst);

  // add element parameters and set state vectors according to time-integration scheme
  // we need here concentration at t+np
  discret_->SetState("phinp",phinp_);

  std::string condstring("ElchBoundaryKinetics");
  // evaluate ElchBoundaryKinetics conditions at time t_{n+1} or t_{n+alpha_F}
  // phinp (view to phinp)
  discret_->EvaluateCondition(condparams,sysmat_,Teuchos::null,residual_,Teuchos::null,Teuchos::null,condstring);
  discret_->ClearState();

  return;
} //  SCATRA::ScaTraTimIntImpl::LinearizationNernstCondition()


/*----------------------------------------------------------------------------*
 | evaluate solution-depending boundary and interface conditions   fang 10/14 |
 *----------------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::EvaluateSolutionDependingConditions(
    Teuchos::RCP<LINALG::SparseOperator> systemmatrix,      //!< system matrix
    Teuchos::RCP<Epetra_Vector>          rhs                //!< rhs vector
)
{
  // evaluate electrode boundary conditions
  EvaluateElectrodeBoundaryConditions(systemmatrix,rhs);

  // call base class routine
  ScaTraTimIntImpl::EvaluateSolutionDependingConditions(systemmatrix,rhs);

  return;
} // ScaTraTimIntElch::EvaluateSolutionDependingConditions


/*----------------------------------------------------------------------*
 | check for zero/negative concentration values               gjb 01/10 |
 *----------------------------------------------------------------------*/
void SCATRA::ScaTraTimIntElch::CheckConcentrationValues(Teuchos::RCP<Epetra_Vector> vec)
{
  // action only for ELCH applications

  // for NURBS discretizations we skip the following check.
  // Control points (i.e., the "nodes" and its associated dofs can be located
  // outside the domain of interest. Thus, they can have negative
  // concentration values although the concentration solution is positive
  // in the whole computational domain!
  if(dynamic_cast<DRT::NURBS::NurbsDiscretization*>(discret_.get())!=NULL)
    return;

  // this option can be helpful in some rare situations
  bool makepositive(false);

  std::vector<int> numfound(numscal_,0);
#if 0
  std::stringstream myerrormessage;
#endif
  for (int i = 0; i < discret_->NumMyRowNodes(); i++)
  {
    DRT::Node* lnode = discret_->lRowNode(i);
    std::vector<int> dofs;
    dofs = discret_->Dof(lnode);

    for (int k = 0; k < numscal_; k++)
    {
      const int lid = discret_->DofRowMap()->LID(dofs[k]);
      if (((*vec)[lid]) < EPS13 )
      {
        numfound[k]++;
        if (makepositive)
          ((*vec)[lid]) = EPS13;
#if 0
        myerrormessage<<"PROC "<<myrank_<<" dof index: "<<k<<setprecision(7)<<scientific<<
            " val: "<<((*vec)[lid])<<" node gid: "<<lnode->Id()<<
            " coord: [x] "<< lnode->X()[0]<<" [y] "<< lnode->X()[1]<<" [z] "<< lnode->X()[2]<<std::endl;
#endif
      }
    }
  }

  // print warning to screen
  for (int k = 0; k < numscal_; k++)
  {
    if (numfound[k] > 0)
    {
      std::cout<<"WARNING: PROC "<<myrank_<<" has "<<numfound[k]<<
      " nodes with zero/neg. concentration values for species "<<k;
      if (makepositive)
        std::cout<<"-> were made positive (set to 1.0e-13)"<<std::endl;
      else
        std::cout<<std::endl;
    }
  }

#if 0
  // print detailed info to error file
  for(int p=0; p < discret_->Comm().NumProc(); p++)
  {
    if (p==myrank_) // is it my turn?
    {
      // finish error message
      myerrormessage.flush();

      // write info to error file
      if ((errfile_!=NULL) and (myerrormessage.str()!=""))
      {
        fprintf(errfile_,myerrormessage.str().c_str());
        // std::cout<<myerrormessage.str()<<std::endl;
      }
    }
    // give time to finish writing to file before going to next proc ?
    discret_->Comm().Barrier();
  }
#endif

  // so much code for a simple check!
  return;
} // ScaTraTimIntImpl::CheckConcentrationValues


/*----------------------------------------------------------------------*
 | print header of convergence table to screen               fang 11/14 |
 *----------------------------------------------------------------------*/
inline void SCATRA::ScaTraTimIntElch::PrintConvergenceHeader()
{
  if (myrank_ == 0)
    std::cout << "+------------+-------------------+--------------+--------------+--------------+--------------+------------------+\n"
             << "|- step/max -|- tol      [norm] -|-- con-res ---|-- pot-res ---|-- con-inc ---|-- pot-inc ---|-- con-res-inf ---|" << std::endl;

  return;
} // SCATRA::ScaTraTimIntImpl::PrintConvergenceHeader


/*----------------------------------------------------------------------*
 | print first line of convergence table to screen           fang 11/14 |
 *----------------------------------------------------------------------*/
inline void SCATRA::ScaTraTimIntElch::PrintConvergenceValuesFirstIter(
    const int&              itnum,          //!< current Newton-Raphson iteration step
    const int&              itemax,         //!< maximum number of Newton-Raphson iteration steps
    const double&           ittol,          //!< relative tolerance for Newton-Raphson scheme
    const double&           conresnorm,     //!< L2 norm of concentration residual
    const double&           potresnorm,     //!< L2 norm of potential residual (only relevant for electrochemistry)
    const double&           conresnorminf   //!< infinity norm of concentration residual
)
{
  if (myrank_ == 0)
    std::cout << "|  " << std::setw(3) << itnum << "/" << std::setw(3) << itemax << "   | "
             << std::setw(10) << std::setprecision(3) << std::scientific << ittol << "[L_2 ]  | "
             << std::setw(10) << std::setprecision(3) << std::scientific << conresnorm << "   | "
             << std::setw(10) << std::setprecision(3) << std::scientific << potresnorm << "   |      --      |      --      | "
             << std::setw(10) << std::setprecision(3) << std::scientific << conresnorminf << "       | (      --     ,te="
             << std::setw(10) << std::setprecision(3) << std::scientific << dtele_ << ")" << std::endl;

  return;
} // SCATRA::ScaTraTimIntImpl::PrintConvergenceValuesFirstIter


/*----------------------------------------------------------------------*
 | print current line of convergence table to screen         fang 11/14 |
 *----------------------------------------------------------------------*/
inline void SCATRA::ScaTraTimIntElch::PrintConvergenceValues(
    const int&              itnum,           //!< current Newton-Raphson iteration step
    const int&              itemax,          //!< maximum number of Newton-Raphson iteration steps
    const double&           ittol,           //!< relative tolerance for Newton-Raphson scheme
    const double&           conresnorm,      //!< L2 norm of concentration residual
    const double&           potresnorm,      //!< L2 norm of potential residual (only relevant for electrochemistry)
    const double&           incconnorm_L2,   //!< L2 norm of concentration increment
    const double&           connorm_L2,      //!< L2 norm of concentration state vector
    const double&           incpotnorm_L2,   //!< L2 norm of potential increment
    const double&           potnorm_L2,      //!< L2 norm of potential state vector
    const double&           conresnorminf    //!< infinity norm of concentration residual
)
{
  if (myrank_ == 0)
    std::cout << "|  " << std::setw(3) << itnum << "/" << std::setw(3) << itemax << "   | "
             << std::setw(10) << std::setprecision(3) << std::scientific << ittol << "[L_2 ]  | "
             << std::setw(10) << std::setprecision(3) << std::scientific << conresnorm << "   | "
             << std::setw(10) << std::setprecision(3) << std::scientific << potresnorm << "   | "
             << std::setw(10) << std::setprecision(3) << std::scientific << incconnorm_L2/connorm_L2 << "   | "
             << std::setw(10) << std::setprecision(3) << std::scientific << incpotnorm_L2/potnorm_L2 << "   | "
             << std::setw(10) << std::setprecision(3) << std::scientific << conresnorminf << "       | (ts="
             << std::setw(10) << std::setprecision(3) << std::scientific << dtsolve_ << ",te="
             << std::setw(10) << std::setprecision(3) << std::scientific << dtele_ << ")" << std::endl;

  return;
} // SCATRA::ScaTraTimIntImpl::PrintConvergenceValues


/*----------------------------------------------------------------------*
 | print finish line of convergence table to screen          fang 11/14 |
 *----------------------------------------------------------------------*/
inline void SCATRA::ScaTraTimIntElch::PrintConvergenceFinishLine()
{
  if (myrank_ == 0)
    std::cout << "+------------+-------------------+--------------+--------------+--------------+--------------+------------------+" << std::endl << std::endl;

  return;
} // SCATRA::ScaTraTimIntImpl::PrintConvergenceFinishLine
