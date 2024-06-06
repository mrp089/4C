/*---------------------------------------------------------------------------*/
/*! \file
\brief adhesion surface energy handler for discrete element method (DEM) interactions
\level 3
*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 | definitions                                                               |
 *---------------------------------------------------------------------------*/
#ifndef FOUR_C_PARTICLE_INTERACTION_DEM_ADHESION_SURFACE_ENERGY_HPP
#define FOUR_C_PARTICLE_INTERACTION_DEM_ADHESION_SURFACE_ENERGY_HPP

/*---------------------------------------------------------------------------*
 | headers                                                                   |
 *---------------------------------------------------------------------------*/
#include "4C_config.hpp"

#include <Teuchos_ParameterList.hpp>

#include <memory>

FOUR_C_NAMESPACE_OPEN

/*---------------------------------------------------------------------------*
 | class declarations                                                        |
 *---------------------------------------------------------------------------*/
namespace ParticleInteraction
{
  class DEMAdhesionSurfaceEnergyBase
  {
   public:
    //! constructor
    explicit DEMAdhesionSurfaceEnergyBase(const Teuchos::ParameterList& params);

    //! virtual destructor
    virtual ~DEMAdhesionSurfaceEnergyBase() = default;

    //! init adhesion surface energy handler
    virtual void Init();

    //! setup adhesion surface energy handler
    virtual void Setup();

    //! calculate adhesion surface energy
    virtual void adhesion_surface_energy(
        const double& mean_surface_energy, double& surface_energy) const = 0;

   protected:
    //! discrete element method parameter list
    const Teuchos::ParameterList& params_dem_;
  };

  class DEMAdhesionSurfaceEnergyConstant : public DEMAdhesionSurfaceEnergyBase
  {
   public:
    //! constructor
    explicit DEMAdhesionSurfaceEnergyConstant(const Teuchos::ParameterList& params);

    //! get adhesion surface energy
    void adhesion_surface_energy(
        const double& mean_surface_energy, double& surface_energy) const override
    {
      surface_energy = mean_surface_energy;
    };
  };

  class DEMAdhesionSurfaceEnergyDistributionBase : public DEMAdhesionSurfaceEnergyBase
  {
   public:
    //! constructor
    explicit DEMAdhesionSurfaceEnergyDistributionBase(const Teuchos::ParameterList& params);

    //! setup adhesion surface energy handler
    void Setup() override;

    //! get adhesion surface energy
    void adhesion_surface_energy(
        const double& mean_surface_energy, double& surface_energy) const override = 0;

   protected:
    //! adjust surface energy to allowed bounds
    void adjust_surface_energy_to_allowed_bounds(
        const double& mean_surface_energy, double& surface_energy) const;

    //! variance of adhesion surface energy distribution
    const double variance_;

    //! cutoff factor of adhesion surface energy to determine minimum and maximum value
    const double cutofffactor_;
  };

  class DEMAdhesionSurfaceEnergyDistributionNormal : public DEMAdhesionSurfaceEnergyDistributionBase
  {
   public:
    //! constructor
    explicit DEMAdhesionSurfaceEnergyDistributionNormal(const Teuchos::ParameterList& params);

    //! get adhesion surface energy
    void adhesion_surface_energy(
        const double& mean_surface_energy, double& surface_energy) const override;
  };

  class DEMAdhesionSurfaceEnergyDistributionLogNormal
      : public DEMAdhesionSurfaceEnergyDistributionBase
  {
   public:
    //! constructor
    explicit DEMAdhesionSurfaceEnergyDistributionLogNormal(const Teuchos::ParameterList& params);

    //! get adhesion surface energy
    void adhesion_surface_energy(
        const double& mean_surface_energy, double& surface_energy) const override;
  };

}  // namespace ParticleInteraction

/*---------------------------------------------------------------------------*/
FOUR_C_NAMESPACE_CLOSE

#endif
