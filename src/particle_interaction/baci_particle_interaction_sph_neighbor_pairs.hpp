/*---------------------------------------------------------------------------*/
/*! \file
\brief neighbor pair handler for smoothed particle hydrodynamics (SPH) interactions
\level 3
*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 | definitions                                                               |
 *---------------------------------------------------------------------------*/
#ifndef BACI_PARTICLE_INTERACTION_SPH_NEIGHBOR_PAIRS_HPP
#define BACI_PARTICLE_INTERACTION_SPH_NEIGHBOR_PAIRS_HPP

/*---------------------------------------------------------------------------*
 | headers                                                                   |
 *---------------------------------------------------------------------------*/
#include "baci_config.hpp"

#include "baci_particle_engine_enums.hpp"
#include "baci_particle_engine_typedefs.hpp"
#include "baci_particle_interaction_sph_neighbor_pair_struct.hpp"

BACI_NAMESPACE_OPEN

/*---------------------------------------------------------------------------*
 | forward declarations                                                      |
 *---------------------------------------------------------------------------*/
namespace PARTICLEENGINE
{
  class ParticleEngineInterface;
  class ParticleContainerBundle;
}  // namespace PARTICLEENGINE

namespace PARTICLEWALL
{
  class WallHandlerInterface;
}

namespace PARTICLEINTERACTION
{
  class SPHKernelBase;
}

/*---------------------------------------------------------------------------*
 | type definitions                                                          |
 *---------------------------------------------------------------------------*/
namespace PARTICLEINTERACTION
{
  using SPHParticlePairData = std::vector<PARTICLEINTERACTION::SPHParticlePair>;
  using SPHParticleWallPairData = std::vector<PARTICLEINTERACTION::SPHParticleWallPair>;
  using SPHIndexOfParticlePairs = std::vector<std::vector<std::vector<int>>>;
  using SPHIndexOfParticleWallPairs = std::vector<std::vector<int>>;
}  // namespace PARTICLEINTERACTION

/*---------------------------------------------------------------------------*
 | class declarations                                                        |
 *---------------------------------------------------------------------------*/
namespace PARTICLEINTERACTION
{
  class SPHNeighborPairs final
  {
   public:
    //! constructor
    explicit SPHNeighborPairs();

    //! init neighbor pair handler
    void Init();

    //! setup neighbor pair handler
    void Setup(
        const std::shared_ptr<PARTICLEENGINE::ParticleEngineInterface> particleengineinterface,
        const std::shared_ptr<PARTICLEWALL::WallHandlerInterface> particlewallinterface,
        const std::shared_ptr<PARTICLEINTERACTION::SPHKernelBase> kernel);

    //! get reference to particle pair data
    inline const SPHParticlePairData& GetRefToParticlePairData() const
    {
      return particlepairdata_;
    };

    //! get reference to particle-wall pair data
    inline const SPHParticleWallPairData& GetRefToParticleWallPairData() const
    {
      return particlewallpairdata_;
    };

    //! get relevant particle pair indices for disjoint combination of particle types
    void GetRelevantParticlePairIndicesForDisjointCombination(
        const std::set<PARTICLEENGINE::TypeEnum>& types_a,
        const std::set<PARTICLEENGINE::TypeEnum>& types_b, std::vector<int>& relindices) const;

    //! get relevant particle pair indices for equal combination of particle types
    void GetRelevantParticlePairIndicesForEqualCombination(
        const std::set<PARTICLEENGINE::TypeEnum>& types_a, std::vector<int>& relindices) const;

    //! get relevant particle wall pair indices for specific particle types
    void GetRelevantParticleWallPairIndices(
        const std::set<PARTICLEENGINE::TypeEnum>& types_a, std::vector<int>& relindices) const;

    //! evaluate neighbor pairs
    void EvaluateNeighborPairs();

   private:
    //! evaluate particle pairs
    void EvaluateParticlePairs();

    //! evaluate particle-wall pairs
    void EvaluateParticleWallPairs();

    //! particle pair data with evaluated quantities
    SPHParticlePairData particlepairdata_;

    //! particle-wall pair data with evaluated quantities
    SPHParticleWallPairData particlewallpairdata_;

    //! index of particle pairs for each type
    SPHIndexOfParticlePairs indexofparticlepairs_;

    //! index of particle-wall pairs for each type
    SPHIndexOfParticleWallPairs indexofparticlewallpairs_;

    //! interface to particle engine
    std::shared_ptr<PARTICLEENGINE::ParticleEngineInterface> particleengineinterface_;

    //! particle container bundle
    PARTICLEENGINE::ParticleContainerBundleShrdPtr particlecontainerbundle_;

    //! interface to particle wall handler
    std::shared_ptr<PARTICLEWALL::WallHandlerInterface> particlewallinterface_;

    //! kernel handler
    std::shared_ptr<PARTICLEINTERACTION::SPHKernelBase> kernel_;
  };

}  // namespace PARTICLEINTERACTION

/*---------------------------------------------------------------------------*/
BACI_NAMESPACE_CLOSE

#endif