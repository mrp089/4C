/*!----------------------------------------------------------------------
\file drt_dofset_transparent_independent.cpp

\brief transparent independent dofset

<pre>
Maintainer: Shadan Shahmiri
            shahmiri@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15240
</pre>

*----------------------------------------------------------------------*/

#include "drt_dofset_transparent_independent.H"
#include "drt_dofset.H"

#include "../linalg/linalg_utils.H"



DRT::TransparentIndependentDofSet::TransparentIndependentDofSet(
  Teuchos::RCP<DRT::Discretization> sourcedis,
  bool parallel)
  : TransparentDofSet(sourcedis, parallel)
{
  return;
}

int DRT::TransparentIndependentDofSet::AssignDegreesOfFreedom(const DRT::Discretization& dis, const unsigned dspos, const int start)
{

  // first, we call the standard AssignDegreesOfFreedom from the base class
  int count = DRT::IndependentDofSet::AssignDegreesOfFreedom(dis,dspos,start);

  if(!parallel_)
  {
    TransferDegreesOfFreedom(*sourcedis_, dis, start);
  }
  else
  {
    ParallelTransferDegreesOfFreedom(*sourcedis_, dis, start);
  }

  // tell all proxies (again!)
  NotifyAssigned();

  return count;
}

int DRT::TransparentIndependentDofSet::NumDofPerNode( const DRT::Node & node ) const
{
  return DRT::DofSet::NumDofPerNode( node );
}


