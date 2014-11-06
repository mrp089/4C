/*----------------------------------------------------------------------*/
/*!
\file elast_coupanisopow.cpp
\brief


the input line should read
   MAT 1 CoupAnisoPow C 100 D 2.0 GAMMA 35.0 INIT 0 ADAPT_ANGLE 0

<pre>
Maintainer: Nagler Andreas
            nagler@lnm.mw.tum.de
            089/289 15249
</pre>
*/

/*----------------------------------------------------------------------*/
/* headers */
#include "elast_coupanisopow.H"
#include "../drt_mat/matpar_material.H"
#include "../drt_lib/standardtypes_cpp.H"
#include "../drt_lib/drt_linedefinition.H"

/*----------------------------------------------------------------------*
 |
 *----------------------------------------------------------------------*/
MAT::ELASTIC::PAR::CoupAnisoPow::CoupAnisoPow(
  Teuchos::RCP<MAT::PAR::Material> matdata
  )
: Parameter(matdata),
  k_(matdata->GetDouble("K")),
  d1_(matdata->GetDouble("D1")),
  d2_(matdata->GetDouble("D2")),
  activethres_(matdata->GetDouble("ACTIVETHRES")),
  gamma_(matdata->GetDouble("GAMMA")),
  init_(matdata->GetInt("INIT")),
  adapt_angle_(matdata->GetInt("ADAPT_ANGLE"))
{
}


/*----------------------------------------------------------------------*
 |  Constructor
 *----------------------------------------------------------------------*/
MAT::ELASTIC::CoupAnisoPow::CoupAnisoPow(MAT::ELASTIC::PAR::CoupAnisoPow* params)
  : params_(params)
{
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ELASTIC::CoupAnisoPow::PackSummand(DRT::PackBuffer& data) const
{
  AddtoPack(data,a_);
  AddtoPack(data,A_);
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ELASTIC::CoupAnisoPow::UnpackSummand(
  const std::vector<char>& data,
  std::vector<char>::size_type& position
  )
{
  ExtractfromPack(position,data,a_);
  ExtractfromPack(position,data,A_);
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ELASTIC::CoupAnisoPow::Setup(DRT::INPUT::LineDefinition* linedef)
{
  // path if fibers aren't given in .dat file
  if (params_->init_ == 0)
  {
    // fibers aligned in YZ-plane with gamma around Z in global cartesian cosy
    LINALG::Matrix<3,3> Id(true);
    for (int i=0; i<3; i++)
      Id(i,i) = 1.0;
    SetFiberVecs(-1.0,Id,Id);
  }

  // path if fibers are given in .dat file
  else if (params_->init_ == 1)
  {

    // CIR-AXI-RAD nomenclature
    if (linedef->HaveNamed("RAD") and
        linedef->HaveNamed("AXI") and
        linedef->HaveNamed("CIR"))
    {
      // Read in of data
      LINALG::Matrix<3,3> locsys(true);
      ReadRadAxiCir(linedef, locsys);
      LINALG::Matrix<3,3> Id(true);
      for (int i=0; i<3; i++)
        Id(i,i) = 1.0;
      // final setup of fiber data
      SetFiberVecs(0.0,locsys,Id);
    }

    // FIBER1 nomenclature
    else if ( linedef->HaveNamed("FIBER1") )
    {
      // Read in of data
      ReadFiber(linedef, "FIBER1", a_);
      SetupStructuralTensor(a_,A_);
    }

    // error path
    else
    {
      dserror("Reading of element local cosy for anisotropic materials failed");
    }

  }
  else
    dserror("INIT mode not implemented");
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ELASTIC::CoupAnisoPow::AddStressAnisoPrincipal(
    const LINALG::Matrix<6,1> rcg,
    LINALG::Matrix<6,6>& cmat,
    LINALG::Matrix<6,1>& stress,
    Teuchos::ParameterList& params
)
{
  // load params
  double k=params_->k_;
  double d1=params_->d1_;
  double d2=params_->d2_;
  double activethres=params_->activethres_;

  if (d2<=1.0)
  {
    dserror("exponential factor D2 should be greater than 1.0, since otherwise one can't achieve a stress free reference state");
  }

  // calc invariant I4
  double I4 = 0.0;
  I4 =  A_(0)*rcg(0) + A_(1)*rcg(1) + A_(2)*rcg(2)
      + A_(3)*rcg(3) + A_(4)*rcg(4) + A_(5)*rcg(5);


  double pow_I4_d1   = pow(I4,d1);
  double pow_I4_d1m1 = pow(I4,d1-1.0);
  double pow_I4_d1m2 = pow(I4,d1-2.0);
  // Compute stress and material update
  // Beware that the fiber will be turned off in case of compression
  double gamma = 0.0;
  if (I4 > activethres )
  {
    gamma = 2.0 * k * d2 * d1 * pow_I4_d1m1 * pow( pow_I4_d1 - 1.0 , d2 - 1.0);
  }
  stress.Update(gamma, A_, 1.0);

  double delta = 0.0;
  if (I4 >  activethres)
  {
    delta = 4.0 * k * d2 * (d2-1) * d1*pow_I4_d1m1*d1*pow_I4_d1m1 *pow( pow_I4_d1 - 1.0 ,d2-2.0) +
            4.0 * k * d2 * d1 * (d1-1.0) * pow_I4_d1m2 * pow( pow_I4_d1 - 1.0,d2-1.0);
  }
  cmat.MultiplyNT(delta, A_, A_, 1.0);

}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ELASTIC::CoupAnisoPow::GetFiberVecs(
    std::vector<LINALG::Matrix<3,1> >& fibervecs ///< vector of all fiber vectors
)
{
  fibervecs.push_back(a_);
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ELASTIC::CoupAnisoPow::SetFiberVecs(
    const double newgamma,
    const LINALG::Matrix<3,3> locsys,
    const LINALG::Matrix<3,3> defgrd
)
{
  if ((params_->gamma_<-90) || (params_->gamma_ >90)) dserror("Fiber angle not in [-90,90]");
  //convert
  double gamma = (params_->gamma_*PI)/180.;

  if (params_->adapt_angle_ && newgamma != -1.0)
  {
    if (gamma*newgamma < 0.0)
      gamma = -1.0 * newgamma;
    else
      gamma = newgamma;
  }

  LINALG::Matrix<3,1> ca(true);
  for (int i = 0; i < 3; ++i)
  {
    // a = cos gamma e3 + sin gamma e2
    ca(i) = cos(gamma)*locsys(i,2) + sin(gamma)*locsys(i,1);
  }
  // pull back in reference configuration
  LINALG::Matrix<3,1> a_0(true);
  LINALG::Matrix<3,3> idefgrd(true);
  idefgrd.Invert(defgrd);

  a_0.Multiply(idefgrd,ca);
  a_.Update(1./a_0.Norm2(),a_0);

  SetupStructuralTensor(a_,A_);
}
