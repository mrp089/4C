/*----------------------------------------------------------------------*/
/*! \file

\brief transparent independent dofset

\level 2


*/
/*----------------------------------------------------------------------*/


#include "4C_discretization_dofset_transparent_independent.hpp"

#include "4C_discretization_dofset.hpp"
#include "4C_linalg_utils_sparse_algebra_math.hpp"

FOUR_C_NAMESPACE_OPEN



Core::DOFSets::TransparentIndependentDofSet::TransparentIndependentDofSet(
    Teuchos::RCP<Discret::Discretization> sourcedis, bool parallel)
    : TransparentDofSet(sourcedis, parallel)
{
  return;
}

int Core::DOFSets::TransparentIndependentDofSet::assign_degrees_of_freedom(
    const Discret::Discretization& dis, const unsigned dspos, const int start)
{
  // first, we call the standard assign_degrees_of_freedom from the base class
  int count = IndependentDofSet::assign_degrees_of_freedom(dis, dspos, start);

  if (!parallel_)
  {
    transfer_degrees_of_freedom(*sourcedis_, dis, start);
  }
  else
  {
    parallel_transfer_degrees_of_freedom(*sourcedis_, dis, start);
  }

  // tell all proxies (again!)
  NotifyAssigned();

  return count;
}

int Core::DOFSets::TransparentIndependentDofSet::NumDofPerNode(const Core::Nodes::Node& node) const
{
  return DofSet::NumDofPerNode(node);
}

FOUR_C_NAMESPACE_CLOSE
