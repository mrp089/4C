/*----------------------------------------------------------------------*/
/*!
\file scatra_ele_impl.cpp

\brief Internal implementation of scalar transport elements

<pre>
Maintainer: Georg Bauer
            bauer@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15252
</pre>
*/
/*----------------------------------------------------------------------*/

#if defined(D_FLUID3)
#ifdef CCADISCRET

#include "scatra_ele_impl.H"
#include "../drt_mat/scatra_mat.H"
#include "../drt_mat/mixfrac.H"
#include "../drt_mat/sutherland.H"
#include "../drt_mat/arrhenius_spec.H"
#include "../drt_mat/arrhenius_temp.H"
#include "../drt_mat/arrhenius_pv.H"
#include "../drt_mat/ferech_pv.H"
#include "../drt_mat/ion.H"
#include "../drt_mat/biofilm.H"
#include "../drt_mat/matlist.H"
#include "../drt_lib/drt_globalproblem.H"
#include "../drt_lib/drt_timecurve.H"
#include "../drt_lib/drt_utils.H"
#include "../drt_fem_general/drt_utils_fem_shapefunctions.H"
#include "../drt_fem_general/drt_utils_nurbs_shapefunctions.H"
#include "../drt_nurbs_discret/drt_nurbs_discret.H"
#include "../drt_nurbs_discret/drt_nurbs_utils.H"
#include "../drt_fem_general/drt_utils_gder2.H"
#include "../drt_geometry/position_array.H"
#include "../linalg/linalg_serialdensematrix.H"
#include <Teuchos_StandardParameterEntryValidators.hpp>
#include "../drt_lib/drt_condition_utils.H"
#include "../drt_inpar/inpar_scatra.H"
#include "scatra_reinit_defines.H" // schott

//#define VISUALIZE_ELEMENT_DATA
#include "../drt_geometry/integrationcell_coordtrafo.H"
//#define VISUALIZE_ELEDATA_GMSH
// only if VISUALIZE_ELEDATA_GMSH
//#include "../drt_io/io_gmsh.H"
//#include "../drt_geometry/integrationcell_coordtrafo.H"

//#define SUBSCALE_ENC
// activate debug screen output
//#define PRINT_ELCH_DEBUG
// use effective diffusion coefficient for stabilization
#define ACTIVATEBINARYELECTROLYTE
#define ELCHOTHERMODELS

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::ScaTraImplInterface* DRT::ELEMENTS::ScaTraImplInterface::Impl(
    const DRT::Element* ele,
    const enum INPAR::SCATRA::ScaTraType scatratype)
{
  // we assume here, that numdofpernode is equal for every node within
  // the discretization and does not change during the computations
  const int numdofpernode = ele->NumDofPerNode(*(ele->Nodes()[0]));
  int numscal = numdofpernode;
  if (SCATRA::IsElchProblem(scatratype))
    numscal -= 1;

  switch (ele->Shape())
  {
  case DRT::Element::hex8:
  {
    return ScaTraImpl<DRT::Element::hex8>::Instance(numdofpernode,numscal);
  }
  case DRT::Element::hex20:
  {
    return ScaTraImpl<DRT::Element::hex20>::Instance(numdofpernode,numscal);
  }
  case DRT::Element::hex27:
  {
    return ScaTraImpl<DRT::Element::hex27>::Instance(numdofpernode,numscal);
  }
  case DRT::Element::nurbs8:
  {
    return ScaTraImpl<DRT::Element::nurbs8>::Instance(numdofpernode,numscal);
  }
  case DRT::Element::nurbs27:
  {
    return ScaTraImpl<DRT::Element::nurbs27>::Instance(numdofpernode,numscal);
  }
  case DRT::Element::tet4:
  {
    return ScaTraImpl<DRT::Element::tet4>::Instance(numdofpernode,numscal);
  }
 /* case DRT::Element::tet10:
  {
    return ScaTraImpl<DRT::Element::tet10>::Instance(numdofpernode,numscal);
  } */
  case DRT::Element::wedge6:
  {
    return ScaTraImpl<DRT::Element::wedge6>::Instance(numdofpernode,numscal);
  }
/*  case DRT::Element::wedge15:
  {
    return ScaTraImpl<DRT::Element::wedge15>::Instance(numdofpernode,numscal);
  } */
  case DRT::Element::pyramid5:
  {
    return ScaTraImpl<DRT::Element::pyramid5>::Instance(numdofpernode,numscal);
  }
  case DRT::Element::quad4:
  {
    return ScaTraImpl<DRT::Element::quad4>::Instance(numdofpernode,numscal);
  }
/*  case DRT::Element::quad8:
  {
    return ScaTraImpl<DRT::Element::quad8>::Instance(numdofpernode,numscal);
  }*/
  case DRT::Element::quad9:
  {
    return ScaTraImpl<DRT::Element::quad9>::Instance(numdofpernode,numscal);
  }
  case DRT::Element::nurbs4:
  {
    return ScaTraImpl<DRT::Element::nurbs4>::Instance(numdofpernode,numscal);
  }
  case DRT::Element::nurbs9:
  {
    return ScaTraImpl<DRT::Element::nurbs9>::Instance(numdofpernode,numscal);
  }
  case DRT::Element::tri3:
  {
    return ScaTraImpl<DRT::Element::tri3>::Instance(numdofpernode,numscal);
  }
/*  case DRT::Element::tri6:
  {
    return ScaTraImpl<DRT::Element::tri6>::Instance(numdofpernode,numscal);
  }*/
  case DRT::Element::line2:
  {
    return ScaTraImpl<DRT::Element::line2>::Instance(numdofpernode,numscal);
  }
  case DRT::Element::line3:
  {
    return ScaTraImpl<DRT::Element::line3>::Instance(numdofpernode,numscal);
  }
  default:
    dserror("Element shape %s not activated. Just do it.",DRT::DistypeToString(ele->Shape()).c_str());
  }
  return NULL;
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template<DRT::Element::DiscretizationType distype>
DRT::ELEMENTS::ScaTraImpl<distype> * DRT::ELEMENTS::ScaTraImpl<distype>::Instance(
    const int numdofpernode,
    const int numscal,
    bool create
    )
{
  static ScaTraImpl<distype> * instance;
  if ( create )
  {
    if ( instance==NULL )
    {
      instance = new ScaTraImpl<distype>(numdofpernode,numscal);
    }
  }
  else
  {
    if ( instance!=NULL )
      delete instance;
    instance = NULL;
  }
  return instance;
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::Done()
{
  // delete this pointer! Afterwards we have to go! But since this is a
  // cleanup call, we can do it this way.
  Instance(0,0,false );
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
DRT::ELEMENTS::ScaTraImpl<distype>::ScaTraImpl(const int numdofpernode, const int numscal)
  : numdofpernode_(numdofpernode),
    numscal_(numscal),
    iselch_((numdofpernode_ - numscal_) == 1),
    isale_(false),
    diffreastafac_(0.0),
    evelnp_(true),   // initialize to zero
    eaccnp_(true),
    eprenp_(true),
    ephi0_Reinit_Reference_(numscal_),
    ephi0_penalty_(numscal_),
    ephinm_(numscal_),   // schott
    ephin_(numscal_),
    ephinp_(numscal_),
    ephiam_(numscal_),
    ehist_(numdofpernode_),
    epotnp_(true),
    emagnetnp_(true),
    fsphinp_(numscal_),
    edispnp_(true),
    xyze_(true),
    weights_(true),
    myknots_(nsd_),
    bodyforce_(numdofpernode_),
    densn_(numscal_),
    densnp_(numscal_),
    densam_(numscal_),
    densgradfac_(numscal_),
    diffus_(numscal_),
    reacoeff_(numscal_),
    valence_(numscal_),
    diffusvalence_(numscal_),
    shcacp_(0.0),
    xsi_(true),
    funct_(true),
    deriv_(true),
    deriv2_(true),
    xjm_(true),
    xij_(true),
    derxy_(true),
    derxy2_(true),
    vderxy_(true),
    rhs_(numdofpernode_),
    reatemprhs_(numdofpernode_),
    hist_(numdofpernode_),
    velint_(true),
    sgvelint_(true),
    migvelint_(true),
    vdiv_(0.0),
    tau_(numscal_),
    sgdiff_(numscal_),
    xder2_(true),
    conv_(true),
    sgconv_(true),
    diff_(true),
    migconv_(true),
    migrea_(true),
    gradpot_(true),
    conint_(numscal_),
    gradphi_(true),
    fsgradphi_(true),
    laplace_(true),
    thermpressnp_(0.0),
    thermpressam_(0.0),
    thermpressdt_(0.0),
    efluxreconstr_(numscal_),
    betterconsistency_(false),
    tauderpot_(numscal_),
    migrationintau_(true),
    migrationstab_(true),
    migrationinresidual_(true),
    reacoeffderiv_(numscal_)
{
  return;
}


// schott
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
//! compute largest element diameter for reinitialization pseudo time step size
template<DRT::Element::DiscretizationType distype, class M1>
double getEleDiameter(const M1& xyze)
{
  double elediam = 0.0;

  // number of nodes of this element
//  const size_t numnode = DRT::UTILS::DisTypeToNumNodePerEle<DISTYPE>::;
  const size_t numnode = DRT::UTILS::DisTypeToNumNodePerEle<distype>::numNodePerElement;

  // check all possible connections between nodes of an element
  // node 1 to 2
  //    :
  // node 1 to 8 = numnode
  // node 2 to 3
  //    :
  // node 2 to 8
  //    :
  //    :
  //    :
  // node 7 to 8
  for(size_t i_start=0; i_start< numnode-2; ++i_start)
  {
    for(size_t i_end= i_start+1; i_end < numnode-1; ++i_end)
    {
      LINALG::Matrix<3,1> direction;
      direction.Clear();
      direction(0) = xyze(0, i_start) - xyze(0, i_end);
      direction(1) = xyze(1, i_start) - xyze(1, i_end);
      direction(2) = xyze(2, i_start) - xyze(2, i_end);

      // update elediam
      if (direction.Norm2() > elediam) elediam=direction.Norm2();
    }
  }

  return elediam;
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
int DRT::ELEMENTS::ScaTraImpl<distype>::Evaluate(
    DRT::Element*              ele,
    ParameterList&             params,
    DRT::Discretization&       discretization,
    vector<int>&               lm,
    Epetra_SerialDenseMatrix&  elemat1_epetra,
    Epetra_SerialDenseMatrix&  elemat2_epetra,
    Epetra_SerialDenseVector&  elevec1_epetra,
    Epetra_SerialDenseVector&  elevec2_epetra,
    Epetra_SerialDenseVector&  elevec3_epetra
    )
{
  // --------mandatory are performed here at first ------------
  // get node coordinates (we do this for all actions!)
  GEO::fillInitialPositionArray<distype,nsd_,LINALG::Matrix<nsd_,nen_> >(ele,xyze_);

  // get additional state vector for ALE case: grid displacement
  isale_ = params.get<bool>("isale");
  if (isale_)
  {
    const RCP<Epetra_MultiVector> dispnp = params.get< RCP<Epetra_MultiVector> >("dispnp",null);
    if (dispnp==null) dserror("Cannot get state vector 'dispnp'");
    DRT::UTILS::ExtractMyNodeBasedValues(ele,edispnp_,dispnp,nsd_);
    // add nodal displacements to point coordinates
    xyze_ += edispnp_;
  }
  else edispnp_.Clear();

  // Now do the nurbs specific stuff (for isogeometric elements)
  if(DRT::NURBS::IsNurbs(distype))
  {
    // access knots and weights for this element
    bool zero_size = DRT::NURBS::GetMyNurbsKnotsAndWeights(discretization,ele,myknots_,weights_);

    // if we have a zero sized element due to a interpolated point -> exit here
    if(zero_size)
      return(0);
  } // Nurbs specific stuff

  // the type of scalar transport problem has to be provided for all actions!
  const INPAR::SCATRA::ScaTraType scatratype = DRT::INPUT::get<INPAR::SCATRA::ScaTraType>(params, "scatratype");
  if (scatratype == INPAR::SCATRA::scatratype_undefined)
    dserror("Set parameter SCATRATYPE in your input file!");

  // check for the action parameter
  const string action = params.get<string>("action","none");
  if (action=="calc_condif_systemmat_and_residual")
  {
    // set flag for including reactive terms to false initially
    // flag will be set to true below when reactive material is included
    reaction_ = false;

    // get control parameters
    is_stationary_  = params.get<bool>("using stationary formulation");
    is_genalpha_    = params.get<bool>("using generalized-alpha time integration");
    is_incremental_ = params.get<bool>("incremental solver");

    // get current time and time-step length
    const double time = params.get<double>("total time");
    const double dt   = params.get<double>("time-step length");

    // get time factor and alpha_F if required
    // one-step-Theta:    timefac = theta*dt
    // BDF2:              timefac = 2/3 * dt
    // generalized-alpha: timefac = alphaF * (gamma*/alpha_M) * dt
    double timefac = 1.0;
    double alphaF  = 1.0;
    if (not is_stationary_)
    {
      timefac = params.get<double>("time factor");
      if (is_genalpha_)
      {
        alphaF = params.get<double>("alpha_F");
        timefac *= alphaF;
      }
      if (timefac < 0.0) dserror("time factor is negative.");
    }

    // set thermodynamic pressure and its time derivative as well as
    // flag for turbulence model if required
    bool turbmodel = false;
    if (scatratype == INPAR::SCATRA::scatratype_loma)
    {
      thermpressnp_ = params.get<double>("thermodynamic pressure");
      thermpressdt_ = params.get<double>("time derivative of thermodynamic pressure");
      if (is_genalpha_)
        thermpressam_ = params.get<double>("thermodynamic pressure at n+alpha_M");

      // set flag for turbulence model
      turbmodel = params.get<bool>("turbulence model");
    }

    // set flag for conservative form
    const INPAR::SCATRA::ConvForm convform =
      DRT::INPUT::get<INPAR::SCATRA::ConvForm>(params, "form of convective term");
    conservative_ = false;
    if (convform ==INPAR::SCATRA::convform_conservative) conservative_ = true;

    bool reinitswitch = params.get<bool>("reinitswitch",false);

    // set parameters for stabilization
    ParameterList& stablist = params.sublist("STABILIZATION");

    // get definition for stabilization parameter tau
    INPAR::SCATRA::TauType whichtau = DRT::INPUT::IntegralValue<INPAR::SCATRA::TauType>(stablist,"DEFINITION_TAU");

    // set correct stationary definition for stabilization parameter automatically
    // and ensure that exact stabilization parameter is only used in stationary case
    if (is_stationary_)
    {
      if (whichtau == INPAR::SCATRA::tau_taylor_hughes_zarins)
        whichtau = INPAR::SCATRA::tau_taylor_hughes_zarins_wo_dt;
      else if (whichtau == INPAR::SCATRA::tau_franca_valentin)
        whichtau = INPAR::SCATRA::tau_franca_valentin_wo_dt;
      else if (whichtau == INPAR::SCATRA::tau_shakib_hughes_codina)
        whichtau = INPAR::SCATRA::tau_shakib_hughes_codina_wo_dt;
      else if (whichtau == INPAR::SCATRA::tau_codina)
        whichtau = INPAR::SCATRA::tau_codina_wo_dt;
    }
    else
    {
      if (whichtau == INPAR::SCATRA::tau_exact_1d)
        dserror("exact stabilization parameter only available for stationary case");
    }

    // set (sign) factor for diffusive and reactive stabilization terms
    // (factor is zero for SUPG) and overwrite tau definition when there
    // is no stabilization
    const INPAR::SCATRA::StabType stabinp = DRT::INPUT::IntegralValue<INPAR::SCATRA::StabType>(stablist,"STABTYPE");
    switch(stabinp)
    {
      case INPAR::SCATRA::stabtype_no_stabilization:
        whichtau = INPAR::SCATRA::tau_zero;
        break;
      case INPAR::SCATRA::stabtype_SUPG:
        diffreastafac_ = 0.0;
        break;
      case INPAR::SCATRA::stabtype_GLS:
        diffreastafac_ = 1.0;
        break;
      case INPAR::SCATRA::stabtype_USFEM:
        diffreastafac_ = -1.0;
        break;
      default:
        dserror("unknown definition for stabilization parameter");
    }

    // set flags for subgrid-scale velocity and all-scale subgrid-diffusivity term
    // (default: "false" for both flags)
    const bool sgvel(DRT::INPUT::IntegralValue<int>(stablist,"SUGRVEL"));
    sgvel_ = sgvel;
    const bool assgd(DRT::INPUT::IntegralValue<int>(stablist,"ASSUGRDIFF"));

    // select type of all-scale subgrid diffusivity if included
    const INPAR::SCATRA::AssgdType whichassgd
    = DRT::INPUT::IntegralValue<INPAR::SCATRA::AssgdType>(stablist,"DEFINITION_ASSGD");

    // set flags for potential evaluation of tau and material law at int. point
    const INPAR::SCATRA::EvalTau tauloc = DRT::INPUT::IntegralValue<INPAR::SCATRA::EvalTau>(stablist,"EVALUATION_TAU");
    tau_gp_ = (tauloc == INPAR::SCATRA::evaltau_integration_point); // set true/false
    const INPAR::SCATRA::EvalMat matloc = DRT::INPUT::IntegralValue<INPAR::SCATRA::EvalMat>(stablist,"EVALUATION_MAT");
    mat_gp_ = (matloc == INPAR::SCATRA::evalmat_integration_point); // set true/false

    // set flag for fine-scale subgrid diffusivity and perform some checks
    bool fssgd = false; //default
    const INPAR::SCATRA::FSSUGRDIFF whichfssgd = DRT::INPUT::get<INPAR::SCATRA::FSSUGRDIFF>(params, "fs subgrid diffusivity");
    if (whichfssgd == INPAR::SCATRA::fssugrdiff_artificial)
    {
      fssgd = true;

      // check for solver type
      if (is_incremental_) dserror("Artificial fine-scale subgrid-diffusivity approach only in combination with non-incremental solver so far!");
    }
    else if (whichfssgd == INPAR::SCATRA::fssugrdiff_smagorinsky_all)
    {
      fssgd = true;

      // check for solver type
      if (not is_incremental_) dserror("Fine-scale subgrid-diffusivity approach using all-scale Smagorinsky model only in combination with incremental solver so far!");
    }
    else if (whichfssgd == INPAR::SCATRA::fssugrdiff_smagorinsky_small)
      dserror("Fine-scale subgrid-diffusivity approach using fine-scale Smagorinsky model not available so far!");

    // check for combination of all-scale and fine-scale subgrid diffusivity
    if (assgd and fssgd) dserror("No combination of all-scale and fine-scale subgrid-diffusivity approach currently possible!");

    // get velocity at nodes
    const RCP<Epetra_MultiVector> velocity = params.get< RCP<Epetra_MultiVector> >("velocity field",null);
    DRT::UTILS::ExtractMyNodeBasedValues(ele,evelnp_,velocity,nsd_);

    // get data required for subgrid-scale velocity: acceleration and pressure
    if (sgvel_)
    {
      // check for matching flags
      if (not mat_gp_ or not tau_gp_) dserror("Evaluation of material and stabilization parameters need to be done at the integration points if subgrid-scale velocity is included!");

      const RCP<Epetra_MultiVector> accpre = params.get< RCP<Epetra_MultiVector> >("acceleration/pressure field",null);
      LINALG::Matrix<nsd_+1,nen_> eaccprenp;
      DRT::UTILS::ExtractMyNodeBasedValues(ele,eaccprenp,accpre,nsd_+1);

      // split acceleration and pressure values
      for (int i=0;i<nen_;++i)
      {
        for (int j=0;j<nsd_;++j)
        {
          eaccnp_(j,i) = eaccprenp(j,i);
        }
        eprenp_(i) = eaccprenp(nsd_,i);
      }
    }

    // extract local values from the global vectors
    RefCountPtr<const Epetra_Vector> hist = discretization.GetState("hist");
    RefCountPtr<const Epetra_Vector> phinp = discretization.GetState("phinp");
    if (hist==null || phinp==null)
      dserror("Cannot get state vector 'hist' and/or 'phinp'");
    vector<double> myhist(lm.size());
    vector<double> myphinp(lm.size());
    DRT::UTILS::ExtractMyValues(*hist,myhist,lm);
    DRT::UTILS::ExtractMyValues(*phinp,myphinp,lm);

    // fill all element arrays
    for (int i=0;i<nen_;++i)
    {
      for (int k = 0; k< numscal_; ++k)
      {
        // split for each transported scalar, insert into element arrays
        ephinp_[k](i,0) = myphinp[k+(i*numdofpernode_)];
      }
      for (int k = 0; k< numdofpernode_; ++k)
      {
        // the history vectors contains information of time step t_n
        ehist_[k](i,0) = myhist[k+(i*numdofpernode_)];
      }
    } // for i

    if ((scatratype == INPAR::SCATRA::scatratype_loma) and is_genalpha_)
    {
      // extract additional local values from global vector
      RefCountPtr<const Epetra_Vector> phiam = discretization.GetState("phiam");
      if (phiam==null) dserror("Cannot get state vector 'phiam'");
      vector<double> myphiam(lm.size());
      DRT::UTILS::ExtractMyValues(*phiam,myphiam,lm);

      // fill element array
      for (int i=0;i<nen_;++i)
      {
        for (int k = 0; k< numscal_; ++k)
        {
          // split for each transported scalar, insert into element arrays
          ephiam_[k](i,0) = myphiam[k+(i*numdofpernode_)];
        }
      } // for i
    }

    if ((is_genalpha_ and not is_incremental_) or (scatratype == INPAR::SCATRA::scatratype_levelset))
    {
      // extract additional local values from global vector
      RefCountPtr<const Epetra_Vector> phin = discretization.GetState("phin");
      if (phin==null) dserror("Cannot get state vector 'phin'");
      vector<double> myphin(lm.size());
      DRT::UTILS::ExtractMyValues(*phin,myphin,lm);

      // fill element array
      for (int i=0;i<nen_;++i)
      {
        for (int k = 0; k< numscal_; ++k)
        {
          // split for each transported scalar, insert into element arrays
          ephin_[k](i,0) = myphin[k+(i*numdofpernode_)];
        }
      } // for i
    }

    double frt(0.0);
    if (iselch_)
    {
      // safety check - only stabilization of SUPG-type available
      if ((stabinp !=INPAR::SCATRA::stabtype_no_stabilization) and (stabinp !=INPAR::SCATRA::stabtype_SUPG))
        dserror("Only SUPG-type stabilization available for ELCH.");

      // get values for el. potential at element nodes
      for (int i=0;i<nen_;++i)
      {
        epotnp_(i) = myphinp[i*numdofpernode_+numscal_];
      }
      // get parameter F/RT needed for ELCH ;-)
      frt = params.get<double>("frt");

      const INPAR::SCATRA::Consistency consistency
        = DRT::INPUT::IntegralValue<INPAR::SCATRA::Consistency>(stablist,"CONSISTENCY");
      betterconsistency_=(consistency==INPAR::SCATRA::consistency_l2_projection_lumped);

      for (int k=0; k < numscal_; k++)
      {
        if (betterconsistency_)
        {
          ostringstream temp;
          temp << k;
          string name = "flux_phi_"+temp.str();
          // try to get the pointer to the entry (and check if type is RCP<Epetra_MultiVector>)
          RCP<Epetra_MultiVector>* f = params.getPtr< RCP<Epetra_MultiVector> >(name);
          if (f!= NULL) // field has been set and is not of type Teuchos::null
          {
            DRT::UTILS::ExtractMyNodeBasedValues(ele,efluxreconstr_[k],*f,nsd_);
          }
          else
            dserror("Could not extract values of flux approximation");
        }
        else
          efluxreconstr_[k].Clear();
      }

      // get magnetic field at nodes (if available)
      // try to get the pointer to the entry (and check if type is RCP<Epetra_MultiVector>)
      RCP<Epetra_MultiVector>* b = params.getPtr< RCP<Epetra_MultiVector> >("magnetic field");
      if (b!= NULL) // magnetic field has been set and is not of type Teuchos::null
          DRT::UTILS::ExtractMyNodeBasedValues(ele,emagnetnp_,*b,nsd_);
       else
        emagnetnp_.Clear();
    }
    else
    {
      epotnp_.Clear();
      emagnetnp_.Clear();
    }

    double Cs(0.0);
    double tpn(1.0);
    // get subgrid-diffusivity vector if turbulence model is used
    if (turbmodel or (is_incremental_ and fssgd))
    {
      // get Smagorinsky constant and turbulent Prandtl number
      Cs  = params.get<double>("Smagorinsky constant");
      tpn = params.get<double>("turbulent Prandtl number");

      if (is_incremental_ and fssgd)
      {
        RCP<const Epetra_Vector> gfsphinp = discretization.GetState("fsphinp");
        if (gfsphinp==null) dserror("Cannot get state vector 'fsphinp'");

        vector<double> myfsphinp(lm.size());
        DRT::UTILS::ExtractMyValues(*gfsphinp,myfsphinp,lm);

        for (int i=0;i<nen_;++i)
        {
          for (int k = 0; k< numscal_; ++k)
          {
            // split for each transported scalar, insert into element arrays
            fsphinp_[k](i,0) = myfsphinp[k+(i*numdofpernode_)];
          }
        }
      }
    }

    // calculate element coefficient matrix and rhs
    Sysmat(
        ele,
        elemat1_epetra,
        elevec1_epetra,
        elevec2_epetra,
        time,
        dt,
        timefac,
        alphaF,
        whichtau,
        whichassgd,
        whichfssgd,
        assgd,
        fssgd,
        turbmodel,
        reinitswitch,
        Cs,
        tpn,
        frt,
        scatratype);
#if 0
    // for debugging of matrix entries
    if(ele->Id()==2) // and (time < 3 or time > 99.0))
    {
      FDcheck(
          ele,
          elemat1_epetra,
          elevec1_epetra,
          elevec2_epetra,
          time,
          dt,
          timefac,
          alphaF,
          whichtau,
          whichassgd,
          whichfssgd,
          assgd,
          fssgd,
          turbmodel,
          reinitswitch,
          Cs,
          tpn,
          frt,
          scatratype);
    }
#endif
  }
  else if (action == "reinitialize_levelset")
  {

	    bool reinitswitch = params.get<bool>("reinitswitch",false);
	    if(reinitswitch == false) dserror("action reinitialize_levelset should be called only with reinitswitch=true");

	    const INPAR::SCATRA::PenaltyMethod reinit_penalty_method = params.get<INPAR::SCATRA::PenaltyMethod>("reinit_penalty_method");
	    const double reinit_epsilon_bandwidth = params.get<double>("reinit_epsilon_bandwidth",false);
	    const double reinit_penalty_interface = params.get<double>("reinit_penalty_interface", false);
	    const INPAR::SCATRA::SmoothedSignType smoothedSignType = params.get<INPAR::SCATRA::SmoothedSignType>("reinit_smoothed_sign_type");
	    const double reinit_pseudo_timestepsize_factor = params.get<double>("reinit_pseudotimestepfactor",false);
	    const INPAR::SCATRA::ReinitializationStrategy reinitstrategy = params.get<INPAR::SCATRA::ReinitializationStrategy>("reinit_strategy");


	    // extract local values from the global vectors
	    RefCountPtr<const Epetra_Vector> phinp = discretization.GetState("phinp");
	    RefCountPtr<const Epetra_Vector> phin  = discretization.GetState("phin");
	    RefCountPtr<const Epetra_Vector> phi0_Reinit_Reference = discretization.GetState("phistart");

	    if (phinp==null || phin==null || phi0_Reinit_Reference==null)
	      dserror("Cannot get state vector 'phinp' or 'phi0_Reinit_Reference'");
	    vector<double> myphinp(lm.size());
	    vector<double> myphin(lm.size());
	    vector<double> myphi0(lm.size());
	    DRT::UTILS::ExtractMyValues(*phinp,myphinp,lm);
	    if (reinitswitch==true) DRT::UTILS::ExtractMyValues(*phin,myphin,lm);
	    if (reinitswitch==true) DRT::UTILS::ExtractMyValues(*phi0_Reinit_Reference,myphi0,lm);


	    // fill all element arrays
	    for (int i=0;i<nen_;++i)
	    {
	      for (int k = 0; k< numscal_; ++k)
	      {
	        // split for each transported scalar, insert into element arrays
	        ephinp_[k](i,0) = myphinp[k+(i*numdofpernode_)];
            // the phi reference vector contains information of pseudo time step tau=0
//            ephin0_Reinit_Reference_[k](i,0) = myphin0[k+(i*numdofpernode_)];
#ifdef PHIN_INSTEAD_OF_PHI_0
	        ephi0_Reinit_Reference_[k](i,0) = myphin[k+(i*numdofpernode_)];
#else
	        ephi0_Reinit_Reference_[k](i,0) = myphi0[k+(i*numdofpernode_)];
#endif
            ephin_[k](i,0) = myphin[k+(i*numdofpernode_)];
            ephi0_penalty_[k](i,0) = myphi0[k+(i*numdofpernode_)];
	      }
	    } // for i


	    if (reinitstrategy == INPAR::SCATRA::reinitstrategy_pdebased_characteristic_galerkin)
	    {
	    		    // calculate element coefficient matrix and rhs
	    SysmatReinitialize(
	        ele,
	        elemat1_epetra,
	        elevec1_epetra,
	        reinitswitch,
	        reinit_pseudo_timestepsize_factor,
	        smoothedSignType,
	        reinitstrategy,
	        reinit_penalty_method,
	        reinit_penalty_interface,
	        reinit_epsilon_bandwidth,
	        scatratype);
	    }
	    else if (reinitstrategy == INPAR::SCATRA::reinitstrategy_pdebased_linear_convection)
	    {
		    const double theta_reinit = params.get<double>("theta_reinit",false);
		    const bool reinit_shock_capturing = params.get<int>("reinit_shock_capturing",false);
		    const double reinit_shock_capturing_diffusivity = params.get<double>("reinit_shock_capturing_diffusivity",false);

		    if(theta_reinit != 1.0) dserror(" correct implementation of hist_vector!!!");
		    const double meshsize = getEleDiameter<distype>(xyze_);
		    const double dt = reinit_pseudo_timestepsize_factor * meshsize;


		    // set flag for including reactive terms to false initially
		    // flag will be set to true below when reactive material is included
		    reaction_ = false;

		    // get control parameters
		    is_stationary_  = params.get<bool>("using stationary formulation");
		    is_genalpha_    = params.get<bool>("using generalized-alpha time integration");
		    is_incremental_ = params.get<bool>("incremental solver");

		    // get time factor and alpha_F if required
		    // one-step-Theta:    timefac = theta*dt
		    // BDF2:              timefac = 2/3 * dt
		    // generalized-alpha: timefac = alphaF * (gamma*/alpha_M) * dt
		    double timefac = 1.0;
		    timefac =  theta_reinit * dt;

		    // set flag for conservative form
		    const INPAR::SCATRA::ConvForm convform =
		      DRT::INPUT::get<INPAR::SCATRA::ConvForm>(params, "form of convective term");
		    conservative_ = false;
		    if (convform ==INPAR::SCATRA::convform_conservative) conservative_ = true;



		    // set parameters for stabilization
		    ParameterList& stablist = params.sublist("STABILIZATION");

		    // get definition for stabilization parameter tau
		    INPAR::SCATRA::TauType whichtau = DRT::INPUT::IntegralValue<INPAR::SCATRA::TauType>(stablist,"DEFINITION_TAU");

		    // set correct stationary definition for stabilization parameter automatically
		    // and ensure that exact stabilization parameter is only used in stationary case
		    if (is_stationary_)
		    {
		      if (whichtau == INPAR::SCATRA::tau_taylor_hughes_zarins)
		        whichtau = INPAR::SCATRA::tau_taylor_hughes_zarins_wo_dt;
		      else if (whichtau == INPAR::SCATRA::tau_franca_valentin)
		        whichtau = INPAR::SCATRA::tau_franca_valentin_wo_dt;
		      else if (whichtau == INPAR::SCATRA::tau_shakib_hughes_codina)
		        whichtau = INPAR::SCATRA::tau_shakib_hughes_codina_wo_dt;
		      else if (whichtau == INPAR::SCATRA::tau_codina)
		        whichtau = INPAR::SCATRA::tau_codina_wo_dt;
		    }
		    else
		    {
		      if (whichtau == INPAR::SCATRA::tau_exact_1d)
		        dserror("exact stabilization parameter only available for stationary case");
		    }

		    // set (sign) factor for diffusive and reactive stabilization terms
		    // (factor is zero for SUPG) and overwrite tau definition when there
		    // is no stabilization
		    const INPAR::SCATRA::StabType stabinp = DRT::INPUT::IntegralValue<INPAR::SCATRA::StabType>(stablist,"STABTYPE");
		    switch(stabinp)
		    {
		      case INPAR::SCATRA::stabtype_no_stabilization:
		        whichtau = INPAR::SCATRA::tau_zero;
		        break;
		      case INPAR::SCATRA::stabtype_SUPG:
		        diffreastafac_ = 0.0;
		        break;
		      case INPAR::SCATRA::stabtype_GLS:
		        diffreastafac_ = 1.0;
		        break;
		      case INPAR::SCATRA::stabtype_USFEM:
		        diffreastafac_ = -1.0;
		        break;
		      default:
		        dserror("unknown definition for stabilization parameter");
		    }

		    // set flags for subgrid-scale velocity and all-scale subgrid-diffusivity term
		    // (default: "false" for both flags)
		    const bool sgvel(false);
		    sgvel_ = sgvel;
		    const bool assgd =false;


		    // set flags for potential evaluation of tau and material law at int. point
		    const INPAR::SCATRA::EvalTau tauloc = DRT::INPUT::IntegralValue<INPAR::SCATRA::EvalTau>(stablist,"EVALUATION_TAU");
		    tau_gp_ = (tauloc == INPAR::SCATRA::evaltau_integration_point); // set true/false
		    const INPAR::SCATRA::EvalMat matloc = DRT::INPUT::IntegralValue<INPAR::SCATRA::EvalMat>(stablist,"EVALUATION_MAT");
		    mat_gp_ = (matloc == INPAR::SCATRA::evalmat_integration_point); // set true/false

		    // set flag for fine-scale subgrid diffusivity and perform some checks
		    bool fssgd = false; //default

		    // check for combination of all-scale and fine-scale subgrid diffusivity
		    if (assgd and fssgd) dserror("No combination of all-scale and fine-scale subgrid-diffusivity approach currently possible!");

		    // get velocity at nodes
		    const RCP<Epetra_MultiVector> reinit_velocity = params.get< RCP<Epetra_MultiVector> >("reinit velocity field",null);
		    DRT::UTILS::ExtractMyNodeBasedValues(ele,evelnp_,reinit_velocity,nsd_);

		    // get data required for subgrid-scale velocity: acceleration and pressure
		    if (sgvel_)
		    {
		      // check for matching flags
		      if (not mat_gp_ or not tau_gp_) dserror("Evaluation of material and stabilization parameters need to be done at the integration points if subgrid-scale velocity is included!");

		      const RCP<Epetra_MultiVector> accpre = params.get< RCP<Epetra_MultiVector> >("acceleration/pressure field",null);
		      LINALG::Matrix<nsd_+1,nen_> eaccprenp;
		      DRT::UTILS::ExtractMyNodeBasedValues(ele,eaccprenp,accpre,nsd_+1);

		      // split acceleration and pressure values
		      for (int i=0;i<nen_;++i)
		      {
		        for (int j=0;j<nsd_;++j)
		        {
		          eaccnp_(j,i) = eaccprenp(j,i);
		        }
		        eprenp_(i) = eaccprenp(nsd_,i);
		      }
		    }


		    // calculate element coefficient matrix and rhs
		    SysmatLinearAdvectionSysmat(
		        ele,
		        elemat1_epetra,
		        elevec1_epetra,
		        elevec2_epetra,
		        dt,
		        timefac,
		        meshsize,
		        whichtau,
		        reinitswitch,
		        reinit_pseudo_timestepsize_factor,
		        smoothedSignType,
		        reinitstrategy,
		        reinit_penalty_method,
		        reinit_penalty_interface,
		        reinit_epsilon_bandwidth,
		        reinit_shock_capturing,
		        reinit_shock_capturing_diffusivity,
		        scatratype);
	    }
	    else dserror("reinitstrategy not a known type");


  }
  // calculate time derivative for time value t_0
  else if (action =="calc_initial_time_deriv")
  {
    // set flag for including reactive terms to false initially
    // flag will be set to true below when reactive material is included
    reaction_ = false;

    // set flag for conservative form
    const INPAR::SCATRA::ConvForm convform =
      DRT::INPUT::get<INPAR::SCATRA::ConvForm>(params, "form of convective term");
    conservative_ = false;
    if (convform ==INPAR::SCATRA::convform_conservative) conservative_ = true;

    // get initial velocity values at the nodes
    const RCP<Epetra_MultiVector> velocity = params.get< RCP<Epetra_MultiVector> >("velocity field",null);
    DRT::UTILS::ExtractMyNodeBasedValues(ele,evelnp_,velocity,nsd_);

    // need initial field -> extract local values from the global vector
    RefCountPtr<const Epetra_Vector> phi0 = discretization.GetState("phi0");
    if (phi0==null) dserror("Cannot get state vector 'phi0'");
    vector<double> myphi0(lm.size());
    DRT::UTILS::ExtractMyValues(*phi0,myphi0,lm);

    // fill element arrays
    for (int i=0;i<nen_;++i)
    {
      for (int k = 0; k< numscal_; ++k)
      {
        // split for each transported scalar, insert into element arrays
        ephinp_[k](i,0) = myphi0[k+(i*numdofpernode_)];
      }
    } // for i

    // set time derivative of thermodynamic pressure if required
    if (scatratype ==INPAR::SCATRA::scatratype_loma)
    {
      thermpressnp_ = params.get<double>("thermodynamic pressure");
      thermpressam_ = thermpressnp_;
      thermpressdt_ = params.get<double>("time derivative of thermodynamic pressure");
    }

    bool reinitswitch = params.get<bool>("reinitswitch",false);

    // get stabilization parameter sublist
    ParameterList& stablist = params.sublist("STABILIZATION");

    // set flags for potential evaluation of material law at int. point
    const INPAR::SCATRA::EvalMat matloc = DRT::INPUT::IntegralValue<INPAR::SCATRA::EvalMat>(stablist,"EVALUATION_MAT");
    mat_gp_ = (matloc == INPAR::SCATRA::evalmat_integration_point); // set true/false

    double frt(0.0);
    if(SCATRA::IsElchProblem(scatratype))
    {
      for (int i=0;i<nen_;++i)
      {
        // get values for el. potential at element nodes
        epotnp_(i) = myphi0[i*numdofpernode_+numscal_];
      } // for i

      // get parameter F/RT
      frt = params.get<double>("frt");
    }
    else epotnp_.Clear();

    // set control parameters to avoid that some actually unused variables are
    // falsely set, on the one hand, and viscosity for unnecessary calculation
    // of subgrid-scale velocity is computed, on the other hand, in
    // GetMaterialParams
    is_genalpha_    = params.get<bool>("using generalized-alpha time integration");
    is_incremental_ = params.get<bool>("incremental solver");
    sgvel_ = DRT::INPUT::IntegralValue<int>(stablist,"SUGRVEL");

    // calculate matrix and rhs
    InitialTimeDerivative(
        ele,
        elemat1_epetra,
        elevec1_epetra,
        reinitswitch,
        frt,
        scatratype);
  }
  else if (action =="calc_subgrid_diffusivity_matrix")
  // calculate normalized subgrid-diffusivity matrix
  {
    // get control parameter
    is_genalpha_   = params.get<bool>("using generalized-alpha time integration");
    is_stationary_ = params.get<bool>("using stationary formulation");

    // One-step-Theta:    timefac = theta*dt
    // BDF2:              timefac = 2/3 * dt
    // generalized-alpha: timefac = alphaF * (gamma*/alpha_M) * dt
    double timefac = 1.0;
    double alphaF  = 1.0;
    if (not is_stationary_)
    {
      timefac = params.get<double>("time factor");
      if (is_genalpha_)
      {
        alphaF = params.get<double>("alpha_F");
        timefac *= alphaF;
      }
      if (timefac < 0.0) dserror("time factor is negative.");
    }

    // calculate mass matrix and rhs
    CalcSubgridDiffMatrix(
        ele,
        elemat1_epetra,
        timefac);
  }
  else if (action=="calc_condif_flux")
  {
    // get velocity values at the nodes
    const RCP<Epetra_MultiVector> velocity = params.get< RCP<Epetra_MultiVector> >("velocity field",null);
    DRT::UTILS::ExtractMyNodeBasedValues(ele,evelnp_,velocity,nsd_);

    // need current values of transported scalar
    // -> extract local values from global vectors
    RefCountPtr<const Epetra_Vector> phinp = discretization.GetState("phinp");
    if (phinp==null) dserror("Cannot get state vector 'phinp'");
    vector<double> myphinp(lm.size());
    DRT::UTILS::ExtractMyValues(*phinp,myphinp,lm);

    // fill all element arrays
    for (int i=0;i<nen_;++i)
    {
      for (int k = 0; k< numscal_; ++k)
      {
        // split for each transported scalar, insert into element arrays
        ephinp_[k](i,0) = myphinp[k+(i*numdofpernode_)];
      }
    } // for i

    // access control parameter for flux calculation
    INPAR::SCATRA::FluxType fluxtype = DRT::INPUT::get<INPAR::SCATRA::FluxType>(params, "fluxtype");

    // set flag for potential evaluation of material law at int. point
    ParameterList& stablist = params.sublist("STABILIZATION");
    const INPAR::SCATRA::EvalMat matloc = DRT::INPUT::IntegralValue<INPAR::SCATRA::EvalMat>(stablist,"EVALUATION_MAT");
    mat_gp_ = (matloc == INPAR::SCATRA::evalmat_integration_point); // set true/false

    // initialize parameter F/RT for ELCH
    double frt(0.0);

    // set values for ELCH
    if (SCATRA::IsElchProblem(scatratype))
    {
      // get values for el. potential at element nodes
      for (int i=0;i<nen_;++i)
      {
        epotnp_(i) = myphinp[i*numdofpernode_+numscal_];
      }

      // get parameter F/RT
      frt = params.get<double>("frt");
    }

    // set control parameters to avoid that some actually unused variables are
    // falsely set, on the one hand, and viscosity for unnecessary calculation
    // of subgrid-scale velocity is computed, on the other hand, in
    // GetMaterialParams
    is_genalpha_    = false;
    is_incremental_ = true;
    sgvel_          = false;

    // we always get an 3D flux vector for each node
    LINALG::Matrix<3,nen_> eflux(true);

    // do a loop for systems of transported scalars
    for (int idof = 0; idof<numscal_; ++idof)
    {
      // calculate flux vectors for actual scalar
      eflux.Clear();
      CalculateFlux(eflux,ele,frt,fluxtype,idof,scatratype);
      // assembly
      for (int inode=0;inode<nen_;inode++)
      {
        const int fvi = inode*numdofpernode_+idof;
        elevec1_epetra[fvi]+=eflux(0,inode);
        elevec2_epetra[fvi]+=eflux(1,inode);
        elevec3_epetra[fvi]+=eflux(2,inode);
      }
    } // loop over numscal
  }
  else if (action=="calc_mean_scalars")
  {
    // NOTE: add integral values only for elements which are NOT ghosted!
    if (ele->Owner() == discretization.Comm().MyPID())
    {
      // get flag for inverting
      bool inverting = params.get<bool>("inverting");

      // need current scalar vector
      // -> extract local values from the global vectors
      RefCountPtr<const Epetra_Vector> phinp = discretization.GetState("phinp");
      if (phinp==null) dserror("Cannot get state vector 'phinp'");
      vector<double> myphinp(lm.size());
      DRT::UTILS::ExtractMyValues(*phinp,myphinp,lm);

      // calculate scalars and domain integral
      CalculateScalars(ele,myphinp,elevec1_epetra,inverting);
    }
  }
  else if (action=="calc_domain_and_bodyforce")
  {
    // NOTE: add integral values only for elements which are NOT ghosted!
    if (ele->Owner() == discretization.Comm().MyPID())
    {
      const double time = params.get<double>("total time");

      bool reinitswitch = params.get<bool>("reinitswitch",false);

      // calculate domain and bodyforce integral
      CalculateDomainAndBodyforce(elevec1_epetra,ele,time,reinitswitch);
    }
  }
  else if (action=="calc_error")
  {
    // check if length suffices
    if (elevec1_epetra.Length() < 1) dserror("Result vector too short");

    // need current solution
    RefCountPtr<const Epetra_Vector> phinp = discretization.GetState("phinp");
    if (phinp==null) dserror("Cannot get state vector 'phinp'");

    // extract local values from the global vector
    vector<double> myphinp(lm.size());
    DRT::UTILS::ExtractMyValues(*phinp,myphinp,lm);

    if (numscal_ != 2)
      dserror("Numscal_ != 2 for error calculation of existing examples");

    // fill element arrays
    for (int i=0;i<nen_;++i)
    {
      // split for each transported scalar, insert into element arrays
      for (int k = 0; k< numscal_; ++k)
      {
        ephinp_[k](i) = myphinp[k+(i*numdofpernode_)];
      }
      // get values for el. potential at element nodes
      epotnp_(i) = myphinp[i*numdofpernode_+numscal_];
    } // for i

    CalErrorComparedToAnalytSolution(
        ele,
        scatratype,
        params,
        elevec1_epetra);
  }
  else if (action=="integrate_shape_functions")
  {
    // calculate integral of shape functions
    const Epetra_IntSerialDenseVector dofids = params.get<Epetra_IntSerialDenseVector>("dofids");
    IntegrateShapeFunctions(ele,elevec1_epetra,dofids);
  }
  else if (action=="calc_elch_conductivity")
  {
    if(iselch_)
    {
      // calculate conductivity of electrolyte solution
      const double frt = params.get<double>("frt");
      // extract local values from the global vector
      RefCountPtr<const Epetra_Vector> phinp = discretization.GetState("phinp");
      vector<double> myphinp(lm.size());
      DRT::UTILS::ExtractMyValues(*phinp,myphinp,lm);

      // fill element arrays
      for (int i=0;i<nen_;++i)
      {
        for (int k = 0; k< numscal_; ++k)
        {
          // split for each transported scalar, insert into element arrays
          ephinp_[k](i,0) = myphinp[k+(i*numdofpernode_)];
        }
      } // for i

      CalculateConductivity(ele,frt,scatratype,elevec1_epetra);
    }
    else // conductivity = diffusivity for a electric potential field
    {
      GetMaterialParams(ele,scatratype);
      elevec1_epetra(0)=diffus_[0];
      elevec1_epetra(1)=diffus_[0];
    }
  }
  else if (action=="get_material_parameters")
  {
    // get the material
    RefCountPtr<MAT::Material> material = ele->Material();

    if (material->MaterialType() == INPAR::MAT::m_sutherland)
    {
      const MAT::Sutherland* actmat = static_cast<const MAT::Sutherland*>(material.get());

      params.set("thermodynamic pressure",actmat->ThermPress());
    }
    else params.set("thermodynamic pressure",0.0);
  }
  else if (action =="calc_time_deriv_reinit")
  {
    // set flag for including reactive terms to false initially
    // flag will be set to true below when reactive material is included
    reaction_ = false;

    // set flag for conservative form
    const INPAR::SCATRA::ConvForm convform = DRT::INPUT::get<INPAR::SCATRA::ConvForm>(params, "form of convective term");
    conservative_ = false;
    if (convform ==INPAR::SCATRA::convform_conservative) conservative_ = true;

    // get initial velocity values at the nodes
    const RCP<Epetra_MultiVector> velocity = params.get< RCP<Epetra_MultiVector> >("velocity field",null);
    DRT::UTILS::ExtractMyNodeBasedValues(ele,evelnp_,velocity,nsd_);

    // need initial field -> extract local values from the global vector
    RefCountPtr<const Epetra_Vector> phi0 = discretization.GetState("phi0");
    if (phi0==null) dserror("Cannot get state vector 'phi0'");
    vector<double> myphi0(lm.size());
    DRT::UTILS::ExtractMyValues(*phi0,myphi0,lm);

    // fill element arrays
    for (int i=0;i<nen_;++i)
    {
      for (int k = 0; k< numscal_; ++k)
      {
        // split for each transported scalar, insert into element arrays
        ephinp_[k](i,0) = myphi0[k+(i*numdofpernode_)];
      }
    } // for i

    // get stabilization parameter sublist
    ParameterList& stablist = params.sublist("STABILIZATION");

    // select tau definition
    INPAR::SCATRA::TauType whichtau = DRT::INPUT::IntegralValue<INPAR::SCATRA::TauType>(stablist,"DEFINITION_TAU");

    // get time-step length
    const double dt   = params.get<double>("time-step length");

    // get time factor and alpha_F if required
    // one-step-Theta:    timefac = theta*dt
    // BDF2:              timefac = 2/3 * dt
    // generalized-alpha: timefac = alphaF * (gamma*/alpha_M) * dt
    double timefac = 1.0;
    double alphaF  = 1.0;
    if (not is_stationary_)
    {
      timefac = params.get<double>("time factor");
      if (is_genalpha_)
      {
        alphaF = params.get<double>("alpha_F");
        timefac *= alphaF;
      }
      if (timefac < 0.0) dserror("time factor is negative.");
    }

    // set flags for potential evaluation of material law at int. point
    const INPAR::SCATRA::EvalMat matloc = DRT::INPUT::IntegralValue<INPAR::SCATRA::EvalMat>(stablist,"EVALUATION_MAT");
    mat_gp_ = (matloc == INPAR::SCATRA::evalmat_integration_point); // set true/false

    epotnp_.Clear();

    // calculate matrix and rhs
    TimeDerivativeReinit(
        ele,
        elemat1_epetra,
        elevec1_epetra,
        whichtau,
        dt,
        timefac,
        scatratype
        );
  }
  // calculate initial electric potential field caused by initial ion concentrations
  else if (action =="calc_initial_potential_field")
  {
    // need initial field -> extract local values from the global vector
    RefCountPtr<const Epetra_Vector> phi0 = discretization.GetState("phi0");
    if (phi0==null) dserror("Cannot get state vector 'phi0'");
    vector<double> myphi0(lm.size());
    DRT::UTILS::ExtractMyValues(*phi0,myphi0,lm);

    // fill element arrays
    for (int i=0;i<nen_;++i)
    {
      for (int k = 0; k< numscal_; ++k)
      {
        // split for each transported scalar, insert into element arrays
        ephinp_[k](i,0) = myphi0[k+(i*numdofpernode_)];
      }
    } // for i
    const double frt = params.get<double>("frt");

    CalculateElectricPotentialField(ele,frt,scatratype,elemat1_epetra,elevec1_epetra);

  }
  else if (action == "levelset_TaylorGalerkin")
  {
	    // get timealgo
	    const INPAR::SCATRA::TimeIntegrationScheme timealgo = DRT::INPUT::get<INPAR::SCATRA::TimeIntegrationScheme>(params, "timealgo");

	    // get current time-step length
	    const double dt   = params.get<double>("time-step length");

	    // get velocity at nodes
	    const RCP<Epetra_MultiVector> velocity = params.get< RCP<Epetra_MultiVector> >("velocity field",null);
	    DRT::UTILS::ExtractMyNodeBasedValues(ele,evelnp_,velocity,nsd_);

	    // extract local values from the global vectors
	    RefCountPtr<const Epetra_Vector> phinp = discretization.GetState("phinp");
	    RefCountPtr<const Epetra_Vector> phin  = discretization.GetState("phin");
	    RefCountPtr<const Epetra_Vector> phinm = discretization.GetState("phinm");

	    if (phinp==null || phin==null || phinm==null)
	      dserror("Cannot get state vector 'phinp' or 'phin_' or 'phinm'");
	    vector<double> myphinp(lm.size());
	    vector<double> myphin(lm.size());
	    vector<double> myphinm(lm.size());

	    DRT::UTILS::ExtractMyValues(*phinp,myphinp,lm);
	    DRT::UTILS::ExtractMyValues(*phin,myphin,lm);
	    if(timealgo == INPAR::SCATRA::timeint_tg4_leapfrog)    DRT::UTILS::ExtractMyValues(*phinm,myphinm,lm);


	    // fill all element arrays
	    for (int i=0;i<nen_;++i)
	    {
	      for (int k = 0; k< numscal_; ++k)
	      {
	        // split for each transported scalar, insert into element arrays
	        ephinp_[k](i,0) = myphinp[k+(i*numdofpernode_)];
            ephin_[k](i,0) = myphin[k+(i*numdofpernode_)];
            if (timealgo == INPAR::SCATRA::timeint_tg4_leapfrog)  ephinm_[k](i,0) = myphinm[k+(i*numdofpernode_)];
	      }
	    } // for i


	    // calculate element coefficient matrix and rhs
	    Sysmat_TaylorGalerkin(
	        ele,
	        elemat1_epetra,
	        elevec1_epetra,
	        dt,
	        scatratype,
	        timealgo);
  }
  else if (action=="calc_error_reinit")
  {
      // add error only for elements which are not ghosted
//cout << ele->Owner() << endl;
      if(ele->Owner() == discretization.Comm().MyPID())
      {

	    // need current solution
	    RefCountPtr<const Epetra_Vector> phinp = discretization.GetState("phinp");
	    if (phinp==null) dserror("Cannot get state vector 'phinp'");

	    // extract local values from the global vector
	    vector<double> myphinp(lm.size());
	    DRT::UTILS::ExtractMyValues(*phinp,myphinp,lm);


	    // fill element arrays
	    for (int i=0;i<nen_;++i)
	    {
	      // split for each transported scalar, insert into element arrays
	      for (int k = 0; k< numscal_; ++k)
	      {
	        ephinp_[k](i) = myphinp[k+(i*numdofpernode_)];
	      }
	    } // for i

	    CalErrorsReinitialization(
	        ele,
	        params);
      }
  }
  else
    dserror("Unknown type of action for Scatra Implementation: %s",action.c_str());
  // work is done
  return 0;
}


/*----------------------------------------------------------------------*
 |  calculate system matrix and rhs (public)                schott 05/11|
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::Sysmat_TaylorGalerkin(
    DRT::Element*                         ele, ///< the element those matrix is calculated
    Epetra_SerialDenseMatrix&             sys_mat,///< element matrix to calculate
    Epetra_SerialDenseVector&             residual, ///< element rhs to calculate
    const double                          dt, ///< current time-step length
    const enum INPAR::SCATRA::ScaTraType  scatratype, ///< type of scalar transport problem
    const enum INPAR::SCATRA::TimeIntegrationScheme timealgo
)
{
  //----------------------------------------------------------------------
  // calculation of element volume both for tau at ele. cent. and int. pt.
  //----------------------------------------------------------------------
  // use one-point Gauss rule to do calculations at the element center
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints_tau(SCATRA::DisTypeToStabGaussRule<distype>::rule);

  // volume of the element (2D: element surface area; 1D: element length)
  // (Integration of f(x) = 1 gives exactly the volume/surface/length of element)
  EvalShapeFuncAndDerivsAtIntPoint(intpoints_tau,0,ele->Id());

  //----------------------------------------------------------------------
  // get material parameters (evaluation at element center)
  //----------------------------------------------------------------------
  if (not mat_gp_ or not tau_gp_) GetMaterialParams(ele, scatratype);


  //----------------------------------------------------------------------
  // integration loop for one element
  //----------------------------------------------------------------------

  if (scatratype==  INPAR::SCATRA::scatratype_levelset)
  {
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints(SCATRA::DisTypeToGaussRuleForExactSol<distype>::rule);

  // Assemble element rhs and vector for domain!!! integrals
  for (int iquad=0; iquad<intpoints.IP().nquad; ++iquad)
  {
	const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());

    // get velocity at integration point
    velint_.Multiply(evelnp_,funct_);


    //----------------------------------------------------------------------
    // get material parameters (evaluation at integration point)
    //----------------------------------------------------------------------
    if (mat_gp_) GetMaterialParams(ele, scatratype);

    for (int k=0;k<numscal_;++k) // deal with a system of transported scalars
    {
    	// REMARK: schott 12/10
    	// the bodyforce vector is evaluated at each Gaussian point as nonlinear function
    	// a node-wise rhs_-vector for the reinitialization bodyforce (smoothed sign-function) leads not the desired results

		// compute matrix and rhs
    	if(timealgo == INPAR::SCATRA::timeint_tg2)
    	{
            CalMatAndRHS_TG2                         (sys_mat,
                                                      residual,
                                                      fac,
                                                      k,
                                                      ele,
                                                      dt
                                                      );
    	}
    	else if(timealgo == INPAR::SCATRA::timeint_tg2_LW)
    	{
            CalMatAndRHS_TG2_LW                      (sys_mat,
                                                      residual,
                                                      fac,
                                                      k,
                                                      ele,
                                                      dt
                                                      );
    	}
    	else if(timealgo == INPAR::SCATRA::timeint_tg3)
    	{
            CalMatAndRHS_TG3                         (sys_mat,
                                                      residual,
                                                      fac,
                                                      k,
                                                      ele,
                                                      dt
                                                      );
    	}
    	else if (timealgo == INPAR::SCATRA::timeint_tg4_leapfrog)
    	{
    		CalMatAndRHS_TG4_Leapfrog   (sys_mat,
                                         residual,
                                         fac,
                                         k,
                                         ele,
                                         dt
                                         );
    	}
    	else if (timealgo == INPAR::SCATRA::timeint_tg4_onestep)
    	{
    		CalMatAndRHS_TG4_1S         (sys_mat,
                                         residual,
                                         fac,
                                         k,
                                         ele,
                                         dt
                                         );
    	}
    	else dserror("no characteristic/Taylor Galerkin method chosen here");

    } // loop over each scalar
  } // integration loop

  }
  else // 'standard' scalar transport
  {
	  cout << "WRONG NOW" << endl;
  }

  return;
}

/*----------------------------------------------------------------------*
 |  calculate system matrix and rhs (public)                 g.bau 08/08|
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::Sysmat(
    DRT::Element*                         ele, ///< the element those matrix is calculated
    Epetra_SerialDenseMatrix&             sys_mat,///< element matrix to calculate
    Epetra_SerialDenseVector&             residual, ///< element rhs to calculate
    Epetra_SerialDenseVector&             subgrdiff, ///< subgrid-diff.-scaling vector
    const double                          time, ///< current simulation time
    const double                          dt, ///< current time-step length
    const double                          timefac, ///< time discretization factor
    const double                          alphaF, ///< factor for generalized-alpha time integration
    const enum INPAR::SCATRA::TauType     whichtau, ///< stabilization parameter definition
    const enum INPAR::SCATRA::AssgdType   whichassgd, ///< all-scale subgrid-diffusivity definition
    const enum INPAR::SCATRA::FSSUGRDIFF  whichfssgd, ///< fine-scale subgrid-diffusivity definition
    const bool                            assgd, ///< all-scale subgrid-diff. flag
    const bool                            fssgd, ///< fine-scale subgrid-diff. flag
    const bool                            turbmodel, ///< turbulence model flag
    const bool                            reinitswitch,
    const double                          Cs, ///< Smagorinsky constant
    const double                          tpn, ///< turbulent Prandtl number
    const double                          frt, ///< factor F/RT needed for ELCH calculations
    const enum INPAR::SCATRA::ScaTraType  scatratype ///< type of scalar transport problem
)
{
  // ---------------------------------------------------------------------
  // call routine for calculation of body force in element nodes
  // (time n+alpha_F for generalized-alpha scheme, at time n+1 otherwise)
  // ---------------------------------------------------------------------
//REINHARD
  if (reinitswitch == false)
    BodyForce(ele,time);
  else
    BodyForceReinit(ele,time);
//end REINHARD

  //----------------------------------------------------------------------
  // calculation of element volume both for tau at ele. cent. and int. pt.
  //----------------------------------------------------------------------
  // use one-point Gauss rule to do calculations at the element center
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints_tau(SCATRA::DisTypeToStabGaussRule<distype>::rule);

  // volume of the element (2D: element surface area; 1D: element length)
  // (Integration of f(x) = 1 gives exactly the volume/surface/length of element)
  const double vol = EvalShapeFuncAndDerivsAtIntPoint(intpoints_tau,0,ele->Id());

  //----------------------------------------------------------------------
  // get material parameters (evaluation at element center)
  //----------------------------------------------------------------------
  if (not mat_gp_ or not tau_gp_) GetMaterialParams(ele,scatratype);

  //----------------------------------------------------------------------
  // calculation of subgrid diffusivity and stabilization parameter(s)
  // at element center
  //----------------------------------------------------------------------
  if (not tau_gp_)
  {
    // get velocity at element center
    velint_.Multiply(evelnp_,funct_);

    bool twoionsystem(false);
    double resdiffus(diffus_[0]);
    if (iselch_) // electrochemistry problem
    {
      // when migration velocity is included to tau (we provide always now)
      {
        // compute global derivatives
        derxy_.Multiply(xij_,deriv_);

        // get "migration velocity" divided by D_k*z_k at element center
        migvelint_.Multiply(-frt,derxy_,epotnp_);
      }

      // ELCH: special stabilization in case of binary electrolytes
      twoionsystem= SCATRA::IsBinaryElectrolyte(valence_);
      if (twoionsystem)
      {
        std::vector<int> indices_twoions = SCATRA::GetIndicesBinaryElectrolyte(valence_);
        resdiffus = SCATRA::CalResDiffCoeff(valence_,diffus_,indices_twoions);
#ifdef ACTIVATEBINARYELECTROLYTE
       migrationstab_=false;
       migrationintau_=false;
#endif
      }
    }

    for (int k = 0;k<numscal_;++k) // loop of each transported scalar
    {
      // calculation of all-scale subgrid diffusivity (artificial or due to
      // constant-coefficient Smagorinsky model) at element center
      if (assgd or turbmodel)
        CalcSubgrDiff(dt,timefac,whichassgd,assgd,turbmodel,Cs,tpn,vol,k);

      // calculation of fine-scale artificial subgrid diffusivity at element center
      if (fssgd) CalcFineScaleSubgrDiff(ele,subgrdiff,whichfssgd,Cs,tpn,vol,k);

#ifdef ACTIVATEBINARYELECTROLYTE
      if (twoionsystem && (abs(valence_[k])>EPS10))
        CalTau(ele,resdiffus,dt,timefac,whichtau,vol,k,frt,false);
      else
#endif
      // calculation of stabilization parameter at element center
      CalTau(ele,diffus_[k],dt,timefac,whichtau,vol,k,frt,migrationintau_);
    }
  }

  //----------------------------------------------------------------------
  // integration loop for one element
  //----------------------------------------------------------------------
  // integrations points and weights
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints(SCATRA::DisTypeToOptGaussRule<distype>::rule);


  //TODO
  // integration loop
  if (iselch_) // electrochemistry problem
  {
    for (int iquad=0; iquad<intpoints.IP().nquad; ++iquad)
    {
      const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());

      //----------------------------------------------------------------------
      // get material parameters (evaluation at integration point)
      //----------------------------------------------------------------------
      if (mat_gp_) GetMaterialParams(ele, scatratype);

      // get velocity at integration point
      velint_.Multiply(evelnp_,funct_);

      // convective part in convective form: u_x*N,x + u_y*N,y + u_z*N,z
      conv_.MultiplyTN(derxy_,velint_);

      // momentum divergence required for conservative form
      if (conservative_) GetDivergence(vdiv_,evelnp_,derxy_);

      //--------------------------------------------------------------------
      // calculation of subgrid diffusivity and stabilization parameter(s)
      // at integration point
      //--------------------------------------------------------------------
      if (tau_gp_)
      {
        // compute global derivatives
        derxy_.Multiply(xij_,deriv_);

        // get "migration velocity" divided by D_k*z_k at element center
        migvelint_.Multiply(-frt,derxy_,epotnp_);

        // ELCH: special stabilization in case of binary electrolytes
        bool twoionsystem(false);
        double resdiffus(diffus_[0]);
        twoionsystem = SCATRA::IsBinaryElectrolyte(valence_);
        if (twoionsystem)
        {
          std::vector<int> indices_twoions = SCATRA::GetIndicesBinaryElectrolyte(valence_);
          resdiffus = SCATRA::CalResDiffCoeff(valence_,diffus_,indices_twoions);
#ifdef ACTIVATEBINARYELECTROLYTE
        migrationstab_=false;
        migrationintau_=false;
#endif
        }

        for (int k = 0;k<numscal_;++k) // loop of each transported scalar
        {
          // calculation of all-scale subgrid diffusivity (artificial or due to
          // constant-coefficient Smagorinsky model) at integration point
          if (assgd or turbmodel)
            CalcSubgrDiff(dt,timefac,whichassgd,assgd,turbmodel,Cs,tpn,vol,k);

          // calculation of fine-scale artificial subgrid diffusivity
          // at integration point
          if (fssgd) CalcFineScaleSubgrDiff(ele,subgrdiff,whichfssgd,Cs,tpn,vol,k);

#ifdef ACTIVATEBINARYELECTROLYTE
          // use resulting diffusion coefficient for binary electrolyte solutions
          if (twoionsystem && (abs(valence_[k])>EPS10))
            CalTau(ele,resdiffus,dt,timefac,whichtau,vol,k,frt,false);
          else
#endif
          // calculation of stabilization parameter at integration point
          CalTau(ele,diffus_[k],dt,timefac,whichtau,vol,k,frt,migrationintau_);
        }
      }

      for (int k = 0;k<numscal_;++k) // loop of each transported scalar
      {
        // get history data at integration point
        hist_[k] = funct_.Dot(ehist_[k]);
        // get bodyforce at integration point
        rhs_[k] = bodyforce_[k].Dot(funct_);
      }

      // safety check
      if (!is_incremental_)
        dserror("ELCH problems are always in incremental formulation");

      // compute matrix and rhs for electrochemistry problem
      CalMatElch(sys_mat,residual,frt,timefac,alphaF,fac,scatratype);
    }

  }
  else if ((scatratype == INPAR::SCATRA::scatratype_levelset) and reinitswitch == true)
  {
//REINHARD
    dserror("Due to Volkers commit on 14.9.09 things have to be rearranged!");
// 	  Epetra_SerialDenseMatrix gradphi_gpscur(8,3);
//	  // up to now only implemented for Hex8 elements...
//		  //calculation of expol_temp, is then inverted and overwritten
//	  Epetra_SerialDenseMatrix expol_invert(8,8);
//	  LINALG::Matrix<8,8> expol_temp;
//
//	  double sq3=sqrt(3.0);
//	  expol_temp(0,0)=1.25+0.75*sq3;
//	  expol_temp(0,1)=-0.25-0.25*sq3;
//	  expol_temp(0,2)=-0.25+0.25*sq3;
//	  expol_temp(0,3)=-0.25-0.25*sq3;
//	  expol_temp(0,4)=-0.25-0.25*sq3;
//	  expol_temp(0,5)=-0.25+0.25*sq3;
//	  expol_temp(0,6)=1.25-0.75*sq3;
//	  expol_temp(0,7)=-0.25+0.25*sq3;
//	  expol_temp(1,1)=1.25+0.75*sq3;
//	  expol_temp(1,2)=-0.25-0.25*sq3;
//	  expol_temp(1,3)=-0.25+0.25*sq3;
// 		  expol_temp(1,4)=-0.25+0.25*sq3;
// 		  expol_temp(1,5)=-0.25-0.25*sq3;
// 		  expol_temp(1,6)=-0.25+0.25*sq3;
// 		  expol_temp(1,7)=1.25-0.75*sq3;
// 		  expol_temp(2,2)=1.25+0.75*sq3;
// 		  expol_temp(2,3)=-0.25-0.25*sq3;
// 		  expol_temp(2,4)=1.25-0.75*sq3;
// 		  expol_temp(2,5)=-0.25+0.25*sq3;
// 		  expol_temp(2,6)=-0.25-0.25*sq3;
// 		  expol_temp(2,7)=-0.25+0.25*sq3;
// 		  expol_temp(3,3)=1.25+0.75*sq3;
// 		  expol_temp(3,4)=-0.25+0.25*sq3;
// 		  expol_temp(3,5)=1.25-0.75*sq3;
// 		  expol_temp(3,6)=-0.25+0.25*sq3;
// 		  expol_temp(3,7)=-0.25-0.25*sq3;
// 		  expol_temp(4,4)=1.25+0.75*sq3;
// 		  expol_temp(4,5)=-0.25-0.25*sq3;
// 		  expol_temp(4,6)=-0.25+0.25*sq3;
// 		  expol_temp(4,7)=-0.25-0.25*sq3;
// 		  expol_temp(5,5)=1.25+0.75*sq3;
// 		  expol_temp(5,6)=-0.25-0.25*sq3;
// 		  expol_temp(5,7)=-0.25+0.25*sq3;
// 		  expol_temp(6,6)=1.25+0.75*sq3;
// 		  expol_temp(6,7)=-0.25-0.25*sq3;
// 		  expol_temp(7,7)=1.25+0.75*sq3;
//
// 		  for (int k=0;k<8;++k)
// 			  for (int l=0;l<k;++l)
// 				  expol_temp(k,l)=expol_temp(l,k);
//
// 		  LINALG::FixedSizeSerialDenseSolver<8,8,1> solve_for_inverseexpol_temp;
// 		  solve_for_inverseexpol_temp.SetMatrix(expol_temp);
// 		  int err2 = solve_for_inverseexpol_temp.Factor();
// 		  int err = solve_for_inverseexpol_temp.Invert();
// 		  if ((err != 0) || (err2!=0)) dserror("Inversion of expol_temp failed");
//
// 		  for (int k=0; k<8; ++k)
// 			  for (int l=0; l<8; ++l)
// 				  expol_invert(k,l) = expol_temp(k,l);
//
// 		  //end calculation of expol_invert
//
// 		  RefCountPtr<const Epetra_Vector> phinp = discretization.GetState("phinp");
// 		  //final gradients of phi at all nodes of current element
// 		  Epetra_SerialDenseMatrix gradphi_nodescur(8,3);
//
// 		  const int* nodeids = ele->NodeIds();
// 		  //loop over all nodes of current element
// 		  for (int lnodeid=0; lnodeid < nen_; lnodeid++)
// 		  {
// 			  //get current node
// 			  DRT::Node* actnode = discretization.lRowNode(nodeids[lnodeid]);
// 			  //get all adjacent elements to this node
// 			  DRT::Element** elements=actnode->Elements();
//
// 			  //final gradient of phi at the one relevant node
// 			  Epetra_SerialDenseMatrix gradphi(3,1);
// 			  for (int j=0; j<3; j++)
// 				  gradphi(j,0) = 0.0;
//
// 			  int numadnodes = actnode->NumElement();
// 			  //loop over all adjacent elements of the current node
// 			  for (int elecur=0; elecur<numadnodes; elecur++)
// 			  {
// 				  Epetra_SerialDenseMatrix gradphi_node(3,1);
// 				  //get current element
// 				   const DRT::Element* ele_cur = elements[elecur];
//
// 				  // create vector "ephinp" holding scalar phi values for this element
// 				  Epetra_SerialDenseMatrix ephinp(nen_,1);
//
// 				  //temporal vector necessary just for function ExtractMyValues...
// 				  //that is requiring vectors and not matrices
// 				  vector<double> etemp(nen_);
//
// 				  // remark: vector "lm" is neccessary, because ExtractMyValues() only accepts "vector<int>"
// 				  // arguments, but ele->NodeIds delivers an "int*" argument
// 				  vector<int> lm(nen_);
//
// 				  // get vector of node GIDs for this element
// 				  const int* nodeids_cur = ele_cur->NodeIds();
// 				  for (int inode=0; inode < nen_; inode++)
// 					  lm[inode] = nodeids_cur[inode];
//
// 				  // get entries in "gfuncvalues" corresponding to node GIDs "lm" and store them in "ephinp"
// 				  DRT::UTILS::ExtractMyValues(*phinp, etemp, lm);
//
// 				  for (int k=0; k<nen_; k++)
// 					  ephinp(k,0) = etemp[k];
//
// 				  //current node is number iquadcur of current element
// 				  int iquadcur = 0;
// 				  for (int k=0; k<nen_; k++)
// 				  {
// 					  if (actnode->Id()==nodeids_cur[k])
// 						  iquadcur = k;
// 				  }
// 				  //local coordinates of current node
// 				  LINALG::Matrix<3,1> xsi;
// 				  LINALG::SerialDenseMatrix eleCoordMatrix=DRT::UTILS::getEleNodeNumbering_nodes_paramspace(distype);
// 				  for (int k=0; k<3; k++)
// 					  xsi(k,0)=eleCoordMatrix(k,iquadcur);
// 				  Epetra_SerialDenseMatrix deriv(3,nen_);
// 				  DRT::UTILS::shape_function_3D_deriv1(deriv,xsi(0,0),xsi(1,0),xsi(2,0),distype);
// 				  //xyze are the positions of the nodes in the global coordinate system
// 				  Epetra_SerialDenseMatrix xyze(3,nen_);
// 				  const DRT::Node*const* nodes = ele->Nodes();
// 				  for (int inode=0; inode<nen_; inode++)
// 				  {
// 					  const double* x = nodes[inode]->X();
// 					  xyze(0,inode) = x[0];
// 					  xyze(1,inode) = x[1];
// 					  xyze(2,inode) = x[2];
// 				  }
//
// 				  //get transposed of the jacobian matrix
// 				  Epetra_SerialDenseMatrix xjm(3,3);
// 				  //computing: this = 0*this+1*deriv*(xyze)T
// 				  xjm.Multiply('N','T',1.0,deriv,xyze,0.0);
// 				  //xji=xjm^-1
// 				  Epetra_SerialDenseMatrix xji(xjm);
// 				  LINALG::Matrix<3,3> xjm_temp;
// 				  for (int i1=0; i1<3; i1++)
// 					  for (int i2=0; i2<3; i2++)
// 						  xjm_temp(i1,i2) = xjm(i1,i2);
// 				  LINALG::Matrix<3,3> xji_temp;
// 				  const double det = xji_temp.Invert(xjm_temp);
// 				  if (det < 1e-16)
// 					  dserror("zero or negative jacobian determinant");
//
// 				  for (int i1=0; i1<3; i1++)
// 					  for (int i2=0; i2<3; i2++)
// 						  xji(i1,i2) = xji_temp(i1,i2);
//
// 				  Epetra_SerialDenseMatrix derxy(3,nen_);
// 				  //compute global derivatives
// 				  derxy.Multiply('N','N',1.0,xji,deriv,0.0);
//
// 				  gradphi_node.Multiply('N','N',1.0,derxy,ephinp,0.0);
//
// 				  //an average value is calculated of all adjacent elements to get second order accuracy
// 				  for (int k=0; k<3; k++)
// 					  gradphi(k,0) = gradphi(k,0)+gradphi_node(k,0);
// 				  if (elecur==(numadnodes-1))
// 				  {
// 					  for (int k=0; k<3; k++)
// 						  gradphi(k,0) = gradphi(k,0)/numadnodes;
// 				  }
//
// 			  }//end loop over all adjacent elements
//
// 			  for (int j=0; j<3; j++)
// 				  gradphi_nodescur(lnodeid,j)=gradphi(j,0);
//
// 		  }//end loop over all nodes of current element
//
// 		 gradphi_gpscur.Multiply('N','N',1.0,expol_invert,gradphi_nodescur,0.0);
//end REINHARD
//
//    for (int iquad=0; iquad<intpoints.IP().nquad; ++iquad)
//    {
//      EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());
//      //----------------------------------------------------------------------
//      // get material parameters (evaluation at integration point)
//      //----------------------------------------------------------------------
//      if (mat_gp_) GetMaterialParams(ele);
//
//      for (int k=0;k<numscal_;++k) // deal with a system of transported scalars
//      {
//        // get velocity at integration point
//        velint_.Multiply(evelnp_,funct_);
//
//        // convective part in convective form: rho*u_x*N,x+ rho*u_y*N,y
//        conv_.MultiplyTN(derxy_,velint_);
//
//        // momentum divergence required for conservative form
//        if (conservative_) GetVelocityDivergence(vdiv_,evelnp_,derxy_);
//
//        //--------------------------------------------------------------------
//        // calculation of subgrid diffusivity and stabilization parameter(s)
//        // at integration point
//        //--------------------------------------------------------------------
//        if (tau_gp_)
//        {
//          // calculation of all-scale subgrid diffusivity (artificial or due to
//          // constant-coefficient Smagorinsky model) at integration point
//          if (assgd or turbmodel)
//            CalcSubgrDiff(dt,timefac,whichassgd,assgd,turbmodel,Cs,tpn,vol,k);
//
//          // calculation of fine-scale artificial subgrid diffusivity
//          // at integration point
//          if (fssgd) CalcFineScaleSubgrDiff(ele,subgrdiff,whichfssgd,Cs,tpn,vol,k);
//
//          // generating copy of diffusivity for use in CalTau routine
//          double diffus = diffus_[k];
//
//          // calculation of stabilization parameter at integration point
//          CalTau(ele,diffus,dt,timefac,whichtau,vol,k);
//        }
//
//        // subgrid-scale velocity
//        if (sgvel_)
//        {
//          // calculate subgrid-scale velocity
//          if (scatratype != INPAR::SCATRA::scatratype_levelset)
//          {
//            CalcSubgrVelocity(ele,time,dt,timefac,k);
//          }
//          else
//          {
//            CalcSubgrVelocityLevelSet(ele,time,dt,timefac,k);
//          }
//
//          // calculate subgrid-scale convective part
//          sgconv_.MultiplyTN(derxy_,sgvelint_);
//        }
//        else
//        {
//          sgvelint_.Clear();
//          sgconv_.Clear();
//        }
//
//        // get history data (or acceleration)
//        hist_[k] = funct_.Dot(ehist_[k]);
//
//        // get bodyforce in gausspoint (divided by shcacp)
//        // (For temperature equation, time derivative of thermodynamic pressure
//        //  is added, if not constant, and for temperature equation of a reactive
//        //  equation system, a reaction-rate term is added.)
//        rhs_[k] = bodyforce_[k].Dot(funct_) / shcacp_;
//        rhs_[k] += thermpressdt_ / shcacp_;
//        rhs_[k] += reatemprhs_[k] / shcacp_;
//
//      // gradient of current scalar value
//
////REINHARD
////        for (int j=0; j<3; j++)
////          gradphi_(j,0)=gradphi_gpscur(iquad,j);
////end REINHARD
//
//        // compute matrix and rhs
//        CalMatAndRHS(sys_mat,
//                     residual,
//                     fssgd,
//                     timefac,
//                     alphaF,
//                     k);
//      } // loop over each scalar
//    }
  }
  else // 'standard' scalar transport
  {
    for (int iquad=0; iquad<intpoints.IP().nquad; ++iquad)
    {
      const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());

      //----------------------------------------------------------------------
      // get material parameters (evaluation at integration point)
      //----------------------------------------------------------------------
      if (mat_gp_) GetMaterialParams(ele,scatratype);

      for (int k=0;k<numscal_;++k) // deal with a system of transported scalars
      {
        // get velocity at integration point
        velint_.Multiply(evelnp_,funct_);

        // convective part in convective form: rho*u_x*N,x+ rho*u_y*N,y
        conv_.MultiplyTN(derxy_,velint_);

        // velocity divergence required for conservative form
        if (conservative_) GetDivergence(vdiv_,evelnp_,derxy_);

        // ensure that subgrid-scale velocity and subgrid-scale convective part
        // are zero if not computed below
        sgvelint_.Clear();
        sgconv_.Clear();

        //--------------------------------------------------------------------
        // calculation of (fine-scale) subgrid diffusivity, subgrid-scale
        // velocity and stabilization parameter(s) at integration point
        //--------------------------------------------------------------------
        if (tau_gp_)
        {
          // calculation of all-scale subgrid diffusivity (artificial or due to
          // constant-coefficient Smagorinsky model) at integration point
          if (assgd or turbmodel)
            CalcSubgrDiff(dt,timefac,whichassgd,assgd,turbmodel,Cs,tpn,vol,k);

          // calculation of fine-scale artificial subgrid diffusivity
          // at integration point
          if (fssgd) CalcFineScaleSubgrDiff(ele,subgrdiff,whichfssgd,Cs,tpn,vol,k);

          // calculation of subgrid-scale velocity at integration point if required
          if (sgvel_)
          {
            // calculation of stabilization parameter related to fluid momentum
            // equation at integration point
            CalTau(ele,visc_,dt,timefac,whichtau,vol,k,0.0,false);

            if (scatratype != INPAR::SCATRA::scatratype_levelset)
                 CalcSubgrVelocity(ele,time,dt,timefac,k);
            else CalcSubgrVelocityLevelSet(ele,time,dt,timefac,k);
            //CalcSubgrVelocityLevelSet(ele,time,dt,timefac,k,ele->Id(),iquad,intpoints, iquad);

            // calculation of subgrid-scale convective part
            sgconv_.MultiplyTN(derxy_,sgvelint_);
          }

          // calculation of stabilization parameter at integration point
          CalTau(ele,diffus_[k],dt,timefac,whichtau,vol,k,0.0,false);
        }

        // get history data (or acceleration)
        hist_[k] = funct_.Dot(ehist_[k]);

        // get bodyforce in gausspoint (divided by shcacp)
        // (For temperature equation, time derivative of thermodynamic pressure
        //  is added, if not constant, and for temperature equation of a reactive
        //  equation system, a reaction-rate term is added.)
        rhs_[k] = bodyforce_[k].Dot(funct_)/shcacp_;
        rhs_[k] += thermpressdt_/shcacp_;
        rhs_[k] += densnp_[k]*reatemprhs_[k];

        // compute matrix and rhs
        CalMatAndRHS(sys_mat,
                     residual,
                     fac,
                     fssgd,
                     timefac,
                     alphaF,
                     k);
      } // loop over each scalar
    }
  } // integration loop

  // usually, we are done here, but
  // for two certain ELCH problem formulations we have to provide
  // additional flux terms / currents across Dirichlet boundaries
  if((scatratype==INPAR::SCATRA::scatratype_elch_enc_pde_elim) or
      (scatratype==INPAR::SCATRA::scatratype_elch_enc_pde))
  {
    double val(0.0);
    const DRT::Node* const* nodes = ele->Nodes();
    string condname = "Dirichlet";

    for (int vi=0; vi<nen_; ++vi)
    {
      std::vector<DRT::Condition*> dirichcond0;
      nodes[vi]->GetCondition(condname,dirichcond0);

      // there is at least one Dirichlet condition on this node
      if (dirichcond0.size() > 0)
      {
        //cout<<"Ele Id = "<<ele->Id()<<"  Found one Dirichlet node for vi="<<vi<<endl;
        const vector<int>*    onoff = dirichcond0[0]->Get<vector<int> >   ("onoff");
        for (int k=0; k<numscal_; ++k)
        {
          if ((*onoff)[k])
          {
            //cout<<"Dirichlet is on for k="<<k<<endl;
            //cout<<"k="<<k<<"  val="<<val<<" valence_k="<<valence_[k]<<endl;
            const int fvi = vi*numdofpernode_+k;
            // We use the fact, that the rhs vector value for boundary nodes
            // is equivalent to the integrated negative normal flux
            // due to diffusion and migration
            val = residual[fvi];
            residual[vi*numdofpernode_+numscal_] += valence_[k]*(-val);
            // corresponding linearization
            for (int ui=0; ui<nen_; ++ui)
            {
              val = sys_mat(vi*numdofpernode_+k,ui*numdofpernode_+k);
              sys_mat(vi*numdofpernode_+numscal_,ui*numdofpernode_+k)+=valence_[k]*(-val);
              val = sys_mat(vi*numdofpernode_+k,ui*numdofpernode_+numscal_);
              sys_mat(vi*numdofpernode_+numscal_,ui*numdofpernode_+numscal_)+=valence_[k]*(-val);
            }
          }
        } // for k
      } // if Dirichlet at node vi
    } // for vi
  }  // elim

  return;
}


/*----------------------------------------------------------------------*
 |  get the body force  (private)                              gjb 06/08|
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::BodyForce(
    const DRT::Element*    ele,
    const double           time
)
{
  vector<DRT::Condition*> myneumcond;

  // check whether all nodes have a unique VolumeNeumann condition
  switch(nsd_)
  {
  case 3:
    DRT::UTILS::FindElementConditions(ele, "VolumeNeumann", myneumcond);
  break;
  case 2:
    DRT::UTILS::FindElementConditions(ele, "SurfaceNeumann", myneumcond);
  break;
  case 1:
    DRT::UTILS::FindElementConditions(ele, "LineNeumann", myneumcond);
  break;
  default:
    dserror("Illegal number of space dimensions: %d",nsd_);
  }

  if (myneumcond.size()>1)
    dserror("more than one VolumeNeumann cond on one node");

  if (myneumcond.size()==1)
  {
    // find out whether we will use a time curve
    const vector<int>* curve  = myneumcond[0]->Get<vector<int> >("curve");
    int curvenum = -1;

    if (curve) curvenum = (*curve)[0];

    // initialisation
    double curvefac(0.0);

    if (curvenum >= 0) // yes, we have a timecurve
    {
      // time factor for the intermediate step
      if(time >= 0.0)
      {
        curvefac = DRT::Problem::Instance()->Curve(curvenum).f(time);
      }
      else
      {
        // A negative time value indicates an error.
        dserror("Negative time value in body force calculation: time = %f",time);
      }
    }
    else // we do not have a timecurve --- timefactors are constant equal 1
    {
      curvefac = 1.0;
    }

    // get values and switches from the condition
    const vector<int>*    onoff = myneumcond[0]->Get<vector<int> >   ("onoff");
    const vector<double>* val   = myneumcond[0]->Get<vector<double> >("val"  );

    // set this condition to the bodyforce array
    for(int idof=0;idof<numdofpernode_;idof++)
    {
      for (int jnode=0; jnode<nen_; jnode++)
      {
        (bodyforce_[idof])(jnode) = (*onoff)[idof]*(*val)[idof]*curvefac;
      }
    }
  }
  else
  {
    for(int idof=0;idof<numdofpernode_;idof++)
    {
      // we have no dead load
      bodyforce_[idof].Clear();
    }
  }

  return;

} //ScaTraImpl::BodyForce


//REINHARD
/*----------------------------------------------------------------------*
 |  body force for sign function on the right hand side  (private)      |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::BodyForceReinit(
       const DRT::Element* ele,
       const double time)
{
//REINHARD

  double signum = 0.0;
  double epsilon = 0.015; //1.5*h in Sussman1994
  vector<int> onoff(numdofpernode_);
  onoff[0]=1;
  for (int k=1; k<numdofpernode_; k++)
    onoff[k]=0;

  for(int idof=0;idof<numdofpernode_;idof++)
  {
    for (int jnode=0; jnode<nen_; jnode++) //loop over all nodes of this element
    {
      if (ephinp_[idof](jnode,0)<-epsilon)
        signum = -1.0;
      else if (ephinp_[idof](jnode,0)>epsilon)
        signum = 1.0;
      else
        signum = ephinp_[idof](jnode,0)/epsilon + sin(PI*ephinp_[idof](jnode,0)/epsilon)/PI;

      // value_rhs
      (bodyforce_[idof])(jnode) = onoff[idof]*signum;
    }
  }

  return;
} //ScaTraImpl::BodyForceReinit
//end REINHARD


/*----------------------------------------------------------------------*
 |  get the material constants  (private)                      gjb 10/08|
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::GetMaterialParams(
    const DRT::Element*  ele,
    const enum INPAR::SCATRA::ScaTraType  scatratype
)
{
// get the material
RefCountPtr<MAT::Material> material = ele->Material();

// get diffusivity / diffusivities
if (material->MaterialType() == INPAR::MAT::m_matlist)
{
  const MAT::MatList* actmat = static_cast<const MAT::MatList*>(material.get());
  if (actmat->NumMat() < numscal_) dserror("Not enough materials in MatList.");

  for (int k = 0;k<numscal_;++k)
  {
    // set reaction coeff. and temperature rhs for reactive equation system to zero
    reacoeff_[k]   = 0.0;
    reatemprhs_[k] = 0.0;

    // set specific heat capacity at constant pressure to 1.0
    shcacp_ = 1.0;

    // set density at various time steps and density gradient factor to 1.0/0.0
    densn_[k]       = 1.0;
    densnp_[k]      = 1.0;
    densam_[k]      = 1.0;
    densgradfac_[k] = 0.0;

    const int matid = actmat->MatID(k);
    Teuchos::RCP<const MAT::Material> singlemat = actmat->MaterialById(matid);

    if (singlemat->MaterialType() == INPAR::MAT::m_ion)
    {
      const MAT::Ion* actsinglemat = static_cast<const MAT::Ion*>(singlemat.get());
      valence_[k] = actsinglemat->Valence();
      diffus_[k] = actsinglemat->Diffusivity();
      diffusvalence_[k] = valence_[k]*diffus_[k];

      // Material data of eliminated ion species is read from the LAST ion material
      // in the matlist!
      if ((scatratype==INPAR::SCATRA::scatratype_elch_enc_pde_elim) and (k==(numscal_-1)))
      {
        if (diffus_.size() == (unsigned) numscal_)
        {
          // For storing additional data, we increase the vector for
          // diffusivity and valences by one!
          cout<<"k = "<<k<<"   Did push back for diffus_ and valence_!"<<endl;
          diffus_.push_back(actsinglemat->ElimDiffusivity());
          valence_.push_back(actsinglemat->ElimValence());
          diffusvalence_.push_back(valence_[numscal_]*diffus_[numscal_]);
        }
        else if (diffus_.size() == (unsigned) (numscal_+1))
        {
          diffus_[numscal_]  = actsinglemat->ElimDiffusivity();
          valence_[numscal_] = actsinglemat->ElimValence();
          diffusvalence_[numscal_] = valence_[numscal_]*diffus_[numscal_];
        }
        else
          dserror("Something is wrong with eliminated ion species data");
        //if (ele->Id()==0)
        //  cout<<"data: "<<diffus_[numscal_]<<"   "<<valence_[numscal_]<<endl;
        // data check:
        if (abs(diffus_[numscal_])< EPS13) dserror("No diffusivity for eliminated species read!");
        if (abs(valence_[numscal_])< EPS13) dserror("No valence for eliminated species read!");
      }
    }
    else if (singlemat->MaterialType() == INPAR::MAT::m_arrhenius_spec)
    {
      const MAT::ArrheniusSpec* actsinglemat = static_cast<const MAT::ArrheniusSpec*>(singlemat.get());

      // compute temperature
      const double tempnp = funct_.Dot(ephinp_[numscal_-1]);

      // compute diffusivity according to Sutherland law
      diffus_[k] = actsinglemat->ComputeDiffusivity(tempnp);

      // compute reaction coefficient for species equation
      reacoeff_[k] = actsinglemat->ComputeReactionCoeff(tempnp);
      reacoeffderiv_[k] = reacoeff_[k];
      // set reaction flag to true
      reaction_ = true;
    }
    else if (singlemat->MaterialType() == INPAR::MAT::m_arrhenius_temp)
    {
      if (k != numscal_-1) dserror("Temperature equation always needs to be the last variable for reactive equation system!");

      const MAT::ArrheniusTemp* actsinglemat = static_cast<const MAT::ArrheniusTemp*>(singlemat.get());

      // get specific heat capacity at constant pressure
      shcacp_ = actsinglemat->Shc();

      // compute species mass fraction and temperature
      const double spmf   = funct_.Dot(ephinp_[0]);
      const double tempnp = funct_.Dot(ephinp_[k]);

      // compute diffusivity according to Sutherland law
      diffus_[k] = actsinglemat->ComputeDiffusivity(tempnp);

      // compute density based on temperature and thermodynamic pressure
      densnp_[k] = actsinglemat->ComputeDensity(tempnp,thermpressnp_);

      if (is_genalpha_)
      {
        // compute density at n+alpha_M
        const double tempam = funct_.Dot(ephiam_[k]);
        densam_[k] = actsinglemat->ComputeDensity(tempam,thermpressam_);

        if (not is_incremental_)
        {
          // compute density at n (thermodynamic pressure approximated at n+alpha_M)
          const double tempn = funct_.Dot(ephin_[k]);
          densn_[k] = actsinglemat->ComputeDensity(tempn,thermpressam_);
        }
        else densn_[k] = 1.0;
      }
      else densam_[k] = densnp_[k];

      // factor for density gradient
      densgradfac_[k] = -densnp_[k]/tempnp;

      // compute sum of reaction rates for temperature equation divided by specific
      // heat capacity -> will be considered a right-hand side contribution
      reatemprhs_[k] = actsinglemat->ComputeReactionRHS(spmf,tempnp)/shcacp_;

      // set reaction flag to true
      reaction_ = true;
    }
    else if (singlemat->MaterialType() == INPAR::MAT::m_scatra)
    {
      const MAT::ScatraMat* actsinglemat = static_cast<const MAT::ScatraMat*>(singlemat.get());
      diffus_[k] = actsinglemat->Diffusivity();

      // in case of reaction with constant coefficient, read coefficient and
      // set reaction flag to true
      reacoeff_[k] = actsinglemat->ReaCoeff();
      if (reacoeff_[k] > EPS14) reaction_ = true;
      if (reacoeff_[k] < -EPS14)
        dserror("Reaction coefficient for species %d is not positive: %f",k, reacoeff_[k]);
      reacoeffderiv_[k] = reacoeff_[k];
    }
    else if (singlemat->MaterialType() == INPAR::MAT::m_biofilm)
       {
         const MAT::Biofilm* actsinglemat = static_cast<const MAT::Biofilm*>(singlemat.get());
         diffus_[k] = actsinglemat->Diffusivity();
         // double rearate_k = actsinglemat->ReaRate();
         // double satcoeff_k = actsinglemat->SatCoeff();

         // set reaction flag to true
         reaction_ = true;

         // get substrate concentration at n+1 or n+alpha_F at integration point
         const double csnp = funct_.Dot(ephinp_[k]);
         //const double conp = funct_.Dot(ephinp_[1]);

         // compute reaction coefficient for species equation
         reacoeff_[k] = actsinglemat->ComputeReactionCoeff(csnp);
         reacoeffderiv_[k] = actsinglemat->ComputeReactionCoeffDeriv(csnp);
       }
    else dserror("material type not allowed");

    // check whether there is negative (physical) diffusivity
    if (diffus_[k] < -EPS15) dserror("negative (physical) diffusivity");
  }
}
else if (material->MaterialType() == INPAR::MAT::m_scatra)
{
  const MAT::ScatraMat* actmat = static_cast<const MAT::ScatraMat*>(material.get());

  dsassert(numdofpernode_==1,"more than 1 dof per node for SCATRA material");

  // get constant diffusivity
  diffus_[0] = actmat->Diffusivity();

  // in case of reaction with (non-zero) constant coefficient:
  // read coefficient and set reaction flag to true
  reacoeff_[0] = actmat->ReaCoeff();
  if (reacoeff_[0] > EPS14) reaction_ = true;
  if (reacoeff_[0] < -EPS14)
    dserror("Reaction coefficient for species %d is not positive: %f",0, reacoeff_[0]);

  reacoeffderiv_[0] = reacoeff_[0];

  // set specific heat capacity at constant pressure to 1.0
  shcacp_ = 1.0;

  // set temperature rhs for reactive equation system to zero
  reatemprhs_[0] = 0.0;

  // set density at various time steps and density gradient factor to 1.0/0.0
  densn_[0]       = 1.0;
  densnp_[0]      = 1.0;
  densam_[0]      = 1.0;
  densgradfac_[0] = 0.0;
}
else if (material->MaterialType() == INPAR::MAT::m_ion)
{
  const MAT::Ion* actsinglemat = static_cast<const MAT::Ion*>(material.get());

  dsassert(numdofpernode_==1,"more than 1 dof per node for single ion material");

  // set reaction coeff. and temperature rhs for reactive equation system to zero
  reacoeff_[0]   = 0.0;
  reatemprhs_[0] = 0.0;
  // set specific heat capacity at constant pressure to 1.0
  shcacp_ = 1.0;
  // set density at various time steps and density gradient factor to 1.0/0.0
  densn_[0]       = 1.0;
  densnp_[0]      = 1.0;
  densam_[0]      = 1.0;
  densgradfac_[0] = 0.0;

  // get constant diffusivity
  diffus_[0] = actsinglemat->Diffusivity();
  valence_[0] = 0.0; // remains unused -> we only do convection-diffusion in this case!
  diffusvalence_[0] = 0.0; // remains unused
}
else if (material->MaterialType() == INPAR::MAT::m_mixfrac)
{
  const MAT::MixFrac* actmat = static_cast<const MAT::MixFrac*>(material.get());

  dsassert(numdofpernode_==1,"more than 1 dof per node for mixture-fraction material");

  // compute mixture fraction at n+1 or n+alpha_F
  const double mixfracnp = funct_.Dot(ephinp_[0]);

  // compute dynamic diffusivity at n+1 or n+alpha_F based on mixture fraction
  diffus_[0] = actmat->ComputeDiffusivity(mixfracnp);

  // compute density at n+1 or n+alpha_F based on mixture fraction
  densnp_[0] = actmat->ComputeDensity(mixfracnp);

  // set specific heat capacity at constant pressure to 1.0
  shcacp_ = 1.0;

  if (is_genalpha_)
  {
    // compute density at n+alpha_M
    const double mixfracam = funct_.Dot(ephiam_[0]);
    densam_[0] = actmat->ComputeDensity(mixfracam);

    if (not is_incremental_)
    {
      // compute density at n
      const double mixfracn = funct_.Dot(ephin_[0]);
      densn_[0] = actmat->ComputeDensity(mixfracn);
    }
    else densn_[0] = 1.0;
  }
  else densam_[0] = densnp_[0];

  // factor for density gradient
  densgradfac_[0] = -densnp_[0]*densnp_[0]*actmat->EosFacA();

  // set reaction coeff. and temperature rhs for reactive equation system to zero
  reacoeff_[0] = 0.0;
  reacoeffderiv_[0] = 0.0;
  reatemprhs_[0] = 0.0;

  // get also fluid viscosity if subgrid-scale velocity is to be included
  if (sgvel_) visc_ = actmat->ComputeViscosity(mixfracnp);
}
else if (material->MaterialType() == INPAR::MAT::m_sutherland)
{
  const MAT::Sutherland* actmat = static_cast<const MAT::Sutherland*>(material.get());

  dsassert(numdofpernode_==1,"more than 1 dof per node for Sutherland material");

  // get specific heat capacity at constant pressure
  shcacp_ = actmat->Shc();

  // compute temperature at n+1 or n+alpha_F
  const double tempnp = funct_.Dot(ephinp_[0]);

  // compute diffusivity according to Sutherland law
  diffus_[0] = actmat->ComputeDiffusivity(tempnp);

  // compute density at n+1 or n+alpha_F based on temperature
  // and thermodynamic pressure
  densnp_[0] = actmat->ComputeDensity(tempnp,thermpressnp_);

  if (is_genalpha_)
  {
    // compute density at n+alpha_M
    const double tempam = funct_.Dot(ephiam_[0]);
    densam_[0] = actmat->ComputeDensity(tempam,thermpressam_);

    if (not is_incremental_)
    {
      // compute density at n (thermodynamic pressure approximated at n+alpha_M)
      const double tempn = funct_.Dot(ephin_[0]);
      densn_[0] = actmat->ComputeDensity(tempn,thermpressam_);
    }
    else densn_[0] = 1.0;
  }
  else densam_[0] = densnp_[0];

  // factor for density gradient
  densgradfac_[0] = -densnp_[0]/tempnp;

  // set reaction coeff. and temperature rhs for reactive equation system to zero
  reacoeff_[0] = 0.0;
  reacoeffderiv_[0] = 0.0;
  reatemprhs_[0] = 0.0;

  // get also fluid viscosity if subgrid-scale velocity is to be included
  if (sgvel_) visc_ = actmat->ComputeViscosity(tempnp);
}
else if (material->MaterialType() == INPAR::MAT::m_arrhenius_pv)
{
  const MAT::ArrheniusPV* actmat = static_cast<const MAT::ArrheniusPV*>(material.get());

  dsassert(numdofpernode_==1,"more than 1 dof per node for progress-variable material");

  // get progress variable at n+1 or n+alpha_F
  const double provarnp = funct_.Dot(ephinp_[0]);

  // get specific heat capacity at constant pressure and
  // compute temperature based on progress variable
  shcacp_ = actmat->ComputeShc(provarnp);
  const double tempnp = actmat->ComputeTemperature(provarnp);

  // compute density at n+1 or n+alpha_F
  densnp_[0] = actmat->ComputeDensity(provarnp);

  if (is_genalpha_)
  {
    // compute density at n+alpha_M
    const double provaram = funct_.Dot(ephiam_[0]);
    densam_[0] = actmat->ComputeDensity(provaram);

    if (not is_incremental_)
    {
      // compute density at n
      const double provarn = funct_.Dot(ephin_[0]);
      densn_[0] = actmat->ComputeDensity(provarn);
    }
    else densn_[0] = 1.0;
  }
  else densam_[0] = densnp_[0];

  // factor for density gradient
  densgradfac_[0] = -densnp_[0]*actmat->ComputeFactor(provarnp);

  // compute diffusivity according to Sutherland law
  diffus_[0] = actmat->ComputeDiffusivity(tempnp);

  // compute reaction coefficient for progress variable
  reacoeff_[0] = actmat->ComputeReactionCoeff(tempnp);
  reacoeffderiv_[0] = reacoeff_[0];
  // compute right-hand side contribution for progress variable
  // -> equal to reaction coefficient
  reatemprhs_[0] = reacoeff_[0];

  // set reaction flag to true
  reaction_ = true;

  // get also fluid viscosity if subgrid-scale velocity is to be included
  if (sgvel_) visc_ = actmat->ComputeViscosity(tempnp);
}
else if (material->MaterialType() == INPAR::MAT::m_ferech_pv)
{
  const MAT::FerEchPV* actmat = static_cast<const MAT::FerEchPV*>(material.get());

  dsassert(numdofpernode_==1,"more than 1 dof per node for progress-variable material");

  // get progress variable at n+1 or n+alpha_F
  const double provarnp = funct_.Dot(ephinp_[0]);

  // get specific heat capacity at constant pressure and
  // compute temperature based on progress variable
  shcacp_ = actmat->ComputeShc(provarnp);
  const double tempnp = actmat->ComputeTemperature(provarnp);

  // compute density at n+1 or n+alpha_F
  densnp_[0] = actmat->ComputeDensity(provarnp);

  if (is_genalpha_)
  {
    // compute density at n+alpha_M
    const double provaram = funct_.Dot(ephiam_[0]);
    densam_[0] = actmat->ComputeDensity(provaram);

    if (not is_incremental_)
    {
      // compute density at n
      const double provarn = funct_.Dot(ephin_[0]);
      densn_[0] = actmat->ComputeDensity(provarn);
    }
    else densn_[0] = 1.0;
  }
  else densam_[0] = densnp_[0];

  // factor for density gradient
  densgradfac_[0] = -densnp_[0]*actmat->ComputeFactor(provarnp);

  // compute diffusivity according to Sutherland law
  diffus_[0] = actmat->ComputeDiffusivity(tempnp);

  // compute reaction coefficient for progress variable
  reacoeff_[0] = actmat->ComputeReactionCoeff(provarnp);
  reacoeffderiv_[0] = reacoeff_[0];
  // compute right-hand side contribution for progress variable
  // -> equal to reaction coefficient
  reatemprhs_[0] = reacoeff_[0];

  // set reaction flag to true
  reaction_ = true;

  // get also fluid viscosity if subgrid-scale velocity is to be included
  if (sgvel_) visc_ = actmat->ComputeViscosity(tempnp);
}
else if (material->MaterialType() == INPAR::MAT::m_biofilm)
{
  dsassert(numdofpernode_==1,"more than 1 dof per node for BIOFILM material");

  const MAT::Biofilm* actmat = static_cast<const MAT::Biofilm*>(material.get());
  diffus_[0] = actmat->Diffusivity();
  // double rearate_k = actmat->ReaRate();
  // double satcoeff_k = actmat->SatCoeff();

  // set reaction flag to true
  reaction_ = true;

  // get substrate concentration at n+1 or n+alpha_F at integration point
  const double csnp = funct_.Dot(ephinp_[0]);
  //const double conp = funct_.Dot(ephinp_[1]);

  // compute reaction coefficient for species equation
  reacoeff_[0] = actmat->ComputeReactionCoeff(csnp);
  reacoeffderiv_[0] = actmat->ComputeReactionCoeffDeriv(csnp);

  // set specific heat capacity at constant pressure to 1.0
  shcacp_ = 1.0;

  // set temperature rhs for reactive equation system to zero
  reatemprhs_[0] = 0.0;

  // set density at various time steps and density gradient factor to 1.0/0.0
  densn_[0]       = 1.0;
  densnp_[0]      = 1.0;
  densam_[0]      = 1.0;
  densgradfac_[0] = 0.0;
}
else dserror("Material type is not supported");

// check whether there is negative (physical) diffusivity
if (diffus_[0] < -EPS15) dserror("negative (physical) diffusivity");

return;
} //ScaTraImpl::GetMaterialParams


/*----------------------------------------------------------------------*
 |  calculate all-scale art. subgrid diffusivity (private)     vg 10/09 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalcSubgrDiff(
    const double                          dt,
    const double                          timefac,
    const enum INPAR::SCATRA::AssgdType   whichassgd,
    const bool                            assgd,
    const bool                            turbmodel,
    const double                          Cs,
    const double                          tpn,
    const double                          vol,
    const int                             k
  )
{
  // get number of dimensions
  const double dim = (double) nsd_;

  // get characteristic element length as cubic root of element volume
  // (2D: square root of element area, 1D: element length)
  const double h = pow(vol,(1.0/dim));

  // artficial all-scale subgrid diffusivity
  if (assgd )
  {
    // classical linear artificial all-scale subgrid diffusivity
    if (whichassgd == INPAR::SCATRA::assgd_artificial)
    {
      // get element-type constant
      const double mk = SCATRA::MK<distype>();

      // velocity norm
      const double vel_norm = velint_.Norm2();

      // parameter relating convective and diffusive forces + respective switch
      const double epe = mk * densnp_[k] * vel_norm * h / diffus_[k];
      const double xi = DMAX(epe,1.0);

      // compute subgrid diffusivity
      sgdiff_[k] = (DSQR(h)*mk*DSQR(vel_norm)*DSQR(densnp_[k]))/(2.0*diffus_[k]*xi);
    }
    else
    {
      // gradient of current scalar value
      gradphi_.Multiply(derxy_,ephinp_[k]);

      // gradient norm
      const double grad_norm = gradphi_.Norm2();

      if (grad_norm > EPS10)
      {
        // initialize residual and compute values required for residual
        double residual  = 0.0;

        // get non-density-weighted history data (or acceleration)
        hist_[k] = funct_.Dot(ehist_[k]);

        // convective term using current scalar value
        double conv_phi = velint_.Dot(gradphi_);

        // diffusive term using current scalar value for higher-order elements
        double diff_phi = 0.0;
        if (use2ndderiv_) diff_phi = diff_.Dot(ephinp_[k]);

       // reactive term using current scalar value
        double rea_phi = 0.0;
        if (reaction_)
        {
          // scalar at integration point
          const double phi = funct_.Dot(ephinp_[k]);

          rea_phi = densnp_[k]*reacoeff_[k]*phi;
        }

        // get bodyforce (divided by shcacp)
        // (For temperature equation, time derivative of thermodynamic pressure
        //  is added, if not constant, and for temperature equation of a reactive
        //  equation system, a reaction-rate term is added.)
        rhs_[k] = bodyforce_[k].Dot(funct_)/shcacp_;
        rhs_[k] += thermpressdt_/shcacp_;
        rhs_[k] += densnp_[k]*reatemprhs_[k];

        // computation of residual depending on respective time-integration scheme
        if (is_genalpha_)
          residual = densam_[k]*hist_[k] + densnp_[k]*conv_phi - diff_phi + rea_phi - rhs_[k];
        else if (is_stationary_)
          residual = conv_phi - diff_phi + rea_phi - rhs_[k];
        else
        {
          // compute density-weighted scalar at integration point
          double dens_phi = funct_.Dot(ephinp_[k]);

          residual = (densnp_[k]*(dens_phi - hist_[k]) + timefac * (densnp_[k]*conv_phi - diff_phi + rea_phi - rhs_[k])) / dt;
        }

        // for the present definitions, sigma and a specific term (either
        // residual or convective term) are different
        double sigma = 0.0;
        double specific_term = 0.0;
        switch (whichassgd)
        {
          case INPAR::SCATRA::assgd_hughes:
          {
            // get norm of velocity vector b_h^par
            const double vel_norm_bhpar = abs(conv_phi/grad_norm);

            // compute stabilization parameter based on b_h^par
            // (so far, only exact formula for stationary 1-D implemented)
            // element Peclet number relating convective and diffusive forces
            double epe = 0.5 * vel_norm_bhpar * h / diffus_[k];
            const double pp = exp(epe);
            const double pm = exp(-epe);
            double xi = 0.0;
            double tau_bhpar = 0.0;
            if (epe >= 700.0) tau_bhpar = 0.5*h/vel_norm_bhpar;
            else if (epe < 700.0 and epe > EPS15)
            {
              xi = (((pp+pm)/(pp-pm))-(1.0/epe)); // xi = coth(epe) - 1/epe
              // compute optimal stabilization parameter
              tau_bhpar = 0.5*h*xi/vel_norm_bhpar;
            }

            // compute sigma
            sigma = max(0.0,tau_bhpar-tau_[k]);

            // set specific term to convective term
            specific_term = conv_phi;
          }
          break;
          case INPAR::SCATRA::assgd_tezduyar:
          {
            // velocity norm
            const double vel_norm = velint_.Norm2();

            // get norm of velocity vector b_h^par
            const double vel_norm_bhpar = abs(conv_phi/grad_norm);

            // compute stabilization parameter based on b_h^par
            // (so far, only exact formula for stationary 1-D implemented)

            // compute sigma (version 1 according to John and Knobloch (2007))
            //sigma = (h/vel_norm)*(1.0-(vel_norm_bhpar/vel_norm));

            // compute sigma (version 2 according to John and Knobloch (2007))
            // setting scaling phi_0=1.0 as in John and Knobloch (2007)
            const double phi0 = 1.0;
            sigma = (h*h*grad_norm/(vel_norm*phi0))*(1.0-(vel_norm_bhpar/vel_norm));

            // set specific term to convective term
            specific_term = conv_phi;
          }
          break;
          case INPAR::SCATRA::assgd_docarmo:
          case INPAR::SCATRA::assgd_almeida:
          {
            // velocity norm
            const double vel_norm = velint_.Norm2();

            // get norm of velocity vector z_h
            const double vel_norm_zh = abs(residual/grad_norm);

            // parameter zeta differentiating approaches by doCarmo and Galeao (1991)
            // and Almeida and Silva (1997)
            double zeta = 0.0;
            if (whichassgd == INPAR::SCATRA::assgd_docarmo)
                 zeta = 1.0;
            else zeta = max(1.0,(conv_phi/residual));

            // compute sigma
            sigma = tau_[k]*max(0.0,(vel_norm/vel_norm_zh)-zeta);

            // set specific term to residual
            specific_term = residual;
          }
          break;
          default: dserror("unknown type of all-scale subgrid diffusivity\n");
        } //switch (whichassgd)

        // computation of subgrid diffusivity
        sgdiff_[k] = sigma*residual*specific_term/(grad_norm*grad_norm);
      }
      else sgdiff_[k] = 0.0;
    }
  }
  // all-scale subgrid diffusivity due to Smagorinsky model divided by
  // turbulent Prandtl number
  else if (turbmodel)
  {
    //
    // SMAGORINSKY MODEL
    // -----------------
    //                                   +-                                 -+ 1
    //                               2   |          / h \           / h \    | -
    //    visc          = dens * lmix  * | 2 * eps | u   |   * eps | u   |   | 2
    //        turbulent           |      |          \   / ij        \   / ij |
    //                            |      +-                                 -+
    //                            |
    //                            |      |                                   |
    //                            |      +-----------------------------------+
    //                            |           'resolved' rate of strain
    //                    mixing length
    // -> either provided by dynamic modeling procedure and stored in Cs_delta_sq
    // -> or computed based on fixed Smagorinsky constant Cs:
    //             Cs = 0.17   (Lilly --- Determined from filter
    //                          analysis of Kolmogorov spectrum of
    //                          isotropic turbulence)
    //             0.1 < Cs < 0.24 (depending on the flow)
    //

    // compute (all-scale) rate of strain
    double rateofstrain = -1.0e30;
    rateofstrain = GetStrainRate(evelnp_,derxy_,vderxy_);

    // subgrid diffusivity = subgrid viscosity / turbulent Prandtl number
    sgdiff_[k] = densnp_[k] * Cs * Cs * h * h * rateofstrain / tpn;

    // add subgrid viscosity to physical viscosity for computation
    // of subgrid-scale velocity when turbulence model is applied
    if (sgvel_) visc_ += sgdiff_[k]*tpn;
  }

  // compute sum of physical and all-scale subgrid diffusivity
  // -> set internal variable for use when calculating matrix and rhs
  diffus_[k] += sgdiff_[k];

  return;
} //ScaTraImpl::CalcSubgrDiff


/*----------------------------------------------------------------------*
 |  calculate fine-scale art. subgrid diffusivity (private)    vg 10/09 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalcFineScaleSubgrDiff(
    DRT::Element*                         ele,
    Epetra_SerialDenseVector&             subgrdiff,
    const enum INPAR::SCATRA::FSSUGRDIFF  whichfssgd,
    const double                          Cs,
    const double                          tpn,
    const double                          vol,
    const int                             k
  )
{
  // get number of dimensions
  const double dim = (double) nsd_;

  // get characteristic element length as cubic root of element volume
  // (2D: square root of element area, 1D: element length)
  const double h = pow(vol,(1.0/dim));

  //----------------------------------------------------------------------
  // computation of fine-scale subgrid diffusivity for non-incremental
  // solver -> only artificial subgrid diffusivity
  // (values are stored in subgrid-diffusivity-scaling vector)
  //----------------------------------------------------------------------
  if (not is_incremental_)
  {
    // get element-type constant
    const double mk = SCATRA::MK<distype>();

    // velocity norm
    const double vel_norm = velint_.Norm2();

    // parameter relating convective and diffusive forces + respective switch
    const double epe = mk * densnp_[k] * vel_norm * h / diffus_[k];
    const double xi = DMAX(epe,1.0);

    // compute artificial subgrid diffusivity
    sgdiff_[k] = (DSQR(h)*mk*DSQR(vel_norm)*DSQR(densnp_[k]))/(2.0*diffus_[k]*xi);

    // compute entries of (fine-scale) subgrid-diffusivity-scaling vector
    for (int vi=0; vi<nen_; ++vi)
    {
      subgrdiff(vi) = sgdiff_[k]/ele->Nodes()[vi]->NumElement();
    }
  }
  //----------------------------------------------------------------------
  // computation of fine-scale subgrid diffusivity for incremental solver
  // -> only all-scale Smagorinsky model
  //----------------------------------------------------------------------
  else
  {
  //if (whichfssgd == INPAR::SCATRA::fssugrdiff_smagorinsky_all)
  //{
    //
    // ALL-SCALE SMAGORINSKY MODEL
    // ---------------------------
    //                                      +-                                 -+ 1
    //                                  2   |          / h \           / h \    | -
    //    visc          = dens * (C_S*h)  * | 2 * eps | u   |   * eps | u   |   | 2
    //        turbulent                     |          \   / ij        \   / ij |
    //                                      +-                                 -+
    //                                      |                                   |
    //                                      +-----------------------------------+
    //                                            'resolved' rate of strain
    //

    // compute (all-scale) rate of strain
    double rateofstrain = -1.0e30;
    rateofstrain = GetStrainRate(evelnp_,derxy_,vderxy_);

    // subgrid diffusivity = subgrid viscosity / turbulent Prandtl number
    sgdiff_[k] = densnp_[k] * Cs * Cs * h * h * rateofstrain / tpn;
  /*}
  else if (whichfssgd == INPAR::SCATRA::fssugrdiff_smagorinsky_small)
  {
    //
    // FINE-SCALE SMAGORINSKY MODEL
    // ----------------------------
    //                                      +-                                 -+ 1
    //                                  2   |          /    \          /   \    | -
    //    visc          = dens * (C_S*h)  * | 2 * eps | fsu |   * eps | fsu |   | 2
    //        turbulent                     |          \   / ij        \   / ij |
    //                                      +-                                 -+
    //                                      |                                   |
    //                                      +-----------------------------------+
    //                                            'resolved' rate of strain
    //

    // fine-scale rate of strain
    double fsrateofstrain = -1.0e30;
    fsrateofstrain = GetStrainRate(fsevelnp_,derxy_,fsvderxy_);

    sgdiff_[k] = densnp_[k] * Cs * Cs * h * h * fsrateofstrain;
  }*/

    // compute gradient of fine-scale part of scalar value
    fsgradphi_.Multiply(derxy_,fsphinp_[k]);
  }

  return;
} //ScaTraImpl::CalcFineScaleSubgrDiff


/*----------------------------------------------------------------------*
 |  calculate stabilization parameter  (private)              gjb 06/08 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalTau(
    DRT::Element*                         ele,
    double                                diffus,
    const double                          dt,
    const double                          timefac,
    const enum INPAR::SCATRA::TauType     whichtau,
    const double                          vol,
    const int                             k,
    const double                          frt,
    const bool                            migrationintau
  )
{
  // get element-type constant for tau
  const double mk = SCATRA::MK<distype>();
  // reset
  tauderpot_[k].Clear();

  //----------------------------------------------------------------------
  // computation of stabilization parameters depending on definition used
  //----------------------------------------------------------------------
  switch (whichtau)
  {
    case INPAR::SCATRA::tau_taylor_hughes_zarins:
    case INPAR::SCATRA::tau_taylor_hughes_zarins_wo_dt:
    {
      /*

      literature:
      1) C.A. Taylor, T.J.R. Hughes, C.K. Zarins, Finite element modeling
         of blood flow in arteries, Comput. Methods Appl. Mech. Engrg. 158
         (1998) 155-196.
      2) V. Gravemeier, W.A. Wall, An algebraic variational multiscale-
         multigrid method for large-eddy simulation of turbulent variable-
         density flow at low Mach number, J. Comput. Phys. 229 (2010)
         6047-6070.
         -> version for variable-density scalar transport equation as
            implemented here, which corresponds to constant-density
            version as given in the previous publication when density
            is constant

                                                                           1
                     +-                                               -+ - -
                     |        2                                        |   2
                     | c_1*rho                                  2      |
           tau = C * | -------   +  c_2*rho*u*G*rho*u  +  c_3*mu *G:G  |
                     |     2                                           |
                     |   dt                                            |
                     +-                                               -+

          with the constants and covariant metric tensor defined as follows:

          C   = 1.0 (not explicitly defined here),
          c_1 = 4.0,
          c_2 = 1.0 (not explicitly defined here),
          c_3 = 12.0/m_k (36.0 for linear and 144.0 for quadratic elements)

                  +-           -+   +-           -+   +-           -+
                  |             |   |             |   |             |
                  |  dr    dr   |   |  ds    ds   |   |  dt    dt   |
            G   = |  --- * ---  | + |  --- * ---  | + |  --- * ---  |
             ij   |  dx    dx   |   |  dx    dx   |   |  dx    dx   |
                  |    i     j  |   |    i     j  |   |    i     j  |
                  +-           -+   +-           -+   +-           -+

                  +----
                   \
          G : G =   +   G   * G
                   /     ij    ij
                  +----
                   i,j
                             +----
                             \
          rho*u*G*rho*u  =   +   rho*u * G  *rho*u
                             /        i   ij      j
                            +----
                              i,j
      */
      // effective velocity at element center:
      // (weighted) convective velocity + individual migration velocity
      LINALG::Matrix<nsd_,1> veleff(velint_,false);
      if (iselch_)
      {
        if (migrationintau) veleff.Update(diffusvalence_[k],migvelint_,1.0);
      }

      // total reaction coefficient sigma_tot: sum of "artificial" reaction
      // due to time factor and reaction coefficient (reaction coefficient
      // ensured to be zero in GetMaterialParams for non-reactive material)
      double sigma_tot = reacoeff_[k];
      if (whichtau == INPAR::SCATRA::tau_taylor_hughes_zarins) sigma_tot += 1.0/dt;

      // computation of various values derived from covariant metric tensor
      double G;
      double normG(0.0);
      double Gnormu(0.0);
      const double dens_sqr = densnp_[k]*densnp_[k];
      for (int nn=0;nn<nsd_;++nn)
      {
        for (int rr=0;rr<nsd_;++rr)
        {
          G = xij_(nn,0)*xij_(rr,0);
          for(int tt=1;tt<nsd_;tt++)
          {
            G += xij_(nn,tt)*xij_(rr,tt);
          }
          normG+=G*G;
          Gnormu+=dens_sqr*veleff(nn,0)*G*veleff(rr,0);
          if (iselch_) // ELCH
          {
            if (migrationintau)
            {
              // for calculation of partial derivative of tau
              for (int jj=0;jj < nen_; jj++)
                (tauderpot_[k])(jj,0) += dens_sqr*frt*diffusvalence_[k]*((derxy_(nn,jj)*G*veleff(rr,0))+(veleff(nn,0)*G*derxy_(rr,jj)));
            }
          } // ELCH
        }
      }

      // definition of constants as described above
      const double c1 = 4.0;
      const double c3 = 12.0/mk;

      // compute diffusive part
      const double Gdiff = c3*diffus*diffus*normG;

      // computation of stabilization parameter tau
      tau_[k] = 1.0/(sqrt(c1*dens_sqr*DSQR(sigma_tot) + Gnormu + Gdiff));

      // finalize derivative of present tau w.r.t electric potential
      if (iselch_)
      {
        if (migrationintau) tauderpot_[k].Scale(0.5*tau_[k]*tau_[k]*tau_[k]);
      }
    }
    break;
    case INPAR::SCATRA::tau_franca_valentin:
    {
      /*

      literature:
         L.P. Franca, F. Valentin, On an improved unusual stabilized
         finite element method for the advective-reactive-diffusive
         equation, Comput. Methods Appl. Mech. Engrg. 190 (2000) 1785-1800.


                  xi1,xi2 ^
                          |      /
                          |     /
                          |    /
                        1 +---+
                          |
                          |
                          |
                          +--------------> re1,re2
                              1

      */
      // get Euclidean norm of (weighted) velocity at element center
      double vel_norm;
      if (iselch_ and migrationintau) migrationstab_=false;
         // dserror("FrancaValentin with migrationintau not available at the moment");
        /*
          // get Euclidean norm of effective velocity at element center:
          // (weighted) convective velocity + individual migration velocity
          LINALG::Matrix<nsd_,1> veleff(velint_,false);

          veleff.Update(diffusvalence_[k],migvelint_,1.0);
          vel_norm = veleff.Norm2();

#ifdef VISUALIZE_ELEMENT_DATA
          veleff.Update(diffusvalence_[k],migvelint_,0.0);
          double vel_norm_mig = veleff.Norm2();
          double migepe2 = mk * vel_norm_mig * h / diffus;

          DRT::ELEMENTS::Transport* actele = dynamic_cast<DRT::ELEMENTS::Transport*>(ele);
          if (!actele) dserror("cast to Transport* failed");
          vector<double> v(1,migepe2);
          ostringstream temp;
          temp << k;
          string name = "Pe_mig_"+temp.str();
          actele->AddToData(name,v);
          name = "hk_"+temp.str();
          v[0] = h;
          actele->AddToData(name,v);
#endif
      }
      else*/
      vel_norm = velint_.Norm2();

      // total reaction coefficient sigma_tot: sum of "artificial" reaction
      // due to time factor and reaction coefficient (reaction coefficient
      // ensured to be zero in GetMaterialParams for non-reactive material)
      const double sigma_tot = 1.0/timefac + reacoeff_[k];

      // calculate characteristic element length
      const double h = CalcCharEleLength(vol,vel_norm);

      // various parameter computations for case without dt:
      // relating convective to viscous part
      if (diffus < EPS14) dserror("Invalid diffusion coefficent");
      const double epe = mk * densnp_[k] * vel_norm * h / diffus;
      // relating viscous to reactive part
      const double epe1 = 2.0*diffus/(mk*densnp_[k]*sigma_tot*DSQR(h));

      // respective "switching" parameters
      const double xi  = DMAX(epe,1.0);
      const double xi1 = DMAX(epe1,1.0);

      tau_[k] = DSQR(h)/(DSQR(h)*densnp_[k]*sigma_tot*xi1 + 2.0*diffus*xi/mk);

#ifdef VISUALIZE_ELEMENT_DATA
      // visualize resultant Pe number
      DRT::ELEMENTS::Transport* actele = dynamic_cast<DRT::ELEMENTS::Transport*>(ele);
      if (!actele) dserror("cast to Transport* failed");
      vector<double> v(1,epe);
      ostringstream temp;
      temp << k;
      string name = "Pe_"+temp.str();
      actele->AddToData(name,v);
#endif
    }
    break;
    case INPAR::SCATRA::tau_franca_valentin_wo_dt:
    {
      /*

      stabilization parameter as above without inclusion of dt-part

      */
      // get Euclidean norm of (weighted) velocity at element center
      double vel_norm;
      if (iselch_ and migrationintau) migrationstab_=false;
       // dserror("FrancaValentin with migrationintau not available at the moment");
        /*
          // get Euclidean norm of effective velocity at element center:
          // (weighted) convective velocity + individual migration velocity
          LINALG::Matrix<nsd_,1> veleff(velint_,false);

          veleff.Update(diffusvalence_[k],migvelint_,1.0);
          vel_norm = veleff.Norm2();

#ifdef VISUALIZE_ELEMENT_DATA
          veleff.Update(diffusvalence_[k],migvelint_,0.0);
          double vel_norm_mig = veleff.Norm2();
          double migepe2 = mk * vel_norm_mig * h / diffus;

          DRT::ELEMENTS::Transport* actele = dynamic_cast<DRT::ELEMENTS::Transport*>(ele);
          if (!actele) dserror("cast to Transport* failed");
          vector<double> v(1,migepe2);
          ostringstream temp;
          temp << k;
          string name = "Pe_mig_"+temp.str();
          actele->AddToData(name,v);
          name = "hk_"+temp.str();
          v[0] = h;
          actele->AddToData(name,v);
#endif
      }
      else*/
      vel_norm = velint_.Norm2();

      // calculate characteristic element length
      const double h = CalcCharEleLength(vol,vel_norm);

      // various parameter computations for case without dt:
      // relating convective to viscous part
      if (diffus < EPS14) dserror("Invalid diffusion coefficent");
      const double epe = mk * densnp_[k] * vel_norm * h / diffus;
      // relating viscous to reactive part
      double epe1 = 0.0;
      if (reaction_) epe1 = 2.0*diffus/(mk*densnp_[k]*reacoeff_[k]*DSQR(h));

      // respective "switching" parameters
      const double xi  = DMAX(epe,1.0);
      const double xi1 = DMAX(epe1,1.0);

      tau_[k] = DSQR(h)/(DSQR(h)*densnp_[k]*reacoeff_[k]*xi1 + 2.0*diffus*xi/mk);

#ifdef VISUALIZE_ELEMENT_DATA
      // visualize resultant Pe number
      DRT::ELEMENTS::Transport* actele = dynamic_cast<DRT::ELEMENTS::Transport*>(ele);
      if (!actele) dserror("cast to Transport* failed");
      vector<double> v(1,epe);
      ostringstream temp;
      temp << k;
      string name = "Pe_"+temp.str();
      actele->AddToData(name,v);
#endif
    }
    break;
    case INPAR::SCATRA::tau_shakib_hughes_codina:
    case INPAR::SCATRA::tau_shakib_hughes_codina_wo_dt:
    {
      /*

      literature:
      1) F. Shakib, Finite element analysis of the compressible Euler and
         Navier-Stokes equations, PhD thesis, Division of Applied Mechanics,
         Stanford University, Stanford, CA, USA, 1989.
      2) F. Shakib, T.J.R. Hughes, A new finite element formulation for
         computational fluid dynamics: IX. Fourier analysis of space-time
         Galerkin/least-squares algorithms, Comput. Methods Appl. Mech.
         Engrg. 87 (1991) 35-58.
      3) R. Codina, Stabilized finite element approximation of transient
         incompressible flows using orthogonal subscales, Comput. Methods
         Appl. Mech. Engrg. 191 (2002) 4295-4321.

         All those proposed definitions were for non-reactive incompressible
         flow; they are adapted to potentially reactive scalar transport
         equations with potential density variations here.

         constants defined as in Shakib (1989) / Shakib and Hughes (1991),
         merely slightly different with respect to c_3:

         c_1 = 4.0,
         c_2 = 4.0,
         c_3 = 4.0/(m_k*m_k) (36.0 for linear, 576.0 for quadratic ele.)

         Codina (2002) proposed present version without dt and explicit
         definition of constants
         (condition for constants as defined here: c_2 <= sqrt(c_3)).

    */
      // get Euclidean norm of velocity
      const double vel_norm = velint_.Norm2();
      if (iselch_ and migrationintau) migrationstab_=false;

      // total reaction coefficient sigma_tot: sum of "artificial" reaction
      // due to time factor and reaction coefficient (reaction coefficient
      // ensured to be zero in GetMaterialParams for non-reactive material)
      double sigma_tot = reacoeff_[k];
      if (whichtau == INPAR::SCATRA::tau_shakib_hughes_codina) sigma_tot += 1.0/dt;

      // calculate characteristic element length
      const double h = CalcCharEleLength(vol,vel_norm);

      // definition of constants as described above
      const double c1 = 4.0;
      const double c2 = 4.0;
      const double c3 = 4.0/(mk*mk);
      // alternative value as proposed in Shakib (1989): c3 = 16.0/(mk*mk);

      tau_[k] = 1.0/(sqrt(c1*DSQR(densnp_[k])*DSQR(sigma_tot)
                        + c2*DSQR(densnp_[k])*DSQR(vel_norm)/DSQR(h)
                        + c3*DSQR(diffus)/(DSQR(h)*DSQR(h))));
    }
    break;
    case INPAR::SCATRA::tau_codina:
    case INPAR::SCATRA::tau_codina_wo_dt:
    {
      /*

      literature:
         R. Codina, Comparison of some finite element methods for solving
         the diffusion-convection-reaction equation, Comput. Methods
         Appl. Mech. Engrg. 156 (1998) 185-210.

         constants:
         c_1 = 1.0,
         c_2 = 2.0,
         c_3 = 4.0/m_k (12.0 for linear, 48.0 for quadratic elements)

         Codina (1998) proposed present version without dt.

      */
      // get Euclidean norm of velocity
      const double vel_norm = velint_.Norm2();

      // total reaction coefficient sigma_tot: sum of "artificial" reaction
      // due to time factor and reaction coefficient (reaction coefficient
      // ensured to be zero in GetMaterialParams for non-reactive material)
      double sigma_tot = reacoeff_[k];
      if (whichtau == INPAR::SCATRA::tau_codina) sigma_tot += 1.0/dt;

      // calculate characteristic element length
      const double h = CalcCharEleLength(vol,vel_norm);

      // definition of constants as described above
      const double c1 = 1.0;
      const double c2 = 2.0;
      const double c3 = 4.0/mk;

      tau_[k] = 1.0/(c1*densnp_[k]*sigma_tot
                   + c2*densnp_[k]*vel_norm/h
                   + c3*diffus/(h*h));
    }
    break;
    case INPAR::SCATRA::tau_exact_1d:
    {
      // get number of dimensions (convert from int to double)
      const double dim = (double) nsd_;

      // get characteristic element length
      double h = pow(vol,(1.0/dim)); // equals streamlength in 1D

      // get Euclidean norm of (weighted) velocity at element center
      double vel_norm(0.0);

      if (iselch_ and migrationintau) // ELCH
      {
        dserror("Migration in tau not considered in Tau_Exact_1d");
      }
      else
        vel_norm = velint_.Norm2();

      if (diffus < EPS14) dserror("Invalid diffusion coefficent");
      double epe = 0.5 * densnp_[k] * vel_norm * h / diffus;

      const double pp = exp(epe);
      const double pm = exp(-epe);
      double xi = 0.0;
      if (epe >= 700.0)
        tau_[k] = 0.5*h/vel_norm;
      else if (epe < 700.0 and epe > EPS15)
      {
        xi = (((pp+pm)/(pp-pm))-(1.0/epe)); // xi = coth(epe) - 1/epe
        // compute optimal stabilization parameter
        tau_[k] = 0.5*h*xi/vel_norm;

#if 0
        cout<<"epe = "<<epe<<endl;
        cout<<"xi_opt  = "<<xi<<endl;
        cout<<"vel_norm  = "<<vel_norm<<endl;
        cout<<"tau_opt = "<<tau_[k]<<endl<<endl;
#endif
      }
      else tau_[k] = 0.0;
    }
    break;
    case INPAR::SCATRA::tau_zero:
    {
      // set tau's to zero (-> no stabilization effect)
      tau_[k] = 0.0;
    }
    break;
    default: dserror("unknown definition for stabilization parameter tau\n");
  } //switch (whichtau)

#if 0
      cout<<"diffus  for k "<<k <<" is = "<<diffus<<endl;
#endif
#ifdef VISUALIZE_ELEMENT_DATA
  // visualize stabilization parameter
  DRT::ELEMENTS::Transport* actele = dynamic_cast<DRT::ELEMENTS::Transport*>(ele);
  if (!actele) dserror("cast to Transport* failed");
  vector<double> v(1,tau_[k]);
  ostringstream temp;
  temp << k;
  string name = "tau_"+ temp.str();
  actele->AddToData(name,v);
#endif

  return;
} //ScaTraImpl::CalTau


/*----------------------------------------------------------------------*
 |  calculation of characteristic element length               vg 01/11 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
double DRT::ELEMENTS::ScaTraImpl<distype>::CalcCharEleLength(
    const double  vol,
    const double  vel_norm
    )
{
  //---------------------------------------------------------------------
  // various definitions for characteristic element length
  //---------------------------------------------------------------------
  // a) streamlength due to Tezduyar et al. (1992) -> default
  // normed velocity vector
  LINALG::Matrix<nsd_,1> velino;
  if (vel_norm>=1e-6) velino.Update(1.0/vel_norm,velint_);
  else
  {
    velino.Clear();
    velino(0,0) = 1;
  }

  // get streamlength using the normed velocity at element centre
  LINALG::Matrix<nen_,1> tmp;
  tmp.MultiplyTN(derxy_,velino);
  const double val = tmp.Norm1();
  const double hk = 2.0/val; // h=streamlength

  // b) volume-equivalent diameter (warning: 3-D formula!)
  //hk = pow((6.*vol/M_PI),(1.0/3.0))/sqrt(3.0);

  // c) cubic/square root of element volume/area or element length (3-/2-/1-D)
  // cast dimension to a double varibale -> pow()
  //const double dim = (double) nsd_;
  //hk = pow(vol,1/dim);

  return hk;
}


/*----------------------------------------------------------------------*
 |  calculate subgrid-scale velocity                           vg 10/09 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalcSubgrVelocity(
    DRT::Element*  ele,
    const double   time,
    const double   dt,
    const double   timefac,
    const int      k
  )
{
  // definitions
  LINALG::Matrix<nsd_,1> acc;
  LINALG::Matrix<nsd_,1> conv;
  LINALG::Matrix<nsd_,1> gradp;
  LINALG::Matrix<nsd_,1> visc;
  LINALG::Matrix<nsd_,1> bodyforce;
  LINALG::Matrix<nsd_,nen_> nodebodyforce;

  // get acceleration or momentum history data
  acc.Multiply(eaccnp_,funct_);

  // get velocity derivatives
  vderxy_.MultiplyNT(evelnp_,derxy_);

  // compute convective fluid term
  conv.Multiply(vderxy_,velint_);

  // get pressure gradient
  gradp.Multiply(derxy_,eprenp_);

  //--------------------------------------------------------------------
  // get nodal values of fluid body force
  //--------------------------------------------------------------------
  vector<DRT::Condition*> myfluidneumcond;

  // check whether all nodes have a unique Fluid Neumann condition
  switch(nsd_)
  {
  case 3:
    DRT::UTILS::FindElementConditions(ele, "FluidVolumeNeumann", myfluidneumcond);
  break;
  case 2:
    DRT::UTILS::FindElementConditions(ele, "FluidSurfaceNeumann", myfluidneumcond);
  break;
  case 1:
    DRT::UTILS::FindElementConditions(ele, "FluidLineNeumann", myfluidneumcond);
  break;
  default:
    dserror("Illegal number of space dimensions: %d",nsd_);
  }

  if (myfluidneumcond.size()>1)
    dserror("more than one Fluid Neumann condition on one node");

  if (myfluidneumcond.size()==1)
  {
    // find out whether we will use a time curve
    const vector<int>* curve  = myfluidneumcond[0]->Get<vector<int> >("curve");
    int curvenum = -1;

    if (curve) curvenum = (*curve)[0];

    // initialisation
    double curvefac(0.0);

    if (curvenum >= 0) // yes, we have a timecurve
    {
      // time factor for the intermediate step
      // (negative time value indicates error)
      if(time >= 0.0)
        curvefac = DRT::Problem::Instance()->Curve(curvenum).f(time);
      else
        dserror("Negative time value in body force calculation: time = %f",time);
    }
    else // we do not have a timecurve: timefactors are constant equal 1
      curvefac = 1.0;

    // get values and switches from the condition
    const vector<int>*    onoff = myfluidneumcond[0]->Get<vector<int> >   ("onoff");
    const vector<double>* val   = myfluidneumcond[0]->Get<vector<double> >("val"  );

    // set this condition to the body force array
    for(int isd=0;isd<nsd_;isd++)
    {
      for (int jnode=0; jnode<nen_; jnode++)
      {
        nodebodyforce(isd,jnode) = (*onoff)[isd]*(*val)[isd]*curvefac;
      }
    }
  }
  else nodebodyforce.Clear();

  // get fluid body force
  bodyforce.Multiply(nodebodyforce,funct_);

  // get viscous term
  if (use2ndderiv_)
  {
    /*--- viscous term: div(epsilon(u)) --------------------------------*/
    /*   /                                                \
         |  2 N_x,xx + N_x,yy + N_y,xy + N_x,zz + N_z,xz  |
       1 |                                                |
       - |  N_y,xx + N_x,yx + 2 N_y,yy + N_z,yz + N_y,zz  |
       2 |                                                |
         |  N_z,xx + N_x,zx + N_y,zy + N_z,yy + 2 N_z,zz  |
         \                                                /

         with N_x .. x-line of N
         N_y .. y-line of N                                             */

    /*--- subtraction for low-Mach-number flow: div((1/3)*(div u)*I) */
    /*   /                            \
         |  N_x,xx + N_y,yx + N_z,zx  |
       1 |                            |
    -  - |  N_x,xy + N_y,yy + N_z,zy  |
       3 |                            |
         |  N_x,xz + N_y,yz + N_z,zz  |
         \                            /

           with N_x .. x-line of N
           N_y .. y-line of N                                             */

    double prefac = 1.0/3.0;
    derxy2_.Scale(prefac);

    for (int i=0; i<nen_; ++i)
    {
      double sum = (derxy2_(0,i)+derxy2_(1,i)+derxy2_(2,i))/prefac;

      visc(0) = ((sum + derxy2_(0,i))*evelnp_(0,i) + derxy2_(3,i)*evelnp_(1,i) + derxy2_(4,i)*evelnp_(2,i))/2.0;
      visc(1) = (derxy2_(3,i)*evelnp_(0,i) + (sum + derxy2_(1,i))*evelnp_(1,i) + derxy2_(5,i)*evelnp_(2,i))/2.0;
      visc(2) = (derxy2_(4,i)*evelnp_(0,i) + derxy2_(5,i)*evelnp_(1,i) + (sum + derxy2_(2,i))*evelnp_(2,i))/2.0;
    }
  }
  else visc.Clear();

  //--------------------------------------------------------------------
  // calculation of subgrid-scale velocity based on momentum residual
  // and stabilization parameter
  // (different for generalized-alpha and other time-integration schemes)
  //--------------------------------------------------------------------
  if (is_genalpha_)
  {
    for (int rr=0;rr<nsd_;++rr)
    {
      sgvelint_(rr) = -tau_[k]*(densam_[k]*acc(rr)+densnp_[k]*conv(rr)+gradp(rr)-2*visc_*visc(rr)-densnp_[k]*bodyforce(rr));
    }
  }
  else
  {
    for (int rr=0;rr<nsd_;++rr)
    {
       sgvelint_(rr) = -tau_[k]*(densnp_[k]*velint_(rr)+timefac*(densnp_[k]*conv(rr)+gradp(rr)-2*visc_*visc(rr)-densnp_[k]*bodyforce(rr))-densn_[k]*acc(rr))/dt;
    }
  }

  return;
} //ScaTraImpl::CalcSubgrVelocity

/*-----------------------------------------------------------------------------------------------*
 |  calculate subgrid-scale velocity for level set / two-phase flow problems      rasthofer 04/10|
 *-----------------------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalcSubgrVelocityLevelSet(
    DRT::Element*  ele,
    const double   time,
    const double   dt,
    const double   timefac,
    const int      k
//    const int id,
//    const int gp,
//    const DRT::UTILS::IntPointsAndWeights<nsd_>& intpoints,  ///< integration points
//    const int                                    iquad      ///< id of current Gauss point
  )
{

  dserror("Read comment!");

  /*
   * Aufgrund der Vernachlaessigung der Anreicherung der Geschwindigkeit in den vom Level-Set geschnittenen Elementen
   * ergibt sich hier ein falsches Residuum der Impulsgleichung. Insbesondere ergeben sich somit subgrid velocities in
   * der Größenordung von u^h, die dann zu einer unphysikalischen Verformung oder sogar zu Zerstoerung des Interfaces
   * führen koennen. Folglich sollten Cross- und Reynoldsstress-Terme für Level-Set-Probleme mit XFEM im Moment nicht
   * verwendet werden.
   */

  /*
   * Hinweis:
   * trotz des Tauschs von G-Feld und Fluid sollte hier phin benoetigt werden
   * das ist aber noch nicht getestet
   */

  std::cout << "* Warning! Check parameter of fluid field! *"<< std::endl;
  // definitions
  LINALG::Matrix<nsd_,1> acc;
  LINALG::Matrix<nsd_,1> conv;
  LINALG::Matrix<nsd_,1> gradp;
  LINALG::Matrix<nsd_,1> visc;
  LINALG::Matrix<nsd_,1> bodyforce;
  LINALG::Matrix<nsd_,nen_> nodebodyforce;

  // get acceleration or momentum history data
  acc.Multiply(eaccnp_,funct_);

  // get velocity derivatives
  vderxy_.MultiplyNT(evelnp_,derxy_);

  // compute convective fluid term
  conv.Multiply(vderxy_,velint_);

  // get pressure gradient
  gradp.Multiply(derxy_,eprenp_);

  //--------------------------------------------------------------------
  // get nodal values of fluid body force
  //--------------------------------------------------------------------
  vector<DRT::Condition*> myfluidneumcond;

  // check whether all nodes have a unique Fluid Neumann condition
  switch(nsd_)
  {
  case 3:
    DRT::UTILS::FindElementConditions(ele, "FluidVolumeNeumann", myfluidneumcond);
  break;
  case 2:
    DRT::UTILS::FindElementConditions(ele, "FluidSurfaceNeumann", myfluidneumcond);
  break;
  case 1:
    DRT::UTILS::FindElementConditions(ele, "FluidLineNeumann", myfluidneumcond);
  break;
  default:
    dserror("Illegal number of space dimensions: %d",nsd_);
  }

  if (myfluidneumcond.size()>1)
    dserror("more than one Fluid Neumann condition on one node");

  if (myfluidneumcond.size()==1)
  {
    // find out whether we will use a time curve
    const vector<int>* curve  = myfluidneumcond[0]->Get<vector<int> >("curve");
    int curvenum = -1;

    if (curve) curvenum = (*curve)[0];

    // initialisation
    double curvefac(0.0);

    if (curvenum >= 0) // yes, we have a timecurve
    {
      // time factor for the intermediate step
      // (negative time value indicates error)
      if(time >= 0.0)
        curvefac = DRT::Problem::Instance()->Curve(curvenum).f(time);
      else
        dserror("Negative time value in body force calculation: time = %f",time);
    }
    else // we do not have a timecurve: timefactors are constant equal 1
      curvefac = 1.0;

    // get values and switches from the condition
    const vector<int>*    onoff = myfluidneumcond[0]->Get<vector<int> >   ("onoff");
    const vector<double>* val   = myfluidneumcond[0]->Get<vector<double> >("val"  );

    // set this condition to the body force array
    for(int isd=0;isd<nsd_;isd++)
    {
      for (int jnode=0; jnode<nen_; jnode++)
      {
        nodebodyforce(isd,jnode) = (*onoff)[isd]*(*val)[isd]*curvefac;
      }
    }
  }
  else nodebodyforce.Clear();

  // get fluid body force
  bodyforce.Multiply(nodebodyforce,funct_);

  //overwrite bodyforce
//  bodyforce(0) = 0.0;
  //bodyforce(1) = -9.81;
//  bodyforce(1) = -980.0;
//  bodyforce(2) = 0.0;

  // get viscous term
  if (use2ndderiv_)
  {
    dserror("second order elements not supported");
    /*--- viscous term: div(epsilon(u)) --------------------------------*/
    /*   /                                                \
         |  2 N_x,xx + N_x,yy + N_y,xy + N_x,zz + N_z,xz  |
       1 |                                                |
       - |  N_y,xx + N_x,yx + 2 N_y,yy + N_z,yz + N_y,zz  |
       2 |                                                |
         |  N_z,xx + N_x,zx + N_y,zy + N_z,yy + 2 N_z,zz  |
         \                                                /
//
//         with N_x .. x-line of N
//         N_y .. y-line of N                                             */
//
    /*--- subtraction for low-Mach-number flow: div((1/3)*(div u)*I) */
    /*   /                            \
         |  N_x,xx + N_y,yx + N_z,zx  |
       1 |                            |
    -  - |  N_x,xy + N_y,yy + N_z,zy  |
       3 |                            |
         |  N_x,xz + N_y,yz + N_z,zz  |
         \                            /
//
//           with N_x .. x-line of N
//           N_y .. y-line of N                                             */
//
//    double prefac = 1.0/3.0;
//    derxy2_.Scale(prefac);
//
//    for (int i=0; i<nsd_; ++i)
//    {
//      double sum = (derxy2_(0,i)+derxy2_(1,i)+derxy2_(2,i))/prefac;
//
//      visc(0) = ((sum + derxy2_(0,i))*evelnp_(0,i) + derxy2_(3,i)*evelnp_(1,i) + derxy2_(4,i)*evelnp_(2,i))/2.0;
//      visc(1) = (derxy2_(3,i)*evelnp_(0,i) + (sum + derxy2_(1,i))*evelnp_(1,i) + derxy2_(5,i)*evelnp_(2,i))/2.0;
//      visc(2) = (derxy2_(4,i)*evelnp_(0,i) + derxy2_(5,i)*evelnp_(1,i) + (sum + derxy2_(2,i))*evelnp_(2,i))/2.0;
//    }
  }
  else visc.Clear();

  double tau = 0.0;
  double dens = 0.0;
  double viscosity = 0.0;

  // theta_Scatra != theta_Fluid
  double timefacmod = (timefac / 0.5) * 0.65;
  //double timefacmod = timefac;

  // compute phi at gausspoint
  double phi = 0.0;
  for (int i = 0; i < nen_; i++)
  {
     phi = phi + funct_(i) * ephin_[k](i,0);
  }

  // set density and viscosity depending on phi
  if (phi == 0.0 or phi > 0.0)
  {
     //std::cout << "in Omega +" << std::endl;
    //dens = 1.5;
    //viscosity = 0.0033;
    //dens = 1000.0;
    //viscosity = 0.35;
    dens = 1.0;
    viscosity = 0.01;
    //dens = 3.0;
    //viscosity = 0.0135;
    //dens = 1.0;
    //viscosity = 0.1;
  }
  else
  {
    //std::cout << "in Omega -" << std::endl;
    //dens = 1.0;
    //viscosity = 0.0022;
    //dens = 1.225;
    //viscosity = 0.00358;
    dens = 1000.0;
    viscosity = 1.0;
    //dens = 1.0;
    //viscosity = 0.0045;
    //dens = 1000.0;
    //viscosity = 0.1;
  }

  // stabilization parameter definition according to Bazilevs et al. (2007)
  // (weighted) convective velocity
  LINALG::Matrix<nsd_,1> veleff(velint_,false);
  /*
                                                                          1.0
             +-                                                      -+ - ---
             |        2                                               |   2.0
             | 4.0*rho         n+1             n+1          2         |
      tau  = | -------  + rho*u     * G * rho*u     + C * mu  * G : G |
             |     2                  -                I        -   - |
             |   dt                   -                         -   - |
             +-                                                      -+

  */
  /*          +-           -+   +-           -+   +-           -+
              |             |   |             |   |             |
              |  dr    dr   |   |  ds    ds   |   |  dt    dt   |
        G   = |  --- * ---  | + |  --- * ---  | + |  --- * ---  |
         ij   |  dx    dx   |   |  dx    dx   |   |  dx    dx   |
              |    i     j  |   |    i     j  |   |    i     j  |
              +-           -+   +-           -+   +-           -+
  */
  /*          +----
               \
      G : G =   +   G   * G
      -   -    /     ij    ij
      -   -   +----
               i,j
  */
  /*                               +----
           n+1             n+1     \         n+1              n+1
      rho*u     * G * rho*u     =   +   rho*u    * G   * rho*u
                  -                /         i     -ij        j
                  -               +----        -
                                    i,j
  */
  double G;
  double normG(0.0);
  double Gnormu(0.0);
  const double dens_sqr = dens*dens;
  for (int nn=0;nn<nsd_;++nn)
  {
    for (int rr=0;rr<nsd_;++rr)
    {
      G = xij_(nn,0)*xij_(rr,0);
      for(int tt=1;tt<nsd_;tt++)
      {
        G += xij_(nn,tt)*xij_(rr,tt);
      }
      normG+=G*G;
      Gnormu+=dens_sqr*veleff(nn,0)*G*veleff(rr,0);
    }
  }

  // definition of constant:
  // 12.0/m_k = 36.0 for linear elements and 144.0 for quadratic elements
  // (differently defined, e.g., in Akkerman et al. (2008))
  // get element-type constant for tau
  const double mk = SCATRA::MK<distype>();
  const double CI = 12.0/mk;

  // stabilization parameters for stationary and instationary case, respectively
  if (is_stationary_)
    tau = 1.0/(sqrt(Gnormu+CI*viscosity*viscosity*normG));
  else
    tau = 1.0/(sqrt(dens_sqr*(4.0/(dt*dt))+Gnormu+CI*viscosity*viscosity*normG));

  //--------------------------------------------------------------------
  // calculation of subgrid-scale velocity based on momentum residual
  // and stabilization parameter
  // (different for generalized-alpha and other time-integration schemes)
  //--------------------------------------------------------------------
  if (is_genalpha_)
  {
    dserror("genalpha not supported");
//    for (int rr=0;rr<nsd_;++rr)
//    {
//      sgvelint_(rr) = -tau_[k]*(densam_[k]*acc(rr)+densnp_[k]*conv(rr)+gradp(rr)-2*visc_*visc(rr)-densnp_[k]*bodyforce(rr));
//    }
  }
  else
  {
    for (int rr=0;rr<nsd_;++rr)
    {
       sgvelint_(rr) = -tau*(dens*velint_(rr)+timefacmod*(dens*conv(rr)+gradp(rr)-2*viscosity*visc(rr)-dens*bodyforce(rr))-dens*acc(rr))/dt;

//test
//       if (id==2076)
//       {
//           std::cout << id << std::endl;
//           std::cout<< "Residuum" << sgvelint_(rr)/(-tau)*dt << std::endl;
//           std::cout<< "subgrid vel" << sgvelint_(rr) << std::endl;
//           std::cout<< "Histvektor" << acc(rr) << std::endl;
//           std::cout<< "Geschw" << velint_(rr) << std::endl;
//           std::cout<< "Druck" << gradp(rr) << std::endl;
//           std::cout<< "Konvek" << conv(rr) << std::endl;
//           std::cout<< "Epsilon" << visc(rr) << std::endl;
//           std::cout<< "Bodyforce" << bodyforce(rr) << std::endl;
//           std::cout<< "dens" << dens << std::endl;
//           std::cout<< "visc" << viscosity << std::endl;
//           std::cout<< "timefac" << timefacmod << std::endl;
//           std::cout << "tau: " << tau<< std::endl;
//       }
    }
  }

#ifdef VISUALIZE_ELEDATA_GMSH
  //Gmsh-output of element data
  {
      const bool screen_out = false;

      const std::string filename = IO::GMSH::GetFileName("SubgridVelocityScatra", 0, screen_out, 0);
      std::ofstream gmshfilecontent(filename.c_str(), ios_base::out | ios_base::app);
      {
        const double* gpcoord = (intpoints.IP().qxg)[iquad];
        if (nsd_!=3)
          dserror("change dimension");
        LINALG::Matrix<3,1> xsi;
        for (int idim=0;idim<nsd_;idim++)
          {xsi(idim) = gpcoord[idim];}
        LINALG::SerialDenseMatrix xyze(3,nen_);
        for(int inode=0;inode<nen_;inode++)
        {
          xyze(0,inode) = xyze_(0,inode);
          xyze(1,inode) = xyze_(1,inode);
          xyze(2,inode) = xyze_(2,inode);
        }
          // transform gp from local (element) coordinates to global (physical) coordinates
          GEO::elementToCurrentCoordinatesInPlace(distype, xyze, xsi);
          IO::GMSH::cellWithVectorFieldToStream(DRT::Element::point1, sgvelint_, xsi, gmshfilecontent);
      }
      gmshfilecontent.close();
  }
#endif

  return;
} //ScaTraImpl::CalcSubgrVelocityLevelSet

/*----------------------------------------------------------------------*
 | evaluate shape functions and derivatives at int. point     gjb 08/08 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
double DRT::ELEMENTS::ScaTraImpl<distype>::EvalShapeFuncAndDerivsAtIntPoint(
    const DRT::UTILS::IntPointsAndWeights<nsd_>& intpoints,  ///< integration points
    const int                                    iquad,      ///< id of current Gauss point
    const int                                    eleid       ///< the element id
)
{
  // coordinates of the current integration point
  const double* gpcoord = (intpoints.IP().qxg)[iquad];
  for (int idim=0;idim<nsd_;idim++)
    {xsi_(idim) = gpcoord[idim];}

  if (not DRT::NURBS::IsNurbs(distype))
  {
    // shape functions and their first derivatives
    DRT::UTILS::shape_function<distype>(xsi_,funct_);
    DRT::UTILS::shape_function_deriv1<distype>(xsi_,deriv_);
    if (use2ndderiv_)
    {
      // get the second derivatives of standard element at current GP
      DRT::UTILS::shape_function_deriv2<distype>(xsi_,deriv2_);
    }
  }
  else // nurbs elements are always somewhat special...
  {
    if (use2ndderiv_)
    {
      DRT::NURBS::UTILS::nurbs_get_funct_deriv_deriv2
      (funct_  ,
          deriv_  ,
          deriv2_ ,
          xsi_    ,
          myknots_,
          weights_,
          distype );
    }
    else
    {
      DRT::NURBS::UTILS::nurbs_get_funct_deriv
      (funct_  ,
          deriv_  ,
          xsi_    ,
          myknots_,
          weights_,
          distype );
    }
  } // IsNurbs()

  // compute Jacobian matrix and determinant
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

  xjm_.MultiplyNT(deriv_,xyze_);
  const double det = xij_.Invert(xjm_);

  if (det < 1E-16)
    dserror("GLOBAL ELEMENT NO.%i\nZERO OR NEGATIVE JACOBIAN DETERMINANT: %f", eleid, det);

  // set integration factor: fac = Gauss weight * det(J)
  const double fac = intpoints.IP().qwgt[iquad]*det;

  // compute global derivatives
  derxy_.Multiply(xij_,deriv_);

  // compute second global derivatives (if needed)
  if (use2ndderiv_)
  {
    // get global second derivatives
    DRT::UTILS::gder2<distype>(xjm_,derxy_,deriv2_,xyze_,derxy2_);
  }
  else
    derxy2_.Clear();

  // return integration factor for current GP: fac = Gauss weight * det(J)
  return fac;

} //ScaTraImpl::CalcSubgrVelocity


/*----------------------------------------------------------------------*
 |  evaluate element matrix and rhs (private)                   vg 02/09|
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalMatAndRHS(
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const double                          fac,
    const bool                            fssgd,
    const double                          timefac,
    const double                          alphaF,
    const int                             dofindex
    )
{
//----------------------------------------------------------------
// 1) element matrix: stationary terms
//----------------------------------------------------------------
// stabilization parameter and integration factors
const double taufac     = tau_[dofindex]*fac;
const double timefacfac = timefac*fac;
const double timetaufac = timefac*taufac;
const double fac_diffus = timefacfac*diffus_[dofindex];

//----------------------------------------------------------------
// standard Galerkin terms
//----------------------------------------------------------------
// convective term in convective form
const double densfac = timefacfac*densnp_[dofindex];
for (int vi=0; vi<nen_; ++vi)
{
  const double v = densfac*funct_(vi);
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) += v*(conv_(ui)+sgconv_(ui));
  }
}

// addition to convective term for conservative form
if (conservative_)
{
  // gradient of current scalar value
  gradphi_.Multiply(derxy_,ephinp_[dofindex]);

  // convective term using current scalar value
  const double cons_conv_phi = velint_.Dot(gradphi_);

  const double consfac = timefacfac*(densnp_[dofindex]*vdiv_+densgradfac_[dofindex]*cons_conv_phi);
  for (int vi=0; vi<nen_; ++vi)
  {
    const double v = consfac*funct_(vi);
    const int fvi = vi*numdofpernode_+dofindex;

    for (int ui=0; ui<nen_; ++ui)
    {
      const int fui = ui*numdofpernode_+dofindex;

      emat(fvi,fui) += v*funct_(ui);
    }
  }
}

// diffusive term
for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;
    double laplawf(0.0);
    GetLaplacianWeakForm(laplawf, derxy_,ui,vi);
    emat(fvi,fui) += fac_diffus*laplawf;
  }
}

//----------------------------------------------------------------
// convective stabilization term
//----------------------------------------------------------------
// convective stabilization of convective term (in convective form)
const double dens2taufac = timetaufac*densnp_[dofindex]*densnp_[dofindex];
for (int vi=0; vi<nen_; ++vi)
{
  const double v = dens2taufac*(conv_(vi)+sgconv_(vi));
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) += v*conv_(ui);
  }
}

//----------------------------------------------------------------
// stabilization terms for higher-order elements
//----------------------------------------------------------------
if (use2ndderiv_)
{
  // diffusive part:  diffus * ( N,xx  +  N,yy +  N,zz )
  GetLaplacianStrongForm(diff_, derxy2_);
  diff_.Scale(diffus_[dofindex]);

  const double denstaufac = timetaufac*densnp_[dofindex];
  // convective stabilization of diffusive term (in convective form)
  for (int vi=0; vi<nen_; ++vi)
  {
    const double v = denstaufac*(conv_(vi)+sgconv_(vi));
    const int fvi = vi*numdofpernode_+dofindex;

    for (int ui=0; ui<nen_; ++ui)
    {
      const int fui = ui*numdofpernode_+dofindex;

      emat(fvi,fui) -= v*diff_(ui);
    }
  }

  const double densdifftaufac = diffreastafac_*denstaufac;
  // diffusive stabilization of convective term (in convective form)
  for (int vi=0; vi<nen_; ++vi)
  {
    const double v = densdifftaufac*diff_(vi);
    const int fvi = vi*numdofpernode_+dofindex;

    for (int ui=0; ui<nen_; ++ui)
    {
      const int fui = ui*numdofpernode_+dofindex;

      emat(fvi,fui) -= v*conv_(ui);
    }
  }

  const double difftaufac = diffreastafac_*timetaufac;
  // diffusive stabilization of diffusive term
  for (int vi=0; vi<nen_; ++vi)
  {
    const double v = difftaufac*diff_(vi);
    const int fvi = vi*numdofpernode_+dofindex;

    for (int ui=0; ui<nen_; ++ui)
    {
      const int fui = ui*numdofpernode_+dofindex;

      emat(fvi,fui) += v*diff_(ui);
    }
  }
}

//----------------------------------------------------------------
// 2) element matrix: instationary terms
//----------------------------------------------------------------
if (not is_stationary_)
{
  const double densamfac = fac*densam_[dofindex];
  //----------------------------------------------------------------
  // standard Galerkin transient term
  //----------------------------------------------------------------
  // transient term
  for (int vi=0; vi<nen_; ++vi)
  {
    const double v = densamfac*funct_(vi);
    const int fvi = vi*numdofpernode_+dofindex;

    for (int ui=0; ui<nen_; ++ui)
    {
      const int fui = ui*numdofpernode_+dofindex;

      emat(fvi,fui) += v*funct_(ui);
    }
  }

  const double densamnptaufac = taufac*densam_[dofindex]*densnp_[dofindex];
  //----------------------------------------------------------------
  // stabilization of transient term
  //----------------------------------------------------------------
  // convective stabilization of transient term (in convective form)
  for (int vi=0; vi<nen_; ++vi)
  {
    const double v = densamnptaufac*(conv_(vi)+sgconv_(vi));
    const int fvi = vi*numdofpernode_+dofindex;

    for (int ui=0; ui<nen_; ++ui)
    {
      const int fui = ui*numdofpernode_+dofindex;

      emat(fvi,fui) += v*funct_(ui);
    }
  }

  if (use2ndderiv_)
  {
    const double densamreataufac = diffreastafac_*taufac*densam_[dofindex];
    // diffusive stabilization of transient term
    for (int vi=0; vi<nen_; ++vi)
    {
      const double v = densamreataufac*diff_(vi);
      const int fvi = vi*numdofpernode_+dofindex;

      for (int ui=0; ui<nen_; ++ui)
      {
        const int fui = ui*numdofpernode_+dofindex;

        emat(fvi,fui) -= v*funct_(ui);
      }
    }
  }
}

//----------------------------------------------------------------
// 3) element matrix: reactive terms
//----------------------------------------------------------------
if (reaction_)
{
  const double fac_reac        = timefacfac*densnp_[dofindex]*reacoeffderiv_[dofindex];
  const double timetaufac_reac = timetaufac*densnp_[dofindex]*reacoeff_[dofindex];
  //----------------------------------------------------------------
  // standard Galerkin reactive term
  //----------------------------------------------------------------
  for (int vi=0; vi<nen_; ++vi)
  {
    const double v = fac_reac*funct_(vi);
    const int fvi = vi*numdofpernode_+dofindex;

    for (int ui=0; ui<nen_; ++ui)
    {
      const int fui = ui*numdofpernode_+dofindex;

      emat(fvi,fui) += v*funct_(ui);
    }
  }

  //----------------------------------------------------------------
  // stabilization of reactive term
  //----------------------------------------------------------------
  double densreataufac = timetaufac_reac*densnp_[dofindex];
  // convective stabilization of reactive term (in convective form)
  for (int vi=0; vi<nen_; ++vi)
  {
    const double v = densreataufac*(conv_(vi)+sgconv_(vi));
    const int fvi = vi*numdofpernode_+dofindex;

    for (int ui=0; ui<nen_; ++ui)
    {
      const int fui = ui*numdofpernode_+dofindex;

      emat(fvi,fui) += v*funct_(ui);
    }
  }

  if (use2ndderiv_)
  {
    // diffusive stabilization of reactive term
    for (int vi=0; vi<nen_; ++vi)
    {
      const double v = diffreastafac_*timetaufac_reac*diff_(vi);
      const int fvi = vi*numdofpernode_+dofindex;

      for (int ui=0; ui<nen_; ++ui)
      {
        const int fui = ui*numdofpernode_+dofindex;

        emat(fvi,fui) -= v*funct_(ui);
      }
    }
  }

  //----------------------------------------------------------------
  // reactive stabilization
  //----------------------------------------------------------------
  densreataufac = diffreastafac_*timetaufac_reac*densnp_[dofindex];
  // reactive stabilization of convective (in convective form) and reactive term
  for (int vi=0; vi<nen_; ++vi)
  {
    const double v = densreataufac*funct_(vi);
    const int fvi = vi*numdofpernode_+dofindex;

    for (int ui=0; ui<nen_; ++ui)
    {
      const int fui = ui*numdofpernode_+dofindex;

      emat(fvi,fui) += v*(conv_(ui)+reacoeff_[dofindex]*funct_(ui));
    }
  }

  if (use2ndderiv_)
  {
    // reactive stabilization of diffusive term
    for (int vi=0; vi<nen_; ++vi)
    {
      const double v = diffreastafac_*timetaufac_reac*funct_(vi);
      const int fvi = vi*numdofpernode_+dofindex;

      for (int ui=0; ui<nen_; ++ui)
      {
        const int fui = ui*numdofpernode_+dofindex;

        emat(fvi,fui) -= v*diff_(ui);
      }
    }
  }
}

//----------------------------------------------------------------
// 4) element right hand side
//----------------------------------------------------------------
//----------------------------------------------------------------
// computation of bodyforce (and potentially history) term,
// residual, integration factors and standard Galerkin transient
// term (if required) on right hand side depending on respective
// (non-)incremental stationary or time-integration scheme
//----------------------------------------------------------------
double rhsint    = rhs_[dofindex];
double residual  = 0.0;
double rhsfac    = 0.0;
double rhstaufac = 0.0;
double conv_phi  = 0.0;
double diff_phi  = 0.0;
double rea_phi   = 0.0;
if (is_incremental_ and is_genalpha_)
{
  // gradient of current scalar value
  gradphi_.Multiply(derxy_,ephinp_[dofindex]);

  // convective term using current scalar value
  conv_phi = velint_.Dot(gradphi_);

  // diffusive term using current scalar value for higher-order elements
  if (use2ndderiv_) diff_phi = diff_.Dot(ephinp_[dofindex]);

  // reactive term using current scalar value
  if (reaction_)
  {
    // scalar at integration point
    const double phi = funct_.Dot(ephinp_[dofindex]);

    rea_phi = densnp_[dofindex]*reacoeff_[dofindex]*phi;
  }

  // time derivative stored on history variable
  residual  = densam_[dofindex]*hist_[dofindex] + densnp_[dofindex]*conv_phi - diff_phi + rea_phi - rhsint;
  rhsfac    = timefacfac/alphaF;
  rhstaufac = timetaufac/alphaF;
  rhsint   *= (timefac/alphaF);

  const double vtrans = rhsfac*densam_[dofindex]*hist_[dofindex];
  for (int vi=0; vi<nen_; ++vi)
  {
    const int fvi = vi*numdofpernode_+dofindex;

    erhs[fvi] -= vtrans*funct_(vi);
  }

  // addition to convective term due to subgrid-scale velocity
  // (not included in residual)
  double sgconv_phi = sgvelint_.Dot(gradphi_);
  conv_phi += sgconv_phi;

  // addition to convective term for conservative form
  // (not included in residual)
  if (conservative_)
  {
    // scalar at integration point at time step n
    const double phi = funct_.Dot(ephinp_[dofindex]);

    // convective term in conservative form
    conv_phi += phi*(vdiv_+(densgradfac_[dofindex]/densnp_[dofindex])*conv_phi);
  }

  // multiply convective term by density
  conv_phi *= densnp_[dofindex];
}
else if (not is_incremental_ and is_genalpha_)
{
  // gradient of current scalar value
  gradphi_.Multiply(derxy_,ephin_[dofindex]);

  // convective term using current scalar value
  conv_phi = velint_.Dot(gradphi_);

  // diffusive term using current scalar value for higher-order elements
  if (use2ndderiv_) diff_phi = diff_.Dot(ephin_[dofindex]);

  // reactive term using current scalar value
  if (reaction_)
  {
    // scalar at integration point
    const double phi = funct_.Dot(ephin_[dofindex]);

    rea_phi = densnp_[dofindex]*reacoeff_[dofindex]*phi;
  }

  rhsint   += densam_[dofindex]*hist_[dofindex]*(alphaF/timefac);
  residual  = (1.0-alphaF) * (densn_[dofindex]*conv_phi - diff_phi + rea_phi) - rhsint;
  rhsfac    = timefacfac*(1.0-alphaF)/alphaF;
  rhstaufac = timetaufac/alphaF;
  rhsint   *= (timefac/alphaF);

  // addition to convective term due to subgrid-scale velocity
  // (not included in residual)
  double sgconv_phi = sgvelint_.Dot(gradphi_);
  conv_phi += sgconv_phi;

  // addition to convective term for conservative form
  // (not included in residual)
  if (conservative_)
  {
    // scalar at integration point at time step n
    const double phi = funct_.Dot(ephin_[dofindex]);

    // convective term in conservative form
    // caution: velocity divergence is for n+1 and not for n!
    // -> hopefully, this inconsistency is of small amount
    conv_phi += phi*(vdiv_+(densgradfac_[dofindex]/densn_[dofindex])*conv_phi);
  }

  // multiply convective term by density
  conv_phi *= densn_[dofindex];
}
else if (is_incremental_ and not is_genalpha_)
{
  // gradient of current scalar value
  gradphi_.Multiply(derxy_,ephinp_[dofindex]);

  // convective term using current scalar value
  conv_phi = velint_.Dot(gradphi_);

  // diffusive term using current scalar value for higher-order elements
  if (use2ndderiv_) diff_phi = diff_.Dot(ephinp_[dofindex]);

  // reactive term using current scalar value
  if (reaction_)
  {
    // scalar at integration point
    const double phi = funct_.Dot(ephinp_[dofindex]);

    rea_phi = densnp_[dofindex]*reacoeff_[dofindex]*phi;
  }

  if (not is_stationary_)
  {
    // compute scalar at integration point
    double dens_phi = funct_.Dot(ephinp_[dofindex]);

    rhsint  *= timefac;
    rhsint  += densnp_[dofindex]*hist_[dofindex];
    residual = densnp_[dofindex]*dens_phi + timefac*(densnp_[dofindex]*conv_phi - diff_phi + rea_phi) - rhsint;
    rhsfac   = timefacfac;

    const double vtrans = fac*densnp_[dofindex]*dens_phi;
    for (int vi=0; vi<nen_; ++vi)
    {
      const int fvi = vi*numdofpernode_+dofindex;

      erhs[fvi] -= vtrans*funct_(vi);
    }
  }
  else
  {
    residual = densnp_[dofindex]*conv_phi - diff_phi + rea_phi - rhsint;
    rhsfac   = fac;
  }
  rhstaufac = taufac;

  // addition to convective term due to subgrid-scale velocity
  // (not included in residual)
  double sgconv_phi = sgvelint_.Dot(gradphi_);
  conv_phi += sgconv_phi;

  // addition to convective term for conservative form
  // (not included in residual)
  if (conservative_)
  {
    // scalar at integration point at time step n
    const double phi = funct_.Dot(ephinp_[dofindex]);

    // convective term in conservative form
    conv_phi += phi*(vdiv_+(densgradfac_[dofindex]/densnp_[dofindex])*conv_phi);
  }

  // multiply convective term by density
  conv_phi *= densnp_[dofindex];
}
else
{
  if (not is_stationary_)
  {
    rhsint *= timefac;
    rhsint += densnp_[dofindex]*hist_[dofindex];
  }
  residual  = -rhsint;
  rhstaufac = taufac;
}

//----------------------------------------------------------------
// standard Galerkin bodyforce term
//----------------------------------------------------------------
double vrhs = fac*rhsint;
for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] += vrhs*funct_(vi);
}

//----------------------------------------------------------------
// standard Galerkin terms on right hand side
//----------------------------------------------------------------
// convective term
vrhs = rhsfac*conv_phi;
for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= vrhs*funct_(vi);
}

// diffusive term
vrhs = rhsfac*diffus_[dofindex];
for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  double laplawf(0.0);
  GetLaplacianWeakFormRHS(laplawf,derxy_,gradphi_,vi);
  erhs[fvi] -= vrhs*laplawf;
}

//----------------------------------------------------------------
// stabilization terms
//----------------------------------------------------------------
// convective rhs stabilization (in convective form)
vrhs = rhstaufac*residual*densnp_[dofindex];
for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= vrhs*(conv_(vi)+sgconv_(vi));
}

// diffusive rhs stabilization
if (use2ndderiv_)
{
  vrhs = rhstaufac*residual;
  // diffusive stabilization of convective temporal rhs term (in convective form)
  for (int vi=0; vi<nen_; ++vi)
  {
    const int fvi = vi*numdofpernode_+dofindex;

    erhs[fvi] += diffreastafac_*vrhs*diff_(vi);
  }
}

//----------------------------------------------------------------
// reactive terms (standard Galerkin and stabilization) on rhs
//----------------------------------------------------------------
// standard Galerkin term
if (reaction_)
{
  vrhs = rhsfac*rea_phi;
  for (int vi=0; vi<nen_; ++vi)
  {
    const int fvi = vi*numdofpernode_+dofindex;

    erhs[fvi] -= vrhs*funct_(vi);
  }

  // reactive rhs stabilization
  vrhs = diffreastafac_*rhstaufac*densnp_[dofindex]*reacoeff_[dofindex]*residual;
  for (int vi=0; vi<nen_; ++vi)
  {
    const int fvi = vi*numdofpernode_+dofindex;

    erhs[fvi] -= vrhs*funct_(vi);
  }
}

//----------------------------------------------------------------
// fine-scale subgrid-diffusivity term on right hand side
//----------------------------------------------------------------
if (is_incremental_ and fssgd)
{
  vrhs = rhsfac*sgdiff_[dofindex];
  for (int vi=0; vi<nen_; ++vi)
  {
    const int fvi = vi*numdofpernode_+dofindex;

    double laplawf(0.0);
    GetLaplacianWeakFormRHS(laplawf,derxy_,fsgradphi_,vi);
    erhs[fvi] -= vrhs*laplawf;
  }
}

return;
} //ScaTraImpl::CalMatAndRHS



/*----------------------------------------------------------------------*
 | calculate mass matrix + rhs for determ. initial time deriv. gjb 08/08|
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::InitialTimeDerivative(
    DRT::Element*                         ele,
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const bool                            reinitswitch,
    const double                          frt,
    const enum INPAR::SCATRA::ScaTraType  scatratype
)
{
  // dead load in element nodes at initial point in time
  const double time = 0.0;

//REINHARD
  if (reinitswitch == false)
    BodyForce(ele,time);
  else
    BodyForceReinit(ele,time);
//end REINHARD

  //----------------------------------------------------------------------
  // get material parameters (evaluation at element center)
  //----------------------------------------------------------------------
  if (not mat_gp_)
  {
    // use one-point Gauss rule to do calculations at the element center
    DRT::UTILS::IntPointsAndWeights<nsd_> intpoints_tau(SCATRA::DisTypeToStabGaussRule<distype>::rule);

    // evaluate shape functions and derivatives at element center
    EvalShapeFuncAndDerivsAtIntPoint(intpoints_tau,0,ele->Id()); //fac unused

    GetMaterialParams(ele,scatratype);
  }

  // integrations points and weights
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints(SCATRA::DisTypeToOptGaussRule<distype>::rule);

  /*----------------------------------------------------------------------*/
  // element integration loop
  /*----------------------------------------------------------------------*/
  for (int iquad=0; iquad<intpoints.IP().nquad; ++iquad)
  {
    const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());

    //----------------------------------------------------------------------
    // get material parameters (evaluation at integration point)
    //----------------------------------------------------------------------
    if (mat_gp_) GetMaterialParams(ele,scatratype);

    //------------ get values of variables at integration point
    for (int k=0;k<numscal_;++k) // deal with a system of transported scalars
    {
      // get bodyforce in gausspoint (divided by shcacp)
      // (For temperature equation, time derivative of thermodynamic pressure
      //  is added, if not constant.)
      rhs_[k] = bodyforce_[k].Dot(funct_) / shcacp_;
      rhs_[k] += thermpressdt_/shcacp_;

      // get gradient of el. potential at integration point
      gradpot_.Multiply(derxy_,epotnp_);

      // migration part
      migconv_.MultiplyTN(-frt,derxy_,gradpot_);

      // get velocity at element center
      velint_.Multiply(evelnp_,funct_);

      // convective part in convective form: u_x*N,x+ u_y*N,y
      conv_.MultiplyTN(derxy_,velint_);

      // velocity divergence required for conservative form
      if (conservative_) GetDivergence(vdiv_,evelnp_,derxy_);

      // diffusive integration factor
      const double fac_diffus = fac*diffus_[k];

      // get value of current scalar
      conint_[k] = funct_.Dot(ephinp_[k]);

      // gradient of current scalar value
      gradphi_.Multiply(derxy_,ephinp_[k]);

      // convective part in convective form times initial scalar field
      double conv_ephi0_k = conv_.Dot(ephinp_[k]);

      // addition to convective term for conservative form
      // -> spatial variation of density not yet accounted for
      if (conservative_)
        conv_ephi0_k += conint_[k]*(vdiv_+(densgradfac_[k]/densnp_[k])*conv_ephi0_k);

      //----------------------------------------------------------------
      // element matrix: transient term
      //----------------------------------------------------------------
      // transient term
      for (int vi=0; vi<nen_; ++vi)
      {
        const double v = fac*funct_(vi)*densnp_[k];
        const int fvi = vi*numdofpernode_+k;

        for (int ui=0; ui<nen_; ++ui)
        {
          const int fui = ui*numdofpernode_+k;

          emat(fvi,fui) += v*funct_(ui);
        }
      }

      //----------------------------------------------------------------
      // element right hand side: convective term in convective form
      //----------------------------------------------------------------
      double vrhs = fac*densnp_[k]*conv_ephi0_k;
      for (int vi=0; vi<nen_; ++vi)
      {
        const int fvi = vi*numdofpernode_+k;

        erhs[fvi] -= vrhs*funct_(vi);
      }

      //----------------------------------------------------------------
      // element right hand side: diffusive term
      //----------------------------------------------------------------
      vrhs = fac_diffus;
      for (int vi=0; vi<nen_; ++vi)
      {
        const int fvi = vi*numdofpernode_+k;

        double laplawf(0.0);
        GetLaplacianWeakFormRHS(laplawf,derxy_,gradphi_,vi);
        erhs[fvi] -= vrhs*laplawf;
      }

      //----------------------------------------------------------------
      // element right hand side: nonlinear migration term
      //----------------------------------------------------------------
      vrhs = fac_diffus*conint_[k]*valence_[k];
      for (int vi=0; vi<nen_; ++vi)
      {
        const int fvi = vi*numdofpernode_+k;

        erhs[fvi] += vrhs*migconv_(vi);
      }

      //----------------------------------------------------------------
      // element right hand side: reactive term
      //----------------------------------------------------------------
      if (reaction_)
      {
        vrhs = fac*densnp_[k]*reacoeff_[k]*conint_[k];
        for (int vi=0; vi<nen_; ++vi)
        {
          const int fvi = vi*numdofpernode_+k;

          erhs[fvi] -= vrhs*funct_(vi);
        }
      }

      //----------------------------------------------------------------
      // element right hand side: bodyforce term
      //----------------------------------------------------------------
      vrhs = fac*rhs_[k];
      for (int vi=0; vi<nen_; ++vi)
      {
        const int fvi = vi*numdofpernode_+k;

        erhs[fvi] += vrhs*funct_(vi);
      }
    } // loop over each scalar k

    if (iselch_) // ELCH
    {
      // we put a dummy mass matrix here in order to have a regular
      // matrix in the lower right block of the whole system-matrix
      // A identity matrix would cause problems with ML solver in the SIMPLE
      // schemes since ML needs to have off-diagonal entries for the aggregation!
      for (int vi=0; vi<nen_; ++vi)
      {
        const double v = fac*funct_(vi); // density assumed to be 1.0 here
        const int fvi = vi*numdofpernode_+numscal_;

        for (int ui=0; ui<nen_; ++ui)
        {
          const int fui = ui*numdofpernode_+numscal_;

          emat(fvi,fui) += v*funct_(ui);
        }
      }
      // dof for el. potential have no 'velocity' -> rhs is zero!
    }

  } // integration loop

  return;
} // ScaTraImpl::InitialTimeDerivative

/*----------------------------------------------------------------------------*
 | calculate mass matrix + rhs for determ. time deriv. reinit. rasthofer 02/10|
 *----------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::TimeDerivativeReinit(
    DRT::Element*                         ele,
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const enum INPAR::SCATRA::TauType     whichtau,
    const double                          dt,
    const double                          timefac,
    const enum INPAR::SCATRA::ScaTraType  scatratype
)
{
  //----------------------------------------------------------------------
  // calculation of element volume both for tau at ele. cent. and int. pt.
  //----------------------------------------------------------------------
  // use one-point Gauss rule to do calculations at the element center
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints_tau(SCATRA::DisTypeToStabGaussRule<distype>::rule);

  // volume of the element (2D: element surface area; 1D: element length)
  // (Integration of f(x) = 1 gives exactly the volume/surface/length of element)
  const double vol = EvalShapeFuncAndDerivsAtIntPoint(intpoints_tau,0,ele->Id());

  //------------------------------------------------------------------------------------
  // get material parameters and stabilization parameters (evaluation at element center)
  //------------------------------------------------------------------------------------
  if (not mat_gp_ or not tau_gp_)
  {

    GetMaterialParams(ele,scatratype);

    if (not tau_gp_)
      {
        // get velocity at element center
        velint_.Multiply(evelnp_,funct_);

        for (int k = 0;k<numscal_;++k) // loop of each transported scalar
        {
          // calculation of stabilization parameter at element center
          CalTau(ele,diffus_[k],dt,timefac,whichtau,vol,k,0.0,false);
        }
      }
  }

  // integrations points and weights
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints(SCATRA::DisTypeToOptGaussRule<distype>::rule);

  /*----------------------------------------------------------------------*/
  // element integration loop
  /*----------------------------------------------------------------------*/
  for (int iquad=0; iquad<intpoints.IP().nquad; ++iquad)
  {
    const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());

    //----------------------------------------------------------------------
    // get material parameters (evaluation at integration point)
    //----------------------------------------------------------------------
    if (mat_gp_) GetMaterialParams(ele,scatratype);

    //------------ get values of variables at integration point
    for (int k=0;k<numscal_;++k) // deal with a system of transported scalars
    {
      // get bodyforce in gausspoint (divided by shcacp)
      // (For temperature equation, time derivative of thermodynamic pressure
      //  is added, if not constant.)
      rhs_[k] = bodyforce_[k].Dot(funct_) / shcacp_;

      // get gradient of el. potential at integration point
      gradpot_.Multiply(derxy_,epotnp_);

      // get velocity at element center
      velint_.Multiply(evelnp_,funct_);

      // convective part in convective form: u_x*N,x+ u_y*N,y
      conv_.MultiplyTN(derxy_,velint_);

      // velocity divergence required for conservative form
      if (conservative_) GetDivergence(vdiv_,evelnp_,derxy_);

      // calculation of stabilization parameter at integration point
      if (tau_gp_) CalTau(ele,diffus_[k],dt,timefac,whichtau,vol,k,0.0,false);

      const double fac_tau = fac*tau_[k];

      // diffusive integration factor
      const double fac_diffus = fac*diffus_[k];

      // get value of current scalar
      conint_[k] = funct_.Dot(ephinp_[k]);

      // gradient of current scalar value
      gradphi_.Multiply(derxy_,ephinp_[k]);

      // convective part in convective form times initial scalar field
      double conv_ephi0_k = conv_.Dot(ephinp_[k]);

      // addition to convective term for conservative form
      // -> spatial variation of density not yet accounted for
      if (conservative_)
        conv_ephi0_k += conint_[k]*(vdiv_+(densgradfac_[k]/densnp_[k])*conv_ephi0_k);

      //----------------------------------------------------------------
      // element matrix: transient term
      //----------------------------------------------------------------
      // transient term
      for (int vi=0; vi<nen_; ++vi)
      {
        const double v = fac*funct_(vi)*densnp_[k];
        const int fvi = vi*numdofpernode_+k;

        for (int ui=0; ui<nen_; ++ui)
        {
          const int fui = ui*numdofpernode_+k;

          emat(fvi,fui) += v*funct_(ui);
        }
      }

      //----------------------------------------------------------------
      // element matrix: stabilization of transient term
      //----------------------------------------------------------------
      // convective stabilization of transient term (in convective form)
      for (int vi=0; vi<nen_; ++vi)
      {
        const double v = fac_tau*conv_(vi)*densnp_[k];//v = densamnptaufac*(conv_(vi)+sgconv_(vi));
        const int fvi = vi*numdofpernode_+k;

        for (int ui=0; ui<nen_; ++ui)
        {
          const int fui = ui*numdofpernode_+k;

          emat(fvi,fui) += v*funct_(ui);
        }
      }

      //----------------------------------------------------------------
      // element right hand side: convective term in convective form
      //----------------------------------------------------------------
      double vrhs = fac*densnp_[k]*conv_ephi0_k;
      for (int vi=0; vi<nen_; ++vi)
      {
        const int fvi = vi*numdofpernode_+k;

        erhs[fvi] -= vrhs*funct_(vi);
      }

      //----------------------------------------------------------------
      // element right hand side: convective stabilization term
      //----------------------------------------------------------------
      // convective stabilization of convective term (in convective form)
      vrhs = fac_tau*densnp_[k]*conv_ephi0_k*densnp_[k];
      for (int vi=0; vi<nen_; ++vi)
      {
         const int fvi = vi*numdofpernode_+k;

         erhs[fvi] -= vrhs*conv_(vi);
      }

      if (use2ndderiv_)
      {
         dserror("TimeDerivativePhidt not yet implemented for higher order elements");
      }

      //----------------------------------------------------------------
      // element right hand side: diffusive term
      //----------------------------------------------------------------
      vrhs = fac_diffus;
      for (int vi=0; vi<nen_; ++vi)
      {
        const int fvi = vi*numdofpernode_+k;

        double laplawf(0.0);
        GetLaplacianWeakFormRHS(laplawf,derxy_,gradphi_,vi);
        erhs[fvi] -= vrhs*laplawf;
      }

      //----------------------------------------------------------------
      // element right hand side: reactive term
      //----------------------------------------------------------------
      if (reaction_)
      {
        vrhs = fac*densnp_[k]*reacoeff_[k]*conint_[k];
        for (int vi=0; vi<nen_; ++vi)
        {
          const int fvi = vi*numdofpernode_+k;

          erhs[fvi] -= vrhs*funct_(vi);
        }
      }

      //----------------------------------------------------------------
      // element right hand side: bodyforce term
      //----------------------------------------------------------------
      vrhs = fac*rhs_[k];
      for (int vi=0; vi<nen_; ++vi)
      {
        const int fvi = vi*numdofpernode_+k;

        erhs[fvi] += vrhs*funct_(vi);
      }
    } // loop over each scalar k

  } // integration loop

  return;
} // ScaTraImpl::TimeDerivativeReinit

/*----------------------------------------------------------------------*
 | calculate normalized subgrid-diffusivity matrix              vg 10/08|
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalcSubgridDiffMatrix(
    const DRT::Element*           ele,
    Epetra_SerialDenseMatrix&     sys_mat_sd,
    const double                  timefac
    )
{
/*----------------------------------------------------------------------*/
// integration loop for one element
/*----------------------------------------------------------------------*/
// integrations points and weights
DRT::UTILS::IntPointsAndWeights<nsd_> intpoints(SCATRA::DisTypeToOptGaussRule<distype>::rule);

// integration loop
for (int iquad=0; iquad<intpoints.IP().nquad; ++iquad)
{
  const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());

  for (int k=0;k<numscal_;++k)
  {
    // parameter for artificial diffusivity (scaled to one here)
    double kartfac = fac;
    if (not is_stationary_) kartfac *= timefac;

    for (int vi=0; vi<nen_; ++vi)
    {
      const int fvi = vi*numdofpernode_+k;

      for (int ui=0; ui<nen_; ++ui)
      {
        const int fui = ui*numdofpernode_+k;
        double laplawf(0.0);
        GetLaplacianWeakForm(laplawf, derxy_,ui,vi);
        sys_mat_sd(fvi,fui) += kartfac*laplawf;

        /*subtract SUPG term */
        //sys_mat_sd(fvi,fui) -= taufac*conv(vi)*conv(ui);
      }
    }
  }
} // integration loop

return;
} // ScaTraImpl::CalcSubgridDiffMatrix


/*----------------------------------------------------------------------*
 | calculate matrix and rhs for electrochemistry problem      gjb 10/08 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalMatElch(
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const double                          frt,
    const double                          timefac,
    const double                          alphaF,
    const double                          fac,
    const enum INPAR::SCATRA::ScaTraType  scatratype
)
{
  // get gradient of electric potential at integration point
  gradpot_.Multiply(derxy_,epotnp_);

  // migration term (convective part without z_k D_k): -F/RT\grad{\Phi}\grad
  migconv_.MultiplyTN(-frt,derxy_,gradpot_);

  // Laplacian of shape functions at integration point
  if (use2ndderiv_)
  {
    GetLaplacianStrongForm(laplace_, derxy2_);
  }

#if 0
  // DEBUG output
  cout<<endl<<"values at GP:"<<endl;
  cout<<"factor F/RT = "<<frt<<endl;
  for (int k=0;k<numscal_;++k)
  {cout<<"conint_["<<k<<"] = "<<conint_[k]<<endl;}
  for (int k=0;k<nsd_;++k)
  {cout<<"gradpot_["<<k<<"] = "<<gradpot_(k)<<endl;}
#endif

#ifdef SUBSCALE_ENC
  int ilcs(0);
  for (int l=0; l < numscal_; l++)
  {
    if (abs(valence_[l]) > EPS12)
      ilcs = l;
  }
#endif

  for (int k = 0; k < numscal_;++k) // loop over all transported scalars
  {
    // get value of transported scalar k at integration point
    conint_[k] = funct_.Dot(ephinp_[k]);

    // compute gradient of scalar k at integration point
    gradphi_.Multiply(derxy_,ephinp_[k]);

    // factor D_k * z_k
    const double diffus_valence_k = diffusvalence_[k];

    double diff_ephinp_k(0.0);
    double migrea_k(0.0);
    if (use2ndderiv_) // only necessary for higher order elements
    {
      diff_.Clear();
      migrea_.Clear();

      // diffusive part:  diffus_k * ( N,xx  +  N,yy +  N,zz )
      diff_.Update(diffus_[k],laplace_);

      // get Laplacian of electric potential at integration point
      double lappot = laplace_.Dot(epotnp_);
      // reactive part of migration term
      migrea_.Update(-frt*diffus_valence_k*lappot,funct_);

      diff_ephinp_k = diff_.Dot(ephinp_[k]);   // diffusion
      migrea_k      = migrea_.Dot(ephinp_[k]); // reactive part of migration term
    }
    else
    {
      diff_.Clear();
      migrea_.Clear();
    }

    // further short cuts and definitions
    const double conv_ephinp_k = conv_.Dot(ephinp_[k]);
    const double Dkzk_mig_ephinp_k = diffus_valence_k*(migconv_.Dot(ephinp_[k]));
    const double conv_eff_k = conv_ephinp_k + Dkzk_mig_ephinp_k;

    const double taufac = tau_[k]*fac;  // corresponding stabilization parameter
    double rhsint       = rhs_[k]; // source/sink terms at int. point
    double residual     = 0.0;
    double timefacfac   = 0.0;
    double timetaufac   = 0.0;
    double rhsfac       = 0.0;
    double rhstaufac    = 0.0;

    // perform time-integration specific actions
    if (is_stationary_)
    {
      // do not include any timefac for stationary calculations!
      timefacfac  = fac;
      timetaufac  = taufac;

      if (migrationinresidual_)
        residual  = conv_eff_k - diff_ephinp_k + migrea_k - rhsint;
      else
        residual  = conv_ephinp_k - diff_ephinp_k - rhsint;

      rhsfac      = fac;
      rhstaufac   = taufac;
    }
    else
    {
      timefacfac  = timefac * fac;
      timetaufac  = timefac * taufac;

      if (is_genalpha_)
      {
        // note: in hist_ we receive the time derivative phidtam at time t_{n+alpha_M} !!
        if (migrationinresidual_)
          residual  = hist_[k] + conv_eff_k - diff_ephinp_k + migrea_k - rhsint;
        else
          residual  = hist_[k] + conv_ephinp_k - diff_ephinp_k - rhsint;

        rhsfac    = timefacfac/alphaF;
        rhstaufac = timetaufac/alphaF;
        rhsint   *= (timefac/alphaF);  // not nice, but necessary !

        // rhs contribution due to incremental formulation (phidtam)
        // Standard Galerkin term
        const double vtrans = rhsfac*hist_[k];
        for (int vi=0; vi<nen_; ++vi)
        {
          const int fvi = vi*numdofpernode_+k;

          erhs[fvi] -= vtrans*funct_(vi);
        }

        // ToDo: conservative form!!!!

      }
      else
      {
        rhsint = hist_[k] + (rhs_[k]*timefac); // contributions from t_n and \theta*dt*bodyforce(t_{n+1})

        if (migrationinresidual_)
          residual  = conint_[k] + timefac*(conv_eff_k - diff_ephinp_k + migrea_k) - rhsint;
        else
          residual  = conint_[k] + timefac*(conv_ephinp_k - diff_ephinp_k) - rhsint;

        rhsfac    = timefacfac;
        rhstaufac = taufac;

        // rhs contribution due to incremental formulation (phinp)
        // Standard Galerkin term
        const double vtrans = fac*conint_[k];
        for (int vi=0; vi<nen_; ++vi)
        {
          const int fvi = vi*numdofpernode_+k;

          erhs[fvi] -= vtrans*funct_(vi);
        }


        // ToDo: conservative form!!!!

      } // if(is_genalpha_)

      //----------------------------------------------------------------
      // 1) element matrix: instationary terms
      //----------------------------------------------------------------
      for (int vi=0; vi<nen_; ++vi)
      {
        const int fvi = vi*numdofpernode_+k;
        const double fac_funct_vi = fac*funct_(vi);

        // compute effective convective stabilization operator
        double conv_eff_vi = conv_(vi);
        if (migrationstab_)
        {
          conv_eff_vi += diffus_valence_k*migconv_(vi);
        }

        for (int ui=0; ui<nen_; ++ui)
        {
          const int fui = ui*numdofpernode_+k;

          /* Standard Galerkin term: */
          emat(fvi, fui) += fac_funct_vi*funct_(ui) ;

          /* 1) convective stabilization of transient term*/
          emat(fvi, fui) += taufac*conv_eff_vi*funct_(ui);

          /* 2) diffusive stabilization */
          // not implemented. Only stabilization of SUPG type

          /* 3) reactive stabilization (reactive part of migration term) */
          // not implemented. Only stabilization of SUPG type

        } // for ui
      } // for vi

    } // if (is_stationary_)

#ifdef PRINT_ELCH_DEBUG
    cout<<"tau["<<k<<"]    = "<<tau_[k]<<endl;
    cout<<"taufac["<<k<<"] = "<<taufac<<endl;
    if (tau_[k] != 0.0)
      cout<<"residual["<<k<<"] = "<< residual<<endl;
    cout<<"conv_eff_k    = "<<conv_eff_k<<endl;
    cout<<"conv_ephinp_k  = "<<conv_ephinp_k<<endl;
    cout<<"Dkzk_mig_ephinp_k = "<<Dkzk_mig_ephinp_k<<endl;
    cout<<"diff_ephinp_k = "<<diff_ephinp_k<<endl;
    cout<<"migrea_k      = "<<migrea_k <<endl;
    cout<<endl;
#endif

    // experimental code part
    if (betterconsistency_)
    {
      dserror("Has to be re-implemented!");
      //double fdiv(0.0); // we get the negative(!) reconstructed flux from outside!
      // compute divergence of approximated diffusive and migrative fluxes
      //GetDivergence(fdiv,efluxreconstr_[k],derxy_);
      //double taufacresidual = taufac*rhsint - timetaufac*(conv_ephinp_k + fdiv);
    } // betterconsistency

    //----------------------------------------------------------------
    // 2) element matrix: stationary terms
    //----------------------------------------------------------------
    for (int vi=0; vi<nen_; ++vi)
    {
      const int    fvi = vi*numdofpernode_+k;

      // compute effective convective stabilization operator
      double conv_eff_vi = conv_(vi);
      if (migrationstab_)
      {
        conv_eff_vi += diffus_valence_k*migconv_(vi);
      }

      const double timefacfac_funct_vi = timefacfac*funct_(vi);
      const double timefacfac_diffus_valence_k_mig_vi = timefacfac*diffus_valence_k*migconv_(vi);
      const double valence_k_fac_funct_vi = valence_[k]*fac*funct_(vi);

      for (int ui=0; ui<nen_; ++ui)
      {
        const int fui = ui*numdofpernode_+k;

        //----------------------------------------------------------------
        // standard Galerkin terms
        //----------------------------------------------------------------

        // convective term
        emat(fvi, fui) += timefacfac_funct_vi*conv_(ui) ;

        // addition to convective term for conservative form
        if (conservative_)
        {
          // convective term using current scalar value
          emat(fvi,fui) += timefacfac_funct_vi*vdiv_*funct_(ui);
        }

        // diffusive term
        double laplawf(0.0);
        GetLaplacianWeakForm(laplawf, derxy_,ui,vi); // compute once, reuse below!
        emat(fvi, fui) += timefacfac*diffus_[k]*laplawf;

        // migration term
        // a) derivative w.r.t. concentration c_k
        emat(fvi, fui) -= timefacfac_diffus_valence_k_mig_vi*funct_(ui);
        // b) derivative w.r.t. electric potential
        emat(fvi,ui*numdofpernode_+numscal_) += frt*timefacfac*diffus_valence_k*conint_[k]*laplawf;

#ifndef ELCHOTHERMODELS
        // electroneutrality condition
        emat(vi*numdofpernode_+numscal_, fui) += alphaF*valence_k_fac_funct_vi*funct_(ui);

#else
        // what's the governing equation for the electric potential field?
        // we provide a lot of different options here:
        if (scatratype==INPAR::SCATRA::scatratype_elch_enc)
        {
          // electroneutrality condition (only derivative w.r.t. concentration c_k)
          emat(vi*numdofpernode_+numscal_, fui) += alphaF*valence_k_fac_funct_vi*funct_(ui);
        }
        else if (scatratype==INPAR::SCATRA::scatratype_elch_enc_pde)
        { // use 2nd order pde derived from electroneutrality condition (k=1,...,m)
          // a) derivative w.r.t. concentration c_k
          emat(vi*numdofpernode_+numscal_, fui) -= valence_[k]*(timefacfac_diffus_valence_k_mig_vi*funct_(ui));
          emat(vi*numdofpernode_+numscal_, fui) += valence_[k]*(timefacfac*diffus_[k]*laplawf);
          // b) derivative w.r.t. electric potential
          emat(vi*numdofpernode_+numscal_, ui*numdofpernode_+numscal_) += valence_[k]*(frt*timefacfac*diffus_valence_k*conint_[k]*laplawf);

          // combine with ENC for reducing "drift-off"???
          //const double beta=0.0;
          //emat(vi*numdofpernode_+numscal_, fui) += beta*alphaF*valence_k_fac_funct_vi*funct_(ui);
        }
        else if (scatratype==INPAR::SCATRA::scatratype_elch_enc_pde_elim)
        {
          // use 2nd order pde derived from electroneutrality condition (k=1,...,m-1)
          // a) derivative w.r.t. concentration c_k
          emat(vi*numdofpernode_+numscal_, fui) -= valence_[k]*(timefacfac_diffus_valence_k_mig_vi*funct_(ui));
          emat(vi*numdofpernode_+numscal_, fui) += valence_[k]*(timefacfac*diffus_[k]*laplawf);
          // b) derivative w.r.t. electric potential
          emat(vi*numdofpernode_+numscal_, ui*numdofpernode_+numscal_) += valence_[k]*(frt*timefacfac*diffus_valence_k*conint_[k]*laplawf);

          // care for eliminated species with index m
          //(diffus_ and valence_ vector were extended in GetMaterialParams()!)
          // a) derivative w.r.t. concentration c_k
          const double timefacfac_diffus_valence_m_mig_vi = timefacfac*diffus_[numscal_]*valence_[numscal_]*migconv_(vi);
          emat(vi*numdofpernode_+numscal_, fui) += valence_[k]*(timefacfac_diffus_valence_m_mig_vi*funct_(ui));
          emat(vi*numdofpernode_+numscal_, fui) -= valence_[k]*(timefacfac*diffus_[numscal_]*laplawf);
          // b) derivative w.r.t. electric potential
          emat(vi*numdofpernode_+numscal_, ui*numdofpernode_+numscal_) -= valence_[k]*(frt*timefacfac*diffus_[numscal_]*valence_[numscal_]*conint_[k]*laplawf);
        }
          else if (scatratype==INPAR::SCATRA::scatratype_elch_poisson)
          {
            const double epsilon = 1.0;
            emat(vi*numdofpernode_+numscal_,ui*numdofpernode_+numscal_) += alphaF*fac*epsilon*laplawf;
          }
          else
            dserror ("How did you reach this point?");
#endif

        //----------------------------------------------------------------
        // Stabilization terms
        //----------------------------------------------------------------

        /* 0) transient stabilization */
        // not implemented. Only stabilization of SUPG type

        /* 1) convective stabilization */
#ifdef SUBSCALE_ENC
        if (k != ilcs)
        {
#endif
          /* convective term */

          // I) linearization of residual part of stabilization term

          // effective convective stabilization of convective term
          // derivative of convective term in residual w.r.t. concentration c_k
          emat(fvi, fui) += timetaufac*conv_eff_vi*conv_(ui);

          // migration convective stabilization of convective term
          double val_ui; GetLaplacianWeakFormRHS(val_ui, derxy_,gradphi_,ui);
          if (migrationinresidual_)
          {
            // a) derivative w.r.t. concentration_k
            emat(fvi, fui) += timetaufac*conv_eff_vi*diffus_valence_k*migconv_(ui);

            // b) derivative w.r.t. electric potential
            emat(fvi,ui*numdofpernode_+numscal_) -= timetaufac*conv_eff_vi*diffus_valence_k*frt*val_ui;

            // note: higher-order and instationary parts of residuum part are linearized elsewhere!
          }

          // II) linearization of convective stabilization operator part of stabilization term
          if (migrationstab_)
          {
            // a) derivative w.r.t. concentration_k
            //    not necessary -> zero

            // b) derivative w.r.t. electric potential
            double laplacewf(0.0);
            GetLaplacianWeakForm(laplacewf, derxy_,ui,vi);
            emat(fvi,ui*numdofpernode_+numscal_) -= timetaufac*residual*diffus_valence_k*frt*laplacewf;

            // migration convective stabilization of convective term
            //emat(fvi,ui*numdofpernode_+numscal_) -= timetaufac*conv_(vi)*diffus_valence_k*frt*val_ui;
            // migration convective stabilization of migration term
            //double myval = timetaufac*diffus_valence_k*migconv_(vi);
            //emat(fvi,ui*numdofpernode_+numscal_) -= 2.0*frt*myval*diffus_valence_k*val_ui;
          }

          // III) linearization of tau part of stabilization term
          if (migrationintau_)
          {
            // derivative of tau (only effective for Taylor_Hughes_Zarins) w.r.t. electric potential
            const double tauderiv_ui = ((tauderpot_[k])(ui,0));
            emat(fvi,ui*numdofpernode_+numscal_) += timefacfac*tauderiv_ui*conv_eff_vi*residual;
          }

#ifdef SUBSCALE_ENC
          const int row = ui*numdofpernode_+ilcs;
          const double myfactor = (-valence_[k]/valence_[ilcs]);
          emat(row, ui*numdofpernode_+ilcs) += 0.0*myfactor*timetaufac*conv_eff_vi*conv_(ui);
          emat(row, ui*numdofpernode_+ilcs) += 0.0*myfactor*timetaufac*conv_eff_vi*diffus_valence_k*migconv_(ui);
          emat(row,ui*numdofpernode_+numscal_) -= 0.0*myfactor*timetaufac*conv_(vi)*diffus_valence_k*frt*val_ui;
          if (migrationintau_)
          {
            // derivative of tau (Bazilevs) with respect to electric potential
            const double conv_ephinp_k = conv_.Dot(ephinp_[k]);
            const double Dkzk_mig_ephinp_k = diffus_valence_k*(migconv_.Dot(ephinp_[k]));
            const double tauderiv_ui = ((tauderpot_[k])(ui,0));
            emat(row,ui*numdofpernode_+numscal_) += 0.0*myfactor*timefacfac*conv_eff_vi*tauderiv_ui*(conv_ephinp_k + Dkzk_mig_ephinp_k);
          }
          if (migrationstab_)
          {
            // migration convective stabilization of convective term
            emat(row,ui*numdofpernode_+numscal_) -= 0.0*myfactor*timetaufac*conv_(vi)*diffus_valence_k*frt*val_ui;
            // migration convective stabilization of migration term
            double myval = timetaufac*diffus_valence_k*migconv_(vi);
            emat(row,ui*numdofpernode_+numscal_) -= 0.0*myfactor *2.0*frt*myval*diffus_valence_k*val_ui;
          }
        } // if (k != ilcs)
#endif

      } // for ui
    } // for vi

    if (use2ndderiv_)
    {
      for (int vi=0; vi<nen_; ++vi)
      {
        const int fvi = vi*numdofpernode_+k;

        // compute effective convective stabilization operator
        double conv_eff_vi = conv_(vi);
        if (migrationstab_)
        {
          conv_eff_vi += diffus_valence_k*migconv_(vi);
        }

        for (int ui=0; ui<nen_; ++ui)
        {
          const int fui = ui*numdofpernode_+k;

          // 1) convective stabilization

          // diffusive term
          // derivative w.r.t. concentration c_k
          emat(fvi, fui) -= timetaufac*conv_eff_vi*diff_(ui) ;

          // reactive part of migration term
          if (migrationinresidual_)
          {
            // a) derivative w.r.t. concentration_k
            emat(fvi, fui) += timetaufac*conv_eff_vi*migrea_(ui) ;
            // note: migrea_ already contains frt*diffus_valence!!!

            // b) derivative w.r.t. electric potential
            emat(fvi, ui*numdofpernode_+numscal_) -= timetaufac*conv_eff_vi*conint_[k]*frt*valence_[k]*diff_(ui);
            // note: diff_ already includes factor D_k
          }

          // 2) diffusive stabilization
          // not implemented. Only stabilization of SUPG type

          // 3) reactive stabilization (reactive part of migration term)
          // not implemented. Only stabilization of SUPG type

        } // for ui
      } // for vi

    } // use2ndderiv


    //-----------------------------------------------------------------------
    // 3) element right hand side vector (neg. residual of nonlinear problem)
    //-----------------------------------------------------------------------
    for (int vi=0; vi<nen_; ++vi)
    {
      const int fvi = vi*numdofpernode_+k;

      //----------------------------------------------------------------
      // standard Galerkin terms
      //----------------------------------------------------------------

      // RHS source term (contains old part of rhs for OST / BDF2)
      erhs[fvi] += fac*funct_(vi)*rhsint ;

      // nonlinear migration term
      erhs[fvi] += rhsfac*conint_[k]*diffus_valence_k*migconv_(vi);

      // convective term
      erhs[fvi] -= rhsfac*funct_(vi)*conv_ephinp_k;

      // addition to convective term for conservative form
      // (not included in residual)
      if (conservative_)
      {
        // convective term in conservative form
        erhs[fvi] -= rhsfac*funct_(vi)*conint_[k]*vdiv_;
      }

      // diffusive term
      double laplawf(0.0);
      GetLaplacianWeakFormRHS(laplawf,derxy_,gradphi_,vi);  // compute once, reuse below!
      erhs[fvi] -= rhsfac*diffus_[k]*laplawf;

#ifndef ELCHOTHERMODELS
      // electroneutrality condition
      // for incremental formulation, there is the residuum on the rhs! : 0-sum(z_k c_k)
      erhs[vi*numdofpernode_+numscal_] -= valence_[k]*fac*funct_(vi)*conint_[k];

#else
      // what's the governing equation for the electric potential field ?
      if (scatratype==INPAR::SCATRA::scatratype_elch_enc)
      {
        // electroneutrality condition
        // for incremental formulation, there is the residuum on the rhs! : 0-sum(z_k c_k)
        erhs[vi*numdofpernode_+numscal_] -= valence_[k]*fac*funct_(vi)*conint_[k];
      }
      else if (scatratype==INPAR::SCATRA::scatratype_elch_enc_pde)
      {
        // use 2nd order pde derived from electroneutrality condition (k=1,...,m)
        erhs[vi*numdofpernode_+numscal_] += rhsfac*valence_[k]*((diffus_valence_k*conint_[k]*migconv_(vi))-(diffus_[k]*laplawf));
        //const double beta=0.0;
        //erhs[vi*numdofpernode_+numscal_] -= beta*valence_[k]*fac*funct_(vi)*conint_[k];
      }
      else if (scatratype==INPAR::SCATRA::scatratype_elch_enc_pde_elim)
      {
        // use 2nd order pde derived from electroneutrality condition (k=0,...,m-1)
        erhs[vi*numdofpernode_+numscal_] += rhsfac*valence_[k]*((diffus_valence_k*conint_[k]*migconv_(vi))-(diffus_[k]*laplawf));
        // care for eliminated species with index m
        //(diffus_ and valence_ vector were extended in GetMaterialParams()!)
        erhs[vi*numdofpernode_+numscal_] -= rhsfac*valence_[k]*((diffus_[numscal_]*valence_[numscal_]*conint_[k]*migconv_(vi))-(diffus_[numscal_]*laplawf));
      }
      else if (scatratype==INPAR::SCATRA::scatratype_elch_poisson)
      {
        const double epsilon = 1.0;
        erhs[vi*numdofpernode_+numscal_] -= fac*epsilon*laplawf;
      }
      else
        dserror ("How did you reach this point?");

#endif

      //----------------------------------------------------------------
      // Stabilization terms
      //----------------------------------------------------------------

      // 0) transient stabilization
      //    not implemented. Only stabilization of SUPG type

      // 1) convective stabilization
#ifdef SUBSCALE_ENC
      if (k != ilcs)
      {
#endif

        erhs[fvi] -= rhstaufac*conv_(vi)*residual;
        if (migrationstab_)
        {
          erhs[fvi] -=  rhstaufac*diffus_valence_k*migconv_(vi)*residual;
        }

#ifdef SUBSCALE_ENC
        const double myfactor = (-valence_[k]/valence_[ilcs]);
        // valence_[k] prevents undesired influence on neutral species automatically
        erhs[vi*numdofpernode_+ilcs] += myfactor*conv_(vi)* taufacresidual *adjust; // last scalar
        if (migrationstab_)
        {
          erhs[vi*numdofpernode_+ilcs] += myfactor*diffus_valence_k*migconv_(vi) * taufacresidual*adjust;
        }
      } // if       if (k != ilcs)
#endif

      // 2) diffusive stabilization
      //    not implemented. Only stabilization of SUPG type

      // 3) reactive stabilization (reactive part of migration term)
      //    not implemented. Only stabilization of SUPG type

    } // for vi
    // RHS vector finished


  } // loop over scalars

  return;
} // ScaTraImpl::CalMatElch


/*---------------------------------------------------------------------*
 |  calculate error compared to analytical solution           gjb 10/08|
 *---------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalErrorComparedToAnalytSolution(
    const DRT::Element*                   ele,
    const enum INPAR::SCATRA::ScaTraType  scatratype,
    ParameterList&                        params,
    Epetra_SerialDenseVector&             errors
)
{
  //at the moment, there is only one analytical test problem available!
  if (params.get<string>("action") != "calc_error")
    dserror("How did you get here?");

  // -------------- prepare common things first ! -----------------------
  // in the ALE case add nodal displacements
  if (isale_) dserror("No ALE for Kwok & Wu error calculation allowed.");

  // set constants for analytical solution
  const double t = params.get<double>("total time");
  const double frt = params.get<double>("frt");

  // get material constants
  GetMaterialParams(ele,scatratype);

  // integrations points and weights
  // more GP than usual due to (possible) cos/exp fcts in analytical solutions
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints(SCATRA::DisTypeToGaussRuleForExactSol<distype>::rule);

  const INPAR::SCATRA::CalcError errortype = DRT::INPUT::get<INPAR::SCATRA::CalcError>(params, "calcerrorflag");
  switch(errortype)
  {
  case INPAR::SCATRA::calcerror_Kwok_Wu:
  {
    //   References:
    //   Kwok, Yue-Kuen and Wu, Charles C. K.
    //   "Fractional step algorithm for solving a multi-dimensional diffusion-migration equation"
    //   Numerical Methods for Partial Differential Equations
    //   1995, Vol 11, 389-397

    //   G. Bauer, V. Gravemeier, W.A. Wall,
    //   A 3D finite element approach for the coupled numerical simulation of
    //   electrochemical systems and fluid flow, IJNME, 86 (2011) 1339–1359.

    // working arrays
    double                  potint(0.0);
    LINALG::Matrix<2,1>     conint(true);
    LINALG::Matrix<nsd_,1>  xint(true);
    LINALG::Matrix<2,1>     c(true);
    double                  deltapot(0.0);
    LINALG::Matrix<2,1>     deltacon(true);

    // start loop over integration points
    for (int iquad=0;iquad<intpoints.IP().nquad;iquad++)
    {
      const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());

      // get values of all transported scalars at integration point
      for (int k=0; k<2; ++k)
      {
        conint(k) = funct_.Dot(ephinp_[k]);
      }

      // get el. potential solution at integration point
      potint = funct_.Dot(epotnp_);

      // get global coordinate of integration point
      xint.Multiply(xyze_,funct_);

      // compute various constants
      const double d = frt*((diffus_[0]*valence_[0]) - (diffus_[1]*valence_[1]));
      if (abs(d) == 0.0) dserror("division by zero");
      const double D = frt*((valence_[0]*diffus_[0]*diffus_[1]) - (valence_[1]*diffus_[1]*diffus_[0]))/d;

      // compute analytical solution for cation and anion concentrations
      const double A0 = 2.0;
      const double m = 1.0;
      const double n = 2.0;
      const double k = 3.0;
      const double A_mnk = 1.0;
      double expterm;
      double c_0_0_0_t;

      if (nsd_==3)
      {
        expterm = exp((-D)*(m*m + n*n + k*k)*t*PI*PI);
        c(0) = A0 + (A_mnk*(cos(m*PI*xint(0))*cos(n*PI*xint(1))*cos(k*PI*xint(2)))*expterm);
        c_0_0_0_t = A0 + (A_mnk*exp((-D)*(m*m + n*n + k*k)*t*PI*PI));
      }
      else if (nsd_==2)
      {
        expterm = exp((-D)*(m*m + n*n)*t*PI*PI);
        c(0) = A0 + (A_mnk*(cos(m*PI*xint(0))*cos(n*PI*xint(1)))*expterm);
        c_0_0_0_t = A0 + (A_mnk*exp((-D)*(m*m + n*n)*t*PI*PI));
      }
      else if (nsd_==1)
      {
        expterm = exp((-D)*(m*m)*t*PI*PI);
        c(0) = A0 + (A_mnk*(cos(m*PI*xint(0)))*expterm);
        c_0_0_0_t = A0 + (A_mnk*exp((-D)*(m*m)*t*PI*PI));
      }
      else
        dserror("Illegal number of space dimensions for analyt. solution: %d",nsd_);

      // compute analytical solution for anion concentration
      c(1) = (-valence_[0]/valence_[1])* c(0);
      // compute analytical solution for el. potential
      const double pot = ((diffus_[1]-diffus_[0])/d) * log(c(0)/c_0_0_0_t);

      // compute differences between analytical solution and numerical solution
      deltapot = potint - pot;
      deltacon.Update(1.0,conint,-1.0,c);

      // add square to L2 error
      errors[0] += deltacon(0)*deltacon(0)*fac; // cation concentration
      errors[1] += deltacon(1)*deltacon(1)*fac; // anion concentration
      errors[2] += deltapot*deltapot*fac; // electric potential in electrolyte solution

    } // end of loop over integration points
  } // Kwok and Wu
  break;
  case INPAR::SCATRA::calcerror_cylinder:
  {
    // two-ion system with Butler-Volmer kinetics between two concentric cylinders
    //   G. Bauer, V. Gravemeier, W.A. Wall,
    //   A 3D finite element approach for the coupled numerical simulation of
    //   electrochemical systems and fluid flow, IJNME, 86 (2011) 1339–1359.

    // working arrays
    LINALG::Matrix<2,1>     conint(true);
    LINALG::Matrix<nsd_,1>  xint(true);
    LINALG::Matrix<2,1>     c(true);
    LINALG::Matrix<2,1>     deltacon(true);

    // some constants that are needed
    const double c0_inner = 0.6147737641011396;
    const double c0_outer = 1.244249192148809;
    const double r_inner = 1.0;
    const double r_outer = 2.0;
    const double pot_inner = 2.758240847314454;
    const double b = log(r_outer/r_inner);

    // start loop over integration points
    for (int iquad=0;iquad<intpoints.IP().nquad;iquad++)
    {
      const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());

      // get values of all transported scalars at integration point
      for (int k=0; k<2; ++k)
      {
        conint(k) = funct_.Dot(ephinp_[k]);
      }

      // get el. potential solution at integration point
      const double potint = funct_.Dot(epotnp_);

      // get global coordinate of integration point
      xint.Multiply(xyze_,funct_);

      // evaluate analytical solution for cation concentration at radial position r
      if (nsd_==3)
      {
        const double r = sqrt(xint(0)*xint(0) + xint(1)*xint(1));
        c(0) = c0_inner + ((c0_outer- c0_inner)*(log(r) - log(r_inner))/b);
      }
      else
        dserror("Illegal number of space dimensions for analyt. solution: %d",nsd_);

      // compute analytical solution for anion concentration
      c(1) = (-valence_[0]/valence_[1])* c(0);
      // compute analytical solution for el. potential
      const double d = frt*((diffus_[0]*valence_[0]) - (diffus_[1]*valence_[1]));
      if (abs(d) == 0.0) dserror("division by zero");
      // reference value + ohmic resistance + concentration potential
      const double pot = pot_inner + log(c(0)/c0_inner); // + (((diffus_[1]-diffus_[0])/d) * log(c(0)/c0_inner));

      // compute differences between analytical solution and numerical solution
      double deltapot = potint - pot;
      deltacon.Update(1.0,conint,-1.0,c);

      // add square to L2 error
      errors[0] += deltacon(0)*deltacon(0)*fac; // cation concentration
      errors[1] += deltacon(1)*deltacon(1)*fac; // anion concentration
      errors[2] += deltapot*deltapot*fac; // electric potential in electrolyte solution

    } // end of loop over integration points
  } // concentric cylinders
  break;
  default: dserror("Unknown analytical solution!");
  } //switch(errortype)

  return;
} // ScaTraImpl::CalErrorComparedToAnalytSolution


  /*----------------------------------------------------------------------*
   |  calculate weighted mass flux (no reactive flux so far)     gjb 06/08|
   *----------------------------------------------------------------------*/
  template <DRT::Element::DiscretizationType distype>
  void DRT::ELEMENTS::ScaTraImpl<distype>::CalculateFlux(
      LINALG::Matrix<3,nen_>&         flux,
      const DRT::Element*             ele,
      const double                    frt,
      const INPAR::SCATRA::FluxType   fluxtype,
      const int                       dofindex,
      const enum INPAR::SCATRA::ScaTraType  scatratype
  )
  {
/*
 * Actually, we compute here a weighted (and integrated) form of the fluxes!
 * On time integration level, these contributions are then used to calculate
 * an L2-projected representation of fluxes.
 * Thus, this method here DOES NOT YET provide flux values that are ready to use!!
    /                                                         \
   |                /   \                               /   \  |
   | w, -D * nabla | phi | + u*phi - frt*z_k*c_k*nabla | pot | |
   |                \   /                               \   /  |
    \                      [optional]      [optional]         /
*/

    // get material parameters (evaluation at element center)
    if (not mat_gp_) GetMaterialParams(ele,scatratype);

    // integration rule
    DRT::UTILS::IntPointsAndWeights<nsd_> intpoints(SCATRA::DisTypeToOptGaussRule<distype>::rule);

    // integration loop
    for (int iquad=0; iquad< intpoints.IP().nquad; ++iquad)
    {
      // evaluate shape functions and derivatives at integration point
      const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());

      // get material parameters (evaluation at integration point)
      if (mat_gp_) GetMaterialParams(ele,scatratype);

      // get velocity at integration point
      velint_.Multiply(evelnp_,funct_);

      // get scalar at integration point
      const double phi = funct_.Dot(ephinp_[dofindex]);

      // get gradient of scalar at integration point
      gradphi_.Multiply(derxy_,ephinp_[dofindex]);

      // get gradient of electric potential at integration point if required
      if (frt > 0.0) gradpot_.Multiply(derxy_,epotnp_);

      // allocate and initialize!
      LINALG::Matrix<nsd_,1> q(true);

      // add different flux contributions as specified by user input
      switch (fluxtype)
      {
      case INPAR::SCATRA::flux_total_domain:

        // convective flux contribution
        q.Update(densnp_[dofindex]*phi,velint_);

        // no break statement here!
      case INPAR::SCATRA::flux_diffusive_domain:
        // diffusive flux contribution
        q.Update(-diffus_[dofindex],gradphi_,1.0);

        // ELCH (migration flux contribution)
        if (frt > 0.0) q.Update(-diffusvalence_[dofindex]*frt*phi,gradpot_,1.0);

        break;
      default:
        dserror("received illegal flag inside flux evaluation for whole domain");
      };
      // q at integration point

      // integrate and assemble everything into the "flux" vector
      for (int vi=0; vi < nen_; vi++)
      {
        for (int idim=0; idim<nsd_ ;idim++)
        {
          flux(idim,vi) += fac*funct_(vi)*q(idim);
        } // idim
      } // vi

    } // integration loop

    //set zeros for unused space dimensions
    for (int idim=nsd_; idim<3; idim++)
    {
      for (int vi=0; vi < nen_; vi++)
      {
        flux(idim,vi) = 0.0;
      }
    }

    return;
  } // ScaTraImpl::CalculateFlux

/*----------------------------------------------------------------------*
 |  calculate scalar(s) and domain integral                     vg 09/08|
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalculateScalars(
    const DRT::Element*             ele,
    const vector<double>&           ephinp,
    Epetra_SerialDenseVector&       scalars,
    const bool                      inverting
)
{
  // integrations points and weights
  const DRT::UTILS::IntPointsAndWeights<nsd_> intpoints(SCATRA::DisTypeToOptGaussRule<distype>::rule);

  // integration loop
  for (int iquad=0; iquad<intpoints.IP().nquad; ++iquad)
  {
    const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());

    // calculate integrals of (inverted) scalar(s) and domain
    if (inverting)
    {
      for (int i=0; i<nen_; i++)
      {
        const double fac_funct_i = fac*funct_(i);
        for (int k = 0; k < numscal_; k++)
        {
          scalars[k] += fac_funct_i/ephinp[i*numdofpernode_+k];
        }
        scalars[numscal_] += fac_funct_i;
      }
    }
    else
    {
      for (int i=0; i<nen_; i++)
      {
        const double fac_funct_i = fac*funct_(i);
        for (int k = 0; k < numscal_; k++)
        {
          scalars[k] += fac_funct_i*ephinp[i*numdofpernode_+k];
        }
        scalars[numscal_] += fac_funct_i;
      }
    }
  } // loop over integration points

  return;
} // ScaTraImpl::CalculateScalars


/*----------------------------------------------------------------------*
 |  calculate domain integral                                   vg 01/09|
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalculateDomainAndBodyforce(
    Epetra_SerialDenseVector&  scalars,
    const DRT::Element*        ele,
    const double               time,
    const bool                 reinitswitch
)
{
  // ---------------------------------------------------------------------
  // call routine for calculation of body force in element nodes
  // (time n+alpha_F for generalized-alpha scheme, at time n+1 otherwise)
  // ---------------------------------------------------------------------

//REINHARD
  if (reinitswitch == false)
    BodyForce(ele,time);
  else
    BodyForceReinit(ele,time);
//end REINHARD

  // integrations points and weights
  const DRT::UTILS::IntPointsAndWeights<nsd_> intpoints(SCATRA::DisTypeToOptGaussRule<distype>::rule);

  // integration loop
  for (int iquad=0; iquad<intpoints.IP().nquad; ++iquad)
  {
    const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());

    // get bodyforce in gausspoint
    rhs_[0] = bodyforce_[0].Dot(funct_);

    // calculate integrals of domain and bodyforce
    for (int i=0; i<nen_; i++)
    {
      scalars[0] += fac*funct_(i);
    }
    scalars[1] += fac*rhs_[0];

  } // loop over integration points

  return;
} // ScaTraImpl::CalculateDomain


/*----------------------------------------------------------------------*
 |  Integrate shape functions over domain (private)           gjb 07/09 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::IntegrateShapeFunctions(
    const DRT::Element*             ele,
    Epetra_SerialDenseVector&       elevec1,
    const Epetra_IntSerialDenseVector& dofids
)
{
  // integrations points and weights
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints(SCATRA::DisTypeToOptGaussRule<distype>::rule);

  // safety check
  if (dofids.M() < numdofpernode_)
    dserror("Dofids vector is too short. Received not enough flags");

  // loop over integration points
  for (int gpid=0; gpid<intpoints.IP().nquad; gpid++)
  {
    const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,gpid,ele->Id());

    // compute integral of shape functions (only for dofid)
    for (int k=0;k<numdofpernode_;k++)
    {
      if (dofids[k] >= 0)
      {
        for (int node=0;node<nen_;node++)
        {
          elevec1[node*numdofpernode_+k] += funct_(node) * fac;
        }
      }
    }

  } //loop over integration points

  return;

} //ScaTraImpl<distype>::IntegrateShapeFunction


/*----------------------------------------------------------------------*
 |  Calculate conductivity (ELCH) (private)                   gjb 07/09 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalculateConductivity(
    const DRT::Element*  ele,
    const double         frt,
    const enum INPAR::SCATRA::ScaTraType  scatratype,
    Epetra_SerialDenseVector& sigma
)
{
  GetMaterialParams(ele,scatratype);

  // use one-point Gauss rule to do calculations at the element center
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints_tau(SCATRA::DisTypeToStabGaussRule<distype>::rule);

  // evaluate shape functions (and not needed derivatives) at element center
  EvalShapeFuncAndDerivsAtIntPoint(intpoints_tau,0,ele->Id());

  // compute the conductivity (1/(\Omega m) = 1 Siemens / m)
  double sigma_all(0.0);
  const double factor = frt*96485.34; // = F^2/RT
  for(int k=0; k < numscal_; k++)
  {
    // concentration of ionic species k at element center
    double conint = funct_.Dot(ephinp_[k]);
    double sigma_k = factor*valence_[k]*diffusvalence_[k]*conint;
    sigma[k] += sigma_k; // insert value for this ionic species
    sigma_all += sigma_k;

    // effect of eliminated species c_m has to be added (c_m = - 1/z_m \sum_{k=1}^{m-1} z_k c_k)
    if(scatratype==INPAR::SCATRA::scatratype_elch_enc_pde_elim)
    {
      sigma_all += factor*diffusvalence_[numscal_]*valence_[k]*(-conint);
    }
  }
  // conductivity based on ALL ionic species (even eliminated ones!)
  sigma[numscal_] += sigma_all;

  return;

} //ScaTraImpl<distype>::CalculateConductivity


/*----------------------------------------------------------------------*
 |  CalculateElectricPotentialField (ELCH) (private)          gjb 04/10 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalculateElectricPotentialField(
    const DRT::Element*         ele,
    const double                frt,
    const enum INPAR::SCATRA::ScaTraType  scatratype,
    Epetra_SerialDenseMatrix&   emat,
    Epetra_SerialDenseVector&   erhs
)
{
  // access material parameters
  GetMaterialParams(ele,scatratype);

  // integration points and weights
  const DRT::UTILS::IntPointsAndWeights<nsd_> intpoints(SCATRA::DisTypeToOptGaussRule<distype>::rule);

  // integration loop
  for (int iquad=0; iquad<intpoints.IP().nquad; ++iquad)
  {
    const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());
    double sigmaint(0.0);
    for (int k=0; k<numscal_; ++k)
    {
      // concentration of ionic species k at element center
      double conintk = funct_.Dot(ephinp_[k]);
      double sigma_k = frt*valence_[k]*diffusvalence_[k]*conintk;
      sigmaint += sigma_k;

      // diffusive terms on rhs
      // gradient of current scalar value
      gradphi_.Multiply(derxy_,ephinp_[k]);
      const double vrhs = fac*diffusvalence_[k];
      for (int vi=0; vi<nen_; ++vi)
      {
        const int fvi = vi*numdofpernode_+numscal_;
        double laplawf(0.0);
        GetLaplacianWeakFormRHS(laplawf,derxy_,gradphi_,vi);
        erhs[fvi] -= vrhs*laplawf;
      }

      // provide something for conc. dofs: a standard mass matrix
      for (int vi=0; vi<nen_; ++vi)
      {
        const int    fvi = vi*numdofpernode_+k;
        for (int ui=0; ui<nen_; ++ui)
        {
          const int fui = ui*numdofpernode_+k;
          emat(fvi,fui) += fac*funct_(vi)*funct_(ui);
        }
      }
    } // for k

    // ----------------------------------------matrix entries
    for (int vi=0; vi<nen_; ++vi)
    {
      const int    fvi = vi*numdofpernode_+numscal_;
      for (int ui=0; ui<nen_; ++ui)
      {
        const int fui = ui*numdofpernode_+numscal_;
        double laplawf(0.0);
        GetLaplacianWeakForm(laplawf, derxy_,ui,vi);
        emat(fvi,fui) += fac*sigmaint*laplawf;
      }
    }
  } // integration loop

  return;

} //ScaTraImpl<distype>::CalculateElectricPotentialField


//Do a finite difference check for a given element id. Meant for debugging only!
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::FDcheck(
  DRT::Element*                         ele,
  Epetra_SerialDenseMatrix&             sys_mat,
  Epetra_SerialDenseVector&             residual,
  Epetra_SerialDenseVector&             subgrdiff,
  const double                          time,
  const double                          dt,
  const double                          timefac,
  const double                          alphaF,
  const enum INPAR::SCATRA::TauType     whichtau,
  const enum INPAR::SCATRA::AssgdType   whichassgd,
  const enum INPAR::SCATRA::FSSUGRDIFF  whichfssgd,
  const bool                            assgd,
  const bool                            fssgd,
  const bool                            turbmodel,
  const bool                            reinitswitch,
  const double                          Cs,
  const double                          tpn,
  const double                          frt,
  const enum INPAR::SCATRA::ScaTraType  scatratype
  )
{
  // magnitude of dof perturbation
  const double epsilon=1e-6; // 1.e-8 seems already too small!

  // make a copy of all input parameters potentially modified by Sysmat
  // call --- they are not intended to be modified

  // alloc the vectors that will store the original, non-perturbed values
  vector<LINALG::Matrix<nen_,1> > origephinp(numscal_);
  LINALG::Matrix<nen_,1>          origepotnp(true);
  vector<LINALG::Matrix<nen_,1> > origehist(numscal_);

  // copy original concentrations and potentials to these storage arrays
  for (int i=0;i<nen_;++i)
  {
    for (int k = 0; k< numscal_; ++k)
    {
      origephinp[k](i,0) = ephinp_[k](i,0);
      origehist[k](i,0)  = ehist_[k](i,0);
    }
    origepotnp(i) = epotnp_(i);
  } // for i

  // allocate arrays to compute element matrices and vectors at perturbed positions
  Epetra_SerialDenseMatrix  checkmat1(sys_mat);
  Epetra_SerialDenseVector  checkvec1(residual);
  Epetra_SerialDenseVector  checkvec2(subgrdiff);

  // echo to screen
  printf("+-------------------------------------------+\n");
  printf("| FINITE DIFFERENCE CHECK FOR ELEMENT %5d |\n",ele->Id());
  printf("+-------------------------------------------+\n");
  printf("\n");

  // loop columns of matrix by looping nodes and then dof per nodes
  // loop nodes
  for(int nn=0;nn<nen_;++nn)
  {
    printf("-------------------------------------\n");
    printf("-------------------------------------\n");
    printf("NODE of element local id %d\n",nn);
    // loop dofs
    for(int rr=0;rr<numdofpernode_;++rr)
    {
      // number of the matrix column to check
      int dof=nn*(numdofpernode_)+rr;

      // clear element matrices and vectors to assemble
      checkmat1.Scale(0.0);
      checkvec1.Scale(0.0);
      checkvec2.Scale(0.0);

      // first put the non-perturbed values to the working arrays
      for (int i=0;i<nen_;++i)
      {
        for (int k = 0; k< numscal_; ++k)
        {
          ephinp_[k](i,0) = origephinp[k](i,0);
          ehist_[k](i,0)  = origehist[k](i,0);
        }
        epotnp_(i) = origepotnp(i);
      } // for i

      // now perturb the respective elemental quantities
      if((iselch_) and (rr==(numdofpernode_-1)))
      {
        printf("potential dof (%d). eps=%g\n",nn,epsilon);

        if (is_genalpha_)
        {
          // we want to disturb phi(n+1) with epsilon
          // => we have to disturb phi(n+alphaF) with alphaF*epsilon
          epotnp_(nn)+=(alphaF*epsilon);
        }
        else
        {
          epotnp_(nn)+=epsilon;
        }
      }
      else
      {
        printf("concentration dof %d (%d)\n",rr,nn);

        if (is_genalpha_)
        {
          // perturbation of phi(n+1) in phi(n+alphaF) => additional factor alphaF
          ephinp_[rr](nn,0)+=(alphaF*epsilon);

          // perturbation of solution variable phi(n+1) for gen.alpha
          // leads to perturbation of phidtam (stored in ehist_)
          // with epsilon*alphaM/(gamma*dt)
          const double factor = alphaF/timefac; // = alphaM/(gamma*dt)
          ehist_[rr](nn,0)+=(factor*epsilon);

        }
        else
        {
          ephinp_[rr](nn,0)+=epsilon;
        }
      }

      // calculate the right hand side for the perturbed vector
      Sysmat(
          ele,
          checkmat1,
          checkvec1,
          checkvec2,
          time,
          dt,
          timefac,
          alphaF,
          whichtau,
          whichassgd,
          whichfssgd,
          assgd,
          fssgd,
          turbmodel,
          reinitswitch,
          Cs,
          tpn,
          frt,
          scatratype);

      // compare the difference between linaer approximation and
      // (nonlinear) right hand side evaluation

      // note that it makes more sense to compare these quantities
      // than to compare the matrix entry to the difference of the
      // the right hand sides --- the latter causes numerical problems
      // do to deletion //gammi

      // however, matrix entries delivered from the element are compared
      // with the finite-difference suggestion, too. It works surprisingly well
      // for epsilon set to 1e-6 (all displayed digits nearly correct)
      // and allows a more obvious comparison!
      // when matrix entries are small, lin. and nonlin. approximation
      // look identical, although the matrix entry may be rubbish!
      // gjb

      for(int mm=0;mm<(numdofpernode_*nen_);++mm)
      {
        double val   =-residual(mm)/epsilon;
        double lin   =-residual(mm)/epsilon+sys_mat(mm,dof);
        double nonlin=-checkvec1(mm)/epsilon;

        double norm=abs(lin);
        if(norm<1e-12)
        {
          norm=1e-12;
          cout<<"warning norm of lin is set to 10e-12"<<endl;
        }

        // output to screen
        {
          printf("relerr  %+12.5e   ",(lin-nonlin)/norm);
          printf("abserr  %+12.5e   ",lin-nonlin);
          printf("orig. value  %+12.5e   ",val);
          printf("lin. approx. %+12.5e   ",lin);
          printf("nonlin. funct.  %+12.5e   ",nonlin);
          printf("matrix[%d,%d]  %+12.5e   ",mm,dof,sys_mat(mm,dof));
          // finite difference approximation (FIRST divide by epsilon and THEN subtract!)
          // ill-conditioned operation has to be done as late as possible!
          printf("FD suggestion  %+12.5e ",((residual(mm)/epsilon)-(checkvec1(mm)/epsilon)) );
          printf("\n");
        }
      }
    }
  } // loop nodes

  // undo changes in state variables
  for (int i=0;i<nen_;++i)
  {
    for (int k = 0; k< numscal_; ++k)
    {
      ephinp_[k](i,0) = origephinp[k](i,0);
      ehist_[k](i,0)  = origehist[k](i,0);
    }
    epotnp_(i) = origepotnp(i);
  } // for i

  return;
}



double EvaluateDerivSmoothedHeavySide(
		double                             phi_0,
		const double                       epsilon_bandwidth,
		const double                       mesh_size,
		INPAR::SCATRA::SmoothedSignType    smoothedSignType
		)
{
	//==========================================================
	// evaluate smoothed sign function for reference phi-field
	//==========================================================

	double deriv_smoothed_Heavyside = 0.0;

	if(smoothedSignType == INPAR::SCATRA::signtype_nonsmoothed)
	{
		deriv_smoothed_Heavyside = 0.0;
	}
	else if (smoothedSignType == INPAR::SCATRA::signtype_LinEtAl2005)
	{
//		double alpha = epsilon_bandwidth * mesh_size;
//		sign_phi_0 = phi_0/sqrt(phi_0*phi_0 + alpha*alpha * grad_norm_phi_0*grad_norm_phi_0);
		dserror("derivative of smoothed Heavyside fucntion not implemented yet");
	}
	else if (smoothedSignType == INPAR::SCATRA::signtype_LinEtAl_normalized)
	{
//		double alpha = epsilon_bandwidth * mesh_size;
//		sign_phi_0 = phi_0/sqrt(phi_0*phi_0 + alpha*alpha);
		dserror("derivative of smoothed Heavyside fucntion not implemented yet");
	}
	else if (smoothedSignType == INPAR::SCATRA::signtype_Nagrath2005)
	{
		double alpha = epsilon_bandwidth * mesh_size;
		if(fabs(alpha) < 1e-15) dserror("divide by zero in evaluate for smoothed sign function");

		if (phi_0 < -alpha)       deriv_smoothed_Heavyside = 0.0;
		else if (phi_0 > alpha)   deriv_smoothed_Heavyside = 0.0;
		else                      deriv_smoothed_Heavyside = 1.0/(2.0*alpha)*(1.0 + cos(PI*phi_0/alpha));
	}
	else dserror("unknown type of smoothed sign function!");


	return deriv_smoothed_Heavyside;
}


double EvaluateSmoothedSign(
		double                             phi_0,
		double                             grad_norm_phi_0,
		const double                       epsilon_bandwidth,
		const double                       mesh_size,
		INPAR::SCATRA::SmoothedSignType    smoothedSignType
		)
{
	//==========================================================
	// evaluate smoothed sign function for reference phi-field
	//==========================================================

	double sign_phi_0 = 0.0;

	if(smoothedSignType == INPAR::SCATRA::signtype_nonsmoothed)
	{
		if      (phi_0 < 0.0) sign_phi_0 = -1.0;
		else if (phi_0 > 0.0) sign_phi_0 = +1.0;
		else                  sign_phi_0 =  0.0;
	}
	else if (smoothedSignType == INPAR::SCATRA::signtype_LinEtAl2005)
	{
		double alpha = epsilon_bandwidth * mesh_size;
		sign_phi_0 = phi_0/sqrt(phi_0*phi_0 + alpha*alpha * grad_norm_phi_0*grad_norm_phi_0);
	}
	else if (smoothedSignType == INPAR::SCATRA::signtype_LinEtAl_normalized)
	{
		double alpha = epsilon_bandwidth * mesh_size;
		sign_phi_0 = phi_0/sqrt(phi_0*phi_0 + alpha*alpha);
	}
	else if (smoothedSignType == INPAR::SCATRA::signtype_Nagrath2005)
	{
		double alpha = epsilon_bandwidth * mesh_size;
		if(fabs(alpha) < 1e-15) dserror("divide by zero in evaluate for smoothed sign function");

		if (phi_0 < -alpha)       sign_phi_0 = -1.0;
		else if (phi_0 > alpha)   sign_phi_0 = +1.0;
		else                      sign_phi_0 = (1.0 + phi_0/alpha + 1.0/PI*sin(PI*phi_0/alpha))-1.0;
	}
	else dserror("unknown type of smoothed sign function!");


	return sign_phi_0;
}




/*-------------------------------------------------------------------------------*
 |  evaluate element matrix and rhs for reinitialization (private)   schott 12/10|
 *-------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalMatAndRHS_LinearAdvection_REINITIALIZATION(
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const double                          fac,
    const int                             dofindex,
    DRT::Element*                         ele,
    double                                pseudo_timestep_size_factor,
    double                                meshsize,
    const INPAR::SCATRA::PenaltyMethod    penalty_method,
    const double                          penalty_interface_reinit,
    const double                          epsilon_bandwidth,
    INPAR::SCATRA::SmoothedSignType       smoothedSignType,
    const bool                            shock_capturing,
    const double                          shock_capturing_diffusivity,
    double                                timefac
    )
{


	//----------------------------------------------------------------
	// 1) element matrix: stationary terms
	//----------------------------------------------------------------
	// stabilization parameter and integration factors
	const double taufac     = tau_[dofindex]*fac;
	const double timefacfac = timefac*fac;
	const double timetaufac = timefac*taufac;
	const double fac_diffus = timefacfac*diffus_[dofindex];



	//----------------------------------------------------------------
	// standard Galerkin terms
	//----------------------------------------------------------------
	// convective term in convective form
	const double densfac = timefacfac*densnp_[dofindex];
	for (int vi=0; vi<nen_; ++vi)
	{
	  const double v = densfac*funct_(vi);
	  const int fvi = vi*numdofpernode_+dofindex;

	  for (int ui=0; ui<nen_; ++ui)
	  {
	    const int fui = ui*numdofpernode_+dofindex;

	    emat(fvi,fui) += v*(conv_(ui)+sgconv_(ui));
	  }
	}

	// addition to convective term for conservative form
	if (conservative_)
	{
	  // gradient of current scalar value
	  gradphi_.Multiply(derxy_,ephinp_[dofindex]);

	  // convective term using current scalar value
	  const double cons_conv_phi = velint_.Dot(gradphi_);

	  const double consfac = timefacfac*(densnp_[dofindex]*vdiv_+densgradfac_[dofindex]*cons_conv_phi);
	  for (int vi=0; vi<nen_; ++vi)
	  {
	    const double v = consfac*funct_(vi);
	    const int fvi = vi*numdofpernode_+dofindex;

	    for (int ui=0; ui<nen_; ++ui)
	    {
	      const int fui = ui*numdofpernode_+dofindex;

	      emat(fvi,fui) += v*funct_(ui);
	    }
	  }
	}

	// diffusive term
	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  for (int ui=0; ui<nen_; ++ui)
	  {
	    const int fui = ui*numdofpernode_+dofindex;
	    double laplawf(0.0);
	    GetLaplacianWeakForm(laplawf, derxy_,ui,vi);
	    emat(fvi,fui) += fac_diffus*laplawf;
	  }
	}


if(shock_capturing)
{
	const double fac_shock_capt = timefacfac*shock_capturing_diffusivity;

	// diffusive shock capturing term
	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  for (int ui=0; ui<nen_; ++ui)
	  {
		const int fui = ui*numdofpernode_+dofindex;
		double laplawf(0.0);
		GetLaplacianWeakForm(laplawf, derxy_,ui,vi);
		emat(fvi,fui) += fac_shock_capt*laplawf;
	  }
	}


	// gradient of current scalar value
	gradphi_.Multiply(derxy_,ephinp_[dofindex]);
	// diffusive shock capturing term
	double vrhs_shock_capt = fac_shock_capt;
	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  double laplawf(0.0);
	  GetLaplacianWeakFormRHS(laplawf,derxy_,gradphi_,vi);
	  erhs[fvi] -= vrhs_shock_capt*laplawf;
	}
}

if(penalty_method == INPAR::SCATRA::penalty_method_akkerman)
{

	// get derivative of smoothed sign_function
	//evaluate phinp and phi_0
	double phinp = funct_.Dot(ephinp_[dofindex]);
	double phi_ref = funct_.Dot(ephi0_Reinit_Reference_[dofindex]);

	double deriv_smoothed_Heavyside = EvaluateDerivSmoothedHeavySide(phi_ref, epsilon_bandwidth,meshsize, smoothedSignType);

	const double densfac_penalty = timefacfac*densnp_[dofindex]*penalty_interface_reinit*deriv_smoothed_Heavyside;
	for (int vi=0; vi<nen_; ++vi)
	{
	  const double v = densfac_penalty*funct_(vi);
	  const int fvi = vi*numdofpernode_+dofindex;

	  for (int ui=0; ui<nen_; ++ui)
	  {
		const int fui = ui*numdofpernode_+dofindex;

		emat(fvi,fui) += v*funct_(ui);
	  }
	}

	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  erhs[fvi] -= funct_(vi)*densfac_penalty*( phinp - phi_ref) ;
	}
}

	//----------------------------------------------------------------
	// convective stabilization term
	//----------------------------------------------------------------
	// convective stabilization of convective term (in convective form)
	const double dens2taufac = timetaufac*densnp_[dofindex]*densnp_[dofindex];
	for (int vi=0; vi<nen_; ++vi)
	{
	  const double v = dens2taufac*(conv_(vi)+sgconv_(vi));
	  const int fvi = vi*numdofpernode_+dofindex;

	  for (int ui=0; ui<nen_; ++ui)
	  {
	    const int fui = ui*numdofpernode_+dofindex;

	    emat(fvi,fui) += v*conv_(ui);
	  }
	}

	//----------------------------------------------------------------
	// stabilization terms for higher-order elements
	//----------------------------------------------------------------
	if (use2ndderiv_)
	{
	  // diffusive part:  diffus * ( N,xx  +  N,yy +  N,zz )
	  GetLaplacianStrongForm(diff_, derxy2_);
	  diff_.Scale(diffus_[dofindex]);

	  const double denstaufac = timetaufac*densnp_[dofindex];
	  // convective stabilization of diffusive term (in convective form)
	  for (int vi=0; vi<nen_; ++vi)
	  {
	    const double v = denstaufac*(conv_(vi)+sgconv_(vi));
	    const int fvi = vi*numdofpernode_+dofindex;

	    for (int ui=0; ui<nen_; ++ui)
	    {
	      const int fui = ui*numdofpernode_+dofindex;

	      emat(fvi,fui) -= v*diff_(ui);
	    }
	  }

	  const double densdifftaufac = diffreastafac_*denstaufac;
	  // diffusive stabilization of convective term (in convective form)
	  for (int vi=0; vi<nen_; ++vi)
	  {
	    const double v = densdifftaufac*diff_(vi);
	    const int fvi = vi*numdofpernode_+dofindex;

	    for (int ui=0; ui<nen_; ++ui)
	    {
	      const int fui = ui*numdofpernode_+dofindex;

	      emat(fvi,fui) -= v*conv_(ui);
	    }
	  }

	  const double difftaufac = diffreastafac_*timetaufac;
	  // diffusive stabilization of diffusive term
	  for (int vi=0; vi<nen_; ++vi)
	  {
	    const double v = difftaufac*diff_(vi);
	    const int fvi = vi*numdofpernode_+dofindex;

	    for (int ui=0; ui<nen_; ++ui)
	    {
	      const int fui = ui*numdofpernode_+dofindex;

	      emat(fvi,fui) += v*diff_(ui);
	    }
	  }
	}

	//----------------------------------------------------------------
	// 2) element matrix: instationary terms
	//----------------------------------------------------------------
	if (not is_stationary_)
	{
	  const double densamfac = fac*densam_[dofindex];
	  //----------------------------------------------------------------
	  // standard Galerkin transient term
	  //----------------------------------------------------------------
	  // transient term
	  for (int vi=0; vi<nen_; ++vi)
	  {
	    const double v = densamfac*funct_(vi);
	    const int fvi = vi*numdofpernode_+dofindex;

	    for (int ui=0; ui<nen_; ++ui)
	    {
	      const int fui = ui*numdofpernode_+dofindex;

	      emat(fvi,fui) += v*funct_(ui);
	    }
	  }

	  const double densamnptaufac = taufac*densam_[dofindex]*densnp_[dofindex];
	  //----------------------------------------------------------------
	  // stabilization of transient term
	  //----------------------------------------------------------------
	  // convective stabilization of transient term (in convective form)
	  for (int vi=0; vi<nen_; ++vi)
	  {
	    const double v = densamnptaufac*(conv_(vi)+sgconv_(vi));
	    const int fvi = vi*numdofpernode_+dofindex;

	    for (int ui=0; ui<nen_; ++ui)
	    {
	      const int fui = ui*numdofpernode_+dofindex;

	      emat(fvi,fui) += v*funct_(ui);
	    }
	  }

	  if (use2ndderiv_)
	  {
	    const double densamreataufac = diffreastafac_*taufac*densam_[dofindex];
	    // diffusive stabilization of transient term
	    for (int vi=0; vi<nen_; ++vi)
	    {
	      const double v = densamreataufac*diff_(vi);
	      const int fvi = vi*numdofpernode_+dofindex;

	      for (int ui=0; ui<nen_; ++ui)
	      {
	        const int fui = ui*numdofpernode_+dofindex;

	        emat(fvi,fui) -= v*funct_(ui);
	      }
	    }
	  }
	}

	//----------------------------------------------------------------
	// 3) element matrix: reactive terms
	//----------------------------------------------------------------
	if (reaction_)
	{
	  const double fac_reac        = timefacfac*densnp_[dofindex]*reacoeff_[dofindex];
	  const double timetaufac_reac = timetaufac*densnp_[dofindex]*reacoeff_[dofindex];
	  //----------------------------------------------------------------
	  // standard Galerkin reactive term
	  //----------------------------------------------------------------
	  for (int vi=0; vi<nen_; ++vi)
	  {
	    const double v = fac_reac*funct_(vi);
	    const int fvi = vi*numdofpernode_+dofindex;

	    for (int ui=0; ui<nen_; ++ui)
	    {
	      const int fui = ui*numdofpernode_+dofindex;

	      emat(fvi,fui) += v*funct_(ui);
	    }
	  }

	  //----------------------------------------------------------------
	  // stabilization of reactive term
	  //----------------------------------------------------------------
	  double densreataufac = timetaufac_reac*densnp_[dofindex];
	  // convective stabilization of reactive term (in convective form)
	  for (int vi=0; vi<nen_; ++vi)
	  {
	    const double v = densreataufac*(conv_(vi)+sgconv_(vi));
	    const int fvi = vi*numdofpernode_+dofindex;

	    for (int ui=0; ui<nen_; ++ui)
	    {
	      const int fui = ui*numdofpernode_+dofindex;

	      emat(fvi,fui) += v*funct_(ui);
	    }
	  }

	  if (use2ndderiv_)
	  {
	    // diffusive stabilization of reactive term
	    for (int vi=0; vi<nen_; ++vi)
	    {
	      const double v = diffreastafac_*timetaufac_reac*diff_(vi);
	      const int fvi = vi*numdofpernode_+dofindex;

	      for (int ui=0; ui<nen_; ++ui)
	      {
	        const int fui = ui*numdofpernode_+dofindex;

	        emat(fvi,fui) -= v*funct_(ui);
	      }
	    }
	  }

	  //----------------------------------------------------------------
	  // reactive stabilization
	  //----------------------------------------------------------------
	  densreataufac = diffreastafac_*timetaufac_reac*densnp_[dofindex];
	  // reactive stabilization of convective (in convective form) and reactive term
	  for (int vi=0; vi<nen_; ++vi)
	  {
	    const double v = densreataufac*funct_(vi);
	    const int fvi = vi*numdofpernode_+dofindex;

	    for (int ui=0; ui<nen_; ++ui)
	    {
	      const int fui = ui*numdofpernode_+dofindex;

	      emat(fvi,fui) += v*(conv_(ui)+reacoeff_[dofindex]*funct_(ui));
	    }
	  }

	  if (use2ndderiv_)
	  {
	    // reactive stabilization of diffusive term
	    for (int vi=0; vi<nen_; ++vi)
	    {
	      const double v = diffreastafac_*timetaufac_reac*funct_(vi);
	      const int fvi = vi*numdofpernode_+dofindex;

	      for (int ui=0; ui<nen_; ++ui)
	      {
	        const int fui = ui*numdofpernode_+dofindex;

	        emat(fvi,fui) -= v*diff_(ui);
	      }
	    }
	  }
	}

	//----------------------------------------------------------------
	// 4) element right hand side
	//----------------------------------------------------------------
	//----------------------------------------------------------------
	// computation of bodyforce (and potentially history) term,
	// residual, integration factors and standard Galerkin transient
	// term (if required) on right hand side depending on respective
	// (non-)incremental stationary or time-integration scheme
	//----------------------------------------------------------------
	double rhsint    = rhs_[dofindex];
	double residual  = 0.0;
	double rhsfac    = 0.0;
	double rhstaufac = 0.0;
	double conv_phi  = 0.0;
	double diff_phi  = 0.0;
	double rea_phi   = 0.0;
	if (is_incremental_ and is_genalpha_)
	{
		dserror("generalized alpha implementation not yet available");
//	  // gradient of current scalar value
//	  gradphi_.Multiply(derxy_,ephinp_[dofindex]);
//
//	  // convective term using current scalar value
//	  conv_phi = velint_.Dot(gradphi_);
//
//	  // diffusive term using current scalar value for higher-order elements
//	  if (use2ndderiv_) diff_phi = diff_.Dot(ephinp_[dofindex]);
//
//	  // reactive term using current scalar value
//	  if (reaction_)
//	  {
//	    // scalar at integration point
//	    const double phi = funct_.Dot(ephinp_[dofindex]);
//
//	    rea_phi = densnp_[dofindex]*reacoeff_[dofindex]*phi;
//	  }
//
//	  // time derivative stored on history variable
//	  residual  = densam_[dofindex]*hist_[dofindex] + densnp_[dofindex]*conv_phi - diff_phi + rea_phi - rhsint;
//	  rhsfac    = timefacfac/alphaF;
//	  rhstaufac = timetaufac/alphaF;
//	  rhsint   *= (timefac/alphaF);
//
//	  const double vtrans = rhsfac*densam_[dofindex]*hist_[dofindex];
//	  for (int vi=0; vi<nen_; ++vi)
//	  {
//	    const int fvi = vi*numdofpernode_+dofindex;
//
//	    erhs[fvi] -= vtrans*funct_(vi);
//	  }
//
//	  // addition to convective term due to subgrid-scale velocity
//	  // (not included in residual)
//	  double sgconv_phi = sgvelint_.Dot(gradphi_);
//	  conv_phi += sgconv_phi;
//
//	  // addition to convective term for conservative form
//	  // (not included in residual)
//	  if (conservative_)
//	  {
//	    // scalar at integration point at time step n
//	    const double phi = funct_.Dot(ephinp_[dofindex]);
//
//	    // convective term in conservative form
//	    conv_phi += phi*(vdiv_+(densgradfac_[dofindex]/densnp_[dofindex])*conv_phi);
//	  }
//
//	  // multiply convective term by density
//	  conv_phi *= densnp_[dofindex];
	}
	else if (not is_incremental_ and is_genalpha_)
	{
		dserror("generalized alpha implementation not yet available");
//	  // gradient of current scalar value
//	  gradphi_.Multiply(derxy_,ephin_[dofindex]);
//
//	  // convective term using current scalar value
//	  conv_phi = velint_.Dot(gradphi_);
//
//	  // diffusive term using current scalar value for higher-order elements
//	  if (use2ndderiv_) diff_phi = diff_.Dot(ephin_[dofindex]);
//
//	  // reactive term using current scalar value
//	  if (reaction_)
//	  {
//	    // scalar at integration point
//	    const double phi = funct_.Dot(ephin_[dofindex]);
//
//	    rea_phi = densnp_[dofindex]*reacoeff_[dofindex]*phi;
//	  }
//
//	  rhsint   += densam_[dofindex]*hist_[dofindex]*(alphaF/timefac);
//	  residual  = (1.0-alphaF) * (densn_[dofindex]*conv_phi - diff_phi + rea_phi) - rhsint;
//	  rhsfac    = timefacfac*(1.0-alphaF)/alphaF;
//	  rhstaufac = timetaufac/alphaF;
//	  rhsint   *= (timefac/alphaF);
//
//	  // addition to convective term due to subgrid-scale velocity
//	  // (not included in residual)
//	  double sgconv_phi = sgvelint_.Dot(gradphi_);
//	  conv_phi += sgconv_phi;
//
//	  // addition to convective term for conservative form
//	  // (not included in residual)
//	  if (conservative_)
//	  {
//	    // scalar at integration point at time step n
//	    const double phi = funct_.Dot(ephin_[dofindex]);
//
//	    // convective term in conservative form
//	    // caution: velocity divergence is for n+1 and not for n!
//	    // -> hopefully, this inconsistency is of small amount
//	    conv_phi += phi*(vdiv_+(densgradfac_[dofindex]/densn_[dofindex])*conv_phi);
//	  }
//
//	  // multiply convective term by density
//	  conv_phi *= densn_[dofindex];
	}
	else if (is_incremental_ and not is_genalpha_)
	{
	  // gradient of current scalar value
	  gradphi_.Multiply(derxy_,ephinp_[dofindex]);

	  // convective term using current scalar value
	  conv_phi = velint_.Dot(gradphi_);

	  // diffusive term using current scalar value for higher-order elements
	  if (use2ndderiv_) diff_phi = diff_.Dot(ephinp_[dofindex]);

	  // reactive term using current scalar value
	  if (reaction_)
	  {
	    // scalar at integration point
	    const double phi = funct_.Dot(ephinp_[dofindex]);

	    rea_phi = densnp_[dofindex]*reacoeff_[dofindex]*phi;
	  }

	  if (not is_stationary_)
	  {
	    // compute scalar at integration point
	    double dens_phi = funct_.Dot(ephinp_[dofindex]);

	    rhsint  *= timefac;
	    rhsint  += densnp_[dofindex]*hist_[dofindex];
	    residual = densnp_[dofindex]*dens_phi + timefac*(densnp_[dofindex]*conv_phi - diff_phi + rea_phi) - rhsint;
	    rhsfac   = timefacfac;

	    const double vtrans = fac*densnp_[dofindex]*dens_phi;
	    for (int vi=0; vi<nen_; ++vi)
	    {
	      const int fvi = vi*numdofpernode_+dofindex;

	      erhs[fvi] -= vtrans*funct_(vi);
	    }
	  }
	  else
	  {
	    residual = densnp_[dofindex]*conv_phi - diff_phi + rea_phi - rhsint;
	    rhsfac   = fac;
	  }
	  rhstaufac = taufac;

	  // addition to convective term due to subgrid-scale velocity
	  // (not included in residual)
	  double sgconv_phi = sgvelint_.Dot(gradphi_);
	  conv_phi += sgconv_phi;

	  // addition to convective term for conservative form
	  // (not included in residual)
	  if (conservative_)
	  {
	    // scalar at integration point at time step n
	    const double phi = funct_.Dot(ephinp_[dofindex]);

	    // convective term in conservative form
	    conv_phi += phi*(vdiv_+(densgradfac_[dofindex]/densnp_[dofindex])*conv_phi);
	  }

	  // multiply convective term by density
	  conv_phi *= densnp_[dofindex];
	}
	else
	{
	  if (not is_stationary_)
	  {
	    rhsint *= timefac;
	    rhsint += densnp_[dofindex]*hist_[dofindex];
	  }
	  residual  = -rhsint;
	  rhstaufac = taufac;
	}

	//----------------------------------------------------------------
	// standard Galerkin bodyforce term
	//----------------------------------------------------------------
	double vrhs = fac*rhsint;
	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  erhs[fvi] += vrhs*funct_(vi);
	}

	//----------------------------------------------------------------
	// standard Galerkin terms on right hand side
	//----------------------------------------------------------------
	// convective term
	vrhs = rhsfac*conv_phi;
	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  erhs[fvi] -= vrhs*funct_(vi);
	}

	// diffusive term
	vrhs = rhsfac*diffus_[dofindex];
	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  double laplawf(0.0);
	  GetLaplacianWeakFormRHS(laplawf,derxy_,gradphi_,vi);
	  erhs[fvi] -= vrhs*laplawf;
	}




	//----------------------------------------------------------------
	// stabilization terms
	//----------------------------------------------------------------
	// convective rhs stabilization (in convective form)
	vrhs = rhstaufac*residual*densnp_[dofindex];
	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  erhs[fvi] -= vrhs*(conv_(vi)+sgconv_(vi));
	}

	// diffusive rhs stabilization
	if (use2ndderiv_)
	{
	  vrhs = rhstaufac*residual;
	  // diffusive stabilization of convective temporal rhs term (in convective form)
	  for (int vi=0; vi<nen_; ++vi)
	  {
	    const int fvi = vi*numdofpernode_+dofindex;

	    erhs[fvi] += diffreastafac_*vrhs*diff_(vi);
	  }
	}

	//----------------------------------------------------------------
	// reactive terms (standard Galerkin and stabilization) on rhs
	//----------------------------------------------------------------
	// standard Galerkin term
	if (reaction_)
	{
	  vrhs = rhsfac*rea_phi;
	  for (int vi=0; vi<nen_; ++vi)
	  {
	    const int fvi = vi*numdofpernode_+dofindex;

	    erhs[fvi] -= vrhs*funct_(vi);
	  }

	  // reactive rhs stabilization
	  vrhs = diffreastafac_*rhstaufac*densnp_[dofindex]*reacoeff_[dofindex]*residual;
	  for (int vi=0; vi<nen_; ++vi)
	  {
	    const int fvi = vi*numdofpernode_+dofindex;

	    erhs[fvi] -= vrhs*funct_(vi);
	  }
	}

//	//----------------------------------------------------------------
//	// fine-scale subgrid-diffusivity term on right hand side
//	//----------------------------------------------------------------
//	if (is_incremental_ and fssgd)
//	{
//		cout << "check the gssgd case for reinitialization" << endl;
////	  vrhs = rhsfac*sgdiff_[dofindex];
////	  for (int vi=0; vi<nen_; ++vi)
////	  {
////	    const int fvi = vi*numdofpernode_+dofindex;
////
////	    double laplawf(0.0);
////	    GetLaplacianWeakFormRHS(laplawf,derxy_,fsgradphi_,vi);
////	    erhs[fvi] -= vrhs*laplawf;
////	  }
//	}

  return;
} //ScaTraImpl::CalMatAndRHS_Characteristic_Galerkin_REINITIALIZATION

/*-------------------------------------------------------------------------------*
 |  evaluate element matrix and rhs for reinitialization (private)   schott 12/10|
 *-------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalMatAndRHS_Characteristic_Galerkin_REINITIALIZATION(
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const double                          fac,
    const int                             dofindex,
    DRT::Element*                         ele,
    double                                pseudo_timestep_size,
    double                                mesh_size,
    const INPAR::SCATRA::PenaltyMethod    penalty_method,
    const double                          penalty_interface_reinit,
    const double                          epsilon_bandwidth,
    INPAR::SCATRA::SmoothedSignType       smoothedSignType
    )
{

//==========================================================
// evaluate element vectors and gradients
//==========================================================
// get element vectors
// dist_npi   distance vector for reinitialization at current timestep np and old increment i
// dist_n     distance vector for reinitialization at old timestep n
// phi_0      reference phi vector for smoothed sign function -> needed for directed transport along characteristics
const double dist_n   = funct_.Dot(ephin_[dofindex]);   // d^n
const double dist_npi = funct_.Dot(ephinp_[dofindex]);  // d^(n+1)_i
const double phi_0    = funct_.Dot(ephi0_Reinit_Reference_[dofindex]);

// get gradients and norms
// grad_dist_npi   distance vector for reinitialization at current timestep np and old increment i
// grad_dist_n     distance vector for reinitialization at old timestep n
// grad_phi_0      reference phi vector for smoothed sign function -> needed for directed transport along characteristics

LINALG::Matrix<nsd_,1> grad_dist_n(true);
grad_dist_n.Multiply(derxy_,ephin_[dofindex]);

LINALG::Matrix<nsd_,1> grad_dist_npi(true);
grad_dist_npi.Multiply(derxy_,ephinp_[dofindex]);

LINALG::Matrix<nsd_,1> grad_phi_0(true);
grad_phi_0.Multiply(derxy_,ephi0_Reinit_Reference_[dofindex]);

double grad_norm_dist_n   = grad_dist_n.Norm2();
#ifdef HIGHER_ORDER_NEW
double grad_norm_dist_npi = grad_dist_npi.Norm2();
#endif
#ifdef FIXPOINTLIKE
double grad_norm_dist_npi = grad_dist_npi.Norm2();
#endif
double grad_norm_phi_0    = grad_phi_0.Norm2();

//bool grad_norm_zero = false;
//if(grad_norm_dist_n< 1e-02) grad_norm_zero=true;	// do not assemble nonlinear higher order terms
//if(grad_norm_dist_npi< 1e-02) grad_norm_zero=true;	// do not assemble nonlinear higher order terms


// get 2nd order derivatives
LINALG::Matrix<numderiv2_,1> second_dist_n;
second_dist_n.Clear();
second_dist_n.Multiply(derxy2_,ephin_[dofindex]);

LINALG::Matrix<numderiv2_,1> second_dist_npi;
second_dist_npi.Clear();
second_dist_npi.Multiply(derxy2_,ephinp_[dofindex]);


double sign_phi_0 = EvaluateSmoothedSign(phi_0, grad_norm_phi_0, epsilon_bandwidth, mesh_size, smoothedSignType);


if(penalty_method == INPAR::SCATRA::penalty_method_akkerman)
{

	// get derivative of smoothed sign_function
	//evaluate phinp and phi_0
	double phinp = funct_.Dot(ephinp_[dofindex]);
	double phi_ref = funct_.Dot(ephi0_Reinit_Reference_[dofindex]);

	double deriv_smoothed_Heavyside = EvaluateDerivSmoothedHeavySide(phi_ref, epsilon_bandwidth,mesh_size, smoothedSignType);
	//deriv_smoohted_sign =

	const double densfac_penalty = pseudo_timestep_size*fac*densnp_[dofindex]*penalty_interface_reinit*deriv_smoothed_Heavyside;
	for (int vi=0; vi<nen_; ++vi)
	{
	  const double v = densfac_penalty*funct_(vi);
	  const int fvi = vi*numdofpernode_+dofindex;

	  for (int ui=0; ui<nen_; ++ui)
	  {
		const int fui = ui*numdofpernode_+dofindex;

		emat(fvi,fui) += v*funct_(ui);
	  }
	}

	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  erhs[fvi] -= funct_(vi)*densfac_penalty*( phinp - phi_ref) ;
	}
}



//----------------------------------------------------------------
// standard Galerkin transient term
//----------------------------------------------------------------

//     |           |
//     | w, D(psi) |
//     |           |

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) += funct_(vi)*fac*funct_(ui);
  }
}


//               |                       |
//   1/4 dtau^2  | grad(w), grad(D(psi)) |
//               |                       |

LINALG::Matrix<nen_,nen_>  derxyMultderxy;
derxyMultderxy.Clear();
derxyMultderxy.MultiplyTN(derxy_,derxy_);

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) += derxyMultderxy(vi,ui)* (fac*pseudo_timestep_size*pseudo_timestep_size/4.0);
  }
}


//----------  --------------    |                       m         |
//  rhs                    dtau | w, so(1.0- || grad(psi ) ||)    |
//--------------------------    |                                 |

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] += funct_(vi)*pseudo_timestep_size*fac*sign_phi_0*(1.0-grad_norm_dist_n);
}


//----------  --------------                |                 m   |
//  rhs                    - 1.0/2.0*dtau^2 | grad(w),grad(psi )  |
//--------------------------                |                     |

LINALG::Matrix<nen_,1> derxyMultGradn;
derxyMultGradn.Clear();
derxyMultGradn.MultiplyTN(derxy_,grad_dist_n);



for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= derxyMultGradn(vi)*pseudo_timestep_size*pseudo_timestep_size*fac/2.0;
}


#ifdef NONLINEAR


#ifdef HIGHER_ORDER_NEW
//if(grad_norm_zero==false)
{
//============================================================================================================================
// Assemble domain 2nd order integrals!!!

//----------  --------------                |                m+1                m+1                              m+1   |
//  rhs                    + 1.0/4.0*dtau^2 | w,so^2/(grad(psi   ))^2  d/dxk psi   * sum(j=1..nsd)(d^2/dxjdxk psi    ) |
//--------------------------                |                i                  i                                i     |

if(numderiv2_ != 6) dserror("this is not a 3D case!!!");


//cout << "second_dist_npi" << second_dist_npi << endl;
double second_deriv_tmp_npi = 0.0;
second_deriv_tmp_npi = second_dist_npi(0,0) *grad_dist_npi(0,0)
		             + second_dist_npi(1,0) *grad_dist_npi(1,0)
		             + second_dist_npi(2,0) *grad_dist_npi(2,0)
		             + second_dist_npi(3,0) *(grad_dist_npi(0,0) +grad_dist_npi(1,0))
		             + second_dist_npi(4,0) *(grad_dist_npi(0,0) +grad_dist_npi(2,0))
		             + second_dist_npi(5,0) *(grad_dist_npi(1,0) +grad_dist_npi(2,0));
//cout << "second_deriv_tmp_npi_1" << second_deriv_tmp_npi;

//cout << sign_phi_0*sign_phi_0* second_deriv_tmp_npi* pseudo_timestep_size*pseudo_timestep_size*fac/4.0 << endl;
if(grad_norm_dist_npi< 1e-13){
	cout << "warning: grad_norm_dist_npi is near zero!!! in element" << ele->Id() << grad_norm_dist_npi << endl;
	cout << grad_dist_npi << endl;
	cout << ephinp_[dofindex] << endl;
}

if(grad_norm_dist_n< 1e-13) cout << "warning: grad_norm_dist_n is near zero!!!" << ele->Id() << grad_norm_dist_n << endl;

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] += funct_(vi)*sign_phi_0*sign_phi_0* second_deriv_tmp_npi* pseudo_timestep_size*pseudo_timestep_size*fac/(4.0*grad_norm_dist_npi*grad_norm_dist_npi);
//  erhs[fvi] += funct_(vi)*1.0* second_deriv_tmp_npi* pseudo_timestep_size*pseudo_timestep_size*fac/(4.0*grad_norm_dist_npi*grad_norm_dist_npi);

}
//cout << "second_deriv_tmp_npi" << second_deriv_tmp_npi << endl;

//----------  --------------
//  mat                    2)
//--------------------------

//double tmp_a = 2.0*sign_phi_0*sign_phi_0*second_deriv_tmp_npi/ pow(grad_norm_dist_npi,4.0);
double tmp_a = 2.0*second_deriv_tmp_npi/ pow(grad_norm_dist_npi,4.0);

LINALG::Matrix<1,nen_> grad_psi_Mult_Dpsi;
grad_psi_Mult_Dpsi.Clear();
grad_psi_Mult_Dpsi.MultiplyTN(grad_dist_npi,derxy_);

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) += funct_(vi)*pseudo_timestep_size*pseudo_timestep_size*fac/4.0*tmp_a*grad_psi_Mult_Dpsi(0,ui);
  }
}

//----------  --------------
//  mat                    3b)
//--------------------------
//double tmp_b = sign_phi_0*sign_phi_0 /grad_norm_dist_npi;
double tmp_b = 1.0 /(grad_norm_dist_npi*grad_norm_dist_npi);

LINALG::Matrix<nen_,1> second_psi_Mult_grad_Dpsi;
second_psi_Mult_grad_Dpsi.Clear();

for (int i= 0; i< nen_; i++)
{
	second_psi_Mult_grad_Dpsi(i,0) = derxy_(0,i)* (second_dist_npi(0,0)+second_dist_npi(3,0)+second_dist_npi(4,0))
								   + derxy_(1,i)* (second_dist_npi(3,0)+second_dist_npi(1,0)+second_dist_npi(5,0))
								   + derxy_(2,i)* (second_dist_npi(4,0)+second_dist_npi(5,0)+second_dist_npi(2,0));
}

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) -= funct_(vi)*pseudo_timestep_size*pseudo_timestep_size*fac/4.0*tmp_b*second_psi_Mult_grad_Dpsi(ui,0);
  }
}



//double tmp_c3 = sign_phi_0*sign_phi_0/ (grad_norm_dist_npi*grad_norm_dist_npi);
double tmp_c3 = 1.0/ (grad_norm_dist_npi*grad_norm_dist_npi);

//double tmp_c3 = -1.0/ (pow(grad_norm_dist_npi,2.0));
LINALG::Matrix<1,nen_> GradPsi_DERIV2_GradPsi;
GradPsi_DERIV2_GradPsi.Clear();
for (int i=0; i< nen_; i++)
{
	GradPsi_DERIV2_GradPsi(0,i) = (derxy2_(0,i)* grad_dist_npi(0))
						   +	  (derxy2_(1,i)* grad_dist_npi(1))
						   +	  (derxy2_(2,i)* grad_dist_npi(2))
						   +      (derxy2_(3,i)* (grad_dist_npi(0) + grad_dist_npi(1)))
						   +      (derxy2_(4,i)* (grad_dist_npi(0) + grad_dist_npi(2)))
						   +      (derxy2_(5,i)* (grad_dist_npi(1) + grad_dist_npi(2)));
}
//cout << GradPsi_DERIV2_GradPsi << endl;

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) -= funct_(vi)*pseudo_timestep_size*pseudo_timestep_size*fac/4.0*tmp_c3 * GradPsi_DERIV2_GradPsi(0,ui);
  }
}



//----------  --------------                |                m                  m                                m     |
//  rhs                    + 1.0/4.0*dtau^2 | w,so^2/(grad(psi   ))^2  d/dxk psi   * sum(j=1..nsd)(d^2/dxjdxk psi    ) |
//--------------------------                |                                                                          |

if(numderiv2_ != 6) dserror("this is not a 3D case!!!");

double second_deriv_tmp_n = 0.0;
second_deriv_tmp_n   = second_dist_n(0,0) *grad_dist_n(0,0)
		             + second_dist_n(1,0) *grad_dist_n(1,0)
		             + second_dist_n(2,0) *grad_dist_n(2,0)
		             + second_dist_n(3,0) *(grad_dist_n(0,0) +grad_dist_n(1,0))
		             + second_dist_n(4,0) *(grad_dist_n(0,0) +grad_dist_n(2,0))
		             + second_dist_n(5,0) *(grad_dist_n(1,0) +grad_dist_n(2,0));
//second_deriv_tmp_n /= (grad_norm_dist_n*grad_norm_dist_n);

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] += funct_(vi)*sign_phi_0*sign_phi_0* second_deriv_tmp_n* pseudo_timestep_size*pseudo_timestep_size*fac/(4.0*grad_norm_dist_n*grad_norm_dist_n);
//  erhs[fvi] += funct_(vi)*1.0* second_deriv_tmp_n* pseudo_timestep_size*pseudo_timestep_size*fac/(4.0*grad_norm_dist_n*grad_norm_dist_n);

}
//cout << "second_deriv_tmp_n" << second_deriv_tmp_n << endl;

} // end if grad_norm_zero==false
// END Assemble domain 2nd order integrals!!!
#endif


#ifdef FIXPOINTLIKE


//cout << "second_dist_npi" << second_dist_npi << endl;
double second_deriv_tmp_npi = 0.0;
second_deriv_tmp_npi = second_dist_npi(0,0) *grad_dist_npi(0,0)
		             + second_dist_npi(1,0) *grad_dist_npi(1,0)
		             + second_dist_npi(2,0) *grad_dist_npi(2,0)
		             + second_dist_npi(3,0) *(grad_dist_npi(0,0) +grad_dist_npi(1,0))
		             + second_dist_npi(4,0) *(grad_dist_npi(0,0) +grad_dist_npi(2,0))
		             + second_dist_npi(5,0) *(grad_dist_npi(1,0) +grad_dist_npi(2,0));
//cout << "second_deriv_tmp_npi_1" << second_deriv_tmp_npi;
//if(fabs(second_dist_npi(0,0))>1e-12 or
//   fabs(second_dist_npi(1,0))>1e-12 or
//   fabs(second_dist_npi(2,0))>1e-12) cout << second_dist_npi << endl;

//cout << sign_phi_0*sign_phi_0* second_deriv_tmp_npi* pseudo_timestep_size*pseudo_timestep_size*fac/4.0 << endl;
bool do_assembly = true;
//if(grad_norm_dist_npi< 1e-1){
////	cout << "warning: grad_norm_dist_npi is near zero!!! in element" << ele->Id() << grad_norm_dist_npi << endl;
////	cout << grad_dist_npi << endl;
////	cout << ephinp_[dofindex] << endl;
//	do_assembly=false;
//}
//
if(grad_norm_dist_n< 1e-1){
	cout << "warning: grad_norm_dist_n is near zero!!!" << ele->Id() << grad_norm_dist_n << endl;
	do_assembly=false;
}

if(do_assembly==true)
{
for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

//  erhs[fvi] += funct_(vi)*sign_phi_0*sign_phi_0* second_deriv_tmp_npi* pseudo_timestep_size*pseudo_timestep_size*fac/(4.0*grad_norm_dist_npi*grad_norm_dist_npi);
  erhs[fvi] += funct_(vi)*1.0* second_deriv_tmp_npi* pseudo_timestep_size*pseudo_timestep_size*fac/(4.0*grad_norm_dist_npi*grad_norm_dist_npi);

}
}


double second_deriv_tmp_n = 0.0;
second_deriv_tmp_n   = second_dist_n(0,0) *grad_dist_n(0,0)
		             + second_dist_n(1,0) *grad_dist_n(1,0)
		             + second_dist_n(2,0) *grad_dist_n(2,0)
		             + second_dist_n(3,0) *(grad_dist_n(0,0) +grad_dist_n(1,0))
		             + second_dist_n(4,0) *(grad_dist_n(0,0) +grad_dist_n(2,0))
		             + second_dist_n(5,0) *(grad_dist_n(1,0) +grad_dist_n(2,0));
//second_deriv_tmp_n /= (grad_norm_dist_n*grad_norm_dist_n);

if(do_assembly==true)
{
for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

//  erhs[fvi] += funct_(vi)*sign_phi_0*sign_phi_0* second_deriv_tmp_n* pseudo_timestep_size*pseudo_timestep_size*fac/(4.0*grad_norm_dist_n*grad_norm_dist_n);
  erhs[fvi] += funct_(vi)*1.0* second_deriv_tmp_n* pseudo_timestep_size*pseudo_timestep_size*fac/(4.0*grad_norm_dist_n*grad_norm_dist_n);

}
}
#endif



// Assemble rhs for linear part of weak formulation for nonlinear iteration


// get difference between psi^m+1 - psi^m
double Delta_Psi = dist_npi - dist_n;
//double Delta_Psi = dist_npi;

LINALG::Matrix<nsd_,1> Delta_Grad_psi;
Delta_Grad_psi.Clear();
Delta_Grad_psi.Update(1.0,grad_dist_npi, -1.0, grad_dist_n);


//     |         m+1     m  |
//     | -w, (psi   - psi ) |
//     |         i          |


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= funct_(vi)*fac*Delta_Psi;
}


//                    |                   m+1     m  |
//    1/4*delta_tau^2 | -grad(w), grad(psi   - psi ) |
//                    |                   i          |

LINALG::Matrix<nen_,1> Grad_w_Grad_Dpsi;
Grad_w_Grad_Dpsi.Clear();
Grad_w_Grad_Dpsi.MultiplyTN(derxy_,Delta_Grad_psi);


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= Grad_w_Grad_Dpsi(vi,0)*fac*pseudo_timestep_size*pseudo_timestep_size/4.0;
}


//============================================================================================================================
#endif


	return;
} //ScaTraImpl::CalMatAndRHS_Characteristic_Galerkin_REINITIALIZATION


/*-------------------------------------------------------------------------------*
 |  evaluate element matrix and rhs for reinitialization (private)   schott 12/10|
 *-------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalMatAndRHS_PENALTY_REINITIALIZATION_assemble(
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const int                             dofindex,
    DRT::Element*                         ele,
    const double                          penalty_interface_reinit,
    LINALG::Matrix<3,1>&                  intersection_local
    )
{

	// evaluate the shape functions at the intersection point
	LINALG::Matrix<nen_,1> funct_intersection;
	funct_intersection.Clear();
	DRT::UTILS::shape_function_3D(funct_intersection,intersection_local(0),intersection_local(1),intersection_local(2),distype);

	// assemble shape functions in sysmat
	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  for (int ui=0; ui<nen_; ++ui)
	  {
		const int fui = ui*numdofpernode_+dofindex;

		emat(fvi,fui) += funct_intersection(ui)*funct_intersection(vi)*penalty_interface_reinit;
	  }
	}

#ifdef NONLINEAR

	LINALG::Matrix<1,1> distnpi_intersection(true);

	distnpi_intersection.MultiplyTN(funct_intersection,ephinp_[dofindex]);

	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  erhs[fvi] -= penalty_interface_reinit*funct_intersection(vi,0)*distnpi_intersection(0,0);
	}
#endif

	return;
}

/*-------------------------------------------------------------------------------*
 |  evaluate element matrix and rhs for reinitialization (private)   schott 12/10|
 *-------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalMatAndRHS_PENALTY_REINITIALIZATION(
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const int                             dofindex,
    DRT::Element*                         ele,
    const double                          penalty_interface_reinit
    )
{

	//=======================================================================================================
	// get intersection point for each edge of the element
	// loop over edges
	//{
		// for each edge: check the G-function values, if there is an intersection point
		// if there is an intersection point: get the right local coordinates (interpolation of the local coordinates of the element)
		// evaluate the shape functions at this point
		// multiply the shape function with the phi-values
		// assemble G^T*G*phi = 0 into the sysmat
	//}
	//=======================================================================================================

	int counter = 0;

	const size_t numnode = ele->NumNode();

	vector<RCP<DRT::Element> > linesVec = ele->Lines();
	typedef vector<RCP<DRT::Element> >::iterator lines_iterator;

	for(lines_iterator line = linesVec.begin(); line != linesVec.end(); line++)
	{
		if(distype==DRT::Element::hex8)
		{
			RCP<DRT::Element> pt_to_line = *line;
			const DRT::Node* const* vecOfPtsToNode = pt_to_line->Nodes();

			const int numberOfNodesOfLine = pt_to_line->NumNode();

			if (numberOfNodesOfLine != 2) dserror("not exact 2 nodes on this line");

			// get phi_value of current node
			int nodeID_start = vecOfPtsToNode[0]->Id();
			int nodeID_end   = vecOfPtsToNode[1]->Id();

			const int* ptToNodeIds_adj = ele->NodeIds();

			int ID_param_space_start = -1;
			int ID_param_space_end   = -1;

			for (size_t inode=0; inode < numnode; inode++)
			{
				 // get local number of node actnode in ele_adj
				 if(nodeID_start == ptToNodeIds_adj[inode]) ID_param_space_start = inode;
				 if(nodeID_end   == ptToNodeIds_adj[inode]) ID_param_space_end   = inode;
			}

			if(ID_param_space_start == -1) dserror("node of line not a node of element!?!?!?");
			if(ID_param_space_end   == -1) dserror("node of line not a node of element!?!?!?");


			// get node xi coordinates
			LINALG::Matrix<3,1> node_Xicoordinates_start;
			node_Xicoordinates_start.Clear();
			node_Xicoordinates_start = DRT::UTILS::getNodeCoordinates(ID_param_space_start, distype);

			LINALG::Matrix<3,1> node_Xicoordinates_end;
			node_Xicoordinates_end.Clear();
			node_Xicoordinates_end = DRT::UTILS::getNodeCoordinates(ID_param_space_end, distype);


			// get the intersection point (linear interpolation)
			double interp_alpha = 0.0;
			double phi_end   = ephi0_penalty_[dofindex](ID_param_space_end);
			double phi_start = ephi0_penalty_[dofindex](ID_param_space_start);

			// if an intersection point is given
			if (phi_end * phi_start <= 0.0)
			{

				double phi_diff = phi_start-phi_end;
				if(fabs(phi_diff)< 1e-12)
				{
					// maybe a complete edge is zero -> do nothing for this element
					cout << "!!! WARNING: one element edge is zero in element " << ele->Id() << "-> check this penalty case!!! (do nothing at the moment)" << endl;
					return;
				}

				interp_alpha = -phi_end/(phi_diff);

				LINALG::Matrix<3,1> intersection_local;
				intersection_local.Clear();
				intersection_local.Update(interp_alpha,node_Xicoordinates_start, (1.0-interp_alpha), node_Xicoordinates_end);


				// evaluate the shape functions at the intersection point
				LINALG::Matrix<nen_,1> funct_intersection;
				funct_intersection.Clear();
				DRT::UTILS::shape_function_3D(funct_intersection,intersection_local(0),intersection_local(1),intersection_local(2),distype);

				// assemble shape functions in sysmat
				for (int vi=0; vi<nen_; ++vi)
				{
				  const int fvi = vi*numdofpernode_+dofindex;

				  for (int ui=0; ui<nen_; ++ui)
				  {
					const int fui = ui*numdofpernode_+dofindex;

					emat(fvi,fui) += funct_intersection(ui)*funct_intersection(vi)*penalty_interface_reinit;
				  }
				}

		#ifdef NONLINEAR

				LINALG::Matrix<1,1> distnpi_intersection(true);

				distnpi_intersection.MultiplyTN(funct_intersection,ephinp_[dofindex]);

				for (int vi=0; vi<nen_; ++vi)
				{
				  const int fvi = vi*numdofpernode_+dofindex;

				  erhs[fvi] -= penalty_interface_reinit*funct_intersection(vi,0)*distnpi_intersection(0,0);
				}
		#endif
			} // end if intersection
		} // end of hex8
		else if(distype==DRT::Element::hex20)
		{
			counter++;
			RCP<DRT::Element> pt_to_line = *line;
			const DRT::Node* const* vecOfPtsToNode = pt_to_line->Nodes();

			const int numberOfNodesOfLine = pt_to_line->NumNode();

			if (numberOfNodesOfLine != 3) dserror("not exact 3 nodes on this line");

			// get phi_value of current node
			int nodeID_start = vecOfPtsToNode[0]->Id();
			int nodeID_end   = vecOfPtsToNode[1]->Id();
			int nodeID_mid   = vecOfPtsToNode[2]->Id();

			const int* ptToNodeIds_adj = ele->NodeIds();

			int ID_param_space_start = -1;
			int ID_param_space_end   = -1;
			int ID_param_space_mid   = -1;

			for (size_t inode=0; inode < numnode; inode++)
			{
				 // get local number of node actnode in ele_adj
				 if(nodeID_start == ptToNodeIds_adj[inode]) ID_param_space_start = inode;
				 if(nodeID_end   == ptToNodeIds_adj[inode]) ID_param_space_end   = inode;
				 if(nodeID_mid   == ptToNodeIds_adj[inode]) ID_param_space_mid   = inode;
			}

			if(ID_param_space_start == -1) dserror("node of line not a node of element!?!?!?");
			if(ID_param_space_end   == -1) dserror("node of line not a node of element!?!?!?");
			if(ID_param_space_mid   == -1) dserror("node of line not a node of element!?!?!?");


			// get node xi coordinates
			LINALG::Matrix<3,1> node_Xicoordinates_start;
			node_Xicoordinates_start.Clear();
			node_Xicoordinates_start = DRT::UTILS::getNodeCoordinates(ID_param_space_start, distype);

			LINALG::Matrix<3,1> node_Xicoordinates_end;
			node_Xicoordinates_end.Clear();
			node_Xicoordinates_end = DRT::UTILS::getNodeCoordinates(ID_param_space_end, distype);

			LINALG::Matrix<3,1> node_Xicoordinates_mid;
			node_Xicoordinates_mid.Clear();
			node_Xicoordinates_mid = DRT::UTILS::getNodeCoordinates(ID_param_space_mid, distype);


			// get the intersection point (linear interpolation)
			double phi_end   = ephi0_Reinit_Reference_[dofindex](ID_param_space_end);
			double phi_start = ephi0_Reinit_Reference_[dofindex](ID_param_space_start);
			double phi_mid   = ephi0_Reinit_Reference_[dofindex](ID_param_space_mid);


			// get intersection or not (roots of quadratic function a*xi^2+b*xi+c=0)
			double a = 0.5*(phi_start + phi_end)-phi_mid;
			double b = 0.5*(phi_end - phi_start);
			double c = phi_mid;

			bool intersection_linear = false;
			bool intersection_1_quadratic =false;
			bool intersection_2_quadratic =false;

			double interp_start = 0.0;
			double interp_end = 0.0;
			double interp_mid = 0.0;

			LINALG::Matrix<3,1> intersection_local;
			intersection_local.Clear();

			if(fabs(a)<1e-8) //linear case -> linear intersection possible (1e-10 is too small!!!)
			{
				if(phi_start*phi_end < 0.0) //intersection
				{
					intersection_linear = true;

					//	dserror("this is not a real quadratic function");
					double phi_diff = phi_start-phi_end;
					if(fabs(phi_diff)< 1e-12)
					{
						// maybe a complete edge is zero -> do nothing for this element
						cout << "!!! WARNING: one element edge is zero in element " << ele->Id() << "-> check this penalty case!!! (do nothing at the moment)" << endl;
						return;
					}

					double interp_alpha = -phi_end/(phi_diff);

					intersection_local.Update(interp_alpha,node_Xicoordinates_start, (1.0-interp_alpha), node_Xicoordinates_end);

					CalMatAndRHS_PENALTY_REINITIALIZATION_assemble(emat,erhs,dofindex,ele,penalty_interface_reinit,intersection_local);
				}
			}
			else // quadratic case
			{
			// determine diskriminante
			double D = b*b - 4.0*a*c;
			if( D > 1e-13){
				// determine intersection points in local coordinates
				// get square root
				double sqrt_D = sqrt(D);
				double inters_1 = (-1.0*b + sqrt_D)/(2*a);
				double inters_2 = (-1.0*b - sqrt_D)/(2*a);

				// check whether the intersection points lie in [-1,1]
				if(fabs(inters_1)< 1.0) intersection_1_quadratic=true;
				if(fabs(inters_2)< 1.0) intersection_2_quadratic=true;
				if(intersection_1_quadratic && intersection_2_quadratic)
				{
					cout << "element:" << ele->Id() << endl;
					cout << "D" << D << endl;
					cout << "phi_start" << phi_start << endl;
					cout << "phi_mid" << phi_mid << endl;
					cout << "phi_end" << phi_end << endl;
					cout << "intersection_1" << inters_1 << endl;
					cout << "intersection_2" << inters_2 << endl;
					cout << "two real roots in [-1,1]-> two intersection points on one line -> check this case" << endl;
				}

				// check this case!!!

//				if(intersection_1_quadratic)
//				{
//					//get intersection_local
//					CalMatAndRHS_PENALTY_REINITIALIZATION_assemble(emat,erhs,dofindex,ele,penalty_interface_reinit,intersection_local);
//				}
//				if(intersection_2_quadratic)
//				{
//					//get intersection_local
//					CalMatAndRHS_PENALTY_REINITIALIZATION_assemble(emat,erhs,dofindex,ele,penalty_interface_reinit,intersection_local);
//				}
			}
			else if (D< -1e-13)
			{
				intersection_1_quadratic = false;
				intersection_2_quadratic = false;
			}
			else
			{
				double inters_1 = (-1.0*b)/(2*a);
				if(fabs(inters_1)< 1.0) intersection_1_quadratic=true;

				intersection_2_quadratic = false;

				if(intersection_1_quadratic)
				{
					double tmp_a  = b/(2.0*a);
					double tmp_b  = b/(4.0*a);
//					double tmp_b = b/(4.0*a);

					interp_start = tmp_b*(1.0+tmp_a);
					interp_end   = tmp_b*(tmp_a-1.0);
					interp_mid   = 1.0-tmp_a*tmp_a;

					intersection_local.Update(interp_start,node_Xicoordinates_start, interp_end, node_Xicoordinates_end);
					intersection_local.Update(interp_mid, node_Xicoordinates_mid, 1.0);

					cout << "ele-Id " << ele->Id() << endl;
					cout << "a " << a << endl;
					cout << "nodeXi_start " << node_Xicoordinates_start << endl;
					cout << "nodeXi_mid "   << node_Xicoordinates_mid << endl;
					cout << "nodeXi_end "   << node_Xicoordinates_end << endl;
					cout << "inters_local " << intersection_local       << endl;
					cout << "phi_start " << phi_start << endl;
					cout << "phi_mid " << phi_mid << endl;
					cout << "phi_end " << phi_end << endl;

					CalMatAndRHS_PENALTY_REINITIALIZATION_assemble(emat,erhs,dofindex,ele,penalty_interface_reinit,intersection_local);
				}

			}

			}


		}//end of hex20
		else dserror("penalty not implemented for this type of element");
	} // end loop over lines

	return;
}



/*---------------------------------------------------------------------*
 |  calculate error for reinitialization                   schott 12/12|
 *---------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalErrorsReinitialization(
    const DRT::Element*                   ele,
    ParameterList&                        params
)
{
	  //==================================== REINITIALIZATION error calculation  ========================
	  // gradient norm of phi || grad(phi) -1.0 ||_L1(Omega)                                 schott 12/10
	  //=================================================================================================

	  // get element params
	  double eleL1gradienterr 	= params.get<double>("L1 integrated gradient error"); //squared errors
	  double elevolume 			= params.get<double>("volume");                       // non-squared volume

//	  cout << "elevol before"<< elevolume << endl;
	  int dofindex = 0; // we assume only one scalar

	  // get Gaussian points for integrated L2-norm and volume calculation
	  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints_reinit(SCATRA::DisTypeToGaussRuleForExactSol<distype>::rule);

	  // caculate element wise errors and volume
	  for (int iquad=0; iquad<intpoints_reinit.IP().nquad; ++iquad)
	  {
		const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints_reinit,iquad,ele->Id());

#ifdef L1_NORM_TRANSITION_REGION
		const double mesh_size = getEleDiameter<distype>(xyze_);

		double phi_ref = funct_.Dot(ephinp_[dofindex]);
		double deriv_smoothed_Heavyside = EvaluateDerivSmoothedHeavySide(phi_ref, 3,mesh_size, INPAR::SCATRA::signtype_Nagrath2005);
#endif

		LINALG::Matrix<nsd_,1> gradphi;
		  gradphi.Clear();
		  gradphi.Multiply(derxy_,ephinp_[dofindex]);
		  double gradphi_norm = gradphi.Norm2();
		  double gradphi_norm_err = gradphi_norm - 1.0;

		  // integrate L21-error ( || grad(phi)-1.0 || )_L1(Omega_ele)

#ifdef L1_NORM_TRANSITION_REGION
		  eleL1gradienterr += fabs(gradphi_norm_err) * fac* deriv_smoothed_Heavyside;
		  // integrate volume
		  elevolume        += fac * deriv_smoothed_Heavyside;
#else
		  eleL1gradienterr += fabs(gradphi_norm_err) * fac;

		  // integrate volume
		  elevolume        += fac;
#endif
	  } // Gaussian points for correction factor

	  // set new element params
	  params.set<double>("L1 integrated gradient error", eleL1gradienterr);
	  params.set<double>("volume", elevolume);


	return;
}



/*----------------------------------------------------------------------*
 | evaluate shape functions and derivatives at int. point  schott 01/11 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
double DRT::ELEMENTS::ScaTraImpl<distype>::EvalShapeFuncAndDerivsAtIntPointReinitialization(
    const DRT::UTILS::IntPointsAndWeights<nsd_>& intpoints,  ///< integration points
    const int                                    iquad,      ///< id of current Gauss point
    const int                                    eleid       ///< the element id
)
{
  // coordinates of the current integration point
  const double* gpcoord = (intpoints.IP().qxg)[iquad];
  for (int idim=0;idim<nsd_;idim++)
    {xsi_(idim) = gpcoord[idim];}

  if (not DRT::NURBS::IsNurbs(distype))
  {
    // shape functions and their first derivatives
    DRT::UTILS::shape_function<distype>(xsi_,funct_);
    DRT::UTILS::shape_function_deriv1<distype>(xsi_,deriv_);
    if (use2ndderivReinitialization_)
    {
      // get the second derivatives of standard element at current GP
      DRT::UTILS::shape_function_deriv2<distype>(xsi_,deriv2_);
    }
  }
  else // nurbs elements are always somewhat special...
  {
    if (use2ndderivReinitialization_)
    {
      DRT::NURBS::UTILS::nurbs_get_funct_deriv_deriv2
      (funct_  ,
          deriv_  ,
          deriv2_ ,
          xsi_    ,
          myknots_,
          weights_,
          distype );
    }
    else
    {
      DRT::NURBS::UTILS::nurbs_get_funct_deriv
      (funct_  ,
          deriv_  ,
          xsi_    ,
          myknots_,
          weights_,
          distype );
    }
  } // IsNurbs()

  // compute Jacobian matrix and determinant
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

  xjm_.MultiplyNT(deriv_,xyze_);
  const double det = xij_.Invert(xjm_);

  if (det < 1E-16)
    dserror("GLOBAL ELEMENT NO.%i\nZERO OR NEGATIVE JACOBIAN DETERMINANT: %f", eleid, det);

  // set integration factor: fac = Gauss weight * det(J)
  const double fac = intpoints.IP().qwgt[iquad]*det;

  // compute global derivatives
  derxy_.Multiply(xij_,deriv_);

  // compute second global derivatives (if needed)
  if (use2ndderivReinitialization_)
  {
    // get global second derivatives
    DRT::UTILS::gder2<distype>(xjm_,derxy_,deriv2_,xyze_,derxy2_);
  }
  else
    derxy2_.Clear();

  // return integration factor for current GP: fac = Gauss weight * det(J)
  return fac;

}


/*-------------------------------------------------------------------------------*
 |  evaluate element matrix and rhs for transport equation with TG2   schott 12/10|
 *-------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalMatAndRHS_TG2(
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const double                          fac,
    const int                             dofindex,
    DRT::Element*                         ele,
    const double                          dt
    )
{

//==========================================================
// evaluate element vectors and gradients
//==========================================================
// get element vectors
// dist_npi   distance vector for reinitialization at current timestep np and old increment i
// dist_n     distance vector for reinitialization at old timestep n
// phi_0      reference phi vector for smoothed sign function -> needed for directed transport along characteristics
const double dist_n   = funct_.Dot(ephin_[dofindex]);   // d^n
const double dist_npi = funct_.Dot(ephinp_[dofindex]);  // d^(n+1)_i


// get gradients and norms
// dist_npi   distance vector for reinitialization at current timestep np and old increment i
// dist_n     distance vector for reinitialization at old timestep n
// phi_0      reference phi vector for smoothed sign function -> needed for directed transport along characteristics

LINALG::Matrix<nsd_,1> grad_dist_n;
grad_dist_n.Clear();
grad_dist_n.Multiply(derxy_,ephin_[dofindex]);

LINALG::Matrix<nsd_,1> grad_dist_npi;
grad_dist_npi.Clear();
grad_dist_npi.Multiply(derxy_,ephinp_[dofindex]);


//double grad_norm_dist_n   = grad_dist_n.Norm2();
//double grad_norm_dist_npi = grad_dist_npi.Norm2();


//----------------------------------------------------------------
// standard Galerkin transient term
//----------------------------------------------------------------

//     |           |
//     | w, D(psi) |
//     |           |

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) += funct_(vi)*fac*funct_(ui);
  }
}


//               |                           |
//   1/4   dt^2  | u*grad(v), u*grad(D(psi)) |
//               |                           |

LINALG::Matrix<nen_,1> uGradDphi;
uGradDphi.Clear();

uGradDphi.MultiplyTN(derxy_,velint_);


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) += uGradDphi(vi,0)*uGradDphi(ui,0)* (fac*dt*dt/4.0);
  }
}


//----------  --------------    |          n+1     n    |
//  rhs                       - | w, u*(phi   - phi  )  |
//--------------------------    |          i            |

//cout << dist_npi-dist_n << endl;

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= funct_(vi)*fac*(dist_npi-dist_n);
}


//----------  --------------                |              n   |
//  rhs                                 -dt | w, u*grad(phi )  |
//--------------------------                |                  |

LINALG::Matrix<1,1> uGradPhi;
uGradPhi.Clear();
uGradPhi.MultiplyTN(velint_,grad_dist_n);

//cout << uGradPhi << endl;
//exit(1);

//cout << "grad_dist_n" << grad_dist_n << endl;

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= funct_(vi)*dt*fac*uGradPhi(0,0);
}



//               |                       n+1     n  |
//    -dt*dt/4.0 |  grad(w)*u, u*grad(psi   + psi ) |
//               |                       i          |


LINALG::Matrix<nsd_,1> sum_phi;
sum_phi.Clear();
sum_phi.Update(1.0,grad_dist_npi, 1.0, grad_dist_n);

LINALG::Matrix<1,1> uGradSumPhi;
uGradSumPhi.Clear();
uGradSumPhi.MultiplyTN(velint_,sum_phi);

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= uGradDphi(vi,0)*dt*dt/4.0*fac*uGradSumPhi(0,0);
}


	return;
} //ScaTraImpl::CalMatAndRHS_TG2



/*-------------------------------------------------------------------------------*
 |  evaluate element matrix and rhs for transport equation with TG3  schott 12/10|
 *-------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalMatAndRHS_TG2_LW(
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const double                          fac,
    const int                             dofindex,
    DRT::Element*                         ele,
    const double                          dt
    )
{

//==========================================================
// evaluate element vectors and gradients
//==========================================================
// get element vectors
// dist_npi   distance vector for reinitialization at current timestep np and old increment i
// dist_n     distance vector for reinitialization at old timestep n
// phi_0      reference phi vector for smoothed sign function -> needed for directed transport along characteristics
const double dist_n   = funct_.Dot(ephin_[dofindex]);   // d^n
const double dist_npi = funct_.Dot(ephinp_[dofindex]);  // d^(n+1)_i


// get gradients and norms
// dist_npi   distance vector for reinitialization at current timestep np and old increment i
// dist_n     distance vector for reinitialization at old timestep n
// phi_0      reference phi vector for smoothed sign function -> needed for directed transport along characteristics

LINALG::Matrix<nsd_,1> grad_dist_n;
grad_dist_n.Clear();
grad_dist_n.Multiply(derxy_,ephin_[dofindex]);

LINALG::Matrix<nsd_,1> grad_dist_npi;
grad_dist_npi.Clear();
grad_dist_npi.Multiply(derxy_,ephinp_[dofindex]);


//double grad_norm_dist_n   = grad_dist_n.Norm2();
//double grad_norm_dist_npi = grad_dist_npi.Norm2();


//----------------------------------------------------------------
// standard Galerkin transient term
//----------------------------------------------------------------

//     |           |
//     | w, D(phi) |
//     |           |

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) += funct_(vi)*fac*funct_(ui);
  }
}

// a*grad(w) bzw. a*grad(D(phi))
LINALG::Matrix<nen_,1> aGradD(true);
aGradD.MultiplyTN(derxy_,velint_);

// a*grad(phi_n)
double a_phi_n = velint_.Dot(grad_dist_n);

// a*grad(phi_npi)
//double a_phi_npi = velint_.Dot(grad_dist_npi);

//----------  --------------    |                n    |
//  rhs                     +dt | a*grad(w) , phi     |
//--------------------------    |                     |


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] += dt*fac*aGradD(vi)*dist_n;
}

//----------  --------------    |        n+1     n|
//  rhs                       - | w , phi    -phi |
//--------------------------    |        i        |


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= fac*funct_(vi)*(dist_npi-dist_n);
}


//----------  --------------                |                      n   |
//  rhs                         -1/2*dt*dt* | a*grad(w), a*grad(phi )  |
//--------------------------                |                          |


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= aGradD(vi)*dt*dt*0.5*fac*a_phi_n;
}


	return;
} //ScaTraImpl::CalMatAndRHS_TG2_LW


/*-------------------------------------------------------------------------------*
 |  evaluate element matrix and rhs for transport equation with TG3  schott 12/10|
 *-------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalMatAndRHS_TG3(
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const double                          fac,
    const int                             dofindex,
    DRT::Element*                         ele,
    const double                          dt
    )
{

	//==========================================================
	// evaluate element vectors and gradients
	//==========================================================
	// get element vectors
	// dist_npi   distance vector for reinitialization at current timestep np and old increment i
	// dist_n     distance vector for reinitialization at old timestep n
	// phi_0      reference phi vector for smoothed sign function -> needed for directed transport along characteristics
	const double dist_n   = funct_.Dot(ephin_[dofindex]);   // d^n
	const double dist_npi = funct_.Dot(ephinp_[dofindex]);  // d^(n+1)_i


	// get gradients and norms
	// dist_npi   distance vector for reinitialization at current timestep np and old increment i
	// dist_n     distance vector for reinitialization at old timestep n
	// phi_0      reference phi vector for smoothed sign function -> needed for directed transport along characteristics

	LINALG::Matrix<nsd_,1> grad_dist_n;
	grad_dist_n.Clear();
	grad_dist_n.Multiply(derxy_,ephin_[dofindex]);

	LINALG::Matrix<nsd_,1> grad_dist_npi;
	grad_dist_npi.Clear();
	grad_dist_npi.Multiply(derxy_,ephinp_[dofindex]);


	//double grad_norm_dist_n   = grad_dist_n.Norm2();
	//double grad_norm_dist_npi = grad_dist_npi.Norm2();


	//----------------------------------------------------------------
	// standard Galerkin transient term
	//----------------------------------------------------------------

	//     |           |
	//     | w, D(phi) |
	//     |           |

	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  for (int ui=0; ui<nen_; ++ui)
	  {
	    const int fui = ui*numdofpernode_+dofindex;

	    emat(fvi,fui) += funct_(vi)*fac*funct_(ui);
	  }
	}


	//               |                           |
	//    1/6 *dt^2  | a*grad(w), a*grad(D(phi)) |
	//               |                           |

	// a*grad(w) bzw. a*grad(D(phi))
	LINALG::Matrix<nen_,1> aGradD(true);
	aGradD.MultiplyTN(derxy_,velint_);

	// a*grad(phi_n)
	double a_phi_n = velint_.Dot(grad_dist_n);

	// a*grad(phi_npi)
	double a_phi_npi = velint_.Dot(grad_dist_npi);


	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  for (int ui=0; ui<nen_; ++ui)
	  {
	    const int fui = ui*numdofpernode_+dofindex;

	    emat(fvi,fui) += aGradD(vi)*aGradD(ui)* (fac*dt*dt/6.0);
	  }
	}


	//----------  --------------    |                n    |
	//  rhs                     +dt | a*grad(w) , phi     |
	//--------------------------    |                     |


	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  erhs[fvi] += dt*fac*aGradD(vi)*dist_n;
	}

	//----------  --------------    |        n+1     n|
	//  rhs                       - | w , phi    -phi |
	//--------------------------    |        i        |


	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  erhs[fvi] -= fac*funct_(vi)*(dist_npi-dist_n);
	}


	//----------  --------------                |                      n   |
	//  rhs                         -1/2*dt*dt* | a*grad(w), a*grad(phi )  |
	//--------------------------                |                          |


	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  erhs[fvi] -= aGradD(vi)*dt*dt*0.5*fac*a_phi_n;
	}

	//----------  --------------                |                      n+1    n    |
	//  rhs                         -1/6*dt*dt* | a*grad(w), a*grad(phi   -phi  )  |
	//--------------------------                |                      i           |


	for (int vi=0; vi<nen_; ++vi)
	{
	  const int fvi = vi*numdofpernode_+dofindex;

	  erhs[fvi] -= aGradD(vi)*dt*dt/6.0*fac*(a_phi_npi-a_phi_n);
	}


		return;
} //ScaTraImpl::CalMatAndRHS_TG3


/*-------------------------------------------------------------------------------*
 |  evaluate element matrix and rhs for transport equation with TG3  schott 12/10|
 *-------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalMatAndRHS_TG4_1S(
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const double                          fac,
    const int                             dofindex,
    DRT::Element*                         ele,
    const double                          dt
    )
{

//==========================================================
// evaluate element vectors and gradients
//==========================================================
// get element vectors
// dist_npi   distance vector for reinitialization at current timestep np and old increment i
// dist_n     distance vector for reinitialization at old timestep n
// phi_0      reference phi vector for smoothed sign function -> needed for directed transport along characteristics
const double dist_n   = funct_.Dot(ephin_[dofindex]);   // d^n
const double dist_npi = funct_.Dot(ephinp_[dofindex]);  // d^(n+1)_i


// get gradients and norms
// dist_npi   distance vector for reinitialization at current timestep np and old increment i
// dist_n     distance vector for reinitialization at old timestep n
// phi_0      reference phi vector for smoothed sign function -> needed for directed transport along characteristics

LINALG::Matrix<nsd_,1> grad_dist_n;
grad_dist_n.Clear();
grad_dist_n.Multiply(derxy_,ephin_[dofindex]);

LINALG::Matrix<nsd_,1> grad_dist_npi;
grad_dist_npi.Clear();
grad_dist_npi.Multiply(derxy_,ephinp_[dofindex]);


//double grad_norm_dist_n   = grad_dist_n.Norm2();
//double grad_norm_dist_npi = grad_dist_npi.Norm2();


//----------------------------------------------------------------
// standard Galerkin transient term
//----------------------------------------------------------------

//     |           |
//     | w, D(phi) |
//     |           |

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) += funct_(vi)*fac*funct_(ui);
  }
}


//               |                   |
//      1/2 *dt  | w, a*grad(D(phi)) |
//               |                   |

// a*grad(w) bzw. a*grad(D(phi))
LINALG::Matrix<nen_,1> aGradD(true);
aGradD.MultiplyTN(derxy_,velint_);


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) += funct_(vi)*aGradD(ui)* (fac*dt/2.0);
  }
}



//               |                           |
//  -1/12 *dt^2  | a*grad(w), a*grad(D(phi)) |
//               |                           |


// a*grad(phi_n)
double a_phi_n = velint_.Dot(grad_dist_n);

// a*grad(phi_npi)
double a_phi_npi = velint_.Dot(grad_dist_npi);


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) -= aGradD(vi)*aGradD(ui)* (fac*dt*dt/12.0);
  }
}


//----------  --------------    |               n+1     n  |
//  rhs                 -1/2*dt | w , a*grad(phi   + phi ) |
//--------------------------    |               i          |


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= 0.5*dt*fac*funct_(vi)*(a_phi_npi + a_phi_n);
}

//----------  --------------    |        n+1     n|
//  rhs                       - | w , phi    -phi |
//--------------------------    |        i        |


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= fac*funct_(vi)*(dist_npi-dist_n);
}


//----------  --------------                |                      n+1   n   |
//  rhs                        +1/12*dt*dt* | a*grad(w), a*grad(phi - phi )  |
//--------------------------                |                      i         |


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] += aGradD(vi)*dt*dt/12.0*fac*(a_phi_npi - a_phi_n);
}


	return;
} //ScaTraImpl::CalMatAndRHS_TG4_1S


/*-------------------------------------------------------------------------------*
 |  evaluate element matrix and rhs for transport equation with TG4  schott 12/10|
 *-------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::CalMatAndRHS_TG4_Leapfrog(
    Epetra_SerialDenseMatrix&             emat,
    Epetra_SerialDenseVector&             erhs,
    const double                          fac,
    const int                             dofindex,
    DRT::Element*                         ele,
    const double                          dt
    )
{

//==========================================================
// evaluate element vectors and gradients
//==========================================================
// get element vectors
// dist_npi   distance vector for reinitialization at current timestep np and old increment i
// dist_n     distance vector for reinitialization at old timestep n
// phi_0      reference phi vector for smoothed sign function -> needed for directed transport along characteristics
const double dist_nm  = funct_.Dot(ephinm_[dofindex]);   // d^n
const double dist_n   = funct_.Dot(ephin_[dofindex]);   // d^n
const double dist_npi = funct_.Dot(ephinp_[dofindex]);  // d^(n+1)_i


// get gradients and norms
// dist_npi   distance vector for reinitialization at current timestep np and old increment i
// dist_n     distance vector for reinitialization at old timestep n
// phi_0      reference phi vector for smoothed sign function -> needed for directed transport along characteristics

LINALG::Matrix<nsd_,1> grad_dist_n;
grad_dist_n.Clear();
grad_dist_n.Multiply(derxy_,ephin_[dofindex]);

LINALG::Matrix<nsd_,1> grad_dist_npi;
grad_dist_npi.Clear();
grad_dist_npi.Multiply(derxy_,ephinp_[dofindex]);

LINALG::Matrix<nsd_,1> grad_dist_nm;
grad_dist_nm.Clear();
grad_dist_nm.Multiply(derxy_,ephinm_[dofindex]);

//double grad_norm_dist_n   = grad_dist_n.Norm2();
//double grad_norm_dist_npi = grad_dist_npi.Norm2();


//----------------------------------------------------------------
// standard Galerkin transient term
//----------------------------------------------------------------

//     |           |
//     | w, D(phi) |
//     |           |

for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) += funct_(vi)*fac*funct_(ui);
  }
}


//               |                           |
//    1/6 *dt^2  | a*grad(w), a*grad(D(phi)) |
//               |                           |

// a*grad(w) bzw. a*grad(D(phi))
LINALG::Matrix<nen_,1> aGradD(true);
aGradD.MultiplyTN(derxy_,velint_);

// a*grad(phi_n)
//double a_phi_n = velint_.Dot(grad_dist_n);

// a*grad(phi_npi)
double a_phi_npi = velint_.Dot(grad_dist_npi);

// a*grad(phi_nm)
double a_phi_nm = velint_.Dot(grad_dist_nm);


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  for (int ui=0; ui<nen_; ++ui)
  {
    const int fui = ui*numdofpernode_+dofindex;

    emat(fvi,fui) += aGradD(vi)*aGradD(ui)* (fac*dt*dt/6.0);
  }
}


//----------  --------------    |                n    |
//  rhs                    +2dt | a*grad(w) , phi     |
//--------------------------    |                     |


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] += 2.0*dt*fac*aGradD(vi)*dist_n;
}

//----------  --------------    |        n+1     n-1|
//  rhs                       - | w , phi    -phi   |
//--------------------------    |        i          |


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= fac*funct_(vi)*(dist_npi-dist_nm);
}


////----------  --------------                |                      n   |
////  rhs                         -1/2*dt*dt* | a*grad(w), a*grad(phi )  |
////--------------------------                |                          |
//
//
//for (int vi=0; vi<nen_; ++vi)
//{
//  const int fvi = vi*numdofpernode_+dofindex;
//
//  erhs[fvi] -= aGradD(vi)*dt*dt*0.5*fac*a_phi_n;
//}

//----------  --------------                |                      n+1    n-1    |
//  rhs                         -1/6*dt*dt* | a*grad(w), a*grad(phi   -phi    )  |
//--------------------------                |                      i             |


for (int vi=0; vi<nen_; ++vi)
{
  const int fvi = vi*numdofpernode_+dofindex;

  erhs[fvi] -= aGradD(vi)*dt*dt/6.0*fac*(a_phi_npi-a_phi_nm);
}


	return;
} //ScaTraImpl::CalMatAndRHS_TG4_Leapfrog



/*----------------------------------------------------------------------*
 |  calculate system matrix and rhs (public)                schott 08/08|
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::SysmatReinitialize(
    DRT::Element*                         ele, ///< the element those matrix is calculated
    Epetra_SerialDenseMatrix&             sys_mat,///< element matrix to calculate
    Epetra_SerialDenseVector&             residual, ///< element rhs to calculate
    const bool                            reinitswitch,
    const double                          reinit_pseudo_timestepsize_factor,
    const INPAR::SCATRA::SmoothedSignType smoothedSignType,
    const INPAR::SCATRA::ReinitializationStrategy reinitstrategy,
    const INPAR::SCATRA::PenaltyMethod    penalty_method,
    const double                          penalty_interface_reinit,
    const double                          epsilon_bandwidth,
    const enum INPAR::SCATRA::ScaTraType  scatratype ///< type of scalar transport problem
)
{
  //----------------------------------------------------------------------
  // calculation of element volume both for tau at ele. cent. and int. pt.
  //----------------------------------------------------------------------
  // use one-point Gauss rule to do calculations at the element center
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints_tau(SCATRA::DisTypeToStabGaussRule<distype>::rule);

  // volume of the element (2D: element surface area; 1D: element length)
  // (Integration of f(x) = 1 gives exactly the volume/surface/length of element)
  EvalShapeFuncAndDerivsAtIntPoint(intpoints_tau,0,ele->Id());

  //----------------------------------------------------------------------
  // get material parameters (evaluation at element center)
  //----------------------------------------------------------------------
  if (not mat_gp_ or not tau_gp_) GetMaterialParams(ele, scatratype);


  if ((scatratype == INPAR::SCATRA::scatratype_levelset))
  {
    const double mesh_size = getEleDiameter<distype>(xyze_);

    const double pseudo_timestep_size = mesh_size * reinit_pseudo_timestepsize_factor;

    DRT::UTILS::IntPointsAndWeights<nsd_> intpoints_reinit(SCATRA::DisTypeToGaussRuleForExactSol<distype>::rule);

    //==================================== new implementation of REINITIALIZATION================================
    // reinitialization according to sussman 1994, nagrath 2005                                     schott 12/10
    //===========================================================================================================


    //----------------------------------------------------------------------
    // integration loop for one element
    //----------------------------------------------------------------------

    // Assemble element rhs and vector for domain!!! integrals
    for (int iquad=0; iquad<intpoints_reinit.IP().nquad; ++iquad)
    {
      const double fac = EvalShapeFuncAndDerivsAtIntPointReinitialization(intpoints_reinit,iquad,ele->Id());

      for (int k=0;k<numscal_;++k) // deal with a system of transported scalars
      {
         // compute matrix and rhs
         if (reinitstrategy == INPAR::SCATRA::reinitstrategy_pdebased_stabilized_convection)
         {
//      	     CalMatAndRHS_REINITIALIZATION(sys_mat,
//                                      residual,
//                                      fac,
//                                      fssgd,
//                                      timefac,
//                                      alphaF,
//                                      k/*,
//                                      sign_phi0*/);
			//TODO: penalty ansatz!!!
         }
         if (reinitstrategy == INPAR::SCATRA::reinitstrategy_pdebased_linear_convection)
         {
//             // compute matrix and rhs
//             CalMatAndRHS_LinearAdvection_REINITIALIZATION(sys_mat,
//                                        residual,
//                                        fac,
//                                        k,
//                                        ele,
//                                        pseudo_timestep_size,
//                                        mesh_size,
//                                        penalty_interface_reinit,
//                                        epsilon_bandwidth,
//                                        smoothedSignType);
        	 dserror("should not be called here!");
         }
         else if (reinitstrategy == INPAR::SCATRA::reinitstrategy_pdebased_characteristic_galerkin)
         {
            // compute matrix and rhs
            CalMatAndRHS_Characteristic_Galerkin_REINITIALIZATION(sys_mat,
                                       residual,
                                       fac,
                                       k,
                                       ele,
                                       pseudo_timestep_size,
                                       mesh_size,
                                       penalty_method,
                                       penalty_interface_reinit,
                                       epsilon_bandwidth,
                                       smoothedSignType);

         }
         else dserror("this reinitstrategy should not be called here!");

      } // loop over each scalar
    } // integration loop

    for (int k=0;k<numscal_;++k) // deal with a system of transported scalars
    {
       if(penalty_method == INPAR::SCATRA::penalty_method_intersection_points)
       {
          CalMatAndRHS_PENALTY_REINITIALIZATION(sys_mat,
                                                residual,
                                                k,
                                                ele,
                                                penalty_interface_reinit);
       }
    }

  }
  else // 'standard' scalar transport
  {
    dserror("wrong scatratype!");
  }

  return;
}


/*----------------------------------------------------------------------*
 |  calculate system matrix and rhs (public)                schott 05/11|
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraImpl<distype>::SysmatLinearAdvectionSysmat(
    DRT::Element*                         ele, ///< the element those matrix is calculated
    Epetra_SerialDenseMatrix&             sys_mat,///< element matrix to calculate
    Epetra_SerialDenseVector&             residual, ///< element rhs to calculate
    Epetra_SerialDenseVector&             subgrdiff, ///< subgrid-diff.-scaling vector
    const double                          dt, ///< current time-step length
    const double                          timefac,
    const double                          meshsize,
    const enum INPAR::SCATRA::TauType     whichtau, ///< stabilization parameter definition
    const bool                            reinitswitch,
    const double                          reinit_pseudo_timestepsize_factor,
    const INPAR::SCATRA::SmoothedSignType smoothedSignType,
    const INPAR::SCATRA::ReinitializationStrategy reinitstrategy,
    const INPAR::SCATRA::PenaltyMethod    penalty_method,
    const double                          penalty_interface_reinit,
    const double                          epsilon_bandwidth,
    const bool                            shock_capturing,
    const double                          shock_capturing_diffusivity,
    const enum INPAR::SCATRA::ScaTraType  scatratype ///< type of scalar transport problem

)
{
  //----------------------------------------------------------------------
  // calculation of element volume both for tau at ele. cent. and int. pt.
  //----------------------------------------------------------------------
  // use one-point Gauss rule to do calculations at the element center
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints_tau(SCATRA::DisTypeToStabGaussRule<distype>::rule);

  // volume of the element (2D: element surface area; 1D: element length)
  // (Integration of f(x) = 1 gives exactly the volume/surface/length of element)
  const double vol = EvalShapeFuncAndDerivsAtIntPoint(intpoints_tau,0,ele->Id());

  //----------------------------------------------------------------------
  // get material parameters (evaluation at element center)
  //----------------------------------------------------------------------
  if (not mat_gp_ or not tau_gp_) GetMaterialParams(ele, scatratype);

  //----------------------------------------------------------------------
  // calculation of subgrid diffusivity and stabilization parameter(s)
  // at element center
  //----------------------------------------------------------------------
//  if (not tau_gp_)
//  {
//	    // get velocity at element center
//	    velint_.Multiply(evelnp_,funct_);
//
//	    bool twoionsystem(false);
//	    double resdiffus(diffus_[0]);
//	    if (iselch_) // electrochemistry problem
//	    {
//	      // when migration velocity is included to tau (we provide always now)
//	      {
//	        // compute global derivatives
//	        derxy_.Multiply(xij_,deriv_);
//
//	        // get "migration velocity" divided by D_k*z_k at element center
//	        migvelint_.Multiply(-frt,derxy_,epotnp_);
//	      }
//
//	      // ELCH: special stabilization in case of binary electrolytes
//	      twoionsystem= SCATRA::IsBinaryElectrolyte(valence_);
//	      if (twoionsystem)
//	      {
//	        std::vector<int> indices_twoions = SCATRA::GetIndicesBinaryElectrolyte(valence_);
//	        resdiffus = SCATRA::CalResDiffCoeff(valence_,diffus_,indices_twoions);
//	#ifdef ACTIVATEBINARYELECTROLYTE
//	       migrationstab_=false;
//	       migrationintau_=false;
//	#endif
//	      }
//	    }
//
//	    for (int k = 0;k<numscal_;++k) // loop of each transported scalar
//	    {
//	      // calculation of all-scale subgrid diffusivity (artificial or due to
//	      // constant-coefficient Smagorinsky model) at element center
//	      if (assgd or turbmodel)
//	        CalcSubgrDiff(dt,timefac,whichassgd,assgd,turbmodel,Cs,tpn,vol,k);
//
//	      // calculation of fine-scale artificial subgrid diffusivity at element center
//	      if (fssgd) CalcFineScaleSubgrDiff(ele,subgrdiff,whichfssgd,Cs,tpn,vol,k);
//
//	#ifdef ACTIVATEBINARYELECTROLYTE
//	      if (twoionsystem && (abs(valence_[k])>EPS10))
//	        CalTau(ele,resdiffus,dt,timefac,whichtau,vol,k,frt,false);
//	      else
//	#endif
//	      // calculation of stabilization parameter at element center
//	      CalTau(ele,diffus_[k],dt,timefac,whichtau,vol,k,frt,migrationintau_);
//	    }
////	  dserror("stabilization should be called at each gaussian point for reinitialization problems!");
//  }

  //----------------------------------------------------------------------
  // integration loop for one element
  //----------------------------------------------------------------------
  // integrations points and weights
  DRT::UTILS::IntPointsAndWeights<nsd_> intpoints(SCATRA::DisTypeToOptGaussRule<distype>::rule);

  //=========================================================================================
  const double phi_gradient_TOL = 1e-003;
  bool do_evaluate = true;

  // decide if element is evaluated or not!!!
  // get phi_gradient at midpoint
  LINALG::Matrix<nsd_,1> midpoint(true); // midpoint is (0.0, 0.0, 0.0)
  DRT::UTILS::shape_function_deriv1<distype>(midpoint,deriv_);

  xjm_.MultiplyNT(deriv_,xyze_);
   const double det = xij_.Invert(xjm_);

   if (det < 1E-16)
     dserror("GLOBAL ELEMENT NO.%i\nZERO OR NEGATIVE JACOBIAN DETERMINANT: %f", ele->Id(), det);

   // compute global derivatives
   derxy_.Multiply(xij_,deriv_);

   grad_phi_0.Clear();
   grad_phi_0.Multiply(derxy_,ephi0_Reinit_Reference_[0]);
   if(numscal_>1) dserror("evaluate check implemented for one scalar only");

   if(fabs(grad_phi_0.Norm2()) < phi_gradient_TOL)
   {
     do_evaluate=false;
     cout << "only mass matrix assembled in element " << ele->Id() << "Too small gradients for reinitialization" << endl;
   }

  //=========================================================================================

    for (int iquad=0; iquad<intpoints.IP().nquad; ++iquad)
    {
      const double fac = EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad,ele->Id());

      //----------------------------------------------------------------------
      // get material parameters (evaluation at integration point)
      //----------------------------------------------------------------------
      if (mat_gp_) GetMaterialParams(ele, scatratype);

      for (int k=0;k<numscal_;++k) // deal with a system of transported scalars
      {
        // get gradients and norms
        grad_phi_0.Clear();
        grad_phi_0.Multiply(derxy_,ephi0_Reinit_Reference_[k]);

        // evaluate phi and gradnormphi
        const double phi_0    = funct_.Dot(ephi0_Reinit_Reference_[k]);

#ifdef REINIT_LINEAR_ADVECTION_PHIGRADIENT
        // use orginial phi-gradients for computation of reinit velocity

        double grad_norm_phi_0    = grad_phi_0.Norm2();
        if(fabs(grad_norm_phi_0)>1e-12) velint_.Update(1.0/grad_norm_phi_0, grad_phi_0, 0.0);
        else                            velint_.Clear();
#endif
#ifdef REINIT_LINEAR_ADVECTION_RECONSTRUCTED_NORMALS

        // use reconstructed phi-gradients for computation of reinit velocity
        // get velocity at integration point
        velint_.Multiply(evelnp_,funct_);
#endif

        double smoothedSign = EvaluateSmoothedSign(phi_0, grad_norm_phi_0, epsilon_bandwidth,meshsize, smoothedSignType);

        // evaluate smoothed sign at nodes of element and then interpolate at Gaussian point
//        LINALG::Matrix<nen_,1> sign_at_corners(true);
//        // evaluate smoothed sign at corners of element -> then bilinear interpolation
//        for(int i=0; i< nen_; i++)
//        {
//        	sign_at_corners(i,0) = EvaluateSmoothedSign(ephi0_Reinit_Reference_[k](i), grad_norm_phi_0, epsilon_bandwidth,meshsize, smoothedSignType);
//        }
//
//        smoothedSign = funct_.Dot(sign_at_corners);

#ifdef DONT_EVALUATE_SMALL_GRADIENTS
        // do not advect in elements with small gradients, but assemble the mass matrix
        if(do_evaluate==false) smoothedSign = 0.0;
#endif


        // evaluate signum function and scale the normalized direction stored in velint_;
        velint_.Scale(smoothedSign);

        // convective part in convective form: rho*u_x*N,x+ rho*u_y*N,y
        conv_.MultiplyTN(derxy_,velint_);

        // velocity divergence required for conservative form
        if (conservative_) GetDivergence(vdiv_,evelnp_,derxy_);

        // ensure that subgrid-scale velocity and subgrid-scale convective part
        // are zero if not computed below
        sgvelint_.Clear();
        sgconv_.Clear();

        //--------------------------------------------------------------------
        // calculation of (fine-scale) subgrid diffusivity, subgrid-scale
        // velocity and stabilization parameter(s) at integration point
        //--------------------------------------------------------------------
        if (true) // tau_gp_
        {
//          // calculation of all-scale subgrid diffusivity (artificial or due to
//          // constant-coefficient Smagorinsky model) at integration point
//          if (assgd or turbmodel)
//            CalcSubgrDiff(dt,timefac,whichassgd,assgd,turbmodel,Cs,tpn,vol,k);
//
//          // calculation of fine-scale artificial subgrid diffusivity
//          // at integration point
//          if (fssgd) CalcFineScaleSubgrDiff(ele,subgrdiff,whichfssgd,Cs,tpn,vol,k);
//
//          // calculation of subgrid-scale velocity at integration point if required
//          if (sgvel_)
//          {
//            // calculation of stabilization parameter related to fluid momentum
//            // equation at integration point
//            CalTau(ele,visc_,dt,timefac,whichtau,vol,k,0.0,false);
//
//            if (scatratype != INPAR::SCATRA::scatratype_levelset)
//                 CalcSubgrVelocity(ele,time,dt,timefac,k);
//            else CalcSubgrVelocityLevelSet(ele,time,dt,timefac,k);
//            //CalcSubgrVelocityLevelSet(ele,time,dt,timefac,k,ele->Id(),iquad,intpoints, iquad);
//
//            // calculation of subgrid-scale convective part
//            sgconv_.MultiplyTN(derxy_,sgvelint_);
//          }

          // calculation of stabilization parameter at integration point
          CalTau(ele,diffus_[k],dt,timefac,whichtau,vol,k,0.0,false);
        }

        // get history data (or acceleration)
//        hist_[k] = funct_.Dot(ehist_[k]);
//        cout <<"hist" << hist_[k] << endl;
        // new local compuation of hist_-vector

        //TODO: thats right for a theta = 1.0 implementation
        hist_[k] = funct_.Dot(ephin_[k]);

        // set the rhs_ for reinitialization problems
        rhs_[k] = densnp_[k]*smoothedSign;


        // compute matrix and rhs
        CalMatAndRHS_LinearAdvection_REINITIALIZATION(sys_mat,
                     residual,
                     fac,
                     k,
                     ele,
                     reinit_pseudo_timestepsize_factor,
                     meshsize,
                     penalty_method,
                     penalty_interface_reinit,
                     epsilon_bandwidth,
                     smoothedSignType,
                     shock_capturing,
                     shock_capturing_diffusivity,
                     timefac);
      } // loop over each scalar
  } // integration loop

  for (int k=0;k<numscal_;++k) // deal with a system of transported scalars
  {
     if(penalty_method == INPAR::SCATRA::penalty_method_intersection_points)
     {
        CalMatAndRHS_PENALTY_REINITIALIZATION(sys_mat,
                                              residual,
                                              k,
                                              ele,
                                              penalty_interface_reinit);
     }
  }

  return;
}



#endif // CCADISCRET
#endif // D_FLUID3
