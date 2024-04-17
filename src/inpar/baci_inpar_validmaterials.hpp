/*----------------------------------------------------------------------*/
/*! \file
\brief Setup of the list of valid materials for input

\level 1


*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* definitions */


#ifndef FOUR_C_INPAR_VALIDMATERIALS_HPP
#define FOUR_C_INPAR_VALIDMATERIALS_HPP

/*----------------------------------------------------------------------*/
/* headers */
#include "baci_config.hpp"

#include <Teuchos_Array.hpp>
#include <Teuchos_RCP.hpp>

#include <iostream>
#include <string>
#include <vector>

FOUR_C_NAMESPACE_OPEN



namespace INPUT
{
  class MaterialDefinition;

  /// construct list with all materials and documentation
  Teuchos::RCP<std::vector<Teuchos::RCP<INPUT::MaterialDefinition>>> ValidMaterials();

  /// print all known material sections without contents
  void PrintEmptyMaterialDefinitions(
      std::ostream& stream, std::vector<Teuchos::RCP<INPUT::MaterialDefinition>>& matlist);
}  // namespace INPUT

/// print empty material sections
void PrintMaterialDatHeader();


FOUR_C_NAMESPACE_CLOSE

#endif
