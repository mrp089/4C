
#ifdef CCADISCRET

#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

#include "fsi_utils.H"
#include "../drt_lib/drt_utils.H"
#include "../drt_lib/linalg_utils.H"

#include <Epetra_CrsMatrix.h>
#include <EpetraExt_RowMatrixOut.h>
#include <NOX.H>
#include <NOX_Epetra.H>
#include <Epetra_SerialDenseMatrix.h>

#ifdef PARALLEL
#include <mpi.h>
#endif

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::UTILS::DumpJacobian(NOX::Epetra::Interface::Required& interface,
                              double alpha,
                              double beta,
                              Teuchos::RefCountPtr<Epetra_Vector> soln,
                              std::string filename)
{
  // that's really stupid again
  const Epetra_BlockMap& bmap = soln->Map();
  Epetra_Map map(bmap.NumGlobalElements(),
                 bmap.NumMyElements(),
                 bmap.MyGlobalElements(),
                 0,
                 bmap.Comm());

  RefCountPtr<Epetra_CrsMatrix> jacobian = rcp(new Epetra_CrsMatrix(Copy, map, map.NumGlobalElements()));

  int nummyelements = map.NumMyElements();
  int mypos = LINALG::FindMyPos(nummyelements, map.Comm());
  double eta = 0.0;

  Epetra_Vector fo(*soln);
  Epetra_Vector fp(*soln);
  Epetra_Vector Jc(*soln);

  // Compute the RHS at the initial solution
  interface.computeF(*soln, fo, NOX::Epetra::Interface::Required::FD_Res);

  Epetra_Vector x_perturb = *soln;

  for (int i=0; i<map.NumGlobalElements(); ++i)
  {
    if (map.Comm().MyPID()==0)
      cout << "calculate column " << i << "\n";

    int proc = 0;
    int idx = 0;
    if (i>=mypos and i<mypos+nummyelements)
    {
      eta = alpha*(*soln)[i-mypos] + beta;
      x_perturb[i-mypos] += eta;
      idx = map.GID(i-mypos);
      proc = map.Comm().MyPID();
    }

    // Find what proc eta is on
    int broadcastProc = 0;
    map.Comm().SumAll(&proc, &broadcastProc, 1);

    // Send the perturbation variable, eta, to all processors
    map.Comm().Broadcast(&eta, 1, broadcastProc);

    map.Comm().Broadcast(&idx, 1, broadcastProc);

    // Compute the perturbed RHS
    interface.computeF(x_perturb, fp, NOX::Epetra::Interface::Required::FD_Res);

    // Compute the column k of the Jacobian
    Jc.Update(1.0, fp, -1.0, fo, 0.0);
    Jc.Scale( 1.0/eta );

    // Insert nonzero column entries into the jacobian
    for (int j = 0; j < map.NumMyElements(); ++j)
    {
      int gid = map.GID(j);
      if (Jc[j] != 0.0)
      {
        int err = jacobian->SumIntoGlobalValues(gid,1,&Jc[j],&idx);
        if (err>0)
        {
          err = jacobian->InsertGlobalValues(gid,1,&Jc[j],&idx);
        }
        if (err != 0)
          dserror("Assembly failed");
      }
    }

    // Unperturb the solution vector
    x_perturb = *soln;
  }

  jacobian->FillComplete();

  EpetraExt::RowMatrixToMatlabFile(filename.c_str(),*jacobian);
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<const Epetra_Map>
FSI::UTILS::ShiftMap(Teuchos::RCP<const Epetra_Map> emap,
                     const std::vector<Teuchos::RCP<const Epetra_Map> >& vecSpaces)
{
  int maxgid = 0;
  for (unsigned i=0; i<vecSpaces.size(); ++i)
  {
    maxgid = max(maxgid,vecSpaces[i]->MaxAllGID());
  }

  std::vector<int> gids;
  gids.reserve(emap->NumMyElements());
  std::transform(emap->MyGlobalElements(),
                 emap->MyGlobalElements()+emap->NumMyElements(),
                 std::back_inserter(gids),
                 std::bind2nd(std::plus<int>(),maxgid+1-emap->MinAllGID()));

  return Teuchos::rcp(new Epetra_Map(-1,gids.size(),&gids[0],0,emap->Comm()));
}


#endif
