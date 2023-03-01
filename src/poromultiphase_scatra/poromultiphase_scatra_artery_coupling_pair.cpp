/*----------------------------------------------------------------------*/
/*! \file
 \brief one pair consisting of exactly one artery element and one poro-
        multiphase-scatra element which might be coupled to each other

   \level 3

 *----------------------------------------------------------------------*/

#include "poromultiphase_scatra_artery_coupling_pair.H"
#include "poromultiphase_scatra_artery_coupling_defines.H"


#include "lib_elementtype.H"
#include "mat_fluidporo_multiphase.H"
#include "lib_utils.H"
#include "lib_globalproblem.H"
#include "lib_get_functionofanything.H"
#include "mat_cnst_1d_art.H"
#include "headers_FAD_utils.H"
#include "porofluidmultiphase_ele_parameter.H"
#include "geometry_coordinate_system_utils.H"

#include "fem_general_utils_fem_shapefunctions.H"
#include "fem_general_utils_integration.H"
#include "lib_element_integration_select.H"

#include <Epetra_MultiVector.h>

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::PoroMultiPhaseScatraArteryCouplingPair()
    : PoroMultiPhaseScatraArteryCouplingPairBase(),
      coupltype_(CouplingType::type_undefined),
      couplmethod_(INPAR::ARTNET::ArteryPoroMultiphaseScatraCouplingMethod::none),
      condname_(""),
      isinit_(false),
      ispreevaluated_(false),
      isactive_(false),
      funct_coupl_active_(false),
      diam_funct_active_(false),
      evaluate_in_ref_config_(true),
      evaluate_on_lateral_surface_(true),
      element1_(nullptr),
      element2_(nullptr),
      arterydiamref_(0.0),
      arterydiamAtGP_(0.0),
      numdof_cont_(0),
      numdof_art_(0),
      dim1_(0),
      dim2_(0),
      numcoupleddofs_(0),
      numfluidphases_(0),
      numvolfrac_(0),
      numscalcont_(0),
      numscalart_(0),
      nds_porofluid_(-1),
      n_gp_(0),
      n_gp_per_patch_(0),
      arteryelelengthref_(0.0),
      arteryelelength_(0.0),
      jacobi_(0.0),
      pp_(0.0),
      eta_a_(0.0),
      eta_b_(0.0),
      curr_segment_length_(0.0),
      constant_part_evaluated_(false),
      coupling_element_type_(""),
      artdiam_funct_(nullptr),
      porosityname_("porosity"),
      artpressname_("p_art"),
      segmentid_(-1),
      numpatch_axi_(0),
      numpatch_rad_(0),
      timefacrhs_art_dens_(0.0),
      timefacrhs_cont_dens_(0.0),
      timefacrhs_art_(0.0),
      timefacrhs_cont_(0.0)
{
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::Init(std::vector<DRT::Element const*> elements,
    const Teuchos::ParameterList& couplingparams, const Teuchos::ParameterList& fluidcouplingparams,
    const std::vector<int>& coupleddofs_cont, const std::vector<int>& coupleddofs_art,
    const std::vector<std::vector<int>>& scale_vec, const std::vector<std::vector<int>>& funct_vec,
    const std::string condname, const double penalty, const std::string couplingtype,
    const int eta_ntp)
{
  // init stuff
  couplmethod_ = DRT::INPUT::IntegralValue<INPAR::ARTNET::ArteryPoroMultiphaseScatraCouplingMethod>(
      couplingparams, "ARTERY_COUPLING_METHOD");

  condname_ = condname;

  evaluate_in_ref_config_ =
      DRT::INPUT::IntegralValue<int>(fluidcouplingparams, "EVALUATE_IN_REF_CONFIG");

  evaluate_on_lateral_surface_ =
      DRT::INPUT::IntegralValue<int>(fluidcouplingparams, "LATERAL_SURFACE_COUPLING");

  coupling_element_type_ = couplingtype;

  numpatch_axi_ = fluidcouplingparams.get<int>("NUMPATCH_AXI");
  numpatch_rad_ = fluidcouplingparams.get<int>("NUMPATCH_RAD");

  element1_ = elements[0];
  element2_ = elements[1];

  // set coupling type
  if (element1_->ElementType().Name() == "ArteryType" &&
      element2_->ElementType().Name() == "PoroFluidMultiPhaseType")
  {
    coupltype_ = type_porofluid;
    nds_porofluid_ = 0;
  }
  else if (element1_->ElementType().Name() == "TransportType" &&
           element2_->ElementType().Name() == "TransportType")
  {
    coupltype_ = type_scatra;
    nds_porofluid_ = 2;
  }
  else
  {
    dserror("Your selected coupling is not possible, type of element1: " +
            element1_->ElementType().Name() +
            ", type of element2: " + element2_->ElementType().Name());
  }

  if (couplmethod_ == INPAR::ARTNET::ArteryPoroMultiphaseScatraCouplingMethod::ntp)
  {
    // set eta
    eta_.resize(1);
    eta_[0] = eta_ntp;

    // check couplingtype
    if (coupling_element_type_ != "ARTERY" && coupling_element_type_ != "AIRWAY")
    {
      if (coupltype_ == type_porofluid)
      {
        dserror(
            "Wrong coupling type in DESIGN 1D ARTERY TO POROFLUID NONCONF COUPLING CONDITIONS.\n "
            "NTP-coupling is only possible for coupling type: "
            " 'ARTERY' or 'AIRWAY'. "
            "Your coupling type "
            "is: " +
            coupling_element_type_);
      }
      else
      {
        dserror(
            "Wrong coupling type in DESIGN 1D ARTERY TO SCATRA NONCONF COUPLING CONDITIONS.\n"
            "NTP-coupling is only possible for coupling type: "
            "'ARTERY' or 'AIRWAY'. "
            "Your coupling type "
            "is: " +
            coupling_element_type_);
      }
    }
  }

  // get number of DOFs of artery or artery-scatra
  const DRT::Node* const* artnodes;
  artnodes = element1_->Nodes();

  numdof_art_ = element1_->NumDofPerNode(*artnodes[0]);
  for (int i = 1; i < element1_->NumNode(); i++)
    if (numdof_art_ != element1_->NumDofPerNode(*artnodes[i]))
      dserror("It is not possible to have different number of Dofs in artery discretization");
  dim1_ = numdof_art_ * element1_->NumNode();

  // get number of DOFs of continuous ele (scatra or porofluid)
  const DRT::Node* const* contnodes;
  contnodes = element2_->Nodes();

  numdof_cont_ = element2_->NumDofPerNode(*contnodes[0]);
  for (int i = 1; i < element2_->NumNode(); i++)
    if (numdof_cont_ != element2_->NumDofPerNode(*contnodes[i]))
      dserror("It is not possible to have different number of Dofs in continuos discretization");
  dim2_ = numdof_cont_ * element2_->NumNode();

  // safety check
  if (numdof_art_ != (int)(scale_vec[0].size())) dserror("Wrong size of scale-vector (artery)");
  if (numdof_art_ != (int)(funct_vec[0].size())) dserror("Wrong size of function-vector (artery)");
  if (numdof_cont_ != (int)(scale_vec[1].size()))
    dserror("Wrong size of scale-vector (continuous discretization)");
  if (numdof_cont_ != (int)(funct_vec[1].size()))
    dserror("Wrong size of function-vector (continuous discretization)");

  // fill scale vector
  scale_vec_ = scale_vec;

  // fill function vector
  funct_vec_.resize(2);
  funct_vec_[0].resize(numdof_art_);
  funct_vec_[1].resize(numdof_cont_);
  FillFunctionVector(funct_vec_[0], funct_vec[0], scale_vec_[0]);
  FillFunctionVector(funct_vec_[1], funct_vec[1], scale_vec_[1]);

  // get the actually coupled dofs
  coupleddofs_cont_ = coupleddofs_cont;
  // Note: this will be overwritten in case of arteryscatra-scatra coupling
  volfracpressid_ = coupleddofs_cont;
  coupleddofs_art_ = coupleddofs_art;
  numcoupleddofs_ = coupleddofs_cont.size();

  // safety check
  for (int icont = 0; icont < numcoupleddofs_; icont++)
  {
    if (coupleddofs_cont_[icont] >= numdof_cont_)
    {
      dserror(
          "You try to couple DOF %d, which is larger than the number of dofs of the continuous "
          "discretization",
          coupleddofs_cont_[icont] + 1);
    }
    if (coupleddofs_cont_[icont] < 0)
    {
      dserror("Your coupling DOF of the continuous discretization must be >= 0, your DOF = %d",
          coupleddofs_cont_[icont] + 1);
    }
  }
  for (int iart = 0; iart < numcoupleddofs_; iart++)
  {
    if (coupleddofs_art_[iart] >= numdof_art_)
    {
      dserror(
          "You try to couple DOF %d, which is larger than the number of dofs of the artery "
          "discretization",
          coupleddofs_art_[iart] + 1);
    }
    if (coupleddofs_art_[iart] < 0)
    {
      dserror("Your coupling DOF of the reduced discretization must be >= 0, your DOF = %d",
          coupleddofs_art_[iart] + 1);
    }
  }

  // Set reference nodal positions for artery element
  for (unsigned int n = 0; n < numnodesart_; ++n)
  {
    const DRT::Node* node = element1_->Nodes()[n];
    for (unsigned int d = 0; d < numdim_; ++d) ele1posref_(numdim_ * n + d) = node->X()[d];
  }

  // get length of 1D element
  static LINALG::Matrix<numdim_, 1> arterypos0;
  for (unsigned int d = 0; d < numdim_; ++d) arterypos0(d) = ele1posref_(d);
  static LINALG::Matrix<numdim_, 1> arterypos1;
  for (unsigned int d = 0; d < numdim_; ++d) arterypos1(d) = ele1posref_(numdim_ + d);

  static LINALG::Matrix<numdim_, 1> dist;
  dist.Update(-1.0, arterypos0, 1.0, arterypos1, 0.0);
  arteryelelengthref_ = dist.Norm2();

  // get initial direction of artery element
  lambda0_.Update(1.0 / arteryelelengthref_, dist, 0.0);

  // Set reference nodal positions for continuous discretization element
  for (unsigned int inode = 0; inode < numnodescont_; ++inode)
  {
    const DRT::Node* node = element2_->Nodes()[inode];
    for (unsigned int idim = 0; idim < numdim_; ++idim) ele2posref_(idim, inode) = node->X()[idim];
  }

  // set current nodal positions to reference nodal positions for continuous discretization
  // element
  ele2pos_.Update(1.0, ele2posref_, 0.0);

  // get penalty parameter
  pp_ = penalty;

  // get out of here
  isinit_ = true;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::SetupFluidManagersAndMaterials(const std::string disname, const double& timefacrhs_art,
    const double& timefacrhs_cont)
{
  // dummy parameter list
  DRT::ELEMENTS::PoroFluidMultiPhaseEleParameter* para =
      DRT::ELEMENTS::PoroFluidMultiPhaseEleParameter::Instance(disname);

  double arterydens = 0.0;

  Teuchos::RCP<MAT::FluidPoroMultiPhase> multiphasemat = Teuchos::null;
  Teuchos::RCP<MAT::MatList> contscatramat = Teuchos::null;
  switch (coupltype_)
  {
    case type_porofluid:
    {
      multiphasemat =
          Teuchos::rcp_dynamic_cast<MAT::FluidPoroMultiPhase>(element2_->Material(nds_porofluid_));
      if (multiphasemat == Teuchos::null)
        dserror("cast to MAT::FluidPoroMultiPhase failed for artery-porofluid coupling!");
      for (int idof = 0; idof < numcoupleddofs_; idof++)
      {
        const int matid = multiphasemat->MatID(coupleddofs_cont_[idof]);
        Teuchos::RCP<MAT::Material> singlemat = multiphasemat->MaterialById(matid);

        // safety check
        if (singlemat->MaterialType() != INPAR::MAT::m_fluidporo_volfracpressure &&
            singlemat->MaterialType() != INPAR::MAT::m_fluidporo_singlephase)
        {
          dserror(
              "You can only couple volume fraction pressures or fluid phases in multiphase "
              "porespace, your material is of type %d",
              singlemat->MaterialType());
        }
      }
      // we have a coupling with scatra -> the scatra-material is the third material in the 2D/3D
      // element
      if (element2_->NumMaterial() == 3)
      {
        if (element2_->Material(2)->MaterialType() == INPAR::MAT::m_matlist or
            element2_->Material(2)->MaterialType() == INPAR::MAT::m_matlist_reactions)
        {
          Teuchos::RCP<MAT::MatList> scatramat =
              Teuchos::rcp_static_cast<MAT::MatList>(element2_->Material(2));
          numscalcont_ = scatramat->NumMat();
        }
        else
          dserror("Only MAT_matlist is valid for poromultiphase-scatra material");
      }
      // we have a coupling with artery-scatra -> the artery-scatra-material is the second
      // material in the artery element
      if (element1_->NumMaterial() == 2)
      {
        if (element1_->Material(1)->MaterialType() == INPAR::MAT::m_matlist)
        {
          Teuchos::RCP<MAT::MatList> artscatramat =
              Teuchos::rcp_static_cast<MAT::MatList>(element1_->Material(1));
          numscalart_ = artscatramat->NumMat();
        }
        else if (element1_->Material(1)->MaterialType() == INPAR::MAT::m_scatra)
          numscalart_ = 1;
        else
          dserror("Only MAT_matlist and MAT_scatra are valid for artery-scatra material");
      }

      arterymat_ = Teuchos::rcp_static_cast<MAT::Cnst_1d_art>(element1_->Material(0));
      if (arterymat_ == Teuchos::null)
        dserror("cast to artery material failed for porofluid-artery coupling!");
      arterydiamAtGP_ = arterymat_->Diam();
      arterydiamref_ = arterymat_->DiamInitial();
      arterydens = arterymat_->Density();

      break;
    }
    case type_scatra:
    {
      // check if we actually have three materials
      if (element2_->NumMaterial() < 3) dserror("no third material available");

      multiphasemat =
          Teuchos::rcp_dynamic_cast<MAT::FluidPoroMultiPhase>(element2_->Material(nds_porofluid_));

      if (multiphasemat == Teuchos::null)
        dserror("cast to MAT::FluidPoroMultiPhase failed for arteryscatra-scatra coupling!");

      contscatramat = Teuchos::rcp_static_cast<MAT::MatList>(element2_->Material(0));
      if (contscatramat == Teuchos::null)
        dserror("cast to ScatraMat failed for arteryscatra-scatra coupling!");

      for (int idof = 0; idof < numcoupleddofs_; idof++)
      {
        const int matid = contscatramat->MatID(coupleddofs_cont_[idof]);
        Teuchos::RCP<MAT::Material> singlemat = contscatramat->MaterialById(matid);

        // safety check
        if (singlemat->MaterialType() != INPAR::MAT::m_scatra_multiporo_volfrac &&
            singlemat->MaterialType() != INPAR::MAT::m_scatra_multiporo_fluid)
        {
          dserror(
              "You can only couple MAT::ScatraMatMultiPoroVolFrac or MAT::ScatraMatMultiPoroFluid, "
              "your material is of type %d",
              singlemat->MaterialType());
        }

        if (singlemat->MaterialType() == INPAR::MAT::m_scatra_multiporo_volfrac)
        {
          const Teuchos::RCP<const MAT::ScatraMatMultiPoroVolFrac>& poromat =
              Teuchos::rcp_dynamic_cast<const MAT::ScatraMatMultiPoroVolFrac>(singlemat);
          volfracpressid_[idof] = poromat->PhaseID() + multiphasemat->NumVolFrac();
        }
      }
      // get the artery scatra-material
      if (element1_->Material(0)->MaterialType() == INPAR::MAT::m_matlist)
      {
        Teuchos::RCP<MAT::MatList> artscatramat =
            Teuchos::rcp_static_cast<MAT::MatList>(element1_->Material(0));
        numscalart_ = artscatramat->NumMat();
      }
      else if (element1_->Material(0)->MaterialType() == INPAR::MAT::m_scatra)
        numscalart_ = 1;
      else
        dserror("Only MAT_matlist and MAT_scatra are valid for artery-scatra material");
      numscalcont_ = numdof_cont_;
      arterymat_ = Teuchos::rcp_static_cast<MAT::Cnst_1d_art>(element1_->Material(1));
      if (arterymat_ == Teuchos::null)
        dserror("cast to artery material failed for arteryscatra-scatra coupling!");
      arterydiamAtGP_ = arterymat_->Diam();
      arterydiamref_ = arterymat_->DiamInitial();
      arterydens = arterymat_->Density();
      break;
    }
    default:
      dserror("Unknown coupling type");
      break;
  }

  // take care of diameter function
  const int diam_funct_num = arterymat_->DiameterFunction();
  if (diam_funct_num > -1)
  {
    // no real use-case without function coupling
    if (not funct_coupl_active_)
      dserror(
          "Diameter function has been defined but no exchange function has been set, this is "
          "currently not possible, if you still want a varying diameter without any exchange "
          "terms, you can still define a zero exchange term");
    diam_funct_active_ = true;
    artdiam_funct_ = &DRT::UTILS::GetFunctionOfAnything(diam_funct_num - 1);
    if (coupltype_ == type_porofluid)
    {
      // cont derivatives + 1 artery pressure derivative
      diamderivs_ = std::vector<double>(numdof_cont_ + 1, 0.0);
      diam_stiffmat11_ = new LINALG::SerialDenseMatrix();
      diam_stiffmat12_ = new LINALG::SerialDenseMatrix();
    }
  }

  // safety checks for lateral surface coupling
  if (evaluate_on_lateral_surface_)
  {
    if (!evaluate_in_ref_config_)
      dserror(
          "Evaluation in current configuration is not yet possible in combination with lateral "
          "surface coupling");
    if (diam_funct_active_)
      dserror(
          "Setting a varying diameter is not yet possible in combination with lateral "
          "surface coupling");
    if (not(distypeCont == DRT::Element::hex8 or distypeCont == DRT::Element::tet4))
      dserror("Only TET4 and HEX8 elements possible for lateral surface coupling");
  }

  numfluidphases_ = multiphasemat->NumFluidPhases();
  numvolfrac_ = multiphasemat->NumVolFrac();

  // create phase-manager
  phasemanager_ =
      DRT::ELEMENTS::POROFLUIDMANAGER::PhaseManagerInterface::CreatePhaseManager(*para, numdim_,
          multiphasemat->MaterialType(), POROFLUIDMULTIPHASE::Action::get_access_from_artcoupling,
          multiphasemat->NumMat(), multiphasemat->NumFluidPhases());

  // setup phasemanager
  phasemanager_->Setup(element2_, nds_porofluid_);

  // create variablemanager
  variablemanager_ = DRT::ELEMENTS::POROFLUIDMANAGER::VariableManagerInterface<numdim_,
      numnodescont_>::CreateVariableManager(*para,
      POROFLUIDMULTIPHASE::Action::get_access_from_artcoupling, multiphasemat,
      multiphasemat->NumMat(), multiphasemat->NumFluidPhases());

  // initialize the names used in functions
  InitializeFunctionNames();

  // fill vector where to assemble rhs-(function) coupling into
  // summed up phase requires special treatment
  InitializeAssembleIntoContDofVector();

  // initialize the functions
  for (int i = 0; i < 2; i++)
    for (unsigned int idof = 0; idof < funct_vec_[i].size(); idof++)
      if (funct_vec_[i][idof] != 0) InitializeFunction(*funct_vec_[i][idof]);
  if (diam_funct_active_) InitializeFunction(*artdiam_funct_);

  // set time fac for right hand side evaluation of coupling
  SetTimeFacRhs(arterydens, contscatramat, timefacrhs_art, timefacrhs_cont);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::PreEvaluate(Teuchos::RCP<Epetra_MultiVector> gp_vector)
{
  if (!isinit_) dserror("MeshTying Pair has not yet been initialized");

  if (couplmethod_ == INPAR::ARTNET::ArteryPoroMultiphaseScatraCouplingMethod::ntp)
  {
    PreEvaluateNodeToPointCoupling();
  }
  else
  {
    if (evaluate_on_lateral_surface_)
      PreEvaluateLateralSurfaceCoupling(gp_vector);
    else
      PreEvaluateCenterlineCoupling();
  }


  ispreevaluated_ = true;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::PreEvaluateLateralSurfaceCoupling(Teuchos::RCP<Epetra_MultiVector> gp_vector)
{
  const int pid = DRT::Problem::Instance()->GetDis("artery")->Comm().MyPID();
  const int mylid = element1_->LID();
  if (element2_->Owner() != pid) return;

  // unit radial basis vectors
  LINALG::Matrix<3, 1> unit_rad_1;
  LINALG::Matrix<3, 1> unit_rad_2;
  // unit tangential basis
  LINALG::Matrix<3, 1> tang;
  if (numdim_ != 3) dserror("surface-based formulation makes only sense in 3D");
  for (int idim = 0; idim < 3; idim++) tang(idim) = lambda0_(idim);

  GEO::BuildOrthonormalBasisFromUnitVector(tang, unit_rad_1, unit_rad_2);

  // get radius
  const int artelematerial = coupltype_ == type_scatra ? 1 : 0;
  Teuchos::RCP<MAT::Cnst_1d_art> arterymat =
      Teuchos::rcp_static_cast<MAT::Cnst_1d_art>(element1_->Material(artelematerial));
  if (arterymat == Teuchos::null)
    dserror("cast to artery material failed for porofluid-artery coupling!");
  double radius = arterymat->Diam() / 2.0;

  // size of one integration patch: 2*pi*R/numpatch_rad_*L/numpatch_axi_
  const double patch_size =
      1.0 / numpatch_axi_ * arteryelelengthref_ * 1.0 / numpatch_rad_ * 2.0 * M_PI * radius;

  // Vectors for shape functions and their derivatives
  static LINALG::Matrix<1, numnodesart_, double> N1(true);      // = N1
  static LINALG::Matrix<1, numnodesart_, double> N1_eta(true);  // = N1,eta
  // Coords and derivatives of 1D and 2D/3D element
  static LINALG::Matrix<numdim_, 1, double> r1(true);      // = r1
  static LINALG::Matrix<numdim_, 1, double> r1_eta(true);  // = r1,eta

  // element parameter space coordinates in 3D element
  std::vector<double> xi(3);
  // number of GPs
  n_gp_ = 0;

  // we use always 25 integration points per integration patch
  DRT::UTILS::IntegrationPoints2D gaussPointsperPatch =
      DRT::UTILS::IntegrationPoints2D(DRT::UTILS::GaussRule2D::quad_25point);
  n_gp_per_patch_ = gaussPointsperPatch.nquad;
  n_gp_ = n_gp_per_patch_ * numpatch_axi_ * numpatch_rad_;
  // define Gauss points and n_gp-sized quantities
  eta_.resize(n_gp_);
  eta_s_.resize(n_gp_);
  wgp_.resize(n_gp_);
  xi_.resize(n_gp_);
  for (int i_gp = 0; i_gp < n_gp_; i_gp++) xi_[i_gp].resize(numdim_);

  // loop over all axial patches
  for (int i_ax = 0; i_ax < numpatch_axi_; i_ax++)
  {
    // loop over all radial patches
    for (int i_rad = 0; i_rad < numpatch_rad_; i_rad++)
    {
      // loop over all GPs of this patch
      for (int i_gp = 0; i_gp < n_gp_per_patch_; i_gp++)
      {
        const int gpid = i_ax * numpatch_rad_ * n_gp_per_patch_ + i_rad * n_gp_per_patch_ + i_gp;
        // axial Gauss point eta lies in [-1; 1]
        double eta = -1.0 - 1.0 / numpatch_axi_ + (i_ax + 1.0) * 2.0 / numpatch_axi_ +
                     gaussPointsperPatch.qxg[i_gp][0] * 1.0 / numpatch_axi_;

        // Update coordinates and derivatives for 1D and 2D/3D element
        Get1DShapeFunctions<double>(N1, N1_eta, eta);
        ComputeArteryCoordsAndDerivsRef<double>(r1, r1_eta, N1, N1_eta);

        // radial Gauss point theta lies in [-pi; pi]
        double theta = (-1.0 - 1.0 / numpatch_rad_ + (i_rad + 1.0) * 2.0 / numpatch_rad_ +
                           gaussPointsperPatch.qxg[i_gp][1] * 1.0 / numpatch_rad_) *
                       M_PI;

        // get point on lateral blood vessel surface
        for (int idim = 0; idim < 3; idim++)
          r1(idim) = r1(idim) + unit_rad_1(idim) * radius * cos(theta) +
                     unit_rad_2(idim) * radius * sin(theta);

        // project into 3D domain
        bool projection_valid = false;
        Projection<double>(r1, xi, projection_valid);
        eta_[gpid] = eta;
        xi_[gpid] = xi;
        // projection is valid and GP is so far unclaimed by other pair
        if (projection_valid && ((*gp_vector)[gpid])[mylid] < 0.5)
        {
          isactive_ = true;
          // include jacobian
          wgp_[gpid] = gaussPointsperPatch.qwgt[i_gp] * patch_size / 4.0;
          gp_vector->SumIntoMyValue(mylid, gpid, 1.0);
        }
        else
          wgp_[gpid] = 0.0;
      }
    }
  }

  // free memory
  if (not isactive_)
  {
    std::vector<double>().swap(eta_);
    std::vector<double>().swap(wgp_);
    std::vector<std::vector<double>>().swap(xi_);
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::PreEvaluateCenterlineCoupling()
{
  // Try to create integration segment [eta_a, eta_b]
  CreateIntegrationSegment();

  // no viable segment found
  if (!isactive_)
  {
    return;
  }

  // choice of optimal Gauss-rule:
  // basically the N^(2)*N^(2) term is crucial
  // for (bi-, tri-)linear elements (only considered so far):
  // in 2D the highest possible polynomial order for this term is 4 since N^(2) can be quadratic
  // for arbitrary integration in element
  // --> we need 3 gp for exact integration
  // in 3D the highest possible polynomial order for this term is 6 since N^(2) can be cubic for
  // arbitrary integration in element
  // --> we need 4 gp for exact integration

  DRT::UTILS::IntegrationPoints1D gaussPoints =
      DRT::UTILS::IntegrationPoints1D(DRT::UTILS::GaussRule1D::line_3point);
  if (numdim_ == 3)
    gaussPoints = DRT::UTILS::IntegrationPoints1D(DRT::UTILS::GaussRule1D::line_4point);

  n_gp_ = gaussPoints.nquad;
  // define Gauss points and n_gp-sized quantities
  eta_.resize(n_gp_);
  eta_s_.resize(n_gp_);
  wgp_.resize(n_gp_);
  invJ_.resize(n_gp_);
  xi_.resize(n_gp_);
  for (int i_gp = 0; i_gp < n_gp_; i_gp++) xi_[i_gp].resize(numdim_);

  // get jacobian determinant
  const double determinant = (eta_b_ - eta_a_) / 2.0;
  jacobi_ = determinant * arteryelelengthref_ / 2.0;

  static LINALG::Matrix<1, numnodescont_> N2(true);           // = N2
  static LINALG::Matrix<numdim_, numnodescont_> N2_xi(true);  // = N2,xi1
  // Vectors for shape functions and their derivatives
  static LINALG::Matrix<1, numnodesart_, double> N1(true);      // = N1
  static LINALG::Matrix<1, numnodesart_, double> N1_eta(true);  // = N1,eta
  // Coords and derivatives of 1D and 2D/3D element
  static LINALG::Matrix<numdim_, 1, double> r1(true);      // = r1
  static LINALG::Matrix<numdim_, 1, double> r1_eta(true);  // = r1,eta

  // project the Gauss points --> those have to able to be projected
  for (int i_gp = 0; i_gp < n_gp_; i_gp++)
  {
    // compute the coordinate trafo, weights and project Gauss points
    eta_[i_gp] = (eta_a_ + eta_b_) / 2.0 + gaussPoints.qxg[i_gp][0] * determinant;
    eta_s_[i_gp] = eta_[i_gp];
    wgp_[i_gp] = gaussPoints.qwgt[i_gp];

    // Update coordinates and derivatives for 1D and 2D/3D element
    Get1DShapeFunctions<double>(N1, N1_eta, eta_[i_gp]);
    ComputeArteryCoordsAndDerivsRef<double>(r1, r1_eta, N1, N1_eta);

    bool projection_valid = false;
    Projection<double>(r1, xi_[i_gp], projection_valid);
    if (!projection_valid) dserror("Gauss point could not be projected");

    // compute (dX/dxi)^-1
    Get2D3DShapeFunctions<double>(N2, N2_xi, xi_[i_gp]);
    invJ_[i_gp].MultiplyNT(N2_xi, ele2pos_);
    invJ_[i_gp].Invert();
  }
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::PreEvaluateNodeToPointCoupling()
{
  xi_.resize(1);
  xi_[0].resize(numdim_);

  static LINALG::Matrix<1, numnodescont_> N2(true);           // = N2
  static LINALG::Matrix<numdim_, numnodescont_> N2_xi(true);  // = N2,xi1
  // Vectors for shape functions and their derivatives
  static LINALG::Matrix<1, numnodesart_, double> N1(true);      // = N1
  static LINALG::Matrix<1, numnodesart_, double> N1_eta(true);  // = N1,eta
  // Coords and derivatives of 1D and 2D/3D element
  static LINALG::Matrix<numdim_, 1, double> r1(true);      // = r1
  static LINALG::Matrix<numdim_, 1, double> r1_eta(true);  // = r1,eta

  // Update coordinates and derivatives for 1D and 2D/3D element
  Get1DShapeFunctions<double>(N1, N1_eta, eta_[0]);
  ComputeArteryCoordsAndDerivsRef<double>(r1, r1_eta, N1, N1_eta);

  bool projection_valid = false;
  Projection<double>(r1, xi_[0], projection_valid);

  // coupling pairs is only active if projection is valid
  isactive_ = projection_valid;

  // compute (dX/dxi)^-1
  Get2D3DShapeFunctions<double>(N2, N2_xi, xi_[0]);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::DeleteUnnecessaryGPs(Teuchos::RCP<Epetra_MultiVector> gp_vector)
{
  const int mylid = element1_->LID();
  n_gp_ = 0;
  for (int igp = 0; igp < n_gp_per_patch_ * numpatch_axi_ * numpatch_rad_; igp++)
    if (wgp_[igp] > 1e-12) n_gp_++;

  std::vector<double> wgp(n_gp_);
  std::vector<double> eta(n_gp_);
  std::vector<std::vector<double>> xi(n_gp_);

  int mygp = 0;
  for (int igp = 0; igp < n_gp_per_patch_ * numpatch_axi_ * numpatch_rad_; igp++)
  {
    if (wgp_[igp] > 1e-12)
    {
      const double scale = 1.0 / ((*gp_vector)[igp])[mylid];
      eta[mygp] = eta_[igp];
      xi[mygp] = xi_[igp];
      wgp[mygp] = wgp_[igp] * scale;
      mygp++;
    }
  }
  if (n_gp_ == 0)
    isactive_ = false;
  else
    isactive_ = true;

  // overwrite
  wgp_ = wgp;
  eta_ = eta;
  xi_ = xi;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::ResetState(Teuchos::RCP<DRT::Discretization> contdis,
    Teuchos::RCP<DRT::Discretization> artdis)
{
  if (!ispreevaluated_) dserror("MeshTying Pair has not yet been pre-evaluated");

  // get location array for continuous ele
  DRT::Element::LocationArray la_cont(contdis->NumDofSets());
  element2_->LocationVector(*contdis, la_cont, false);

  // get location array for artery ele
  DRT::Element::LocationArray la_art(artdis->NumDofSets());
  element1_->LocationVector(*artdis, la_art, false);

  switch (coupltype_)
  {
    case type_porofluid:
    {
      // extract element and node values of fluid
      variablemanager_->ExtractElementAndNodeValues(*element2_, *contdis, la_cont, ele2pos_, 0);

      // get contelephinp_ and artelephinp_
      DRT::UTILS::ExtractMyValues(*contdis->GetState("phinp_fluid"), contelephinp_, la_cont[0].lm_);
      DRT::UTILS::ExtractMyValues(
          *artdis->GetState("one_d_artery_pressure"), artelephinp_, la_art[0].lm_);

      // extract velocity of solid phase
      ExtractSolidVel(contdis);

      // extract values of artery-scatra discretization
      if (numscalart_ > 0)
      {
        Teuchos::RCP<const Epetra_Vector> artscalarnp =
            artdis->GetState(ndsartery_scatra_, "one_d_artery_phinp");
        if (artscalarnp != Teuchos::null)
        {
          DRT::Element::LocationArray la(artdis->NumDofSets());
          element1_->LocationVector(*artdis, la, false);
          // rebuild scalar vector
          eartscalarnp_.clear();
          eartscalarnp_.resize(numscalart_, LINALG::Matrix<numnodesart_, 1>(true));
          // extract local values of artery-scatra field from global state vector
          DRT::UTILS::ExtractMyValues<LINALG::Matrix<numnodesart_, 1>>(
              *artscalarnp, eartscalarnp_, la[ndsartery_scatra_].lm_);
        }
        else
          dserror("Cannot get artery-scatra from artery discretization");
      }
      // extract values of continuous scatra discretization
      if (numscalcont_ > 0)
      {
        // get state vector from discretization
        Teuchos::RCP<const Epetra_Vector> contscalarnp = contdis->GetState(3, "scalars");
        if (contscalarnp != Teuchos::null)
        {
          DRT::Element::LocationArray la(contdis->NumDofSets());
          element2_->LocationVector(*contdis, la, false);
          // rebuild scalar vector
          econtscalarnp_.clear();
          econtscalarnp_.resize(numscalcont_, LINALG::Matrix<numnodescont_, 1>(true));
          // extract local values of continuous-scatra field from global state vector
          DRT::UTILS::ExtractMyValues<LINALG::Matrix<numnodescont_, 1>>(
              *contscalarnp, econtscalarnp_, la[3].lm_);
        }
        else
          dserror("Cannot get state vector 'scalars'");
      }
      break;
    }
    case type_scatra:
    {
      // extract element and node values of fluid
      variablemanager_->ExtractElementAndNodeValues(*element2_, *contdis, la_cont, ele2pos_, 2);

      // get contelephinp_ and artelephinp_
      DRT::UTILS::ExtractMyValues(*contdis->GetState("phinp"), contelephinp_, la_cont[0].lm_);
      DRT::UTILS::ExtractMyValues(
          *artdis->GetState("one_d_artery_phinp"), artelephinp_, la_art[0].lm_);

      // extract artery pressure
      Teuchos::RCP<const Epetra_Vector> artpressnp =
          artdis->GetState(ndsscatra_artery_, "one_d_artery_pressure");
      if (artpressnp != Teuchos::null)
      {
        DRT::Element::LocationArray la(artdis->NumDofSets());
        element1_->LocationVector(*artdis, la, false);
        DRT::UTILS::ExtractMyValues<LINALG::Matrix<numnodesart_, 1>>(
            *artpressnp, earterypressurenp_, la[ndsscatra_artery_].lm_);
      }
      else
        dserror("Cannot get arterypressure from artery-scatra discretization");
      break;
    }
    default:
      dserror("Unknown coupling type");
      break;
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
double POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::Evaluate(LINALG::SerialDenseVector* forcevec1, LINALG::SerialDenseVector* forcevec2,
    LINALG::SerialDenseMatrix* stiffmat11, LINALG::SerialDenseMatrix* stiffmat12,
    LINALG::SerialDenseMatrix* stiffmat21, LINALG::SerialDenseMatrix* stiffmat22,
    LINALG::SerialDenseMatrix* D_ele, LINALG::SerialDenseMatrix* M_ele,
    LINALG::SerialDenseVector* Kappa_ele, const std::vector<double>& segmentlengths)
{
  if (!ispreevaluated_) dserror("MeshTying Pair has not yet been pre-evaluated");

  // resize and initialize variables to zero
  if (forcevec1 != NULL) forcevec1->Size(dim1_);
  if (forcevec2 != NULL) forcevec2->Size(dim2_);

  if (stiffmat11 != NULL) stiffmat11->Shape(dim1_, dim1_);
  if (stiffmat12 != NULL) stiffmat12->Shape(dim1_, dim2_);
  if (stiffmat21 != NULL) stiffmat21->Shape(dim2_, dim1_);
  if (stiffmat22 != NULL) stiffmat22->Shape(dim2_, dim2_);

  if (arterymat_->IsCollapsed()) return 0.0;

  std::vector<double> myEta;
  std::vector<std::vector<double>> myXi;

  double etaA = 0.0;
  double etaB = 0.0;

  switch (couplmethod_)
  {
    case INPAR::ARTNET::ArteryPoroMultiphaseScatraCouplingMethod::gpts:
    {
      // initialize eta and xi
      myEta.assign(n_gp_, double{});
      myXi.assign(n_gp_, std::vector<double>(numdim_, double{}));
      // recompute eta and xi --> see note in this function
      RecomputeEtaAndXiInDeformedConfiguration(segmentlengths, myEta, myXi, etaA, etaB);
      // actual evaluate
      EvaluateGPTS(myEta, myXi, segmentlengths, forcevec1, forcevec2, stiffmat11, stiffmat12,
          stiffmat21, stiffmat22);
      break;
    }
    case INPAR::ARTNET::ArteryPoroMultiphaseScatraCouplingMethod::mp:
    {
      // initialize eta and xi
      myEta.assign(n_gp_, double{});
      myXi.assign(n_gp_, std::vector<double>(numdim_, double{}));
      // recompute eta and xi --> see note in this function
      RecomputeEtaAndXiInDeformedConfiguration(segmentlengths, myEta, myXi, etaA, etaB);
      // actual evaluate
      EvaluateDMKappa(myEta, myXi, segmentlengths, D_ele, M_ele, Kappa_ele);
      break;
    }
    case INPAR::ARTNET::ArteryPoroMultiphaseScatraCouplingMethod::ntp:
    {
      // define eta and xi
      myEta = eta_;
      myXi = xi_;
      // actual evaluate
      EvaluateNTP(eta_, xi_, forcevec1, forcevec2, stiffmat11, stiffmat12, stiffmat21, stiffmat22);
      break;
    }
    default:
      dserror("Unknown coupling type for artery to poro coupling");
      break;
  }

  // case where diameter is constant
  double integrated_diam = arterydiamref_ * segmentlengths[segmentid_];

  // evaluate the function coupling (with possibly varying diameter)
  if (funct_coupl_active_)
    EvaluateFunctionCoupling(myEta, myXi, segmentlengths, forcevec1, forcevec2, stiffmat11,
        stiffmat12, stiffmat21, stiffmat22, integrated_diam);

  // evaluate derivative of 1D shape function times solid velocity
  EvaluatedNdsSolidVel(myEta, myXi, segmentlengths, *forcevec1, etaA, etaB);

  return integrated_diam;
}

/*------------------------------------------------------------------------*
 *------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateAdditionalLinearizationofIntegratedDiam(LINALG::SerialDenseMatrix* stiffmat11,
    LINALG::SerialDenseMatrix* stiffmat12)
{
  if (!ispreevaluated_) dserror("MeshTying Pair has not yet been pre-evaluated");

  if (stiffmat11 != NULL) stiffmat11->Shape(dim1_, dim1_);
  if (stiffmat12 != NULL) stiffmat12->Shape(dim1_, dim2_);

  // do not evaluate if element is collapsed
  if (arterymat_->IsCollapsed()) return;

  // this is the integrated diameter over the entire element (all segments)
  const double arterydiam = arterymat_->Diam();
  const double prefac = M_PI * std::pow(arterydiam, 3) / 32.0 / arterymat_->Viscosity();
  // TODO: for viscosity law blood, viscosity depends on diameter, this linearization is still
  // missing

  stiffmat11->Update(prefac, diam_stiffmat11_, 0.0);
  stiffmat12->Update(prefac, diam_stiffmat12_, 0.0);
}

/*------------------------------------------------------------------------*
 *------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
int POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::Ele1GID() const
{
  return element1_->Id();
}

/*------------------------------------------------------------------------*
 *------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
int POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::Ele2GID() const
{
  return element2_->Id();
}

/*------------------------------------------------------------------------*
 *------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
int POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::GetSegmentID() const
{
  return segmentid_;
}

/*------------------------------------------------------------------------*
 *------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
double POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::CalculateVol2D3D() const
{
  // use one-point Gauss rule
  DRT::UTILS::IntPointsAndWeights<numdim_> intpoints_stab(
      DRT::ELEMENTS::DisTypeToStabGaussRule<distypeCont>::rule);

  const double* gpcoord = intpoints_stab.IP().qxg[0];   // actual integration point (coords)
  const double gpweight = intpoints_stab.IP().qwgt[0];  // actual integration point (weight)

  LINALG::Matrix<numdim_, 1> xsi(gpcoord, true);
  static LINALG::Matrix<numnodescont_, 1> funct;
  static LINALG::Matrix<numdim_, numnodescont_> deriv;
  static LINALG::Matrix<numdim_, numdim_> xjm;
  static LINALG::Matrix<numdim_, numdim_> xji;

  // shape functions and their first derivatives
  DRT::UTILS::shape_function<distypeCont>(xsi, funct);
  DRT::UTILS::shape_function_deriv1<distypeCont>(xsi, deriv);

  //

  // get Jacobian matrix and determinant
  // actually compute its transpose....
  /*
    +-            -+ T      +-            -+
    | dx   dx   dx |        | dx   dy   dz |
    | --   --   -- |        | --   --   -- |
    | dr   ds   dt |        | dr   dr   dr |
    |              |        |              |
    | dy   dy   dy |        | dx   dy   dz |
    | --   --   -- |   =    | --   --   -- |
    | dr   ds   dt |        | ds   ds   ds |
    |              |        |              |
    | dz   dz   dz |        | dx   dy   dz |
    | --   --   -- |        | --   --   -- |
    | dr   ds   dt |        | dt   dt   dt |
    +-            -+        +-            -+
  */
  xjm.MultiplyNT(deriv, ele2pos_);
  double det = xji.Invert(xjm);

  if (det < 1E-16) dserror("GLOBAL ELEMENT ZERO OR NEGATIVE JACOBIAN DETERMINANT: %f", det);

  // compute integration factor
  return gpweight * det;
}

/*------------------------------------------------------------------------*
 *------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::SetSegmentID(const int& segmentid)
{
  segmentid_ = segmentid;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
double POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::ApplyMeshMovement(const bool firstcall, Teuchos::RCP<DRT::Discretization> contdis)
{
  // nodal displacement values for ALE
  LINALG::Matrix<numdim_, numnodescont_> edispnp(true);

  if (!firstcall)
  {
    Teuchos::RCP<const Epetra_Vector> dispnp = contdis->GetState(1, "dispnp");
    DRT::Element::LocationArray la(contdis->NumDofSets());
    element2_->LocationVector(*contdis, la, false);

    // construct location vector for displacement related dofs
    std::vector<int> lmdisp(numdim_ * numnodescont_, -1);
    for (unsigned int inode = 0; inode < numnodescont_; ++inode)
      for (unsigned int idim = 0; idim < numdim_; ++idim)
        lmdisp[inode * numdim_ + idim] = la[1].lm_[inode * numdim_ + idim];

    // extract local values of displacement field from global state vector
    DRT::UTILS::ExtractMyValues<LINALG::Matrix<numdim_, numnodescont_>>(*dispnp, edispnp, lmdisp);
  }
  else
    return (eta_b_ - eta_a_) / 2.0 * arteryelelengthref_;

  // update current configuration
  ele2pos_.Update(1.0, ele2posref_, 1.0, edispnp, 0.0);

  static LINALG::Matrix<1, numnodescont_> N2(true);            // = N2
  static LINALG::Matrix<numdim_, numnodescont_> N2_xi(true);   // = N2,xi1
  static LINALG::Matrix<numdim_, numnodescont_> N2_XYZ(true);  // = N2,X
  static LINALG::Matrix<numdim_, numdim_> defgrad(true);       // = dx/dX = F
  static LINALG::Matrix<numdim_, 1> Ft0;                       // = F*t0

  curr_segment_length_ = 0.0;
  // current segment length = \int_{\eta_a}^{eta_b} || F*t0 ||_2 ds
  // all under the assumption that artery element completely follows deformation of
  // underlying 2D/3D problem
  for (int i_gp = 0; i_gp < n_gp_; i_gp++)
  {
    // get shape functions of continuous element
    const std::vector<double> xi = xi_[i_gp];
    Get2D3DShapeFunctions<double>(N2, N2_xi, xi);
    // dN/dX = dN/dxi * dxi/dX = dN/dxi * (dX/dxi)^-1
    N2_XYZ.Multiply(invJ_[i_gp], N2_xi);
    // dx/dX = x * N_XYZ^T
    defgrad.MultiplyNT(ele2pos_, N2_XYZ);
    Ft0.Multiply(defgrad, lambda0_);
    curr_segment_length_ += Ft0.Norm2() * wgp_[i_gp] * jacobi_;
  }

  return curr_segment_length_;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::RecomputeEtaAndXiInDeformedConfiguration(const std::vector<double>& segmentlengths,
    std::vector<double>& myEta, std::vector<std::vector<double>>& myXi, double& etaA, double& etaB)
{
  // NOTE: we assume that the 1D artery element completely follows the deformation of the
  // underlying
  //       porous medium, so its length might change. Interaction between artery element and
  //       porous medium has to be evaluated in current/deformed configuration. However, Gauss
  //       points of the original projection (in reference configuration) cannot be used then
  //       anymore but we must define a new parameter space [-1, 1] which maps into the current
  //       configuration. First, we determine the new etaA and etaB of this segment as sum of
  //       segment lengths etaA_new = -1.0 + 2.0* ( \sum_{i=0}_{this_seg-1} l_i / total_ele_length
  //       ) etaB_new = -1.0 + 2.0* ( \sum_{i=0}_{this_seg} l_i / total_ele_length ) then GPs are
  //       distributed in the interval [etaA_new, etaB_new] The last step is to get the new
  //       projected xi_i in the 2D/3D parameter space of the second element For each new GP
  //       eta_new, this can be done by finding the point in reference configuration which deforms
  //       to the point in current configuration where the GP now lies as \int_{\eta_a}^{eta_s} ||
  //       F*t0 ||_2 ds where eta_s is the unknown. Linearization of this nonlinear equation
  //       within the Newton loop is done with FAD

  // not necessary if we do not take into account mesh movement or ntp-coupling
  if (evaluate_in_ref_config_)
  {
    myEta = eta_;
    myXi = xi_;
    arteryelelength_ = arteryelelengthref_;
  }
  else
  {
    // Vectors for shape functions and their derivatives
    static LINALG::Matrix<1, numnodesart_, double> N1(true);      // = N1
    static LINALG::Matrix<1, numnodesart_, double> N1_eta(true);  // = N1,eta
    // Coords and derivatives of 1D and 2D/3D element
    static LINALG::Matrix<numdim_, 1, double> r1(true);      // = r1
    static LINALG::Matrix<numdim_, 1, double> r1_eta(true);  // = r1,eta

    // current length of artery
    arteryelelength_ = std::accumulate(segmentlengths.begin(), segmentlengths.end(), 0.0);

    // length of segments [0, 1, ..., this_seg-1]
    double length_so_far = 0.0;
    for (int iseg = 0; iseg < segmentid_; iseg++) length_so_far += segmentlengths[iseg];

    // length of this segment
    const double curr_seg_length = segmentlengths[segmentid_];

    // get new etaA and etaB
    etaA = -1.0 + 2.0 * (length_so_far / arteryelelength_);
    etaB = -1.0 + 2.0 * ((length_so_far + curr_seg_length) / arteryelelength_);

    DRT::UTILS::IntegrationPoints1D gaussPoints =
        DRT::UTILS::IntegrationPoints1D(DRT::UTILS::GaussRule1D::line_3point);
    if (numdim_ == 3)
      gaussPoints = DRT::UTILS::IntegrationPoints1D(DRT::UTILS::GaussRule1D::line_4point);

    // distribute new Gauss points
    const double determinant = (etaB - etaA) / 2.0;
    for (int i_gp = 0; i_gp < n_gp_; i_gp++)
      myEta[i_gp] = (etaA + etaB) / 2.0 + gaussPoints.qxg[i_gp][0] * determinant;

    // solution variable for Newton loop
    FAD eta_s = 0.0;
    eta_s.diff(0, 1);  // independent variable 0 out of a total of 1

    // the GP loop
    bool converged = false;
    for (int i_gp = 0; i_gp < n_gp_; i_gp++)
    {
      // start value for Newton: last converged value of previous evaluation (should be pretty
      // close)
      eta_s.val() = eta_s_[i_gp];
      const double desired_length = curr_seg_length * (myEta[i_gp] - etaA) / (etaB - etaA);
      double val = -1.0;
      // Netwon loop
      for (int istep = 0; istep < MESHMOVEMENTMAXITER; istep++)
      {
        // integrate \int_{\eta_a}^{eta_s} || F*t0 ||_2 ds
        const FAD curr_length = IntegrateLengthToEtaS(eta_s);

        val = curr_length.val() - desired_length;
        if (fabs(val) < CONVTOLNEWTONMESHMOVEMENT)
        {
          converged = true;
          break;
        }
        const double deriv = curr_length.fastAccessDx(0);
        // Newton update
        eta_s.val() -= val / deriv;
      }

      if (!converged)
        std::cout << "WARNING: could not find Gauss point position in reference configuration";
      // finally find new xi_i by projection eta_s in reference configuration

      // Update coordinates and derivatives for 1D and 2D/3D element
      Get1DShapeFunctions<double>(N1, N1_eta, eta_s.val());
      ComputeArteryCoordsAndDerivsRef<double>(r1, r1_eta, N1, N1_eta);

      bool projection_valid = false;
      Projection<double>(r1, myXi[i_gp], projection_valid);
      if (!projection_valid) dserror("Gauss point could not be projected");
      // save the converged value
      eta_s_[i_gp] = eta_s.val();
    }  // GP loop
  }    // !evaluate_in_ref_config_
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateGPTS(const std::vector<double>& eta, const std::vector<std::vector<double>>& xi,
    const std::vector<double>& segmentlengths, LINALG::SerialDenseVector* forcevec1,
    LINALG::SerialDenseVector* forcevec2, LINALG::SerialDenseMatrix* stiffmat11,
    LINALG::SerialDenseMatrix* stiffmat12, LINALG::SerialDenseMatrix* stiffmat21,
    LINALG::SerialDenseMatrix* stiffmat22)
{
  if (numcoupleddofs_ > 0)
  {
    if (!constant_part_evaluated_)
    {
      GPTS_NTP_stiffmat11_ = new LINALG::SerialDenseMatrix();
      GPTS_NTP_stiffmat12_ = new LINALG::SerialDenseMatrix();
      GPTS_NTP_stiffmat21_ = new LINALG::SerialDenseMatrix();
      GPTS_NTP_stiffmat22_ = new LINALG::SerialDenseMatrix();
    }

    // we only have to this once if evaluated in reference configuration
    if (!constant_part_evaluated_ or !evaluate_in_ref_config_)
    {
      // Vectors for shape functions and their derivatives
      static LINALG::Matrix<1, numnodesart_> N1(true);      // = N1
      static LINALG::Matrix<1, numnodesart_> N1_eta(true);  // = N1,eta

      static LINALG::Matrix<1, numnodescont_> N2(true);           // = N2
      static LINALG::Matrix<numdim_, numnodescont_> N2_xi(true);  // = N2,xi1

      GPTS_NTP_stiffmat11_.Shape(dim1_, dim1_);
      GPTS_NTP_stiffmat12_.Shape(dim1_, dim2_);
      GPTS_NTP_stiffmat21_.Shape(dim2_, dim1_);
      GPTS_NTP_stiffmat22_.Shape(dim2_, dim2_);

      const double curr_seg_length = segmentlengths[segmentid_];

      for (int i_gp = 0; i_gp < n_gp_; i_gp++)
      {
        // Get constant values from projection
        const double w_gp = wgp_[i_gp];
        const double myeta = eta[i_gp];
        const double jac = curr_seg_length / 2.0;
        const std::vector<double> myxi = xi[i_gp];

        // Update shape functions and their derivatives for 1D and 2D/3D element
        Get1DShapeFunctions<double>(N1, N1_eta, myeta);
        Get2D3DShapeFunctions<double>(N2, N2_xi, myxi);

        // evaluate
        EvaluateGPTSStiff(w_gp, N1, N2, jac, pp_);
      }
    }  //! constant_part_evaluated_ or !evaluate_in_ref_config_

    UpdateGPTSNTPStiff(*stiffmat11, *stiffmat12, *stiffmat21, *stiffmat22);
    CheckValidVolumeFractionPressureCoupling(*stiffmat11, *stiffmat12, *stiffmat21, *stiffmat22);
    EvaluateGPTSNTPForce(
        *forcevec1, *forcevec2, *stiffmat11, *stiffmat12, *stiffmat21, *stiffmat22);
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateNTP(const std::vector<double>& eta, const std::vector<std::vector<double>>& xi,
    LINALG::SerialDenseVector* forcevec1, LINALG::SerialDenseVector* forcevec2,
    LINALG::SerialDenseMatrix* stiffmat11, LINALG::SerialDenseMatrix* stiffmat12,
    LINALG::SerialDenseMatrix* stiffmat21, LINALG::SerialDenseMatrix* stiffmat22)
{
  if (numcoupleddofs_ > 0)
  {
    if (!constant_part_evaluated_)
    {
      GPTS_NTP_stiffmat11_ = new LINALG::SerialDenseMatrix();
      GPTS_NTP_stiffmat12_ = new LINALG::SerialDenseMatrix();
      GPTS_NTP_stiffmat21_ = new LINALG::SerialDenseMatrix();
      GPTS_NTP_stiffmat22_ = new LINALG::SerialDenseMatrix();
    }

    // we only have to this once if evaluated in reference configuration
    if (!constant_part_evaluated_ or !evaluate_in_ref_config_)
    {
      // Vectors for shape functions and their derivatives
      static LINALG::Matrix<1, numnodesart_> N1(true);      // = N1
      static LINALG::Matrix<1, numnodesart_> N1_eta(true);  // = N1,eta

      static LINALG::Matrix<1, numnodescont_> N2(true);           // = N2
      static LINALG::Matrix<numdim_, numnodescont_> N2_xi(true);  // = N2,xi1

      GPTS_NTP_stiffmat11_.Shape(dim1_, dim1_);
      GPTS_NTP_stiffmat12_.Shape(dim1_, dim2_);
      GPTS_NTP_stiffmat21_.Shape(dim2_, dim1_);
      GPTS_NTP_stiffmat22_.Shape(dim2_, dim2_);


      // Get constant values from projection
      const double myeta = eta[0];
      const std::vector<double> myxi = xi[0];

      // Update shape functions and their derivatives for 1D and 2D/3D element
      Get1DShapeFunctions<double>(N1, N1_eta, myeta);
      Get2D3DShapeFunctions<double>(N2, N2_xi, myxi);

      // evaluate
      EvaluateNTPStiff(N1, N2, pp_);
    }
  }  //! constant_part_evaluated_ or !evaluate_in_ref_config_

  UpdateGPTSNTPStiff(*stiffmat11, *stiffmat12, *stiffmat21, *stiffmat22);
  //! safety check for coupling with additional porous network (= Artery coupling)
  if (coupling_element_type_ == "ARTERY")
    CheckValidVolumeFractionPressureCoupling(*stiffmat11, *stiffmat12, *stiffmat21, *stiffmat22);
  EvaluateGPTSNTPForce(*forcevec1, *forcevec2, *stiffmat11, *stiffmat12, *stiffmat21, *stiffmat22);
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateDMKappa(const std::vector<double>& eta,
    const std::vector<std::vector<double>>& xi, const std::vector<double>& segmentlengths,
    LINALG::SerialDenseMatrix* D_ele, LINALG::SerialDenseMatrix* M_ele,
    LINALG::SerialDenseVector* Kappa_ele)
{
  if (D_ele != NULL) D_ele->Shape(dim1_, dim1_);
  if (M_ele != NULL) M_ele->Shape(dim1_, dim2_);
  if (Kappa_ele != NULL) Kappa_ele->Size(dim1_);

  if (numcoupleddofs_ > 0)
  {
    // initialize
    if (!constant_part_evaluated_)
    {
      D_ = new LINALG::SerialDenseMatrix();
      M_ = new LINALG::SerialDenseMatrix();
    }
    // we only have to this once if evaluated in reference configuration
    if (!constant_part_evaluated_ or !evaluate_in_ref_config_)
    {
      // Vectors for shape functions and their derivatives
      static LINALG::Matrix<1, numnodesart_> N1(true);      // = N1
      static LINALG::Matrix<1, numnodesart_> N1_eta(true);  // = N1,eta

      static LINALG::Matrix<1, numnodescont_> N2(true);           // = N2
      static LINALG::Matrix<numdim_, numnodescont_> N2_xi(true);  // = N2,xi1

      D_.Shape(dim1_, dim1_);
      M_.Shape(dim1_, dim2_);
      Kappa_.Size(dim1_);

      const double curr_seg_length = segmentlengths[segmentid_];

      for (int i_gp = 0; i_gp < n_gp_; i_gp++)
      {
        // Get constant values from projection
        const double w_gp = wgp_[i_gp];
        const double myeta = eta[i_gp];
        const double jac = curr_seg_length / 2.0;
        const std::vector<double> myxi = xi[i_gp];

        // Update shape functions and their derivatives for 1D and 2D/3D element
        Get1DShapeFunctions<double>(N1, N1_eta, myeta);
        Get2D3DShapeFunctions<double>(N2, N2_xi, myxi);

        EvaluateDMKappa(w_gp, N1, N2, jac);
      }
    }  //! constant_part_evaluated_ or !evaluate_in_ref_config_

    UpdateDMKappa(*D_ele, *M_ele, *Kappa_ele);
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateFunctionCoupling(const std::vector<double>& eta,
    const std::vector<std::vector<double>>& xi, const std::vector<double>& segmentlengths,
    LINALG::SerialDenseVector* forcevec1, LINALG::SerialDenseVector* forcevec2,
    LINALG::SerialDenseMatrix* stiffmat11, LINALG::SerialDenseMatrix* stiffmat12,
    LINALG::SerialDenseMatrix* stiffmat21, LINALG::SerialDenseMatrix* stiffmat22,
    double& integrated_diam)
{
  // Vectors for shape functions and their derivatives
  static LINALG::Matrix<1, numnodesart_> N1(true);      // = N1
  static LINALG::Matrix<1, numnodesart_> N1_eta(true);  // = N1,eta

  static LINALG::Matrix<1, numnodescont_> N2(true);            // = N2
  static LINALG::Matrix<numnodescont_, 1> N2_transpose(true);  // = N2^T

  static LINALG::Matrix<numdim_, numnodescont_> N2_xi(true);  // = N2,xi1
  static LINALG::Matrix<numdim_, numnodescont_> derxy(true);  // = N2,xi1

  static LINALG::Matrix<numdim_, numdim_> xjm;
  static LINALG::Matrix<numdim_, numdim_> xjm0;
  static LINALG::Matrix<numdim_, numdim_> xji;

  const double curr_seg_length = segmentlengths[segmentid_];

  // case with varying diameter and type porofluid: (integral and linearizations have to be
  // calculated) --> reset
  if (diam_funct_active_ && coupltype_ == type_porofluid)
  {
    integrated_diam = 0.0;
    diam_stiffmat11_.Shape(dim1_, dim1_);
    diam_stiffmat12_.Shape(dim1_, dim2_);
  }

  for (int i_gp = 0; i_gp < n_gp_; i_gp++)
  {
    // Get constant values from projection
    const double w_gp = wgp_[i_gp];
    const double myeta = eta[i_gp];
    const std::vector<double> myxi = xi[i_gp];

    const double jac = curr_seg_length / 2.0;

    // Update shape functions and their derivatives for 1D and 2D/3D element
    Get1DShapeFunctions<double>(N1, N1_eta, myeta);
    Get2D3DShapeFunctions<double>(N2, N2_xi, myxi);
    N2_transpose.UpdateT(N2);

    xjm.MultiplyNT(N2_xi, ele2pos_);
    xjm0.MultiplyNT(N2_xi, ele2posref_);

    const double det = xji.Invert(xjm);
    // inverse of transposed jacobian "ds/dX"
    const double det0 = xjm0.Determinant();

    derxy.Multiply(xji, N2_xi);

    // determinant of deformationgradient
    // det F = det ( d x / d X ) = det (dx/ds) * ( det(dX/ds) )^-1
    const double JacobianDefGradient = det / det0;

    // clear current gauss point data for safety
    phasemanager_->ClearGPState();
    variablemanager_->EvaluateGPVariables(N2_transpose, derxy);

    phasemanager_->EvaluateGPState(JacobianDefGradient, *variablemanager_, nds_porofluid_);

    EvaluateFunctionCoupling(w_gp, N1, N2, jac, *forcevec1, *forcevec2, *stiffmat11, *stiffmat12,
        *stiffmat21, *stiffmat22, integrated_diam);
  }
}

/*---------------------------------------------------------------------------------*
 *---------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluatedNdsSolidVel(const std::vector<double>& eta,
    const std::vector<std::vector<double>>& xi, const std::vector<double>& segmentlengths,
    LINALG::SerialDenseVector& forcevec1, const double& etaA, const double& etaB)
{
  if (evaluate_in_ref_config_ || coupltype_ == type_scatra ||
      couplmethod_ == INPAR::ARTNET::ArteryPoroMultiphaseScatraCouplingMethod::ntp)
    return;

  // Vectors for shape functions and their derivatives
  static LINALG::Matrix<1, numnodesart_> N1(true);      // = N1
  static LINALG::Matrix<1, numnodesart_> N1_eta(true);  // = N1,eta

  static LINALG::Matrix<1, numnodescont_> N2(true);            // = N2
  static LINALG::Matrix<numdim_, numnodescont_> N2_xi(true);   // = N2,xi1
  static LINALG::Matrix<numdim_, numnodescont_> N2_XYZ(true);  // = N2,X
  static LINALG::Matrix<numdim_, numdim_> defgrad(true);       // = dx/dX = F

  static LINALG::Matrix<numdim_, numdim_> xjm;
  static LINALG::Matrix<numdim_, numdim_> xjm0;
  static LINALG::Matrix<numdim_, numdim_> invJ;
  static LINALG::Matrix<numdim_, numdim_> xji;

  static LINALG::Matrix<numdim_, 1> lambda_t;  // direction in current conf.

  // Evaluate $-\int_a^b d N^(1)/ds*pi*R^2 * lambda_t*v_s ds$
  //        = $-\int_\eta_a^\eta_b d N^(1)/deta*2/L_ele*pi*R^2 * lambda_t*v_s*L_seg/2.0 d\eta$
  //        = $-\int_\eta_a^\eta_b d N^(1)/deta*pi*R^2 * lambda_t*v_s*(\eta_a-\eta_b)/2.0 d\eta$
  const double jac = (etaB - etaA) / 2.0;
  for (int i_gp = 0; i_gp < n_gp_; i_gp++)
  {
    // Get constant values from projection
    const double w_gp = wgp_[i_gp];
    const double myeta = eta[i_gp];
    const std::vector<double> myxi = xi[i_gp];

    // Update shape functions and their derivatives for 1D and 2D/3D element
    Get1DShapeFunctions<double>(N1, N1_eta, myeta);
    Get2D3DShapeFunctions<double>(N2, N2_xi, myxi);

    xjm.MultiplyNT(N2_xi, ele2pos_);
    xji.Invert(xjm);

    // dX/dpsi
    xjm0.MultiplyNT(N2_xi, ele2posref_);
    // dpsi/dX
    // note: cannot use invJ_ here -> defined at original Gauss points
    invJ.Invert(xjm0);
    // dN/dX = dN/dxi * dxi/dX = dN/dxi * (dX/dxi)^-1
    N2_XYZ.Multiply(invJ, N2_xi);
    // dx/dX = x * N_XYZ^T
    defgrad.MultiplyNT(ele2pos_, N2_XYZ);

    // current direction of artery element at GP
    lambda_t.Multiply(defgrad, lambda0_);
    lambda_t.Scale(1.0 / lambda_t.Norm2());

    std::vector<double> myvel(3, 0.0);
    for (unsigned int j = 0; j < numnodescont_; j++)
      for (unsigned int idim = 0; idim < numdim_; idim++) myvel[idim] += N2(j) * ele2vel_(idim, j);

    double lambda_t_vel = 0.0;
    for (unsigned int idim = 0; idim < numdim_; idim++)
      lambda_t_vel += myvel[idim] * lambda_t(idim);

    // TODO: here reference diameter is taken
    for (unsigned int i = 0; i < numnodesart_; i++)
      forcevec1(i) +=
          N1_eta(i) * w_gp * jac * lambda_t_vel * arterydiamref_ * arterydiamref_ * M_PI / 4.0;
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateGPTSStiff(const double& w_gp, const LINALG::Matrix<1, numnodesart_>& N1,
    const LINALG::Matrix<1, numnodescont_>& N2, const double& jacobi, const double& pp)
{
  // Evaluate meshtying stiffness for artery element N_1^T * N_1
  for (unsigned int i = 0; i < numnodesart_; i++)
  {
    for (unsigned int j = 0; j < numnodesart_; j++)
    {
      const double stiff = timefacrhs_art_ * pp * jacobi * w_gp * N1(i) * N1(j);
      for (int dof = 0; dof < numcoupleddofs_; dof++)
        GPTS_NTP_stiffmat11_(i * numdof_art_ + coupleddofs_art_[dof],
            j * numdof_art_ + coupleddofs_art_[dof]) += stiff;
    }
  }

  // Evaluate meshtying stiffness for artery element "mixed" N_1^T * (-N_2)
  for (unsigned int i = 0; i < numnodesart_; i++)
  {
    for (unsigned int j = 0; j < numnodescont_; j++)
    {
      const double stiff = timefacrhs_art_ * pp * jacobi * w_gp * N1(i) * (-N2(j));
      for (int dof = 0; dof < numcoupleddofs_; dof++)
        GPTS_NTP_stiffmat12_(i * numdof_art_ + coupleddofs_art_[dof],
            j * numdof_cont_ + coupleddofs_cont_[dof]) += stiff;
    }
  }

  // Evaluate meshtying stiffness for continuous element "mixed" N_2^T * (-N_1)
  for (unsigned int i = 0; i < numnodescont_; i++)
  {
    for (unsigned int j = 0; j < numnodesart_; j++)
    {
      const double stiff = timefacrhs_cont_ * pp * jacobi * w_gp * N2(i) * (-N1(j));
      for (int dof = 0; dof < numcoupleddofs_; dof++)
        GPTS_NTP_stiffmat21_(i * numdof_cont_ + coupleddofs_cont_[dof],
            j * numdof_art_ + coupleddofs_art_[dof]) += stiff;
    }
  }

  // Evaluate meshtying stiffness for continuous element N_2^T * N_2
  for (unsigned int i = 0; i < numnodescont_; i++)
  {
    for (unsigned int j = 0; j < numnodescont_; j++)
    {
      const double stiff = timefacrhs_cont_ * pp * jacobi * w_gp * N2(i) * N2(j);
      for (int dof = 0; dof < numcoupleddofs_; dof++)
        GPTS_NTP_stiffmat22_(i * numdof_cont_ + coupleddofs_cont_[dof],
            j * numdof_cont_ + coupleddofs_cont_[dof]) += stiff;
    }
  }

  constant_part_evaluated_ = true;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateNTPStiff(const LINALG::Matrix<1, numnodesart_>& N1,
    const LINALG::Matrix<1, numnodescont_>& N2, const double& pp)
{
  // Evaluate meshtying stiffness for artery element N_1^T * N_1
  for (unsigned int i = 0; i < numnodesart_; i++)
  {
    for (unsigned int j = 0; j < numnodesart_; j++)
    {
      const double stiff = timefacrhs_art_ * pp * N1(i) * N1(j);
      for (int dof = 0; dof < numcoupleddofs_; dof++)
        GPTS_NTP_stiffmat11_(i * numdof_art_ + coupleddofs_art_[dof],
            j * numdof_art_ + coupleddofs_art_[dof]) += stiff;
    }
  }

  // Evaluate meshtying stiffness for artery element "mixed" N_1^T * (-N_2)
  for (unsigned int i = 0; i < numnodesart_; i++)
  {
    for (unsigned int j = 0; j < numnodescont_; j++)
    {
      const double stiff = timefacrhs_art_ * pp * N1(i) * (-N2(j));
      for (int dof = 0; dof < numcoupleddofs_; dof++)
        GPTS_NTP_stiffmat12_(i * numdof_art_ + coupleddofs_art_[dof],
            j * numdof_cont_ + coupleddofs_cont_[dof]) += stiff;
    }
  }

  // Evaluate meshtying stiffness for continuous element "mixed" N_2^T * (-N_1)
  for (unsigned int i = 0; i < numnodescont_; i++)
  {
    for (unsigned int j = 0; j < numnodesart_; j++)
    {
      const double stiff = timefacrhs_cont_ * pp * N2(i) * (-N1(j));
      for (int dof = 0; dof < numcoupleddofs_; dof++)
        GPTS_NTP_stiffmat21_(i * numdof_cont_ + coupleddofs_cont_[dof],
            j * numdof_art_ + coupleddofs_art_[dof]) += stiff;
    }
  }

  // Evaluate meshtying stiffness for continuous element N_2^T * N_2
  for (unsigned int i = 0; i < numnodescont_; i++)
  {
    for (unsigned int j = 0; j < numnodescont_; j++)
    {
      const double stiff = timefacrhs_cont_ * pp * N2(i) * N2(j);
      for (int dof = 0; dof < numcoupleddofs_; dof++)
        GPTS_NTP_stiffmat22_(i * numdof_cont_ + coupleddofs_cont_[dof],
            j * numdof_cont_ + coupleddofs_cont_[dof]) += stiff;
    }
  }

  constant_part_evaluated_ = true;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateDMKappa(const double& w_gp, const LINALG::Matrix<1, numnodesart_>& N1,
    const LINALG::Matrix<1, numnodescont_>& N2, const double& jacobi)
{
  // Evaluate element mortar coupling operator kappa = N_1
  for (unsigned int inode = 0; inode < numnodesart_; inode++)
  {
    const double mykappa = w_gp * jacobi * N1(inode);
    for (int dof = 0; dof < numdof_art_; dof++) Kappa_(inode * numdof_art_ + dof) += mykappa;
  }

  // Evaluate element mortar coupling operator D = N_1^T * N_1
  for (unsigned int i = 0; i < numnodesart_; i++)
  {
    for (unsigned int j = 0; j < numnodesart_; j++)
    {
      const double D = jacobi * w_gp * N1(i) * N1(j);
      for (int dof = 0; dof < numcoupleddofs_; dof++)
        D_(i * numdof_art_ + coupleddofs_art_[dof], j * numdof_art_ + coupleddofs_art_[dof]) += D;
    }
  }

  // Evaluate element mortar coupling operator M = N_1^T * N_2
  for (unsigned int i = 0; i < numnodesart_; i++)
  {
    for (unsigned int j = 0; j < numnodescont_; j++)
    {
      const double M = jacobi * w_gp * N1(i) * N2(j);
      for (int dof = 0; dof < numcoupleddofs_; dof++)
        M_(i * numdof_art_ + coupleddofs_art_[dof], j * numdof_cont_ + coupleddofs_cont_[dof]) += M;
    }
  }

  constant_part_evaluated_ = true;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateGPTSNTPForce(LINALG::SerialDenseVector& forcevec1,
    LINALG::SerialDenseVector& forcevec2, const LINALG::SerialDenseMatrix& stiffmat11,
    const LINALG::SerialDenseMatrix& stiffmat12, const LINALG::SerialDenseMatrix& stiffmat21,
    const LINALG::SerialDenseMatrix& stiffmat22)
{
  // Evaluate meshtying forces for artery element
  for (int i = 0; i < dim1_; i++)
    for (int j = 0; j < dim1_; j++) forcevec1(i) -= stiffmat11(i, j) * artelephinp_[j];

  for (int i = 0; i < dim1_; i++)
    for (int j = 0; j < dim2_; j++) forcevec1(i) -= stiffmat12(i, j) * contelephinp_[j];

  // Evaluate meshtying forces for continuous-dis element
  for (int i = 0; i < dim2_; i++)
    for (int j = 0; j < dim1_; j++) forcevec2(i) -= stiffmat21(i, j) * artelephinp_[j];

  for (int i = 0; i < dim2_; i++)
    for (int j = 0; j < dim2_; j++) forcevec2(i) -= stiffmat22(i, j) * contelephinp_[j];
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::UpdateGPTSNTPStiff(LINALG::SerialDenseMatrix& stiffmat11,
    LINALG::SerialDenseMatrix& stiffmat12, LINALG::SerialDenseMatrix& stiffmat21,
    LINALG::SerialDenseMatrix& stiffmat22)
{
  stiffmat11.Update(1.0, GPTS_NTP_stiffmat11_, 0.0);
  stiffmat12.Update(1.0, GPTS_NTP_stiffmat12_, 0.0);
  stiffmat21.Update(1.0, GPTS_NTP_stiffmat21_, 0.0);
  stiffmat22.Update(1.0, GPTS_NTP_stiffmat22_, 0.0);
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::CheckValidVolumeFractionPressureCoupling(LINALG::SerialDenseMatrix& stiffmat11,
    LINALG::SerialDenseMatrix& stiffmat12, LINALG::SerialDenseMatrix& stiffmat21,
    LINALG::SerialDenseMatrix& stiffmat22)
{
  for (int idof = 0; idof < numcoupleddofs_; idof++)
  {
    if (!variablemanager_->ElementHasValidVolFracPressure(
            volfracpressid_[idof] - numfluidphases_ - numvolfrac_))
    {
      // reset to zero for this dof
      for (unsigned int i = 0; i < numnodesart_; i++)
      {
        for (unsigned int j = 0; j < numnodesart_; j++)
          stiffmat11(i * numdof_art_ + coupleddofs_art_[idof],
              j * numdof_art_ + coupleddofs_art_[idof]) = 0.0;
      }

      for (unsigned int i = 0; i < numnodesart_; i++)
      {
        for (unsigned int j = 0; j < numnodescont_; j++)
          stiffmat12(i * numdof_art_ + coupleddofs_art_[idof],
              j * numdof_cont_ + coupleddofs_cont_[idof]) = 0.0;
      }

      for (unsigned int i = 0; i < numnodescont_; i++)
      {
        for (unsigned int j = 0; j < numnodesart_; j++)
          stiffmat21(i * numdof_cont_ + coupleddofs_cont_[idof],
              j * numdof_art_ + coupleddofs_art_[idof]) = 0.0;
      }

      for (unsigned int i = 0; i < numnodescont_; i++)
      {
        for (unsigned int j = 0; j < numnodescont_; j++)
          stiffmat22(i * numdof_cont_ + coupleddofs_cont_[idof],
              j * numdof_cont_ + coupleddofs_cont_[idof]) = 0.0;
      }
    }
  }
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::UpdateDMKappa(LINALG::SerialDenseMatrix& D_ele, LINALG::SerialDenseMatrix& M_ele,
    LINALG::SerialDenseVector& Kappa_ele)
{
  D_ele.Update(1.0, D_, 0.0);
  M_ele.Update(1.0, M_, 0.0);
  Kappa_ele.Update(1.0, Kappa_, 0.0);

  for (int idof = 0; idof < numcoupleddofs_; idof++)
  {
    // this coupling is only possible if we also have an element with a valid volume fraction
    // pressure, i.e., if we also have a smeared representation of the neovasculature at this
    // point if not ---> corresponding matrices are set to zero
    if (!variablemanager_->ElementHasValidVolFracPressure(
            volfracpressid_[idof] - numfluidphases_ - numvolfrac_))
    {
      // reset to zero for this dof
      for (unsigned int i = 0; i < numnodesart_; i++)
        for (unsigned int j = 0; j < numnodesart_; j++)
          D_ele(i * numdof_art_ + coupleddofs_art_[idof],
              j * numdof_art_ + coupleddofs_art_[idof]) = 0.0;

      for (unsigned int i = 0; i < numnodesart_; i++)
        for (unsigned int j = 0; j < numnodescont_; j++)
          M_ele(i * numdof_art_ + coupleddofs_art_[idof],
              j * numdof_cont_ + coupleddofs_cont_[idof]) = 0.0;
    }
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateFunctionCoupling(const double& w_gp, const LINALG::Matrix<1, numnodesart_>& N1,
    const LINALG::Matrix<1, numnodescont_>& N2, const double& jacobi,
    LINALG::SerialDenseVector& forcevec1, LINALG::SerialDenseVector& forcevec2,
    LINALG::SerialDenseMatrix& stiffmat11, LINALG::SerialDenseMatrix& stiffmat12,
    LINALG::SerialDenseMatrix& stiffmat21, LINALG::SerialDenseMatrix& stiffmat22,
    double& integrated_diam)
{
  // resize
  std::vector<double> artscalarnpAtGP(numscalart_, 0.0);
  std::vector<double> contscalarnpAtGP(numscalcont_, 0.0);
  double artpressAtGP = 0.0;

  // get artery values at GP
  GetArteryValuesAtGP(N1, artpressAtGP, artscalarnpAtGP);
  // get scatra values at GP
  GetContScalarValuesAtGP(N2, contscalarnpAtGP);
  // NOTE: values of fluid held by managers

  if (diam_funct_active_)
  {
    EvaluateDiamFunctionAndDeriv(artpressAtGP, w_gp, N1, N2, jacobi);
    // integral is only calculated in this case
    if (coupltype_ == type_porofluid) integrated_diam += w_gp * jacobi * arterydiamAtGP_;
  }

  // artery functions
  for (int i_art = 0; i_art < numdof_art_; i_art++)
  {
    if (funct_vec_[0][i_art] != 0)
    {
      // resize
      std::vector<double> artderivs(numdof_art_, 0.0);
      std::vector<double> contderivs(numdof_cont_, 0.0);
      double functval = 0.0;
      // evaluate and assemble
      EvaluateFunctionAndDeriv(*funct_vec_[0][i_art], artpressAtGP, artscalarnpAtGP,
          contscalarnpAtGP, functval, artderivs, contderivs);
      AssembleFunctionCouplingIntoForceStiffArt(i_art, w_gp, N1, N2, jacobi, scale_vec_[0][i_art],
          functval, artderivs, contderivs, forcevec1, stiffmat11, stiffmat12);
    }
  }
  // continuous discretization functions
  for (int i_cont = 0; i_cont < numdof_cont_; i_cont++)
  {
    if (funct_vec_[1][i_cont] != 0)
    {
      // resize
      std::vector<double> artderivs(numdof_art_, 0.0);
      std::vector<double> contderivs(numdof_cont_, 0.0);
      double functval = 0.0;
      // evaluate and assemble
      EvaluateFunctionAndDeriv(*funct_vec_[1][i_cont], artpressAtGP, artscalarnpAtGP,
          contscalarnpAtGP, functval, artderivs, contderivs);
      AssembleFunctionCouplingIntoForceStiffCont(cont_dofs_to_assemble_functions_into_[i_cont],
          w_gp, N1, N2, jacobi, scale_vec_[1][i_cont], timefacrhs_cont_dens_[i_cont], functval,
          artderivs, contderivs, forcevec2, stiffmat21, stiffmat22);
    }
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateDiamFunctionAndDeriv(const double artpressnpAtGP, const double& w_gp,
    const LINALG::Matrix<1, numnodesart_>& N1, const LINALG::Matrix<1, numnodescont_>& N2,
    const double& jacobi)
{
  // we have to derive w.r.t. fluid variables
  std::vector<std::pair<std::string, double>> variables;
  variables.reserve(numfluidphases_ + numfluidphases_ + 1 + numvolfrac_ + numvolfrac_ + 1);

  // reference diameter is constant
  std::vector<std::pair<std::string, double>> constants;
  constants.reserve(2);
  constants.push_back(std::pair<std::string, double>("D0", arterydiamref_));
  constants.push_back(std::pair<std::string, double>("Dprev", arterymat_->DiamPreviousTimeStep()));

  // set fluid values as variables
  SetFluidValuesAsVariables(variables, artpressnpAtGP);

  // evaluate the diameter at GP by evaluating the function
  arterydiamAtGP_ = artdiam_funct_->Evaluate(variables, constants, 0);

  // derivatives and linearizations are so far only calculated for coupltype porofluid
  if (coupltype_ == type_porofluid)
  {
    // function derivatives
    std::vector<double> curderivs(artdiam_funct_->EvaluateDerivative(variables, constants, 0));
    // derivatives w.r.t. primary variables
    std::fill(diamderivs_.begin(), diamderivs_.end(), 0.0);

    // diameter derivatives w.r.t. saturations and pressures
    for (int doftoderive = 0; doftoderive < numfluidphases_; doftoderive++)
    {
      for (int idof = 0; idof < numfluidphases_; idof++)
        diamderivs_[doftoderive] +=
            curderivs[idof] * phasemanager_->PressureDeriv(idof, doftoderive) +
            curderivs[idof + numfluidphases_] * phasemanager_->SaturationDeriv(idof, doftoderive);
      if (phasemanager_->PorosityDependsOnFluid())
        diamderivs_[doftoderive] +=
            curderivs[2 * numfluidphases_] * phasemanager_->PorosityDeriv(doftoderive);
    }
    // diameter derivs w.r.t. to volume fraction phases
    for (int doftoderive = numfluidphases_; doftoderive < numdof_cont_ - numvolfrac_; doftoderive++)
    {
      // diameter derivatives w.r.t. volume fractions directly appearing
      //                             and porosity (since it depends on volfrac)
      diamderivs_[doftoderive] +=
          curderivs[doftoderive + numfluidphases_ + 1] +
          curderivs[2 * numfluidphases_] * phasemanager_->PorosityDeriv(doftoderive);
    }
    // diameter derivs w.r.t. to volume fraction pressures
    for (int doftoderive = numfluidphases_ + numvolfrac_; doftoderive < numdof_cont_; doftoderive++)
      diamderivs_[doftoderive] += curderivs[doftoderive + numfluidphases_ + 1];

    // diameter derivs w.r.t. to artery pressure
    diamderivs_[numfluidphases_ + 2 * numvolfrac_] +=
        curderivs[numfluidphases_ + numfluidphases_ + 1 + numvolfrac_ + numvolfrac_];


    // Now the derivative of the integrated (element) diameter needed in the Hagen-Poiseuille
    // terms is built and stored in the respective stiffness matrices for later use

    // pre-compute some values
    const double pressgrad = (artelephinp_[1] - artelephinp_[0]) / arteryelelength_;
    std::vector<double> dummy = {-1.0, 1.0};
    const double preprefac = pressgrad * w_gp * jacobi / arteryelelength_;

    // assemble into the respective stiffness matrices
    for (unsigned int i = 0; i < numnodesart_; i++)
    {
      // build diameter stiffness matrix w.r.t. artery primary variables
      const double prefac_art =
          dummy[i] * diamderivs_[numfluidphases_ + 2 * numvolfrac_] * preprefac;
      for (unsigned int j = 0; j < numnodesart_; j++) diam_stiffmat11_(i, j) += prefac_art * N1(j);

      // build diameter stiffness matrix w.r.t. 2D/3D primary variables
      const double prefac_cont = dummy[i] * preprefac;
      for (unsigned int j = 0; j < numnodescont_; j++)
      {
        const double prefac_cont2 = prefac_cont * N2(j);
        for (int j_cont = 0; j_cont < numdof_cont_; j_cont++)
          diam_stiffmat12_(i, j * numdof_cont_ + j_cont) += prefac_cont2 * diamderivs_[j_cont];
      }
    }
  }
}
/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::SetTimeFacRhs(const double& arterydensity, Teuchos::RCP<MAT::MatList> contscatramat,
    const double& timefacrhs_art, const double& timefacrhs_cont)
{
  // set
  timefacrhs_art_ = timefacrhs_art;
  timefacrhs_cont_ = timefacrhs_cont;

  // resize
  timefacrhs_cont_dens_.resize(numdof_cont_);

  // fill fluid densities vector
  std::vector<double> fluiddensities(numfluidphases_ + 2 * numvolfrac_);
  for (int ifluidphase = 0; ifluidphase < numfluidphases_; ifluidphase++)
    fluiddensities[ifluidphase] = phasemanager_->Density(ifluidphase);
  for (int ivolfrac = 0; ivolfrac < numvolfrac_; ivolfrac++)
  {
    fluiddensities[numfluidphases_ + ivolfrac] = phasemanager_->VolFracDensity(ivolfrac);
    fluiddensities[numfluidphases_ + numvolfrac_ + ivolfrac] =
        phasemanager_->VolFracDensity(ivolfrac);
  }

  switch (coupltype_)
  {
    case type_porofluid:
    {
      // artery
      timefacrhs_art_dens_ = timefacrhs_art_ / arterydensity;
      // continuous
      for (int idof = 0; idof < numdof_cont_; idof++)
        timefacrhs_cont_dens_[idof] = timefacrhs_cont_ / fluiddensities[idof];

      break;
    }
    case type_scatra:
    {
      // artery
      timefacrhs_art_dens_ = timefacrhs_art_ / arterydensity;
      // continuous
      for (int idof = 0; idof < numdof_cont_; idof++)
      {
        const int matid = contscatramat->MatID(idof);
        Teuchos::RCP<MAT::Material> singlemat = contscatramat->MaterialById(matid);
        int phaseid = -1;
        if (singlemat->MaterialType() == INPAR::MAT::m_scatra_multiporo_fluid)
        {
          const Teuchos::RCP<const MAT::ScatraMatMultiPoroFluid>& poromat =
              Teuchos::rcp_dynamic_cast<const MAT::ScatraMatMultiPoroFluid>(singlemat);
          phaseid = poromat->PhaseID();
        }
        else if (singlemat->MaterialType() == INPAR::MAT::m_scatra_multiporo_volfrac)
        {
          const Teuchos::RCP<const MAT::ScatraMatMultiPoroVolFrac>& poromat =
              Teuchos::rcp_dynamic_cast<const MAT::ScatraMatMultiPoroVolFrac>(singlemat);
          phaseid = poromat->PhaseID();
        }
        else
          dserror(
              "Only MAT::ScatraMatMultiPoroVolFrac and MAT::ScatraMatMultiPoroFluid, your "
              "material "
              "is of type %d",
              singlemat->MaterialType());
        timefacrhs_cont_dens_[idof] = timefacrhs_cont_ / fluiddensities[phaseid];
      }
      break;
    }
    default:
      dserror("Unknown coupling type");
      break;
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::ExtractSolidVel(Teuchos::RCP<DRT::Discretization> contdis)
{
  // no need for this
  if (evaluate_in_ref_config_) return;

  Teuchos::RCP<const Epetra_Vector> velocity = contdis->GetState(1, "velocity field");
  DRT::Element::LocationArray la(contdis->NumDofSets());
  element2_->LocationVector(*contdis, la, false);

  // construct location vector for displacement related dofs
  std::vector<int> lmdisp(numdim_ * numnodescont_, -1);
  for (unsigned int inode = 0; inode < numnodescont_; ++inode)
    for (unsigned int idim = 0; idim < numdim_; ++idim)
      lmdisp[inode * numdim_ + idim] = la[1].lm_[inode * numdim_ + idim];

  // extract local values of displacement field from global state vector
  DRT::UTILS::ExtractMyValues<LINALG::Matrix<numdim_, numnodescont_>>(*velocity, ele2vel_, lmdisp);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
FAD POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::IntegrateLengthToEtaS(const FAD& eta_s)
{
  FAD length = 0.0;

  // define GPs
  DRT::UTILS::IntegrationPoints1D gaussPoints =
      DRT::UTILS::IntegrationPoints1D(DRT::UTILS::GaussRule1D::line_3point);
  if (numdim_ == 3)
    gaussPoints = DRT::UTILS::IntegrationPoints1D(DRT::UTILS::GaussRule1D::line_4point);

  static LINALG::Matrix<1, numnodescont_, FAD> N2(true);           // = N2
  static LINALG::Matrix<numdim_, numnodescont_, FAD> N2_xi(true);  // = N2,xi1

  static LINALG::Matrix<numdim_, numdim_, FAD> InvJ(true);          // (dX/dxi)^-1
  static LINALG::Matrix<numdim_, numdim_, FAD> defGrad(true);       // (dX/dx) = F
  static LINALG::Matrix<numdim_, numnodescont_, FAD> N2_XYZ(true);  // = N2,X
  static LINALG::Matrix<numdim_, 1, FAD> Ft0(true);                 // = F*t0

  // Vectors for shape functions and their derivatives
  static LINALG::Matrix<1, numnodesart_, FAD> N1(true);      // = N1
  static LINALG::Matrix<1, numnodesart_, FAD> N1_eta(true);  // = N1,eta
  // Coords and derivatives of 1D and 2D/3D element
  static LINALG::Matrix<numdim_, 1, FAD> r1(true);      // = r1
  static LINALG::Matrix<numdim_, 1, FAD> r1_eta(true);  // = r1,eta

  // t0
  static LINALG::Matrix<numdim_, 1, FAD> t0;
  for (unsigned int i = 0; i < numdim_; i++) t0(i).val() = lambda0_(i);
  // ele2posref
  static LINALG::Matrix<numdim_, numnodescont_, FAD> ele2posref;
  for (unsigned int i = 0; i < numdim_; i++)
    for (unsigned int j = 0; j < numnodescont_; j++) ele2posref(i, j).val() = ele2posref_(i, j);
  // ele2pos
  static LINALG::Matrix<numdim_, numnodescont_, FAD> ele2pos;
  for (unsigned int i = 0; i < numdim_; i++)
    for (unsigned int j = 0; j < numnodescont_; j++) ele2pos(i, j).val() = ele2pos_(i, j);

  const FAD determinant = (eta_s - eta_a_) / 2.0;
  const FAD jacobi = determinant * arteryelelengthref_ / 2.0;

  // integrate from etaA to eta_s
  for (int i_gp = 0; i_gp < n_gp_; i_gp++)
  {
    const double w_gp = wgp_[i_gp];
    const FAD eta = (eta_s + eta_a_) / 2.0 + gaussPoints.qxg[i_gp][0] * determinant;
    // Update coordinates and derivatives for 1D and 2D/3D element
    Get1DShapeFunctions<FAD>(N1, N1_eta, eta);
    ComputeArteryCoordsAndDerivsRef<FAD>(r1, r1_eta, N1, N1_eta);

    // project
    bool projection_valid = false;
    std::vector<FAD> xi(numdim_, 0.0);
    Projection<FAD>(r1, xi, projection_valid);
    if (!projection_valid) dserror("Gauss point could not be projected");

    Get2D3DShapeFunctions<FAD>(N2, N2_xi, xi);

    InvJ.MultiplyNT(N2_xi, ele2posref);
    InvJ.Invert();

    // dN/dX = dN/dxi * dxi/dX = dN/dxi * (dX/dxi)^-1
    N2_XYZ.Multiply(InvJ, N2_xi);
    // dx/dX = x * N_XYZ^T
    defGrad.MultiplyNT(ele2pos, N2_XYZ);
    Ft0.Multiply(defGrad, t0);
    const FAD Ft0Norm = FADUTILS::VectorNorm(Ft0);
    // finally get the length
    length += Ft0Norm * w_gp * jacobi;
  }

  return length;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::GetArteryValuesAtGP(const LINALG::Matrix<1, numnodesart_>& N1, double& artpress,
    std::vector<double>& artscalar)
{
  switch (coupltype_)
  {
    case type_porofluid:
    {
      for (unsigned int i = 0; i < numnodesart_; i++)
      {
        artpress += N1(i) * artelephinp_[i];
        for (int i_scal = 0; i_scal < numscalart_; i_scal++)
          artscalar[i_scal] += N1(i) * eartscalarnp_[i_scal](i);
      }
      break;
    }
    case type_scatra:
    {
      for (unsigned int i = 0; i < numnodesart_; i++)
      {
        artpress += N1(i) * earterypressurenp_(i);
        for (int i_art = 0; i_art < numdof_art_; i_art++)
          artscalar[i_art] += N1(i) * artelephinp_[i * numdof_art_ + i_art];
      }
      break;
    }
    default:
      dserror("Unknown coupling type");
      break;
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::GetContScalarValuesAtGP(const LINALG::Matrix<1, numnodescont_>& N2,
    std::vector<double>& contscalarnp)
{
  switch (coupltype_)
  {
    case type_porofluid:
    {
      for (unsigned int i = 0; i < numnodescont_; i++)
      {
        for (int i_cont = 0; i_cont < numscalcont_; i_cont++)
          contscalarnp[i_cont] += N2(i) * econtscalarnp_[i_cont](i);
      }
      break;
    }
    case type_scatra:
    {
      for (unsigned int i = 0; i < numnodescont_; i++)
      {
        for (int i_cont = 0; i_cont < numdof_cont_; i_cont++)
          contscalarnp[i_cont] += N2(i) * contelephinp_[i * numdof_cont_ + i_cont];
      }
      break;
    }
    default:
      dserror("Unknown coupling type");
      break;
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::AssembleFunctionCouplingIntoForceStiffArt(const int& i_art, const double& w_gp,
    const LINALG::Matrix<1, numnodesart_>& N1, const LINALG::Matrix<1, numnodescont_>& N2,
    const double& jacobi, const int& scale, const double& functval,
    const std::vector<double>& artderivs, const std::vector<double>& contderivs,
    LINALG::SerialDenseVector& forcevec1, LINALG::SerialDenseMatrix& stiffmat11,
    LINALG::SerialDenseMatrix& stiffmat12)
{
  const double myscale = (double)(scale);

  // rhs ---> +
  for (unsigned int i = 0; i < numnodesart_; i++)
    forcevec1(i * numdof_art_ + i_art) +=
        N1(i) * myscale * w_gp * jacobi * functval * timefacrhs_art_dens_;

  // matrix --> -
  for (unsigned int i = 0; i < numnodesart_; i++)
    for (unsigned int j = 0; j < numnodesart_; j++)
      for (int j_art = 0; j_art < numdof_art_; j_art++)
        stiffmat11(i * numdof_art_ + i_art, j * numdof_art_ + j_art) -=
            N1(i) * N1(j) * myscale * w_gp * jacobi * artderivs[j_art] * timefacrhs_art_dens_;

  // matrix --> -
  for (unsigned int i = 0; i < numnodesart_; i++)
    for (unsigned int j = 0; j < numnodescont_; j++)
      for (int j_cont = 0; j_cont < numdof_cont_; j_cont++)
        stiffmat12(i * numdof_art_ + i_art, j * numdof_cont_ + j_cont) -=
            N1(i) * N2(j) * myscale * w_gp * jacobi * contderivs[j_cont] * timefacrhs_art_dens_;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::AssembleFunctionCouplingIntoForceStiffCont(const std::vector<int>& assembleInto,
    const double& w_gp, const LINALG::Matrix<1, numnodesart_>& N1,
    const LINALG::Matrix<1, numnodescont_>& N2, const double& jacobi, const int& scale,
    const double& timefacrhs_cont, const double& functval, const std::vector<double>& artderivs,
    const std::vector<double>& contderivs, LINALG::SerialDenseVector& forcevec2,
    LINALG::SerialDenseMatrix& stiffmat21, LINALG::SerialDenseMatrix& stiffmat22)
{
  const double myscale = (double)(scale);

  // rhs ---> +
  for (unsigned int i = 0; i < numnodescont_; i++)
  {
    const double rhsval = N2(i) * myscale * w_gp * jacobi * functval * timefacrhs_cont;
    for (unsigned int idof = 0; idof < assembleInto.size(); idof++)
      forcevec2(i * numdof_cont_ + assembleInto[idof]) += rhsval;
  }

  // matrix --> -
  for (unsigned int i = 0; i < numnodescont_; i++)
    for (unsigned int j = 0; j < numnodesart_; j++)
    {
      const double massmatrixfac = N2(i) * N1(j) * myscale * w_gp * jacobi * timefacrhs_cont;
      for (int j_art = 0; j_art < numdof_art_; j_art++)
      {
        const double stiffval = massmatrixfac * artderivs[j_art];
        for (unsigned int idof = 0; idof < assembleInto.size(); idof++)
          stiffmat21(i * numdof_cont_ + assembleInto[idof], j * numdof_art_ + j_art) -= stiffval;
      }
    }

  // matrix --> -
  for (unsigned int i = 0; i < numnodescont_; i++)
    for (unsigned int j = 0; j < numnodescont_; j++)
    {
      const double massmatrixfac = N2(i) * N2(j) * myscale * w_gp * jacobi * timefacrhs_cont;
      for (int j_cont = 0; j_cont < numdof_cont_; j_cont++)
      {
        const double stiffval = massmatrixfac * contderivs[j_cont];
        for (unsigned int idof = 0; idof < assembleInto.size(); idof++)
          stiffmat22(i * numdof_cont_ + assembleInto[idof], j * numdof_cont_ + j_cont) -= stiffval;
      }
    }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateFunctionAndDeriv(DRT::UTILS::FunctionOfAnything& funct,
    const double& artpressnpAtGP, const std::vector<double>& artscalarnpAtGP,
    const std::vector<double>& scalarnpAtGP, double& functval, std::vector<double>& artderivs,
    std::vector<double>& contderivs)
{
  switch (coupltype_)
  {
    case type_porofluid:
    {
      // we have to derive w.r.t. fluid variables plus diameter
      std::vector<std::pair<std::string, double>> variables;
      variables.reserve(numfluidphases_ + numfluidphases_ + 1 + numvolfrac_ + numvolfrac_ + 1 + 1);

      // scalar variables are constants (plus reference artery diameter and diameter of previous
      // time step)
      std::vector<std::pair<std::string, double>> constants;
      constants.reserve(numscalcont_ + numscalart_ + 2);

      SetScalarValuesAsConstants(constants, artscalarnpAtGP, scalarnpAtGP);

      SetFluidValuesAsVariables(variables, artpressnpAtGP);

      // set reference artery diameter as constant
      constants.push_back(std::pair<std::string, double>("D0", arterydiamref_));

      // set artery diameter of previous time step as constant
      constants.push_back(
          std::pair<std::string, double>("Dprev", arterymat_->DiamPreviousTimeStep()));

      // set artery diameter as variable
      variables.push_back(std::pair<std::string, double>("D", arterydiamAtGP_));

      // evaluate the reaction term
      functval = funct.Evaluate(variables, constants, 0);
      // evaluate derivatives
      std::vector<double> curderivs(funct.EvaluateDerivative(variables, constants, 0));

      EvaluateFluidDerivs(artderivs, contderivs, curderivs);

      break;
    }
    case type_scatra:
    {
      // scalars (both cont and art) are variables
      std::vector<std::pair<std::string, double>> variables;
      variables.reserve(numscalcont_ + numscalart_);

      // fluid variables are constants
      std::vector<std::pair<std::string, double>> constants;
      constants.reserve(
          numfluidphases_ + numfluidphases_ + 1 + numvolfrac_ + numvolfrac_ + 1 + 1 + 1 + 1);

      SetScalarValuesAsVariables(variables, artscalarnpAtGP, scalarnpAtGP);

      SetFluidValuesAsConstants(constants, artpressnpAtGP);

      // evaluate the reaction term
      functval = funct.Evaluate(variables, constants, 0);
      // evaluate derivatives
      std::vector<double> curderivs(funct.EvaluateDerivative(variables, constants, 0));

      EvaluateScalarDerivs(artderivs, contderivs, curderivs);

      break;
    }
    default:
      dserror("Unknown coupling type");
      break;
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::SetScalarValuesAsConstants(std::vector<std::pair<std::string, double>>& constants,
    const std::vector<double>& artscalarnpAtGP, const std::vector<double>& scalarnpAtGP)
{
  // set scalar values as constant
  for (int k = 0; k < numscalcont_; k++)
    constants.push_back(std::pair<std::string, double>(scalarnames_[k], scalarnpAtGP[k]));

  // set artery-scalar values as constant
  for (int k = 0; k < numscalart_; k++)
    constants.push_back(std::pair<std::string, double>(artscalarnames_[k], artscalarnpAtGP[k]));
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::SetFluidValuesAsVariables(std::vector<std::pair<std::string, double>>& variables,
    const double& artpressnpAtGP)
{
  // set pressure values as variable
  for (int k = 0; k < numfluidphases_; k++)
    variables.push_back(
        std::pair<std::string, double>(pressurenames_[k], phasemanager_->Pressure(k)));

  // set saturation values as variable
  for (int k = 0; k < numfluidphases_; k++)
    variables.push_back(
        std::pair<std::string, double>(saturationnames_[k], phasemanager_->Saturation(k)));

  // set porosity value as variable
  variables.push_back(std::pair<std::string, double>(porosityname_, phasemanager_->Porosity()));

  // set volfrac values as variables
  for (int k = 0; k < numvolfrac_; k++)
    variables.push_back(
        std::pair<std::string, double>(volfracnames_[k], phasemanager_->VolFrac(k)));

  // set volfrac pressure values as variables
  for (int k = 0; k < numvolfrac_; k++)
    variables.push_back(std::pair<std::string, double>(
        volfracpressurenames_[k], phasemanager_->VolFracPressure(k)));

  // set artery pressure as variable
  variables.push_back(std::pair<std::string, double>(artpressname_, artpressnpAtGP));
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::SetFluidValuesAsConstants(std::vector<std::pair<std::string, double>>& constants,
    const double& artpressnpAtGP)
{
  // set pressure values as constants
  for (int k = 0; k < numfluidphases_; k++)
    constants.push_back(
        std::pair<std::string, double>(pressurenames_[k], phasemanager_->Pressure(k)));

  // set saturation values as constants
  for (int k = 0; k < numfluidphases_; k++)
    constants.push_back(
        std::pair<std::string, double>(saturationnames_[k], phasemanager_->Saturation(k)));

  // set porosity value as constants
  constants.push_back(std::pair<std::string, double>(porosityname_, phasemanager_->Porosity()));

  // set volfrac values as constants
  for (int k = 0; k < numvolfrac_; k++)
    constants.push_back(
        std::pair<std::string, double>(volfracnames_[k], phasemanager_->VolFrac(k)));

  // set volfrac pressure values as constants
  for (int k = 0; k < numvolfrac_; k++)
    constants.push_back(std::pair<std::string, double>(
        volfracpressurenames_[k], phasemanager_->VolFracPressure(k)));

  // set artery pressure as constant
  constants.push_back(std::pair<std::string, double>(artpressname_, artpressnpAtGP));

  // set artery diameter as constant
  constants.push_back(std::pair<std::string, double>("D", arterydiamAtGP_));

  // set reference artery diameter as constant
  constants.push_back(std::pair<std::string, double>("D0", arterydiamref_));

  // set artery diameter of previous time step as constant
  constants.push_back(std::pair<std::string, double>("Dprev", arterymat_->DiamPreviousTimeStep()));
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::SetScalarValuesAsVariables(std::vector<std::pair<std::string, double>>& variables,
    const std::vector<double>& artscalarnpAtGP, const std::vector<double>& scalarnpAtGP)
{
  // set scalar values as variables
  for (int k = 0; k < numscalcont_; k++)
    variables.push_back(std::pair<std::string, double>(scalarnames_[k], scalarnpAtGP[k]));

  // set artery-scalar values as variables
  for (int k = 0; k < numscalart_; k++)
    variables.push_back(std::pair<std::string, double>(artscalarnames_[k], artscalarnpAtGP[k]));
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateFluidDerivs(std::vector<double>& artderivs, std::vector<double>& contderivs,
    const std::vector<double>& functderivs)
{
  // basically dependency of exchange terms f is as follows:
  // f(D(p),g(p)) where p are generic 1D and 3D fluid primary variables
  // so derivative is df/dp = df/dg*dg/dp + df/dD*dD/dp
  // first part
  // function derivs w.r.t. to fluid phases: df/dg*dp/dp
  for (int doftoderive = 0; doftoderive < numfluidphases_; doftoderive++)
  {
    for (int idof = 0; idof < numfluidphases_; idof++)
      contderivs[doftoderive] +=
          functderivs[idof] * phasemanager_->PressureDeriv(idof, doftoderive) +
          functderivs[idof + numfluidphases_] * phasemanager_->SaturationDeriv(idof, doftoderive);
    if (phasemanager_->PorosityDependsOnFluid())
      contderivs[doftoderive] +=
          functderivs[2 * numfluidphases_] * phasemanager_->PorosityDeriv(doftoderive);
  }
  // function derivs w.r.t. to volume fraction phases
  for (int doftoderive = numfluidphases_; doftoderive < numdof_cont_ - numvolfrac_; doftoderive++)
  {
    // derivatives w.r.t. volume fractions directly appearing
    //                and porosity (since it depends on volfrac)
    contderivs[doftoderive] +=
        functderivs[doftoderive + numfluidphases_ + 1] +
        functderivs[2 * numfluidphases_] * phasemanager_->PorosityDeriv(doftoderive);
  }
  // function derivs w.r.t. to volume fraction pressures
  for (int doftoderive = numfluidphases_ + numvolfrac_; doftoderive < numdof_cont_; doftoderive++)
    contderivs[doftoderive] += functderivs[doftoderive + numfluidphases_ + 1];

  // function derivs w.r.t. to artery pressure
  artderivs[0] += functderivs[numfluidphases_ + numfluidphases_ + 1 + numvolfrac_ + numvolfrac_];

  // second part
  // derivatives w.r.t. to fluid diameter: df/dD*dD/dp
  if (diam_funct_active_)
  {
    // df/dD
    const double functderivdiam =
        functderivs[numfluidphases_ + numfluidphases_ + 1 + numvolfrac_ + numvolfrac_ + 1];

    // now follow the terms df/dD*dD/dp
    for (int doftoderive = 0; doftoderive < numdof_cont_; doftoderive++)
      contderivs[doftoderive] += functderivdiam * diamderivs_[doftoderive];

    // diameter derivs w.r.t. to artery pressure
    artderivs[0] += functderivdiam * diamderivs_[numfluidphases_ + 2 * numvolfrac_];
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::EvaluateScalarDerivs(std::vector<double>& artderivs, std::vector<double>& contderivs,
    const std::vector<double>& functderivs)
{
  // derivatives after continuous scalars
  for (int doftoderive = 0; doftoderive < numdof_cont_; doftoderive++)
    contderivs[doftoderive] += functderivs[doftoderive];

  // derivatives after artery scalars
  for (int doftoderive = 0; doftoderive < numdof_art_; doftoderive++)
    artderivs[doftoderive] += functderivs[doftoderive + numdof_cont_];
}
/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::CreateIntegrationSegment()
{
  if (PROJOUTPUT)
  {
    std::cout << "========================= getting intersection for artery ele " << Ele1GID()
              << std::endl;
  }

  // get the intersections
  std::vector<double> intersections = GetAllInterSections();
  const int numintersections = intersections.size();

  if (PROJOUTPUT)
  {
    if (numintersections == 0)
      std::cout << "no intersections found" << std::endl;
    else
    {
      std::cout << "intersections are:" << std::endl;
      for (auto i = intersections.begin(); i != intersections.end(); ++i) std::cout << *i << " ";
      std::cout << " " << std::endl;
    }
  }

  std::vector<double> xi(numdim_);
  // Vectors for shape functions and their derivatives
  static LINALG::Matrix<1, numnodesart_, double> N1(true);      // = N1
  static LINALG::Matrix<1, numnodesart_, double> N1_eta(true);  // = N1,eta
  // Coords and derivatives of 1D and 2D/3D element
  static LINALG::Matrix<numdim_, 1, double> r1(true);      // = r1
  static LINALG::Matrix<numdim_, 1, double> r1_eta(true);  // = r1,eta
  bool projection_valid = false;

  // 1st case: no intersection found
  if (numintersections == 0)
  {
    Get1DShapeFunctions<double>(N1, N1_eta, 0.0);
    ComputeArteryCoordsAndDerivsRef<double>(r1, r1_eta, N1, N1_eta);
    Projection<double>(r1, xi, projection_valid);
    // case: completely inside
    if (projection_valid)
    {
      eta_a_ = -1.0;
      eta_b_ = 1.0;
      isactive_ = true;
    }
    // case: completely outside
    else
      isactive_ = false;
  }
  // 2nd case: 1 intersection found
  else if (numintersections == 1)
  {
    // special case: eta = -1.0 lies directly at boundary of 3D element:
    if (fabs(intersections[0] + 1.0) < XIETATOL)
    {
      // first possibility: segment goes from [-1; 1], second point lies inside 3D element
      Get1DShapeFunctions<double>(N1, N1_eta, 1.0);
      ComputeArteryCoordsAndDerivsRef<double>(r1, r1_eta, N1, N1_eta);
      Projection<double>(r1, xi, projection_valid);
      if (projection_valid)
      {
        eta_a_ = -1.0;
        eta_b_ = 1.0;
        isactive_ = true;
      }
      else
      {
        // we have found a segment between [-1.0; -1.0 +  XIETATOL] --> this can be sorted out
        if (PROJOUTPUT)
        {
          std::cout << "probably found a very small integration segment for artery element "
                    << Ele1GID() << " and 3D element " << Ele2GID() << " which is sorted out!"
                    << std::endl;
        }
        isactive_ = false;
      }
    }
    // special case: eta = 1.0 lies directly at boundary of 3D element:
    else if (fabs(intersections[0] - 1.0) < XIETATOL)
    {
      // first possibility: segment goes from [-1; 1], second point lies inside 3D element
      Get1DShapeFunctions<double>(N1, N1_eta, -1.0);
      ComputeArteryCoordsAndDerivsRef<double>(r1, r1_eta, N1, N1_eta);
      Projection<double>(r1, xi, projection_valid);
      if (projection_valid)
      {
        eta_a_ = -1.0;
        eta_b_ = 1.0;
        isactive_ = true;
      }
      else
      {
        // we have found a segment between [1.0 - XIETATOL; 1.0] --> this can be sorted out
        if (PROJOUTPUT)
        {
          std::cout << "probably found a very small integration segment for artery element "
                    << Ele1GID() << " and 3D element " << Ele2GID() << " which is sorted out!"
                    << std::endl;
        }
        isactive_ = false;
      }
    }
    // normal case: found one intersection: check if -1.0 or 1.0 are inside
    else
    {
      Get1DShapeFunctions<double>(N1, N1_eta, -1.0);
      ComputeArteryCoordsAndDerivsRef<double>(r1, r1_eta, N1, N1_eta);
      Projection<double>(r1, xi, projection_valid);
      // case: segment goes from [-1.0; intersections[0]]
      if (projection_valid)
      {
        eta_a_ = -1.0;
        eta_b_ = intersections[0];
        isactive_ = true;
      }
      else
      {
        // case: segment goes from [intersections[0]; 1.0]
        Get1DShapeFunctions<double>(N1, N1_eta, 1.0);
        ComputeArteryCoordsAndDerivsRef<double>(r1, r1_eta, N1, N1_eta);
        Projection<double>(r1, xi, projection_valid);
        eta_a_ = intersections[0];
        eta_b_ = 1.0;
        isactive_ = true;
        // special case: projection lies directly at corner of element, this can be sorted out
        if (!projection_valid)
        {
          if (PROJOUTPUT)
          {
            std::cout << "original point " << intersections[0] << std::endl;
            std::cout << "Neither -1.0 nor 1.0 could be projected" << std::endl;
          }
          isactive_ = false;
        }
      }
    }
  }
  // 3rd case: two intersections found
  else if (numintersections == 2)
  {
    eta_a_ = intersections[0];
    eta_b_ = intersections[1];
    isactive_ = true;
  }
  // rest is not possible
  else
    dserror(
        "Found more than two intersections for artery element %d and 2D/3D element %d, this "
        "should "
        "not be possible",
        Ele1GID(), Ele2GID());

  // safety checks
  if (isactive_)
  {
    if (eta_a_ > eta_b_)
      dserror(
          "something went terribly wrong for artery element %d and 2D/3D element %d, eta_a is "
          "bigger than eta_b",
          Ele1GID(), Ele2GID());
    if (fabs(eta_a_ - eta_b_) < XIETATOL)
      dserror(
          "something went terribly wrong for artery element %d and 2D/3D element %d, found "
          "extremely small integration segment",
          Ele1GID(), Ele2GID());
  }

  return;
}
/*-----------------------------------------------------------------------------*
 *-----------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
std::vector<double> POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt,
    distypeCont, dim>::GetAllInterSections()
{
  std::vector<double> intersections(0);

  std::vector<double> xi(numdim_, 0.0);
  double eta = 0.0;
  bool projection_valid = true;

  switch (element2_->Shape())
  {
    case DRT::Element::quad4:
    {
      for (unsigned int j = 0; j < numdim_; j++)
      {
        // project edge xi1 or xi2 = 1.0
        InterSectWith2D3D(xi, eta, j, 1.0, projection_valid);
        if (projection_valid && ProjectionNotYetFound(intersections, eta))
          intersections.push_back(eta);

        // project edge xi1 or xi2 = -1.0
        InterSectWith2D3D(xi, eta, j, -1.0, projection_valid);
        if (projection_valid && ProjectionNotYetFound(intersections, eta))
          intersections.push_back(eta);
      }
      break;
    }
    case DRT::Element::hex8:
    {
      for (unsigned int j = 0; j < numdim_; j++)
      {
        // project surface xi1 or xi2 or xi3 = 1.0
        InterSectWith2D3D(xi, eta, j, 1.0, projection_valid);
        if (projection_valid && ProjectionNotYetFound(intersections, eta))
          intersections.push_back(eta);

        // project surface xi1 or xi2 or xi3 = -1.0
        InterSectWith2D3D(xi, eta, j, -1.0, projection_valid);
        if (projection_valid && ProjectionNotYetFound(intersections, eta))
          intersections.push_back(eta);
      }
      break;
    }
    case DRT::Element::tet4:
    {
      for (unsigned int j = 0; j < 3; j++)
      {
        // project surface xi1 or xi2 or xi3 = 0.0
        InterSectWith2D3D(xi, eta, j, 0.0, projection_valid);
        if (projection_valid && ProjectionNotYetFound(intersections, eta))
          intersections.push_back(eta);
      }
      // project fourth surface of tetahedron xi1 + xi2 + xi3 = 1
      InterSectWith2D3D(xi, eta, 3, 0.0, projection_valid);
      if (projection_valid && ProjectionNotYetFound(intersections, eta))
        intersections.push_back(eta);
      break;
    }
    default:
      dserror("Only quad4, hex8 and tet4 are valid so far for second element");
  }

  std::sort(intersections.begin(), intersections.end());

  return intersections;
}

/*-----------------------------------------------------------------------------*
 *-----------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
bool POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::ProjectionNotYetFound(const std::vector<double>& intersections, const double& eta)
{
  for (unsigned int i = 0; i < intersections.size(); i++)
  {
    if (fabs(intersections[i] - eta) < XIETATOL)
    {
      if (PROJOUTPUT) std::cout << "duplicate intersection found" << std::endl;
      return false;
    }
  }
  return true;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::InterSectWith2D3D(std::vector<double>& xi, double& eta, const int& fixedPar,
    const double& fixedAt, bool& projection_valid)
{
  projection_valid = true;
  bool parallel = false;

  // Initialize limit for parameter values (interval [-limit, limit])
  const double limit = 1.0 + XIETATOL;

  // reset iteration variables
  eta = 0.0;
  switch (element2_->Shape())
  {
    case DRT::Element::quad4:
    {
      if (fixedPar == 0)  // xi1 fixed
      {
        xi[0] = fixedAt;
        xi[1] = 0.0;
      }
      else if (fixedPar == 1)  // xi2 fixed
      {
        xi[0] = 0.0;
        xi[1] = fixedAt;
      }
      else
        dserror("wrong input for fixedPar");
      break;
    }
    case DRT::Element::hex8:
    {
      if (fixedPar == 0)  // xi1 fixed
      {
        xi[0] = fixedAt;
        xi[1] = 0.0;
        xi[2] = 0.0;
      }
      else if (fixedPar == 1)  // xi2 fixed
      {
        xi[0] = 0.0;
        xi[1] = fixedAt;
        xi[2] = 0.0;
      }
      else if (fixedPar == 2)  // xi3 fixed
      {
        xi[0] = 0.0;
        xi[1] = 0.0;
        xi[2] = fixedAt;
      }
      else
        dserror("wrong input for fixedPar");
      break;
    }
    case DRT::Element::tet4:
    {
      if (fixedPar == 0)  // xi1 fixed
      {
        xi[0] = fixedAt;
        xi[1] = 0.25;
        xi[2] = 0.25;
      }
      else if (fixedPar == 1)  // xi2 fixed
      {
        xi[0] = 0.25;
        xi[1] = fixedAt;
        xi[2] = 0.25;
      }
      else if (fixedPar == 2)  // xi3 fixed
      {
        xi[0] = 0.25;
        xi[1] = 0.25;
        xi[2] = fixedAt;
      }
      else if (fixedPar == 3)  // fourth surface xi1 + xi2 + xi3 = 1 fixed
      {
        xi[0] = 0.25;
        xi[1] = 0.25;
        xi[2] = 0.5;
      }
      else
        dserror(
            "only fixedPar = 0, fixedPar = 1, fixedPar = 2 or fixedPar = 3 possible (for tet "
            "elements");
      break;
    }
    default:
      dserror("Only quad4, hex8 and tet4 are valid so far for second element");
  }

  if (PROJOUTPUT)
  {
    std::cout << "Projection output:" << std::endl;
    std::cout << "Start parameters eta: " << eta;
    switch (element2_->Shape())
    {
        // 2D case
      case DRT::Element::quad4:
      {
        std::cout << ", xi1: " << xi[0] << ", xi2: " << xi[1] << std::endl;
        break;
      }
        // 3D case
      case DRT::Element::hex8:
      case DRT::Element::tet4:
      {
        std::cout << ", xi1: " << xi[0] << ", xi2: " << xi[1] << ", xi3: " << xi[2] << std::endl;
        break;
      }
      default:
        dserror("Only quad4, hex8 and tet4 are valid so far for second element");
    }
  }

  // Initialize function f and Jacobian J for Newton iteration
  LINALG::Matrix<numdim_, 1> f(true);
  LINALG::Matrix<numdim_, numdim_> J(true);
  LINALG::Matrix<numdim_, numdim_> Jinv(true);

  // Vectors for shape functions and their derivatives
  static LINALG::Matrix<1, numnodesart_> N1(true);      // = N1
  static LINALG::Matrix<1, numnodesart_> N1_eta(true);  // = N1,eta

  static LINALG::Matrix<1, numnodescont_> N2(true);           // = N2
  static LINALG::Matrix<numdim_, numnodescont_> N2_xi(true);  // = N2,xi1

  // Coords and derivatives of for 1D and 2D/3D element
  static LINALG::Matrix<numdim_, 1> r1(true);      // = r1
  static LINALG::Matrix<numdim_, 1> r1_eta(true);  // = r1,eta

  static LINALG::Matrix<numdim_, 1> x2(true);           // = x2
  static LINALG::Matrix<numdim_, numdim_> x2_xi(true);  // = x2,xi

  // Initial scalar residual (L2-norm of f)
  double residual;

  // Local newton iteration
  // -----------------------------------------------------------------
  int iter;
  double first_residual = 1.0e-4;  // used for convergence check

  for (iter = 0; iter < PROJMAXITER; iter++)
  {
    // Update shape functions and their derivatives for 1D and 2D/3D element
    Get1DShapeFunctions<double>(N1, N1_eta, eta);
    Get2D3DShapeFunctions<double>(N2, N2_xi, xi);

    // Update coordinates and derivatives for 1D and 2D/3D element
    ComputeArteryCoordsAndDerivsRef<double>(r1, r1_eta, N1, N1_eta);
    Compute2D3DCoordsAndDerivsRef<double>(x2, x2_xi, N2, N2_xi);

    // Evaluate f at current xi1, xi2, alpha
    f.Clear();
    for (unsigned int i = 0; i < numdim_; i++) f(i) = x2(i) - r1(i);

    // Compute scalar residuum
    residual = 0.0;
    for (unsigned int i = 0; i < numdim_; i++) residual += f(i) * f(i);
    residual = sqrt(residual);
    if (iter == 0) first_residual = std::max(first_residual, residual);

    J.Clear();

    if (fixedPar == 0)  // xi1 fixed --> we need x_{,xi2} (and x_{,xi3} in case of 3D)
    {
      for (unsigned int idim = 0; idim < numdim_; idim++)
        for (unsigned int jdim = 1; jdim < numdim_; jdim++) J(idim, jdim - 1) = x2_xi(idim, jdim);
    }
    else if (fixedPar == 1)  // xi2 fixed --> we need x_{,xi1} (and x_{,xi3} in case of 3D)
    {
      switch (element2_->Shape())
      {
          // 2D case
        case DRT::Element::quad4:
        {
          for (unsigned int jdim = 0; jdim < numdim_; jdim++) J(jdim, 0) = x2_xi(jdim, 0);
          break;
        }
          // 3D case
        case DRT::Element::hex8:
        case DRT::Element::tet4:
        {
          for (unsigned int jdim = 0; jdim < numdim_; jdim++)
          {
            J(jdim, 0) = x2_xi(jdim, 0);
            J(jdim, 1) = x2_xi(jdim, 2);
          }
          break;
        }
        default:
          dserror("Only quad4, hex8 and tet4 are valid so far for second element");
      }
    }
    else if (fixedPar == 2)  // xi3 fixed  --> we need x_{,xi1} (and x_{,xi2} in case of 3D)
    {
      for (unsigned int idim = 0; idim < numdim_; idim++)
        for (unsigned int jdim = 0; jdim < numdim_ - 1; jdim++) J(idim, jdim) = x2_xi(idim, jdim);
    }
    else if (fixedPar == 3)  // xi3 fixed at xi3 = 1.0 - xi1 - xi2
    {
      for (unsigned int idim = 0; idim < numdim_; idim++)
      {
        // df/dxi1 = df/dxi1 - df/dxi3
        J(idim, 0) = x2_xi(idim, 0) - x2_xi(idim, 2);
        // df/dxi_2 = df/dxi2 - df/dxi3
        J(idim, 1) = x2_xi(idim, 1) - x2_xi(idim, 2);
      }
    }
    else
      dserror(
          "only fixedPar = 0, fixedPar = 1, fixedPar = 2 or fixedPar = 3 possible (for tet "
          "elements)");

    // fill dr_deta into Jacobian
    for (unsigned int idim = 0; idim < numdim_; idim++) J(idim, numdim_ - 1) = -r1_eta(idim);

    double jacdet = J.Determinant();

    // If det_J = 0 we assume, that the artery and the surface edge are parallel.
    // These projection is not needed due the fact that the contact interval can also be
    // identified by other projections
    parallel = fabs(jacdet) < COLINEARTOL * first_residual;
    if (!parallel) jacdet = J.Invert();

    // Check if the local Newton iteration has converged
    if (residual < CONVTOLNEWTONPROJ * first_residual && !parallel)
    {
      if (PROJOUTPUT)
      {
        std::cout << "Local Newton iteration converged after " << iter << " iterations"
                  << std::endl;
        std::cout << "Found point at xi1: " << xi[0] << ", xi2: " << xi[1];
        if (numdim_ == 3) std::cout << ", xi3: " << xi[2];
        std::cout << ", eta: " << eta << " with residual: " << residual << std::endl;
        std::cout << "r1:\n" << r1 << ", x2:\n" << x2 << std::endl;
      }
      // Local Newton iteration converged
      break;
    }
    else if (PROJOUTPUT && iter > 0)
    {
      std::cout << "New point at xi1: " << xi[0] << ", xi2: " << xi[0];
      if (numdim_ == 3) std::cout << ", xi3: " << xi[2];
      std::cout << ", eta: " << eta << " with residual: " << residual << std::endl;
    }

    // Singular J
    if (parallel)
    {
      // Sort out
      if (PROJOUTPUT)
      {
        std::cout << "elementscolinear: det_J = " << jacdet << std::endl;
      }
      break;
    }
    // Regular J (inversion possible)

    if (fixedPar == 0)  // xi1 fixed --> we have to update xi2 (and xi3 in case of 3D)
    {
      for (unsigned int idim = 1; idim < numdim_; idim++)
        for (unsigned int jdim = 0; jdim < numdim_; jdim++)
          xi[idim] += -J(idim - 1, jdim) * f(jdim);
    }
    else if (fixedPar == 1)  // xi2 fixed --> we have to update xi1 (and xi3 in case of 3D)
    {
      switch (element2_->Shape())
      {
          // 2D case
        case DRT::Element::quad4:
        {
          for (unsigned int jdim = 0; jdim < numdim_; jdim++) xi[0] += -J(0, jdim) * f(jdim);
          break;
        }
          // 3D case
        case DRT::Element::hex8:
        case DRT::Element::tet4:
        {
          for (unsigned int jdim = 0; jdim < numdim_; jdim++)
          {
            xi[0] += -J(0, jdim) * f(jdim);
            xi[2] += -J(1, jdim) * f(jdim);
          }
          break;
        }
        default:
          dserror("Only quad4, hex8 and tet4 are valid so far for second element");
      }
    }
    else if (fixedPar == 2)  // xi3 fixed --> we have to update xi1 (and xi2 in case of 3D)
    {
      for (unsigned int idim = 0; idim < numdim_ - 1; idim++)
        for (unsigned int jdim = 0; jdim < numdim_; jdim++) xi[idim] += -J(idim, jdim) * f(jdim);
    }
    else if (fixedPar == 3)
    {
      // xi3 fixed at 1.0 - xi1 - xi2
      for (unsigned int idim = 0; idim < numdim_ - 1; idim++)
        for (unsigned int jdim = 0; jdim < numdim_; jdim++) xi[idim] += -J(idim, jdim) * f(jdim);
      xi[2] = 1.0 - xi[0] - xi[1];
    }
    else
      dserror(
          "only fixedPar = 0, fixedPar = 1, fixedPar = 2 or fixedPar = 3 possible (for tet "
          "elements)");

    // update also eta
    for (unsigned int jdim = 0; jdim < numdim_; jdim++) eta += -J(numdim_ - 1, jdim) * f(jdim);
  }

  // Local Newton iteration unconverged after PROJMAXITER
  if (residual > CONVTOLNEWTONPROJ * first_residual || parallel)
  {
    for (unsigned int idim = 0; idim < numdim_; idim++) xi[idim] = 1e+12;
    eta = 1e+12;

    if (PROJOUTPUT)
      std::cout << "Local Newton iteration unconverged (!) after " << iter + 1 << " iterations"
                << std::endl;
  }

  switch (element2_->Shape())
  {
    case DRT::Element::quad4:
    {
      if (fabs(xi[0]) > limit || fabs(xi[1]) > limit || fabs(eta) > limit) projection_valid = false;
      break;
    }
    case DRT::Element::hex8:
    {
      if (fabs(xi[0]) > limit || fabs(xi[1]) > limit || fabs(xi[2]) > limit || fabs(eta) > limit)
        projection_valid = false;
      break;
    }
    case DRT::Element::tet4:
    {
      if (xi[0] < -XIETATOL || xi[1] < -XIETATOL || xi[2] < -XIETATOL ||
          xi[0] + xi[1] + xi[2] > limit || fabs(eta) > limit)
        projection_valid = false;
      break;
    }
    default:
      dserror("Only quad4, hex8 and tet4 are valid so far for second element");
  }

  if (PROJOUTPUT)
  {
    if (projection_valid)
      std::cout << "Projection allowed" << std::endl;
    else
      std::cout << "Projection not allowed" << std::endl;
  }

  return;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
template <typename T>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::Projection(LINALG::Matrix<numdim_, 1, T>& r1, std::vector<T>& xi, bool& projection_valid)
{
  projection_valid = true;
  bool parallel = false;

  // Initialize limit for parameter values (interval [-limit, limit])
  const double limit = 1.0 + XIETATOL;

  switch (element2_->Shape())
  {
    case DRT::Element::quad4:
    {
      xi[0] = 0.0;
      xi[1] = 0.0;
      break;
    }
    case DRT::Element::hex8:
    {
      xi[0] = 0.0;
      xi[1] = 0.0;
      xi[2] = 0.0;
      break;
    }
    case DRT::Element::tet4:
    {
      xi[0] = 0.25;
      xi[1] = 0.25;
      xi[2] = 0.25;
      break;
    }
    default:
      dserror("Only quad4, hex8 and tet4 are valid so far for second element");
  }

  if (PROJOUTPUT)
  {
    std::cout << "Projection output:" << std::endl;
    std::cout << "Start parameters ";
    switch (element2_->Shape())
    {
        // 2D case
      case DRT::Element::quad4:
      {
        std::cout << "xi1: " << xi[0] << ", xi2: " << xi[1] << std::endl;
        break;
      }
        // 3D case
      case DRT::Element::hex8:
      case DRT::Element::tet4:
      {
        std::cout << "xi1: " << xi[0] << ", xi2: " << xi[1] << ", xi3: " << xi[2] << std::endl;
        break;
      }
      default:
        dserror("Only quad4, hex8 and tet4 are valid so far for second element");
    }
  }

  // Initialize function f and Jacobian J for Newton iteration
  LINALG::Matrix<numdim_, 1, T> f(true);
  LINALG::Matrix<numdim_, numdim_, T> J(true);
  LINALG::Matrix<numdim_, numdim_, T> Jinv(true);

  // Vectors for shape functions and their derivatives
  static LINALG::Matrix<1, numnodesart_, T> N1(true);      // = N1
  static LINALG::Matrix<1, numnodesart_, T> N1_eta(true);  // = N1,eta

  static LINALG::Matrix<1, numnodescont_, T> N2(true);           // = N2
  static LINALG::Matrix<numdim_, numnodescont_, T> N2_xi(true);  // = N2,xi1

  static LINALG::Matrix<numdim_, 1, T> x2(true);           // = x2
  static LINALG::Matrix<numdim_, numdim_, T> x2_xi(true);  // = x2,xi

  // Initial scalar residual (L2-norm of f)
  T residual;

  // Local newton iteration
  // -----------------------------------------------------------------

  int iter;
  double first_residual = 1.0e-4;  // used for convergence check

  for (iter = 0; iter < PROJMAXITER; iter++)
  {
    // Update shape functions and their derivatives for 1D and 2D/3D element
    Get2D3DShapeFunctions<T>(N2, N2_xi, xi);

    // Update coordinates and derivatives for 1D and 2D/3D element
    Compute2D3DCoordsAndDerivsRef<T>(x2, x2_xi, N2, N2_xi);

    // Evaluate f at current xi1, xi2, alpha
    f.Clear();
    for (unsigned int i = 0; i < numdim_; i++) f(i) = x2(i) - r1(i);

    residual = FADUTILS::VectorNorm(f);
    if (iter == 0) first_residual = std::max(first_residual, FADUTILS::CastToDouble(residual));

    // Reset matrices
    for (unsigned int i = 0; i < numdim_; i++)
      for (unsigned int j = 0; j < numdim_; j++) J(i, j) = x2_xi(i, j);

    const double jacdet = FADUTILS::CastToDouble<T, numdim_, numdim_>(J).Determinant();

    // If det_J = 0 we assume, that the artery element and the surface edge are parallel.
    // These projection is not needed due the fact that the contact interval can also be
    // identified by two contact interval borders found with the GetContactLines method
    parallel = fabs(jacdet) < COLINEARTOL * first_residual;
    if (!parallel) J.Invert();

    // Check if the local Newton iteration has converged
    // If the start point fulfills the orthogonalty conditions (residual < CONVTOLNEWTONPROJ*
    // first_residual), we also check if the artery element and the surface edge are parallel.
    // This is done by calculating det_J before checking if the local Newton iteration has
    // converged by fulfilling the condition residual < CONVTOLNEWTONPROJ*first_residual
    if (residual < CONVTOLNEWTONPROJ * first_residual && !parallel)
    {
      if (PROJOUTPUT)
      {
        std::cout << "Local Newton iteration converged after " << iter << " iterations"
                  << std::endl;
        std::cout << "Found point at ";
        switch (element2_->Shape())
        {
            // 2D case
          case DRT::Element::quad4:
          {
            std::cout << "xi1: " << xi[0] << ", xi2: " << xi[1] << std::endl;
            break;
          }
            // 3D case
          case DRT::Element::hex8:
          case DRT::Element::tet4:
          {
            std::cout << "xi1: " << xi[0] << ", xi2: " << xi[1] << ", xi3: " << xi[2] << std::endl;
            break;
          }
          default:
            dserror("Only quad4, hex8 and tet4 are valid so far for second element");
        }
        std::cout << " with residual: " << residual << std::endl;
        std::cout << "r1:\n" << r1 << ", x2:\n" << x2 << std::endl;
      }
      // Local Newton iteration converged
      break;
    }
    else if (PROJOUTPUT && iter > 0)
    {
      std::cout << "New point at xi1: ";
      switch (element2_->Shape())
      {
          // 2D case
        case DRT::Element::quad4:
        {
          std::cout << "xi1: " << xi[0] << ", xi2: " << xi[1] << std::endl;
          break;
        }
          // 3D case
        case DRT::Element::hex8:
        case DRT::Element::tet4:
        {
          std::cout << "xi1: " << xi[0] << ", xi2: " << xi[1] << ", xi3: " << xi[2] << std::endl;
          break;
        }
        default:
          dserror("Only quad4, hex8 and tet4 are valid so far for second element");
      }
      std::cout << " with residual: " << residual << std::endl;
    }

    // Singular J
    if (parallel)
    {
      // Sort out
      if (PROJOUTPUT)
      {
        std::cout << "elementscolinear: det_J = " << jacdet << std::endl;
      }
      break;
    }
    // Regular J (inversion possible)
    for (unsigned int idim = 0; idim < numdim_; idim++)
      for (unsigned int jdim = 0; jdim < numdim_; jdim++) xi[idim] += -J(idim, jdim) * f(jdim);

    // xi1 += -J(0, 0) * f(0) - J(0, 1) * f(1) - J(0, 2) * f(2);
    // xi2 += -J(1, 0) * f(0) - J(1, 1) * f(1) - J(1, 2) * f(2);
    // xi3 += -J(2, 0) * f(0) - J(2, 1) * f(1) - J(2, 2) * f(2);
  }
  // -----------------------------------------------------------------
  // End: Local Newton iteration

  // Local Newton iteration unconverged after PROJMAXITER
  if (residual > CONVTOLNEWTONPROJ * first_residual || parallel)
  {
    for (unsigned int idim = 0; idim < numdim_; idim++) xi[idim] = 1e+12;

    if (PROJOUTPUT)
      std::cout << "Local Newton iteration unconverged (!) after " << iter + 1 << " iterations"
                << std::endl;
  }

  switch (element2_->Shape())
  {
    case DRT::Element::quad4:
    {
      if (fabs(xi[0]) > limit || fabs(xi[1]) > limit) projection_valid = false;
      break;
    }
    case DRT::Element::hex8:
    {
      if (fabs(xi[0]) > limit || fabs(xi[1]) > limit || fabs(xi[2]) > limit)
        projection_valid = false;
      break;
    }
    case DRT::Element::tet4:
    {
      if (xi[0] < -XIETATOL || xi[1] < -XIETATOL || xi[2] < -XIETATOL ||
          xi[0] + xi[1] + xi[2] > limit)
        projection_valid = false;
      break;
    }
    default:
      dserror("Only quad4, hex8 and tet4 are valid so far for second element");
  }

  if (PROJOUTPUT)
  {
    if (projection_valid)
      std::cout << "Projection allowed" << std::endl;
    else
      std::cout << "Projection not allowed" << std::endl;
  }

  return;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
template <typename T>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::Get1DShapeFunctions(LINALG::Matrix<1, numnodesart_, T>& N1,
    LINALG::Matrix<1, numnodesart_, T>& N1_eta, const T& eta)
{
  // Clear shape functions and derivatives
  N1.Clear();
  N1_eta.Clear();

  // Get discretization type
  const DRT::Element::DiscretizationType distype = element1_->Shape();

  // Get values and derivatives of shape functions
  DRT::UTILS::shape_function_1D(N1, eta, distype);
  DRT::UTILS::shape_function_1D_deriv1(N1_eta, eta, distype);

  return;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
template <typename T>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::Get2D3DShapeFunctions(LINALG::Matrix<1, numnodescont_, T>& N2,
    LINALG::Matrix<numdim_, numnodescont_, T>& N2_xi, const std::vector<T>& xi)
{
  // Clear shape functions and derivatives
  N2.Clear();
  N2_xi.Clear();

  switch (element2_->Shape())
  {
      // 2D case
    case DRT::Element::quad4:
    {
      DRT::UTILS::shape_function_2D(N2, xi[0], xi[1], distypeCont);
      DRT::UTILS::shape_function_2D_deriv1(N2_xi, xi[0], xi[1], distypeCont);
      break;
    }
      // 3D case
    case DRT::Element::hex8:
    case DRT::Element::tet4:
    {
      DRT::UTILS::shape_function_3D(N2, xi[0], xi[1], xi[2], distypeCont);
      DRT::UTILS::shape_function_3D_deriv1(N2_xi, xi[0], xi[1], xi[2], distypeCont);
      break;
    }
    default:
      dserror("Only quad4, hex8 and tet4 are valid so far for second element");
  }

  return;
}

/*------------------------------------------------------------------------*
 *------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
template <typename T>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::ComputeArteryCoordsAndDerivsRef(LINALG::Matrix<numdim_, 1, T>& r1,
    LINALG::Matrix<numdim_, 1, T>& r1_eta, const LINALG::Matrix<1, numnodesart_, T>& N1,
    const LINALG::Matrix<1, numnodesart_, T>& N1_eta)
{
  r1.Clear();
  r1_eta.Clear();

  for (unsigned int j = 0; j < numnodesart_; j++)
  {
    for (unsigned int idim = 0; idim < numdim_; idim++)
    {
      r1(idim) += N1(j) * ele1posref_(numdim_ * j + idim);
      r1_eta(idim) += N1_eta(j) * ele1posref_(numdim_ * j + idim);
    }
  }
  return;
}

/*------------------------------------------------------------------------*
 *------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
template <typename T>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::Compute2D3DCoordsAndDerivsRef(LINALG::Matrix<numdim_, 1, T>& x2,
    LINALG::Matrix<numdim_, numdim_, T>& x2_xi, const LINALG::Matrix<1, numnodescont_, T>& N2,
    const LINALG::Matrix<numdim_, numnodescont_, T>& N2_xi)
{
  x2.Clear();
  x2_xi.Clear();

  for (unsigned int j = 0; j < numnodescont_; j++)
  {
    for (unsigned int idim = 0; idim < numdim_; idim++)
    {
      x2(idim) += N2(j) * ele2posref_(idim, j);
      for (unsigned int jdim = 0; jdim < numdim_; jdim++)
      {
        x2_xi(idim, jdim) += N2_xi(jdim, j) * ele2posref_(idim, j);
      }
    }
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::FillFunctionVector(std::vector<DRT::UTILS::FunctionOfAnything*>& my_funct_vec,
    const std::vector<int>& funct_vec, const std::vector<int>& scale_vec)
{
  for (unsigned int i = 0; i < funct_vec.size(); i++)
  {
    if (funct_vec[i] >= 0 && abs(scale_vec[i]) > 0)
    {
      my_funct_vec.at(i) = &DRT::UTILS::GetFunctionOfAnything(funct_vec[i]);
      funct_coupl_active_ = true;
    }
    else
      my_funct_vec.at(i) = 0;
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::InitializeFunction(const DRT::UTILS::FunctionOfAnything& funct)
{
  // safety check
  if (funct.NumberComponents() != 1) dserror("expected only one component for coupling function!");
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::InitializeFunctionNames()
{
  pressurenames_.resize(numfluidphases_);
  saturationnames_.resize(numfluidphases_);
  volfracnames_.resize(numvolfrac_);
  volfracpressurenames_.resize(numvolfrac_);
  scalarnames_.resize(numscalcont_);
  artscalarnames_.resize(numscalart_);

  for (int k = 0; k < numscalcont_; k++)
  {
    // add scalar names
    {
      std::ostringstream temp;
      temp << k + 1;
      scalarnames_[k] = "phi" + temp.str();
    }
  }

  for (int k = 0; k < numscalart_; k++)
  {
    // add artery-scalar names
    {
      std::ostringstream temp;
      temp << k + 1;
      artscalarnames_[k] = "phi_art" + temp.str();
    }
  }

  for (int k = 0; k < numfluidphases_; k++)
  {
    // add pressure names
    {
      std::ostringstream temp;
      temp << k + 1;
      pressurenames_[k] = "p" + temp.str();
    }

    // add saturation names
    {
      std::ostringstream temp;
      temp << k + 1;
      saturationnames_[k] = "S" + temp.str();
    }
  }

  // add additional volume fractions
  for (int k = 0; k < numvolfrac_; k++)
  {
    // add volume fraction names
    {
      std::ostringstream temp;
      temp << k + 1;
      volfracnames_[k] = "VF" + temp.str();
    }
    // add volume fraction pressure names
    {
      std::ostringstream temp;
      temp << k + 1;
      volfracpressurenames_[k] = "VFP" + temp.str();
    }
  }
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distypeArt, DRT::Element::DiscretizationType distypeCont,
    int dim>
void POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<distypeArt, distypeCont,
    dim>::InitializeAssembleIntoContDofVector()
{
  cont_dofs_to_assemble_functions_into_.resize(numdof_cont_);

  // just standard, each dof assembles into own equation
  for (int icont = 0; icont < numdof_cont_; icont++)
  {
    std::vector<int> thisdof = {icont};
    cont_dofs_to_assemble_functions_into_[icont] = thisdof;
  }

  switch (coupltype_)
  {
    case type_porofluid:
    {
      // special case for phases [0, ..., numfluidphases - 2]:
      // those have to assemble into the summed up fluid phase
      // see also porofluid_evaluator
      for (int curphase = 0; curphase < numfluidphases_ - 1; curphase++)
        cont_dofs_to_assemble_functions_into_[curphase].push_back(numfluidphases_ - 1);
      break;
    }
    case type_scatra:
    {
      // do nothing
      break;
    }
    default:
      dserror("Unknown coupling type");
      break;
  }
}


// explicit template instantiations
template class POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<DRT::Element::line2,
    DRT::Element::quad4, 1>;
template class POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<DRT::Element::line2,
    DRT::Element::hex8, 1>;
template class POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<DRT::Element::line2,
    DRT::Element::tet4, 1>;

template class POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<DRT::Element::line2,
    DRT::Element::quad4, 2>;
template class POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<DRT::Element::line2,
    DRT::Element::hex8, 2>;
template class POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<DRT::Element::line2,
    DRT::Element::tet4, 2>;

template class POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<DRT::Element::line2,
    DRT::Element::quad4, 3>;
template class POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<DRT::Element::line2,
    DRT::Element::hex8, 3>;
template class POROMULTIPHASESCATRA::PoroMultiPhaseScatraArteryCouplingPair<DRT::Element::line2,
    DRT::Element::tet4, 3>;