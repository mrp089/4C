/*-----------------------------------------------------------*/
/*! \file

\brief input parameter for rebalancing the discretization

\level 2

*/
/*-----------------------------------------------------------*/

#ifndef FOUR_C_INPAR_REBALANCE_HPP
#define FOUR_C_INPAR_REBALANCE_HPP

#include "baci_config.hpp"

#include "baci_utils_parameter_list.hpp"

FOUR_C_NAMESPACE_OPEN

namespace INPAR::REBALANCE
{
  enum class RebalanceType
  {
    none,                            //< no partitioning method
    hypergraph,                      //< hypergraph based partitioning
    recursive_coordinate_bisection,  //< recursive coordinate bisection, geometric based
                                     // partitioning
    monolithic  //< hypergraph based partitioning by using a global monolithic graph constructed
                // via a global collision search
  };

  //! set the parameters for the geometric search strategy
  void SetValidParameters(Teuchos::RCP<Teuchos::ParameterList> list);
}  // namespace INPAR::REBALANCE

FOUR_C_NAMESPACE_CLOSE

#endif
