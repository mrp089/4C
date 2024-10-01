/*----------------------------------------------------------------------*/
/*! \file

\brief Filter methods for the dynamic Smagorinsky model

Reference:

    Subgrid viscosity:
    D. You, P. Moin:
    A dynamic global-coefficient
    subgrid-scale eddy-viscosity model for large-eddy simulation
    in complex geometries
    (Phys. Fluids 2007)

    Subgrid diffusivity:
    D. You, P. Moin:
    A dynamic global-coefficient subgrid-scale odel for large-eddy simulation
    of turbulent scalar transport in complex geometries
    (Phys. Fluids 2009)

    Remark:
    alphaij=transpose(velderxy)
    rows and colums of the jacobian alphaij are swapped in comparison
    to the jacobian velderxy used in 4C routines for bfda test case


\level 2

*/
/*----------------------------------------------------------------------*/

#ifndef FOUR_C_FLUID_TURBULENCE_DYN_VREMAN_HPP
#define FOUR_C_FLUID_TURBULENCE_DYN_VREMAN_HPP


#include "4C_config.hpp"

#include "4C_fem_discretization.hpp"
#include "4C_inpar_fluid.hpp"
#include "4C_inpar_scatra.hpp"
#include "4C_linalg_utils_sparse_algebra_math.hpp"
#include "4C_linalg_vector.hpp"
#include "4C_utils_parameter_list.fwd.hpp"

#include <Teuchos_RCP.hpp>
#include <Teuchos_TimeMonitor.hpp>

FOUR_C_NAMESPACE_OPEN



namespace FLD
{
  class Boxfilter;

  class Vreman
  {
   public:
    /*!
    \brief Standard Constructor (public)

    */
    Vreman(Teuchos::RCP<Core::FE::Discretization> actdis, Teuchos::ParameterList& params);

    /*!
    \brief Destructor

    */
    virtual ~Vreman() = default;



    void apply_filter_for_dynamic_computation_of_cv(
        const Teuchos::RCP<const Core::LinAlg::Vector<double>> velocity,
        const Teuchos::RCP<const Core::LinAlg::Vector<double>> scalar, const double thermpress,
        const Teuchos::RCP<const Core::LinAlg::Vector<double>> dirichtoggle);

    void apply_filter_for_dynamic_computation_of_dt(
        const Teuchos::RCP<const Core::LinAlg::Vector<double>> scalar, const double thermpress,
        const Teuchos::RCP<const Core::LinAlg::Vector<double>> dirichtoggle,
        Teuchos::ParameterList& extraparams, const int ndsvel);

    void add_scatra(Teuchos::RCP<Core::FE::Discretization> scatradis);

    void get_cv(double& Cv)
    {
      Cv = Cv_;
      return;
    }

    double Cv_;

   private:
    /// provide access to the box filter
    Teuchos::RCP<FLD::Boxfilter> boxfilter();
    // Boxfilter
    Teuchos::RCP<FLD::Boxfilter> boxf_;
    Teuchos::RCP<FLD::Boxfilter> boxfsc_;



    //! the discretization
    Teuchos::RCP<Core::FE::Discretization> discret_;
    //! parameterlist including time params, stabilization params and turbulence sublist
    Teuchos::ParameterList& params_;
    //! flag for physical type of fluid flow
    Inpar::FLUID::PhysicalType physicaltype_;
    // scatra specific
    Teuchos::RCP<Core::FE::Discretization> scatradiscret_;


    double dyn_vreman_compute_cv();

    void dyn_vreman_compute_dt(Teuchos::ParameterList& extraparams);

    //@}

    //! @name vectors used for filtering (for dynamic Smagorinsky model)
    //        --------------------------

    //! the filtered reystress exported to column map
    Teuchos::RCP<Epetra_MultiVector> col_filtered_strainrate_;
    Teuchos::RCP<Core::LinAlg::Vector<double>> col_filtered_expression_;
    Teuchos::RCP<Epetra_MultiVector> col_filtered_alphaij_;
    Teuchos::RCP<Core::LinAlg::Vector<double>> col_filtered_alpha2_;
    Teuchos::RCP<Epetra_MultiVector> col_filtered_phi_;
    Teuchos::RCP<Core::LinAlg::Vector<double>> col_filtered_phi2_;
    Teuchos::RCP<Core::LinAlg::Vector<double>> col_filtered_phiexpression_;
    Teuchos::RCP<Epetra_MultiVector> col_filtered_alphaijsc_;
    //@}

    //! @name homogeneous flow specials
    //        -------------------------------

    //@}

  };  // end class Vreman

}  // end namespace FLD

FOUR_C_NAMESPACE_CLOSE

#endif
