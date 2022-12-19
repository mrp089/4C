/*---------------------------------------------------------------------*/
/*! \file

\brief A collection of helper methods for the nullspace calculation at
       node level

\level 0


*/
/*---------------------------------------------------------------------*/

#include <Teuchos_SerialDenseMatrix.hpp>
#include "linalg_utils_nullspace.H"

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::SerialDenseMatrix<int, double> LINALG::ComputeSolid3DNullSpace(
    const DRT::Node& node, const double* x0)
{
  /* the rigid body modes for structures are:

        xtrans   ytrans  ztrans   xrot       yrot       zrot
        mode[0]  mode[1] mode[2]  mode[3]    mode[4]    mode[5]
    -----------------------------------------------------------
  x   |    1       0       0       0          z-z0      -y+y0
  y   |    0       1       0      -z+z0       0          x-x0
  z   |    0       0       1       y-y0      -x+x0       0

  valid element types: ale3, so_hex8, so_hex20, so_hex27, sosh8, so_tet4,
                       so_tet10, so_weg6, sodisp, so_shw6, truss3, torsion3
  */

  const double* x = node.X();

  Teuchos::SerialDenseMatrix<int, double> nullspace(3, 6);
  // x-modes
  nullspace(0, 0) = 1.0;
  nullspace(0, 1) = 0.0;
  nullspace(0, 2) = 0.0;
  nullspace(0, 3) = 0.0;
  nullspace(0, 4) = x[2] - x0[2];
  nullspace(0, 5) = -x[1] + x0[1];
  // y-modes
  nullspace(1, 0) = 0.0;
  nullspace(1, 1) = 1.0;
  nullspace(1, 2) = 0.0;
  nullspace(1, 3) = -x[2] + x0[2];
  nullspace(1, 4) = 0.0;
  nullspace(1, 5) = x[0] - x0[0];
  // z-modes
  nullspace(2, 0) = 0.0;
  nullspace(2, 1) = 0.0;
  nullspace(2, 2) = 1.0;
  nullspace(2, 3) = x[1] - x0[1];
  nullspace(2, 4) = -x[0] + x0[0];
  nullspace(2, 5) = 0.0;

  return nullspace;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::SerialDenseMatrix<int, double> LINALG::ComputeSolid2DNullSpace(
    const DRT::Node& node, const double* x0)
{
  /* the rigid body modes for structures are:

        xtrans   ytrans   zrot
        mode[0]  mode[1]  mode[3]
      ----------------------------
  x   |    1       0       -y+y0
  y   |    0       1       x-x0

  valid element types: wall1, ale2, torsion2

   */

  const double* x = node.X();

  Teuchos::SerialDenseMatrix<int, double> nullspace(2, 3);
  // x-modes
  nullspace(0, 0) = 1.0;
  nullspace(0, 1) = 0.0;
  nullspace(0, 2) = -x[1] + x0[1];
  // y-modes
  nullspace(1, 0) = 0.0;
  nullspace(1, 1) = 1.0;
  nullspace(1, 2) = x[0] - x0[0];

  return nullspace;
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
Teuchos::SerialDenseMatrix<int, double> LINALG::ComputeFluidNullSpace(
    const DRT::Node& node, const int numdof, const int dimnsp)
{
  /* the rigid body modes for fluids are:

            xtrans   ytrans  ztrans   pressure
            mode[0]  mode[1] mode[2]  mode[3]
      ----------------------------------------
      x   |    1       0       0       0
      y   |    0       1       0       0
      z   |    0       0       1       0
      p   |    0       0       0       1

      valid element types: fluid3, xfluid3
  */

  if (numdof > 10) dserror("Cannot define more than 10 degrees of freedom!");

  Teuchos::SerialDenseMatrix<int, double> nullspace(numdof, dimnsp);
  for (int i = 0; i < numdof; i++)
  {
    for (int j = 0; j < dimnsp; j++)
    {
      if (i == j)
        nullspace(i, j) = 1.0;
      else
        nullspace(i, j) = 0.0;
    }
  }

  return nullspace;
}
