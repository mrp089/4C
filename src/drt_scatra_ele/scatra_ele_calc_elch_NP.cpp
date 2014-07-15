/*--------------------------------------------------------------------------*/
/*!
\file scatra_ele_calc_elch.cpp

\brief evaluation of ScaTra elements for Nernst-Planck ion-transport equations

<pre>
Maintainer: Andreas Ehrl
            ehrl@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089-289-15252
</pre>
*/
/*--------------------------------------------------------------------------*/

#include "scatra_ele_calc_elch_NP.H"
#include "scatra_ele_parameter.H"

#include "../drt_lib/drt_discret.H"
#include "../drt_lib/drt_utils.H"
#include "../drt_mat/material.H"
#include "../drt_mat/matlist.H"

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template<DRT::Element::DiscretizationType distype>
DRT::ELEMENTS::ScaTraEleCalcElchNP<distype> * DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::Instance(
  const int numdofpernode,
  const int numscal,
  bool create )
{
  static ScaTraEleCalcElchNP<distype> * instance;
  if ( create )
  {
    if ( instance==NULL )
    {
      instance = new ScaTraEleCalcElchNP<distype>(numdofpernode,numscal);
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
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::Done()
{
  // delete this pointer! Afterwards we have to go! But since this is a
  // cleanup call, we can do it this way.
  Instance( 0, 0, false );
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::ScaTraEleCalcElchNP(const int numdofpernode,const int numscal)
  : DRT::ELEMENTS::ScaTraEleCalcElch<distype>::ScaTraEleCalcElch(numdofpernode,numscal)
{
  // initialize internal variable manager
  myelch::varmanager_ = Teuchos::rcp(new ScaTraEleInternalVariableManagerElchNP<my::nsd_, my::nen_>(my::numscal_,my::nsd_,myelch::elchpara_));

  return;
}


/*----------------------------------------------------------------------*
|  calculate system matrix and rhs                           fang 05/14 |
*-----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcMatAndRhs(
  Teuchos::RCP<ScaTraEleInternalVariableManagerElch <my::nsd_,my::nen_> >&   vm,
  Epetra_SerialDenseMatrix&                                                  emat,         //!< element matrix to calculate
  Epetra_SerialDenseVector&                                                  erhs,         //!< element rhs to calculate
  const int                                                                  k,            //!< index of current scalar
  const double                                                               fac,          //!< domain-integration factor
  const double                                                               timefacfac,   //!< domain-integration factor times time-integration factor
  const double                                                               rhsfac,       //!< time-integration factor for rhs times domain-integration factor
  const double                                                               taufac,       //!< tau times domain-integration factor
  const double                                                               timetaufac,   //!< domain-integration factor times tau times time-integration factor
  const double                                                               rhstaufac,    //!< time-integration factor for rhs times tau times domain-integration factor
  LINALG::Matrix<my::nen_,1>&                                                tauderpot,    //!< derivatives of stabilization parameter w.r.t. electric potential
  Teuchos::RCP<ScaTraEleDiffManagerElch>&                                    dme,          //!< diffusion manager
  double&                                                                    rhsint,       //!< rhs of Nernst-Planck equation (not of Newton-Raphson scheme) at Gauss point
  const double                                                               hist          //!< history
  )
{
  // dynamic cast to Nernst-Planck-specific internal variable manager
  Teuchos::RCP<ScaTraEleInternalVariableManagerElchNP <my::nsd_,my::nen_> > vmnp
    = Teuchos::rcp_dynamic_cast<ScaTraEleInternalVariableManagerElchNP <my::nsd_,my::nen_> >(vm);

  // Compute residual of Nernst-Planck equation in strong form and subgrid-scale part of concentration c_k
  const double residual = CalcRes(k,vmnp->ConInt(k),hist,vmnp->ConvPhi(k),vmnp->FRT(),dme,vmnp->MigConv(),rhsint);

  //--------------------------------------------------------------------------
  // 1) element matrix: instationary terms arising from Nernst-Planck equation
  //--------------------------------------------------------------------------

  if (not my::scatraparatimint_->IsStationary())
  {
    // 1a) element matrix: standard Galerkin mass term
    my::CalcMatMass(emat,k,fac,1.);

    // 1b) element matrix: stabilization of mass term
    // not implemented, only SUPG stabilization of convective term due to fluid flow and migration available
  }

  //------------------------------------------------------------------------
  // 2) element matrix: stationary terms arising from Nernst-Planck equation
  //------------------------------------------------------------------------

  // 2a) element matrix: standard Galerkin convective term due to fluid flow
  my::CalcMatConv(emat,k,timefacfac,1.,vmnp->Conv(),vmnp->SGConv());

  // 2b) element matrix: additional terms in conservative formulation if needed
  if (my::scatrapara_->IsConservative())
  {
    double vdiv(0.);
    my::GetDivergence(vdiv,my::evelnp_);
    my::CalcMatConvAddCons(emat,k,timefacfac,vdiv,1.);
  }

  // 2c) element matrix: stabilization of convective term due to fluid flow and migration
  CalcMatConvStab(emat,k,timefacfac,taufac,timetaufac,tauderpot,vmnp->FRT(),dme,vmnp->Conv(),vmnp->MigConv(),vmnp->ConInt(k),vmnp->GradPhi(k),residual);

  // 2d) element matrix: standard Galerkin diffusive term (constant diffusion coefficient)
  my::CalcMatDiff(emat,k,timefacfac,dme);

  // 2e) element matrix: stabilization of diffusive term
  // not implemented, only SUPG stabilization of convective term due to fluid flow and migration available

  // 2f) element matrix: standard Galerkin migration term (can be split up into convective and reactive parts)
  CalcMatMigr(emat,k,timefacfac,vmnp->FRT(),dme,vmnp->MigConv(),vmnp->ConInt(k));

  // 2g) element matrix: stabilization of reactive term due to migration
  // not implemented, only SUPG stabilization of convective term due to fluid flow and migration available

  //-------------------------------------------------------------------------------------------
  // 3) element matrix: stationary terms arising from governing equation for electric potential
  //-------------------------------------------------------------------------------------------

  // element matrix: standard Galerkin terms from governing equation for electric potential field
  switch (myelch::elchpara_->EquPot())   // determine type of equation used for electric potential field
  {
  case INPAR::ELCH::equpot_enc:
  {
    myelch::CalcMatPotEquENC(emat,k,fac,my::scatraparatimint_->AlphaF(),dme);
    break;
  }
  case INPAR::ELCH::equpot_enc_pde:
  {
    CalcMatPotEquENCPDE(emat,k,timefacfac,vmnp->FRT(),dme,vmnp->MigConv(),vmnp->ConInt(k));
    break;
  }
  case INPAR::ELCH::equpot_enc_pde_elim:
  {
    CalcMatPotEquENCPDEElim(emat,k,timefacfac,vmnp->FRT(),dme,vmnp->MigConv(),vmnp->ConInt(k));
    break;
  }
  case INPAR::ELCH::equpot_poisson:
  {
    CalcMatPotEquPoisson(emat,k,fac,vmnp->Epsilon(),vmnp->Faraday(),dme);
    break;
  }
  case INPAR::ELCH::equpot_laplace:
  {
    CalcMatPotEquLaplace(emat,k,fac);
    break;
  }
  default:
  {
    dserror ("Closing equation for electric potential not recognized!");
    break;
  }
  } // end switch(elchparam_->EquPot())

  //----------------------------------------------------------------------------
  // 4) element right hand side vector (negative residual of nonlinear problem):
  //    terms arising from Nernst-Planck equation
  //----------------------------------------------------------------------------

  // 4a) element rhs: standard Galerkin contributions from non-history part of instationary term if needed
  if (not my::scatraparatimint_->IsStationary())
    my::CalcRHSLinMass(erhs,k,rhsfac,fac,1.,1.,vmnp->ConInt(k),hist);

  // 4b) element rhs: standard Galerkin contributions from rhsint vector (contains body force vector and history vector)
  // need to adapt rhsint vector to time integration scheme first
  my::ComputeRhsInt(rhsint,1.,1.,hist);
  my::CalcRHSHistAndSource(erhs,k,fac,rhsint);

  // 4c) element rhs: stabilization of mass term
  // not implemented, only SUPG stabilization of convective term due to fluid flow and migration available

  // 4d) element rhs: standard Galerkin convective term
  my::CalcRHSConv(erhs,k,rhsfac,vmnp->ConvPhi(k));

  // 4e) element rhs: additional terms in conservative formulation if needed
  if (my::scatrapara_->IsConservative())
  {
    double vdiv(0.);
    my::GetDivergence(vdiv,my::evelnp_);
    CalcRhsConvAddCons(erhs,k,rhsfac,vmnp->ConInt(k),vdiv);
  }

  // 4f) element rhs: stabilization of convective term due to fluid flow and migration
  CalcRhsConvStab(erhs,k,rhstaufac,dme,vmnp->Conv(),vmnp->MigConv(),residual);

  // 4g) element rhs: standard Galerkin diffusion term
  my::CalcRHSDiff(erhs,k,rhsfac,dme,vmnp->GradPhi(k));

  // 4h) element rhs: stabilization of diffusive term
  // not implemented, only SUPG stabilization of convective term due to fluid flow and migration available

  // 4i) element rhs: standard Galerkin migration term (can be split up into convective and reactive parts)
  CalcRhsMigr(erhs,k,rhsfac,dme,vmnp->MigConv(),vmnp->ConInt(k));

  // 4j) element rhs: stabilization of reactive term due to migration
  // not implemented, only SUPG stabilization of convective term due to fluid flow and migration available

  //----------------------------------------------------------------------------
  // 5) element right hand side vector (negative residual of nonlinear problem):
  //    terms arising from governing equation for electric potential
  //----------------------------------------------------------------------------

  // element rhs: standard Galerkin terms from governing equation for electric potential field
  switch (myelch::elchpara_->EquPot())   // determine type of equation used for electric potential field
  {
  case INPAR::ELCH::equpot_enc:
  {
    myelch::CalcRhsPotEquENC(erhs,k,fac,dme,vmnp->ConInt(k));
    break;
  }
  case INPAR::ELCH::equpot_enc_pde:
  {
    CalcRhsPotEquENCPDE(erhs,k,rhsfac,dme,vmnp->MigConv(),vmnp->ConInt(k),vmnp->GradPhi(k));
    break;
  }
  case INPAR::ELCH::equpot_enc_pde_elim:
  {
    CalcRhsPotEquENCPDEElim(erhs,k,rhsfac,dme,vmnp->MigConv(),vmnp->ConInt(k),vmnp->GradPhi(k));
    break;
  }
  case INPAR::ELCH::equpot_poisson:
  {
    CalcRhsPotEquPoisson(erhs,k,fac,vmnp->Epsilon(),vmnp->Faraday(),dme,vmnp->ConInt(k),vmnp->GradPot());
    break;
  }
  case INPAR::ELCH::equpot_laplace:
  {
    CalcRhsPotEquLaplace(erhs,k,fac,vmnp->GradPot());
    break;
  }
  default:
  {
    dserror ("Closing equation for electric potential not recognized!");
    break;
  }
  } // end switch (elchparam_->EquPot())
  return;
}


/*-----------------------------------------------------------------------------------------*
 | CalcRes: Residual of Nernst-Planck equation in strong form (private)         fang 06/14 |
 *------------------------------------------------------ ----------------------------------*/
template <DRT::Element::DiscretizationType distype>
double DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcRes(
  const int                                 k,            //!< index of current scalar
  const double                              conint,       //!< concentration at GP
  const double                              hist,         //!< history value at GP
  const double                              convphi,      //!< convective term (without convective part of migration term)
  const double                              frt,          //!< F/(RT)
  Teuchos::RCP<ScaTraEleDiffManagerElch>&   dme,          //!< diffusion manager
  const LINALG::Matrix<my::nen_,1>&         migconv,      //!< migration operator: -F/(RT) \grad{\Phi} * \grad{N}
  const double                              rhsint        //!< rhs of Nernst-Planck equation (not of Newton-Raphson scheme) at Gauss point
  )
{
  // Compute convective term including convective part of migration term
  const double convmigphi = convphi + dme->GetIsotropicDiff(k)*dme->GetValence(k)*(migconv.Dot(my::ephinp_[k]));

  // Compute diffusive term and reactive part of migration term (only significant for higher-order elements)
  double diffphi(0.);
  double reamigphi(0.);

  if (my::use2ndderiv_)
  {
    LINALG::Matrix<my::nen_,1> laplace(true);
    my::GetLaplacianStrongForm(laplace);

    diffphi = dme->GetIsotropicDiff(k)*laplace.Dot(my::ephinp_[k]);
    reamigphi = -frt*dme->GetIsotropicDiff(k)*dme->GetValence(k)*laplace.Dot(myelch::epotnp_)*conint;
  }

  if(my::scatraparatimint_->IsStationary())
    return convmigphi - diffphi + reamigphi - rhsint;
  else if(my::scatraparatimint_->IsGenAlpha())
    return hist + convmigphi - diffphi + reamigphi - rhsint;
  else
    return conint - hist + my::scatraparatimint_->TimeFac()*(convmigphi - diffphi + reamigphi - rhsint);
} // ScaTraEleCalcElchNP<distype>::CalcRes


/*-------------------------------------------------------------------------------------------------------*
 | CalcMat: SUPG Stabilization of convective term due to fluid flow and migration (private)   fang 06/14 |
 *------------------------------------------------------ ------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcMatConvStab(
  Epetra_SerialDenseMatrix&                 emat,         //!< element matrix to calculate
  const int                                 k,            //!< index of current scalar
  const double                              timefacfac,   //!< domain-integration factor times time-integration factor
  const double                              taufac,       //!< stabilization parameter tau times domain-integration factor
  const double                              timetaufac,   //!< domain-integration factor times tau times time-integration factor
  LINALG::Matrix<my::nen_,1>&               tauderpot,    //!< derivatives of stabilization parameter w.r.t. electric potential
  const double                              frt,          //!< F/(RT)
  Teuchos::RCP<ScaTraEleDiffManagerElch>&   dme,          //!< diffusion manager
  const LINALG::Matrix<my::nen_,1>&         conv,         //!< convection operator: u_x*N,x + u_y*N,y + u_z*N,z
  const LINALG::Matrix<my::nen_,1>&         migconv,      //!< migration operator: -F/(RT) \grad{\Phi} * \grad{N}
  const double                              conint,       //!< concentration at GP
  const LINALG::Matrix<my::nsd_,1>&         gradphi,      //!< gradient of concentration at GP
  const double                              residual      //!< residual of Nernst-Planck equation in strong form
  )
{
  // Compute Laplacian N,xx  +  N,yy +  N,zz of all shape functions at current integration point if needed
  LINALG::Matrix<my::nen_,1> laplace(true);
  if (my::use2ndderiv_)
    my::GetLaplacianStrongForm(laplace);

  for (int vi=0; vi<my::nen_; ++vi)
  {
    // compute effective convective stabilization operator
    double conv_eff_vi = conv(vi);
    if (myelch::migrationstab_)
      conv_eff_vi += dme->GetIsotropicDiff(k)*dme->GetValence(k)*migconv(vi);

    // shortcuts
    const double timetaufac_conv_eff_vi = timetaufac*conv_eff_vi;
    const double timetaufac_conv_eff_vi_conint_k_frt_valence_k = timetaufac_conv_eff_vi*conint*frt*dme->GetValence(k);

    for (int ui=0; ui<my::nen_; ++ui)
    {
      // matrix entries
      double matvalconc = 0.;
      double matvalpot = 0.;

      // 1) transient term
      if (not my::scatraparatimint_->IsStationary())
        matvalconc += taufac*conv_eff_vi*my::funct_(ui);

      // 2) convective term due to fluid flow and migration
      // 2a) linearization of residual w.r.t. concentration c_k
      matvalconc += timetaufac*conv_eff_vi*(conv(ui)+dme->GetIsotropicDiff(k)*dme->GetValence(k)*migconv(ui));

      // 2b) linearization of residual w.r.t. electric potential Phi
      double laplawf(0.);
      my::GetLaplacianWeakFormRHS(laplawf,gradphi,ui);
      matvalpot -= timetaufac*conv_eff_vi*dme->GetIsotropicDiff(k)*dme->GetValence(k)*frt*laplawf;

      if (myelch::migrationstab_)
      {
        // 2c) linearization of migration operator w.r.t. concentration c_k
        // not necessary, since migration operator not a function of c_k

        // 2d) linearization of migration operator w.r.t. electric potential Phi
        double laplacewf(0.);
        my::GetLaplacianWeakForm(laplacewf,ui,vi);
        matvalpot -= timetaufac*residual*dme->GetIsotropicDiff(k)*dme->GetValence(k)*frt*laplacewf;
      }

      if (not SCATRA::IsBinaryElectrolyte(dme->GetValence()))
      {
        // 2e) linearization of tau w.r.t. concentration c_k
        // not necessary, since tau not a function of c_k

        // 2f) linearization of tau w.r.t. electric potential Phi (only non-zero for Taylor_Hughes_Zarins at the moment)
        matvalpot += timefacfac*tauderpot(ui)*conv_eff_vi*residual;
      }

      if (my::use2ndderiv_)
      {
        // 3) diffusive term
        // 3a) linearization w.r.t. concentration c_k
        matvalconc -= timetaufac_conv_eff_vi*dme->GetIsotropicDiff(k)*laplace(ui);

        // 3b) linearization w.r.t. electric potential Phi
        // not necessary, since diffusive term not a function of Phi

        // 4) reactive term due to migration
        // 4a) linearization w.r.t. concentration c_k
        matvalconc -= timetaufac_conv_eff_vi*frt*dme->GetIsotropicDiff(k)*dme->GetValence(k)*laplace.Dot(myelch::epotnp_)*my::funct_(ui);

        // 4b) linearization w.r.t. electric potential Phi
        matvalpot -= timetaufac_conv_eff_vi_conint_k_frt_valence_k*dme->GetIsotropicDiff(k)*laplace(ui);
      }

      // try to access the element matrix not too often, can be costly
      const int fvi = vi*my::numdofpernode_+k;
      emat(fvi,ui*my::numdofpernode_+k)            += matvalconc;
      emat(fvi,ui*my::numdofpernode_+my::numscal_) += matvalpot;
    }
  }

  return;
} // ScaTraEleCalcElchNP<distype>::CalcMatMassStab


/*-----------------------------------------------------------------------*
 |  CalcMat: Migration term (private)                         fang 05/14 |
 *-------------------------------------------------- --------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcMatMigr(
  Epetra_SerialDenseMatrix&                 emat,         //!< element matrix to calculate
  const int                                 k,            //!< index of current scalar
  const double                              timefacfac,   //!< domain-integration factor times time-integration factor
  const double                              frt,          //!< F/(RT)
  Teuchos::RCP<ScaTraEleDiffManagerElch>&   dme,          //!< diffusion manager
  const LINALG::Matrix<my::nen_,1>&         migconv,      //!< migration operator: -F/(RT) \grad{\Phi} * \grad{N}
  const double                              conint        //!< concentration at GP
  )
{
  const double timefacfac_diffus_valence_k = timefacfac * dme->GetIsotropicDiff(k) * dme->GetValence(k);
  for (int vi=0; vi<my::nen_; ++vi)
  {
    const double v = timefacfac_diffus_valence_k*migconv(vi);
    const int fvi = vi*my::numdofpernode_+k;

    for (int ui=0; ui<my::nen_; ++ui)
    {
      const int fui = ui*my::numdofpernode_+k;

      // a) derivative w.r.t. concentration c_k
      emat(fvi,fui) -= v*my::funct_(ui);

      // b) derivative w.r.t. electric potential
      double laplawf(0.);
      my::GetLaplacianWeakForm(laplawf,ui,vi);
      emat(fvi,ui*my::numdofpernode_+my::numscal_) += frt*timefacfac*dme->GetIsotropicDiff(k)*dme->GetValence(k)*conint*laplawf;
    }
  }

  return;
} // ScaTraEleCalcElchNP<distype>::CalcMatMigr


/*-----------------------------------------------------------------------*
 |  CalcMat: Electroneutrality in PDE form (private)          fang 05/14 |
 *-----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcMatPotEquENCPDE(
  Epetra_SerialDenseMatrix&                 emat,         //!< element matrix to be filled
  const int                                 k,            //!< index of current scalar
  const double                              timefacfac,   //!< domain-integration factor times time-integration factor
  const double                              frt,          //!< F/(RT)
  Teuchos::RCP<ScaTraEleDiffManagerElch>&   dme,          //!< diffusion manager
  const LINALG::Matrix<my::nen_,1>&         migconv,      //!< migration operator
  const double                              conint        //!< concentration at GP
  )
{
  for (int vi=0; vi<my::nen_; ++vi)
  {
    const int pvi = vi*my::numdofpernode_+my::numscal_;

    // Inclusion of time integration factor results in a matrix with better condition number
    const double timefacfac_diffus_valence_k_mig_vi = timefacfac*dme->GetIsotropicDiff(k)*dme->GetValence(k)*migconv(vi);

    for (int ui=0; ui<my::nen_; ++ui)
    {
      const int fui = ui*my::numdofpernode_+k;

      double laplawf(0.);
      my::GetLaplacianWeakForm(laplawf,ui,vi);

      // use 2nd order pde derived from electroneutrality condition (k=1,...,m)
      // a) derivative w.r.t. concentration c_k
      emat(pvi, fui) -= dme->GetValence(k)*(timefacfac_diffus_valence_k_mig_vi*my::funct_(ui));
      emat(pvi, fui) += dme->GetValence(k)*(timefacfac*dme->GetIsotropicDiff(k)*laplawf);
      // b) derivative w.r.t. electric potential
      emat(pvi, ui*my::numdofpernode_+my::numscal_) += dme->GetValence(k)*(frt*timefacfac*dme->GetIsotropicDiff(k)*dme->GetValence(k)*conint*laplawf);
    } // for ui
  } // for vi

  return;
}


/*-------------------------------------------------------------------------------------------*
 |  CalcMat: ENC in PDE form with NP equation for species m eliminated (private)  fang 05/14 |
 *-------------------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcMatPotEquENCPDEElim(
  Epetra_SerialDenseMatrix&                 emat,         //!< element matrix to be filled
  const int                                 k,            //!< index of current scalar
  const double                              timefacfac,   //!< domain-integration factor times time-integration factor
  const double                              frt,          //!< F/(RT)
  Teuchos::RCP<ScaTraEleDiffManagerElch>&   dme,          //!< diffusion manager
  const LINALG::Matrix<my::nen_,1>&         migconv,      //!< migration operator
  const double                              conint        //!< concentration at GP
  )
{
  for (int vi=0; vi<my::nen_; ++vi)
  {
    const int pvi = vi*my::numdofpernode_+my::numscal_;

    // Inclusion of time integration factor results in a matrix with better condition number
    const double timefacfac_diffus_valence_k_mig_vi = timefacfac*dme->GetIsotropicDiff(k)*dme->GetValence(k)*migconv(vi);
    const double timefacfac_diffus_valence_m_mig_vi = timefacfac*dme->GetIsotropicDiff(my::numscal_)*dme->GetValence(my::numscal_)*migconv(vi);

    for (int ui=0; ui<my::nen_; ++ui)
    {
      // matrix entries
      double matvalconc = 0.;
      double matvalpot = 0.;

      double laplawf(0.);
      my::GetLaplacianWeakForm(laplawf,ui,vi);

      // use 2nd order pde derived from electroneutrality condition (k=1,...,m-1)
      // a) derivative w.r.t. concentration c_k
      matvalconc -= timefacfac_diffus_valence_k_mig_vi*my::funct_(ui);
      matvalconc += timefacfac*dme->GetIsotropicDiff(k)*laplawf;
      // b) derivative w.r.t. electric potential
      matvalpot += frt*timefacfac*dme->GetIsotropicDiff(k)*dme->GetValence(k)*conint*laplawf;

      // care for eliminated species with index m
      // Note: diffus_ and valence_ vectors were extended in GetMaterialParams() so that they
      // also contain the properties of the eliminated species at index m (= my::numscal_))
      // a) derivative w.r.t. concentration c_k
      matvalconc += timefacfac_diffus_valence_m_mig_vi*my::funct_(ui);
      matvalconc -= timefacfac*dme->GetIsotropicDiff(my::numscal_)*laplawf;
      // b) derivative w.r.t. electric potential
      matvalpot -= frt*timefacfac*dme->GetIsotropicDiff(my::numscal_)*dme->GetValence(my::numscal_)*conint*laplawf;

      // try to access the element matrix not too often, can be costly
      const int fui = ui*my::numdofpernode_+k;
      emat(pvi,fui) += dme->GetValence(k)*matvalconc;
      const int pui = ui*my::numdofpernode_+my::numscal_;
      emat(pvi,pui) += dme->GetValence(k)*matvalpot;
    } // for ui
  } // for vi

  return;
}


/*-------------------------------------------------------------------------------------*
 |  CalcMat: Poisson equation for electric potential (private)              fang 05/14 |
 *-------------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcMatPotEquPoisson(
  Epetra_SerialDenseMatrix&                 emat,      //!< element matrix to be filled
  const int                                 k,         //!< index of current scalar
  const double                              fac,       //!< domain-integration factor
  const double                              epsilon,   //!< dielectric constant
  const double                              faraday,   //!< Faraday constant
  Teuchos::RCP<ScaTraEleDiffManagerElch>&   dme        //!< diffusion manager
  )
{
  for (int vi=0; vi<my::nen_; ++vi)
  {
    const int pvi = vi*my::numdofpernode_+my::numscal_;
    const double alphaF_valence_k_fac_funct_vi = my::scatraparatimint_->AlphaF()*dme->GetValence(k)*fac*my::funct_(vi);

    for (int ui=0; ui<my::nen_; ++ui)
    {
      // We have a loop over the species index k around. So prevent that the potential term is added more than once!
      if (k==0)
      {
        const int pui = ui*my::numdofpernode_+my::numscal_;
        double laplawf(0.);
        my::GetLaplacianWeakForm(laplawf,ui,vi);

        const double epsbyF = epsilon/faraday;

        emat(pvi,pui) += my::scatraparatimint_->AlphaF()*fac*epsbyF*laplawf;
      }

      const int fui = ui*my::numdofpernode_+k;

      // electroneutrality condition (only derivative w.r.t. concentration c_k)
      emat(pvi,fui) -= alphaF_valence_k_fac_funct_vi*my::funct_(ui);
    } // for ui
  } // for vi

  return;
}


/*-------------------------------------------------------------------------------------*
 |  CalcMat: Laplace equation for electric potential (private)              fang 05/14 |
 *-------------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcMatPotEquLaplace(
  Epetra_SerialDenseMatrix&                 emat,      //!< element matrix to be filled
  const int                                 k,         //!< index of current scalar
  const double                              fac        //!< domain-integration factor
  )
{
  for (int vi=0; vi<my::nen_; ++vi)
  {
    const int pvi = vi*my::numdofpernode_+my::numscal_;

    for (int ui=0; ui<my::nen_; ++ui)
    {
      // We have a loop over the species index k around. So prevent that the potential term is added more than once!
      if (k==0)
      {
        const int pui = ui*my::numdofpernode_+my::numscal_;

        double laplawf(0.);
        my::GetLaplacianWeakForm(laplawf,ui,vi);

        emat(pvi,pui) += my::scatraparatimint_->AlphaF()*fac*laplawf;
      }
    } // for ui
  } // for vi

  return;
}


/*-----------------------------------------------------------------------------------------*
 |  CalcRhs: Additional contributions from conservative formulation (private)   fang 05/14 |
 *-----------------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcRhsConvAddCons(
    Epetra_SerialDenseVector&                 erhs,     //!< element vector to be filled
    const int                                 k,        //!< index of current scalar
    const double                              rhsfac,   //!< time-integration factor for rhs times domain-integration factor
    const double                              conint,   //!< concentration at GP
    const double                              vdiv      //!< velocity divergence
  )
{
  for (int vi=0; vi<my::nen_; ++vi)
    erhs[vi*my::numdofpernode_+k] -= rhsfac*my::funct_(vi)*conint*vdiv;

  return;
}


/*-------------------------------------------------------------------------------------------------------*
 | CalcRhs: SUPG Stabilization of convective term due to fluid flow and migration (private)   fang 06/14 |
 *------------------------------------------------------ ------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcRhsConvStab(
  Epetra_SerialDenseVector&                 erhs,        //!< element vector to be filled
  const int                                 k,           //!< index of current scalar
  const double                              rhstaufac,   //!< time-integration factor for rhs times tau times domain-integration factor
  Teuchos::RCP<ScaTraEleDiffManagerElch>&   dme,         //!< diffusion manager
  const LINALG::Matrix<my::nen_,1>&         conv,        //!< convection operator: u_x*N,x + u_y*N,y + u_z*N,z
  const LINALG::Matrix<my::nen_,1>&         migconv,     //!< migration operator: -F/(RT) \grad{\Phi} * \grad{N}
  const double                              residual     //!< residual of Nernst-Planck equation in strong form
  )
{
  for (int vi=0; vi< my::nen_; ++vi)
  {
    const int fvi = vi*my::numdofpernode_+k;

    erhs[fvi] -= rhstaufac*conv(vi)*residual;

    if (myelch::migrationstab_)
      erhs[fvi] -=  rhstaufac*dme->GetIsotropicDiff(k)*dme->GetValence(k)*migconv(vi)*residual;
  }

  return;
} // ScaTraEleCalcElchNP<distype>::CalcRhsConvStab


/*-------------------------------------------------------------------------------------*
 |  CalcRhs: Migration term (private)                                       fang 05/14 |
 *-------------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcRhsMigr(
  Epetra_SerialDenseVector&                 erhs,      //!< element vector to be filled
  const int                                 k,         //!< index of current scalar
  const double                              rhsfac,    //!< time-integration factor for rhs times domain-integration factor
  Teuchos::RCP<ScaTraEleDiffManagerElch>&   dme,       //!< diffusion manager
  const LINALG::Matrix<my::nen_,1>&         migconv,   //!< migration operator
  const double                              conint     //!< concentration at GP
  )
{
  const double rhsfac_con_diffus_valence_k = rhsfac*conint*dme->GetIsotropicDiff(k)*dme->GetValence(k);

  for (int vi=0; vi<my::nen_; ++vi)
    erhs[vi*my::numdofpernode_+k] += rhsfac_con_diffus_valence_k*migconv(vi);

  return;
}


/*-------------------------------------------------------------------------------------*
 |  CalcRhs: Electroneutrality condition in PDE form (private)              fang 05/14 |
 *-------------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcRhsPotEquENCPDE(
  Epetra_SerialDenseVector&                         erhs,      //!< element vector to be filled
  const int                                         k,         //!< index of current scalar
  const double                                      rhsfac,    //!< time-integration factor for rhs times domain-integration factor
  Teuchos::RCP<ScaTraEleDiffManagerElch>&           dme,       //!< diffusion manager
  const LINALG::Matrix<my::nen_,1>&                 migconv,   //!< migration operator
  const double                                      conint,    //!< concentration at GP
  const LINALG::Matrix<my::nsd_,1>&                 gradphi    //!< gradient of concentration at GP
  )
{
  for (int vi=0; vi<my::nen_; ++vi)
  {
    double laplawf(0.);
    my::GetLaplacianWeakFormRHS(laplawf,gradphi,vi);

    // use 2nd order pde derived from electroneutrality condition (k=1,...,m)
    // Inclusion of time integration factor results in a matrix with better condition number
    erhs[vi*my::numdofpernode_+my::numscal_] += rhsfac*dme->GetValence(k)*(dme->GetIsotropicDiff(k)*dme->GetValence(k)*conint*migconv(vi)-dme->GetIsotropicDiff(k)*laplawf);
  } // for vi

  return;
}


/*-------------------------------------------------------------------------------------------*
 |  CalcRhs: ENC in PDE form with NP equation for species m eliminated (private)  fang 05/14 |
 *-------------------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcRhsPotEquENCPDEElim(
  Epetra_SerialDenseVector&                         erhs,      //!< element vector to be filled
  const int                                         k,         //!< index of current scalar
  const double                                      rhsfac,    //!< time-integration factor for rhs times domain-integration factor
  Teuchos::RCP<ScaTraEleDiffManagerElch>&           dme,       //!< diffusion manager
  const LINALG::Matrix<my::nen_,1>&                 migconv,   //!< migration operator
  const double                                      conint,    //!< concentration at GP
  const LINALG::Matrix<my::nsd_,1>&                 gradphi    //!< gradient of concentration at GP
  )
{
  for (int vi=0; vi<my::nen_; ++vi)
  {
    const int pvi = vi*my::numdofpernode_+my::numscal_;

    double laplawf(0.);
    my::GetLaplacianWeakFormRHS(laplawf,gradphi,vi);

    // use 2nd order pde derived from electroneutrality condition (k=0,...,m-1)
    // Inclusion of time integration factor results in a matrix with better condition number
    erhs[pvi] += rhsfac*dme->GetValence(k)*(dme->GetIsotropicDiff(k)*dme->GetValence(k)*conint*migconv(vi)-dme->GetIsotropicDiff(k)*laplawf);

    // care for eliminated species with index m
    // Note: diffus_ and valence_ vectors were extended in GetMaterialParams() so that they
    // also contain the properties of the eliminated species at index m (= my::numscal_))
    erhs[pvi] -= rhsfac*dme->GetValence(k)*(dme->GetIsotropicDiff(my::numscal_)*dme->GetValence(my::numscal_)*conint*migconv(vi)-dme->GetIsotropicDiff(my::numscal_)*laplawf);
  } // for vi

  return;
}


/*-------------------------------------------------------------------------------------*
 |  CalcRhs: Poisson equation for electric potential (private)              fang 05/14 |
 *-------------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcRhsPotEquPoisson(
  Epetra_SerialDenseVector&                 erhs,      //!< element vector to be filled
  const int                                 k,         //!< index of current scalar
  const double                              fac,       //!< domain-integration factor
  const double                              epsilon,   //!< dielectric constant
  const double                              faraday,   //!< Faraday constant
  Teuchos::RCP<ScaTraEleDiffManagerElch>&   dme,       //!< diffusion manager
  const double                              conint,    //!< concentration at GP
  const LINALG::Matrix<my::nsd_,1>&         gradpot    //!< gradient of potential at GP
  )
{
  for (int vi=0; vi<my::nen_; ++vi)
  {
    const int pvi = vi*my::numdofpernode_+my::numscal_;

    // We have a loop over the species index k around. So prevent that the potential term is added more than once!
    if (k==0)
    {
      double laplawf(0.);
      my::GetLaplacianWeakFormRHS(laplawf,gradpot,vi);

      const double epsbyF = epsilon/faraday;

      erhs[pvi] -= fac*epsbyF*laplawf;
    }

    // residuum of Poisson equation on the rhs
    erhs[pvi] += dme->GetValence(k)*fac*my::funct_(vi)*conint;
  } // for vi

  return;
}


/*-------------------------------------------------------------------------------------*
 |  CalcRhs: Laplace equation for electric potential (private)              fang 05/14 |
 *-------------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcRhsPotEquLaplace(
  Epetra_SerialDenseVector&                 erhs,      //!< element vector to be filled
  const int                                 k,         //!< index of current scalar
  const double                              fac,       //!< domain-integration factor
  const LINALG::Matrix<my::nsd_,1>&         gradpot    //!< gradient of potential at GP
  )
{
  for (int vi=0; vi<my::nen_; ++vi)
  {
    const int pvi = vi*my::numdofpernode_+my::numscal_;

    // We have a loop over the species index k around. So prevent that the potential term is added more than once!
    if (k==0)
    {
      double laplawf(0.);
      my::GetLaplacianWeakFormRHS(laplawf,gradpot,vi);
      erhs[pvi] -= fac*laplawf;
    }
  } // for vi

  return;
}


/*------------------------------------------------------------------------*
 |  Correct sysmat for fluxes across DC                        fang 05/14 |
 *------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CorrectionForFluxAcrossDC(
  DRT::Discretization&        discretization,
  const std::vector<int>&     lm,
  Epetra_SerialDenseMatrix&   emat,
  Epetra_SerialDenseVector&   erhs)
{
  if((myelch::elchpara_->EquPot() == INPAR::ELCH::equpot_enc_pde) or
     (myelch::elchpara_->EquPot() == INPAR::ELCH::equpot_enc_pde_elim))
  {
    // get dirichlet toggle from the discretization
    Teuchos::RCP<const Epetra_Vector> dctoggle = discretization.GetState("dctoggle");
    std::vector<double> mydctoggle(lm.size());
    DRT::UTILS::ExtractMyValues(*dctoggle,mydctoggle,lm);

    // dynamic cast to elch-specific diffusion manager
    Teuchos::RCP<ScaTraEleDiffManagerElch> dme = Teuchos::rcp_dynamic_cast<ScaTraEleDiffManagerElch>(my::diffmanager_);

    double val = 0.;
    for (int vi=0; vi<my::nen_; ++vi)
    {
      for (int k=0; k<my::numscal_; ++k)
      {
        //
        if (mydctoggle[vi*my::numdofpernode_+k] == 1)
        {
          //std::cout<<"Ele Id = "<<ele->Id()<<"  Found one Dirichlet node for vi="<<vi<<std::endl;
          //std::cout<<"Dirichlet is on for k="<<k<<std::endl;
          //std::cout<<"k="<<k<<"  val="<<val<<" valence_k="<<valence_[k]<<std::endl;

          const int fvi = vi*my::numdofpernode_+k;

          // We use the fact, that the rhs vector value for boundary nodes
          // is equivalent to the integrated negative normal flux
          // due to diffusion and migration
          val = erhs[fvi];
          erhs[vi*my::numdofpernode_+my::numscal_] += dme->GetValence(k)*(-val);

          // corresponding linearization
          for (int ui=0; ui<my::nen_; ++ui)
          {
            val = emat(vi*my::numdofpernode_+k,ui*my::numdofpernode_+k);
            emat(vi*my::numdofpernode_+my::numscal_,ui*my::numdofpernode_+k)+=dme->GetValence(k)*(-val);
            val = emat(vi*my::numdofpernode_+k,ui*my::numdofpernode_+my::numscal_);
            emat(vi*my::numdofpernode_+my::numscal_,ui*my::numdofpernode_+my::numscal_)+=dme->GetValence(k)*(-val);
          }
        } // if mydctoggle
      } // for k
    } // for vi
  } // if scatratype

  return;
}


/*----------------------------------------------------------------------*
 |  set Nernst-Planck-specific variables in manager          fang 05/14 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::SetFormulationSpecificInternalVariables(
  Teuchos::RCP<ScaTraEleDiffManagerElch>&                                   dme,
  Teuchos::RCP<ScaTraEleInternalVariableManagerElch <my::nsd_,my::nen_> >&  vm
)
{
  // dynamic cast to elch Nernst-Planck-specific internal variable manager
  Teuchos::RCP<ScaTraEleInternalVariableManagerElchNP <my::nsd_,my::nen_> > vmnp =
      Teuchos::rcp_dynamic_cast< ScaTraEleInternalVariableManagerElchNP <my::nsd_,my::nen_> >(vm);

  vmnp->SetInternalVariablesElchNP(my::derxy_);

  return;
}


/*----------------------------------------------------------------------*
 |  get the material constants  (private)                     ehrl 01/14|
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::GetMaterialParams(
  const DRT::Element* ele,       //!< the element we are dealing with
  double&             densn,     //!< density at t_(n)
  double&             densnp,    //!< density at t_(n+1) or t_(n+alpha_F)
  double&             densam,    //!< density at t_(n+alpha_M)
  Teuchos::RCP<ScaTraEleDiffManager> diffmanager,  //!< diffusion manager handling diffusivity / diffusivities (in case of systems) or (thermal conductivity/specific heat) in case of loma
  Teuchos::RCP<ScaTraEleReaManager>  reamanager,   //!< reaction manager
  double&             visc,      //!< fluid viscosity
  const int           iquad      //!< id of current gauss point
  )
{
  // get the material
  Teuchos::RCP<MAT::Material> material = ele->Material();

  if(material->MaterialType() == INPAR::MAT::m_matlist)
  {
    const Teuchos::RCP<const MAT::MatList>& actmat
      = Teuchos::rcp_dynamic_cast<const MAT::MatList>(material);
    if (actmat->NumMat() < my::numscal_) dserror("Not enough materials in MatList.");

    for (int k = 0;k<my::numscal_;++k)
    {
      int matid = actmat->MatID(k);
      Teuchos::RCP< MAT::Material> singlemat = actmat->MaterialById(matid);

      Materials(singlemat,k,densn,densnp,densam,diffmanager,reamanager,visc,iquad);
    }
  }
  else
    dserror("");

  return;
} //ScaTraEleCalc::GetMaterialParams


/*----------------------------------------------------------------------*
 |  evaluate single material  (protected)                    ehrl 11/13 |
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::Materials(
  const Teuchos::RCP<const MAT::Material> material, //!< pointer to current material
  const int                               k,        //!< id of current scalar
  double&                                 densn,    //!< density at t_(n)
  double&                                 densnp,   //!< density at t_(n+1) or t_(n+alpha_F)
  double&                                 densam,   //!< density at t_(n+alpha_M)
  Teuchos::RCP<ScaTraEleDiffManager>  diffmanager,  //!< diffusion manager handling diffusivity / diffusivities (in case of systems) or (thermal conductivity/specific heat) in case of loma
  Teuchos::RCP<ScaTraEleReaManager>       reamanager,   //!< reaction manager
  double&                                 visc,         //!< fluid viscosity
  const int                               iquad         //!< id of current gauss point
  )
{
  if(material->MaterialType() == INPAR::MAT::m_ion)
    myelch::MatIon(material,k,densn,densnp,densam,Teuchos::rcp_dynamic_cast<ScaTraEleDiffManagerElch>(diffmanager),reamanager,visc,iquad);
  else dserror("Material type is not supported");

  return;
}


/*-----------------------------------------------------------------------------------------------------------------------*
 | Calculate derivative of tau w.r.t. electric potential according to Taylor, Hughes and Zarins (protected)   fang 06/14 |
 *-----------------------------------------------------------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::ScaTraEleCalcElchNP<distype>::CalcTauDerPotTaylorHughesZarins(
  LINALG::Matrix<my::nen_,1>&         tauderpot,       //!< derivatives of stabilization parameter w.r.t. electric potential
  double&                             tau,             //!< stabilization parameter
  const double                        densnp,          //!< density at t_(n+1)
  const double                        frt,             //!< F/(RT)
  const double                        diffusvalence,   //!< diffusion coefficient times valence
  const LINALG::Matrix<my::nsd_,1>&   veleff           //!< effective convective velocity (fluid velocity plus migration velocity if applicable)
  )
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
  */

  // Initialization
  tauderpot.Clear();

  // Compute entries of covariant metric tensor
  double G;
  const double dens_sqr = densnp*densnp;
  for (int nn=0; nn<my::nsd_; ++nn)
  {
    for (int rr=0; rr<my::nsd_; ++rr)
    {
      G = my::xij_(nn,0)*my::xij_(rr,0);

      for(int tt=1; tt<my::nsd_; ++tt)
        G += my::xij_(nn,tt)*my::xij_(rr,tt);

      for (int jj=0; jj<my::nen_; ++jj)
        tauderpot(jj) += dens_sqr*frt*diffusvalence*((my::derxy_(nn,jj)*G*veleff(rr,0))+(veleff(nn,0)*G*my::derxy_(rr,jj)));
    }
  }

  // Finalize derivative of present tau w.r.t electric potential
  // Note: Factor alpha_f in gen-alpha time integration scheme is included at a later point
  tauderpot.Scale(0.5*tau*tau*tau);

  return;
}


// template classes

// 1D elements
template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::line2>;
template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::line3>;

// 2D elements
//template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::tri3>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::tri6>;
template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::quad4>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::quad8>;
template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::quad9>;
template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::nurbs9>;

// 3D elements
template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::hex8>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::hex20>;
template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::hex27>;
template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::tet4>;
template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::tet10>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::wedge6>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::pyramid5>;
//template class DRT::ELEMENTS::ScaTraEleCalcElchNP<DRT::Element::nurbs27>;



