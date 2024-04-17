/*---------------------------------------------------------------------------*/
/*! \file

\brief functionality to read particles from file

\level 3


*/
/*---------------------------------------------------------------------------*/

#ifndef FOUR_C_PARTICLE_ENGINE_PARTICLEREADER_HPP
#define FOUR_C_PARTICLE_ENGINE_PARTICLEREADER_HPP

#include "baci_config.hpp"

#include "baci_io_inputreader.hpp"
#include "baci_particle_engine_typedefs.hpp"

FOUR_C_NAMESPACE_OPEN

namespace INPUT
{
  class ParticleReader
  {
   public:
    //! construct a reader that reads a given section
    ParticleReader(const DatFileReader& reader, std::string sectionname);

    //! do the actual reading of particles
    void Read(std::vector<PARTICLEENGINE::ParticleObjShrdPtr>& particles);

   private:
    //! the main dat file reader
    const DatFileReader& reader_;

    //! my comm
    Teuchos::RCP<Epetra_Comm> comm_;

    //! my section to read
    std::string sectionname_;
  };

}  // namespace INPUT

FOUR_C_NAMESPACE_CLOSE

#endif
