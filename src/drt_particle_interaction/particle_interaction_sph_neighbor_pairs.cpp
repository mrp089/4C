/*---------------------------------------------------------------------------*/
/*!
\brief neighbor pair handler for smoothed particle hydrodynamics (SPH) interactions

\level 3

\maintainer  Sebastian Fuchs
*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 | headers                                                    sfuchs 08/2018 |
 *---------------------------------------------------------------------------*/
#include "particle_interaction_sph_neighbor_pairs.H"

#include "particle_interaction_sph_kernel.H"

#include "particle_interaction_utils.H"

#include "../drt_particle_engine/particle_engine_interface.H"
#include "../drt_particle_engine/particle_container.H"

#include "../drt_lib/drt_dserror.H"

#include <Teuchos_TimeMonitor.hpp>

/*---------------------------------------------------------------------------*
 | constructor                                                sfuchs 08/2018 |
 *---------------------------------------------------------------------------*/
PARTICLEINTERACTION::SPHNeighborPairs::SPHNeighborPairs()
{
  // empty constructor
}

/*---------------------------------------------------------------------------*
 | init neighbor pair handler                                 sfuchs 08/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEINTERACTION::SPHNeighborPairs::Init()
{
  // nothing to do
}

/*---------------------------------------------------------------------------*
 | setup neighbor pair handler                                sfuchs 08/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEINTERACTION::SPHNeighborPairs::Setup(
    const std::shared_ptr<PARTICLEENGINE::ParticleEngineInterface> particleengineinterface,
    const std::shared_ptr<PARTICLEWALL::WallHandlerInterface> particlewallinterface,
    const std::shared_ptr<PARTICLEINTERACTION::SPHKernelBase> kernel)
{
  // set interface to particle engine
  particleengineinterface_ = particleengineinterface;

  // set particle container bundle
  particlecontainerbundle_ = particleengineinterface_->GetParticleContainerBundle();

  // set interface to particle wall handler
  particlewallinterface_ = particlewallinterface;

  // set kernel handler
  kernel_ = kernel;

  // determine size of vectors indexed by particle types
  const int typevectorsize = *(--particlecontainerbundle_->GetParticleTypes().end()) + 1;

  // allocate memory to hold index of particle pairs for each type
  indexofparticlepairs_.resize(typevectorsize);
}

/*---------------------------------------------------------------------------*
 | write restart of neighbor pair handler                     sfuchs 08/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEINTERACTION::SPHNeighborPairs::WriteRestart(const int step, const double time) const
{
  // nothing to do
}

/*---------------------------------------------------------------------------*
 | read restart of neighbor pair handler                      sfuchs 08/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEINTERACTION::SPHNeighborPairs::ReadRestart(
    const std::shared_ptr<IO::DiscretizationReader> reader)
{
  // nothing to do
}

/*---------------------------------------------------------------------------*
 | get relevant particle pair indices for particle types      sfuchs 04/2019 |
 *---------------------------------------------------------------------------*/
void PARTICLEINTERACTION::SPHNeighborPairs::GetRelevantParticlePairIndices(
    const std::set<PARTICLEENGINE::TypeEnum>& reltypes, std::vector<int>& relindices) const
{
  // iterate over particle types to consider
  for (const auto& type_i : reltypes)
    relindices.insert(relindices.end(), indexofparticlepairs_[type_i].begin(),
        indexofparticlepairs_[type_i].end());

  // sort and erase duplicate indices of relevant particle pairs
  if (reltypes.size() > 1)
  {
    std::sort(relindices.begin(), relindices.end());
    relindices.erase(std::unique(relindices.begin(), relindices.end()), relindices.end());
  }
}

/*---------------------------------------------------------------------------*
 | evaluate neighbor pairs                                    sfuchs 06/2019 |
 *---------------------------------------------------------------------------*/
void PARTICLEINTERACTION::SPHNeighborPairs::EvaluateNeighborPairs()
{
  // evaluate particle pairs
  EvaluateParticlePairs();

  // evaluate particle-wall pairs
  if (particlewallinterface_) EvaluateParticleWallPairs();
}

/*---------------------------------------------------------------------------*
 | evaluate particle pairs                                    sfuchs 08/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEINTERACTION::SPHNeighborPairs::EvaluateParticlePairs()
{
  TEUCHOS_FUNC_TIME_MONITOR("PARTICLEINTERACTION::SPHNeighborPairs::EvaluateParticlePairs");

  // clear particle pair data
  particlepairdata_.clear();

  // clear index of particle pairs for each type
  for (const auto& type_i : particlecontainerbundle_->GetParticleTypes())
    indexofparticlepairs_[type_i].clear();

  // index of particle pairs
  int particlepairindex = 0;

  // iterate over potential particle neighbors
  for (auto& potentialneighbors : particleengineinterface_->GetPotentialParticleNeighbors())
  {
    // access values of local index tuples of particle i and j
    PARTICLEENGINE::TypeEnum type_i;
    PARTICLEENGINE::StatusEnum status_i;
    int particle_i;
    std::tie(type_i, status_i, particle_i) = potentialneighbors.first;

    PARTICLEENGINE::TypeEnum type_j;
    PARTICLEENGINE::StatusEnum status_j;
    int particle_j;
    std::tie(type_j, status_j, particle_j) = potentialneighbors.second;

    if (type_i == PARTICLEENGINE::BoundaryPhase and type_j == PARTICLEENGINE::BoundaryPhase)
      continue;

    // get corresponding particle containers
    PARTICLEENGINE::ParticleContainer* container_i =
        particlecontainerbundle_->GetSpecificContainer(type_i, status_i);

    PARTICLEENGINE::ParticleContainer* container_j =
        particlecontainerbundle_->GetSpecificContainer(type_j, status_j);

    // declare pointer variables for particle i and j
    const double *pos_i, *rad_i;
    const double *pos_j, *rad_j;

    // get pointer to particle states
    pos_i = container_i->GetPtrToParticleState(PARTICLEENGINE::Position, particle_i);
    rad_i = container_i->GetPtrToParticleState(PARTICLEENGINE::Radius, particle_i);

    pos_j = container_j->GetPtrToParticleState(PARTICLEENGINE::Position, particle_j);
    rad_j = container_j->GetPtrToParticleState(PARTICLEENGINE::Radius, particle_j);

    // vector from particle i to j
    double r_ji[3];

    // distance between particles considering periodic boundaries
    particleengineinterface_->DistanceBetweenParticles(pos_i, pos_j, r_ji);

    // absolute distance between particles
    const double absdist = UTILS::vec_norm2(r_ji);

    if (absdist < rad_i[0] or (absdist < rad_j[0] and status_j == PARTICLEENGINE::Owned))
    {
      // initialize particle pair
      particlepairdata_.push_back(SPHParticlePair());

      // get reference to current particle pair
      SPHParticlePair& particlepair = particlepairdata_[particlepairindex];

      // store index of particle pairs for each type (owned and ghosted status)
      indexofparticlepairs_[type_i].push_back(particlepairindex);
      if (type_i != type_j) indexofparticlepairs_[type_j].push_back(particlepairindex);

      // increase index
      ++particlepairindex;

      // set local index tuple of particles i and j
      particlepair.tuple_i_ = potentialneighbors.first;
      particlepair.tuple_j_ = potentialneighbors.second;

      // set absolute distance between particles
      particlepair.absdist_ = absdist;

      // versor from particle j to i
      UTILS::vec_setscale(particlepair.e_ij_, -1.0 / absdist, r_ji);

      // particle j within support radius of particle i
      if (absdist < rad_i[0])
      {
        // evaluate kernel
        particlepair.Wij_ = kernel_->W(absdist, rad_i[0]);

        // evaluate first derivative of kernel
        particlepair.dWdrij_ = kernel_->dWdrij(absdist, rad_i[0]);
      }

      // particle i within support radius of owned particle j
      if (absdist < rad_j[0] and status_j == PARTICLEENGINE::Owned)
      {
        // equal support radius for particle i and j
        if (rad_i[0] == rad_j[0])
        {
          // evaluate kernel
          particlepair.Wji_ = particlepair.Wij_;

          // evaluate first derivative of kernel
          particlepair.dWdrji_ = particlepair.dWdrij_;
        }
        else
        {
          // evaluate kernel
          particlepair.Wji_ = kernel_->W(absdist, rad_j[0]);

          // evaluate first derivative of kernel
          particlepair.dWdrji_ = kernel_->dWdrij(absdist, rad_j[0]);
        }
      }
    }
  }
}

/*---------------------------------------------------------------------------*
 | evaluate particle-wall pairs                               sfuchs 06/2019 |
 *---------------------------------------------------------------------------*/
void PARTICLEINTERACTION::SPHNeighborPairs::EvaluateParticleWallPairs()
{
  TEUCHOS_FUNC_TIME_MONITOR("PARTICLEINTERACTION::SPHNeighborPairs::EvaluateParticleWallPairs");

  // clear particle-wall pair data
  particlewallpairdata_.clear();
}
