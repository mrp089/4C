/*----------------------------------------------------------------------*/
/*! \file
\brief input parameters for VTP output of structural problem at runtime

\level 2

*/
/*----------------------------------------------------------------------*/

#include "inpar_IO_runtime_vtp_output_structure.H"

#include "inpar_validparameters.H"
#include "inpar.H"
#include "inpar_parameterlist_utils.H"

#include <Teuchos_ParameterList.hpp>

namespace INPAR
{
  namespace IO_RUNTIME_VTP_STRUCTURE
  {
    /*----------------------------------------------------------------------*
     *----------------------------------------------------------------------*/
    void SetValidParameters(Teuchos::RCP<Teuchos::ParameterList> list)
    {
      using namespace DRT::INPUT;
      using Teuchos::setStringToIntegralParameter;
      using Teuchos::tuple;

      // related sublist
      Teuchos::ParameterList& sublist_IO = list->sublist("IO", false, "");
      Teuchos::ParameterList& sublist_IO_VTP_structure =
          sublist_IO.sublist("RUNTIME VTP OUTPUT STRUCTURE", false, "");


      // output interval regarding steps: write output every INTERVAL_STEPS steps
      IntParameter("INTERVAL_STEPS", -1, "write VTP output at runtime every INTERVAL_STEPS steps",
          &sublist_IO_VTP_structure);


      // data format for written numeric data
      setStringToIntegralParameter<int>("OUTPUT_DATA_FORMAT", "binary",
          "data format for written numeric data",
          tuple<std::string>("binary", "Binary", "ascii", "ASCII"),
          tuple<int>(INPAR::IO_RUNTIME_VTP_STRUCTURE::binary,
              INPAR::IO_RUNTIME_VTP_STRUCTURE::binary, INPAR::IO_RUNTIME_VTP_STRUCTURE::ascii,
              INPAR::IO_RUNTIME_VTP_STRUCTURE::ascii),
          &sublist_IO_VTP_structure);


      // whether to write output in every iteration of the nonlinear solver
      BoolParameter("EVERY_ITERATION", "No",
          "write output in every iteration of the nonlinear solver", &sublist_IO_VTP_structure);

      // write owner at every visualization point
      BoolParameter("OWNER", "No", "write owner of every point", &sublist_IO_VTP_structure);

      // write orientation at every visualization point
      BoolParameter("ORIENTATIONANDLENGTH", "No", "write orientation at every point",
          &sublist_IO_VTP_structure);

      // write number of bonds at every visualization point
      BoolParameter(
          "NUMBEROFBONDS", "No", "write number of bonds of every point", &sublist_IO_VTP_structure);

      // write force actin in linker
      BoolParameter(
          "LINKINGFORCE", "No", "write force acting in linker", &sublist_IO_VTP_structure);
    }


  }  // namespace IO_RUNTIME_VTP_STRUCTURE
}  // namespace INPAR