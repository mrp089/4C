#ifdef CCADISCRET

//#include "micromaterialgp.H"
#include "micromaterialgp_static.H"
#include "../drt_lib/drt_globalproblem.H"
#include "../drt_lib/drt_discret.H"
#include "../drt_lib/drt_utils.H"
#include "../drt_lib/drt_dserror.H"
#include "../drt_lib/linalg_utils.H"

//#include "../drt_stru_multi/microstrugenalpha.H"
#include "../drt_stru_multi/microstatic.H"

#include "../io/io_drt_micro.H"

using namespace std;
using namespace Teuchos;
using namespace IO;

/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | vector of numfld FIELDs, defined in global_control.c                 |
 *----------------------------------------------------------------------*/
extern struct _FIELD      *field;

/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | general problem data                                                 |
 | global variable GENPROB genprob is defined in global_control.c       |
 *----------------------------------------------------------------------*/
extern struct _GENPROB     genprob;

/*----------------------------------------------------------------------*
 | global variable *solv, vector of lenght numfld of structures SOLVAR  |
 | defined in solver_control.c                                          |
 |                                                                      |
 |                                                       m.gee 11/00    |
 *----------------------------------------------------------------------*/
extern struct _SOLVAR  *solv;

/*!----------------------------------------------------------------------
\brief file pointers

<pre>                                                         m.gee 8/00
This structure struct _FILES allfiles is defined in input_control_global.c
and the type is in standardtypes.h
It holds all file pointers and some variables needed for the FRSYSTEM
</pre>
*----------------------------------------------------------------------*/
extern struct _FILES  allfiles;

/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | structure of flags to control output                                 |
 | defined in out_global.c                                              |
 *----------------------------------------------------------------------*/
//extern struct _IO_FLAGS     ioflags;

extern struct _MATERIAL    *mat;


RefCountPtr<MicroStatic> MAT::MicroMaterialGP::microstatic_;


/// construct an instance of MicroMaterial for a given Gauss point and
/// microscale discretization

MAT::MicroMaterialGP::MicroMaterialGP(const int gp, const int ele_ID)
  : gp_(gp),
    ele_ID_(ele_ID)
{
  RefCountPtr<DRT::Problem> microproblem = DRT::Problem::Instance(1);
  RefCountPtr<DRT::Discretization> microdis = microproblem->Dis(0, 0);
  dism_ = LINALG::CreateVector(*microdis->DofRowMap(),true);
  dis_ = LINALG::CreateVector(*microdis->DofRowMap(),true);
}

/// destructor

MAT::MicroMaterialGP::~MicroMaterialGP()
{ }


/// Set up microscale generalized alpha

void MAT::MicroMaterialGP::SetUpMicroStatic()
{
  // -------------------------------------------------------------------
  // access the discretization
  // -------------------------------------------------------------------
  RefCountPtr<DRT::Discretization> actdis = null;
  actdis = DRT::Problem::Instance(1)->Dis(genprob.numsf,0);

  // set degrees of freedom in the discretization
  if (!actdis->Filled()) actdis->FillComplete();

  // -------------------------------------------------------------------
  // set some pointers and variables
  // -------------------------------------------------------------------
  const Teuchos::ParameterList& sdyn     = DRT::Problem::Instance()->StructuralDynamicParams();

  // -------------------------------------------------------------------
  // create a solver
  // -------------------------------------------------------------------
  // always choose UMFPACK as microstructural solver
  RefCountPtr<ParameterList> solveparams = rcp(new ParameterList());
  solveparams->set("solver","umfpack");
  solveparams->set("symmetric",false);
  RefCountPtr<LINALG::Solver> solver =
    rcp(new LINALG::Solver(solveparams,actdis->Comm(),allfiles.out_err));
  actdis->ComputeNullSpaceIfNecessary(*solveparams);

  // -------------------------------------------------------------------
  // create a static "time integrator"
  // -------------------------------------------------------------------
  RefCountPtr<ParameterList> params = rcp(new ParameterList());
  MicroStatic::SetDefaults(*params);

  params->set<double>("beta",sdyn.get<double>("BETA"));
  params->set<double>("gamma",sdyn.get<double>("GAMMA"));
  params->set<double>("alpha m",sdyn.get<double>("ALPHA_M"));
  params->set<double>("alpha f",sdyn.get<double>("ALPHA_F"));

  params->set<double>("total time",0.0);
  params->set<double>("delta time",sdyn.get<double>("TIMESTEP"));
  params->set<int>   ("step",0);
  params->set<int>   ("nstep",sdyn.get<int>("NUMSTEP"));
  params->set<int>   ("max iterations",sdyn.get<int>("MAXITER"));
  params->set<int>   ("num iterations",-1);
  params->set<double>("tolerance displacements",sdyn.get<double>("TOLDISP"));

  // takes values "full newton" , "modified newton" , "nonlinear cg"
  params->set<string>("equilibrium iteration","full newton");

  // takes values "constant" "consistent"
  params->set<string>("predictor","constant");

  microstatic_ = rcp(new MicroStatic(params,actdis,solver));
}


/// perform microscale simulation

void MAT::MicroMaterialGP::PerformMicroSimulation(const Epetra_SerialDenseMatrix* defgrd,
                                                  Epetra_SerialDenseVector* stress,
                                                  Epetra_SerialDenseMatrix* cmat,
                                                  double* density,
                                                  const double time,
                                                  string action)
{
  // if MicroDiscretizationWriter is not yet initialized -> set up
  // and write mesh

  if (micro_output_ == null)
  {
    RefCountPtr<DRT::Discretization> actdis = DRT::Problem::Instance(1)->Dis(genprob.numsf,0);

    micro_output_ = rcp(new MicroDiscretizationWriter(actdis, 1, ele_ID_, gp_));

    micro_output_->WriteMesh(0, 0.);

    // initialize total time, time step number and set time step size

    const Teuchos::ParameterList& sdyn     = DRT::Problem::Instance()->StructuralDynamicParams();
    timen_ = 0.;
    istep_ = 0;
    dt_    = sdyn.get<double>("TIMESTEP");
  }

  // if derived generalized alpha class for microscale simulations is
  // not yet initialized -> set up

  if (microstatic_ == null)
  {
    MAT::MicroMaterialGP::SetUpMicroStatic();
  }

  if (time != timen_)
  {
    microstatic_->UpdateNewTimeStep(dis_, dism_);
  }

  // set displacements, velocities and accelerations of last step
  microstatic_->SetOldState(dis_, dism_);

  // check if we have to update absolute time and step number
  if (time != timen_)
  {
    // Microscale data should be output when macroscale is entering a
    // new timestep, not in every macroscopic iteration! Therefore
    // output is written in the beginning of a microscopic step if
    // necessary at all. Problem: we don't get any output for the very
    // last time step since the macro-program finishes and the
    // micro-program is not called again to write output.

    // We don't want to write results after just having constructed
    // the StruGenAlpha class which corresponds to a total time of 0.
    // (in the calculation phase, total time is instantly set to the first
    // time step)
    if (timen_ != 0.)
      microstatic_->Output(micro_output_, timen_, istep_);
    timen_ = time;
    istep_++;
  }

  // set current absolute time, step number
  microstatic_->SetTime(timen_, istep_);

  microstatic_->ConstantPredictor(defgrd);
  microstatic_->FullNewton();
  //microstatic_->Homogenization(stress, cmat, density, defgrd, action);
  microstatic_->StaticHomogenization(stress, cmat, density, defgrd);

  // save calculated displacements, velocities and accelerations
  dism_ = microstatic_->ReturnNewDism();

  // clear displacements in MicroStruGenAlpha for next usage
  microstatic_->ClearState();
}

#endif
