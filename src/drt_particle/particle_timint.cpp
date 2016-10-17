/*----------------------------------------------------------------------*/
/*!
\file particle_timint.cpp

\brief Time integration for particle dynamics

\level 1

\maintainer Georg Hammerl
*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* headers */
#include "particle_timint.H"
#include "particle_algorithm.H"
#include "particle_contact.H"
#include "particle_resulttest.H"
#include "../drt_io/io.H"
#include "../drt_io/io_control.H"
#include "../drt_io/io_pstream.H"
#include "../drt_lib/drt_discret.H"
#include "../linalg/linalg_utils.H"
#include "../drt_lib/drt_globalproblem.H"

#include "../drt_mat/particle_mat.H"
#include "../drt_mat/extparticle_mat.H"

#include <Teuchos_TimeMonitor.hpp>

/*----------------------------------------------------------------------*/
/* print particle time logo */
void PARTICLE::TimInt::Logo()
{
 IO::cout << "Welcome to Particle Time Integration " <<IO::endl;
 IO::cout << "    ---                      ---     " <<IO::endl;
 IO::cout << "  /     \\                  /     \\   " <<IO::endl;
 IO::cout << "  |     |   ---->  <----   |     |   " <<IO::endl;
 IO::cout << "  \\     /                  \\     /   " <<IO::endl;
 IO::cout << "    ---                      ---     " <<IO::endl;
 IO::cout <<IO::endl;
}

/*----------------------------------------------------------------------*/
/* constructor */
PARTICLE::TimInt::TimInt
(
  const Teuchos::ParameterList& ioparams,
  const Teuchos::ParameterList& particledynparams,
  const Teuchos::ParameterList& xparams,
  Teuchos::RCP<DRT::Discretization> actdis,
  Teuchos::RCP<IO::DiscretizationWriter> output
)
: discret_(actdis),
  myrank_(actdis->Comm().MyPID()),
  dbcmaps_(Teuchos::null),
  output_(output),
  printlogo_(true),
  printscreen_(ioparams.get<int>("STDOUTEVRY")),
  errfile_(xparams.get<FILE*>("err file")),
  printerrfile_(errfile_),
  writerestartevery_(particledynparams.get<int>("RESTARTEVRY")),
  writestate_((bool) DRT::INPUT::IntegralValue<int>(ioparams,"STRUCT_DISP")),
  writevelacc_((bool) DRT::INPUT::IntegralValue<int>(ioparams,"STRUCT_VEL_ACC")),
  writeresultsevery_(particledynparams.get<int>("RESULTSEVRY")),
  writeenergyevery_(particledynparams.get<int>("RESEVRYERGY")),
  energyfile_(Teuchos::null),
  writeorientation_(false),
  time_(Teuchos::null),
  timen_(0.0),
  dt_(Teuchos::null),
  timemax_(particledynparams.get<double>("MAXTIME")),
  stepmax_(particledynparams.get<int>("NUMSTEP")),
  step_(0),
  stepn_(0),
  restart_(0),
  dis_(Teuchos::null),
  vel_(Teuchos::null),
  acc_(Teuchos::null),
  angVel_(Teuchos::null),
  angAcc_(Teuchos::null),
  radius_(Teuchos::null),
  density_(Teuchos::null),
  densityDot_(Teuchos::null),
  specEnthalpy_(Teuchos::null),

  disn_(Teuchos::null),
  veln_(Teuchos::null),
  accn_(Teuchos::null),
  angVeln_(Teuchos::null),
  angAccn_(Teuchos::null),
  radiusn_(Teuchos::null),
  densityn_(Teuchos::null),
  densityDotn_(Teuchos::null),
  specEnthalpyn_(Teuchos::null),

  fifc_(Teuchos::null),
  orient_(Teuchos::null),

  radius0_(Teuchos::null),
  radiusDot_(Teuchos::null),
  mass_(Teuchos::null),
  inertia_(Teuchos::null),
  pressure_(Teuchos::null),

  variableradius_((bool)DRT::INPUT::IntegralValue<int>(DRT::Problem::Instance()->CavitationParams(),"COMPUTE_RADIUS_RP_BASED")),
  collhandler_(Teuchos::null),
  interHandler_(Teuchos::null)
{
  // welcome user
  if ( (printlogo_) and (myrank_ == 0) )
  {
    Logo();
  }

  // check whether discretisation has been completed
  if (not discret_->Filled() || not actdis->HaveDofs())
  {
    dserror("Discretisation is not complete or has no dofs!");
  }

  // time state
  time_ = Teuchos::rcp(new TIMINT::TimIntMStep<double>(0, 0, 0.0));  // HERE SHOULD BE SOMETHING LIKE (particledynparams.get<double>("TIMEINIT"))
  dt_ = Teuchos::rcp(new TIMINT::TimIntMStep<double>(0, 0, particledynparams.get<double>("TIMESTEP")));
  step_ = 0;
  timen_ = (*time_)[0] + (*dt_)[0];  // set target time to initial time plus step size
  stepn_ = step_ + 1;

  // output file for energy
  if ( (writeenergyevery_ != 0) and (myrank_ == 0) )
    AttachEnergyFile();

  return;
}

/*----------------------------------------------------------------------*/
/* initialization of time integration */
void PARTICLE::TimInt::Init()
{
  // initialize the vectors
  dis_ = Teuchos::rcp(new TIMINT::TimIntMStep<Epetra_Vector>(0, 0, DofRowMapView(), true));
  vel_ = Teuchos::rcp(new TIMINT::TimIntMStep<Epetra_Vector>(0, 0, DofRowMapView(), true));
  acc_ = Teuchos::rcp(new TIMINT::TimIntMStep<Epetra_Vector>(0, 0, DofRowMapView(), true));
  radius_  = Teuchos::rcp(new TIMINT::TimIntMStep<Epetra_Vector>(0, 0, NodeRowMapView(), true));

  fifc_ = LINALG::CreateVector(*DofRowMapView(), true);
  mass_ = LINALG::CreateVector(*NodeRowMapView(), true);

  switch (particle_algorithm_->ParticleInteractionType())
  {
  case INPAR::PARTICLE::MeshFree :
  {
    densityDot_ = Teuchos::rcp(new TIMINT::TimIntMStep<Epetra_Vector>(0, 0, NodeRowMapView(), true));
    pressure_ = LINALG::CreateVector(*NodeRowMapView(), true);
  }// no break
  case INPAR::PARTICLE::Normal_DEM_thermo :
  {
    density_  = Teuchos::rcp(new TIMINT::TimIntMStep<Epetra_Vector>(0, 0, NodeRowMapView(), true));
    specEnthalpy_ = Teuchos::rcp(new TIMINT::TimIntMStep<Epetra_Vector>(0, 0, NodeRowMapView(), true));
    break;
  }
  default : //do nothing
    break;
  }

  if(variableradius_)
  {
    // initial radius of each particle for time dependent radius
    radius0_  = LINALG::CreateVector(*discret_->NodeRowMap(), true);
    // time derivative of radius of each particle for time dependent radius
    radiusDot_  = LINALG::CreateVector(*discret_->NodeRowMap(), true);
  }

  // set initial fields
  SetInitialFields();

  // Apply Dirichlet BC and create dbc map extractor
  {
    dbcmaps_ = Teuchos::rcp(new LINALG::MapExtractor());
    Teuchos::ParameterList p;
    p.set("total time", (*time_)[0]);
    discret_->EvaluateDirichlet(p, (*dis_)(0), (*vel_)(0), (*acc_)(0), Teuchos::null, dbcmaps_);
  }

  // copy everything into the n+1 state vectors
  disn_ = Teuchos::rcp(new Epetra_Vector(*(*dis_)(0)));
  veln_ = Teuchos::rcp(new Epetra_Vector(*(*vel_)(0)));
  accn_ = Teuchos::rcp(new Epetra_Vector(*(*acc_)(0)));

  switch (particle_algorithm_->ParticleInteractionType())
  {
  case INPAR::PARTICLE::MeshFree :
  {
    densityDotn_ = Teuchos::rcp(new Epetra_Vector(*(*densityDot_)(0)));
  }// no break
  case INPAR::PARTICLE::Normal_DEM_thermo :
  {
    radiusn_  = Teuchos::rcp(new Epetra_Vector(*(*radius_)(0)));
    densityn_ = Teuchos::rcp(new Epetra_Vector(*(*density_)(0)));
    specEnthalpyn_ = Teuchos::rcp(new Epetra_Vector(*(*specEnthalpy_)(0)));
    break;
  }
  default : //do nothing
    break;
  }

  // decide whether there is particle contact
  if(particle_algorithm_->ParticleInteractionType() != INPAR::PARTICLE::None)
  {

    // allocate vectors
    angVel_ = Teuchos::rcp(new TIMINT::TimIntMStep<Epetra_Vector>(0, 0, DofRowMapView(), true));
    angAcc_ = Teuchos::rcp(new TIMINT::TimIntMStep<Epetra_Vector>(0, 0, DofRowMapView(), true));

    // copy the vectors to the (n+1) state vectors
    angVeln_ = LINALG::CreateVector(*DofRowMapView(),true);
    angAccn_ = LINALG::CreateVector(*DofRowMapView(),true);

    if(writeorientation_)
    {
      // initialize orientation-vector for visualization
      orient_ = LINALG::CreateVector(*DofRowMapView(),true);
      InitializeOrientVector();
    }

    // create and fill inertia
    ComputeInertia(true);
  }
}

/*----------------------------------------------------------------------*/
/* Set intitial fields in structure (e.g. initial velocities) */
void PARTICLE::TimInt::SetInitialFields()
{

  // -----------------------------------------//
  // set material properties
  // -----------------------------------------//

  // all particles have identical density and radius (for now)
  const double initRadius = particle_algorithm_->ParticleMat()->initRadius_;
  const double initDensity = particle_algorithm_->ParticleMat()->initDensity_;

  double amplitude = DRT::Problem::Instance()->ParticleParams().get<double>("RANDOM_AMPLITUDE");

  (*radius_)(0)->PutScalar(initRadius);

  // mass-vector: m = rho * 4/3 * PI *r^3
  mass_->PutScalar(initDensity * Radius2Volume(initRadius));

  // -----------------------------------------//
  // set initial radius condition if existing
  // -----------------------------------------//

  std::vector<DRT::Condition*> condition;
  discret_->GetCondition("InitialParticleRadius", condition);

  // loop over conditions
  for (size_t i=0; i<condition.size(); ++i)
  {
    double scalar  = condition[i]->GetDouble("SCALAR");
    int funct_num  = condition[i]->GetInt("FUNCT");

    const std::vector<int>* nodeids = condition[i]->Nodes();
    //loop over particles in current condition
    for(size_t counter=0; counter<(*nodeids).size(); ++counter)
    {
      int lid = discret_->NodeRowMap()->LID((*nodeids)[counter]);
      if(lid != -1)
      {
        DRT::Node *currparticle = discret_->gNode((*nodeids)[counter]);
        double function_value =  DRT::Problem::Instance()->Funct(funct_num-1).Evaluate(0, currparticle->X(),0.0,discret_.get());
        double r_p = (*(*radius_)(0))[lid];
        r_p *= function_value * scalar;
        (*(*radius_)(0))[lid] = r_p;
        if(r_p <= 0.0)
          dserror("negative initial radius");

        // mass-vector: m = rho * 4/3 * PI * r^3
        (*mass_)[lid] = initDensity * Radius2Volume(r_p);
      }
    }
  }

  // -----------------------------------------//
  // evaluate random normal distribution for particle radii if applicable
  // -----------------------------------------//

  if(DRT::INPUT::IntegralValue<int>(DRT::Problem::Instance()->ParticleParams(),"RADIUS_DISTRIBUTION"))
  {
    // get minimum and maximum radius for particles
    const double min_radius = DRT::Problem::Instance()->ParticleParams().get<double>("MIN_RADIUS");
    const double max_radius = DRT::Problem::Instance()->ParticleParams().get<double>("MAX_RADIUS");

    // loop over all particles
    for(int n=0; n<discret_->NumMyRowNodes(); ++n)
    {
      // get local ID of current particle
      const int lid = discret_->NodeRowMap()->LID(discret_->lRowNode(n)->Id());

      // initialize random number generator with current particle radius as mean and input parameter value as standard deviation
      DRT::Problem::Instance()->Random()->SetMeanVariance((*(*radius_)(0))[lid],DRT::Problem::Instance()->ParticleParams().get<double>("RADIUS_DISTRIBUTION_SIGMA"));

      // generate normally distributed random value for particle radius
      double random_radius = DRT::Problem::Instance()->Random()->Normal();

      // check whether random value lies within allowed bounds, and adjust otherwise
      if(random_radius > max_radius)
      {
        random_radius = max_radius;
      }
      else if(random_radius < min_radius)
      {
        random_radius = min_radius;
      }

      // set particle radius to random value
      (*(*radius_)(0))[lid] = random_radius;
      // recompute particle mass
      (*mass_)[lid] = initDensity * Radius2Volume(random_radius);
    }
  }

  // -----------------------------------------//
  // initialize displacement field
  // -----------------------------------------//

  for(int n=0; n<discret_->NumMyRowNodes(); ++n)
  {
    DRT::Node* actnode = discret_->lRowNode(n);
    // get the first gid of a node and convert it into a LID
    int gid = discret_->Dof(actnode, 0);
    int lid = discret_->DofRowMap()->LID(gid);
    for (int dim=0; dim<3; ++dim)
    {
      if(amplitude)
      {
        double randomValue = DRT::Problem::Instance()->Random()->Uni();
        (*(*dis_)(0))[lid+dim] = actnode->X()[dim] + randomValue * amplitude * initRadius;
      }
      else
      {
        (*(*dis_)(0))[lid+dim] = actnode->X()[dim];
      }
    }
  }

  // -----------------------------------------//
  // set initial velocity field if existing
  // -----------------------------------------//

  const std::string field = "Velocity";
  std::vector<int> localdofs;
  localdofs.push_back(0);
  localdofs.push_back(1);
  localdofs.push_back(2);
  discret_->EvaluateInitialField(field,(*vel_)(0),localdofs);

  // -----------------------------------------//
  // set the other parameters. In case of meshfree set also pressure and density
  // -----------------------------------------//

  const MAT::PAR::ExtParticleMat* extParticleMat = particle_algorithm_->ExtParticleMat();
  switch (particle_algorithm_->ParticleInteractionType())
  {
  case INPAR::PARTICLE::MeshFree :
  {
    // the pressure at the beginning is null because the density is everywhere equal to the nominal initial density
    // ComputePressure();
  }// no break
  case INPAR::PARTICLE::Normal_DEM_thermo :
  {
    // set density in the density vector (useful only for thermodynamics)
    (*density_)(0)->PutScalar(initDensity);

    // initialize temperature of particles
    const double initTemperature = extParticleMat->initTemperature_;
    const double transitionTemperature = extParticleMat->transitionTemperature_;
    const double tempDiff = initTemperature - transitionTemperature;

    if (tempDiff > 0)
      (*specEnthalpy_)(0)->PutScalar(extParticleMat->SpecEnthalpyTL() + tempDiff * extParticleMat->CPL_);
    else if (initTemperature < transitionTemperature)
      (*specEnthalpy_)(0)->PutScalar(initTemperature * extParticleMat->CPS_);
    else
      dserror("TODO: start in the transition point - solid <-> liquid - still not implemented");
    break;
  }
  default : //do nothing
    break;
  }
}

/*----------------------------------------------------------------------*/
/* prepare time step and apply Dirichlet boundary conditions */
void PARTICLE::TimInt::PrepareTimeStep()
{
  // Update map containing Dirichlet DOFs if existing
  if(dbcmaps_ != Teuchos::null && dbcmaps_->CondMap()->NumGlobalElements() != 0)
  {
    // apply Dirichlet BC and rebuild map extractor
    ApplyDirichletBC(timen_, disn_, veln_, accn_, true);

    // do particle business
    particle_algorithm_->TransferParticles(true);
  }

  return;
}

/*----------------------------------------------------------------------*/
/* equilibrate system at initial state and identify consistent accelerations */
void PARTICLE::TimInt::DetermineMassDampConsistAccel()
{
  ComputeAcc(Teuchos::null, Teuchos::null, (*acc_)(0), Teuchos::null);

  return;
}

/*----------------------------------------------------------------------*/
/* acceleration is applied from given forces */
void PARTICLE::TimInt::ComputeAcc(
  Teuchos::RCP<Epetra_Vector> f_contact,
  Teuchos::RCP<Epetra_Vector> m_contact,
  Teuchos::RCP<Epetra_Vector> global_acc,
  Teuchos::RCP<Epetra_Vector> global_angAcc)
{
  int numrownodes = discret_->NodeRowMap()->NumMyElements();

  // in case of contact, consider corresponding forces and moments
  if(f_contact != Teuchos::null)
  {
    // sum all forces (contact and external)
    fifc_->Update(1.0, *f_contact, 1.0);

    // zero out non-planar entries in case of 2D
    if(particle_algorithm_->ParticleDim() == INPAR::PARTICLE::particle_2Dz)
    {
      for(int i=0; i<numrownodes; ++i)
      {
        (*m_contact)[i*3+0] = 0.0;
        (*m_contact)[i*3+1] = 0.0;
      }
    }

    // compute angular acceleration
    for(int i=0; i<numrownodes; ++i)
    {
      const double invinertia = 1.0/(*inertia_)[i];
      for(int dim=0; dim<3; ++dim)
        (*global_angAcc)[i*3+dim] = invinertia * (*m_contact)[i*3+dim];
    }
  }

  // zero out non-planar entries in case of 2D
  if(particle_algorithm_->ParticleDim() == INPAR::PARTICLE::particle_2Dz)
  {
    for(int i=0; i<numrownodes; ++i)
      (*fifc_)[i*3+2] = 0.0;
  }

  // update of translational acceleration
  for(int i=0; i<numrownodes; ++i)
  {
    const double invmass = 1.0/(*mass_)[i];
    for(int dim=0; dim<3; ++dim)
      (*global_acc)[i*3+dim] = invmass * (*fifc_)[i*3+dim];
  }

  return;
}

/*---------------------------------------------------------------*/
/* Apply Dirichlet boundary conditions on provided state vectors */
void PARTICLE::TimInt::ApplyDirichletBC
(
  const double time,
  Teuchos::RCP<Epetra_Vector> dis,
  Teuchos::RCP<Epetra_Vector> vel,
  Teuchos::RCP<Epetra_Vector> acc,
  bool recreatemap
)
{
  // needed parameters
  Teuchos::ParameterList p;
  p.set("total time", time);  // target time

  // predicted Dirichlet values
  // \c dis then also holds prescribed new Dirichlet displacements
  discret_->ClearState();
  if (recreatemap)
    discret_->EvaluateDirichlet(p, dis, vel, acc, Teuchos::null, dbcmaps_);
  else
    discret_->EvaluateDirichlet(p, dis, vel, acc, Teuchos::null, Teuchos::null);
  discret_->ClearState();

  return;
}

/*----------------------------------------------------------------------*/
/* Update time and step counter */
void PARTICLE::TimInt::UpdateStepTime()
{
  // update time and step
  time_->UpdateSteps(timen_);  // t_{n} := t_{n+1}, etc
  step_ = stepn_;  // n := n+1
  //
  timen_ += (*dt_)[0];
  stepn_ += 1;

  // new deal
  return;
}

/*----------------------------------------------------------------------*/
/* State vectors are updated according to the new distribution of particles */

void PARTICLE::TimInt::UpdateStatesAfterParticleTransfer()
{
  UpdateStateVectorMap(dis_);
  UpdateStateVectorMap(vel_);
  UpdateStateVectorMap(acc_);
  UpdateStateVectorMap(angVel_);
  UpdateStateVectorMap(angAcc_);
  UpdateStateVectorMap(radius_,true);
  UpdateStateVectorMap(density_,true);
  UpdateStateVectorMap(densityDot_,true);
  UpdateStateVectorMap(specEnthalpy_,true);

  UpdateStateVectorMap(disn_);
  UpdateStateVectorMap(veln_);
  UpdateStateVectorMap(accn_);
  UpdateStateVectorMap(angVeln_);
  UpdateStateVectorMap(angAccn_);
  UpdateStateVectorMap(radiusn_,true);
  UpdateStateVectorMap(densityn_,true);
  UpdateStateVectorMap(densityDotn_,true);
  UpdateStateVectorMap(specEnthalpyn_,true);

  UpdateStateVectorMap(fifc_);
  UpdateStateVectorMap(orient_);

  UpdateStateVectorMap(radius0_,true);
  UpdateStateVectorMap(radiusDot_,true);
  UpdateStateVectorMap(mass_,true);
  UpdateStateVectorMap(inertia_,true);
  UpdateStateVectorMap(pressure_,true);
}

/*----------------------------------------------------------------------*/
/* Read and set restart values */
void PARTICLE::TimInt::ReadRestart
(
  const int step
)
{
  IO::DiscretizationReader reader(discret_, step);
  if (step != reader.ReadInt("step"))
    dserror("Time step on file not equal to given step");

  restart_ = step;
  step_ = step;
  stepn_ = step_ + 1;
  time_ = Teuchos::rcp(new TIMINT::TimIntMStep<double>(0, 0, reader.ReadDouble("time")));
  timen_ = (*time_)[0] + (*dt_)[0];

  ReadRestartState();

  return;
}

/*----------------------------------------------------------------------*/
/* Read and set restart state */
void PARTICLE::TimInt::ReadRestartState()
{
  IO::DiscretizationReader reader(discret_, step_);
  // maps need to be adapted to restarted discretization

  UpdateStatesAfterParticleTransfer();

  // start with reading mass in order to find out whether particles exist
  reader.ReadVector(mass_, "mass");

  // check, in case there is nothing, do not read the file
  if(mass_->GlobalLength() == 0)
    return;

  // now, the remaining state vectors an be read in
  reader.ReadVector(disn_, "displacement");
  dis_->UpdateSteps(*disn_);
  reader.ReadVector(veln_, "velocity");
  vel_->UpdateSteps(*veln_);
  reader.ReadVector(accn_, "acceleration");
  acc_->UpdateSteps(*accn_);


  switch (particle_algorithm_->ParticleInteractionType())
  {
  case INPAR::PARTICLE::MeshFree :
  {
    // read densityDot
    reader.ReadVector(densityDotn_, "densityDot");
    densityDot_->UpdateSteps(*densityDotn_);
  }// no break
  case INPAR::PARTICLE::Normal_DEM_thermo :
  {
    // read radius
    reader.ReadVector(radiusn_, "radius");
    radius_->UpdateSteps(*radiusn_);
    // read density
    reader.ReadVector(densityn_, "density");
    density_->UpdateSteps(*densityn_);
    // read specEnthalpy
    reader.ReadVector(specEnthalpyn_, "specEnthalpy");
    specEnthalpy_->UpdateSteps(*specEnthalpyn_);
    break;
  }
  default :
  {
    // create a dummy vector to extract the radius vector (radiusn_ does not exist)
    Teuchos::RCP<Epetra_Vector> radius = LINALG::CreateVector(*discret_->NodeRowMap(), true);
    reader.ReadVector(radius, "radius");
    radius_->UpdateSteps(*radius);
    break;
  }
  }

  // read in particle collision relevant data
  if(collhandler_ != Teuchos::null)
  {
    // initialize inertia
    ComputeInertia();

    reader.ReadVector(angVeln_, "ang_velocity");
    angVel_->UpdateSteps(*angVeln_);
    reader.ReadVector(angAccn_, "ang_acceleration");
    angAcc_->UpdateSteps(*angAccn_);
    if(writeorientation_)
      reader.ReadVector(orient_, "orientation");
  }

  // read in variable radius relevant data
  if(variableradius_ == true)
  {
    reader.ReadVector(radius0_, "radius0");
    reader.ReadVector(radiusDot_, "radiusdot");
  }
}

/*----------------------------------------------------------------------*/
/* Calculate all output quantities that depend on a potential material history */
void PARTICLE::TimInt::PrepareOutput()
{
  DetermineEnergy();
  return;
}

/*----------------------------------------------------------------------*/
/* output to file */
void PARTICLE::TimInt::OutputStep(bool forced_writerestart)
{
  // this flag is passed along subroutines and prevents
  // repeated initialising of output writer, printing of
  // state vectors, or similar
  bool datawritten = false;

  // output restart (try this first)
  // write restart step
  if ( (writerestartevery_ and ((step_-restart_)%writerestartevery_ == 0)) or forced_writerestart )
  {
    OutputRestart(datawritten);
  }

  // output results (not necessary if restart in same step)
  if ( writestate_
       and writeresultsevery_ and ((step_-restart_)%writeresultsevery_ == 0)
       and (not datawritten) )
  {
    OutputState(datawritten);
  }

  // output energy
  if ( writeenergyevery_ and ((step_-restart_)%writeenergyevery_ == 0) )
  {
    OutputEnergy();
  }

  return;
}

/*----------------------------------------------------------------------*/
/* write restart */
void PARTICLE::TimInt::OutputRestart
(
  bool& datawritten
)
{
  // Yes, we are going to write...
  datawritten = true;

  // mesh is written to disc
  output_->ParticleOutput(step_, (*time_)[0], true);
  output_->NewStep(step_, (*time_)[0]);

  output_->WriteVector("displacement", (*dis_)(0));
  output_->WriteVector("velocity", (*vel_)(0));
  output_->WriteVector("acceleration", (*acc_)(0));

  output_->WriteVector("radius", (*radius_)(0), output_->nodevector);
  output_->WriteVector("mass", mass_, output_->nodevector);

  switch (particle_algorithm_->ParticleInteractionType())
  {
  case INPAR::PARTICLE::MeshFree :
  case INPAR::PARTICLE::Normal_DEM_thermo :
  {
    output_->WriteVector("density", (*density_)(0), output_->nodevector);
    output_->WriteVector("specEnthalpy", (*specEnthalpy_)(0), output_->nodevector);
    output_->WriteVector("temperature", Temperaturen(), output_->nodevector);
    break;
  }
  default : // do nothing
    break;
  }

  if(variableradius_)
  {
    output_->WriteVector("radius0", radius0_, output_->nodevector);
    output_->WriteVector("radiusdot", radiusDot_, output_->nodevector);
  }

  if(collhandler_ != Teuchos::null)
  {
    if(angVeln_ != Teuchos::null)
    {
      output_->WriteVector("ang_velocity", (*angVel_)(0));
      output_->WriteVector("ang_acceleration", (*angAcc_)(0));
    }

    if(writeorientation_)
      output_->WriteVector("orientation", orient_);
  }

  // maps are rebuild in every step so that reuse is not possible
  // keeps memory usage bounded
  output_->ClearMapCache();

  // info dedicated to user's eyes staring at standard out
  if ( (myrank_ == 0) and printscreen_ and ((step_-restart_)%printscreen_==0))
  {
    printf("====== Restart written in step %d\n", step_);
    fflush(stdout);
  }

  // info dedicated to processor error file
  if (printerrfile_)
  {
    fprintf(errfile_, "====== Restart written in step %d\n", step_);
    fflush(errfile_);
  }

  return;
}

/*----------------------------------------------------------------------*/
/* output displacements, velocities, accelerations, temperatures, and pressure */
void PARTICLE::TimInt::OutputState
(
  bool& datawritten
)
{
  // Yes, we are going to write...
  datawritten = true;

  // mesh is not written to disc, only maximum node id is important for output
  output_->ParticleOutput(step_, (*time_)[0], false);
  output_->NewStep(step_, (*time_)[0]);

  output_->WriteVector("displacement", (*dis_)(0));
  output_->WriteVector("velocity", (*vel_)(0));
  if(writevelacc_)
    output_->WriteVector("acceleration", (*acc_)(0));

  output_->WriteVector("radius", (*radius_)(0), output_->nodevector);
  switch (particle_algorithm_->ParticleInteractionType())
  {
  case INPAR::PARTICLE::MeshFree :
  case INPAR::PARTICLE::Normal_DEM_thermo :
  {
    output_->WriteVector("density", (*density_)(0), output_->nodevector);
    output_->WriteVector("specEnthalpy", (*specEnthalpy_)(0), output_->nodevector);
    output_->WriteVector("temperature", Temperaturen(), output_->nodevector);
    break;
  }
  default : //do nothing
    break;
  }
  if(collhandler_ != Teuchos::null and writeorientation_)
    output_->WriteVector("orientation", orient_);
  // maps are rebuild in every step so that reuse is not possible
  // keeps memory usage bounded
  output_->ClearMapCache();
}

/*----------------------------------------------------------------------*/
/* Calculation of internal, external and kinetic energy */
void PARTICLE::TimInt::DetermineEnergy()
{
  if ( writeenergyevery_ and (stepn_%writeenergyevery_ == 0) and collhandler_ != Teuchos::null)
  {
    LINALG::Matrix<3,1> gravity_acc = particle_algorithm_->GetGravityAcc();

    // total kinetic energy
    kinergy_ = 0.0;

    int numrownodes = discret_->NodeRowMap()->NumMyElements();
    for(int i=0; i<numrownodes; ++i)
    {
      double specific_energy = 0.0;
      double kinetic_energy = 0.0;
      double rot_energy = 0.0;

      for(int dim=0; dim<3; ++dim)
      {
        // gravitation
        specific_energy -=  gravity_acc(dim) * (*disn_)[i*3+dim];

        // kinetic energy
        kinetic_energy += pow((*veln_)[i*3+dim], 2.0 );

        // rotation
        rot_energy += pow((*angVeln_)[i*3+dim], 2.0);
      }

      intergy_ += (*mass_)[i] * specific_energy;
      kinergy_ += 0.5 * ((*mass_)[i] * kinetic_energy + (*inertia_)[i] * rot_energy);
    }

    double global_energy[2] = {0.0, 0.0};
    double energies[2] = {intergy_, kinergy_};
    discret_->Comm().SumAll(&energies[0], &global_energy[0], 2);

    intergy_ = global_energy[0];
    kinergy_ = global_energy[1];
    // total external energy not available
    extergy_ = 0.0;
  }

  return;
}

/*----------------------------------------------------------------------*/
/* output system energies */
void PARTICLE::TimInt::OutputEnergy()
{
  // total energy
  double totergy = kinergy_ + intergy_ - extergy_;

  // the output
  if (myrank_ == 0)
  {
    *energyfile_ << " " << std::setw(9) << step_
                 << std::scientific  << std::setprecision(16)
                 << " " << (*time_)[0]
                 << " " << totergy
                 << " " << kinergy_
                 << " " << intergy_
                 << " " << extergy_
                 << " " << collhandler_->GetMaxPenetration()
                 << std::endl;
  }
  return;
}

/*----------------------------------------------------------------------*/
/* Set forces due to interface loads, the force is expected external-force-like */
void PARTICLE::TimInt::SetForceInterface
(
  Teuchos::RCP<Epetra_MultiVector> iforce  ///< the force on interface
)
{
  fifc_->Update(1.0, *iforce, 0.0);
  return;
}


/*----------------------------------------------------------------------*/
/* Attach file handle for energy file #energyfile_                      */
void PARTICLE::TimInt::AttachEnergyFile()
{
  if (energyfile_.is_null())
  {
    std::string energyname
      = DRT::Problem::Instance()->OutputControlFile()->FileName()
      + "_particle.energy";
    energyfile_ = Teuchos::rcp(new std::ofstream(energyname.c_str()));
    (*energyfile_) << "# timestep time total_energy"
                   << " kinetic_energy internal_energy external_energy max_particle_penetration"
                   << std::endl;
  }
  return;
}

/*----------------------------------------------------------------------*/
/* Creates the field test                                               */
Teuchos::RCP<DRT::ResultTest> PARTICLE::TimInt::CreateFieldTest()
{
  return Teuchos::rcp(new PartResultTest(*this));
}

/*----------------------------------------------------------------------*/
/* dof map of vector of unknowns                                        */
Teuchos::RCP<const Epetra_Map> PARTICLE::TimInt::DofRowMap()
{
  const Epetra_Map* dofrowmap = discret_->DofRowMap();
  return Teuchos::rcp(new Epetra_Map(*dofrowmap));
}

/*----------------------------------------------------------------------*/
/* view of dof map of vector of unknowns                                */
const Epetra_Map* PARTICLE::TimInt::DofRowMapView()
{
  return discret_->DofRowMap();
}

/*----------------------------------------------------------------------*/
/* node map of particles                                                */
Teuchos::RCP<const Epetra_Map> PARTICLE::TimInt::NodeRowMap()
{
  const Epetra_Map* noderowmap = discret_->NodeRowMap();
  return Teuchos::rcp(new Epetra_Map(*noderowmap));
}

/*----------------------------------------------------------------------*/
/* view of node map of particles                                        */
const Epetra_Map* PARTICLE::TimInt::NodeRowMapView()
{
  return discret_->NodeRowMap();
}

/*-----------------------------------------------------------------------------*/
/* Update TimIntMStep state vector with the new (appropriate) map from discret_*/
void PARTICLE::TimInt::UpdateStateVectorMap(Teuchos::RCP<TIMINT::TimIntMStep<Epetra_Vector> > &stateVector, bool trg_nodeVectorType)
{
  if (stateVector != Teuchos::null and (*stateVector)(0) != Teuchos::null)
  {
    const Teuchos::RCP<Epetra_Vector> old = Teuchos::rcp(new Epetra_Vector(*(*stateVector)(0)));

    if (trg_nodeVectorType)
      stateVector->ReplaceMaps(NodeRowMapView());
    else
      stateVector->ReplaceMaps(DofRowMapView());

    LINALG::Export(*old, *(*stateVector)(0));
  }
}

/*-----------------------------------------------------------------------------*/
/* Update state vector with the new (appropriate) map from discret_*/
void PARTICLE::TimInt::UpdateStateVectorMap(Teuchos::RCP<Epetra_Vector > &stateVector, bool trg_nodeVectorType)
{
  if (stateVector != Teuchos::null)
  {
    Teuchos::RCP<Epetra_Vector> old = stateVector;

    if (trg_nodeVectorType)
      stateVector = LINALG::CreateVector(*discret_->NodeRowMap(),true);
    else
      stateVector = LINALG::CreateVector(*discret_->DofRowMap(),true);


    LINALG::Export(*old, *stateVector);
  }
}

/*-----------------------------------------------------------------------------*/
// Compute inertia
void PARTICLE::TimInt::ComputeInertia(const int &lidNode)
{
  // Find the most-updated radius vector
  double radius;
  if (radiusn_ != Teuchos::null)
    radius = (*radiusn_)[lidNode];
  else
    radius = (*(*radius_)(0))[lidNode];

  // inertia-vector -> sphere: I = 2/5 * m * r^2
  (*inertia_)[lidNode] = 0.4 * (*mass_)[lidNode] *radius * radius;

}

/*-----------------------------------------------------------------------------*/
/* Compute the inertia vector
 * The most updated radius is used (radius_ vs radiusn_)
 * inertia-vector -> sphere: I = 2/5 * m * r^2 */
void PARTICLE::TimInt::ComputeInertia(bool trg_createInertiaVector)
{
  // rebuild the inertia vector
  if (trg_createInertiaVector || inertia_ == Teuchos::null)
    inertia_  = LINALG::CreateVector(*discret_->NodeRowMap(), true);

  // compute inertia for every particle
  for (int lidNode = 0; lidNode < discret_->NodeRowMap()->NumMyElements(); ++lidNode)
    ComputeInertia(lidNode);
}

/*----------------------------------------------------------------------*/
/* compute temperature from the specEnthalpy  */
Teuchos::RCP<const Epetra_Vector> PARTICLE::TimInt::ComputeTemperature(Teuchos::RCP<const Epetra_Vector> specEnthalpy)
{
  // check: no specEnthalpy? no temperature :)
  if (specEnthalpy == Teuchos::null)
    return Teuchos::null;

  //extract the interesting parameters
  const double specEnthalpyST = particle_algorithm_->ExtParticleMat()->SpecEnthalpyST();
  const double specEnthalpyTL = particle_algorithm_->ExtParticleMat()->SpecEnthalpyTL();
  const double transitionTemperature = particle_algorithm_->ExtParticleMat()->transitionTemperature_;
  const double CPS = particle_algorithm_->ExtParticleMat()->CPS_;
  const double inv_CPS = 1/CPS;
  const double CPL = particle_algorithm_->ExtParticleMat()->CPL_;
  const double inv_CPL = 1/CPL;

  // create temperature vector
  Teuchos::RCP<Epetra_Vector> temperature = LINALG::CreateVector(*NodeRowMapView(), true);


  for (int lidNode = 0; lidNode < discret_->NodeRowMap()->NumMyElements(); ++lidNode)
  {
    // extract the nodes values
    const double &currNodeSpecEnthalpy = (*specEnthalpy)[lidNode];
    double &currNodeTemperature = (*temperature)[lidNode];

    // compute temperature of the node
    if (currNodeSpecEnthalpy < specEnthalpyST)
      currNodeTemperature = currNodeSpecEnthalpy * inv_CPS;
    else if (currNodeSpecEnthalpy > specEnthalpyTL)
      currNodeTemperature = transitionTemperature + (currNodeSpecEnthalpy - specEnthalpyTL) * inv_CPL;
    else
      currNodeTemperature = transitionTemperature;
  }

  return temperature;
}

/*-----------------------------------------------------------------------------*/
// Compute the pressure
void PARTICLE::TimInt::ComputePressure(const int &lidNode, bool trg_Nplus1)
{
  // n+1 or not? Let's pick the correct pointer
  Teuchos::RCP<Epetra_Vector> specEnthalpy, density;
  if (trg_Nplus1)
  {
    specEnthalpy = specEnthalpyn_;
    density = densityn_;
  }
  else
  {
    specEnthalpy = (*specEnthalpy_)(0);
    density = (*density_)(0);
  }

  // extract the material parameters
  const MAT::PAR::ExtParticleMat* extParticleMat = particle_algorithm_->ExtParticleMat();
  const double initDensity = extParticleMat->initDensity_;
  const double specEnthalpyST = extParticleMat->SpecEnthalpyST();
  const double specEnthalpyTL = extParticleMat->SpecEnthalpyTL();
  const double speedOfSoundS = extParticleMat->SpeedOfSoundS();
  const double speedOfSoundL = extParticleMat->SpeedOfSoundL();

  const double densityDelta = (*density)[lidNode] - initDensity;
  if ((*specEnthalpy)[lidNode] <= specEnthalpyST)
    (*pressure_)[lidNode] = std::pow(speedOfSoundS,2) * densityDelta;
  else if ((*specEnthalpy)[lidNode] >= specEnthalpyTL)
    (*pressure_)[lidNode] = std::pow(speedOfSoundL,2) * densityDelta;
  else
  {
    const double speedOfSoundT = extParticleMat->SpeedOfSoundT((*specEnthalpy)[lidNode]);
    (*pressure_)[lidNode] = std::pow(speedOfSoundT,2) * densityDelta;
  }
}

/*-----------------------------------------------------------------------------*/
// Compute pressure vector
void PARTICLE::TimInt::ComputePressure(bool trg_createPressureVector, bool trg_Nplus1)
{
  // rebuild the pressure vector
  if (trg_createPressureVector || pressure_ == Teuchos::null)
    pressure_ = LINALG::CreateVector(*discret_->NodeRowMap(), true);

  // compute inertia for every particle
  for (int lidNode = 0; lidNode < discret_->NodeRowMap()->NumMyElements(); ++lidNode)
    ComputePressure(lidNode, trg_Nplus1);
}

/*----------------------------------------------------------------------*/
/* initialization of vector for visualization of the particle orientation */
void PARTICLE::TimInt::InitializeOrientVector()
{
  int numrownodes = discret_->NodeRowMap()->NumMyElements();
  for(int i=0; i<numrownodes; ++i)
  {
    (*orient_)[i*3] = 0.0;
    (*orient_)[i*3+1] = 0.0;
    (*orient_)[i*3+2] = 1.0;
  }
}
