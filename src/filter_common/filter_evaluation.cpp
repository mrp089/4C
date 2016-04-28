/*----------------------------------------------------------------------*/
/*!
\file filter_evaluation.cpp

\brief compatibility definitions

<pre>
\maintainer Rui Fang
            fang@lnm.mw.tum.de
            http://www.lnm.mw.tum.de/
            089-289-15251
</pre>

Some discretization functions cannot be included in the filter build
because they use ccarat facilities that are not available inside the
filter. But to link the filter, stubs of these functions are needed.
*/
/*----------------------------------------------------------------------*/
#include "../drt_mat/micromaterial.H"
#include "../drt_mat/scatra_mat_multiscale.H"

// forward declarations
namespace MAT
{
  class MicroMaterialGP;
  class ScatraMatMultiScaleGP;
}

void MAT::MicroMaterial::Evaluate(const LINALG::Matrix<3,3>* defgrd,
                                          const LINALG::Matrix<6,1>* glstrain,
                                          Teuchos::ParameterList& params,
                                          LINALG::Matrix<6,1>* stress,
                                          LINALG::Matrix<6,6>* cmat,
                                          const int eleGID)
{
  dserror("MAT::MicroMaterial::Evaluate not available");
}

double MAT::MicroMaterial::Density() const
{
  dserror("MAT::MicroMaterial::Density not available");
  return 0.0;
}

void MAT::MicroMaterial::PrepareOutput()
{
  dserror("MAT::MicroMaterial::PrepareOutput not available");
}

void MAT::MicroMaterial::Output()
{
  dserror("MAT::MicroMaterial::Output not available");
}

void MAT::MicroMaterial::Update()
{
  dserror("MAT::MicroMaterial::Update not available");
}

void MAT::MicroMaterial::ReadRestart(const int gp, const int eleID, const bool eleowner)
{
  dserror("MAT::MicroMaterial::ReadRestart not available");
}

void MAT::MicroMaterial::InvAnaInit(bool eleowner, int eleID)
{
  dserror("Mat::MicroMaterial::InvAna_Init not available");
}


/*--------------------------------------------------------------------*
 | initialize multi-scale scalar transport material        fang 02/16 |
 *--------------------------------------------------------------------*/
void MAT::ScatraMatMultiScale::Initialize(
    const int   ele_id,   //!< macro-scale element ID
    const int   gp_id     //!< macro-scale Gauss point ID
    )
{
  // this function should never be called
  dserror("MAT::ScatraMatMultiScale::Initialize not available!");

  return;
}


/*--------------------------------------------------------------------*
 | prepare time step on micro scale                        fang 02/16 |
 *--------------------------------------------------------------------*/
void MAT::ScatraMatMultiScale::PrepareTimeStep(
    const int      gp_id,        //!< macro-scale Gauss point ID
    const double   phinp_macro   //!< macro-scale state variable
    ) const
{
  // this function should never be called
  dserror("MAT::ScatraMatMultiScale::PrepareTimeStep not available!");

  return;
}


/*--------------------------------------------------------------------*
 | evaluate multi-scale scalar transport material          fang 11/15 |
 *--------------------------------------------------------------------*/
void MAT::ScatraMatMultiScale::Evaluate(
    const int      gp_id,          //!< macro-scale Gauss point ID
    const double   phinp_macro,    //!< macro-scale state variable
    double&        q_micro,        //!< micro-scale flux
    double&        dq_dphi_micro   //!< derivative of micro-scale flux w.r.t. macro-scale state variable
    ) const
{
  // this function should never be called
  dserror("MAT::ScatraMatMultiScale::Evaluate not available!");

  return;
}


/*--------------------------------------------------------------------*
 | update multi-scale scalar transport material            fang 11/15 |
 *--------------------------------------------------------------------*/
void MAT::ScatraMatMultiScale::Update(
    const int   gp_id   //!< macro-scale Gauss point ID
    ) const
{
  // this function should never be called
  dserror("MAT::ScatraMatMultiScale::Update not available!");

  return;
}


/*--------------------------------------------------------------------*
 | create output on micro scale                            fang 02/16 |
 *--------------------------------------------------------------------*/
void MAT::ScatraMatMultiScale::Output(
    const int   gp_id   //!< macro-scale Gauss point ID
    ) const
{
  // this function should never be called
  dserror("MAT::ScatraMatMultiScale::Output not available!");

  return;
}


/*--------------------------------------------------------------------*
 | read restart on micro scale                             fang 03/16 |
 *--------------------------------------------------------------------*/
void MAT::ScatraMatMultiScale::ReadRestart(
    const int   gp_id   //!< macro-scale Gauss point ID
    ) const
{
  // this function should never be called
  dserror("MAT::ScatraMatMultiScale::ReadRestart not available!");

  return;
}
