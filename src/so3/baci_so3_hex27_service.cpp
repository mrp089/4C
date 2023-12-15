/*----------------------------------------------------------------------*/
/*! \file
\brief tri-quadratic displacement based solid element
\level 1

*----------------------------------------------------------------------*/


#include "baci_discretization_fem_general_utils_fem_shapefunctions.H"
#include "baci_lib_node.H"
#include "baci_so3_hex27.H"

BACI_NAMESPACE_OPEN


/*----------------------------------------------------------------------*
 |  return Center Coords in Reference System                            |
 *----------------------------------------------------------------------*/
std::vector<double> DRT::ELEMENTS::So_hex27::soh27_ElementCenterRefeCoords()
{
  // update element geometry
  DRT::Node** nodes = Nodes();
  CORE::LINALG::Matrix<NUMNOD_SOH27, NUMDIM_SOH27> xrefe;  // material coord. of element
  for (int i = 0; i < NUMNOD_SOH27; ++i)
  {
    const auto& x = nodes[i]->X();
    xrefe(i, 0) = x[0];
    xrefe(i, 1) = x[1];
    xrefe(i, 2) = x[2];
  }
  const CORE::FE::CellType distype = Shape();
  CORE::LINALG::Matrix<NUMNOD_SOH27, 1> funct;
  // Element midpoint at r=s=t=0.0
  CORE::DRT::UTILS::shape_function_3D(funct, 0.0, 0.0, 0.0, distype);
  CORE::LINALG::Matrix<1, NUMDIM_SOH27> midpoint;
  // midpoint.Multiply('T','N',1.0,funct,xrefe,0.0);
  midpoint.MultiplyTN(funct, xrefe);
  std::vector<double> centercoords(3);
  centercoords[0] = midpoint(0, 0);
  centercoords[1] = midpoint(0, 1);
  centercoords[2] = midpoint(0, 2);
  return centercoords;
}

BACI_NAMESPACE_CLOSE
