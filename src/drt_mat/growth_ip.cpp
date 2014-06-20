/*!----------------------------------------------------------------------
\file growth_ip.cpp
\brief
This file contains routines for an integration point based growth law
example input line
MAT 1 MAT_GROWTH DENS 1.0 IDMATELASTIC 2 STARTTIME 0.2 ENDTIME 100.0 KPLUS 0.5 MPLUS 4.0 KMINUS 0.25 MMINUS 5.0

Here a kinematic integration point based approach of growth is modeled.
For a detailed description see:
- Lubarda, V. & Hoger, A., On the mechanics of solids with a growing mass,
  International Journal of Solids and Structures, 2002, 39, 4627-4664
- Himpel, G.; Kuhl, E.; Menzel, A. & Steinmann, P., Computational modelling
  of isotropic multiplicative growth, Computer Modeling in Engineering
  and Sciences, 2005, 8, 119-134

<pre>
Maintainer: Susanna Tinkl
            tinkl@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15265
</pre>
*----------------------------------------------------------------------*/


#include "growth_ip.H"
#include "growth_law.H"
#include "../drt_lib/drt_globalproblem.H"
#include "../drt_mat/matpar_bundle.H"
#include "../drt_lib/drt_utils_factory.H"  // for function Factory in Unpack
#include "../drt_lib/drt_utils.H"  // for debug plotting with gmsh
#include "../drt_io/io_gmsh.H" // for debug plotting with gmsh
#include "../drt_io/io_control.H" // for debug plotting with gmsh
#include "../drt_fem_general/drt_utils_fem_shapefunctions.H" // for debug plotting with gmsh
#include "../drt_fem_general/drt_utils_integration.H" // for debug plotting with gmsh


/*----------------------------------------------------------------------*
 |                                                                      |
 *----------------------------------------------------------------------*/
MAT::PAR::Growth::Growth(
  Teuchos::RCP<MAT::PAR::Material> matdata
  )
: Parameter(matdata),
  idmatelastic_(matdata->GetInt("IDMATELASTIC")),
  idgrowthlaw_(matdata->GetInt("GROWTHLAW")),
  starttime_(matdata->GetDouble("STARTTIME")),
  endtime_(matdata->GetDouble("ENDTIME")),
  abstol_(matdata->GetDouble("TOL"))
{
  // retrieve problem instance to read from
  const int probinst = DRT::Problem::Instance()->Materials()->GetReadFromProblem();

  // for the sake of safety
  if (DRT::Problem::Instance(probinst)->Materials() == Teuchos::null)
    dserror("Sorry dude, cannot work out problem instance.");
  // yet another safety check
  if (DRT::Problem::Instance(probinst)->Materials()->Num() == 0)
    dserror("Sorry dude, no materials defined.");

  // retrieve validated input line of material ID in question
  Teuchos::RCP<MAT::PAR::Material> curmat = DRT::Problem::Instance(probinst)->Materials()->ById(idgrowthlaw_);

  switch (curmat->Type())
  {
  case INPAR::MAT::m_growth_linear:
  {
    if (curmat->Parameter() == NULL)
      curmat->SetParameter(new MAT::PAR::GrowthLawLinear(curmat));
    MAT::PAR::GrowthLawLinear* params = static_cast<MAT::PAR::GrowthLawLinear*>(curmat->Parameter());
    growthlaw_ = params->CreateGrowthLaw();
    break;
  }
  case INPAR::MAT::m_growth_exponential:
  {
    if (curmat->Parameter() == NULL)
      curmat->SetParameter(new MAT::PAR::GrowthLawExp(curmat));
    MAT::PAR::GrowthLawExp* params = static_cast<MAT::PAR::GrowthLawExp*>(curmat->Parameter());
    growthlaw_ = params->CreateGrowthLaw();
    break;
  }
  default:
    dserror("unknown material type %d", curmat->Type());
    break;
  }
}


Teuchos::RCP<MAT::Material> MAT::PAR::Growth::CreateMaterial()
{
  return Teuchos::rcp(new MAT::Growth(this));
}


MAT::GrowthType MAT::GrowthType::instance_;

DRT::ParObject* MAT::GrowthType::Create( const std::vector<char> & data )
{
  MAT::Growth* grow = new MAT::Growth();
  grow->Unpack(data);
  return grow;
}

/*----------------------------------------------------------------------*
 |  Constructor                                   (public)         02/10|
 *----------------------------------------------------------------------*/
MAT::Growth::Growth()
  : params_(NULL)
{
}


/*----------------------------------------------------------------------*
 |  Copy-Constructor                             (public)          02/10|
 *----------------------------------------------------------------------*/
MAT::Growth::Growth(MAT::PAR::Growth* params)
  : params_(params)
{
}


/*----------------------------------------------------------------------*
 |  Pack                                          (public)         02/10|
 *----------------------------------------------------------------------*/
void MAT::Growth::Pack(DRT::PackBuffer& data) const
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

  int numgp;
  if (!isinit_)
  {
    numgp = 0; // not initialized -> nothing to pack
  }
  else
  {
    numgp = theta_->size();   // size is number of gausspoints
  }
  AddtoPack(data,numgp);
  // Pack internal variables
  for (int gp = 0; gp < numgp; ++gp)
  {
    AddtoPack(data,theta_->at(gp));
    AddtoPack(data,thetaold_->at(gp));
    AddtoPack(data,mandel_->at(gp));
  }

  // Pack data of elastic material
  if (matelastic_!=Teuchos::null) {
    matelastic_->Pack(data);
  }

  return;
}


/*----------------------------------------------------------------------*
 |  Unpack                                        (public)         02/10|
 *----------------------------------------------------------------------*/
void MAT::Growth::Unpack(const std::vector<char>& data)
{
  isinit_=true;
  std::vector<char>::size_type position = 0;
  // extract type
  int type = 0;
  ExtractfromPack(position,data,type);
  if (type != UniqueParObjectId()) dserror("wrong instance type data");

  // matid and recover params_
  int matid;
  ExtractfromPack(position,data,matid);
  params_ = NULL;
  if (DRT::Problem::Instance()->Materials() != Teuchos::null)
    if (DRT::Problem::Instance()->Materials()->Num() != 0)
    {
      const int probinst = DRT::Problem::Instance()->Materials()->GetReadFromProblem();
      MAT::PAR::Parameter* mat = DRT::Problem::Instance(probinst)->Materials()->ParameterById(matid);
      if (mat->Type() == MaterialType())
        params_ = static_cast<MAT::PAR::Growth*>(mat);
      else
        dserror("Type of parameter material %d does not fit to calling type %d", mat->Type(), MaterialType());
    }

  int numgp;
  ExtractfromPack(position,data,numgp);
  if (numgp == 0){ // no history data to unpack
    isinit_=false;
    if (position != data.size())
      dserror("Mismatch in size of data %d <-> %d",data.size(),position);
    return;
  }

  // unpack growth internal variables
  theta_ = Teuchos::rcp(new std::vector<double> (numgp));
  thetaold_ = Teuchos::rcp(new std::vector<double> (numgp));
  mandel_ = Teuchos::rcp(new std::vector<double> (numgp));
  for (int gp = 0; gp < numgp; ++gp) {
    double a;
    ExtractfromPack(position,data,a);
    theta_->at(gp) = a;
    ExtractfromPack(position,data,a);
    thetaold_->at(gp) = a;
    ExtractfromPack(position,data,a);
    mandel_->at(gp) = a;
  }

  // Unpack data of elastic material (these lines are copied from drt_element.cpp)
  std::vector<char> dataelastic;
  ExtractfromPack(position,data,dataelastic);
  if (dataelastic.size()>0)
  {
    DRT::ParObject* o = DRT::UTILS::Factory(dataelastic);  // Unpack is done here
    MAT::So3Material* matel = dynamic_cast<MAT::So3Material*>(o);
    if (matel==NULL)
      dserror("failed to unpack elastic material");
    matelastic_ = Teuchos::rcp(matel);
  }
  else
    matelastic_ = Teuchos::null;

  // alternative way to unpack, but not in postprocessing
  // if (params_!=NULL) {
  //   matelastic_ = MAT::Material::Factory(params_->matelastic_);
  //   matelastic_->Unpack(dataelastic);
  // }

  if (position != data.size())
    dserror("Mismatch in size of data %d <-> %d",data.size(),position);

  return;
}

/*----------------------------------------------------------------------*
 |  Setup                                         (public)         02/10|
 *----------------------------------------------------------------------*/
void MAT::Growth::Setup(int numgp, DRT::INPUT::LineDefinition* linedef)
{
  theta_ = Teuchos::rcp(new std::vector<double> (numgp));
  thetaold_ = Teuchos::rcp(new std::vector<double> (numgp));
  mandel_ = Teuchos::rcp(new std::vector<double> (numgp));
  for (int j=0; j<numgp; ++j)
  {
    theta_->at(j) = 1.0;
    thetaold_->at(j) = 1.0;
    mandel_->at(j) = 0.0;
  }

  // Setup of elastic material
  matelastic_ = Teuchos::rcp_dynamic_cast<MAT::So3Material>(MAT::Material::Factory(params_->idmatelastic_));
  matelastic_->Setup(numgp, linedef);

  isinit_ = true;
  return;
}

/*----------------------------------------------------------------------*
 |  ResetAll                                      (public)         11/12|
 *----------------------------------------------------------------------*/
void MAT::Growth::ResetAll(const int numgp)
{
  for (int j=0; j<numgp; ++j)
  {
    theta_->at(j) = 1.0;
    thetaold_->at(j) = 1.0;
    mandel_->at(j) = 0.0;
  }

  matelastic_->ResetAll(numgp);
}

/*----------------------------------------------------------------------*
 |  Update internal growth variables              (public)         02/10|
 *----------------------------------------------------------------------*/
void MAT::Growth::Update()
{
  const int histsize = theta_->size();
  for (int i=0; i<histsize; i++)
  {
    thetaold_->at(i) = theta_->at(i);
  }

  matelastic_->Update();
}

/*----------------------------------------------------------------------*
 |  Reset internal variables                      (public)         03/13|
 *----------------------------------------------------------------------*/
void MAT::Growth::ResetStep()
{
  matelastic_->ResetStep();
}

/*----------------------------------------------------------------------*
 |  Evaluate Material                             (public)         02/10|
 *----------------------------------------------------------------------*
 The deformation gradient is decomposed into an elastic and growth part:
     F = Felastic * F_g
 Only the elastic part contributes to the stresses, thus we have to
 compute the elastic Cauchy Green Tensor Cdach and elastic 2PK stress Sdach.
 */
void MAT::Growth::Evaluate( const LINALG::Matrix<3, 3>* defgrd,
                            const LINALG::Matrix<6, 1>* glstrain,
                            Teuchos::ParameterList& params,
                            LINALG::Matrix<6, 1>* stress,
                            LINALG::Matrix<6, 6>* cmat,
                            const int eleGID)
{
  // get gauss point number
  const int gp = params.get<int>("gp", -1);
  if (gp == -1)
    dserror("no Gauss point number provided in material");

  double dt = params.get<double>("delta time", -1.0);
  double time = params.get<double>("total time", -1.0);
  if(dt==-1.0 or time == -1.0) dserror("no time step or no total time given for growth material!");
  std::string action = params.get<std::string>("action", "none");
  bool output = false;
  if (action == "calc_struct_stress")
    output = true;

  double eps = 1.0e-12;
  double endtime = params_->endtime_;

  // when stress output is calculated the final parameters already exist
  // we should not do another local Newton iteration, which uses eventually a wrong thetaold
  if (output)
    time = endtime + dt;

  if (time > params_->starttime_ + eps && time <= endtime + eps)
  {
    LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D> cmatelastic(true);
    LINALG::Matrix<NUM_STRESS_3D, 1> Sdach(true);
    const double thetaold = thetaold_->at(gp);
    //double theta = theta_->at(gp);
    double theta = thetaold;
    //double mandelcrit = 0.0; //1.0E-6;
    //double signmandel = 1.0; // adjusts sign of mandelcrit to sign of mandel

    // check wether starttime is divisible by dt, if not adapt dt in first growth step
    if (time < params_->starttime_ + dt - eps)
      dt = time - params_->starttime_;

    //--------------------------------------------------------------------------------------
    // build identity tensor I
    LINALG::Matrix<NUM_STRESS_3D, 1> Id(true);
    for (int i = 0; i < 3; i++)
      Id(i) = 1.0;

    // right Cauchy-Green Tensor  C = 2 * E + I
    LINALG::Matrix<NUM_STRESS_3D, 1> C(*glstrain);
    C.Scale(2.0);
    C += Id;

    // elastic right Cauchy-Green Tensor Cdach = F_g^-T C F_g^-1
    LINALG::Matrix<NUM_STRESS_3D, 1> Cdach(C);
    Cdach.Scale(1.0 / theta / theta);
    LINALG::Matrix<3, 3> defgrddach(*defgrd);
    defgrddach.Scale(1.0 / theta);
    // elastic Green Lagrange strain
    LINALG::Matrix<NUM_STRESS_3D, 1> glstraindach(Cdach);
    glstraindach -= Id;
    glstraindach.Scale(0.5);
    // elastic 2 PK stress and constitutive matrix
    matelastic_->Evaluate(&defgrddach,
                          &glstraindach,
                          params,
                          &Sdach,
                          &cmatelastic,
                          eleGID);

    // trace of elastic Mandel stress Mdach = Cdach Sdach
    double mandel =   Cdach(0) * Sdach(0)
                    + Cdach(1) * Sdach(1)
                    + Cdach(2) * Sdach(2)
                    + Cdach(3) * Sdach(3)
                    + Cdach(4) * Sdach(4)
                    + Cdach(5) * Sdach(5);
    //if (signmandel*mandel < 0) signmandel = -1.0*signmandel;

    // Evaluate growth law
    double growthfunc = 0.0;
    double dgrowthfunctheta = 0.0;
    EvaluateGrowthFunction(growthfunc, mandel, theta);
    //evaluate derivative of growth function w.r.t. growth factor
    EvaluateGrowthFunctionDerivTheta(dgrowthfunctheta, mandel, theta, Cdach, cmatelastic);

    double residual = thetaold - theta + growthfunc * dt;

    int localistep = 0;
    double thetaquer = 0.0;
    int maxstep = 30;
    double abstol = params_->abstol_;

    // local Newton iteration to obtain exact theta
    while (abs(residual) > abstol && localistep < maxstep)
    {
      localistep += 1;

      //evaluate derivative of growth function w.r.t. growth factor
      //EvaluateGrowthFunctionDerivTheta(dgrowthfunctheta, mandel, thetatemp, Cdach, cmatelastic);
      thetaquer =   1.0 - dgrowthfunctheta * dt;

      // damping strategy
      double omega = 2.0;
      double thetatemp = theta;
      double residualtemp = residual;
      double omegamin = 1.0 / 64.0;
      while ( abs(residualtemp) > (1.0 - 0.5 * omega) * abs(residual) and
              omega > omegamin )
      {
        // update of theta
        omega = 0.5*omega;
        thetatemp = theta + omega * residual / thetaquer;
        //std::cout << gp << ": Theta " << thetatemp << " residual " << residualtemp << " stress " << mandel << std::endl;

        // update elastic variables
        Cdach = C;
        Cdach.Scale(1.0 / thetatemp / thetatemp);
        LINALG::Matrix<3, 3> defgrddach(*defgrd);
        defgrddach.Scale(1.0 / thetatemp);
        glstraindach = Cdach;
        glstraindach -= Id;
        glstraindach.Scale(0.5);
        cmatelastic.Scale(0.0);
        Sdach.Scale(0.0);
        matelastic_->Evaluate(&defgrddach,
                              &glstraindach,
                              params,
                              &Sdach,
                              &cmatelastic,
                              eleGID);

        // trace of mandel stress
        mandel =   Cdach(0) * Sdach(0) + Cdach(1) * Sdach(1) + Cdach(2) * Sdach(2)
                 + Cdach(3) * Sdach(3) + Cdach(4) * Sdach(4) + Cdach(5) * Sdach(5);
        //if (signmandel*mandel < 0) signmandel = -1.0*signmandel;

        growthfunc = 0.0;
        EvaluateGrowthFunction(growthfunc, mandel, thetatemp);

        residualtemp =    thetaold - thetatemp + growthfunc * dt;
      } // end of damping loop
      residual = residualtemp;
      theta = thetatemp;

      //evaluate derivative of growth function w.r.t. growth factor
      EvaluateGrowthFunctionDerivTheta(dgrowthfunctheta, mandel, theta, Cdach, cmatelastic);

      if ( omega <= omegamin and
           abs(residualtemp) > (1.0 - 0.5 * omega) * abs(residual))
      {
        std::cout << gp << ": Theta " << thetatemp << " residual "
            << residualtemp << " stress " << mandel << std::endl;
        //dserror("no damping coefficient found");
      }

    } // end of local Newton iteration
    //std::cout.precision(13);
    //if ((time-1.2) > 1.0E-8) std::cout << gp << " strain " << *glstrain << std::endl;
    //if ((time-2.1E-4) > 1.0E-8) std::cout << gp << ": theta " << theta << " thetaold " << thetaold << std::endl;
    if (localistep == maxstep && abs(residual) > abstol)
      dserror("local Newton iteration did not converge after %i steps: residual: %e, thetaold: %f,"
          " theta:  %f, mandel: %e",maxstep, residual, thetaold, theta, mandel);

    thetaquer =   1.0 - dgrowthfunctheta * dt;

    // 2PK stress S = F_g^-1 Sdach F_g^-T
    LINALG::Matrix<NUM_STRESS_3D, 1> S(Sdach);
    S.Scale(1.0 / theta / theta);
    *stress = S;

    // constitutive matrix including growth
    cmatelastic.Scale(1.0 / theta / theta / theta / theta);

    LINALG::Matrix<NUM_STRESS_3D, 1> dgrowthfuncdC(true);
    EvaluateGrowthFunctionDerivC(dgrowthfuncdC,mandel,theta,C,S,cmatelastic);

    for (int i = 0; i < 6; i++)
    {
      double cmatelasCi =   cmatelastic(i, 0) * C(0) + cmatelastic(i, 1) * C(1)
                          + cmatelastic(i, 2) * C(2) + cmatelastic(i, 3) * C(3)
                          + cmatelastic(i, 4) * C(4) + cmatelastic(i, 5) * C(5);

      for (int j = 0; j < 6; j++)
      {
        (*cmat)(i, j) =  cmatelastic(i, j)
                        -2.0 / theta / thetaquer * dt
                        * (2.0 * S(i) + cmatelasCi) * dgrowthfuncdC(j);
      }
    }

    // store theta
    theta_->at(gp) = theta;
    mandel_->at(gp) = mandel;
    //std::cout.precision(10);
    //std::cout << gp << ": theta " << theta << " thetaold " << thetaold << " residual " << residual << std::endl;

  }
  else if (time > endtime + eps)
  { // turn off growth or calculate stresses for output
    LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D> cmatelastic(true);
    LINALG::Matrix<NUM_STRESS_3D, 1> Sdach(true);
    double theta = theta_->at(gp);

    //--------------------------------------------------------------------------------------
    // build identity tensor I
    LINALG::Matrix<NUM_STRESS_3D, 1> Id(true);
    for (int i = 0; i < 3; i++)
      Id(i) = 1.0;

    // right Cauchy-Green Tensor  C = 2 * E + I
    LINALG::Matrix<NUM_STRESS_3D, 1> C(*glstrain);
    C.Scale(2.0);
    C += Id;

    // elastic right Cauchy-Green Tensor Cdach = F_g^-T C F_g^-1
    LINALG::Matrix<NUM_STRESS_3D, 1> Cdach(C);
    Cdach.Scale(1.0 / theta / theta);
    LINALG::Matrix<3, 3> defgrddach(*defgrd);
    defgrddach.Scale(1.0 / theta);
    // elastic Green Lagrange strain
    LINALG::Matrix<NUM_STRESS_3D, 1> glstraindach(Cdach);
    glstraindach -= Id;
    glstraindach.Scale(0.5);
    // elastic 2 PK stress and constitutive matrix
    matelastic_->Evaluate( &defgrddach,
                           &glstraindach,
                           params,
                           &Sdach,
                           &cmatelastic,
                           eleGID);

    // 2PK stress S = F_g^-1 Sdach F_g^-T
    LINALG::Matrix<NUM_STRESS_3D, 1> S(Sdach);
    S.Scale(1.0 / theta / theta);
    *stress = S;

    // constitutive matrix including growth
    cmatelastic.Scale(1.0 / theta / theta / theta / theta);
    *cmat = cmatelastic;

    // trace of elastic Mandel stress Mdach = Cdach Sdach
    double mandel =   Cdach(0) * Sdach(0) + Cdach(1) * Sdach(1)
                    + Cdach(2) * Sdach(2) + Cdach(3) * Sdach(3) + Cdach(4) * Sdach(4)
                    + Cdach(5) * Sdach(5);
    mandel_->at(gp) = mandel;

  }
  else
  {
    matelastic_->Evaluate(defgrd, glstrain, params, stress, cmat, eleGID);
    // build identity tensor I
    LINALG::Matrix<NUM_STRESS_3D, 1> Id(true);
    for (int i = 0; i < 3; i++)
      Id(i) = 1.0;
    // right Cauchy-Green Tensor  C = 2 * E + I
    LINALG::Matrix<NUM_STRESS_3D, 1> C(*glstrain);
    C.Scale(2.0);
    C += Id;
    LINALG::Matrix<NUM_STRESS_3D, 1> S(true);
    S = *stress;
    mandel_->at(gp) =   C(0) * S(0) + C(1) * S(1) + C(2) * S(2) + C(3) * S(3)
                      + C(4) * S(4) + C(5) * S(5);
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void MAT::Growth::EvaluateNonLinMass( const LINALG::Matrix<3, 3>* defgrd,
                            const LINALG::Matrix<6, 1>* glstrain,
                            Teuchos::ParameterList& params,
                            LINALG::Matrix<NUM_STRESS_3D,1>* linmass_disp,
                            LINALG::Matrix<NUM_STRESS_3D,1>* linmass_vel,
                            const int eleGID)
{
  double eps = 1.0e-12;
  double endtime = params_->endtime_;
  double time = params.get<double>("total time", -1.0);

  if (time > params_->starttime_ + eps and time <= endtime + eps)
  {
    // get gauss point number
    const int gp = params.get<int>("gp", -1);
    if (gp == -1)
      dserror("no Gauss point number provided in material");

    LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D> cmatelastic(true);
    LINALG::Matrix<NUM_STRESS_3D, 1> Sdach(true);
    double theta = theta_->at(gp);

    //--------------------------------------------------------------------------------------
    // build identity tensor I
    LINALG::Matrix<NUM_STRESS_3D, 1> Id(true);
    for (int i = 0; i < 3; i++)
      Id(i) = 1.0;

    // right Cauchy-Green Tensor  C = 2 * E + I
    LINALG::Matrix<NUM_STRESS_3D, 1> C(*glstrain);
    C.Scale(2.0);
    C += Id;

    // elastic right Cauchy-Green Tensor Cdach = F_g^-T C F_g^-1
    LINALG::Matrix<NUM_STRESS_3D, 1> Cdach(C);
    Cdach.Scale(1.0 / theta / theta);
    LINALG::Matrix<3, 3> defgrddach(*defgrd);
    defgrddach.Scale(1.0 / theta);
    // elastic Green Lagrange strain
    LINALG::Matrix<NUM_STRESS_3D, 1> glstraindach(Cdach);
    glstraindach -= Id;
    glstraindach.Scale(0.5);
    // elastic 2 PK stress and constitutive matrix
    matelastic_->Evaluate( &defgrddach,
                           &glstraindach,
                           params,
                           &Sdach,
                           &cmatelastic,
                           eleGID);

    // trace of elastic Mandel stress Mdach = Cdach Sdach
    double mandel =   Cdach(0) * Sdach(0)
                    + Cdach(1) * Sdach(1)
                    + Cdach(2) * Sdach(2)
                    + Cdach(3) * Sdach(3)
                    + Cdach(4) * Sdach(4)
                    + Cdach(5) * Sdach(5);

    double dgrowthfunctheta = 0.0;
    //evaluate derivative of growth function w.r.t. growth factor
    EvaluateGrowthFunctionDerivTheta(dgrowthfunctheta, mandel, theta, Cdach, cmatelastic);

    // 2PK stress S = F_g^-1 Sdach F_g^-T
    LINALG::Matrix<NUM_STRESS_3D, 1> S(Sdach);
    S.Scale(1.0 / theta / theta);

    // constitutive matrix including growth
    cmatelastic.Scale(1.0 / theta / theta / theta / theta);

    EvaluateGrowthFunctionDerivC(*linmass_disp,mandel,theta,C,S,cmatelastic);

    double dt = params.get<double>("delta time", -1.0);

    double thetaquer =   1.0 - dgrowthfunctheta * dt;

    linmass_disp->Scale(dt / thetaquer * 3.0*theta*theta*matelastic_->Density());
    linmass_vel->Clear();
  }
  else
  {
    //no growth. set to zero
    linmass_disp->Clear();
    linmass_vel->Clear();
  }
}

/*----------------------------------------------------------------------*
 |  Evaluate growth function                           (protected)        02/10|
 *----------------------------------------------------------------------*/
void MAT::Growth::EvaluateGrowthFunction
(
  double & growthfunc,
  double traceM,
  double theta
)
{
  params_->growthlaw_->EvaluateGrowthFunction(growthfunc, traceM, theta);
  return;
}

/*----------------------------------------------------------------------*
 |  Evaluate derivative of growth function            (protected)   02/10|
 *----------------------------------------------------------------------*/
void MAT::Growth::EvaluateGrowthFunctionDerivTheta
(
  double & dgrowthfunctheta,
  double traceM,
  double theta,
  const LINALG::Matrix<NUM_STRESS_3D, 1>& Cdach,
  const LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D>& cmatelastic
)
{
  params_->growthlaw_->EvaluateGrowthFunctionDerivTheta(dgrowthfunctheta, traceM, theta, Cdach, cmatelastic);
  return;
}

/*----------------------------------------------------------------------*
 |  Evaluate derivative of growth function            (protected)   02/10|
 *----------------------------------------------------------------------*/
void MAT::Growth::EvaluateGrowthFunctionDerivC
(
  LINALG::Matrix<NUM_STRESS_3D, 1>& dgrowthfuncdC,
  double traceM,
  double theta,
  const LINALG::Matrix<NUM_STRESS_3D, 1>& C,
  const LINALG::Matrix<NUM_STRESS_3D, 1>& S,
  const LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D>& cmat
)
{
  params_->growthlaw_->EvaluateGrowthFunctionDerivC(dgrowthfuncdC,traceM,theta,C,S,cmat);

  return;
}

/*----------------------------------------------------------------------*
 |  Names of gp data to be visualized             (public)         03/13|
 *----------------------------------------------------------------------*/
void MAT::Growth::VisNames(std::map<std::string,int>& names)
{
  std::string fiber = "Theta";
  names[fiber] = 1;
  fiber = "Mandel";
  names[fiber] = 1;
  matelastic_->VisNames(names);
}

/*----------------------------------------------------------------------*
 |  gp data to be visualized                      (public)         03/13|
 *----------------------------------------------------------------------*/
bool MAT::Growth::VisData(const std::string& name, std::vector<double>& data, int numgp , int eleID)
{
  if (name == "Theta")
  {
    if ((int)data.size()!=1)
      dserror("size mismatch");
    double temp = 0.0;
    for (int iter=0; iter<numgp; iter++)
      temp += theta_()->at(iter);
    data[0] = temp/numgp;
  }
  else if (name == "Mandel")
  {
    if ((int)data.size()!=1)
      dserror("size mismatch");
    double temp = 0.0;
    for (int iter=0; iter<numgp; iter++)
      temp += mandel_->at(iter);
    data[0] = temp/numgp;
  }
  else
  {
    return matelastic_->VisData(name, data, numgp, eleID);
  }
  return true;
}


/*----------------------------------------------------------------------*
 |  Debug output to gmsh-file                                      10/10|
 *----------------------------------------------------------------------*
 this needs to be copied to STR::TimInt::OutputStep() to enable debug output
 {
   discret_->SetState("displacement",Dis());
   MAT::GrowthOutputToGmsh(discret_, StepOld(), 1);
 }
 don't forget to include growth_ip.H */
void MAT::GrowthOutputToGmsh
(
  const Teuchos::RCP<DRT::Discretization> dis,
  const int timestep,
  const int iter
)
{
  const std::string filebase = DRT::Problem::Instance()->OutputControlFile()->FileName();
  // file for mandel stress
  std::stringstream filename_mandel;
  filename_mandel << filebase << "_mandel" << std::setw(3) << std::setfill('0') << timestep << std::setw(2) << std::setfill('0') << iter << ".pos";
  std::ofstream f_system_mandel(filename_mandel.str().c_str());
  std::stringstream gmshfilecontent_mandel;
  gmshfilecontent_mandel << "View \" Time: " << timestep << " Iter: " << iter << " \" {" << std::endl;

  // file for theta
  std::stringstream filename_theta;
  filename_theta << filebase << "_theta" << std::setw(3) << std::setfill('0') << timestep << std::setw(2) << std::setfill('0') << iter << ".pos";
  std::ofstream f_system_theta(filename_theta.str().c_str());
  std::stringstream gmshfilecontent_theta;
  gmshfilecontent_theta << "View \" Time: " << timestep << " Iter: " << iter << " \" {" << std::endl;

  for (int iele=0; iele<dis->NumMyColElements(); ++iele)
  {
    const DRT::Element* actele = dis->lColElement(iele);

    // build current configuration
    std::vector<int> lm;
    std::vector<int> lmowner;
    std::vector<int> lmstride;
    actele->LocationVector(*dis,lm,lmowner,lmstride);
    Teuchos::RCP<const Epetra_Vector> disp = dis->GetState("displacement");
    std::vector<double> mydisp(lm.size(),0);
    DRT::UTILS::ExtractMyValues(*disp,mydisp,lm);

    Teuchos::RCP<MAT::Material> mat = actele->Material();
    MAT::Growth* grow = static_cast <MAT::Growth*>(mat.get());
    Teuchos::RCP<std::vector<double> > mandel = grow->Getmandel();
    Teuchos::RCP<std::vector<double> > theta = grow->Gettheta();

    // material plot at gauss points
    int ngp = theta->size();

    // update element geometry
    const int numnode = actele->NumNode();
    const int numdof = 3;
    Epetra_SerialDenseMatrix xcurr(numnode,3);  // material coord. of element
    for (int i=0; i<numnode; ++i)
    {
      xcurr(i,0) = actele->Nodes()[i]->X()[0]+ mydisp[i*numdof+0];
      xcurr(i,1) = actele->Nodes()[i]->X()[1]+ mydisp[i*numdof+1];
      xcurr(i,2) = actele->Nodes()[i]->X()[2]+ mydisp[i*numdof+2];
    }
    const DRT::Element::DiscretizationType distype = actele->Shape();
    Epetra_SerialDenseVector funct(numnode);

    // define gauss rule
    DRT::UTILS::GaussRule3D gaussrule_ = DRT::UTILS::intrule3D_undefined;
    switch (distype)
    {
    case DRT::Element::hex8:
    {
      gaussrule_ = DRT::UTILS::intrule_hex_8point;
      if (ngp != 8)
        dserror("hex8 has not 8 gauss points: %d", ngp);
      break;
    }
    case DRT::Element::wedge6:
    {
      gaussrule_ = DRT::UTILS::intrule_wedge_6point;
      if (ngp != 6)
        dserror("wedge6 has not 6 gauss points: %d", ngp);
      break;
    }
    case DRT::Element::tet4:
    {
      gaussrule_ = DRT::UTILS::intrule_tet_1point;
      if (ngp != 1)
        dserror("tet4 has not 1 gauss point: %d", ngp);
      break;
    }
    default:
      dserror("unknown element in ConstraintMixtureOutputToGmsh");
      break;
    }

    const DRT::UTILS::IntegrationPoints3D intpoints(gaussrule_);

    for (int gp = 0; gp < ngp; ++gp)
    {
      DRT::UTILS::shape_function_3D(funct, intpoints.qxg[gp][0], intpoints.qxg[gp][1], intpoints.qxg[gp][2], distype);
      Epetra_SerialDenseMatrix point(1,3);
      point.Multiply('T','N',1.0,funct,xcurr,0.0);

      // write mandel stress
      double mandelgp = mandel->at(gp);
      gmshfilecontent_mandel << "SP(" << std::scientific << point(0,0) << ",";
      gmshfilecontent_mandel << std::scientific << point(0,1) << ",";
      gmshfilecontent_mandel << std::scientific << point(0,2) << ")";
      gmshfilecontent_mandel << "{" << std::scientific
      << mandelgp
      << "};" << std::endl;

      // write theta
      double thetagp = theta->at(gp);
      gmshfilecontent_theta << "SP(" << std::scientific << point(0,0) << ",";
      gmshfilecontent_theta << std::scientific << point(0,1) << ",";
      gmshfilecontent_theta << std::scientific << point(0,2) << ")";
      gmshfilecontent_theta << "{" << std::scientific
      << thetagp
      << "};" << std::endl;
    }
  }
  gmshfilecontent_mandel << "};" << std::endl;
  f_system_mandel << gmshfilecontent_mandel.str();
  f_system_mandel.close();

  gmshfilecontent_theta << "};" << std::endl;
  f_system_theta << gmshfilecontent_theta.str();
  f_system_theta.close();

  return;
}
