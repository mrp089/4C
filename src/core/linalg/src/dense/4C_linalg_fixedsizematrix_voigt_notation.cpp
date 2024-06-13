/*! \file
\brief Voigt notation definition and utilities
\level 1
*/

#include "4C_linalg_fixedsizematrix_voigt_notation.hpp"

#include <Sacado.hpp>

FOUR_C_NAMESPACE_OPEN

using NotationType = Core::LinAlg::Voigt::NotationType;

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void Core::LinAlg::Voigt::matrix_3x3_to_9x1(
    Core::LinAlg::Matrix<3, 3> const& in, Core::LinAlg::Matrix<9, 1>& out)
{
  for (int i = 0; i < 3; ++i) out(i) = in(i, i);
  out(3) = in(0, 1);
  out(4) = in(1, 2);
  out(5) = in(0, 2);
  out(6) = in(1, 0);
  out(7) = in(2, 1);
  out(8) = in(2, 0);
}

void Core::LinAlg::Voigt::matrix_9x1_to_3x3(
    Core::LinAlg::Matrix<9, 1> const& in, Core::LinAlg::Matrix<3, 3>& out)
{
  for (int i = 0; i < 3; ++i) out(i, i) = in(i);
  out(0, 1) = in(3);
  out(1, 2) = in(4);
  out(0, 2) = in(5);
  out(1, 0) = in(6);
  out(2, 1) = in(7);
  out(2, 0) = in(8);
}

template <NotationType rows_notation, NotationType cols_notation>
void Core::LinAlg::Voigt::fourth_order_identity_matrix(Core::LinAlg::Matrix<6, 6>& id)
{
  id.Clear();

  for (unsigned int i = 0; i < 3; ++i) id(i, i) = 1.0;

  for (unsigned int i = 3; i < 6; ++i)
    id(i, i) = 0.5 * VoigtUtils<rows_notation>::scale_factor(i) *
               VoigtUtils<cols_notation>::scale_factor(i);
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <NotationType type>
void Core::LinAlg::Voigt::VoigtUtils<type>::symmetric_outer_product(
    const Core::LinAlg::Matrix<3, 1>& vec_a, const Core::LinAlg::Matrix<3, 1>& vec_b,
    Core::LinAlg::Matrix<6, 1>& ab_ba)
{
  std::fill(ab_ba.A(), ab_ba.A() + 6, 0.0);

  Core::LinAlg::Matrix<3, 3> outer_product;
  outer_product.MultiplyNT(vec_a, vec_b);

  for (unsigned i = 0; i < 3; ++i)
    for (unsigned j = i; j < 3; ++j)
      ab_ba(IndexMappings::symmetric_tensor_to_voigt6_index(i, j)) +=
          outer_product(i, j) + outer_product(j, i);

  // scale off-diagonal values
  scale_off_diagonal_vals(ab_ba);
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <NotationType type>
void Core::LinAlg::Voigt::VoigtUtils<type>::multiply_tensor_vector(
    const Core::LinAlg::Matrix<6, 1>& strain, const Core::LinAlg::Matrix<3, 1>& vec,
    Core::LinAlg::Matrix<3, 1>& res)
{
  for (unsigned i = 0; i < 3; ++i)
  {
    for (unsigned j = 0; j < 3; ++j)
    {
      const double fac = unscale_factor(IndexMappings::symmetric_tensor_to_voigt6_index(i, j));
      res(i, 0) += strain(IndexMappings::symmetric_tensor_to_voigt6_index(i, j)) * fac * vec(j, 0);
    }
  }
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <NotationType type>
void Core::LinAlg::Voigt::VoigtUtils<type>::power_of_symmetric_tensor(const unsigned pow,
    const Core::LinAlg::Matrix<6, 1>& strain, Core::LinAlg::Matrix<6, 1>& strain_pow)
{
  std::copy(strain.A(), strain.A() + 6, strain_pow.A());

  if (pow > 1)
  {
    // unscale the off-diagonal values
    unscale_off_diagonal_vals(strain_pow);

    Core::LinAlg::Matrix<6, 1> prod(false);

    for (unsigned p = 1; p < pow; ++p)
    {
      std::fill(prod.A(), prod.A() + 6, 0.0);

      for (unsigned i = 0; i < 3; ++i)
      {
        for (unsigned j = i; j < 3; ++j)
        {
          for (unsigned k = 0; k < 3; ++k)
          {
            prod(IndexMappings::symmetric_tensor_to_voigt6_index(i, j), 0) +=
                strain_pow(IndexMappings::symmetric_tensor_to_voigt6_index(i, k), 0) *
                unscale_fac_[IndexMappings::symmetric_tensor_to_voigt6_index(k, j)] *
                strain(IndexMappings::symmetric_tensor_to_voigt6_index(k, j), 0);
          }
        }
      }

      std::copy(prod.A(), prod.A() + 6, strain_pow.A());
    }

    // scale the off-diagonal values again
    scale_off_diagonal_vals(strain_pow);
  }
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <NotationType type>
void Core::LinAlg::Voigt::VoigtUtils<type>::inverse_tensor(
    const Core::LinAlg::Matrix<6, 1>& tens, Core::LinAlg::Matrix<6, 1>& tens_inv)
{
  double det = determinant(tens);
  tens_inv(0) = (tens(1) * tens(2) - unscale_factor(4) * unscale_factor(4) * tens(4) * tens(4)) /
                det * scale_factor(0);
  tens_inv(1) = (tens(0) * tens(2) - unscale_factor(5) * unscale_factor(5) * tens(5) * tens(5)) /
                det * scale_factor(1);
  tens_inv(2) = (tens(0) * tens(1) - unscale_factor(3) * unscale_factor(3) * tens(3) * tens(3)) /
                det * scale_factor(2);
  tens_inv(3) = (unscale_factor(5) * unscale_factor(4) * tens(5) * tens(4) -
                    unscale_factor(3) * unscale_factor(2) * tens(3) * tens(2)) /
                det * scale_factor(3);
  tens_inv(4) = (unscale_factor(3) * unscale_factor(5) * tens(3) * tens(5) -
                    unscale_factor(0) * unscale_factor(4) * tens(0) * tens(4)) /
                det * scale_factor(4);
  tens_inv(5) = (unscale_factor(3) * unscale_factor(4) * tens(3) * tens(4) -
                    unscale_factor(5) * unscale_factor(1) * tens(5) * tens(1)) /
                det * scale_factor(5);
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <NotationType type>
void Core::LinAlg::Voigt::VoigtUtils<type>::to_stress_like(
    const Core::LinAlg::Matrix<6, 1>& vtensor_in, Core::LinAlg::Matrix<6, 1>& vtensor_out)
{
  for (unsigned i = 0; i < 6; ++i) vtensor_out(i) = unscale_factor(i) * vtensor_in(i);
}

template <NotationType type>
void Core::LinAlg::Voigt::VoigtUtils<type>::to_strain_like(
    const Core::LinAlg::Matrix<6, 1>& vtensor_in, Core::LinAlg::Matrix<6, 1>& vtensor_out)
{
  for (unsigned i = 0; i < 6; ++i)
    vtensor_out(i) = unscale_factor(i) * vtensor_in(i) * Strains::scale_factor(i);
}

template <NotationType type>
void Core::LinAlg::Voigt::VoigtUtils<type>::vector_to_matrix(
    const Core::LinAlg::Matrix<6, 1>& vtensor_in, Core::LinAlg::Matrix<3, 3>& tensor_out)
{
  for (int i = 0; i < 3; ++i) tensor_out(i, i) = vtensor_in(i);
  tensor_out(0, 1) = tensor_out(1, 0) = unscale_factor(3) * vtensor_in(3);
  tensor_out(1, 2) = tensor_out(2, 1) = unscale_factor(4) * vtensor_in(4);
  tensor_out(0, 2) = tensor_out(2, 0) = unscale_factor(5) * vtensor_in(5);
}

template <NotationType type>
template <typename T>
void Core::LinAlg::Voigt::VoigtUtils<type>::matrix_to_vector(
    const Core::LinAlg::Matrix<3, 3, T>& tensor_in, Core::LinAlg::Matrix<6, 1, T>& vtensor_out)
{
  for (int i = 0; i < 3; ++i) vtensor_out(i) = tensor_in(i, i);
  vtensor_out(3) = 0.5 * scale_factor(3) * (tensor_in(0, 1) + tensor_in(1, 0));
  vtensor_out(4) = 0.5 * scale_factor(4) * (tensor_in(1, 2) + tensor_in(2, 1));
  vtensor_out(5) = 0.5 * scale_factor(5) * (tensor_in(0, 2) + tensor_in(2, 0));
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <NotationType type>
void Core::LinAlg::Voigt::VoigtUtils<type>::scale_off_diagonal_vals(
    Core::LinAlg::Matrix<6, 1>& strain)
{
  for (unsigned i = 3; i < 6; ++i) strain(i, 0) *= scale_factor(i);
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <NotationType type>
void Core::LinAlg::Voigt::VoigtUtils<type>::unscale_off_diagonal_vals(
    Core::LinAlg::Matrix<6, 1>& strain)
{
  for (unsigned i = 3; i < 6; ++i) strain(i, 0) *= unscale_factor(i);
}

// explicit template declarations
template class Core::LinAlg::Voigt::VoigtUtils<NotationType::strain>;
template class Core::LinAlg::Voigt::VoigtUtils<NotationType::stress>;

template void Core::LinAlg::Voigt::VoigtUtils<Core::LinAlg::Voigt::NotationType::strain>::
    matrix_to_vector<double>(
        Core::LinAlg::Matrix<3, 3, double> const& in, Core::LinAlg::Matrix<6, 1, double>& out);
template void Core::LinAlg::Voigt::VoigtUtils<Core::LinAlg::Voigt::NotationType::stress>::
    matrix_to_vector<double>(
        Core::LinAlg::Matrix<3, 3, double> const& in, Core::LinAlg::Matrix<6, 1, double>& out);

using FAD = Sacado::Fad::DFad<double>;
template void
Core::LinAlg::Voigt::VoigtUtils<Core::LinAlg::Voigt::NotationType::strain>::matrix_to_vector<FAD>(
    Core::LinAlg::Matrix<3, 3, FAD> const& in, Core::LinAlg::Matrix<6, 1, FAD>& out);
template void
Core::LinAlg::Voigt::VoigtUtils<Core::LinAlg::Voigt::NotationType::stress>::matrix_to_vector<FAD>(
    Core::LinAlg::Matrix<3, 3, FAD> const& in, Core::LinAlg::Matrix<6, 1, FAD>& out);

template void Core::LinAlg::Voigt::fourth_order_identity_matrix<NotationType::stress,
    NotationType::stress>(Core::LinAlg::Matrix<6, 6>& id);
template void Core::LinAlg::Voigt::fourth_order_identity_matrix<NotationType::stress,
    NotationType::strain>(Core::LinAlg::Matrix<6, 6>& id);

FOUR_C_NAMESPACE_CLOSE
