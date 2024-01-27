/*---------------------------------------------------------------------*/
/*! \file

\brief A collection of helper methods for namespace DRT

\level 1


*/
/*---------------------------------------------------------------------*/


#include "baci_lib_utils_materials.H"

#include "baci_global_data.H"
#include "baci_lib_discret.H"
#include "baci_linalg_utils_sparse_algebra_manipulation.H"

#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <vector>

BACI_NAMESPACE_OPEN

/*----------------------------------------------------------------------*
 *  Update material configuration of discretization with
 *  given displacement field                                             |
 *----------------------------------------------------------------------*/
void DRT::UTILS::UpdateMaterialConfigWithDispVector(
    const Teuchos::RCP<DRT::Discretization>& dis, const Teuchos::RCP<const Epetra_Vector>& disp)
{
  const int numnode = (dis->NodeColMap())->NumMyElements();

  // Create Vector which holds all col-displacments of processor
  Teuchos::RCP<Epetra_Vector> coldisp = Teuchos::rcp(new Epetra_Vector(*(dis->DofColMap())));

  // Export row-displacments to col-displacements
  CORE::LINALG::Export(*disp, *coldisp);

  // determine number of space dimensions
  const int numdim = GLOBAL::Problem::Instance()->NDim();

  // loop over all nodes
  for (int index = 0; index < numnode; ++index)
  {
    // get current node
    DRT::Node* mynode = dis->lColNode(index);

    std::vector<int> globaldofs = dis->Dof(0, mynode);
    // Since ChangePos requires vector of length 3, we use hardcoded length here
    // init with zero just to be sure
    std::vector<double> nvector(3, 0.0);

    // numdim can be 2 or 3
    for (int i = 0; i < numdim; ++i)
    {
      const int lid = coldisp->Map().LID(globaldofs[i]);

      if (lid < 0)
        dserror(
            "Proc %d: Cannot find gid=%d in Epetra_Vector", coldisp->Comm().MyPID(), globaldofs[i]);
      nvector[i] += (*coldisp)[lid];
    }
    // changepos takes vector with length = 3
    mynode->ChangePos(nvector);
  }

  return;
}

BACI_NAMESPACE_CLOSE
