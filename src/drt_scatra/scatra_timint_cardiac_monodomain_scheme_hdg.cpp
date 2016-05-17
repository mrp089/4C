/*----------------------------------------------------------------------*/
/*!
\file scatra_timint_cardiac_monodomain_scheme_hdg.cpp
\brief time-integration scheme for HDG with extensions for
       cardiac monodomain problems

<pre>
\level 3

\maintainer Julia Hoermann
            hoermann@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15264
</pre>
*/
/*----------------------------------------------------------------------*/

#include "scatra_timint_cardiac_monodomain_scheme_hdg.H"
#include "../drt_scatra_ele/scatra_ele_action.H"

#include "../drt_lib/drt_globalproblem.H"
#include "../drt_io/io.H"



/*----------------------------------------------------------------------*
 |  Constructor (public)                                 hoermann 09/15 |
 *----------------------------------------------------------------------*/
SCATRA::TimIntCardiacMonodomainHDG::TimIntCardiacMonodomainHDG(
    Teuchos::RCP<DRT::Discretization>      actdis,
    Teuchos::RCP<LINALG::Solver>           solver,
    Teuchos::RCP<Teuchos::ParameterList>   params,
    Teuchos::RCP<Teuchos::ParameterList>   sctratimintparams,
    Teuchos::RCP<Teuchos::ParameterList>   extraparams,
    Teuchos::RCP<IO::DiscretizationWriter> output)
: ScaTraTimIntImpl(actdis,solver,sctratimintparams,extraparams,output),
  TimIntCardiacMonodomain(actdis,solver,params,sctratimintparams,extraparams,output),
  TimIntHDG(actdis,solver,sctratimintparams,extraparams,output)
{
  return;
}

/*----------------------------------------------------------------------*
| Destructor dtor (public)                               hoermann 09/15 |
*-----------------------------------------------------------------------*/
SCATRA::TimIntCardiacMonodomainHDG::~TimIntCardiacMonodomainHDG()
{
  return;
}

/*----------------------------------------------------------------------*
 |  initialize time integration                          hoermann 09/15 |
 *----------------------------------------------------------------------*/
void SCATRA::TimIntCardiacMonodomainHDG::Init()
{
  // call Init()-functions of base classes
  // note: this order is important
  TimIntHDG::Init();
  TimIntCardiacMonodomain::Init();

  // Activation time at time n+1
  activation_time_interpol_.reset(new Epetra_Vector(*discret_->NodeRowMap()));

  return;
}

/*----------------------------------------------------------------------*
 | current solution becomes most recent solution of next timestep       |
 |                                                       hoermann 09/15 |
 *----------------------------------------------------------------------*/
void SCATRA::TimIntCardiacMonodomainHDG::Update(const int num)
{
  // Standard Update
  TimIntHDG::Update(num);

  // time update of myocard material
  ElementMaterialTimeUpdate();

  return;
}

/*----------------------------------------------------------------------*
 | write additional data required for restart            hoermann 09/15 |
 *----------------------------------------------------------------------*/
void SCATRA::TimIntCardiacMonodomainHDG::OutputRestart()
{

  // Call function from baseclass
  TimIntHDG::OutputRestart();

  // Cardiac Monodomain specific
  output_->WriteMesh(step_,time_); // add info to control file for reading all variables in restart

  return;
}


/*----------------------------------------------------------------------*
 |                                                       hoermann 09/15 |
 -----------------------------------------------------------------------*/
void SCATRA::TimIntCardiacMonodomainHDG::ReadRestart(const int step)
{

  dserror("Restart not implemented yet");

  // Call function from baseclass
  TimIntHDG::ReadRestart(step);

  IO::DiscretizationReader reader(discret_,step);

  reader.ReadMultiVector(material_internal_state_np_, "mat_int_state");
  // Recover internal state of the material (for electrophysiology) for restart
  Teuchos::ParameterList params;
  params.set<int>("action", SCATRA::set_material_internal_state);
  params.set< Teuchos::RCP<Epetra_MultiVector> >("material_internal_state", material_internal_state_np_);     // Probably do it once at the beginning
  discret_->Evaluate(params);

  return;
}

/*----------------------------------------------------------------------*
 | time update of time-dependent materials               hoermann 09/15 |
 *----------------------------------------------------------------------*/
void SCATRA::TimIntCardiacMonodomainHDG::ElementMaterialTimeUpdate()
{
  discret_->ClearState(true);

  Teuchos::ParameterList eleparams;
  eleparams.set<int>("action",SCATRA::time_update_material);

  discret_->SetState("phiaf",phinp_);
  discret_->SetState(nds_intvar_, "intphin",intphin_);
  discret_->SetState(0, "phin", phin_);


  Epetra_SerialDenseMatrix dummyMat;
  Epetra_SerialDenseVector dummyVec;
  DRT::Element::LocationArray la(discret_->NumDofSets());


  for (int iele=0; iele<discret_->NumMyColElements(); ++iele)
  {
    DRT::Element *ele = discret_->lColElement(iele);
    ele->LocationVector(*discret_,la,false);

    ele->Evaluate(eleparams,*discret_,la,dummyMat,dummyMat,dummyVec,dummyVec,dummyVec);
  }

  discret_->ClearState(true);
  return;
}

/*----------------------------------------------------------------------*
 |  write current state to BINIO                          hoermann 09/15|
 *----------------------------------------------------------------------*/
void SCATRA::TimIntCardiacMonodomainHDG::OutputState()
{
  // Call function from base class
  SCATRA::TimIntHDG::OutputState();

  material_internal_state_np_->PutScalar(0.0);
  Teuchos::ParameterList params;
  params.set<int>("action", SCATRA::get_material_internal_state);
  params.set< Teuchos::RCP<Epetra_MultiVector> >("material_internal_state", material_internal_state_np_);
  discret_->Evaluate(params);
  material_internal_state_np_ = params.get< Teuchos::RCP<Epetra_MultiVector> >("material_internal_state");
  if (material_internal_state_np_==Teuchos::null)
    dserror("Cannot get state vector material internal state");

  output_->WriteVector("ionic_currents_hdg",material_internal_state_np_);

 for(int k = 0; k < material_internal_state_np_->NumVectors(); ++k)
   {
     std::ostringstream temp;
     temp << k+1;
     material_internal_state_np_component_ = Teuchos::rcp((*material_internal_state_np_)(k),false);
     output_->WriteVector("mat_int_state_hdg"+temp.str(), material_internal_state_np_component_,IO::DiscretizationWriter::elementvector);
   }


  return;
}

/*----------------------------------------------------------------------*
 |  write problem specific output                         hoermann 09/15|
 *----------------------------------------------------------------------*/
void SCATRA::TimIntCardiacMonodomainHDG::WriteProblemSpecificOutput(
    Teuchos::RCP<Epetra_Vector>    interpolatedPhi
     )
{
  //Compute and write activation time
  if (activation_time_interpol_ != Teuchos::null){
    for(int k=0;k<interpolatedPhi->MyLength();k++){
      if( (*interpolatedPhi)[k] >= activation_threshold_ && (*activation_time_interpol_)[k] <= dta_*0.9)
        (*activation_time_interpol_)[k] =  time_;
    }
    output_->WriteVector("activation_time_np_hdg", activation_time_interpol_, IO::DiscretizationWriter::nodevector);
  }
}
