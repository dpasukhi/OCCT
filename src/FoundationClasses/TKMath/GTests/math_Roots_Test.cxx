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

#include <math_Roots.hxx>

#include <cmath>

namespace
{
  constexpr double THE_TOLERANCE = 1.0e-10;
  constexpr double THE_PI = 3.14159265358979323846;

  //! Function with derivative: f(x) = x^2 - 2, f'(x) = 2x
  //! Root at x = sqrt(2) ≈ 1.41421356...
  class SqrtTwoFunc
  {
  public:
    bool Value(double theX, double& theF) const
    {
      theF = theX * theX - 2.0;
      return true;
    }

    bool Values(double theX, double& theF, double& theDf) const
    {
      theF = theX * theX - 2.0;
      theDf = 2.0 * theX;
      return true;
    }
  };

  //! Function: f(x) = cos(x) - x
  //! Root at x ≈ 0.739085...
  class CosMinusXFunc
  {
  public:
    bool Value(double theX, double& theF) const
    {
      theF = std::cos(theX) - theX;
      return true;
    }

    bool Values(double theX, double& theF, double& theDf) const
    {
      theF = std::cos(theX) - theX;
      theDf = -std::sin(theX) - 1.0;
      return true;
    }
  };

  //! Function: f(x) = x^3 - x - 2
  //! Root at x ≈ 1.5214...
  class CubicFunc
  {
  public:
    bool Value(double theX, double& theF) const
    {
      theF = theX * theX * theX - theX - 2.0;
      return true;
    }

    bool Values(double theX, double& theF, double& theDf) const
    {
      theF = theX * theX * theX - theX - 2.0;
      theDf = 3.0 * theX * theX - 1.0;
      return true;
    }
  };

  //! Function: f(x) = sin(x)
  //! Roots at x = n*PI for integer n
  class SinFunc
  {
  public:
    bool Value(double theX, double& theF) const
    {
      theF = std::sin(theX);
      return true;
    }

    bool Values(double theX, double& theF, double& theDf) const
    {
      theF = std::sin(theX);
      theDf = std::cos(theX);
      return true;
    }
  };

  //! Function: f(x) = e^x - 3
  //! Root at x = ln(3) ≈ 1.0986...
  class ExpMinusThreeFunc
  {
  public:
    bool Value(double theX, double& theF) const
    {
      theF = std::exp(theX) - 3.0;
      return true;
    }

    bool Values(double theX, double& theF, double& theDf) const
    {
      theF = std::exp(theX) - 3.0;
      theDf = std::exp(theX);
      return true;
    }
  };

  //! Linear function: f(x) = 2x - 4
  //! Root at x = 2
  class LinearFunc
  {
  public:
    bool Value(double theX, double& theF) const
    {
      theF = 2.0 * theX - 4.0;
      return true;
    }

    bool Values(double theX, double& theF, double& theDf) const
    {
      theF = 2.0 * theX - 4.0;
      theDf = 2.0;
      return true;
    }
  };
}

// ============================================================================
// Newton method tests
// ============================================================================

TEST(math_Roots_NewtonTest, SqrtTwo)
{
  SqrtTwoFunc aFunc;
  math::ScalarResult aResult = math::Roots::Newton(aFunc, 1.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, std::sqrt(2.0), THE_TOLERANCE);
  EXPECT_NEAR(*aResult.Value, 0.0, THE_TOLERANCE);
}

TEST(math_Roots_NewtonTest, CosMinusX)
{
  CosMinusXFunc aFunc;
  math::ScalarResult aResult = math::Roots::Newton(aFunc, 0.5);
  ASSERT_TRUE(aResult.IsDone());
  // Verify root satisfies equation
  double aFx = std::cos(*aResult.Root) - *aResult.Root;
  EXPECT_NEAR(aFx, 0.0, THE_TOLERANCE);
}

TEST(math_Roots_NewtonTest, CubicEquation)
{
  CubicFunc aFunc;
  math::ScalarResult aResult = math::Roots::Newton(aFunc, 1.5);
  ASSERT_TRUE(aResult.IsDone());
  // Verify root
  double aFx = *aResult.Root * *aResult.Root * *aResult.Root - *aResult.Root - 2.0;
  EXPECT_NEAR(aFx, 0.0, THE_TOLERANCE);
}

TEST(math_Roots_NewtonTest, LinearFunction)
{
  LinearFunc aFunc;
  math::ScalarResult aResult = math::Roots::Newton(aFunc, 0.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, 2.0, THE_TOLERANCE);
}

TEST(math_Roots_NewtonTest, ExpFunction)
{
  ExpMinusThreeFunc aFunc;
  math::ScalarResult aResult = math::Roots::Newton(aFunc, 1.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, std::log(3.0), THE_TOLERANCE);
}

TEST(math_Roots_NewtonTest, CustomTolerance)
{
  SqrtTwoFunc aFunc;
  math::Config aConfig;
  aConfig.XTolerance = 1.0e-14;
  aConfig.FTolerance = 1.0e-14;
  aConfig.MaxIterations = 100;

  math::ScalarResult aResult = math::Roots::Newton(aFunc, 1.0, aConfig);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, std::sqrt(2.0), 1.0e-14);
}

// ============================================================================
// Newton bounded tests
// ============================================================================

TEST(math_Roots_NewtonBoundedTest, SqrtTwoWithBounds)
{
  SqrtTwoFunc aFunc;
  math::ScalarResult aResult = math::Roots::NewtonBounded(aFunc, 1.5, 1.0, 2.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, std::sqrt(2.0), THE_TOLERANCE);
}

TEST(math_Roots_NewtonBoundedTest, SinWithBounds)
{
  SinFunc aFunc;
  math::ScalarResult aResult = math::Roots::NewtonBounded(aFunc, 3.0, 2.0, 4.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, THE_PI, THE_TOLERANCE);
}

// ============================================================================
// Secant method tests
// ============================================================================

TEST(math_Roots_SecantTest, SqrtTwo)
{
  SqrtTwoFunc aFunc;
  math::ScalarResult aResult = math::Roots::Secant(aFunc, 1.0, 2.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, std::sqrt(2.0), THE_TOLERANCE);
}

TEST(math_Roots_SecantTest, CosMinusX)
{
  CosMinusXFunc aFunc;
  math::ScalarResult aResult = math::Roots::Secant(aFunc, 0.0, 1.0);
  ASSERT_TRUE(aResult.IsDone());
  double aFx = std::cos(*aResult.Root) - *aResult.Root;
  EXPECT_NEAR(aFx, 0.0, THE_TOLERANCE);
}

TEST(math_Roots_SecantTest, ExpFunction)
{
  ExpMinusThreeFunc aFunc;
  math::ScalarResult aResult = math::Roots::Secant(aFunc, 0.0, 2.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, std::log(3.0), THE_TOLERANCE);
}

// ============================================================================
// Brent method tests
// ============================================================================

TEST(math_Roots_BrentTest, SqrtTwo)
{
  SqrtTwoFunc aFunc;
  math::ScalarResult aResult = math::Roots::Brent(aFunc, 1.0, 2.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, std::sqrt(2.0), THE_TOLERANCE);
}

TEST(math_Roots_BrentTest, CosMinusX)
{
  CosMinusXFunc aFunc;
  math::ScalarResult aResult = math::Roots::Brent(aFunc, 0.0, 1.0);
  ASSERT_TRUE(aResult.IsDone());
  double aFx = std::cos(*aResult.Root) - *aResult.Root;
  EXPECT_NEAR(aFx, 0.0, THE_TOLERANCE);
}

TEST(math_Roots_BrentTest, CubicEquation)
{
  CubicFunc aFunc;
  math::ScalarResult aResult = math::Roots::Brent(aFunc, 1.0, 2.0);
  ASSERT_TRUE(aResult.IsDone());
  double aFx = *aResult.Root * *aResult.Root * *aResult.Root - *aResult.Root - 2.0;
  EXPECT_NEAR(aFx, 0.0, THE_TOLERANCE);
}

TEST(math_Roots_BrentTest, SinPi)
{
  SinFunc aFunc;
  math::ScalarResult aResult = math::Roots::Brent(aFunc, 2.0, 4.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, THE_PI, THE_TOLERANCE);
}

TEST(math_Roots_BrentTest, ExpFunction)
{
  ExpMinusThreeFunc aFunc;
  math::ScalarResult aResult = math::Roots::Brent(aFunc, 0.0, 2.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, std::log(3.0), THE_TOLERANCE);
}

TEST(math_Roots_BrentTest, InvalidBracket)
{
  SqrtTwoFunc aFunc;
  // Both endpoints positive - no sign change
  math::ScalarResult aResult = math::Roots::Brent(aFunc, 2.0, 3.0);
  EXPECT_EQ(aResult.Status, math::Status::InvalidInput);
}

TEST(math_Roots_BrentTest, ReversedBracket)
{
  SqrtTwoFunc aFunc;
  // Reversed bracket (upper < lower)
  math::ScalarResult aResult = math::Roots::Brent(aFunc, 2.0, 1.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, std::sqrt(2.0), THE_TOLERANCE);
}

// ============================================================================
// Bisection method tests
// ============================================================================

TEST(math_Roots_BisectionTest, SqrtTwo)
{
  SqrtTwoFunc aFunc;
  math::ScalarResult aResult = math::Roots::Bisection(aFunc, 1.0, 2.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, std::sqrt(2.0), THE_TOLERANCE);
}

TEST(math_Roots_BisectionTest, CosMinusX)
{
  CosMinusXFunc aFunc;
  math::ScalarResult aResult = math::Roots::Bisection(aFunc, 0.0, 1.0);
  ASSERT_TRUE(aResult.IsDone());
  double aFx = std::cos(*aResult.Root) - *aResult.Root;
  EXPECT_NEAR(aFx, 0.0, THE_TOLERANCE);
}

TEST(math_Roots_BisectionTest, SinPi)
{
  SinFunc aFunc;
  math::ScalarResult aResult = math::Roots::Bisection(aFunc, 2.0, 4.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, THE_PI, THE_TOLERANCE);
}

TEST(math_Roots_BisectionTest, InvalidBracket)
{
  SqrtTwoFunc aFunc;
  // No sign change
  math::ScalarResult aResult = math::Roots::Bisection(aFunc, 2.0, 3.0);
  EXPECT_EQ(aResult.Status, math::Status::InvalidInput);
}

// ============================================================================
// Bisection-Newton hybrid tests
// ============================================================================

TEST(math_Roots_BisectionNewtonTest, SqrtTwo)
{
  SqrtTwoFunc aFunc;
  math::ScalarResult aResult = math::Roots::BisectionNewton(aFunc, 1.0, 2.0);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Root, std::sqrt(2.0), THE_TOLERANCE);
}

TEST(math_Roots_BisectionNewtonTest, CosMinusX)
{
  CosMinusXFunc aFunc;
  math::ScalarResult aResult = math::Roots::BisectionNewton(aFunc, 0.0, 1.0);
  ASSERT_TRUE(aResult.IsDone());
  double aFx = std::cos(*aResult.Root) - *aResult.Root;
  EXPECT_NEAR(aFx, 0.0, THE_TOLERANCE);
}

TEST(math_Roots_BisectionNewtonTest, FasterThanPureBisection)
{
  SqrtTwoFunc aFunc;
  math::Config aConfig;
  aConfig.MaxIterations = 100;

  math::ScalarResult aBisec = math::Roots::Bisection(aFunc, 1.0, 2.0, aConfig);
  math::ScalarResult aHybrid = math::Roots::BisectionNewton(aFunc, 1.0, 2.0, aConfig);

  ASSERT_TRUE(aBisec.IsDone());
  ASSERT_TRUE(aHybrid.IsDone());

  // Hybrid should converge in fewer iterations
  EXPECT_LE(aHybrid.NbIterations, aBisec.NbIterations);
}

// ============================================================================
// Convergence and iteration tests
// ============================================================================

TEST(math_Roots_ConvergenceTest, NewtonIterationCount)
{
  SqrtTwoFunc aFunc;
  math::ScalarResult aResult = math::Roots::Newton(aFunc, 1.0);
  ASSERT_TRUE(aResult.IsDone());
  // Newton should converge quickly (typically < 10 iterations)
  EXPECT_LT(aResult.NbIterations, 15);
}

TEST(math_Roots_ConvergenceTest, BrentIterationCount)
{
  SqrtTwoFunc aFunc;
  math::ScalarResult aResult = math::Roots::Brent(aFunc, 1.0, 2.0);
  ASSERT_TRUE(aResult.IsDone());
  // Brent should converge reasonably fast
  EXPECT_LT(aResult.NbIterations, 50);
}

// ============================================================================
// Boolean conversion tests
// ============================================================================

TEST(math_Roots_BoolConversionTest, SuccessfulResultIsTrue)
{
  SqrtTwoFunc aFunc;
  math::ScalarResult aResult = math::Roots::Newton(aFunc, 1.0);
  EXPECT_TRUE(static_cast<bool>(aResult));
}

TEST(math_Roots_BoolConversionTest, InvalidInputIsFalse)
{
  SqrtTwoFunc aFunc;
  math::ScalarResult aResult = math::Roots::Brent(aFunc, 2.0, 3.0);
  EXPECT_FALSE(static_cast<bool>(aResult));
}
