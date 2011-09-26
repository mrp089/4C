/*!----------------------------------------------------------------------
\file so_nstet_multiscale.cpp
\brief

<pre>
Maintainer: Lena Wiechert
            wiechert@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15303
</pre>

*----------------------------------------------------------------------*/
#ifdef D_SOLID3
#ifdef CCADISCRET
#include "so_nstet.H"
#include "../drt_mat/micromaterial.H"
#include "../drt_lib/drt_globalproblem.H"

using namespace std; // cout etc.

extern struct _GENPROB     genprob;


/*----------------------------------------------------------------------*
 |  homogenize material density (public)                        lw 07/07|
 *----------------------------------------------------------------------*/
// this routine is intended to determine a homogenized material
// density for multi-scale analyses by averaging over the initial volume

void DRT::ELEMENTS::NStet::nstet_homog(ParameterList&  params)
{
  const double density = Material()->Density();

  double homogdens = V_ * density;

  double homogdensity = params.get<double>("homogdens", 0.0);
  params.set("homogdens", homogdensity+homogdens);

  return;
}


/*----------------------------------------------------------------------*
 |  Read restart on the microscale                              lw 05/08|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::NStet::nstet_read_restart_multi()
{
  const int gp = 0; // there is only one Gauss point

  RefCountPtr<MAT::Material> mat = Material();

  if (mat->MaterialType() == INPAR::MAT::m_struct_multiscale)
  {
    MAT::MicroMaterial* micro = static_cast <MAT::MicroMaterial*>(mat.get());
    int eleID = Id();
    bool eleowner = false;
    if (DRT::Problem::Instance()->Dis(genprob.numsf,0)->Comm().MyPID()==Owner()) eleowner = true;

    micro->ReadRestart(gp, eleID, eleowner);
  }

  return;
}

#endif
#endif
