/*-----------------------------------------------------------------------------------------------*/
/*! \file

\brief Define a container that holds the visualization data and the visualization writer

\level 0

*/
/*-----------------------------------------------------------------------------------------------*/

#include "baci_io_visualization_writer_factory.H"

#include "baci_inpar_IO_runtime_output.H"
#include "baci_io_visualization_writer_vtu_per_rank.H"
#include "baci_utils_exceptions.H"

BACI_NAMESPACE_OPEN


/**
 *
 */
std::unique_ptr<IO::VisualizationWriterBase> IO::VisualizationWriterFactory(
    const VisualizationParameters& parameters, const Epetra_Comm& comm,
    const std::string& visualization_data_name)
{
  if (parameters.writer_ == INPAR::IO_RUNTIME_OUTPUT::OutputWriter::vtu_per_rank)
  {
    return std::make_unique<VisualizationWriterVtuPerRank>(
        parameters, comm, visualization_data_name);
  }
  else
  {
    dserror("You have to select a valid visualization writer in the input file");
  }
}
BACI_NAMESPACE_CLOSE
