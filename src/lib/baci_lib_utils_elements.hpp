/*---------------------------------------------------------------------*/
/*! \file

\brief Collection of general element utility functions


\level 1

*/
/*---------------------------------------------------------------------*/

#ifndef BACI_LIB_UTILS_ELEMENTS_HPP
#define BACI_LIB_UTILS_ELEMENTS_HPP

#include "baci_config.hpp"

#include "baci_discretization_fem_general_utils_fem_shapefunctions.hpp"

BACI_NAMESPACE_OPEN

namespace DRT
{
  namespace UTILS
  {
    /// \brief get the minimal Jacobian determinant value calculated at the node positions
    /**
     *  \param xcurr  Current nodal positions
     *
     *  \return minimal value.
     *
     *  \author hiermeier \date 09/18 */
    template <CORE::FE::CellType type, unsigned numnode, unsigned numdim>
    double GetMinimalJacDeterminantAtNodes(const CORE::LINALG::Matrix<numdim, numnode>& xcurr)
    {
      // check input
      static_assert(numnode == CORE::FE::num_nodes<type>, "Wrong matrix dimension.");
      static_assert(numdim == CORE::FE::dim<type>, "Wrong matrix dimension.");

      CORE::LINALG::Matrix<numdim, numnode> deriv_at_c(false);
      CORE::LINALG::Matrix<numdim, numdim> jac_at_c(false);

      // parametric coordinates of the HEX8 corners
      static const CORE::LINALG::SerialDenseMatrix rst =
          CORE::FE::getEleNodeNumbering_nodes_paramspace(type);
      double min_detJ = std::numeric_limits<double>::max();

      for (unsigned c = 0; c < numnode; ++c)
      {
        const CORE::LINALG::Matrix<numdim, 1> rst_c(&rst(0, c), true);

        CORE::FE::shape_function_deriv1<type>(rst_c, deriv_at_c);
        jac_at_c.MultiplyNT(deriv_at_c, xcurr);

        const double detJ_at_c = jac_at_c.Determinant();
        if (detJ_at_c < min_detJ) min_detJ = detJ_at_c;
      }

      return min_detJ;
    }
  }  // namespace UTILS
}  // namespace DRT


BACI_NAMESPACE_CLOSE

#endif  // LIB_UTILS_ELEMENTS_H