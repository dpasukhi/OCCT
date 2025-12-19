// Copyright (c) 2024 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#include <gtest/gtest.h>

#include <math_Lin_SVD.hxx>
#include <math_Lin_Householder.hxx>
#include <math_Lin_Jacobi.hxx>
#include <math_Lin_LeastSquares.hxx>
#include <math_Lin_Gauss.hxx>
#include <math_SVD.hxx>
#include <math_Householder.hxx>
#include <math_Jacobi.hxx>
#include <math_GaussLeastSquare.hxx>
#include <math_Vector.hxx>
#include <math_Matrix.hxx>

#include <cmath>

namespace
{
constexpr double THE_TOLERANCE = 1.0e-10;

//! Create identity matrix.
math_Matrix CreateIdentity(int theN)
{
  math_Matrix aMat(1, theN, 1, theN, 0.0);
  for (int i = 1; i <= theN; ++i)
  {
    aMat(i, i) = 1.0;
  }
  return aMat;
}

//! Create symmetric positive definite matrix.
math_Matrix CreateSPD(int theN)
{
  math_Matrix aMat(1, theN, 1, theN, 0.0);
  for (int i = 1; i <= theN; ++i)
  {
    for (int j = 1; j <= theN; ++j)
    {
      aMat(i, j) = 1.0 / (i + j - 1); // Hilbert matrix
    }
    aMat(i, i) += static_cast<double>(theN); // Make well-conditioned
  }
  return aMat;
}

//! Create random matrix.
math_Matrix CreateRandom(int theM, int theN, int theSeed = 42)
{
  math_Matrix aMat(1, theM, 1, theN);
  srand(static_cast<unsigned int>(theSeed));
  for (int i = 1; i <= theM; ++i)
  {
    for (int j = 1; j <= theN; ++j)
    {
      aMat(i, j) = static_cast<double>(rand()) / RAND_MAX * 2.0 - 1.0;
    }
  }
  return aMat;
}

//! Compute Frobenius norm of matrix.
double FrobeniusNorm(const math_Matrix& theMat)
{
  double aNorm = 0.0;
  for (int i = theMat.LowerRow(); i <= theMat.UpperRow(); ++i)
  {
    for (int j = theMat.LowerCol(); j <= theMat.UpperCol(); ++j)
    {
      aNorm += theMat(i, j) * theMat(i, j);
    }
  }
  return std::sqrt(aNorm);
}

//! Compute L2 norm of vector.
double VectorNorm(const math_Vector& theVec)
{
  double aNorm = 0.0;
  for (int i = theVec.Lower(); i <= theVec.Upper(); ++i)
  {
    aNorm += theVec(i) * theVec(i);
  }
  return std::sqrt(aNorm);
}

//! Matrix multiplication A*B.
math_Matrix MatMul(const math_Matrix& theA, const math_Matrix& theB)
{
  const int aM = theA.RowNumber();
  const int aN = theB.ColNumber();
  const int aK = theA.ColNumber();
  math_Matrix aResult(1, aM, 1, aN, 0.0);
  for (int i = 1; i <= aM; ++i)
  {
    for (int j = 1; j <= aN; ++j)
    {
      for (int k = 1; k <= aK; ++k)
      {
        aResult(i, j) += theA(i, k) * theB(k, j);
      }
    }
  }
  return aResult;
}

//! Transpose matrix.
math_Matrix Transpose(const math_Matrix& theMat)
{
  const int aM = theMat.RowNumber();
  const int aN = theMat.ColNumber();
  math_Matrix aResult(1, aN, 1, aM);
  for (int i = 1; i <= aM; ++i)
  {
    for (int j = 1; j <= aN; ++j)
    {
      aResult(j, i) = theMat(i, j);
    }
  }
  return aResult;
}

} // namespace

// ============================================================================
// SVD tests
// ============================================================================

TEST(math_Lin_SVD_NewTest, BasicDecomposition_2x2)
{
  math_Matrix aMat(1, 2, 1, 2);
  aMat(1, 1) = 3.0; aMat(1, 2) = 2.0;
  aMat(2, 1) = 2.0; aMat(2, 2) = 3.0;

  auto aResult = math::Lin::SVD(aMat);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.Rank, 2);

  // Verify U * S * V^T = A
  const math_Matrix& aU = *aResult.U;
  const math_Vector& aS = *aResult.SingularValues;
  const math_Matrix& aV = *aResult.V;

  // Construct diagonal matrix from singular values
  math_Matrix aSigma(1, 2, 1, 2, 0.0);
  aSigma(1, 1) = aS(1);
  aSigma(2, 2) = aS(2);

  // Compute U * Sigma * V^T
  math_Matrix aUSigma = MatMul(aU, aSigma);
  math_Matrix aVt = Transpose(aV);
  math_Matrix aReconstructed = MatMul(aUSigma, aVt);

  // Check reconstruction
  for (int i = 1; i <= 2; ++i)
  {
    for (int j = 1; j <= 2; ++j)
    {
      EXPECT_NEAR(aReconstructed(i, j), aMat(i, j), THE_TOLERANCE);
    }
  }
}

TEST(math_Lin_SVD_NewTest, SingularValues)
{
  math_Matrix aMat(1, 3, 1, 3);
  aMat(1, 1) = 1.0; aMat(1, 2) = 2.0; aMat(1, 3) = 3.0;
  aMat(2, 1) = 4.0; aMat(2, 2) = 5.0; aMat(2, 3) = 6.0;
  aMat(3, 1) = 7.0; aMat(3, 2) = 8.0; aMat(3, 3) = 9.0;

  auto aResult = math::Lin::SVD(aMat);

  ASSERT_TRUE(aResult.IsDone());

  // Singular values should be non-negative and in descending order
  const math_Vector& aS = *aResult.SingularValues;
  for (int i = aS.Lower(); i < aS.Upper(); ++i)
  {
    EXPECT_GE(aS(i), 0.0);
    EXPECT_GE(aS(i), aS(i + 1));
  }

  // This matrix is rank-deficient (rank 2)
  EXPECT_LE(aResult.Rank, 2);
}

TEST(math_Lin_SVD_NewTest, SolveSystem)
{
  math_Matrix aMat(1, 2, 1, 2);
  aMat(1, 1) = 3.0; aMat(1, 2) = 1.0;
  aMat(2, 1) = 1.0; aMat(2, 2) = 2.0;

  math_Vector aB(1, 2);
  aB(1) = 9.0;
  aB(2) = 8.0;

  auto aResult = math::Lin::SolveSVD(aMat, aB);

  ASSERT_TRUE(aResult.IsDone());

  // Check solution: Ax = b
  const math_Vector& aX = *aResult.Solution;
  double aCheck1 = aMat(1, 1) * aX(1) + aMat(1, 2) * aX(2);
  double aCheck2 = aMat(2, 1) * aX(1) + aMat(2, 2) * aX(2);

  EXPECT_NEAR(aCheck1, aB(1), THE_TOLERANCE);
  EXPECT_NEAR(aCheck2, aB(2), THE_TOLERANCE);
}

TEST(math_Lin_SVD_NewTest, PseudoInverse)
{
  math_Matrix aMat(1, 2, 1, 2);
  aMat(1, 1) = 1.0; aMat(1, 2) = 2.0;
  aMat(2, 1) = 3.0; aMat(2, 2) = 4.0;

  auto aPinv = math::Lin::PseudoInverse(aMat);

  ASSERT_TRUE(aPinv.IsDone());

  // A * A+ * A = A
  math_Matrix aTemp = MatMul(aMat, *aPinv.Inverse);
  math_Matrix aCheck = MatMul(aTemp, aMat);

  for (int i = 1; i <= 2; ++i)
  {
    for (int j = 1; j <= 2; ++j)
    {
      EXPECT_NEAR(aCheck(i, j), aMat(i, j), THE_TOLERANCE);
    }
  }
}

TEST(math_Lin_SVD_NewTest, ConditionNumber)
{
  // Well-conditioned identity matrix
  math_Matrix aI = CreateIdentity(3);
  double aCondI = math::Lin::ConditionNumber(aI);
  EXPECT_NEAR(aCondI, 1.0, THE_TOLERANCE);

  // Ill-conditioned matrix
  math_Matrix aHilbert(1, 3, 1, 3);
  for (int i = 1; i <= 3; ++i)
  {
    for (int j = 1; j <= 3; ++j)
    {
      aHilbert(i, j) = 1.0 / (i + j - 1);
    }
  }
  double aCondH = math::Lin::ConditionNumber(aHilbert);
  EXPECT_GT(aCondH, 100.0); // Hilbert matrices are ill-conditioned
}

// ============================================================================
// Householder QR tests
// ============================================================================

TEST(math_Lin_Householder_NewTest, BasicQR_2x2)
{
  math_Matrix aMat(1, 2, 1, 2);
  aMat(1, 1) = 1.0; aMat(1, 2) = 2.0;
  aMat(2, 1) = 3.0; aMat(2, 2) = 4.0;

  auto aResult = math::Lin::QR(aMat);

  ASSERT_TRUE(aResult.IsDone());

  const math_Matrix& aQ = *aResult.Q;
  const math_Matrix& aR = *aResult.R;

  // Check Q is orthogonal: Q * Q^T = I
  math_Matrix aQQt = MatMul(aQ, Transpose(aQ));
  for (int i = 1; i <= 2; ++i)
  {
    for (int j = 1; j <= 2; ++j)
    {
      double aExpected = (i == j) ? 1.0 : 0.0;
      EXPECT_NEAR(aQQt(i, j), aExpected, THE_TOLERANCE);
    }
  }

  // Check R is upper triangular
  EXPECT_NEAR(aR(2, 1), 0.0, THE_TOLERANCE);

  // Check Q * R = A
  math_Matrix aQR = MatMul(aQ, aR);
  for (int i = 1; i <= 2; ++i)
  {
    for (int j = 1; j <= 2; ++j)
    {
      EXPECT_NEAR(aQR(i, j), aMat(i, j), THE_TOLERANCE);
    }
  }
}

TEST(math_Lin_Householder_NewTest, SolveSystem)
{
  math_Matrix aMat(1, 2, 1, 2);
  aMat(1, 1) = 3.0; aMat(1, 2) = 1.0;
  aMat(2, 1) = 1.0; aMat(2, 2) = 2.0;

  math_Vector aB(1, 2);
  aB(1) = 9.0;
  aB(2) = 8.0;

  auto aResult = math::Lin::SolveQR(aMat, aB);

  ASSERT_TRUE(aResult.IsDone());

  // Check solution: Ax = b
  const math_Vector& aX = *aResult.Solution;
  double aCheck1 = aMat(1, 1) * aX(1) + aMat(1, 2) * aX(2);
  double aCheck2 = aMat(2, 1) * aX(1) + aMat(2, 2) * aX(2);

  EXPECT_NEAR(aCheck1, aB(1), THE_TOLERANCE);
  EXPECT_NEAR(aCheck2, aB(2), THE_TOLERANCE);
}

TEST(math_Lin_Householder_NewTest, Overdetermined)
{
  // 3x2 system (overdetermined)
  math_Matrix aMat(1, 3, 1, 2);
  aMat(1, 1) = 1.0; aMat(1, 2) = 1.0;
  aMat(2, 1) = 1.0; aMat(2, 2) = 2.0;
  aMat(3, 1) = 1.0; aMat(3, 2) = 3.0;

  math_Vector aB(1, 3);
  aB(1) = 1.0;
  aB(2) = 2.0;
  aB(3) = 3.0;

  auto aResult = math::Lin::SolveQR(aMat, aB);

  ASSERT_TRUE(aResult.IsDone());

  // This is a least squares solution
  EXPECT_EQ((*aResult.Solution).Length(), 2);
}

// ============================================================================
// Jacobi eigenvalue tests
// ============================================================================

TEST(math_Lin_Jacobi_NewTest, Eigenvalues_Diagonal)
{
  math_Matrix aMat(1, 3, 1, 3, 0.0);
  aMat(1, 1) = 3.0;
  aMat(2, 2) = 1.0;
  aMat(3, 3) = 2.0;

  auto aResult = math::Lin::Jacobi(aMat, true);

  ASSERT_TRUE(aResult.IsDone());

  const math_Vector& aEigenVals = *aResult.EigenValues;

  // Eigenvalues of diagonal matrix are the diagonal elements
  // Sorted in descending order
  EXPECT_NEAR(aEigenVals(1), 3.0, THE_TOLERANCE);
  EXPECT_NEAR(aEigenVals(2), 2.0, THE_TOLERANCE);
  EXPECT_NEAR(aEigenVals(3), 1.0, THE_TOLERANCE);
}

TEST(math_Lin_Jacobi_NewTest, Eigenvalues_Symmetric)
{
  math_Matrix aMat(1, 2, 1, 2);
  aMat(1, 1) = 3.0; aMat(1, 2) = 1.0;
  aMat(2, 1) = 1.0; aMat(2, 2) = 3.0;

  auto aResult = math::Lin::Jacobi(aMat, true);

  ASSERT_TRUE(aResult.IsDone());

  const math_Vector& aEigenVals = *aResult.EigenValues;

  // Eigenvalues are 4 and 2
  EXPECT_NEAR(aEigenVals(1), 4.0, THE_TOLERANCE);
  EXPECT_NEAR(aEigenVals(2), 2.0, THE_TOLERANCE);
}

TEST(math_Lin_Jacobi_NewTest, EigenvectorsOrthogonal)
{
  math_Matrix aMat = CreateSPD(3);

  auto aResult = math::Lin::Jacobi(aMat, false);

  ASSERT_TRUE(aResult.IsDone());

  const math_Matrix& aV = *aResult.EigenVectors;

  // Eigenvectors should be orthogonal: V^T * V = I
  math_Matrix aVtV = MatMul(Transpose(aV), aV);
  for (int i = 1; i <= 3; ++i)
  {
    for (int j = 1; j <= 3; ++j)
    {
      double aExpected = (i == j) ? 1.0 : 0.0;
      EXPECT_NEAR(aVtV(i, j), aExpected, 1.0e-8);
    }
  }
}

TEST(math_Lin_Jacobi_NewTest, SpectralDecomposition)
{
  math_Matrix aMat = CreateSPD(3);

  auto aResult = math::Lin::SpectralDecomposition(aMat);

  ASSERT_TRUE(aResult.IsDone());

  const math_Vector& aD = *aResult.EigenValues;
  const math_Matrix& aV = *aResult.EigenVectors;

  // Reconstruct: A = V * D * V^T
  math_Matrix aDiag(1, 3, 1, 3, 0.0);
  for (int i = 1; i <= 3; ++i)
  {
    aDiag(i, i) = aD(i);
  }

  math_Matrix aVD = MatMul(aV, aDiag);
  math_Matrix aReconstructed = MatMul(aVD, Transpose(aV));

  for (int i = 1; i <= 3; ++i)
  {
    for (int j = 1; j <= 3; ++j)
    {
      EXPECT_NEAR(aReconstructed(i, j), aMat(i, j), 1.0e-8);
    }
  }
}

TEST(math_Lin_Jacobi_NewTest, MatrixSqrt)
{
  math_Matrix aMat = CreateSPD(2);

  auto aSqrt = math::Lin::MatrixSqrt(aMat);

  ASSERT_TRUE(aSqrt.has_value());

  // sqrt(A) * sqrt(A) = A
  math_Matrix aCheck = MatMul(*aSqrt, *aSqrt);

  for (int i = 1; i <= 2; ++i)
  {
    for (int j = 1; j <= 2; ++j)
    {
      EXPECT_NEAR(aCheck(i, j), aMat(i, j), 1.0e-8);
    }
  }
}

// ============================================================================
// Least squares tests
// ============================================================================

TEST(math_Lin_LeastSquares_NewTest, SquareSystem)
{
  math_Matrix aMat(1, 2, 1, 2);
  aMat(1, 1) = 3.0; aMat(1, 2) = 1.0;
  aMat(2, 1) = 1.0; aMat(2, 2) = 2.0;

  math_Vector aB(1, 2);
  aB(1) = 9.0;
  aB(2) = 8.0;

  auto aResult = math::Lin::LeastSquares(aMat, aB, math::Lin::LeastSquaresMethod::QR);

  ASSERT_TRUE(aResult.IsDone());

  // For square systems, residual should be near zero
  EXPECT_LT(*aResult.Residual, THE_TOLERANCE);
}

TEST(math_Lin_LeastSquares_NewTest, Overdetermined)
{
  // 4x2 overdetermined system
  math_Matrix aMat(1, 4, 1, 2);
  aMat(1, 1) = 1.0; aMat(1, 2) = 1.0;
  aMat(2, 1) = 1.0; aMat(2, 2) = 2.0;
  aMat(3, 1) = 1.0; aMat(3, 2) = 3.0;
  aMat(4, 1) = 1.0; aMat(4, 2) = 4.0;

  // Perfect line: y = 1 + x
  math_Vector aB(1, 4);
  aB(1) = 2.0;
  aB(2) = 3.0;
  aB(3) = 4.0;
  aB(4) = 5.0;

  auto aResult = math::Lin::LeastSquares(aMat, aB, math::Lin::LeastSquaresMethod::QR);

  ASSERT_TRUE(aResult.IsDone());

  const math_Vector& aX = *aResult.Solution;

  // Solution should be approximately [1, 1] (intercept=1, slope=1)
  EXPECT_NEAR(aX(1), 1.0, THE_TOLERANCE);
  EXPECT_NEAR(aX(2), 1.0, THE_TOLERANCE);

  // Residual should be near zero for consistent system
  EXPECT_LT(*aResult.Residual, THE_TOLERANCE);
}

TEST(math_Lin_LeastSquares_NewTest, MethodComparison)
{
  math_Matrix aMat = CreateRandom(5, 3);
  math_Vector aB(1, 5);
  for (int i = 1; i <= 5; ++i)
  {
    aB(i) = static_cast<double>(i);
  }

  auto aResultNE  = math::Lin::LeastSquares(aMat, aB, math::Lin::LeastSquaresMethod::NormalEquations);
  auto aResultQR  = math::Lin::LeastSquares(aMat, aB, math::Lin::LeastSquaresMethod::QR);
  auto aResultSVD = math::Lin::LeastSquares(aMat, aB, math::Lin::LeastSquaresMethod::SVD);

  ASSERT_TRUE(aResultNE.IsDone());
  ASSERT_TRUE(aResultQR.IsDone());
  ASSERT_TRUE(aResultSVD.IsDone());

  // All methods should give similar results
  for (int i = 1; i <= 3; ++i)
  {
    EXPECT_NEAR((*aResultNE.Solution)(i), (*aResultQR.Solution)(i), 1.0e-6);
    EXPECT_NEAR((*aResultQR.Solution)(i), (*aResultSVD.Solution)(i), 1.0e-6);
  }
}

TEST(math_Lin_LeastSquares_NewTest, WeightedLeastSquares)
{
  math_Matrix aMat(1, 3, 1, 2);
  aMat(1, 1) = 1.0; aMat(1, 2) = 1.0;
  aMat(2, 1) = 1.0; aMat(2, 2) = 2.0;
  aMat(3, 1) = 1.0; aMat(3, 2) = 3.0;

  math_Vector aB(1, 3);
  aB(1) = 2.0;
  aB(2) = 3.0;
  aB(3) = 4.5; // Slightly off from the line

  // Equal weights
  math_Vector aW1(1, 3, 1.0);
  auto aResult1 = math::Lin::WeightedLeastSquares(aMat, aB, aW1);

  // Higher weight on first two points
  math_Vector aW2(1, 3);
  aW2(1) = 10.0;
  aW2(2) = 10.0;
  aW2(3) = 0.1;
  auto aResult2 = math::Lin::WeightedLeastSquares(aMat, aB, aW2);

  ASSERT_TRUE(aResult1.IsDone());
  ASSERT_TRUE(aResult2.IsDone());

  // Weighted solution should fit first two points better
  // (Different weights should give different solutions)
  EXPECT_NE((*aResult1.Solution)(1), (*aResult2.Solution)(1));
}

TEST(math_Lin_LeastSquares_NewTest, RegularizedLeastSquares)
{
  // Ill-conditioned system
  math_Matrix aMat(1, 3, 1, 3);
  for (int i = 1; i <= 3; ++i)
  {
    for (int j = 1; j <= 3; ++j)
    {
      aMat(i, j) = 1.0 / (i + j - 1); // Hilbert matrix
    }
  }

  math_Vector aB(1, 3);
  aB(1) = 1.0;
  aB(2) = 0.5;
  aB(3) = 0.333;

  auto aResultNoReg = math::Lin::LeastSquares(aMat, aB);
  auto aResultReg   = math::Lin::RegularizedLeastSquares(aMat, aB, 0.01);

  ASSERT_TRUE(aResultNoReg.IsDone());
  ASSERT_TRUE(aResultReg.IsDone());

  // Regularized solution should have smaller norm
  double aNormNoReg = VectorNorm(*aResultNoReg.Solution);
  double aNormReg   = VectorNorm(*aResultReg.Solution);
  EXPECT_LT(aNormReg, aNormNoReg);
}

// ============================================================================
// Comparison with old API tests
// ============================================================================

TEST(math_Lin_NewTest, CompareWithOldAPI_SVD)
{
  math_Matrix aMat(1, 3, 1, 3);
  aMat(1, 1) = 1.0; aMat(1, 2) = 2.0; aMat(1, 3) = 3.0;
  aMat(2, 1) = 4.0; aMat(2, 2) = 5.0; aMat(2, 3) = 6.0;
  aMat(3, 1) = 7.0; aMat(3, 2) = 8.0; aMat(3, 3) = 10.0;

  math_Vector aB(1, 3);
  aB(1) = 1.0;
  aB(2) = 2.0;
  aB(3) = 3.0;

  // Old API
  math_SVD anOldSVD(aMat);
  math_Vector anOldSol(1, 3);
  anOldSVD.Solve(aB, anOldSol);

  // New API
  auto aNewResult = math::Lin::SolveSVD(aMat, aB);

  ASSERT_TRUE(anOldSVD.IsDone());
  ASSERT_TRUE(aNewResult.IsDone());

  // Solutions should match
  for (int i = 1; i <= 3; ++i)
  {
    EXPECT_NEAR(anOldSol(i), (*aNewResult.Solution)(i), 1.0e-8);
  }
}

TEST(math_Lin_NewTest, CompareWithOldAPI_Householder)
{
  math_Matrix aMat(1, 3, 1, 2);
  aMat(1, 1) = 1.0; aMat(1, 2) = 1.0;
  aMat(2, 1) = 1.0; aMat(2, 2) = 2.0;
  aMat(3, 1) = 1.0; aMat(3, 2) = 3.0;

  math_Vector aB(1, 3);
  aB(1) = 2.0;
  aB(2) = 3.0;
  aB(3) = 4.0;

  // Old API
  math_Matrix aBMat(1, 3, 1, 1);
  aBMat(1, 1) = aB(1);
  aBMat(2, 1) = aB(2);
  aBMat(3, 1) = aB(3);
  math_Householder anOldHH(aMat, aBMat);

  // New API
  auto aNewResult = math::Lin::SolveQR(aMat, aB);

  ASSERT_TRUE(anOldHH.IsDone());
  ASSERT_TRUE(aNewResult.IsDone());

  // Get the old solution using the proper API
  math_Vector anOldSol(1, 2);
  anOldHH.Value(anOldSol, 1);

  // Solutions should match
  for (int i = 1; i <= 2; ++i)
  {
    EXPECT_NEAR(anOldSol(i), (*aNewResult.Solution)(i), 1.0e-8);
  }
}

TEST(math_Lin_NewTest, CompareWithOldAPI_Jacobi)
{
  math_Matrix aMat(1, 3, 1, 3);
  aMat(1, 1) = 3.0; aMat(1, 2) = 1.0; aMat(1, 3) = 0.0;
  aMat(2, 1) = 1.0; aMat(2, 2) = 3.0; aMat(2, 3) = 1.0;
  aMat(3, 1) = 0.0; aMat(3, 2) = 1.0; aMat(3, 3) = 3.0;

  // Old API
  math_Jacobi anOldJacobi(aMat);

  // New API
  auto aNewResult = math::Lin::Jacobi(aMat, true);

  ASSERT_TRUE(anOldJacobi.IsDone());
  ASSERT_TRUE(aNewResult.IsDone());

  // Eigenvalues should match (both sorted descending)
  const math_Vector& aNewEig = *aNewResult.EigenValues;
  for (int i = 1; i <= 3; ++i)
  {
    EXPECT_NEAR(anOldJacobi.Value(i), aNewEig(i), 1.0e-8);
  }
}

TEST(math_Lin_NewTest, CompareWithOldAPI_GaussLeastSquare)
{
  math_Matrix aMat(1, 4, 1, 2);
  aMat(1, 1) = 1.0; aMat(1, 2) = 1.0;
  aMat(2, 1) = 1.0; aMat(2, 2) = 2.0;
  aMat(3, 1) = 1.0; aMat(3, 2) = 3.0;
  aMat(4, 1) = 1.0; aMat(4, 2) = 4.0;

  math_Vector aB(1, 4);
  aB(1) = 2.0;
  aB(2) = 3.0;
  aB(3) = 4.0;
  aB(4) = 5.0;

  // Old API
  math_GaussLeastSquare anOldLS(aMat);
  math_Vector anOldSol(1, 2);
  anOldLS.Solve(aB, anOldSol);

  // New API
  auto aNewResult = math::Lin::LeastSquares(aMat, aB);

  ASSERT_TRUE(anOldLS.IsDone());
  ASSERT_TRUE(aNewResult.IsDone());

  // Solutions should match
  for (int i = 1; i <= 2; ++i)
  {
    EXPECT_NEAR(anOldSol(i), (*aNewResult.Solution)(i), 1.0e-8);
  }
}
