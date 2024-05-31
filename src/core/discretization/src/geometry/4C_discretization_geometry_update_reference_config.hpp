/*---------------------------------------------------------------------*/
/*! \file

\brief A collection of helper methods for namespace DRT

\level 1


*/
/*---------------------------------------------------------------------*/

#ifndef FOUR_C_DISCRETIZATION_GEOMETRY_UPDATE_REFERENCE_CONFIG_HPP
#define FOUR_C_DISCRETIZATION_GEOMETRY_UPDATE_REFERENCE_CONFIG_HPP


#include "4C_config.hpp"

#include "4C_discretization_fem_general_element.hpp"
#include "4C_discretization_fem_general_node.hpp"

#include <Teuchos_RCP.hpp>

FOUR_C_NAMESPACE_OPEN

namespace CORE::GEO
{
  //! Update material configuration of @p dis with @p disp
  void update_reference_config_with_disp(
      Teuchos::RCP<const DRT::Discretization> dis, Teuchos::RCP<const Epetra_Vector> disp);
}  // namespace CORE::GEO

FOUR_C_NAMESPACE_CLOSE

#endif