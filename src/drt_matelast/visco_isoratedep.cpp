/*----------------------------------------------------------------------*/
/*!
\file visco_isofreqratedep.cpp
\brief


the input line should read
  MAT 1 VISCO_IsRateDep N 1

<pre>
Maintainer: Anna Birzle
	    anna.birzle@tum.de
            089/289 15255
</pre>
*/

/*----------------------------------------------------------------------*/
/* macros */

/*----------------------------------------------------------------------*/
/* headers */
#include "visco_isoratedep.H"
#include "../drt_mat/matpar_material.H"

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
MAT::ELASTIC::PAR::IsoRateDep::IsoRateDep(
  Teuchos::RCP<MAT::PAR::Material> matdata
  )
: Parameter(matdata),
  n_(matdata->GetDouble("N"))
{
}


Teuchos::RCP<MAT::Material> MAT::ELASTIC::PAR::IsoRateDep::CreateMaterial()
{
  return Teuchos::null;
  //return Teuchos::rcp( new MAT::ELASTIC::IsoRateDep( this ) );
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
MAT::ELASTIC::IsoRateDep::IsoRateDep()
  : Summand(),
    params_(NULL)
{
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
MAT::ELASTIC::IsoRateDep::IsoRateDep(MAT::ELASTIC::PAR::IsoRateDep* params)
  : params_(params)
{
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ELASTIC::IsoRateDep::AddCoefficientsViscoModified(
  const LINALG::Matrix<3,1>& modinv,
  LINALG::Matrix<8,1>& modmy,
  LINALG::Matrix<33,1>& modxi,
  LINALG::Matrix<7,1>& modrateinv,
  Teuchos::ParameterList& params
  )
{

  const double n = params_ -> n_;

  // get time algorithmic parameters.
  double dt = params.get<double>("delta time"); // TIMESTEP in the .dat file

  modmy(1) += 2* n * modrateinv(1) ;
  modmy(2) += (2* n *(modinv(0)-3) ) / dt;

  modxi(1) += (4* n) / dt;
  modxi(2) += (4* n *(modinv(0)-3) ) / (dt*dt);

  return;
}


/*----------------------------------------------------------------------*/
