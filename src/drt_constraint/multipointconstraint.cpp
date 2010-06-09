/*!----------------------------------------------------------------------
\file multipointconstraint.cpp

\brief Basic constraint class, dealing with multi point constraints
<pre>
Maintainer: Thomas Kloeppel
            kloeppel@lnm.mw.tum.de
            http://www.lnm.mw.tum.de/Members/kloeppel
            089 - 289-15257
</pre>

*----------------------------------------------------------------------*/
#ifdef CCADISCRET

#include <iostream>

#include "multipointconstraint.H"

#include "../drt_lib/drt_discret.H"
#include "../linalg/linalg_utils.H"

/*----------------------------------------------------------------------*
 |  ctor (public)                                               tk 07/08|
 *----------------------------------------------------------------------*/
UTILS::MPConstraint::MPConstraint(RCP<DRT::Discretization> discr,
        const string& conditionname,
        int& minID,
        int& maxID)
: UTILS::Constraint
  (
    discr,
    conditionname,
    minID,
    maxID
  )
{
  return;
}

/*----------------------------------------------------------------------*
 |  ctor (public)                                               tk 07/08|
 *----------------------------------------------------------------------*/
UTILS::MPConstraint::MPConstraint(RCP<DRT::Discretization> discr,
        const string& conditionname)
: UTILS::Constraint
  (
    discr,
    conditionname
  )
{
  return;
}

/// Set state of the underlying constraint discretization
void UTILS::MPConstraint::SetConstrState
(
  const string& state,  ///< name of state to set
  RCP<Epetra_Vector> V  ///< values to set
)
{
  if (constrtype_!=none)
  {
    map<int,RCP<DRT::Discretization> >::iterator discrit;
    for(discrit=constraintdis_.begin();discrit!=constraintdis_.end();++discrit)
    {
      RCP<Epetra_Vector> tmp = LINALG::CreateVector(*(discrit->second)->DofColMap(),false);
      LINALG::Export(*V,*tmp);
      (discrit->second)->ClearState();
      (discrit->second)->SetState(state,tmp);
    }
  }
};

#endif
