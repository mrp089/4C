/*----------------------------------------------------------------------*/
/*! \file

\brief Unit tests for linalg dense eigen routines.

\level 0

*----------------------------------------------------------------------*/
#include <gtest/gtest.h>

#include "baci_linalg_utils_densematrix_eigen.hpp"

#include "baci_linalg_serialdensematrix.hpp"
#include "baci_linalg_serialdensevector.hpp"
#include "baci_linalg_utils_densematrix_multiply.hpp"
#include "baci_unittest_utils_assertions_test.hpp"

FOUR_C_NAMESPACE_OPEN

namespace
{
  /*
   * \note Helper functions for the dense eigen test routines.
   *
   */
  template <size_t length>
  void AssertEigenValues(const CORE::LINALG::SerialDenseVector& eigenvalues,
      const std::array<double, length>& eig_compare)
  {
    EXPECT_EQ(eigenvalues.length(), length);
    for (unsigned i = 0; i < length; ++i) EXPECT_NEAR(eigenvalues[i], eig_compare[i], 1e-9);
  }

  template <unsigned int size, size_t length>
  void AssertEigenValues(const CORE::LINALG::Matrix<size, size>& eigenvalues,
      const std::array<double, length>& eig_compare)
  {
    for (unsigned i = 0; i < size; ++i)
    {
      for (unsigned j = 0; j < size; ++j)
      {
        if (i == j)
          EXPECT_NEAR(eigenvalues(i, j), eig_compare[i], 1e-9) << "for i=" << i << ", j=" << j;
        else
          EXPECT_NEAR(eigenvalues(i, j), 0.0, 1e-9) << "for i=" << i << ", j=" << j;
      }
    }
  }

  template <size_t length>
  void AssertEigenProblem(const CORE::LINALG::SerialDenseMatrix& A,
      const CORE::LINALG::SerialDenseVector& eigenvalues,
      const CORE::LINALG::SerialDenseMatrix& eigenvectors,
      const std::array<double, length>& eig_compare)
  {
    EXPECT_EQ(eigenvalues.length(), length);
    EXPECT_EQ(A.numRows(), length);
    EXPECT_EQ(A.numCols(), length);
    EXPECT_EQ(eigenvectors.numRows(), length);
    EXPECT_EQ(eigenvectors.numCols(), length);

    AssertEigenValues(eigenvalues, eig_compare);

    CORE::LINALG::SerialDenseMatrix A_result(length, length, true);
    for (std::size_t i = 0; i < length; ++i)
    {
      CORE::LINALG::SerialDenseMatrix v(length, 1, false);
      for (std::size_t j = 0; j < length; ++j) v(j, 0) = eigenvectors(j, i);
      CORE::LINALG::multiplyNT(1.0, A_result, eigenvalues(i), v, v);
    }

    FOUR_C_EXPECT_NEAR(A, A_result, 1e-9);
  }

  template <unsigned int size, size_t length>
  void AssertEigenProblem(const CORE::LINALG::Matrix<size, size>& A,
      const CORE::LINALG::Matrix<size, size>& eigenvalues,
      const CORE::LINALG::Matrix<size, size>& eigenvectors,
      const std::array<double, length>& eig_compare)
  {
    AssertEigenValues(eigenvalues, eig_compare);

    CORE::LINALG::Matrix<size, size> A_result(true);
    for (unsigned int i = 0; i < size; ++i)
    {
      CORE::LINALG::Matrix<size, 1> v(false);
      for (unsigned int j = 0; j < size; ++j) v(j, 0) = eigenvectors(j, i);
      A_result.MultiplyNT(eigenvalues(i, i), v, v, 1.0);
    }

    FOUR_C_EXPECT_NEAR(A, A_result, 1e-9);
  }


  /*
   * \note The values for the matrix used in tests below are generated with python/numpy
   */
  TEST(LinalgDenseMatrixEigenTest, 2x2SymmetricEigenValues)
  {
    CORE::LINALG::SerialDenseMatrix A(2, 2, false);
    A(0, 0) = 0.9964456203546112;
    A(0, 1) = 0.490484665405466;
    A(1, 0) = 0.490484665405466;
    A(1, 1) = 0.5611378979071144;

    std::array eigenvalues{0.24218351254540577, 1.3154000057163198};

    CORE::LINALG::SerialDenseVector L(2);
    CORE::LINALG::SymmetricEigenValues(A, L, false);

    AssertEigenValues(L, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 2x2SymmetricEigenProblem)
  {
    CORE::LINALG::SerialDenseMatrix A(2, 2, false);
    A(0, 0) = 0.9964456203546112;
    A(0, 1) = 0.490484665405466;
    A(1, 0) = 0.490484665405466;
    A(1, 1) = 0.5611378979071144;

    std::array eigenvalues{0.24218351254540577, 1.3154000057163198};

    CORE::LINALG::SerialDenseMatrix eigenvectors(A);
    CORE::LINALG::SerialDenseVector L(2);
    CORE::LINALG::SymmetricEigenProblem(eigenvectors, L, false);

    AssertEigenProblem(A, L, eigenvectors, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 2x2SymmetricEigenNoVectors)
  {
    CORE::LINALG::SerialDenseMatrix A(2, 2, false);
    A(0, 0) = 0.9964456203546112;
    A(0, 1) = 0.490484665405466;
    A(1, 0) = 0.490484665405466;
    A(1, 1) = 0.5611378979071144;

    std::array eigenvalues{0.24218351254540577, 1.3154000057163198};

    CORE::LINALG::SerialDenseVector L(2);
    CORE::LINALG::SymmetricEigen(A, L, 'N', false);

    AssertEigenValues(L, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 2x2SymmetricEigenVectors)
  {
    CORE::LINALG::SerialDenseMatrix A(2, 2, false);
    A(0, 0) = 0.9964456203546112;
    A(0, 1) = 0.490484665405466;
    A(1, 0) = 0.490484665405466;
    A(1, 1) = 0.5611378979071144;

    std::array eigenvalues{0.24218351254540577, 1.3154000057163198};

    CORE::LINALG::SerialDenseMatrix eigenvectors(A);
    CORE::LINALG::SerialDenseVector L(2);
    CORE::LINALG::SymmetricEigen(eigenvectors, L, 'V', false);

    AssertEigenProblem(A, L, eigenvectors, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 2x2SYEV)
  {
    CORE::LINALG::Matrix<2, 2> A(false);
    A(0, 0) = 0.9964456203546112;
    A(0, 1) = 0.490484665405466;
    A(1, 0) = 0.490484665405466;
    A(1, 1) = 0.5611378979071144;

    std::array eigenvalues{0.24218351254540577, 1.3154000057163198};

    CORE::LINALG::Matrix<2, 2> V(false);
    CORE::LINALG::Matrix<2, 2> S(false);
    CORE::LINALG::SYEV(A, S, V);

    AssertEigenProblem(A, S, V, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 3x3SymmetricEigenValues)
  {
    CORE::LINALG::SerialDenseMatrix A(3, 3, false);
    A(0, 0) = 1.2966342861458506;
    A(0, 1) = 0.8940941796919223;
    A(0, 2) = 0.16862685184206302;
    A(1, 0) = 0.8940941796919223;
    A(1, 1) = 0.9880908794535803;
    A(1, 2) = 0.06322733832497837;
    A(2, 0) = 0.16862685184206302;
    A(2, 1) = 0.06322733832497837;
    A(2, 2) = 0.047048409972083906;

    std::array eigenvalues{0.01628207201103285, 0.2515293645924337, 2.0639621389680487};

    CORE::LINALG::SerialDenseVector L(3);
    CORE::LINALG::SymmetricEigenValues(A, L, false);

    AssertEigenValues(L, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 3x3SymmetricEigenProblem)
  {
    CORE::LINALG::SerialDenseMatrix A(3, 3, false);
    A(0, 0) = 1.2966342861458506;
    A(0, 1) = 0.8940941796919223;
    A(0, 2) = 0.16862685184206302;
    A(1, 0) = 0.8940941796919223;
    A(1, 1) = 0.9880908794535803;
    A(1, 2) = 0.06322733832497837;
    A(2, 0) = 0.16862685184206302;
    A(2, 1) = 0.06322733832497837;
    A(2, 2) = 0.047048409972083906;

    std::array eigenvalues{0.01628207201103285, 0.2515293645924337, 2.0639621389680487};

    CORE::LINALG::SerialDenseMatrix V(A);
    CORE::LINALG::SerialDenseVector L(3);
    CORE::LINALG::SymmetricEigenProblem(V, L, false);

    AssertEigenProblem(A, L, V, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 3x3SymmetricEigenNoVectors)
  {
    CORE::LINALG::SerialDenseMatrix A(3, 3, false);
    A(0, 0) = 1.2966342861458506;
    A(0, 1) = 0.8940941796919223;
    A(0, 2) = 0.16862685184206302;
    A(1, 0) = 0.8940941796919223;
    A(1, 1) = 0.9880908794535803;
    A(1, 2) = 0.06322733832497837;
    A(2, 0) = 0.16862685184206302;
    A(2, 1) = 0.06322733832497837;
    A(2, 2) = 0.047048409972083906;

    std::array eigenvalues{0.01628207201103285, 0.2515293645924337, 2.0639621389680487};

    CORE::LINALG::SerialDenseVector L(3);
    CORE::LINALG::SymmetricEigen(A, L, 'N', false);

    AssertEigenValues(L, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 3x3SymmetricEigenVectors)
  {
    CORE::LINALG::SerialDenseMatrix A(3, 3, false);
    A(0, 0) = 1.2966342861458506;
    A(0, 1) = 0.8940941796919223;
    A(0, 2) = 0.16862685184206302;
    A(1, 0) = 0.8940941796919223;
    A(1, 1) = 0.9880908794535803;
    A(1, 2) = 0.06322733832497837;
    A(2, 0) = 0.16862685184206302;
    A(2, 1) = 0.06322733832497837;
    A(2, 2) = 0.047048409972083906;

    std::array eigenvalues{0.01628207201103285, 0.2515293645924337, 2.0639621389680487};

    CORE::LINALG::SerialDenseMatrix V(A);
    CORE::LINALG::SerialDenseVector L(3);
    CORE::LINALG::SymmetricEigen(V, L, 'V', false);

    AssertEigenProblem(A, L, V, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 3x3SYEV)
  {
    CORE::LINALG::Matrix<3, 3> A(false);
    A(0, 0) = 1.2966342861458506;
    A(0, 1) = 0.8940941796919223;
    A(0, 2) = 0.16862685184206302;
    A(1, 0) = 0.8940941796919223;
    A(1, 1) = 0.9880908794535803;
    A(1, 2) = 0.06322733832497837;
    A(2, 0) = 0.16862685184206302;
    A(2, 1) = 0.06322733832497837;
    A(2, 2) = 0.047048409972083906;

    std::array eigenvalues{0.01628207201103285, 0.2515293645924337, 2.0639621389680487};

    CORE::LINALG::Matrix<3, 3> V(false);
    CORE::LINALG::Matrix<3, 3> S(false);
    CORE::LINALG::SYEV(A, S, V);

    AssertEigenProblem(A, S, V, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 4x4SymmetricEigenValues)
  {
    CORE::LINALG::SerialDenseMatrix A(4, 4, false);
    A(0, 0) = 0.5561130226871257;
    A(0, 1) = 1.0052918588741722;
    A(0, 2) = 0.8408494685470309;
    A(0, 3) = 0.8731301282118089;
    A(1, 0) = 1.0052918588741722;
    A(1, 1) = 2.023681530073728;
    A(1, 2) = 1.7222521019056944;
    A(1, 3) = 1.6511949164466262;
    A(2, 0) = 0.8408494685470309;
    A(2, 1) = 1.7222521019056944;
    A(2, 2) = 1.6035737196981317;
    A(2, 3) = 1.4613812746280035;
    A(3, 0) = 0.8731301282118089;
    A(3, 1) = 1.6511949164466262;
    A(3, 2) = 1.4613812746280035;
    A(3, 3) = 1.4335181777869124;

    std::array eigenvalues{
        0.00023212100268553735, 0.06219024553961773, 0.11100584442852221, 5.443458239275074};

    CORE::LINALG::SerialDenseVector L(4);
    CORE::LINALG::SymmetricEigenValues(A, L, false);

    AssertEigenValues(L, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 4x4SymmetricEigenProblem)
  {
    CORE::LINALG::SerialDenseMatrix A(4, 4, false);
    A(0, 0) = 0.5561130226871257;
    A(0, 1) = 1.0052918588741722;
    A(0, 2) = 0.8408494685470309;
    A(0, 3) = 0.8731301282118089;
    A(1, 0) = 1.0052918588741722;
    A(1, 1) = 2.023681530073728;
    A(1, 2) = 1.7222521019056944;
    A(1, 3) = 1.6511949164466262;
    A(2, 0) = 0.8408494685470309;
    A(2, 1) = 1.7222521019056944;
    A(2, 2) = 1.6035737196981317;
    A(2, 3) = 1.4613812746280035;
    A(3, 0) = 0.8731301282118089;
    A(3, 1) = 1.6511949164466262;
    A(3, 2) = 1.4613812746280035;
    A(3, 3) = 1.4335181777869124;

    std::array eigenvalues{
        0.00023212100268553735, 0.06219024553961773, 0.11100584442852221, 5.443458239275074};

    CORE::LINALG::SerialDenseMatrix V(A);
    CORE::LINALG::SerialDenseVector L(4);
    CORE::LINALG::SymmetricEigenProblem(V, L, false);

    AssertEigenProblem(A, L, V, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 4x4SymmetricEigenNoVectors)
  {
    CORE::LINALG::SerialDenseMatrix A(4, 4, false);
    A(0, 0) = 0.5561130226871257;
    A(0, 1) = 1.0052918588741722;
    A(0, 2) = 0.8408494685470309;
    A(0, 3) = 0.8731301282118089;
    A(1, 0) = 1.0052918588741722;
    A(1, 1) = 2.023681530073728;
    A(1, 2) = 1.7222521019056944;
    A(1, 3) = 1.6511949164466262;
    A(2, 0) = 0.8408494685470309;
    A(2, 1) = 1.7222521019056944;
    A(2, 2) = 1.6035737196981317;
    A(2, 3) = 1.4613812746280035;
    A(3, 0) = 0.8731301282118089;
    A(3, 1) = 1.6511949164466262;
    A(3, 2) = 1.4613812746280035;
    A(3, 3) = 1.4335181777869124;

    std::array eigenvalues{
        0.00023212100268553735, 0.06219024553961773, 0.11100584442852221, 5.443458239275074};

    CORE::LINALG::SerialDenseVector L(4);
    CORE::LINALG::SymmetricEigen(A, L, 'N', false);

    AssertEigenValues(L, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 4x4SymmetricEigenVectors)
  {
    CORE::LINALG::SerialDenseMatrix A(4, 4, false);
    A(0, 0) = 0.5561130226871257;
    A(0, 1) = 1.0052918588741722;
    A(0, 2) = 0.8408494685470309;
    A(0, 3) = 0.8731301282118089;
    A(1, 0) = 1.0052918588741722;
    A(1, 1) = 2.023681530073728;
    A(1, 2) = 1.7222521019056944;
    A(1, 3) = 1.6511949164466262;
    A(2, 0) = 0.8408494685470309;
    A(2, 1) = 1.7222521019056944;
    A(2, 2) = 1.6035737196981317;
    A(2, 3) = 1.4613812746280035;
    A(3, 0) = 0.8731301282118089;
    A(3, 1) = 1.6511949164466262;
    A(3, 2) = 1.4613812746280035;
    A(3, 3) = 1.4335181777869124;

    std::array eigenvalues{
        0.00023212100268553735, 0.06219024553961773, 0.11100584442852221, 5.443458239275074};

    CORE::LINALG::SerialDenseMatrix V(A);
    CORE::LINALG::SerialDenseVector L(4);
    CORE::LINALG::SymmetricEigen(V, L, 'V', false);

    AssertEigenProblem(A, L, V, eigenvalues);
  }

  TEST(LinalgDenseMatrixEigenTest, 4x4SYEV)
  {
    CORE::LINALG::Matrix<4, 4> A(false);
    A(0, 0) = 0.5561130226871257;
    A(0, 1) = 1.0052918588741722;
    A(0, 2) = 0.8408494685470309;
    A(0, 3) = 0.8731301282118089;
    A(1, 0) = 1.0052918588741722;
    A(1, 1) = 2.023681530073728;
    A(1, 2) = 1.7222521019056944;
    A(1, 3) = 1.6511949164466262;
    A(2, 0) = 0.8408494685470309;
    A(2, 1) = 1.7222521019056944;
    A(2, 2) = 1.6035737196981317;
    A(2, 3) = 1.4613812746280035;
    A(3, 0) = 0.8731301282118089;
    A(3, 1) = 1.6511949164466262;
    A(3, 2) = 1.4613812746280035;
    A(3, 3) = 1.4335181777869124;

    std::array eigenvalues{
        0.00023212100268553735, 0.06219024553961773, 0.11100584442852221, 5.443458239275074};

    CORE::LINALG::Matrix<4, 4> V(false);
    CORE::LINALG::Matrix<4, 4> S(false);
    CORE::LINALG::SYEV(A, S, V);

    AssertEigenProblem(A, S, V, eigenvalues);
  }
}  // namespace

FOUR_C_NAMESPACE_CLOSE
