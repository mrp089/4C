
#ifdef CCADISCRET

#include "elch_algorithm.H"

#include "../drt_lib/drt_globalproblem.H"
//#include "../drt_lib/drt_validparameters.H"
//#include <Teuchos_StandardParameterEntryValidators.hpp>


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


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
ELCH::Algorithm::Algorithm(Epetra_Comm& comm)
  :  FluidBaseAlgorithm(DRT::Problem::Instance()->FluidDynamicParams(),false),
     comm_(comm),
     step_(0),
     time_(0.0)
{
    // taking time loop control parameters out of fluid dynamics section
    const Teuchos::ParameterList& fluiddyn = DRT::Problem::Instance()->FluidDynamicParams();
    // maximum simulation time
    maxtime_=fluiddyn.get<double>("MAXTIME");
    // maximum number of timesteps
    nstep_ = fluiddyn.get<int>("NUMSTEP");
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
ELCH::Algorithm::~Algorithm()
{
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ELCH::Algorithm::TimeLoop()
{
    // print out the ELCH module logo
    if (Comm().MyPID() == 0)
    {
        cout<<"    _____ _     _____  _   _  "<<endl;
        cout<<"   |  ___| |   /  __ \\| | | | "<<endl;
        cout<<"   | |__ | |   | /  \\/| |_| | "<<endl;
        cout<<"   |  __|| |   | |    |  _  | "<<endl;
        cout<<"   | |___| |___| \\__/\\| | | | "<<endl;
        cout<<"   \\____/\\_____/\\____/\\_| |_/ "<<endl;
        cout<<"                               "<<endl;
        // more at http://www.ascii-art.de under entry "moose" (or "elk")
        cout<<"       ___            ___  "<<endl;
        cout<<"      /   \\          /   \\ "<<endl;
        cout<<"      \\_   \\        /  __/ "<<endl;
        cout<<"       _\\   \\      /  /__  "<<endl;
        cout<<"       \\___  \\____/   __/  "<<endl;
        cout<<"           \\_       _/     "<<endl;
        cout<<"             | @ @  \\_     "<<endl;
        cout<<"             |             "<<endl;
        cout<<"           _/     /\\       "<<endl;
        cout<<"          /o)  (o/\\ \\_     "<<endl;
        cout<<"          \\_____/ /        "<<endl;
        cout<<"            \\____/         "<<endl;
        cout<<"                           "<<endl;
    }
  
  // time loop
  while (NotFinished())
  {
    PrepareTimeStep();
    FluidField().NonlinearSolve();
    Update();
    Output();
  } // time loop
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ELCH::Algorithm::PrepareTimeStep()
{
  step_ += 1;
  time_ += dt_;

  FluidField().PrepareTimeStep();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ELCH::Algorithm::Update()
{
  FluidField().Update();
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ELCH::Algorithm::Output()
{
  // Note: The order is important here! In here control file entries are
  // written. And these entries define the order in which the filters handle
  // the Discretizations, which in turn defines the dof number ordering of the
  // Discretizations.
  FluidField().Output();

  FluidField().LiftDrag();
}


#endif // CCADISCRET
