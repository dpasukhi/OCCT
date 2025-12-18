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

#include <math_Poly.hxx>

#include <cmath>

namespace
{
  constexpr double THE_TOLERANCE = 1.0e-10;

  //! Helper to verify a root satisfies the polynomial equation.
  double EvalCubic(double theA, double theB, double theC, double theD, double theX)
  {
    return theA * theX * theX * theX + theB * theX * theX + theC * theX + theD;
  }

  double EvalQuartic(double theA, double theB, double theC, double theD, double theE, double theX)
  {
    const double aX2 = theX * theX;
    return theA * aX2 * aX2 + theB * aX2 * theX + theC * aX2 + theD * theX + theE;
  }
}

// ============================================================================
// Linear equation tests
// ============================================================================

TEST(math_Poly_LinearTest, SimpleLinear)
{
  // 2x + 4 = 0 -> x = -2
  math::PolyResult aResult = math::Poly::Linear(2.0, 4.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 1);
  EXPECT_NEAR(aResult.Roots[0], -2.0, THE_TOLERANCE);
}

TEST(math_Poly_LinearTest, ZeroCoefficient_InfiniteSolutions)
{
  // 0*x + 0 = 0 -> infinite solutions
  math::PolyResult aResult = math::Poly::Linear(0.0, 0.0);
  EXPECT_EQ(aResult.Status, math::Status::InfiniteSolutions);
}

TEST(math_Poly_LinearTest, ZeroCoefficient_NoSolution)
{
  // 0*x + 5 = 0 -> no solution
  math::PolyResult aResult = math::Poly::Linear(0.0, 5.0);
  EXPECT_EQ(aResult.Status, math::Status::NoSolution);
}

// ============================================================================
// Quadratic equation tests
// ============================================================================

TEST(math_Poly_QuadraticTest, TwoDistinctRoots)
{
  // x^2 - 5x + 6 = 0 -> roots: 2, 3
  math::PolyResult aResult = math::Poly::Quadratic(1.0, -5.0, 6.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 2);
  EXPECT_NEAR(aResult.Roots[0], 2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 3.0, THE_TOLERANCE);
}

TEST(math_Poly_QuadraticTest, DoubleRoot)
{
  // x^2 - 4x + 4 = 0 -> root: 2 (double)
  math::PolyResult aResult = math::Poly::Quadratic(1.0, -4.0, 4.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 1);
  EXPECT_NEAR(aResult.Roots[0], 2.0, THE_TOLERANCE);
}

TEST(math_Poly_QuadraticTest, NoRealRoots)
{
  // x^2 + 1 = 0 -> no real roots
  math::PolyResult aResult = math::Poly::Quadratic(1.0, 0.0, 1.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 0);
}

TEST(math_Poly_QuadraticTest, NegativeRoots)
{
  // x^2 + 5x + 6 = 0 -> roots: -3, -2
  math::PolyResult aResult = math::Poly::Quadratic(1.0, 5.0, 6.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 2);
  EXPECT_NEAR(aResult.Roots[0], -3.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], -2.0, THE_TOLERANCE);
}

TEST(math_Poly_QuadraticTest, MixedSignRoots)
{
  // x^2 - 1 = 0 -> roots: -1, 1
  math::PolyResult aResult = math::Poly::Quadratic(1.0, 0.0, -1.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 2);
  EXPECT_NEAR(aResult.Roots[0], -1.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 1.0, THE_TOLERANCE);
}

TEST(math_Poly_QuadraticTest, ReducesToLinear)
{
  // 0*x^2 + 2x + 4 = 0 -> x = -2
  math::PolyResult aResult = math::Poly::Quadratic(0.0, 2.0, 4.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 1);
  EXPECT_NEAR(aResult.Roots[0], -2.0, THE_TOLERANCE);
}

TEST(math_Poly_QuadraticTest, LargeCoefficients)
{
  // 1e6*x^2 - 2e6*x + 1e6 = 0 -> root: 1 (double)
  math::PolyResult aResult = math::Poly::Quadratic(1.0e6, -2.0e6, 1.0e6);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 1);
  EXPECT_NEAR(aResult.Roots[0], 1.0, THE_TOLERANCE);
}

TEST(math_Poly_QuadraticTest, SmallCoefficients)
{
  // 1e-6*x^2 - 5e-6*x + 6e-6 = 0 -> roots: 2, 3
  math::PolyResult aResult = math::Poly::Quadratic(1.0e-6, -5.0e-6, 6.0e-6);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 2);
  EXPECT_NEAR(aResult.Roots[0], 2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 3.0, THE_TOLERANCE);
}

TEST(math_Poly_QuadraticTest, RootsAreSorted)
{
  // 2x^2 + 3x - 2 = 0 -> roots: -2, 0.5
  math::PolyResult aResult = math::Poly::Quadratic(2.0, 3.0, -2.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 2);
  EXPECT_LT(aResult.Roots[0], aResult.Roots[1]);
}

// ============================================================================
// Cubic equation tests
// ============================================================================

TEST(math_Poly_CubicTest, ThreeDistinctRoots)
{
  // x^3 - 6x^2 + 11x - 6 = 0 -> roots: 1, 2, 3
  math::PolyResult aResult = math::Poly::Cubic(1.0, -6.0, 11.0, -6.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 3);
  EXPECT_NEAR(aResult.Roots[0], 1.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[2], 3.0, THE_TOLERANCE);
}

TEST(math_Poly_CubicTest, OneRealRoot)
{
  // x^3 + x + 1 = 0 -> one real root approximately -0.6824
  math::PolyResult aResult = math::Poly::Cubic(1.0, 0.0, 1.0, 1.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 1);
  // Verify the root satisfies the equation
  double aValue = EvalCubic(1.0, 0.0, 1.0, 1.0, aResult.Roots[0]);
  EXPECT_NEAR(aValue, 0.0, THE_TOLERANCE);
}

TEST(math_Poly_CubicTest, TripleRoot)
{
  // x^3 - 3x^2 + 3x - 1 = 0 -> root: 1 (triple)
  // This is (x-1)^3
  math::PolyResult aResult = math::Poly::Cubic(1.0, -3.0, 3.0, -1.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_GE(aResult.NbRoots, 1);
  EXPECT_NEAR(aResult.Roots[0], 1.0, THE_TOLERANCE);
}

TEST(math_Poly_CubicTest, OneSimpleOneDouble)
{
  // x^3 - 5x^2 + 8x - 4 = 0 -> roots: 1, 2 (double)
  // This is (x-1)(x-2)^2
  math::PolyResult aResult = math::Poly::Cubic(1.0, -5.0, 8.0, -4.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_GE(aResult.NbRoots, 2);
  // Verify roots satisfy the equation
  for (int i = 0; i < aResult.NbRoots; ++i)
  {
    double aValue = EvalCubic(1.0, -5.0, 8.0, -4.0, aResult.Roots[i]);
    EXPECT_NEAR(aValue, 0.0, THE_TOLERANCE);
  }
}

TEST(math_Poly_CubicTest, ReducesToQuadratic)
{
  // 0*x^3 + x^2 - 5x + 6 = 0 -> roots: 2, 3
  math::PolyResult aResult = math::Poly::Cubic(0.0, 1.0, -5.0, 6.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 2);
  EXPECT_NEAR(aResult.Roots[0], 2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 3.0, THE_TOLERANCE);
}

TEST(math_Poly_CubicTest, NegativeRoots)
{
  // x^3 + 6x^2 + 11x + 6 = 0 -> roots: -3, -2, -1
  math::PolyResult aResult = math::Poly::Cubic(1.0, 6.0, 11.0, 6.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 3);
  EXPECT_NEAR(aResult.Roots[0], -3.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], -2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[2], -1.0, THE_TOLERANCE);
}

TEST(math_Poly_CubicTest, DepressedCubic)
{
  // x^3 - 7x + 6 = 0 -> roots: -3, 1, 2
  math::PolyResult aResult = math::Poly::Cubic(1.0, 0.0, -7.0, 6.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 3);
  EXPECT_NEAR(aResult.Roots[0], -3.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 1.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[2], 2.0, THE_TOLERANCE);
}

TEST(math_Poly_CubicTest, RootsAreSorted)
{
  math::PolyResult aResult = math::Poly::Cubic(1.0, -6.0, 11.0, -6.0);
  ASSERT_TRUE(aResult.IsDone());
  for (int i = 1; i < aResult.NbRoots; ++i)
  {
    EXPECT_LE(aResult.Roots[i - 1], aResult.Roots[i]);
  }
}

// ============================================================================
// Quartic equation tests
// ============================================================================

TEST(math_Poly_QuarticTest, FourDistinctRoots)
{
  // (x-1)(x-2)(x-3)(x-4) = x^4 - 10x^3 + 35x^2 - 50x + 24 = 0
  math::PolyResult aResult = math::Poly::Quartic(1.0, -10.0, 35.0, -50.0, 24.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 4);
  EXPECT_NEAR(aResult.Roots[0], 1.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[2], 3.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[3], 4.0, THE_TOLERANCE);
}

TEST(math_Poly_QuarticTest, TwoRealRoots)
{
  // x^4 - 5x^2 + 4 = 0 -> roots: -2, -1, 1, 2
  math::PolyResult aResult = math::Poly::Quartic(1.0, 0.0, -5.0, 0.0, 4.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 4);
  EXPECT_NEAR(aResult.Roots[0], -2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], -1.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[2], 1.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[3], 2.0, THE_TOLERANCE);
}

TEST(math_Poly_QuarticTest, NoRealRoots)
{
  // x^4 + 1 = 0 -> no real roots
  math::PolyResult aResult = math::Poly::Quartic(1.0, 0.0, 0.0, 0.0, 1.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 0);
}

TEST(math_Poly_QuarticTest, Biquadratic)
{
  // x^4 - 5x^2 + 4 = 0 is biquadratic (no x^3 or x term)
  math::PolyResult aResult = math::Poly::Quartic(1.0, 0.0, -5.0, 0.0, 4.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 4);
  for (int i = 0; i < aResult.NbRoots; ++i)
  {
    double aValue = EvalQuartic(1.0, 0.0, -5.0, 0.0, 4.0, aResult.Roots[i]);
    EXPECT_NEAR(aValue, 0.0, THE_TOLERANCE);
  }
}

TEST(math_Poly_QuarticTest, ReducesToCubic)
{
  // 0*x^4 + x^3 - 6x^2 + 11x - 6 = 0 -> roots: 1, 2, 3
  math::PolyResult aResult = math::Poly::Quartic(0.0, 1.0, -6.0, 11.0, -6.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots, 3);
  EXPECT_NEAR(aResult.Roots[0], 1.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[2], 3.0, THE_TOLERANCE);
}

TEST(math_Poly_QuarticTest, QuadrupleRoot)
{
  // (x-2)^4 = x^4 - 8x^3 + 24x^2 - 32x + 16 = 0
  math::PolyResult aResult = math::Poly::Quartic(1.0, -8.0, 24.0, -32.0, 16.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_GE(aResult.NbRoots, 1);
  EXPECT_NEAR(aResult.Roots[0], 2.0, THE_TOLERANCE);
}

TEST(math_Poly_QuarticTest, TwoDoubleRoots)
{
  // (x-1)^2 * (x-3)^2 = x^4 - 8x^3 + 22x^2 - 24x + 9 = 0
  math::PolyResult aResult = math::Poly::Quartic(1.0, -8.0, 22.0, -24.0, 9.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_GE(aResult.NbRoots, 2);
  // Verify roots satisfy equation
  for (int i = 0; i < aResult.NbRoots; ++i)
  {
    double aValue = EvalQuartic(1.0, -8.0, 22.0, -24.0, 9.0, aResult.Roots[i]);
    EXPECT_NEAR(aValue, 0.0, THE_TOLERANCE);
  }
}

TEST(math_Poly_QuarticTest, RootsAreSorted)
{
  math::PolyResult aResult = math::Poly::Quartic(1.0, -10.0, 35.0, -50.0, 24.0);
  ASSERT_TRUE(aResult.IsDone());
  for (int i = 1; i < aResult.NbRoots; ++i)
  {
    EXPECT_LE(aResult.Roots[i - 1], aResult.Roots[i]);
  }
}

TEST(math_Poly_QuarticTest, VerifyRootsSatisfyEquation)
{
  // General quartic with all roots
  math::PolyResult aResult = math::Poly::Quartic(1.0, -10.0, 35.0, -50.0, 24.0);
  ASSERT_TRUE(aResult.IsDone());
  for (int i = 0; i < aResult.NbRoots; ++i)
  {
    double aValue = EvalQuartic(1.0, -10.0, 35.0, -50.0, 24.0, aResult.Roots[i]);
    EXPECT_NEAR(aValue, 0.0, THE_TOLERANCE);
  }
}

// ============================================================================
// Boolean conversion tests
// ============================================================================

TEST(math_Poly_BoolConversionTest, SuccessfulResultIsTrue)
{
  math::PolyResult aResult = math::Poly::Quadratic(1.0, -5.0, 6.0);
  EXPECT_TRUE(static_cast<bool>(aResult));
}

TEST(math_Poly_BoolConversionTest, NoSolutionResultIsFalse)
{
  math::PolyResult aResult = math::Poly::Linear(0.0, 5.0);
  EXPECT_FALSE(static_cast<bool>(aResult));
}

// ============================================================================
// Indexing operator tests
// ============================================================================

TEST(math_Poly_IndexingTest, BracketOperator)
{
  math::PolyResult aResult = math::Poly::Quadratic(1.0, -5.0, 6.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult[0], aResult.Roots[0]);
  EXPECT_EQ(aResult[1], aResult.Roots[1]);
}
