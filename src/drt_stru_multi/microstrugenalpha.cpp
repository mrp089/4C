/*!----------------------------------------------------------------------
\file microstrugenalpha.cpp
\brief Generalized Alpha time integration for microstructural problems
in case of multiscale analyses

<pre>
Maintainer: Lena Wiechert
            wiechert@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15303
</pre>

*----------------------------------------------------------------------*/

#ifdef CCADISCRET

#include <Epetra_LinearProblem.h>
#include <Amesos_Klu.h>

#include "microstrugenalpha.H"

#include <vector>

#include "../drt_lib/drt_condition.H"
#include "../drt_lib/drt_globalproblem.H"
#include "../io/io_drt_micro.H"

using namespace IO;

/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | general problem data                                                 |
 | global variable GENPROB genprob is defined in global_control.c       |
 *----------------------------------------------------------------------*/
extern struct _GENPROB     genprob;

/*----------------------------------------------------------------------*
 |  ctor (public)                                            mwgee 03/07|
 *----------------------------------------------------------------------*/
MicroStruGenAlpha::MicroStruGenAlpha(RefCountPtr<ParameterList> params,
                                     RefCountPtr<DRT::Discretization> dis,
                                     RefCountPtr<LINALG::Solver> solver) :
params_(params),
discret_(dis),
solver_(solver)
{
  // -------------------------------------------------------------------
  // get some parameters from parameter list
  // -------------------------------------------------------------------
  double time    = params_->get<double>("total time"      ,0.0);
  double dt      = params_->get<double>("delta time"      ,0.01);
  bool damping   = params_->get<bool>  ("damping"         ,false);
  double kdamp   = params_->get<double>("damping factor K",0.0);
  double mdamp   = params_->get<double>("damping factor M",0.0);
  int istep      = params_->get<int>   ("step"            ,0);

  // -------------------------------------------------------------------
  // get a vector layout from the discretization to construct matching
  // vectors and matrices
  // -------------------------------------------------------------------
  if (!discret_->Filled()) discret_->FillComplete();
  const Epetra_Map* dofrowmap = discret_->DofRowMap();
  myrank_ = discret_->Comm().MyPID();

  // -------------------------------------------------------------------
  // create empty matrices
  // -------------------------------------------------------------------
  stiff_ = LINALG::CreateMatrix(*dofrowmap,81);
  mass_  = LINALG::CreateMatrix(*dofrowmap,81);
  if (damping) damp_ = LINALG::CreateMatrix(*dofrowmap,81);

  // -------------------------------------------------------------------
  // create empty vectors
  // -------------------------------------------------------------------
  // a zero vector of full length
  zeros_ = LINALG::CreateVector(*dofrowmap,true);
  // vector of full length; for each component
  //                /  1   i-th DOF is supported, ie Dirichlet BC
  //    vector_i =  <
  //                \  0   i-th DOF is free
  dirichtoggle_ = LINALG::CreateVector(*dofrowmap,true);
  // opposite of dirichtoggle vector, ie for each component
  //                /  0   i-th DOF is supported, ie Dirichlet BC
  //    vector_i =  <
  //                \  1   i-th DOF is free
  invtoggle_ = LINALG::CreateVector(*dofrowmap,false);

  // displacements D_{n} at last time
  dis_ = LINALG::CreateVector(*dofrowmap,true);
  // velocities V_{n} at last time
  vel_ = LINALG::CreateVector(*dofrowmap,true);
  // accelerations A_{n} at last time
  acc_ = LINALG::CreateVector(*dofrowmap,true);

  // displacements D_{n+1} at new time
  disn_ = LINALG::CreateVector(*dofrowmap,true);
  // velocities V_{n+1} at new time
  veln_ = LINALG::CreateVector(*dofrowmap,true);
  // accelerations A_{n+1} at new time
  accn_ = LINALG::CreateVector(*dofrowmap,true);

  // mid-displacements D_{n+1-alpha_f}
  dism_ = LINALG::CreateVector(*dofrowmap,true);
  // mid-velocities V_{n+1-alpha_f}
  velm_ = LINALG::CreateVector(*dofrowmap,true);
  // mid-accelerations A_{n+1-alpha_m}
  accm_ = LINALG::CreateVector(*dofrowmap,true);

  // iterative displacement increments IncD_{n+1}
  // also known as residual displacements
  disi_ = LINALG::CreateVector(*dofrowmap,true);

  // internal force vector F_int at different times
  fint_ = LINALG::CreateVector(*dofrowmap,true);
  // external force vector F_ext at last times
  fext_ = LINALG::CreateVector(*dofrowmap,true);
  // external mid-force vector F_{ext;n+1-alpha_f}
  fextm_ = LINALG::CreateVector(*dofrowmap,true);
  // external force vector F_{n+1} at new time
  fextn_ = LINALG::CreateVector(*dofrowmap,true);

  // dynamic force residual at mid-time R_{n+1-alpha}
  // also known at out-of-balance-force
  fresm_ = LINALG::CreateVector(*dofrowmap,false);

  // dynamic force residual at mid-time R_{n+1-alpha}
  // holding also boundary forces due to Dirichlet/Microboundary
  // conditions
  fresm_dirich_ = LINALG::CreateVector(*dofrowmap,false);

  //-------------------------------------------- calculate external forces
  {
    ParameterList p;
    // action for elements
    p.set("action","calc_struct_eleload");
    // choose what to assemble
    p.set("assemble matrix 1",false);
    p.set("assemble matrix 2",false);
    p.set("assemble vector 1",true);
    p.set("assemble vector 2",false);
    p.set("assemble vector 3",false);
    // other parameters needed by the elements
    p.set("total time",time);
    p.set("delta time",dt);
    // set vector values needed by elements
    discret_->ClearState();
    discret_->SetState("displacement",dis_);
    fext_->PutScalar(0.);
  }

  // -------------------------------------------------------------------
  // call elements to calculate stiffness and mass
  // -------------------------------------------------------------------
  {
    // create the parameters for the discretization
    ParameterList p;
    // action for elements
    p.set("action","calc_struct_nlnstiffmass");
    // choose what to assemble
    p.set("assemble matrix 1",true);
    p.set("assemble matrix 2",true);
    p.set("assemble vector 1",true);
    p.set("assemble vector 2",false);
    p.set("assemble vector 3",false);
    // other parameters that might be needed by the elements
    p.set("total time",time);
    p.set("delta time",dt);
    // set vector values needed by elements
    discret_->ClearState();
    discret_->SetState("residual displacement",zeros_);
    discret_->SetState("displacement",dis_);
    //discret_.SetState("velocity",vel_); // not used at the moment
    discret_->Evaluate(p,stiff_,mass_,fint_,null,null);
    discret_->ClearState();
  }
  // build damping matrix if desired
  LINALG::Complete(*mass_);
  maxentriesperrow_ = mass_->MaxNumEntries();
  if (damping)
  {
    LINALG::Complete(*stiff_);
    LINALG::Add(*stiff_,false,kdamp,*damp_,0.0);
    stiff_ = null;
    LINALG::Add(*mass_,false,mdamp,*damp_,1.0);
    LINALG::Complete(*damp_);
  }

  //--------------------------- calculate consistent initial accelerations
  {
    RefCountPtr<Epetra_Vector> rhs = LINALG::CreateVector(*dofrowmap,true);
    if (damping) damp_->Multiply(false,*vel_,*rhs);
    rhs->Update(-1.0,*fint_,1.0,*fext_,-1.0);
    Epetra_Vector rhscopy(*rhs);
    rhs->Multiply(1.0,*invtoggle_,rhscopy,0.0);
    solver->Solve(mass_,acc_,rhs,true,true);
  }

  //------------------------------------------------------ time step index
  istep = 0;
  params_->set<int>("step",istep);

  // Determine dirichtoggle_ and its inverse since boundary conditions for
  // microscale simulations are due to the MicroBoundary condition
  // (and not Dirichlet BC)

  MicroStruGenAlpha::DetermineToggle();
  MicroStruGenAlpha::SetUpHomogenization();

  //----------------------- compute an inverse of the dirichtoggle vector
  invtoggle_->PutScalar(1.0);
  invtoggle_->Update(-1.0,*dirichtoggle_,1.0);

  // -------------------------- Calculate initial volume of microstructure
  ParameterList p;
  // action for elements
  p.set("action","calc_init_vol");
  discret_->Evaluate(p,null,null,null,null,null);
  V0_ = p.get<double>("V0", -1.0);

  return;
} // MicroStruGenAlpha::MicroStruGenAlpha


/*----------------------------------------------------------------------*
 |  do constant predictor step (public)                      mwgee 03/07|
 *----------------------------------------------------------------------*/
void MicroStruGenAlpha::ConstantPredictor(const Epetra_SerialDenseMatrix* defgrd)
{
  // -------------------------------------------------------------------
  // get some parameters from parameter list
  // -------------------------------------------------------------------
  double time        = params_->get<double>("total time"     ,0.0);
  double dt          = params_->get<double>("delta time"     ,0.01);
  int    istep       = params_->get<int>   ("step"           ,0);
  bool   damping     = params_->get<bool>  ("damping"        ,false);
  double alphaf      = params_->get<double>("alpha f"        ,0.459);
  const Epetra_Map* dofrowmap = discret_->DofRowMap();

  // increment time and step
  double timen = time += dt;
  istep++;
  params_->set<double>("total time",timen);
  params_->set<int>   ("step"      ,istep);

  //--------------------------------------------------- predicting state
  // constant predictor : displacement in domain
  disn_->Update(1.0,*dis_,0.0);

  // constant predictor
  veln_->Update(1.0,*vel_,0.0);
  accn_->Update(1.0,*acc_,0.0);

  //------------------------------ compute interpolated dis, vel and acc
  // constant predictor
  // mid-displacements D_{n+1-alpha_f} (dism)
  //    D_{n+1-alpha_f} := (1.-alphaf) * D_{n+1} + alpha_f * D_{n}
  dism_->Update(1.-alphaf,*disn_,alphaf,*dis_,0.0);
  velm_->Update(1.0,*vel_,0.0);
  accm_->Update(1.0,*acc_,0.0);

  // apply new displacements at DBCs -> this has to be done with the
  // mid-displacements since the given macroscopic deformation
  // gradient is evaluated at the mid-point!
  {
    // dism then also holds prescribed new dirichlet displacements
    EvaluateMicroBC(defgrd);
    discret_->ClearState();
    fextn_->PutScalar(0.0);  // initialize external force vector (load vect)
  }

  //------------------------------- compute interpolated external forces
  // external mid-forces F_{ext;n+1-alpha_f} (fextm)
  //    F_{ext;n+1-alpha_f} := (1.-alphaf) * F_{ext;n+1}
  //                         + alpha_f * F_{ext;n}
  fextm_->Update(1.-alphaf,*fextn_,alphaf,*fext_,0.0);

  //------------- eval fint at interpolated state, eval stiffness matrix
  {
    // zero out stiffness
    stiff_ = LINALG::CreateMatrix(*dofrowmap,maxentriesperrow_);
    // create the parameters for the discretization
    ParameterList p;
    // action for elements
    p.set("action","calc_struct_nlnstiff");
    // choose what to assemble
    p.set("assemble matrix 1",true);
    p.set("assemble matrix 2",false);
    p.set("assemble vector 1",true);
    p.set("assemble vector 2",false);
    p.set("assemble vector 3",false);
    // other parameters that might be needed by the elements
    p.set("total time",timen);
    p.set("delta time",dt);
    // set vector values needed by elements
    discret_->ClearState();
    discret_->SetState("residual displacement",disi_);
    discret_->SetState("displacement",dism_);
    //discret_.SetState("velocity",velm_); // not used at the moment
    fint_->PutScalar(0.0);  // initialise internal force vector
    discret_->Evaluate(p,stiff_,null,fint_,null,null);
    discret_->ClearState();
    // do NOT finalize the stiffness matrix, add mass and damping to it later
  }

  //-------------------------------------------- compute residual forces
  // Res = M . A_{n+1-alpha_m}
  //     + C . V_{n+1-alpha_f}
  //     + F_int(D_{n+1-alpha_f})
  //     - F_{ext;n+1-alpha_f}
  // add mid-inertial force
  mass_->Multiply(false,*accm_,*fresm_);
  // add mid-viscous damping force
  if (damping)
  {
      RefCountPtr<Epetra_Vector> fviscm = LINALG::CreateVector(*dofrowmap,true);
      damp_->Multiply(false,*velm_,*fviscm);
      fresm_->Update(1.0,*fviscm,1.0);
  }

  // add static mid-balance
  fresm_->Update(1.0,*fint_,-1.0,*fextm_,1.0);

  // blank residual at DOFs on Dirichlet BC
  {
    Epetra_Vector fresmcopy(*fresm_);

    // save this vector for homogenization
    *fresm_dirich_ = fresmcopy;

    fresm_->Multiply(1.0,*invtoggle_,fresmcopy,0.0);
  }

  //------------------------------------------------ build residual norm
  fresm_->Norm2(&norm_);

  return;
} // MicroStruGenAlpha::ConstantPredictor()


/*----------------------------------------------------------------------*
 |  do Newton iteration (public)                             mwgee 03/07|
 *----------------------------------------------------------------------*/
void MicroStruGenAlpha::FullNewton()
{
  // -------------------------------------------------------------------
  // get some parameters from parameter list
  // -------------------------------------------------------------------
  double time      = params_->get<double>("total time"             ,0.0);
  double dt        = params_->get<double>("delta time"             ,0.01);
  int    maxiter   = params_->get<int>   ("max iterations"         ,10);
  bool   damping   = params_->get<bool>  ("damping"                ,false);
  double beta      = params_->get<double>("beta"                   ,0.292);
  double gamma     = params_->get<double>("gamma"                  ,0.581);
  double alpham    = params_->get<double>("alpha m"                ,0.378);
  double alphaf    = params_->get<double>("alpha f"                ,0.459);
  double toldisp   = params_->get<double>("tolerance displacements",1.0e-07);
  const Epetra_Map* dofrowmap = discret_->DofRowMap();

  // check whether we have a stiffness matrix, that is not filled yet
  // and mass and damping are present
  if (stiff_->Filled()) dserror("stiffness matrix may not be filled here");
  if (!mass_->Filled()) dserror("mass matrix must be filled here");
  if (damping)
    if (!damp_->Filled()) dserror("damping matrix must be filled here");

  //=================================================== equilibrium loop
  int numiter=0;

  while (norm_>toldisp && numiter<=maxiter)
  {
    //------------------------------------------- effective rhs is fresm
    //---------------------------------------------- build effective lhs
    // (using matrix stiff_ as effective matrix)
    LINALG::Add(*mass_,false,(1.-alpham)/(beta*dt*dt),*stiff_,1.-alphaf);
    if (damping)
      LINALG::Add(*damp_,false,(1.-alphaf)*gamma/(beta*dt),*stiff_,1.0);
    LINALG::Complete(*stiff_);

    //----------------------- apply dirichlet BCs to system of equations
    fresm_->Scale(-1.0);   // delete this by building fresm with other sign
    disi_->PutScalar(0.0);  // Useful? depends on solver and more

    LINALG::ApplyDirichlettoSystem(stiff_,disi_,fresm_,zeros_,dirichtoggle_);

    //--------------------------------------------------- solve for disi
    // Solve K_Teffdyn . IncD = -R  ===>  IncD_{n+1}
    if (!numiter)
      solver_->Solve(stiff_,disi_,fresm_,true,true);
    else
      solver_->Solve(stiff_,disi_,fresm_,true,false);
    stiff_ = null;

    //---------------------------------- update mid configuration values
    // displacements
    // D_{n+1-alpha_f} := D_{n+1-alpha_f} + (1-alpha_f)*IncD_{n+1}
    dism_->Update(1.-alphaf,*disi_,1.0);
    // velocities
    // iterative
    // V_{n+1-alpha_f} := V_{n+1-alpha_f}
    //                  + (1-alpha_f)*gamma/beta/dt*IncD_{n+1}
    //velm_->Update((1.-alphaf)*gamma/(beta*dt),*disi_,1.0);
    // incremental (required for constant predictor)
    velm_->Update(1.0,*dism_,-1.0,*dis_,0.0);
    velm_->Update((beta-(1.0-alphaf)*gamma)/beta,*vel_,
                  (1.0-alphaf)*(2.*beta-gamma)*dt/(2.*beta),*acc_,
                  gamma/(beta*dt));
    // accelerations
    // iterative
    // A_{n+1-alpha_m} := A_{n+1-alpha_m}
    //                  + (1-alpha_m)/beta/dt^2*IncD_{n+1}
    //accm_->Update((1.-alpham)/(beta*dt*dt),*disi_,1.0);
    // incremental (required for constant predictor)
    accm_->Update(1.0,*dism_,-1.0,*dis_,0.0);
    accm_->Update(-(1.-alpham)/(beta*dt),*vel_,
                  (2.*beta-1.+alpham)/(2.*beta),*acc_,
                  (1.-alpham)/((1.-alphaf)*beta*dt*dt));

    //---------------------------- compute internal forces and stiffness
    {
      // zero out stiffness
      stiff_ = LINALG::CreateMatrix(*dofrowmap,maxentriesperrow_);
      // create the parameters for the discretization
      ParameterList p;
      // action for elements
      p.set("action","calc_struct_nlnstiff");
      // choose what to assemble
      p.set("assemble matrix 1",true);
      p.set("assemble matrix 2",false);
      p.set("assemble vector 1",true);
      p.set("assemble vector 2",false);
      p.set("assemble vector 3",false);
      // other parameters that might be needed by the elements
      p.set("total time",time);
      p.set("delta time",dt);
      // set vector values needed by elements
      discret_->ClearState();
      discret_->SetState("residual displacement",disi_);
      discret_->SetState("displacement",dism_);
      //discret_.SetState("velocity",velm_); // not used at the moment
      fint_->PutScalar(0.0);  // initialise internal force vector
      discret_->Evaluate(p,stiff_,null,fint_,null,null);
      discret_->ClearState();
      // do NOT finalize the stiffness matrix to add masses to it later
    }

    //------------------------------------------ compute residual forces
    // Res = M . A_{n+1-alpha_m}
    //     + C . V_{n+1-alpha_f}
    //     + F_int(D_{n+1-alpha_f})
    //     - F_{ext;n+1-alpha_f}
    // add inertia mid-forces
    mass_->Multiply(false,*accm_,*fresm_);
    // add viscous mid-forces
    if (damping)
    {
      RefCountPtr<Epetra_Vector> fviscm = LINALG::CreateVector(*dofrowmap,false);
      damp_->Multiply(false,*velm_,*fviscm);
      fresm_->Update(1.0,*fviscm,1.0);
    }
    // add static mid-balance
    fresm_->Update(1.0,*fint_,-1.0,*fextm_,1.0);
    // blank residual DOFs which are on Dirichlet BC
    {
      Epetra_Vector fresmcopy(*fresm_);
      fresm_->Multiply(1.0,*invtoggle_,fresmcopy,0.0);
    }

    //---------------------------------------------- build residual norm
    double disinorm;
    disi_->Norm2(&disinorm);

    fresm_->Norm2(&norm_);

    norm_ = disinorm;

    //--------------------------------- increment equilibrium loop index
    ++numiter;

  } // while (norm_>toldisp && numiter<=maxiter)

  //============================================= end equilibrium loop

  //-------------------------------- test whether max iterations was hit
  if (numiter>=maxiter) dserror("Newton unconverged in %d iterations",numiter);
  params_->set<int>("num iterations",numiter);

  //stiff_ = null;   -> microscale needs this for homogenization purposes

  return;
} // MicroStruGenAlpha::FullNewton()


/*----------------------------------------------------------------------*
 |  do Update (public)                                       mwgee 03/07|
 *----------------------------------------------------------------------*/
void MicroStruGenAlpha::Update()
{
  // -------------------------------------------------------------------
  // get some parameters from parameter list
  // -------------------------------------------------------------------
  double time          = params_->get<double>("total time"             ,0.0);
  double dt            = params_->get<double>("delta time"             ,0.01);

  double alpham        = params_->get<double>("alpha m"                ,0.378);
  double alphaf        = params_->get<double>("alpha f"                ,0.459);

  //---------------------------- determine new end-quantities and update
  // new displacements at t_{n+1} -> t_n
  //    D_{n} := D_{n+1} = 1./(1.-alphaf) * D_{n+1-alpha_f}
  //                     - alphaf/(1.-alphaf) * D_n
  dis_->Update(1./(1.-alphaf),*dism_,-alphaf/(1.-alphaf));
  // new velocities at t_{n+1} -> t_n
  //    V_{n} := V_{n+1} = 1./(1.-alphaf) * V_{n+1-alpha_f}
  //                     - alphaf/(1.-alphaf) * V_n
  vel_->Update(1./(1.-alphaf),*velm_,-alphaf/(1.-alphaf));
  // new accelerations at t_{n+1} -> t_n
  //    A_{n} := A_{n+1} = 1./(1.-alpham) * A_{n+1-alpha_m}
  //                     - alpham/(1.-alpham) * A_n
  acc_->Update(1./(1.-alpham),*accm_,-alpham/(1.-alpham));
  // update new external force
  //    F_{ext;n} := F_{ext;n+1}
  fext_->Update(1.0,*fextn_,0.0);

  //----- update anything that needs to be updated at the element level
  {
    // create the parameters for the discretization
    ParameterList p;
    // action for elements
    p.set("action","calc_struct_update_istep");
    // choose what to assemble
    p.set("assemble matrix 1",false);
    p.set("assemble matrix 2",false);
    p.set("assemble vector 1",false);
    p.set("assemble vector 2",false);
    p.set("assemble vector 3",false);
    // other parameters that might be needed by the elements
    p.set("total time",time);
    p.set("delta time",dt);
    discret_->Evaluate(p,null,null,null,null,null);
  }

  return;
} // MicroStruGenAlpha::Update()


/*----------------------------------------------------------------------*
 |  write output (public)                                    mwgee 03/07|
 *----------------------------------------------------------------------*/
void MicroStruGenAlpha::Output(RefCountPtr<MicroDiscretizationWriter> output,
                               const double time,
                               const int istep)
{
  // -------------------------------------------------------------------
  // get some parameters from parameter list
  // -------------------------------------------------------------------

  bool   iodisp        = params_->get<bool>  ("io structural disp"     ,true);
  int    updevrydisp   = params_->get<int>   ("io disp every nstep"    ,1);

  //----------------------------------------------------- output results
  if (iodisp && updevrydisp && istep%updevrydisp==0)
  {
    output->NewStep(istep, time);
    output->WriteVector("displacement",dis_);
    //output->WriteVector("velocity",vel_);
    //output->WriteVector("acceleration",acc_);
  }

  return;
} // MicroStruGenAlpha::Output()


/*----------------------------------------------------------------------*
 |  set default parameter list (static/public)               mwgee 03/07|
 *----------------------------------------------------------------------*/
void MicroStruGenAlpha::SetDefaults(ParameterList& params)
{
  params.set<bool>  ("print to screen"        ,false);
  params.set<bool>  ("print to err"           ,false);
  params.set<FILE*> ("err file"               ,NULL);
  params.set<bool>  ("damping"                ,false);
  params.set<double>("damping factor K"       ,0.00001);
  params.set<double>("damping factor M"       ,0.00001);
  params.set<double>("beta"                   ,0.292);
  params.set<double>("gamma"                  ,0.581);
  params.set<double>("alpha m"                ,0.378);
  params.set<double>("alpha f"                ,0.459);
  params.set<double>("total time"             ,0.0);
  params.set<double>("delta time"             ,0.01);
  params.set<int>   ("step"                   ,0);
  params.set<int>   ("nstep"                  ,5);
  params.set<int>   ("max iterations"         ,10);
  params.set<int>   ("num iterations"         ,-1);
  params.set<double>("tolerance displacements",1.0e-07);
  params.set<bool>  ("io structural disp"     ,true);
  params.set<int>   ("io disp every nstep"    ,1);
  params.set<bool>  ("io structural stress"   ,false);
  params.set<int>   ("restart"                ,0);
  params.set<int>   ("write restart every"    ,0);
  // takes values "constant" consistent"
  params.set<string>("predictor"              ,"constant");
  // takes values "full newton" , "modified newton" , "nonlinear cg"
  params.set<string>("equilibrium iteration"  ,"full newton");
  return;
}


/*----------------------------------------------------------------------*
 |  dtor (public)                                            mwgee 03/07|
 *----------------------------------------------------------------------*/
MicroStruGenAlpha::~MicroStruGenAlpha()
{
  return;
}


void MicroStruGenAlpha::DetermineToggle()
{
  int np = 0;   // number of prescribed (=boundary) dofs needed for the
                // creation of vectors and matrices for homogenization
                // procedure

  RefCountPtr<DRT::Discretization> dis =
    DRT::Problem::Instance(1)->Dis(genprob.numsf,0);

  vector<DRT::Condition*> conds;
  dis->GetCondition("MicroBoundary", conds);
  for (unsigned i=0; i<conds.size(); ++i)
  {
    const vector<int>* nodeids = conds[i]->Get<vector<int> >("Node Ids");
    if (!nodeids) dserror("Dirichlet condition does not have nodal cloud");
    const int nnode = (*nodeids).size();

    for (int i=0; i<nnode; ++i)
    {
      // do only nodes in my row map
      if (!dis->NodeRowMap()->MyGID((*nodeids)[i])) continue;
      DRT::Node* actnode = dis->gNode((*nodeids)[i]);
      if (!actnode) dserror("Cannot find global node %d",(*nodeids)[i]);
      vector<int> dofs = dis->Dof(actnode);
      const unsigned numdf = dofs.size();

      for (unsigned j=0; j<numdf; ++j)
      {
        const int gid = dofs[j];

        const int lid = disn_->Map().LID(gid);
        if (lid<0) dserror("Global id %d not on this proc in system vector",gid);

        if ((*dirichtoggle_)[lid] != 1.0)  // be careful not to count dofs more
                                           // than once since nodes belong to
                                           // several surfaces simultaneously
          ++np;

        (*dirichtoggle_)[lid] = 1.0;
      }
    }
  }

  np_ = np;
}

void MicroStruGenAlpha::EvaluateMicroBC(const Epetra_SerialDenseMatrix* defgrd)
{
  RefCountPtr<DRT::Discretization> dis =
    DRT::Problem::Instance(1)->Dis(genprob.numsf,0);

  vector<DRT::Condition*> conds;
  dis->GetCondition("MicroBoundary", conds);
  for (unsigned i=0; i<conds.size(); ++i)
  {
    const vector<int>* nodeids = conds[i]->Get<vector<int> >("Node Ids");
    if (!nodeids) dserror("MicroBoundary condition does not have nodal cloud");
    const int nnode = (*nodeids).size();

    for (int i=0; i<nnode; ++i)
    {
      // do only nodes in my row map
      if (!dis->NodeRowMap()->MyGID((*nodeids)[i])) continue;
      DRT::Node* actnode = dis->gNode((*nodeids)[i]);
      if (!actnode) dserror("Cannot find global node %d",(*nodeids)[i]);

      // nodal coordinates
      const double* x = actnode->X();

      // boundary displacements are prescribed via the macroscopic
      // deformation gradient
      double dism_prescribed[3];
      for (int i=0; i<3;i++)
      {
        double dis = 0.;

        for (int j=0;j<3;j++)
        {
          dis += (*defgrd)(i, j) * x[j];
        }

        dism_prescribed[i] = dis - x[i];
      }

      vector<int> dofs = dis->Dof(actnode);

      for (int k=0; k<3; ++k)
      {
        const int gid = dofs[k];

        const int lid = dism_->Map().LID(gid);
        if (lid<0) dserror("Global id %d not on this proc in system vector",gid);
        (*dism_)[lid] = dism_prescribed[k];
      }
    }
  }

}

void MicroStruGenAlpha::SetOldState(RefCountPtr<Epetra_Vector> disp,
                                    RefCountPtr<Epetra_Vector> vel,
                                    RefCountPtr<Epetra_Vector> acc,
                                    RefCountPtr<Epetra_Vector> disi)
{
  dis_ = disp;
  vel_ = vel;
  acc_ = acc;
  disi_ = disi;
  fext_->PutScalar(0.);     // we do not have any external loads on
                            // the microscale, so assign all components
                            // to zero
}

void MicroStruGenAlpha::SetTime(double timen, int istep)
{
  params_->set<double>("total time", timen);
  params_->set<int>   ("step", istep);
}

RefCountPtr<Epetra_Vector> MicroStruGenAlpha::ReturnNewDisp() { return rcp(new Epetra_Vector(*dis_)); }

RefCountPtr<Epetra_Vector> MicroStruGenAlpha::ReturnNewVel() { return rcp(new Epetra_Vector(*vel_)); }

RefCountPtr<Epetra_Vector> MicroStruGenAlpha::ReturnNewAcc() { return rcp(new Epetra_Vector(*acc_)); }

RefCountPtr<Epetra_Vector> MicroStruGenAlpha::ReturnNewResDisp() { return rcp(new Epetra_Vector(*disi_)); }

void MicroStruGenAlpha::ClearState()
{
  dis_ = null;
  vel_ = null;
  acc_ = null;
  disi_ = null;
}

void MicroStruGenAlpha::SetUpHomogenization()
{
  int indp = 0;
  int indf = 0;

  RefCountPtr<DRT::Discretization> dis =
    DRT::Problem::Instance(1)->Dis(genprob.numsf,0);

  int toggle_len = dis->NodeRowMap()->NumMyElements();
  toggle_len*=3; // three dofs per node

  ndof_ = toggle_len;

  std::vector <int>   pdof(np_);
  std::vector <int>   fdof(ndof_-np_);        // changed this, previously this
                                              // has been just fdof(np_),
                                              // but how should that
                                              // work for ndof_-np_>np_???

  for (int it=0; it<toggle_len; ++it)
  {
    if ((*dirichtoggle_)[it] == 1.0)
    {
      pdof[indp]=it;
      ++indp;
    }
    else
    {
      fdof[indf]=it;
      ++indf;
    }
  }

  // create map based on the determined dofs of prescribed and free nodes
  pdof_ = rcp(new Epetra_Map(-1, np_, &pdof[0], 0, dis->Comm()));
  fdof_ = rcp(new Epetra_Map(-1, ndof_-np_, &fdof[0], 0, dis->Comm()));

  // create an exporter
  export_ = rcp(new Epetra_Export(*(dis->DofRowMap()), *pdof_));

  // create vector containing material coordinates of prescribed nodes
  Epetra_Vector Xp_temp(*pdof_);

  vector<DRT::Condition*> conds;
  dis->GetCondition("MicroBoundary", conds);
  for (unsigned i=0; i<conds.size(); ++i)
  {
    const vector<int>* nodeids = conds[i]->Get<vector<int> >("Node Ids");
    if (!nodeids) dserror("MicroBoundary condition does not have nodal cloud");
    const int nnode = (*nodeids).size();

    for (int i=0; i<nnode; ++i)
    {
      // do only nodes in my row map
      if (!dis->NodeRowMap()->MyGID((*nodeids)[i])) continue;
      DRT::Node* actnode = dis->gNode((*nodeids)[i]);
      if (!actnode) dserror("Cannot find global node %d",(*nodeids)[i]);

      // nodal coordinates
      const double* x = actnode->X();

      vector<int> dofs = dis->Dof(actnode);

      for (int k=0; k<3; ++k)
      {
        const int gid = dofs[k];

        const int lid = disn_->Map().LID(gid);
        if (lid<0) dserror("Global id %d not on this proc in system vector",gid);

        for (int l=0;l<np_;++l)
        {
          if (pdof[l]==gid)
            Xp_temp[l]=x[k];
        }
      }
    }
  }

  Xp_ = LINALG::CreateVector(*pdof_,true);
  *Xp_ = Xp_temp;
}


// after having finished all the testing, remove cmat from the input
// parameters, since no constitutive tensor is calculated here.

void MicroStruGenAlpha::Homogenization(Epetra_SerialDenseVector* stress,
                                       Epetra_SerialDenseMatrix* cmat,
                                       double *density,
                                       const Epetra_SerialDenseMatrix* defgrd,
                                       const string action)
{
  // determine macroscopic parameters via averaging (homogenization) of
  // microscopic features
  // this was implemented against the background of serial usage
  // -> if a parallel version of microscale simulations is EVER wanted,
  // carefully check if/what/where things have to change

  // create the parameters for the discretization
  ParameterList p;
  // action for elements
  p.set("action","calc_homog_stressdens");
  // choose what to assemble
  p.set("assemble matrix 1",false);
  p.set("assemble matrix 2",false);
  p.set("assemble vector 1",false);
  p.set("assemble vector 2",false);
  p.set("assemble vector 3",false);
  // set stresses and densities to zero
  p.set("homogdens", 0.0);

  p.set("homogP11", 0.0);
  p.set("homogP12", 0.0);
  p.set("homogP13", 0.0);
  p.set("homogP21", 0.0);
  p.set("homogP22", 0.0);
  p.set("homogP23", 0.0);
  p.set("homogP31", 0.0);
  p.set("homogP32", 0.0);
  p.set("homogP33", 0.0);

  // set vector values needed by elements
  discret_->ClearState();
  discret_->SetState("residual displacement",zeros_);

  // we have to distinguish here whether we are in a homogenization
  // procedure during the nonlinear solution or in the post-processing
  // (macroscopic stress calculation) phase -> in the former case, the
  // displacement has to be chosen at the generalized mid-point, in
  // the latter one we need to evaluate everything at the end of the
  // time step
  if (action == "stress_calc")
    discret_->SetState("displacement",dis_);
  else
    discret_->SetState("displacement",dism_);
  discret_->Evaluate(p,null,null,null,null,null);
  discret_->ClearState();

  *density = 1/V0_*p.get<double>("homogdens", 0.0);
  if (*density == 0.0)
    dserror("Density determined from homogenization procedure equals zero!");

  Epetra_SerialDenseMatrix P(3, 3);
  P(0, 0) = 1/V0_*p.get<double>("homogP11", 0.0);
  P(0, 1) = 1/V0_*p.get<double>("homogP12", 0.0);
  P(0, 2) = 1/V0_*p.get<double>("homogP13", 0.0);
  P(1, 0) = 1/V0_*p.get<double>("homogP21", 0.0);
  P(1, 1) = 1/V0_*p.get<double>("homogP22", 0.0);
  P(1, 2) = 1/V0_*p.get<double>("homogP23", 0.0);
  P(2, 0) = 1/V0_*p.get<double>("homogP31", 0.0);
  P(2, 1) = 1/V0_*p.get<double>("homogP32", 0.0);
  P(2, 2) = 1/V0_*p.get<double>("homogP33", 0.0);

  // determine inverse of deformation gradient

  Epetra_SerialDenseMatrix F_inv(3,3);

  double detF= (*defgrd)(0,0) * (*defgrd)(1,1) * (*defgrd)(2,2)
             + (*defgrd)(0,1) * (*defgrd)(1,2) * (*defgrd)(2,0)
             + (*defgrd)(0,2) * (*defgrd)(1,0) * (*defgrd)(2,1)
             - (*defgrd)(0,0) * (*defgrd)(1,2) * (*defgrd)(2,1)
             - (*defgrd)(0,1) * (*defgrd)(1,0) * (*defgrd)(2,2)
             - (*defgrd)(0,2) * (*defgrd)(1,1) * (*defgrd)(2,0);

  F_inv(0,0) = ((*defgrd)(1,1)*(*defgrd)(2,2)-(*defgrd)(1,2)*(*defgrd)(2,1))/detF;
  F_inv(0,1) = ((*defgrd)(0,2)*(*defgrd)(2,1)-(*defgrd)(2,2)*(*defgrd)(0,1))/detF;
  F_inv(0,2) = ((*defgrd)(0,1)*(*defgrd)(1,2)-(*defgrd)(1,1)*(*defgrd)(0,2))/detF;
  F_inv(1,0) = ((*defgrd)(1,2)*(*defgrd)(2,0)-(*defgrd)(2,2)*(*defgrd)(1,0))/detF;
  F_inv(1,1) = ((*defgrd)(0,0)*(*defgrd)(2,2)-(*defgrd)(2,0)*(*defgrd)(0,2))/detF;
  F_inv(1,2) = ((*defgrd)(0,2)*(*defgrd)(1,0)-(*defgrd)(1,2)*(*defgrd)(0,0))/detF;
  F_inv(2,0) = ((*defgrd)(1,0)*(*defgrd)(2,1)-(*defgrd)(2,0)*(*defgrd)(1,1))/detF;
  F_inv(2,1) = ((*defgrd)(0,1)*(*defgrd)(2,0)-(*defgrd)(2,1)*(*defgrd)(0,0))/detF;
  F_inv(2,2) = ((*defgrd)(0,0)*(*defgrd)(1,1)-(*defgrd)(1,0)*(*defgrd)(0,1))/detF;

  for (int i=0; i<3; ++i)
  {
    (*stress)[0] += F_inv(0, i)*P(i,0);                     // S11
    (*stress)[1] += F_inv(1, i)*P(i,1);                     // S22
    (*stress)[2] += F_inv(2, i)*P(i,2);                     // S33
    (*stress)[3] += F_inv(0, i)*P(i,1);                     // S12
    (*stress)[4] += F_inv(1, i)*P(i,2);                     // S23
    (*stress)[5] += F_inv(0, i)*P(i,2);                     // S13
  }

  // for testing reasons only!!!!!!!!!! begin testing region
  const double Emod  = 100.0;
  const double nu  = 0.;

  double mfac = Emod/((1.0+nu)*(1.0-2.0*nu));  /* factor */
  /* write non-zero components */
  (*cmat)(0,0) = mfac*(1.0-nu);
  (*cmat)(0,1) = mfac*nu;
  (*cmat)(0,2) = mfac*nu;
  (*cmat)(1,0) = mfac*nu;
  (*cmat)(1,1) = mfac*(1.0-nu);
  (*cmat)(1,2) = mfac*nu;
  (*cmat)(2,0) = mfac*nu;
  (*cmat)(2,1) = mfac*nu;
  (*cmat)(2,2) = mfac*(1.0-nu);
  /* ~~~ */
  (*cmat)(3,3) = mfac*0.5*(1.0-2.0*nu);
  (*cmat)(4,4) = mfac*0.5*(1.0-2.0*nu);
  (*cmat)(5,5) = mfac*0.5*(1.0-2.0*nu);

  // Right Cauchy-Green tensor = F^T * F
  Epetra_SerialDenseMatrix cauchygreen(3,3);
  cauchygreen.Multiply('T','N',1.0,*defgrd,*defgrd,1.0);

  // Green-Lagrange strains matrix E = 0.5 * (Cauchygreen - Identity)
  // GL strain vector glstrain={E11,E22,E33,2*E12,2*E23,2*E31}
  Epetra_SerialDenseVector glstrain(6);
  glstrain(0) = 0.5 * (cauchygreen(0,0) - 1.0);
  glstrain(1) = 0.5 * (cauchygreen(1,1) - 1.0);
  glstrain(2) = 0.5 * (cauchygreen(2,2) - 1.0);
  glstrain(3) = cauchygreen(0,1);
  glstrain(4) = cauchygreen(1,2);
  glstrain(5) = cauchygreen(2,0);

  Epetra_SerialDenseVector ref_stress(6);
  (*cmat).Multiply('N',glstrain,ref_stress);
  /// end testing region!!!!!!!!!
}

void MicroStruGenAlpha::StaticHomogenization(Epetra_SerialDenseVector* stress,
                                             Epetra_SerialDenseMatrix* cmat,
                                             double *density,
                                             const Epetra_SerialDenseMatrix* defgrd)
{
  // determine macroscopic parameters via averaging (homogenization) of
  // microscopic features accoring to Kouznetsova, Miehe etc.
  // this was implemented against the background of serial usage
  // -> if a parallel version of microscale simulations is EVER wanted,
  // carefully check if/what/where things have to change

  // split microscale stiffness and residual forces into parts
  // corresponding to prescribed and free dofs -> see thesis
  // of Kouznetsova (Computational homogenization for the multi-scale
  // analysis of multi-phase materials, Eindhoven, 2002)

  // split residual forces -> we want to extract fp

  RefCountPtr<DRT::Discretization> dis =
    DRT::Problem::Instance(1)->Dis(genprob.numsf,0);

  Epetra_Vector fp(*pdof_);

  int err = fp.Export(*fresm_dirich_, *export_, Insert);
  if (err)
    dserror("Exporting external forces of prescribed dofs using exporter returned err=%d",err);

  // Now we have all forces in the material description acting on the
  // boundary nodes together in one vector
  // -> for calculating the stresses, we need to choose the
  // right three components corresponding to a single node and
  // take the inner product with the material coordinates of this
  // node. The sum over all boundary nodes delivers the first
  // Piola-Kirchhoff macroscopic stress which has to be transformed
  // into the second Piola-Kirchhoff counterpart.
  // All these complicated conversions are necessary since only for
  // the energy-conjugated pair of first Piola-Kirchhoff and
  // deformation gradient the averaging integrals can be transformed
  // into integrals over the boundaries only in case of negligible
  // inertial forces (which simplifies matters significantly) whereas
  // the calling macroscopic material routine demands a second
  // Piola-Kirchhoff stress tensor.

  // IMPORTANT: the RVE has to be centered around (0,0,0), otherwise
  // this approach does not work. This was also confirmed by
  // Kouznetsova in a discussion during USNCCM 9.

  Epetra_SerialDenseMatrix P(3,3);

  for (int i=0; i<3; ++i)
  {
    for (int j=0; j<3; ++j)
    {
      for (int n=0; n<np_/3; ++n)
      {
        P(i,j) += fp[n*3+i]*(*Xp_)[n*3+j];
      }
      P(i,j) /= V0_;
    }
  }

  // determine inverse of deformation gradient

  Epetra_SerialDenseMatrix F_inv(3,3);

  double detF= (*defgrd)(0,0) * (*defgrd)(1,1) * (*defgrd)(2,2)
             + (*defgrd)(0,1) * (*defgrd)(1,2) * (*defgrd)(2,0)
             + (*defgrd)(0,2) * (*defgrd)(1,0) * (*defgrd)(2,1)
             - (*defgrd)(0,0) * (*defgrd)(1,2) * (*defgrd)(2,1)
             - (*defgrd)(0,1) * (*defgrd)(1,0) * (*defgrd)(2,2)
             - (*defgrd)(0,2) * (*defgrd)(1,1) * (*defgrd)(2,0);

  F_inv(0,0) = ((*defgrd)(1,1)*(*defgrd)(2,2)-(*defgrd)(1,2)*(*defgrd)(2,1))/detF;
  F_inv(0,1) = ((*defgrd)(0,2)*(*defgrd)(2,1)-(*defgrd)(2,2)*(*defgrd)(0,1))/detF;
  F_inv(0,2) = ((*defgrd)(0,1)*(*defgrd)(1,2)-(*defgrd)(1,1)*(*defgrd)(0,2))/detF;
  F_inv(1,0) = ((*defgrd)(1,2)*(*defgrd)(2,0)-(*defgrd)(2,2)*(*defgrd)(1,0))/detF;
  F_inv(1,1) = ((*defgrd)(0,0)*(*defgrd)(2,2)-(*defgrd)(2,0)*(*defgrd)(0,2))/detF;
  F_inv(1,2) = ((*defgrd)(0,2)*(*defgrd)(1,0)-(*defgrd)(1,2)*(*defgrd)(0,0))/detF;
  F_inv(2,0) = ((*defgrd)(1,0)*(*defgrd)(2,1)-(*defgrd)(2,0)*(*defgrd)(1,1))/detF;
  F_inv(2,1) = ((*defgrd)(0,1)*(*defgrd)(2,0)-(*defgrd)(2,1)*(*defgrd)(0,0))/detF;
  F_inv(2,2) = ((*defgrd)(0,0)*(*defgrd)(1,1)-(*defgrd)(1,0)*(*defgrd)(0,1))/detF;

  // convert to second Piola-Kirchhoff stresses and store them in
  // vector format
  // assembly of stresses (cf Solid3 Hex8): S11,S22,S33,S12,S23,S13

  Epetra_SerialDenseVector S(6);

  for (int i=0; i<3; ++i)
  {
    S[0] += F_inv(0, i)*P(i,0);                     // S11
    S[1] += F_inv(1, i)*P(i,1);                     // S22
    S[2] += F_inv(2, i)*P(i,2);                     // S33
    S[3] += F_inv(0, i)*P(i,1);                     // S12
    S[4] += F_inv(1, i)*P(i,2);                     // S23
    S[5] += F_inv(0, i)*P(i,2);                     // S13
  }

  for (int i=0; i<6; ++i)
  {
    (*stress)[i]=S[i];
  }

  // split effective dynamic stiffness -> we want Kpp, Kpf, Kfp and Kff
  // Kpp and Kff are sparse matrices, whereas Kpf and Kfp are MultiVectors
  // (we need that for the solution of Kpf*inv(Kff)*Kfp)

  int *IndexOffset;
  int *Indices;
  double *Values;

  Epetra_MultiVector Kpp(*pdof_, np_);
  Epetra_CrsMatrix   Kff(Copy, *fdof_, 81);
  Epetra_MultiVector Kpf(*pdof_,  ndof_-np_);
  Epetra_MultiVector Kfp(*fdof_, np_);
  Epetra_MultiVector x(*fdof_, np_);

  stiff_->FillComplete();                   // needed for ExtractCrsDataPointers
  stiff_->OptimizeStorage();                // needed for ExtractCrsDataPointers

  err = stiff_->ExtractCrsDataPointers(IndexOffset, Indices, Values);
  if (err)
    dserror("Extraction of internal data pointers associated with Crs matrix failed");

  const Epetra_Map* dofrowmap = dis->DofRowMap();

  for (int row=0; row<dofrowmap->NumMyElements(); ++row)
  {
    int rowgid = dofrowmap->GID(row);

    if (pdof_->MyGID(rowgid))              // this means we are dealing with
                                           // either Kpp or Kpf
    {
      for (int col=IndexOffset[row]; col<IndexOffset[row+1]; ++col)
      {
        int colgid = Indices[col];

        if (pdof_->MyGID(colgid))          // -> Kpp
        {
          Kpp.ReplaceMyValue(pdof_->LID(rowgid), pdof_->LID(colgid), Values[col]);
        }
        // we don't need to compute Kpf here since in the SYMMETRIC case
        // Kpf is the transpose pf Kfp
        //else                               // -> Kpf
        //{
        //  Kpf.ReplaceMyValue(rowgid, colgid, Values[colgid]);
        //}
      }
    }

    else if (fdof_->MyGID(rowgid))         // this means we are dealing with
                                           // either Kff or Kfp
    {
      for (int col=IndexOffset[row]; col<IndexOffset[row+1]; ++col)
      {
        int colgid = Indices[col];

        if (fdof_->MyGID(colgid))          // -> Kff
        {
          err = Kff.InsertGlobalValues(rowgid, 1, &(Values[col]), &colgid);
          if (err)
            dserror("Insertion of values into Kff failed");
        }
        else
        {
          Kfp.ReplaceMyValue(fdof_->LID(rowgid), pdof_->LID(colgid), Values[col]);
        }
      }
    }
    else
      dserror("GID neither in DofMap for prescribed nor for free dofs");
  }

  // define an Epetra_LinearProblem object for solving Kff*x=Kfp (thus
  // circumventing the explicit inversion of Kff for the static condensation)

  err = Kff.FillComplete();
  if (err!=0)
    dserror("FillComplete failed with err=%d", err);

  Epetra_LinearProblem linprob(&Kff, &x, &Kfp);
  int error=linprob.CheckInput();
  if (error)
    dserror("Input for linear problem inconsistent");

  // Solve for x

  Amesos_Klu solver(linprob);
  err = solver.Solve();
  if (err!=0)
    dserror("Solve");

  // now static condensation of free (not prescribed) dofs can be done
  // KM = Kpp+Kpf*x = Kpp+Ktemp
  // result will be saved in Kpp to avoid copying

  Epetra_MultiVector Ktemp(*pdof_, np_);
  err = Ktemp.Multiply('T', 'N', 1., Kfp, x, 0.);
  if (err!=0)
    dserror("Multiply");

  Kpp.Update(-1., Ktemp, 1.);    // Kpp now holds KM=Kpp-Kpf*inv(Kff)*Kfp

  // Now we have to calculate 1/V0 Xp_ Kpp Xp_ (inner product) to obtain
  // the contitutive tensor relating the first Piola Kirchhoff stress
  // tensor to the deformation gradient -> keep in mind the underlying
  // assumption that inertial forces are negligible (actually this
  // only works strictly in the static case, so we have to think about
  // dynamic homogenization - what we are actually interested in - later on)
  // With corresponding push forward operations the constitutive
  // tensor relating second Piola Kirchhoff stresses to Green Lagrange
  // strains has to be determined (this is what Solid3 Hex8 wants to
  // be returned by the material routine)

  MicroStruGenAlpha::calc_cmat(Kpp, F_inv, S, cmat, defgrd);

  // after having all homogenization stuff done, we now really don't need stiff_ anymore

  stiff_ = null;

  // the macroscopic density has to be averaged over the entire
  // microstructural reference volume

  // create the parameters for the discretization
  ParameterList p;
  // action for elements
  p.set("action","calc_homog_stressdens");
  // choose what to assemble
  p.set("assemble matrix 1",false);
  p.set("assemble matrix 2",false);
  p.set("assemble vector 1",false);
  p.set("assemble vector 2",false);
  p.set("assemble vector 3",false);
  // set density to zero
  p.set("homogdens", 0.0);
  // set flag that only density has to be calculated
  p.set("onlydens", true);

  // set vector values needed by elements
  discret_->ClearState();
  discret_->SetState("residual displacement",zeros_);

  discret_->SetState("displacement",dism_);

  discret_->Evaluate(p,null,null,null,null,null);
  discret_->ClearState();

  *density = 1/V0_*p.get<double>("homogdens", 0.0);
  if (*density == 0.0)
    dserror("Density determined from homogenization procedure equals zero!");
}


#endif
