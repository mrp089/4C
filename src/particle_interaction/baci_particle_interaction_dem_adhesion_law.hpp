/*---------------------------------------------------------------------------*/
/*! \file
\brief adhesion law handler for discrete element method (DEM) interactions
\level 3
*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 | definitions                                                               |
 *---------------------------------------------------------------------------*/
#ifndef BACI_PARTICLE_INTERACTION_DEM_ADHESION_LAW_HPP
#define BACI_PARTICLE_INTERACTION_DEM_ADHESION_LAW_HPP

/*---------------------------------------------------------------------------*
 | headers                                                                   |
 *---------------------------------------------------------------------------*/
#include "baci_config.hpp"

#include <Teuchos_ParameterList.hpp>

#include <memory>

BACI_NAMESPACE_OPEN

/*---------------------------------------------------------------------------*
 | class declarations                                                        |
 *---------------------------------------------------------------------------*/
namespace PARTICLEINTERACTION
{
  class DEMAdhesionLawBase
  {
   public:
    //! constructor
    explicit DEMAdhesionLawBase(const Teuchos::ParameterList& params);

    //! virtual destructor
    virtual ~DEMAdhesionLawBase() = default;

    //! init adhesion law handler
    virtual void Init();

    //! setup adhesion law handler
    virtual void Setup(const double& k_normal);

    //! calculate adhesion force
    virtual void AdhesionForce(const double& gap, const double& surfaceenergy, const double& r_eff,
        const double& v_rel_normal, const double& m_eff, double& adhesionforce) const = 0;

   protected:
    //! discrete element method parameter list
    const Teuchos::ParameterList& params_dem_;

    //! factor to calculate minimum adhesion surface energy
    const double adhesion_surface_energy_factor_;

    //! adhesion maximum contact pressure
    const double adhesion_max_contact_pressure_;

    //! adhesion maximum contact force
    const double adhesion_max_contact_force_;

    //! use maximum contact force instead of maximum contact pressure
    const bool adhesion_use_max_contact_force_;

    //! factor for calculation of maximum contact force using maximum contact pressure
    double adhesion_max_contact_force_fac_;

    //! shift van-der-Waals-curve to g = 0
    const bool adhesion_vdW_curve_shift_;

    //! inverse normal contact stiffness
    double inv_k_normal_;
  };

  class DEMAdhesionLawVdWDMT : public DEMAdhesionLawBase
  {
   public:
    //! constructor
    explicit DEMAdhesionLawVdWDMT(const Teuchos::ParameterList& params);

    //! init adhesion law handler
    void Init() override;

    //! calculate adhesion force
    void AdhesionForce(const double& gap, const double& surfaceenergy, const double& r_eff,
        const double& v_rel_normal, const double& m_eff, double& adhesionforce) const override;

   private:
    //! calculate gap at which vdW-curve intersects linear ramp
    void CalculateIntersectionGap(
        double a, double b, double c, double d, double& gap_intersect) const;

    //! hamaker constant
    const double hamaker_constant;
  };

  class DEMAdhesionLawRegDMT : public DEMAdhesionLawBase
  {
   public:
    //! constructor
    explicit DEMAdhesionLawRegDMT(const Teuchos::ParameterList& params);

    //! calculate adhesion force
    void AdhesionForce(const double& gap, const double& surfaceenergy, const double& r_eff,
        const double& v_rel_normal, const double& m_eff, double& adhesionforce) const override;

   private:
    //! adhesion distance
    const double adhesion_distance_;
  };

}  // namespace PARTICLEINTERACTION

/*---------------------------------------------------------------------------*/
BACI_NAMESPACE_CLOSE

#endif