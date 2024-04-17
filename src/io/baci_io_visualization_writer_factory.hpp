/*-----------------------------------------------------------------------------------------------*/
/*! \file

\brief Factory that creates the visualization writer that is specified in the input file

\level 0

*/
/*-----------------------------------------------------------------------------------------------*/

#ifndef FOUR_C_IO_VISUALIZATION_WRITER_FACTORY_HPP
#define FOUR_C_IO_VISUALIZATION_WRITER_FACTORY_HPP


#include "baci_config.hpp"

#include "baci_io_visualization_parameters.hpp"
#include "baci_io_visualization_writer_base.hpp"

#include <Epetra_Comm.h>

#include <memory>

FOUR_C_NAMESPACE_OPEN

namespace IO
{
  /**
   * @brief Creates the visualization writer that is specified in the parameters object
   */
  [[nodiscard]] std::unique_ptr<VisualizationWriterBase> VisualizationWriterFactory(
      const VisualizationParameters& parameters, const Epetra_Comm& comm,
      const std::string& visualization_data_name);
}  // namespace IO

FOUR_C_NAMESPACE_CLOSE

#endif