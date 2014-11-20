/*----------------------------------------------------------------------*/
/*!
\file elasthyper.cpp
\brief
This file contains the hyperelastic toolbox. It allows summing up several summands
of several types (isotropic or anisotropic, splitted or not) to build a hyperelastic
strain energy function.

The input line should read
MAT 0   MAT_ElastHyper   NUMMAT 0 MATIDS  DENS 0 GAMMA 0 INIT_MODE -1

<pre>
Maintainer: Burkhard Bornemann
            bornemann@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15237
</pre>
*/

/*----------------------------------------------------------------------*/

#include "elasthyper.H"
#include "../drt_lib/standardtypes_cpp.H"
#include "../drt_matelast/elast_summand.H"
#include "../linalg/linalg_utils.H"
#include "../drt_lib/drt_linedefinition.H"
#include "../drt_lib/drt_globalproblem.H"
#include "../drt_mat/matpar_bundle.H"
#include "../drt_mat/material_service.H"

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
MAT::PAR::ElastHyper::ElastHyper(
  Teuchos::RCP<MAT::PAR::Material> matdata
  )
: Parameter(matdata),
  nummat_(matdata->GetInt("NUMMAT")),
  matids_(matdata->Get<std::vector<int> >("MATIDS")),
  density_(matdata->GetDouble("DENS"))
{
  // check if sizes fit
  if (nummat_ != (int)matids_->size())
    dserror("number of materials %d does not fit to size of material vector %d", nummat_, matids_->size());
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<MAT::Material> MAT::PAR::ElastHyper::CreateMaterial()
{
  return Teuchos::rcp(new MAT::ElastHyper(this));
}


MAT::ElastHyperType MAT::ElastHyperType::instance_;


DRT::ParObject* MAT::ElastHyperType::Create( const std::vector<char> & data )
{
  MAT::ElastHyper* elhy = new MAT::ElastHyper();
  elhy->Unpack(data);

  return elhy;
}


/*----------------------------------------------------------------------*
 |  initialise static arrays                                 bborn 08/09|
 *----------------------------------------------------------------------*/
// 6-Voigt C-index                              0 1 2  3 4 5
const int MAT::ElastHyper::VOIGT6ROW_[6] = {0,1,2, 0,1,2};
const int MAT::ElastHyper::VOIGT6COL_[6] = {0,1,2, 1,2,0};

// tensor indices ij = 11, 12, 13, 21, 22, 23, 31, 32, 33
// C indices           00, 01, 02, 10, 11, 12, 20, 21, 22
// Access : 3*i+j
// 6-Voigt C-indices    0   3   5   3   1   4   5   4   2
const int MAT::ElastHyper::VOIGT3X3SYM_[9] = {0,3,5, 3,1,4, 5,4,2};


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
MAT::ElastHyper::ElastHyper()
  : params_(NULL),
    potsum_(0)

{
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
MAT::ElastHyper::ElastHyper(MAT::PAR::ElastHyper* params)
  : params_(params),
    potsum_(0)

{
  // make sure the referenced materials in material list have quick access parameters
  std::vector<int>::const_iterator m;
  for (m=params_->matids_->begin(); m!=params_->matids_->end(); ++m)
  {
    const int matid = *m;
    Teuchos::RCP<MAT::ELASTIC::Summand> sum = MAT::ELASTIC::Summand::Factory(matid);
    if (sum == Teuchos::null) dserror("Failed to allocate");
    potsum_.push_back(sum);
  }
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::Pack(DRT::PackBuffer& data) const
{
  DRT::PackBuffer::SizeMarker sm( data );
  sm.Insert();

  // pack type of this instance of ParObject
  int type = UniqueParObjectId();
  AddtoPack(data,type);
  // matid
  int matid = -1;
  if (params_ != NULL) matid = params_->Id();  // in case we are in post-process mode
  AddtoPack(data,matid);
  AddtoPack(data,isoprinc_);
  AddtoPack(data,isomod_);
  AddtoPack(data,anisoprinc_);
  AddtoPack(data,anisomod_);
  AddtoPack(data,isomodvisco_);
  AddtoPack(data,viscogenmax_);

  if (params_ != NULL) // summands are not accessible in postprocessing mode
  {
    // loop map of associated potential summands
    for (unsigned int p=0; p<potsum_.size(); ++p)
    {
     potsum_[p]->PackSummand(data);
    }
  }
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::Unpack(const std::vector<char>& data)
{
  // make sure we have a pristine material
  params_ = NULL;
  potsum_.clear();

  isoprinc_ = false;
  isomod_ = false;
  anisoprinc_ = false;
  anisomod_ = false;
  isomodvisco_ = false;
  viscogenmax_ = false;

  std::vector<char>::size_type position = 0;
  // extract type
  int type = 0;
  ExtractfromPack(position,data,type);
  if (type != UniqueParObjectId()) dserror("wrong instance type data");

  // matid and recover params_
  int matid;
  ExtractfromPack(position,data,matid);
  if (DRT::Problem::Instance()->Materials() != Teuchos::null)
  {
    if (DRT::Problem::Instance()->Materials()->Num() != 0)
    {
      const unsigned int probinst = DRT::Problem::Instance()->Materials()->GetReadFromProblem();
      MAT::PAR::Parameter* mat = DRT::Problem::Instance(probinst)->Materials()->ParameterById(matid);
      if (mat->Type() == MaterialType())
        params_ = static_cast<MAT::PAR::ElastHyper*>(mat);
      else
        dserror("Type of parameter material %d does not fit to calling type %d", mat->Type(), MaterialType());
    }
  }

  int isoprinc;
  int isomod;
  int anisoprinc;
  int anisomod;
  int isomodvisco;
  int viscogenmax;

  ExtractfromPack(position,data,isoprinc);
  ExtractfromPack(position,data,isomod);
  ExtractfromPack(position,data,anisoprinc);
  ExtractfromPack(position,data,anisomod);
  ExtractfromPack(position,data,isomodvisco);
  ExtractfromPack(position,data,viscogenmax);

  if (isoprinc != 0) isoprinc_ = true;
  if (isomod != 0) isomod_ = true;
  if (anisoprinc != 0) anisoprinc_ = true;
  if (anisomod != 0) anisomod_ = true;
  if (isomodvisco != 0) isomodvisco_ = true;
  if (viscogenmax != 0) viscogenmax_ = true;

  if (params_ != NULL) // summands are not accessible in postprocessing mode
  {
    // make sure the referenced materials in material list have quick access parameters
    std::vector<int>::const_iterator m;
    for (m=params_->matids_->begin(); m!=params_->matids_->end(); ++m)
    {
      const int matid = *m;
      Teuchos::RCP<MAT::ELASTIC::Summand> sum = MAT::ELASTIC::Summand::Factory(matid);
      if (sum == Teuchos::null) dserror("Failed to allocate");
      potsum_.push_back(sum);
    }

    // loop map of associated potential summands
    for (unsigned int p=0; p<potsum_.size(); ++p)
    {
     potsum_[p]->UnpackSummand(data,position);
    }
    // in the postprocessing mode, we do not unpack everything we have packed
    // -> position check cannot be done in this case
    if (position != data.size())
      dserror("Mismatch in size of data %d <-> %d",data.size(),position);
  }
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
int MAT::ElastHyper::MatID(
  const unsigned index
) const
{
  if ((int)index < params_->nummat_)
    return params_->matids_->at(index);
  else
  {
    dserror("Index too large");
    return -1;
  }
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
double MAT::ElastHyper::ShearMod() const
{
  // principal coefficients
  bool haveshearmod = false;
  double shearmod = 0.0;
  {
    // loop map of associated potential summands
    for (unsigned int p=0; p<potsum_.size(); ++p)
    {
     potsum_[p]->AddShearMod(haveshearmod,shearmod);
    }
  }

  if (haveshearmod)
  {
    return shearmod;
  }
  else
  {
    dserror("Cannot provide shear modulus equivalent");
    return -1.0;
  }
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::SetupAAA(Teuchos::ParameterList& params)
{
  // loop map of associated potential summands
  for (unsigned int p=0; p<potsum_.size(); ++p)
  {
    potsum_[p]->SetupAAA(params);
  }

  return;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::Setup(int numgp, DRT::INPUT::LineDefinition* linedef)
{
  // Setup summands
  for (unsigned int p=0; p<potsum_.size(); ++p)
  {
    potsum_[p]->Setup(linedef);
  }

  // find out which formulations are used

  isoprinc_ = false ;
  isomod_ = false ;
  anisoprinc_ = false ;
  anisomod_ = false;
  isomodvisco_ = false;
  viscogenmax_ = false;

  for (unsigned int p=0; p<potsum_.size(); ++p)
  {
    potsum_[p]->SpecifyFormulation(isoprinc_,isomod_,anisoprinc_,anisomod_,isomodvisco_,viscogenmax_);
  }

  return;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::Update()
{
  // loop map of associated potential summands
  for (unsigned int p=0; p<potsum_.size(); ++p)
  {
    potsum_[p]->Update();
  }

  return;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::GetFiberVecs(std::vector<LINALG::Matrix<3,1> >& fibervecs)
{
  if (anisoprinc_ || anisomod_)
  {
    for (unsigned int p=0; p<potsum_.size(); ++p)
    {
      potsum_[p]->GetFiberVecs(fibervecs);
    }
  }
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::EvaluateFiberVecs(
    const double newgamma,
    const LINALG::Matrix<3,3>& locsys,
    const LINALG::Matrix<3,3>& defgrd)
{
  if (anisoprinc_ || anisomod_)
  {
    for (unsigned int p=0; p<potsum_.size(); ++p)
    {
      potsum_[p]->SetFiberVecs(newgamma,locsys,defgrd);
    }
  }
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::InvariantsPrincipal(
    LINALG::Matrix<3,1>& prinv,
    const LINALG::Matrix<6,1>& rcg)
{
  // 1st invariant, trace
  prinv(0) = rcg(0) + rcg(1) + rcg(2);
  // 2nd invariant
  prinv(1) = 0.5*( prinv(0)*prinv(0)
                   - rcg(0)*rcg(0) - rcg(1)*rcg(1) - rcg(2)*rcg(2)
                   - .5*rcg(3)*rcg(3) - .5*rcg(4)*rcg(4) - .5*rcg(5)*rcg(5) );
  // 3rd invariant, determinant
  prinv(2) = rcg(0)*rcg(1)*rcg(2)
    + 0.25 * rcg(3)*rcg(4)*rcg(5)
    - 0.25 * rcg(1)*rcg(5)*rcg(5)
    - 0.25 * rcg(2)*rcg(3)*rcg(3)
    - 0.25 * rcg(0)*rcg(4)*rcg(4);
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::InvariantsModified(
    LINALG::Matrix<3,1>& modinv,  ///< modified invariants
    const LINALG::Matrix<3,1>& prinv  ///< principal invariants
    )
{
  // 1st invariant, trace
  modinv(0) = prinv(0)*std::pow(prinv(2),-1./3.);
  // 2nd invariant
  modinv(1) = prinv(1)*std::pow(prinv(2),-2./3.);
  // J
  modinv(2) = std::pow(prinv(2),1./2.);

  return;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::StretchesPrincipal(
  LINALG::Matrix<3,1>& prstr,
  LINALG::Matrix<3,3>& prdir,
  const LINALG::Matrix<6,1>& rcg
  )
{
  // create right Cauchy-Green 2-tensor
  LINALG::Matrix<3,3> rcgt(false);
  rcgt(0,0) = rcg(0);
  rcgt(1,1) = rcg(1);
  rcgt(2,2) = rcg(2);
  rcgt(0,1) = rcgt(1,0) = 0.5*rcg(3);
  rcgt(1,2) = rcgt(2,1) = 0.5*rcg(4);
  rcgt(2,0) = rcgt(0,2) = 0.5*rcg(5);

  // eigenvalue decomposition
  LINALG::Matrix<3,3> prstr2;  // squared principal stretches
  LINALG::SYEV(rcgt,prstr2,prdir);

  // THE principal stretches
  for (int al=0; al<3; ++al) prstr(al) = std::sqrt(prstr2(al,al));

  // bye
  return;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::StretchesModified(
  LINALG::Matrix<3,1>& modstr,
  const LINALG::Matrix<3,1>& prstr
  )
{
  // determinant of deformation gradient
  const double detdefgrad = prstr(0)*prstr(1)*prstr(2);

  // determine modified principal stretches
  modstr.Update(std::pow(detdefgrad,-1.0/3.0),prstr);

  return;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
bool MAT::ElastHyper::HaveCoefficientsStretchesPrincipal()
{
  // set default
  bool havecoeff = false;

  // loop map of associated potential summands and see
  {
    for (unsigned int p=0; p<potsum_.size(); ++p)
    {
     havecoeff = havecoeff or potsum_[p]->HaveCoefficientsStretchesPrincipal();
    }
  }

  // deliver
  return havecoeff;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
bool MAT::ElastHyper::HaveCoefficientsStretchesModified()
{
  // set default
  bool havecoeff = false;

  // loop map of associated potential summands and see
  {
    for (unsigned int p=0; p<potsum_.size(); ++p)
    {
      havecoeff = havecoeff or potsum_[p]->HaveCoefficientsStretchesModified();
    }
  }

  // deliver
  return havecoeff;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::StrainEnergy(const LINALG::Matrix<6,1>& glstrain,
                                   double& psi)
{
  LINALG::Matrix<6,1> id2(true) ;
  LINALG::Matrix<6,1> rcg(true) ;
  LINALG::Matrix<6,1> scg(true) ;
  LINALG::Matrix<6,1> icg(true) ;
  LINALG::Matrix<6,6> id4(true) ;
  LINALG::Matrix<6,6> id4sharp(true) ;

  LINALG::Matrix<3,1> prinv(true);
  LINALG::Matrix<3,1> modinv(true);

  // evaluate kinematic quantities
  EvaluateKinQuant(glstrain,id2,scg,rcg,icg,id4,id4sharp,prinv,modinv);

  // loop map of associated potential summands
  for (unsigned int p=0; p<potsum_.size(); ++p)
  {
    potsum_[p]->AddStrainEnergy(psi,prinv,modinv);
  }

  return;
}


/*----------------------------------------------------------------------*
 |  Evaluate for GEMM time integration                        popp 11/13|
 *----------------------------------------------------------------------*/
void MAT::ElastHyper::EvaluateGEMM(LINALG::Matrix<MAT::NUM_STRESS_3D,1>* stress,
                                   LINALG::Matrix<MAT::NUM_STRESS_3D,MAT::NUM_STRESS_3D>* cmat,
                                   double* density,
                                   LINALG::Matrix<MAT::NUM_STRESS_3D,1>* glstrain_m,
                                   LINALG::Matrix<MAT::NUM_STRESS_3D,1>* glstrain_new,
                                   LINALG::Matrix<MAT::NUM_STRESS_3D,1>* glstrain_old,
                                   LINALG::Matrix<3,3>* rcg_new,
                                   LINALG::Matrix<3,3>* rcg_old,
                                   const int eleGID)
{
  #ifdef DEBUG
  if (!stress) dserror("No stress vector supplied");
  if (!cmat) dserror("No material tangent matrix supplied");
  if (!glstrain_m) dserror("No GL strains supplied");
  if (!glstrain_new) dserror("No GL strains supplied");
  if (!glstrain_old) dserror("No GL strains supplied");
#endif

  // standard material evaluate call at midpoint t_{n+1/2}
  Teuchos::ParameterList params;
  LINALG::Matrix<3,3> defgrd(true);
  Evaluate(&defgrd,glstrain_m,params,stress,cmat,eleGID);
  *density = Density();

  //**********************************************************************
  // CHECK IF GEMM ALGORITHMIC STRESSES NEED TO BE APPLIED
  //**********************************************************************
  // increment of Cauchy-Green tensor in Voigt notation
  LINALG::Matrix<6,1> M;
  M(0) = (*rcg_new)(0,0) - (*rcg_old)(0,0);
  M(1) = (*rcg_new)(1,1) - (*rcg_old)(1,1);
  M(2) = (*rcg_new)(2,2) - (*rcg_old)(2,2);
  M(3) = (*rcg_new)(0,1) + (*rcg_new)(1,0) - (*rcg_old)(0,1) - (*rcg_old)(1,0);
  M(4) = (*rcg_new)(1,2) + (*rcg_new)(2,1) - (*rcg_old)(1,2) - (*rcg_old)(2,1);
  M(5) = (*rcg_new)(0,2) + (*rcg_new)(2,0) - (*rcg_old)(0,2) - (*rcg_old)(2,0);

  // second variant of M in Voigt notation
  LINALG::Matrix<6,1> Mtilde;
  Mtilde(0) = M(0);
  Mtilde(1) = M(1);
  Mtilde(2) = M(2);
  Mtilde(3) = 0.5 * M(3);
  Mtilde(4) = 0.5 * M(4);
  Mtilde(5) = 0.5 * M(5);

  // dot product M * Mtilde
  double Mb = M(0)*Mtilde(0) + M(1)*Mtilde(1) + M(2)*Mtilde(2)
            + M(3)*Mtilde(3) + M(4)*Mtilde(4) + M(5)*Mtilde(5);

  // second term in algorithmic stresses only exists if Mb > 0
  // see: O. Gonzalez, Exact energy and momentum conserving algorithms for
  // general models in nonlinear elasticity, CMAME, 190(2000), pp. 1763-1783
  if (Mb < 1.0e-12) return;

  //**********************************************************************
  // COMPUTE GEMM ALGORITHMIC STRESSES
  //**********************************************************************
  // some helper definitions
  LINALG::Matrix<6,1> vecid(true);
  for (int k=0;k<6;++k) vecid(k) = 1.0;
  LINALG::Matrix<6,6> halfid(true);
  for (int k=0;k<3;++k) halfid(k,k) = 1.0;
  for (int k=3;k<6;++k) halfid(k,k) = 0.5;

  // strain energy function at t_{n+1} and t_{n}
  double psi = 0.0;
  double psio = 0.0;
  StrainEnergy(*glstrain_new,psi);
  StrainEnergy(*glstrain_old,psio);

  // derivative of strain energy function dpsi = 0.5*stress
  // double contraction dpsi : M
  double dpsiM = 0.5*(*stress)(0)*M(0) + 0.5*(*stress)(1)*M(1) + 0.5*(*stress)(2)*M(2)
               + 0.5*(*stress)(3)*M(3) + 0.5*(*stress)(4)*M(4) + 0.5*(*stress)(5)*M(5);

  // factor for algorithmic stresses
  double fac = 2.0 * ((psi - psio - dpsiM) / Mb);

  // algorithmic stresses
  LINALG::Matrix<6,1> algstress(true);
  algstress.Update(fac,Mtilde,1.0);

  //**********************************************************************
  // COMPUTE GEMM ALGORITHMIC MATERIAL TENSOR
  //**********************************************************************
  // algorithmic material tensor requires stresses at t_{n+1}
  LINALG::Matrix<6,1> stressnew(true);
  LINALG::Matrix<6,6> cmatnew(true);
  Evaluate(&defgrd,glstrain_new,params,&stressnew,&cmatnew,eleGID);

  // initialize algorithmic material tensor
  LINALG::Matrix<6,6> algcmat(true);

  // part 1 (derivative of Mtilde)
  algcmat.Update(4.0*fac,halfid,1.0);

  // part 2a (derivative of strain energy in fac)
  LINALG::Matrix<6,1> dfac(true);
  dfac.Update(2.0/Mb,stressnew,1.0);

  // part 2b (derivative of dpsiM in fac)
  LINALG::Matrix<6,1> tmp(true);
  tmp.Multiply(*cmat,M);
  dfac.Update(-0.5/Mb,tmp,1.0);
  dfac.Update(-2.0/Mb,*stress,1.0);

  // part 2c (derivative of Mb in fac)
  tmp.Multiply(halfid,M);
  dfac.Update(-4.0*(psi-psio-dpsiM)/(Mb*Mb),tmp,1.0);
  dfac.Update(-4.0*(psi-psio-dpsiM)/(Mb*Mb),Mtilde,1.0);

  // part 2 (derivative of fac, put together parts 2a,2b and 2c)
  LINALG::Matrix<6,6> tmpmat(true);
  tmpmat.MultiplyNT(2.0,Mtilde,dfac);
  algcmat.Update(1.0,tmpmat,1.0);

  //**********************************************************************
  // EXTEND ORIGINAL STRESSES / CMAT WITH GEMM CONTRIBUTIONS
  //**********************************************************************
  stress->Update(1.0,algstress,1.0);
  cmat->Update(1.0,algcmat,1.0);

  return;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::Evaluate(const LINALG::Matrix<3,3>* defgrd,
                               const LINALG::Matrix<6,1>* glstrain,
                               Teuchos::ParameterList& params,
                               LINALG::Matrix<6,1>* stress,
                               LINALG::Matrix<6,6>* cmat,
                               const int eleGID)
{

  LINALG::Matrix<6,1> id2(true) ;
  LINALG::Matrix<6,1> rcg(true) ;
  LINALG::Matrix<6,1> scg(true) ;
  LINALG::Matrix<6,1> icg(true) ;
  LINALG::Matrix<6,6> id4(true) ;
  LINALG::Matrix<6,6> id4sharp(true) ;

  LINALG::Matrix<3,1> prinv(true);
  LINALG::Matrix<3,1> modinv(true);

  LINALG::Matrix<3,1> gamma(true);
  LINALG::Matrix<8,1> delta(true);
  LINALG::Matrix<3,1> modgamma(true);
  LINALG::Matrix<5,1> moddelta(true);

  EvaluateKinQuant(*glstrain,id2,scg,rcg,icg,id4,id4sharp,prinv,modinv);
  EvaluateGammaDelta(prinv,modinv,gamma,delta,modgamma,moddelta);

  // blank resulting quantities
  // ... even if it is an implicit law that cmat is zero upon input
  stress->Clear();
  cmat->Clear();

  // build stress response and elasticity tensor
  // for potentials based on principal invariants
  if (isoprinc_)
  {
    LINALG::Matrix<NUM_STRESS_3D,1> stressisoprinc(true) ;
    LINALG::Matrix<NUM_STRESS_3D,NUM_STRESS_3D> cmatisoprinc(true) ;
    EvaluateIsotropicPrinc(stressisoprinc,cmatisoprinc,scg,id2,icg,id4sharp,gamma,delta);
    stress->Update(1.0, stressisoprinc, 1.0);
    cmat->Update(1.0,cmatisoprinc,1.0);
  }

  if (isomod_)
  {
    LINALG::Matrix<NUM_STRESS_3D,1> stressisomodiso(true) ;
    LINALG::Matrix<NUM_STRESS_3D,NUM_STRESS_3D> cmatisomodiso(true);
    LINALG::Matrix<NUM_STRESS_3D,1> stressisomodvol(true) ;
    LINALG::Matrix<NUM_STRESS_3D,NUM_STRESS_3D> cmatisomodvol(true) ;
    EvaluateIsotropicMod(stressisomodiso,stressisomodvol,cmatisomodiso,cmatisomodvol,rcg,id2,icg,id4,id4sharp,modinv,prinv,modgamma,moddelta);
    stress->Update(1.0, stressisomodiso, 1.0);
    stress->Update(1.0, stressisomodvol, 1.0);
    cmat->Update(1.0,cmatisomodiso,1.0);
    cmat->Update(1.0,cmatisomodvol,1.0);
  }

  /*----------------------------------------------------------------------*/
  // coefficients in principal stretches
  const bool havecoeffstrpr = HaveCoefficientsStretchesPrincipal();
  const bool havecoeffstrmod = HaveCoefficientsStretchesModified();
  if (havecoeffstrpr or havecoeffstrmod) {
    ResponseStretches(*cmat,*stress,rcg,havecoeffstrpr,havecoeffstrmod);
  }

  /*----------------------------------------------------------------------*/
  //Do all the anisotropic stuff!
  if (anisoprinc_)
  {
      LINALG::Matrix<NUM_STRESS_3D,1> stressanisoprinc(true) ;
      LINALG::Matrix<NUM_STRESS_3D,NUM_STRESS_3D> cmatanisoprinc(true) ;
      EvaluateAnisotropicPrinc(stressanisoprinc,cmatanisoprinc,rcg,params);
      stress->Update(1.0, stressanisoprinc, 1.0);
      cmat->Update(1.0, cmatanisoprinc, 1.0);
  }

  if (anisomod_)
  {
      LINALG::Matrix<NUM_STRESS_3D,1> stressanisomod(true) ;
      LINALG::Matrix<NUM_STRESS_3D,NUM_STRESS_3D> cmatanisomod(true) ;
      EvaluateAnisotropicMod(stressanisomod,cmatanisomod,rcg,icg,prinv);
      stress->Update(1.0, stressanisomod, 1.0);
      cmat->Update(1.0, cmatanisomod, 1.0);
  }

  return ;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::EvaluateKinQuant(
    const LINALG::Matrix<6,1>& glstrain,
    LINALG::Matrix<6,1>& id2,
    LINALG::Matrix<6,1>& scg,
    LINALG::Matrix<6,1>& rcg,
    LINALG::Matrix<6,1>& icg,
    LINALG::Matrix<6,6>& id4,
    LINALG::Matrix<6,6>& id4sharp,
    LINALG::Matrix<3,1>& prinv,
    LINALG::Matrix<3,1>& modinv)

{
  // build Cartesian identity 2-tensor I_{AB}
  for (int i=0; i<3; i++) id2(i) = 1.0;

  // right Cauchy-Green Tensor  C_{AB} = 2 * E_{AB} + I_{AB}
  // REMARK: strain-like 6-Voigt vector
  rcg.Update(2.0,glstrain,1.0);
  rcg.Update(1.0, id2, 1.0);

  // 'contra-variant' right Cauchy-Green Tensor C^{AB}
  // REMARK: stress-like 6-Voigt vector of right CG
  scg.Update(1.0,rcg,1.0);
  for (int i=3; i<6; i++) scg(i) *= 0.5;

  // principal invariants of right Cauchy-Green strain
  InvariantsPrincipal(prinv,rcg);

  // invert right Cauchy-Green tensor
  // REMARK: stress-like 6-Voigt vector
  {
    icg(0) = ( rcg(1)*rcg(2) - 0.25*rcg(4)*rcg(4) ) / prinv(2);
    icg(1) = ( rcg(0)*rcg(2) - 0.25*rcg(5)*rcg(5) ) / prinv(2);
    icg(2) = ( rcg(0)*rcg(1) - 0.25*rcg(3)*rcg(3) ) / prinv(2);
    icg(3) = ( 0.25*rcg(5)*rcg(4) - 0.5*rcg(3)*rcg(2) ) / prinv(2);
    icg(4) = ( 0.25*rcg(3)*rcg(5) - 0.5*rcg(0)*rcg(4) ) / prinv(2);
    icg(5) = ( 0.25*rcg(3)*rcg(4) - 0.5*rcg(5)*rcg(1) ) / prinv(2);
  }

  // set Cartesian identity 4-tensor in 6-Voigt matrix notation
  // this is fully 'contra-variant' identity tensor, ie I^{ABCD}
  // REMARK: rows are stress-like 6-Voigt
  //         columns are stress-like 6-Voigt
  for (int i=0; i<3; i++) id4sharp(i,i) = 1.0;
  for (int i=3; i<6; i++) id4sharp(i,i) = 0.5;

  // set Cartesian identity 4-tensor in 6x6-matrix notation (stress-like)
  // this is a 'mixed co- and contra-variant' identity 4-tensor, ie I^{AB}_{CD}
  // REMARK: rows are stress-like 6-Voigt
  //         columns are strain-like 6-Voigt
  for (int i=0; i<6; i++) id4(i,i) = 1.0;

  // modified invariants of right Cauchy-Green strain
  InvariantsModified(modinv,prinv);
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::EvaluateGammaDelta(
    LINALG::Matrix<3,1> prinv,
    LINALG::Matrix<3,1> modinv,
    LINALG::Matrix<3,1>& gamma,
    LINALG::Matrix<8,1>& delta,
    LINALG::Matrix<3,1>& modgamma,
    LINALG::Matrix<5,1>& moddelta
    )

{
  // principal coefficients
  if (isoprinc_)
  {
    // loop map of associated potential summands
    for (unsigned int p=0; p<potsum_.size(); ++p)
    {
      potsum_[p]->AddCoefficientsPrincipal(gamma,delta,prinv);
    }
  }

  // modified coefficients
  if (isomod_)
  {

    // loop map of associated potential summands
    for (unsigned int p=0; p<potsum_.size(); ++p)
    {
      potsum_[p]->AddCoefficientsModified(modgamma,moddelta,modinv);
    }
  }
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::EvaluateIsotropicPrinc(
    LINALG::Matrix<6,1>& stressisoprinc,
    LINALG::Matrix<6,6>& cmatisoprinc,
    LINALG::Matrix<6,1> scg,
    LINALG::Matrix<6,1> id2,
    LINALG::Matrix<6,1> icg,
    LINALG::Matrix<6,6> id4sharp,
    LINALG::Matrix<3,1> gamma,
    LINALG::Matrix<8,1> delta
    )
{

  // 2nd Piola Kirchhoff stresses
  stressisoprinc.Update(gamma(0), id2, 1.0);
  stressisoprinc.Update(gamma(1), scg, 1.0);
  stressisoprinc.Update(gamma(2), icg, 1.0);

  // constitutive tensor
  // contribution: Id \otimes Id
  cmatisoprinc.MultiplyNT(delta(0), id2, id2, 1.0);
  // contribution: Id \otimes C + C \otimes Id
  cmatisoprinc.MultiplyNT(delta(1), id2, scg, 1.0);
  cmatisoprinc.MultiplyNT(delta(1), scg, id2, 1.0);
  // contribution: Id \otimes Cinv + Cinv \otimes Id
  cmatisoprinc.MultiplyNT(delta(2), id2, icg, 1.0);
  cmatisoprinc.MultiplyNT(delta(2), icg, id2, 1.0);
  // contribution: C \otimes C
  cmatisoprinc.MultiplyNT(delta(3), scg, scg, 1.0);
  // contribution: C \otimes Cinv + Cinv \otimes C
  cmatisoprinc.MultiplyNT(delta(4), scg, icg, 1.0);
  cmatisoprinc.MultiplyNT(delta(4), icg, scg, 1.0);
  // contribution: Cinv \otimes Cinv
  cmatisoprinc.MultiplyNT(delta(5), icg, icg, 1.0);
  // contribution: Cinv \odot Cinv
  AddtoCmatHolzapfelProduct(cmatisoprinc, icg, delta(6));
  // contribution: Id4^#
  cmatisoprinc.Update(delta(7), id4sharp, 1.0);

  return ;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::EvaluateIsotropicMod(
    LINALG::Matrix<6,1>& stressisomodiso,
    LINALG::Matrix<6,1>& stressisomodvol,
    LINALG::Matrix<6,6>& cmatisomodiso,
    LINALG::Matrix<6,6>& cmatisomodvol,
    LINALG::Matrix<6,1> rcg,
    LINALG::Matrix<6,1> id2,
    LINALG::Matrix<6,1> icg,
    LINALG::Matrix<6,6> id4,
    LINALG::Matrix<6,6> id4sharp,
    LINALG::Matrix<3,1> modinv,
    LINALG::Matrix<3,1> prinv,
    LINALG::Matrix<3,1> modgamma,
    LINALG::Matrix<5,1> moddelta
    )
{
  // define necessary variables
  const double modscale = std::pow(prinv(2),-1./3.);
  // modified right Cauchy-Green
  LINALG::Matrix<6,1> modrcg(true);
  modrcg.Update(modscale,rcg);
  LINALG::Matrix<6,1> modscg(modrcg);
  for (int i=3; i<6; ++i) modscg(i) *=.5;
  LINALG::Matrix<6,1> scg(rcg);
  for (int i=3; i<6; ++i) scg(i)*=.5;

  // 2nd Piola Kirchhoff stresses

  // isochoric contribution
  LINALG::Matrix<6,1> modstress(true);
  modstress.Update(modgamma(0), id2);
  modstress.Update(modgamma(1), modscg, 1.0);
  // build 4-tensor for projection as 6x6 tensor
  LINALG::Matrix<6,6> Projection;
  Projection.MultiplyNT(1./3., icg, rcg);
  Projection.Update(1.0, id4, -1.0);
  // isochoric stress
  LINALG::Matrix<6,1> isostress(true);
  stressisomodiso.MultiplyNN(modscale,Projection,modstress,1.0);

  // volumetric contribution
  stressisomodvol.Update(modgamma(2)*modinv(2), icg, 1.0);

  // constitutive tensor

  //isochoric contribution
  // modified constitutive tensor
  LINALG::Matrix<6,6> modcmat(true);
  LINALG::Matrix<6,6> modcmat2(true);
  // contribution: Id \otimes Id
  modcmat.MultiplyNT(moddelta(0), id2, id2);
  // contribution: Id \otimes C + C \otimes Id
  modcmat.MultiplyNT(moddelta(1), id2, modscg, 1.0);
  modcmat.MultiplyNT(moddelta(1), modscg, id2, 1.0);
  // contribution: C \otimes C
  modcmat.MultiplyNT(moddelta(2), modscg, modscg, 1.0);
  // contribution: Id4^#
  modcmat.Update(moddelta(3), id4sharp, 1.0);
  //scaling
  modcmat.Scale(std::pow(modinv(2),-4./3.));
  //contribution: P:modC:P
  modcmat2.MultiplyNN(Projection,modcmat);
  cmatisomodiso.MultiplyNT(1.0,modcmat2,Projection,1.0);
  // contribution: 2/3*Tr(J^(-2/3)modstress) (Cinv \odot Cinv - 1/3 Cinv \otimes Cinv)
  modcmat.Clear();
  modcmat.MultiplyNT(-1.0/3.0,icg,icg);
  AddtoCmatHolzapfelProduct(modcmat, icg, 1.0);
  cmatisomodiso.Update(2./3.*std::pow(modinv(2),-2./3.)*modstress.Dot(rcg),modcmat,1.0);
  //contribution: -2/3 (Cinv \otimes S_iso + S_iso \otimes Cinv)
  cmatisomodiso.MultiplyNT(-2./3.,icg,stressisomodiso,1.0);
  cmatisomodiso.MultiplyNT(-2./3.,stressisomodiso,icg,1.0);

  //volumetric contribution
  //contribution: 2 \tilde p Cinv \otimes Cinv
  cmatisomodvol.MultiplyNT(modinv(2)* moddelta(4),icg,icg,1.0);
  //contribution: -2 J*p Cinv \odot Cinv
  AddtoCmatHolzapfelProduct(cmatisomodvol, icg, -2*modinv(2)*modgamma(2));

  return ;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::EvaluateAnisotropicPrinc(
    LINALG::Matrix<6,1>& stressanisoprinc,
    LINALG::Matrix<6,6>& cmatanisoprinc,
    LINALG::Matrix<6,1> rcg,
    Teuchos::ParameterList& params
    )
{
  // loop map of associated potential summands
  for (unsigned int p=0; p<potsum_.size(); ++p)
  {
    potsum_[p]->AddStressAnisoPrincipal(rcg,cmatanisoprinc,stressanisoprinc,params);
  }

  return ;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::EvaluateAnisotropicMod(
    LINALG::Matrix<6,1>& stressanisomod,
    LINALG::Matrix<6,6>& cmatanisomod,
    LINALG::Matrix<6,1> rcg,
    LINALG::Matrix<6,1> icg,
    LINALG::Matrix<3,1> prinv
    )
{

  // loop map of associated potential summands
  for (unsigned int p=0; p<potsum_.size(); ++p)
  {
    potsum_[p]->AddStressAnisoModified(rcg,icg,cmatanisomod,stressanisomod,prinv(2));
  }

  return ;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::ResponseStretches(
  LINALG::Matrix<6,6>& cmat,
  LINALG::Matrix<6,1>& stress,
  const LINALG::Matrix<6,1> rcg,
  const bool& havecoeffstrpr,
  const bool& havecoeffstrmod
  )
{
  // get principal stretches and directions
  LINALG::Matrix<3,1> prstr;
  LINALG::Matrix<3,3> prdir;
  StretchesPrincipal(prstr,prdir,rcg);
  // modified stretches
  LINALG::Matrix<3,1> modstr;
  StretchesModified(modstr,prstr);
  // determinant of deformation gradient
  const double detdefgrad = prstr(0)*prstr(1)*prstr(2);

  // get coefficients
  LINALG::Matrix<3,1> gamma_(true);
  LINALG::Matrix<6,1> delta_(true);
  if (havecoeffstrpr) {
    // loop map of associated potential summands
    for (unsigned int p=0; p<potsum_.size(); ++p)
    {
      potsum_[p]->AddCoefficientsStretchesPrincipal(gamma_,delta_,prstr);
    }
  }
  if (havecoeffstrmod) {
    // reciprocal of cubic root of determinant of deformation gradient (convenience)
    const double detdefgrad13 = std::pow(detdefgrad,-1.0/3.0);
    // retrieve coefficients with respect to modified principal stretches
    LINALG::Matrix<3,1> modgamma(true);
    LINALG::Matrix<6,1> moddelta(true);
    {
      // loop map of associated potential summands
      for (unsigned int p=0; p<potsum_.size(); ++p)
      {
        potsum_[p]->AddCoefficientsStretchesModified(modgamma,moddelta,modstr);
      }
    }
    // convert modified coefficients to oridinary counterparts
    //
    // derivatives of modified pr. stretches WRT pr. stretches
    LINALG::Matrix<3,3> modbypr(false);
    for (int al=0; al<3; ++al) {
      for (int be=0; be<3; ++be) {
        modbypr(al,be) = -modstr(al)/modstr(be);
      }
      modbypr(al,al) += 3.0;
    }
    modbypr.Scale(detdefgrad13/3.0);
    // determine unmodified coefficients gamma and add them
    gamma_.MultiplyTN(1.0,modbypr,modgamma,1.0);
    // determine unmodified coefficients delta and add them
    //
    // rewrite mod.coeff. as 2-tensor
    LINALG::Matrix<3,3> moddeltat(false);
    moddeltat(0,0) = moddelta(0);
    moddeltat(1,1) = moddelta(1);
    moddeltat(2,2) = moddelta(2);
    moddeltat(0,1) = moddeltat(1,0) = moddelta(3);
    moddeltat(1,2) = moddeltat(2,1) = moddelta(4);
    moddeltat(2,0) = moddeltat(0,2) = moddelta(5);
    // Psi_{,barlam barlam} barlam_{,lam} barlam_{,lam}
    LINALG::Matrix<3,3> aux(false);
    aux.MultiplyTN(modbypr,moddeltat);
    LINALG::Matrix<3,3> deltat(false);
    deltat.MultiplyNN(aux,modbypr);
    // Psi_{,barlam} barlam_{,lam lam}
    for (int be=0; be<3; ++be) {
      for (int ga=0; ga<3; ++ga) {
        double deltat_bega = 0.0;
        for (int al=0; al<3; ++al) {
          deltat_bega += -modgamma(al)*modbypr(al,be)/(3.0*prstr(ga));
          if (ga==al)
            deltat_bega += -modgamma(al)*detdefgrad13/(3.0*prstr(be));
          if (be==ga)
            deltat_bega += modgamma(al)*detdefgrad13*prstr(al)/(3.0*prstr(be)*prstr(be));
        }
        deltat(be,ga) += deltat_bega;
      }
    }
    // add to delta
    // Psi_{lam lam} = Psi_{,barlam barlam} barlam_{,lam} barlam_{,lam}
    //               + Psi_{,barlam} barlam_{,lam lam}
    delta_(0) += deltat(0,0);
    delta_(1) += deltat(1,1);
    delta_(2) += deltat(2,2);
    delta_(3) += deltat(0,1);
    delta_(4) += deltat(1,2);
    delta_(5) += deltat(2,0);
  }

  // principal 2nd Piola--Kirchhoff stress tensor, cf [1] Eq (6.47)
  LINALG::Matrix<3,1> prsts(true);
  for (int al=0; al<3; ++al) {
    // PK2 principal stresses
    prsts(al) = gamma_(al)/prstr(al);
    // PK2 tensor in Voigt notation
    stress(0) += prsts(al)*prdir(0,al)*prdir(0,al);  // S^11
    stress(1) += prsts(al)*prdir(1,al)*prdir(1,al);  // S^22
    stress(2) += prsts(al)*prdir(2,al)*prdir(2,al);  // S^33
    stress(3) += prsts(al)*prdir(0,al)*prdir(1,al);  // S^12
    stress(4) += prsts(al)*prdir(1,al)*prdir(2,al);  // S^23
    stress(5) += prsts(al)*prdir(2,al)*prdir(0,al);  // S^31
  }

  // integration factor prfact_{al be}
  LINALG::Matrix<6,1> prfact1(true);
  LINALG::Matrix<6,1> prfact2(true);
  for (int albe=0; albe<6; ++albe) {
    const int al = VOIGT6ROW_[albe];
    const int be = VOIGT6COL_[albe];
    double prfact1_albe = delta_(albe)/(prstr(al)*prstr(be));
    if (albe<3) prfact1_albe -= gamma_(al)/(prstr(be)*prstr(al)*prstr(al));
    prfact1(albe) = prfact1_albe;
    if (al != be) {
      if (fabs(prstr(al)-prstr(be)) < EPS6)
        prfact2(albe) = (prfact1(be) - prfact1(albe))/2.0;
      else
        prfact2(albe) = (prsts(be)-prsts(al))/(prstr(be)*prstr(be)-prstr(al)*prstr(al));
    }
  }

  // add elasticity 4-tensor, cf Holzapfel [1] Eq (6.180),(6.196)
  for (int kl=0; kl<6; ++kl) {
    const int k = VOIGT6ROW_[kl];
    const int l = VOIGT6COL_[kl];
    for (int ij=0; ij<6; ++ij) {
      const int i = VOIGT6ROW_[ij];
      const int j = VOIGT6COL_[ij];
      double c_ijkl = 0.0;
      for (int albe=0; albe<6; ++albe) {
        const int al = VOIGT6ROW_[albe];
        const int be = VOIGT6COL_[albe];
        const double fact1 = prfact1(albe);
        c_ijkl += fact1*prdir(i,al)*prdir(j,al)*prdir(k,be)*prdir(l,be);
        if (albe>=3) { // al!=be
          c_ijkl += fact1*prdir(i,be)*prdir(j,be)*prdir(k,al)*prdir(l,al);
          const double fact2 = prfact2(albe);
          c_ijkl += fact2*prdir(i,al)*prdir(j,be)*prdir(k,al)*prdir(l,be)
                  + fact2*prdir(i,al)*prdir(j,be)*prdir(k,be)*prdir(l,al)
                  + fact2*prdir(i,be)*prdir(j,al)*prdir(k,be)*prdir(l,al)
                  + fact2*prdir(i,be)*prdir(j,al)*prdir(k,al)*prdir(l,be);
        }
      }
      cmat(ij,kl) += c_ijkl;
    }
  }
  // ready
  return;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ElastHyper::VisNames(std::map<std::string,int>& names)
{
  if (AnisotropicPrincipal() or AnisotropicModified())
  {
    std::vector<LINALG::Matrix<3,1> > fibervecs;
    GetFiberVecs(fibervecs);
    int vissize = fibervecs.size();
    std::string fiber;
    for (int i = 0; i < vissize; i++)
    {
      std::ostringstream s;
      s << "Fiber" << i+1;
      fiber = s.str();
      names[fiber] = 3; // 3-dim vector
    }
  }
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
bool MAT::ElastHyper::VisData(const std::string& name, std::vector<double>& data, int numgp, int eleID)
{
  if (AnisotropicPrincipal() or AnisotropicModified())
  {
    std::vector<LINALG::Matrix<3,1> > fibervecs;
    GetFiberVecs(fibervecs);
    int vissize = fibervecs.size();
    for (int i = 0; i < vissize; i++)
    {
      std::ostringstream s;
      s << "Fiber" << i+1;
      std::string fiber;
      fiber = s.str();
      if (name == fiber)
      {
        if ((int)data.size()!=3)
          dserror("size mismatch");
        data[0] = fibervecs.at(i)(0);
        data[1] = fibervecs.at(i)(1);
        data[2] = fibervecs.at(i)(2);
      }
    }
    return true;
  }
  return false;
}
