/*--------------------------------------------------------------------------*/
/*!
\file scatra_ele_calc_service_elch_diffcond.cpp

\brief evaluation of scatra elements for elch

<pre>
Maintainer: Andreas Ehrl
            ehrl@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089-289-15252
</pre>
*/
/*--------------------------------------------------------------------------*/

#include "scatra_ele_calc_elch_diffcond.H"

#include "scatra_ele.H"
#include "scatra_ele_action.H"
#include "scatra_ele_parameter_elch.H"

#include "../drt_geometry/position_array.H"
//TODO: SCATRA_ELE_CLEANING: Wie bekommen wir das sonst?
#include "../drt_lib/drt_discret.H"  // for time curve in body force
#include "../drt_lib/drt_utils.H"
#include "../drt_fem_general/drt_utils_fem_shapefunctions.H"
#include "../drt_lib/standardtypes_cpp.H"  // for EPS13 and so on
#include "../drt_lib/drt_globalproblem.H"  // consistency check of formulation and material

#include "../drt_inpar/inpar_elch.H"
#include "../drt_mat/elchmat.H"
#include "../drt_mat/newman.H"
#include "../drt_mat/elchphase.H"

//#include "scatra_ele_parameter_timint.H"
//
//#include "../drt_lib/drt_utils.H"

//#include "../drt_lib/drt_discret.H"


/*----------------------------------------------------------------------*
 * Add dummy mass matrix to sysmat
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<distype>::PrepMatAndRhsInitialTimeDerivative(
  Epetra_SerialDenseMatrix&  elemat1_epetra,
  Epetra_SerialDenseVector&  elevec1_epetra
  )
{
  // integrations points and weights
  DRT::UTILS::IntPointsAndWeights<my::nsd_> intpoints(SCATRA::DisTypeToOptGaussRule<distype>::rule);

  /*----------------------------------------------------------------------*/
  // element integration loop                                  ehrl 02/14 |
  /*----------------------------------------------------------------------*/
  for (int iquad=0; iquad<intpoints.IP().nquad; ++iquad)
  {
    const double fac = my::EvalShapeFuncAndDerivsAtIntPoint(intpoints,iquad);

    // loop starts at k=numscal_ !!
    for (int vi=0; vi<my::nen_; ++vi)
    {
      const double v = fac*my::funct_(vi); // no density required here
      const int fvi = vi*my::numdofpernode_+my::numscal_;

      for (int ui=0; ui<my::nen_; ++ui)
      {
        const int fui = ui*my::numdofpernode_+my::numscal_;

        elemat1_epetra(fvi,fui) += v*my::funct_(ui);
      }
    }

    // current as a solution variable
    if(cursolvar_)
    {
      for(int idim=0;idim<my::nsd_;++idim)
      {
        // loop starts at k=numscal_ !!
        for (int vi=0; vi<my::nen_; ++vi)
        {
          const double v = fac*my::funct_(vi); // no density required here
          const int fvi = vi*my::numdofpernode_+my::numscal_+1+idim;

          for (int ui=0; ui<my::nen_; ++ui)
          {
            const int fui = ui*my::numdofpernode_+my::numscal_+1+idim;

            elemat1_epetra(fvi,fui) += v*my::funct_(ui);
          }
        }
      }
    }
  }

  // set zero for the rhs of the potential
  for (int vi=0; vi<my::nen_; ++vi)
  {
    const int fvi = vi*my::numdofpernode_+my::numscal_;

    elevec1_epetra[fvi] = 0.0; // zero out!
  }

  if(cursolvar_)
  {
    for(int idim=0;idim<my::nsd_;++idim)
    {
      // loop starts at k=numscal_ !!
      for (int vi=0; vi<my::nen_; ++vi)
      {
        elevec1_epetra[vi*my::numdofpernode_+my::numscal_+1+idim]=0.0;
      }
    }
  }

  return;
}

/*----------------------------------------------------------------------*
 * Get Conductivity                                          ehrl 02/14 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<distype>::GetConductivity(
    const enum INPAR::ELCH::ElchType  elchtype,
    double&                    sigma_all,
    Epetra_SerialDenseVector&  sigma
  )
{
  // dynamic cast to elch-specific diffusion manager
  Teuchos::RCP<ScaTraEleDiffManagerElchDiffCond> dme = Teuchos::rcp_static_cast<ScaTraEleDiffManagerElchDiffCond>(my::diffmanager_);

  // pre-computed conductivity is used:
  // Conductivity is computed by
  // sigma = F^2/RT*Sum(z_k^2 D_k c_k)
  sigma_all=dme->GetCond();

  return;
}

/*----------------------------------------------------------------------*
 * Calculate Mat and Rhs for electric potential field        ehrl 02/14 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<distype>::CalMatAndRhsElectricPotentialField(
  Teuchos::RCP<ScaTraEleInternalVariableManagerElch <my::nsd_,my::nen_> >& vm,
  const enum INPAR::ELCH::ElchType  elchtype,
  Epetra_SerialDenseMatrix&         emat,
  Epetra_SerialDenseVector&         erhs,
  const double                      fac,
  Teuchos::RCP<ScaTraEleDiffManagerElch>& dme
)
{
  // dynamic cast to elch-specific diffusion manager
  Teuchos::RCP<ScaTraEleDiffManagerElchDiffCond> dmedc = Teuchos::rcp_static_cast<ScaTraEleDiffManagerElchDiffCond>(dme);

  // dynamic cast to elch-specific diffusion manager
  Teuchos::RCP<ScaTraEleInternalVariableManagerElchDiffCond <my::nsd_,my::nen_> > vmdc
    = Teuchos::rcp_static_cast<ScaTraEleInternalVariableManagerElchDiffCond <my::nsd_,my::nen_> >(vm);

  // specific constants for the Newman-material:
  // switch between a dilute solution theory like formulation and the classical concentrated solution theory
  double newman_const_a = myelch::elchpara_->NewmanConstA();
  double newman_const_b = myelch::elchpara_->NewmanConstB();

  if(diffcondmat_==INPAR::ELCH::diffcondmat_ion)
    dserror("The function CalcInitialPotential is only implemented for Newman materials");

  if(cursolvar_==true)
    dserror("The function CalcInitialPotential is only implemented for Newman materials without the current as solution variable");

  for (int k=0; k<my::numscal_; ++k)
  {
    for (int vi=0; vi<my::nen_; ++vi)
    {
      const int fvi = vi*my::numdofpernode_+my::numscal_;
      double laplawf(0.0);
      my::GetLaplacianWeakFormRHS(laplawf,vmdc->GradPhi(k),vi);

      for (int iscal=0; iscal < my::numscal_; ++iscal)
      {
        erhs[fvi] -= fac*epstort_[0]*vmdc->RTFFC()*dmedc->GetCond()*(dmedc->GetThermFac())*(newman_const_a+(newman_const_b*dmedc->GetTransNum(iscal)))*vmdc->ConIntInv(iscal)*laplawf;
      }
    }

    // provide something for conc. dofs: a standard mass matrix
    for (int vi=0; vi<my::nen_; ++vi)
    {
      const int    fvi = vi*my::numdofpernode_+k;
      for (int ui=0; ui<my::nen_; ++ui)
      {
        const int fui = ui*my::numdofpernode_+k;
        emat(fvi,fui) += fac*my::funct_(vi)*my::funct_(ui);
      }
    }
  } // for k

  // ----------------------------------------matrix entries
  for (int vi=0; vi<my::nen_; ++vi)
  {
    const int    fvi = vi*my::numdofpernode_+my::numscal_;
    for (int ui=0; ui<my::nen_; ++ui)
    {
      const int fui = ui*my::numdofpernode_+my::numscal_;
      double laplawf(0.0);
      my::GetLaplacianWeakForm(laplawf,ui,vi);
      emat(fvi,fui) += fac*epstort_[0]*vmdc->InvF()*dmedc->GetCond()*laplawf;
    }

    double laplawf(0.0);
    my::GetLaplacianWeakFormRHS(laplawf,vmdc->GradPot(),vi);
    erhs[fvi] -= fac*epstort_[0]*vmdc->InvF()*dmedc->GetCond()*laplawf;
  }

  return;
}

// template classes

// 2D elements
template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::line2>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::tri3>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::tri6>;
template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::quad4>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::quad8>;
template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::quad9>;
template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::nurbs9>;

// 3D elements
template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::hex8>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::hex20>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::hex27>;
template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::tet4>;
template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::tet10>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::wedge6>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::pyramid5>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchDiffCond<DRT::Element::nurbs27>;
