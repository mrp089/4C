/*----------------------------------------------------------------------*/
/*!
\file strutimint_ost.cpp
\brief Structural time integration with one-step-theta

<pre>
Maintainer: Burkhard Bornemann
            bornemann@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15237
</pre>
*/

/*----------------------------------------------------------------------*/
/* macros */
#ifdef CCADISCRET

/*----------------------------------------------------------------------*/
/* headers */
#include "strutimint_ost.H"

/*======================================================================*/
/* constructor */
StruTimIntOneStepTheta::StruTimIntOneStepTheta
(
  const Teuchos::ParameterList& ioparams,
  const Teuchos::ParameterList& sdynparams,
  const Teuchos::ParameterList& xparams,
  const Teuchos::ParameterList& onestepthetaparams,
  DRT::Discretization& actis,
  LINALG::Solver& solver,
  IO::DiscretizationWriter& output
)
: StruTimIntImpl
  (
    ioparams,
    sdynparams,
    xparams,
    actis,
    solver,
    output
  ),
  theta_(onestepthetaparams.get<double>("THETA")),
  dist_(Teuchos::null),
  velt_(Teuchos::null),
  acct_(Teuchos::null),
  fint_(Teuchos::null),
  fintt_(Teuchos::null),
  fintn_(Teuchos::null),
  fext_(Teuchos::null),
  fextt_(Teuchos::null),
  fextn_(Teuchos::null),
  finertt_(Teuchos::null),
  fvisct_(Teuchos::null),
  frobin_(Teuchos::null)
{
  // info to user
  if (myrank_ == 0)
  {
    std::cout << "with one-step-theta (theta=" 
              << theta_ << ")" << std::endl << std::endl;
  }

  // create state vectors
  
  // mid-displacements
  dist_ = LINALG::CreateVector(*dofrowmap_, true);
  // mid-velocities
  velt_ = LINALG::CreateVector(*dofrowmap_, true);
  // mid-accelerations
  acct_ = LINALG::CreateVector(*dofrowmap_, true);

  // create force vectors

  // internal forces
  // internal force vector F_{int;n} at last time
  fint_ = LINALG::CreateVector(*dofrowmap_, true);
  // internal force vector F_{int;n+1} at new time
  fintn_ = LINALG::CreateVector(*dofrowmap_, true);

  // external force vector F_ext at last times
  fext_ = LINALG::CreateVector(*dofrowmap_, true);
  // external mid-force vector F_{ext;n+1-alpha_f}
  fextt_ = LINALG::CreateVector(*dofrowmap_, true);
  // external force vector F_{n+1} at new time
  fextn_ = LINALG::CreateVector(*dofrowmap_, true);

  // set initial external force vector
  ApplyForceExternal(time_, dis_, fext_);

  // inertial mid-point force vector F_inert
  finertt_ = LINALG::CreateVector(*dofrowmap_, true);
  // viscous mid-point force vector F_visc
  fvisct_ = LINALG::CreateVector(*dofrowmap_, true);

  // external pseudo force due to RobinBC
  frobin_ = LINALG::CreateVector(*dofrowmap_, true);

  // have a nice day
  return;
}

/*----------------------------------------------------------------------*/
/* Consistent predictor with constant displacements
 * and consistent velocities and displacements */
void StruTimIntOneStepTheta::PredictConstDisConsistVelAcc()
{
  // constant predictor : displacement in domain
  disn_->Update(1.0, *dis_, 0.0);
  /*
  // apply Dirichlet condition onto this
  // This is not absolutely necessary, as the DBCs are set
  // by StruTimIntImpl::Predict. 
  // However, here they are
  // applied to achieve a more better guess for the remaining
  // DOFs.
  ApplyDirichletBC(timen_, disn_, Teuchos::null, Teuchos::null);
  */

  // PULL EMERGENCY BREAK
  dserror("Code this before use");

  // consistent velocities
  veln_->Update(1.0, *disn_, -1.0, *dis_, 0.0);
  veln_->Update((theta_-theta_)/theta_, *vel_,
                (2.*theta_-theta_)*dt_/(2.*theta_), *acc_,
                theta_/(theta_*dt_));

  // consistent accelerations
  accn_->Update(1.0, *disn_, -1.0, *dis_, 0.0);
  accn_->Update(-1./(theta_*dt_), *vel_,
                (2.*theta_-1.)/(2.*theta_), *acc_,
                1./(theta_*dt_*dt_));
  
  // watch out
  return;
}

/*----------------------------------------------------------------------*/
/* evaluate residual force and its stiffness, ie derivative
 * with respect to end-point displacements \f$D_{n+1}\f$ */
void StruTimIntOneStepTheta::EvaluateForceStiffResidual()
{
  // theta-interpolate state vectors
  EvaluateMidState();

  // build new external forces
  fextn_->PutScalar(0.0);
  ApplyForceExternal(timen_, disn_, fextn_);
  // external mid-forces F_{ext;n+1-alpha_f} (fextm)
  //    F_{ext;n+1-alpha_f} := (1.-alphaf) * F_{ext;n+1}
  //                         + alpha_f * F_{ext;n}
  fextt_->Update(theta_, *fextn_, 1.0-theta_, *fext_,0.0);

  // initialise internal forces
  fintn_->PutScalar(0.0);

  // initialise stiffness matrix to zero
  stiff_->Zero();

  // ordinary internal force and stiffness
  ApplyForceStiffInternal(timen_, disn_, disi_,  fintn_, stiff_);

  // surface stress force
  ApplyForceStiffSurfstress(disn_, fintn_, stiff_);
  
  // potential forces
  ApplyForceStiffPotential(disn_, fintn_, stiff_);

  // close stiffness matrix
  stiff_->Complete();

  // inertial forces #finertt_
  mass_->Multiply(false, *acct_, *finertt_);

  // viscous forces due Rayleigh damping
  if (damping_)
  {
    damp_->Multiply(false, *velt_, *fvisct_);
  }


  // build negative residual  Res = -( M . A_{n+theta}
  //                                   + C . V_{n+theta}
  //                                   + F_{int;n+theta}
  //                                   - F_{ext;n+theta} )
  fres_->Update(1.0, *fextt_, 0.0);
  fres_->Update(-theta_, *fintn_, -(1.0-theta_), *fint_, 1.0);
  if (damping_)
  {
    fres_->Update(-1.0, *fvisct_, 1.0);
  }
  fres_->Update(-1.0, *finertt_, 1.0);

  // build tangent matrix : effective dynamic stiffness matrix
  //    K_{Teffdyn} = 1/(theta*dt^2) M
  //                + 1/dt C     
  //                + theta K_{T}
  stiff_->Add(*mass_, false, 1.0/(theta_*dt_*dt_), theta_);
  if (damping_)
  {
    stiff_->Add(*damp_, false, 1.0/dt_, 1.0);
  }
  stiff_->Complete();  // close stiffness matrix

  // hallelujah
  return;
}

/*----------------------------------------------------------------------*/
/* evaluate theta-state vectors by averaging end-point vectors */
void StruTimIntOneStepTheta::EvaluateMidState()
{
  // mid-displacements D_{n+1-alpha_f} (dism)
  //    D_{n+1-alpha_f} := (1.-alphaf) * D_{n+1} + alpha_f * D_{n}
  dist_->Update(theta_, *disn_, 1.0-theta_, *dis_, 0.0);
  
  // mid-velocities V_{n+1-alpha_f} (velm)
  //    V_{n+1-alpha_f} := (1.-alphaf) * V_{n+1} + alpha_f * V_{n}
  velt_->Update(theta_, *veln_, 1.0-theta_, *vel_, 0.0);
  
  // mid-accelerations A_{n+1-alpha_m} (accm)
  //    A_{n+1-alpha_m} := (1.-alpha_m) * A_{n+1} + alpha_m * A_{n}
  acct_->Update(theta_, *accn_, 1.0-theta_, *acc_, 0.0);

  // jump
  return;
}

/*----------------------------------------------------------------------*/
/* calculate characteristic/reference norms for displacements
 * originally by lw */
double StruTimIntOneStepTheta::CalcRefNormDisplacement()
{
  // The reference norms are used to scale the calculated iterative
  // displacement norm and/or the residual force norm. For this
  // purpose we only need the right order of magnitude, so we don't
  // mind evaluating the corresponding norms at possibly different
  // points within the timestep (end point, generalized midpoint).

  double charnormdis = 0.0;
  dis_->Norm2(&charnormdis);

  // rise your hat
  return charnormdis;
}

/*----------------------------------------------------------------------*/
/* calculate characteristic/reference norms for forces
 * originally by lw */
double StruTimIntOneStepTheta::CalcRefNormForce()
{
  // The reference norms are used to scale the calculated iterative
  // displacement norm and/or the residual force norm. For this
  // purpose we only need the right order of magnitude, so we don't
  // mind evaluating the corresponding norms at possibly different
  // points within the timestep (end point, generalized midpoint).

  // norm of the internal forces
  double fintnorm = 0.0;
  fintn_->Norm2(&fintnorm);

  // norm of the external forces
  double fextnorm = 0.0;
  fextt_->Norm2(&fextnorm);

  // norm of the inertial forces
  double finertnorm = 0.0;
  finertt_->Norm2(&finertnorm);

  // norm of viscous forces
  double fviscnorm = 0.0;
  if (damping_)
  {
    fvisct_->Norm2(&fviscnorm);
  }

  // return char norm
  return max(fviscnorm, max(finertnorm, max(fintnorm, fextnorm)));
}

/*----------------------------------------------------------------------*/
/* iterative update of state */
void StruTimIntOneStepTheta::UpdateIter()
{
  // new end-point displacements
  // D_{n+1}^{<k+1>} := D_{n+1}^{<k>} + IncD_{n+1}^{<k>}
  disn_->Update(1.0, *disi_, 1.0);

  // new end-point velocities
  //veln_->Update(1.0/dt_, *disi_, 1.0);
  veln_->Update(1.0/(theta_*dt_), *disn_,
                -1.0/(theta_*dt_), *dis_, 
                0.0);
  veln_->Update((theta_-1.0)/theta_, *vel_, 1.0);

  // new end-point accelerations
  //accn_->Update(1.0/(dt_*dt_*theta_), *disi_, 1.0);
  accn_->Update(1.0/(theta_*theta_*dt_*dt_), *disn_, 
                -1.0/(theta_*theta_*dt_*dt_), *dis_, 
                0.0);
  accn_->Update(-1.0/(theta_*theta_*dt_), *vel_, 
                (theta_-1.0)/theta_, *acc_,
                1.0);

  // bye
  return;
}

/*----------------------------------------------------------------------*/
/* update after time step */
void StruTimIntOneStepTheta::UpdateStep()
{
  // update state
  // new displacements at t_{n+1} -> t_n
  //    D_{n} := D_{n+1}
  dis_->Update(1.0, *disn_, 0.0);
  // new velocities at t_{n+1} -> t_n
  //    V_{n} := V_{n+1}
  vel_->Update(1.0, *veln_, 0.0);
  // new accelerations at t_{n+1} -> t_n
  //    A_{n} := A_{n+1}
  acc_->Update(1.0, *accn_, 0.0);

  // update new external force
  //    F_{ext;n} := F_{ext;n+1}
  fext_->Update(1.0, *fextn_, 0.0);

  // update new internal force
  //    F_{int;n} := F_{int;n+1}
  fint_->Update(1.0, *fintn_, 0.0);

  // update anything that needs to be updated at the element level
  {
    // create the parameters for the discretization
    ParameterList p;
    // other parameters that might be needed by the elements
    p.set("total time", timen_);
    p.set("delta time", dt_);
    p.set("alpha f", theta_);
    // action for elements
    p.set("action", "calc_struct_update_istep");    
    // go to elements
    discret_.Evaluate(p, null, null, null, null, null);
  }

  // update surface stress
  UpdateStepSurfstress();

  // update potential forces
  UpdateStepPotential();

  // look out
  return;
}


/*----------------------------------------------------------------------*/
#endif  // #ifdef CCADISCRET
