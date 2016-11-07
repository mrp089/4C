/*-----------------------------------------------------------------------------*/
/*!
\file particle_utils.cpp

\brief General functions for the Particle-MeshFree dynamics

\level 3

\maintainer Alessandro Cattabiani
*/


#include "particle_utils.H"
#include "../drt_mat/particle_mat.H"
#include "../drt_mat/extparticle_mat.H"



/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Compute the inertia vector */
void PARTICLE::Utils::ComputeInertia(
  const Teuchos::RCP<const Epetra_Vector> radius,
  const Teuchos::RCP<const Epetra_Vector> mass,
  Teuchos::RCP<Epetra_Vector> &inertia,
  bool trg_createInertiaVector)
{
  // checks
  if (radius == Teuchos::null ||
      mass == Teuchos::null)
    dserror("radius or mass vectors are empty");

  // rebuild the inertia vector
  if (trg_createInertiaVector || inertia == Teuchos::null)
    inertia = Teuchos::rcp(new Epetra_Vector(mass->Map(), true));

  // compute inertia for every particle
  for (int lidNode = 0; lidNode < mass->MyLength(); ++lidNode)
    (*inertia)[lidNode] = ComputeInertia((*radius)[lidNode], (*mass)[lidNode]);
}


/*----------------------------------------------------------------------*/
/* compute temperature from the specEnthalpy  */
double PARTICLE::Utils::SpecEnthalpy2Temperature(
    const double& specEnthalpy,
    const MAT::PAR::ExtParticleMat* extParticleMat)
{
  //extract the interesting parameters
  const double specEnthalpyST = extParticleMat->SpecEnthalpyST();
  const double specEnthalpyTL = extParticleMat->SpecEnthalpyTL();
  const double transitionTemperature = extParticleMat->transitionTemperature_;
  const double CPS = extParticleMat->CPS_;
  const double inv_CPS = 1/CPS;
  const double CPL = extParticleMat->CPL_;
  const double inv_CPL = 1/CPL;

  // compute temperature of the node
  if (specEnthalpy < specEnthalpyST)
    return specEnthalpy * inv_CPS;
  else if (specEnthalpy > specEnthalpyTL)
    return transitionTemperature + (specEnthalpy - specEnthalpyTL) * inv_CPL;
  else
    return transitionTemperature;
}


/*----------------------------------------------------------------------*/
/* compute temperature from the specEnthalpy - (vector version) */
Teuchos::RCP<const Epetra_Vector> PARTICLE::Utils::SpecEnthalpy2Temperature(
    const Teuchos::RCP<const Epetra_Vector>& specEnthalpy,
    const MAT::PAR::ExtParticleMat* extParticleMat)
{
  // check: no specEnthalpy? no temperature :)
  if (specEnthalpy == Teuchos::null)
    return Teuchos::null;

  // create temperature vector
  Teuchos::RCP<Epetra_Vector> temperature = Teuchos::rcp(new Epetra_Vector(specEnthalpy->Map(), true));

  for (int lidNode = 0; lidNode < specEnthalpy->MyLength(); ++lidNode)
  {
    (*temperature)[lidNode] = PARTICLE::Utils::SpecEnthalpy2Temperature((*specEnthalpy)[lidNode],extParticleMat);
  }
  return temperature;
}

/*-----------------------------------------------------------------------------*/
// Compute pressure vector
void PARTICLE::Utils::Density2Pressure(
  const Teuchos::RCP<const Epetra_Vector> deltaDensity,
  const Teuchos::RCP<const Epetra_Vector> specEnthalpy,
  Teuchos::RCP<Epetra_Vector> &pressure,
  const MAT::PAR::ExtParticleMat* extParticleMat,
  bool trg_createPressureVector)
{
  // checks
  if (deltaDensity == Teuchos::null)
  {
    pressure = Teuchos::null;
    return;
  }
  if (specEnthalpy == Teuchos::null)
    dserror("specEnthalpy is a null pointer!");

  //extract the interesting parameters
  const double specEnthalpyST = extParticleMat->SpecEnthalpyST();
  const double specEnthalpyTL = extParticleMat->SpecEnthalpyTL();
  const double speedOfSoundS = extParticleMat->SpeedOfSoundS();
  const double speedOfSoundL = extParticleMat->SpeedOfSoundL();

  // rebuild the pressure vector
  if (trg_createPressureVector || pressure == Teuchos::null)
    Teuchos::RCP<Epetra_Vector> pressure = Teuchos::rcp(new Epetra_Vector(deltaDensity->Map(), true));

  // compute inertia for every particle
  for (int lidNode = 0; lidNode < deltaDensity->MyLength(); ++lidNode)
  {
    const double densityDelta = (*deltaDensity)[lidNode];// - baseDensity;
    if ((*specEnthalpy)[lidNode] <= specEnthalpyST)
      (*pressure)[lidNode] = Density2Pressure(speedOfSoundS, densityDelta);
    else if ((*specEnthalpy)[lidNode] >= specEnthalpyTL)
      (*pressure)[lidNode] = Density2Pressure(speedOfSoundL, densityDelta);
    else
    {
      const double speedOfSoundT = extParticleMat->SpeedOfSoundT((*specEnthalpy)[lidNode]);
      (*pressure)[lidNode] = Density2Pressure(speedOfSoundT, densityDelta);
    }
  }
}


/*-----------------------------------------------------------------------------*/
// compute the intersection area of two particles that are in contact. It returns 0 if there is no contact
double PARTICLE::Utils::IntersectionAreaPvsP(const double& radius1, const double& radius2, const double& dis)
{
  //checks
  if (radius1<=0 || radius2<=0 || dis<=0)
    dserror("input parameters are unacceptable");
  if (dis >= radius1 + radius2)
    return 0;

  return M_PI * radius1 * radius1 - (std::pow(radius1 * radius1 - radius2 * radius2 + dis * dis,2) / (4 * dis * dis));
}