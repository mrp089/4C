/*-----------------------------------------------------------*/
/*! \file

\brief Class to assemble pair based contributions into global matrices.


\level 1

*/
/*-----------------------------------------------------------*/


#include "baci_fbi_partitioned_penaltycoupling_assembly_manager.H"

#include "baci_beaminteraction_contact_pair.H"

BACI_NAMESPACE_OPEN


/**
 *
 */
BEAMINTERACTION::SUBMODELEVALUATOR::PartitionedBeamInteractionAssemblyManager::
    PartitionedBeamInteractionAssemblyManager(
        std::vector<Teuchos::RCP<BEAMINTERACTION::BeamContactPair>>& assembly_contact_elepairs)
    : assembly_contact_elepairs_(assembly_contact_elepairs)
{
}

BACI_NAMESPACE_CLOSE
